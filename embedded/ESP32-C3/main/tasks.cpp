#include <stdio.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_http_client.h"

#include "ina231.h"
#include "pms9103m.h"
#include "sgp30.h"
#include "sound_meter.h"
#include "uv_meter.h"
#include "Adafruit_VEML7700_espidf.h"

#include "tasks.h"
#include "device_secret.h"

static const char *TAG = "sensor_task";

/* ────────── I2C 及 INA231 配置 ────────── */
#define I2C_MASTER_PORT    I2C_NUM_0
#define INA231_ADDR        INA231_ADDRESS_DEFAULT
#define INA231_MAX_CURR_A  1.0f
#define INA231_SHUNT_UOHM  100000UL

/* ────────── 上报配置 ────────── */
#define UPLOAD_URL   "http://172.20.10.3:8080/api/sensor-upload"

/* ────────── 熔断器 ────────── */
#define CB_FAIL_THRESHOLD  3
#define CB_COOLDOWN_SEC    30

/* ────────── 重试配置 ────────── */
#define INA231_INIT_MAX_RETRIES     3
#define VEML7700_INIT_MAX_RETRIES   5
#define SGP30_INIT_MAX_RETRIES      3
#define SENSOR_READ_MAX_RETRIES     2

/* ────────── 静态变量 ────────── */
static uint32_t g_ina231_current_lsb_nA = 0;
static bus_calibration_t g_bus_cal = {};
static bool g_ina231_ready = false;

static SemaphoreHandle_t s_upload_mutex = NULL;
static SemaphoreHandle_t s_i2c_mutex = NULL;

static int  s_cb_fail_count   = 0;
static int64_t s_cb_cooldown_until = 0;

/* ────────── 全局传感器对象（供 on-demand 查询复用） ────────── */
static SGP30              *g_sgp30       = nullptr;
static Adafruit_VEML7700  *g_veml7700    = nullptr;
static sound_meter_handle_t g_sound_handle = {};
static uv_meter_handle_t    g_uv_handle    = {};
static bool g_sgp30_ready    = false;
static bool g_veml7700_ready = false;
static bool g_pms_ready      = false;
static bool g_sound_ready    = false;
static bool g_uv_ready       = false;

/* ────────── device_secret 由 device_secret.c 提供（4MB 分区表需要） ────────── */

/* ═══════════════════════════════════════════════════════════
 * 熔断器逻辑
 * ═══════════════════════════════════════════════════════════ */

static bool cb_is_open(void)
{
    if (s_cb_fail_count < CB_FAIL_THRESHOLD) return false;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t now_ms = (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return (now_ms < s_cb_cooldown_until);
}

static void cb_record_failure(void)
{
    s_cb_fail_count++;
    if (s_cb_fail_count >= CB_FAIL_THRESHOLD) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        s_cb_cooldown_until = (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000
                              + (int64_t)CB_COOLDOWN_SEC * 1000;
        ESP_LOGW(TAG, "熔断器打开: 连续 %d 次失败，冷却 %d 秒",
                 s_cb_fail_count, CB_COOLDOWN_SEC);
    }
}

static void cb_record_success(void)
{
    if (s_cb_fail_count > 0) {
        ESP_LOGI(TAG, "熔断器关闭: 恢复正常上传");
    }
    s_cb_fail_count = 0;
}

/* ═══════════════════════════════════════════════════════════
 * I2C 总线扫描
 * ═══════════════════════════════════════════════════════════ */

static void i2c_scan_devices(i2c_port_t port, uint8_t start_addr, uint8_t end_addr)
{
    ESP_LOGI(TAG, "I2C 扫描 (端口 %d, 0x%02x-0x%02x)...", port, start_addr, end_addr);
    int found = 0;
    for (uint8_t addr = start_addr; addr <= end_addr; addr++) {
        if (xSemaphoreTake(s_i2c_mutex, pdMS_TO_TICKS(500)) != pdTRUE) continue;
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);
        xSemaphoreGive(s_i2c_mutex);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "  找到设备: 0x%02x", addr);
            found++;
        }
        vTaskDelay(pdMS_TO_TICKS(3));
    }
    if (found == 0) {
        ESP_LOGW(TAG, "未找到任何 I2C 设备");
    } else {
        ESP_LOGI(TAG, "共找到 %d 个 I2C 设备", found);
    }
}

/* ═══════════════════════════════════════════════════════════
 * 生成 ISO8601 时间戳
 * ═══════════════════════════════════════════════════════════ */

static int make_timestamp(char *buf, size_t buf_size)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t now = tv.tv_sec;
    struct tm tm_info;
    gmtime_r(&now, &tm_info);
    return snprintf(buf, buf_size,
                    "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
                    tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
                    tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec,
                    (int)(tv.tv_usec / 1000));
}

/* ═══════════════════════════════════════════════════════════
 * 公共初始化
 * ═══════════════════════════════════════════════════════════ */

void sensor_tasks_init(void)
{
    s_upload_mutex = xSemaphoreCreateMutex();
    s_i2c_mutex = xSemaphoreCreateMutex();
    configASSERT(s_upload_mutex);
    configASSERT(s_i2c_mutex);

    i2c_scan_devices(I2C_MASTER_PORT, 0x00, 0x7F);

    g_ina231_current_lsb_nA = ina231_current_lsb_nA(INA231_MAX_CURR_A);

    static const bus_calibration_point_t bus_cal_points[] = {
        { .measured_v = 3.115f,  .reference_v = 3.3008f },
        { .measured_v = 4.918f,  .reference_v = 5.011f  },
        { .measured_v = 11.866f, .reference_v = 12.010f },
        { .measured_v = 15.869f, .reference_v = 16.000f },
        { .measured_v = 17.887f, .reference_v = 18.009f },
    };
    g_bus_cal = ina231_bus_calibration_compute(
        bus_cal_points, sizeof(bus_cal_points) / sizeof(bus_cal_points[0]));
    ESP_LOGI(TAG, "INA231 bus cal: gain=%.6f offset=%.6f valid=%d",
             g_bus_cal.gain, g_bus_cal.offset, g_bus_cal.valid);
}

SemaphoreHandle_t sensor_get_i2c_mutex(void)
{
    return s_i2c_mutex;
}

/* ═══════════════════════════════════════════════════════════
 * 逐传感器上传（保留兼容）
 * ═══════════════════════════════════════════════════════════ */

void sensor_upload(const char *sensor_type, const char *data_type, const char *data_json)
{
    if (cb_is_open()) return;

    if (xSemaphoreTake(s_upload_mutex, pdMS_TO_TICKS(10000)) != pdTRUE) return;

    char timestamp[40];
    if (make_timestamp(timestamp, sizeof(timestamp)) <= 0) {
        xSemaphoreGive(s_upload_mutex);
        return;
    }

    char data_escaped[512];
    size_t si = 0, di = 0;
    while (data_json[si] && di + 2 < sizeof(data_escaped)) {
        char ch = data_json[si++];
        if (ch == '"' || ch == '\\') data_escaped[di++] = '\\';
        data_escaped[di++] = ch;
    }
    data_escaped[di] = '\0';

    char body[1024];
    int len = snprintf(body, sizeof(body),
        "{\"secret\":\"%s\",\"sensor_type\":\"%s\",\"data_type\":\"%s\","
        "\"data\":\"%s\",\"timestamp\":\"%s\"}",
        secret, sensor_type, data_type, data_escaped, timestamp);
    if (len <= 0 || len >= (int)sizeof(body)) {
        xSemaphoreGive(s_upload_mutex);
        return;
    }

    esp_http_client_config_t cfg = {};
    cfg.url = UPLOAD_URL;
    cfg.method = HTTP_METHOD_POST;
    cfg.timeout_ms = 3000;
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        xSemaphoreGive(s_upload_mutex);
        return;
    }
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, body, len);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        cb_record_success();
    } else {
        ESP_LOGE(TAG, "[%s/%s] 上报失败: %s", sensor_type, data_type, esp_err_to_name(err));
        cb_record_failure();
    }
    esp_http_client_cleanup(client);
    xSemaphoreGive(s_upload_mutex);
}

/* ═══════════════════════════════════════════════════════════
 * 聚合上报（一次性发送所有传感器数据）
 * ═══════════════════════════════════════════════════════════ */

void sensor_upload_aggregated(const char *aggregated_json)
{
    if (cb_is_open()) return;

    if (xSemaphoreTake(s_upload_mutex, pdMS_TO_TICKS(15000)) != pdTRUE) return;

    char timestamp[40];
    if (make_timestamp(timestamp, sizeof(timestamp)) <= 0) {
        xSemaphoreGive(s_upload_mutex);
        return;
    }

    char body[2048];
    int len = snprintf(body, sizeof(body),
        "{\"secret\":\"%s\",\"timestamp\":\"%s\",\"sensors\":%s}",
        secret, timestamp, aggregated_json);
    if (len <= 0 || len >= (int)sizeof(body)) {
        ESP_LOGE(TAG, "聚合报文过长 (%d), 跳过上报", len);
        xSemaphoreGive(s_upload_mutex);
        return;
    }

    ESP_LOGI(TAG, "聚合上报开始: body_len=%d, url=%s", len, UPLOAD_URL);

    esp_http_client_config_t cfg = {};
    cfg.url = UPLOAD_URL;
    cfg.method = HTTP_METHOD_POST;
    cfg.timeout_ms = 5000;
    cfg.buffer_size = 512;
    cfg.buffer_size_tx = 512;
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        ESP_LOGE(TAG, "聚合上报: esp_http_client_init 失败");
        xSemaphoreGive(s_upload_mutex);
        return;
    }
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, body, len);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        int64_t content_len = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "聚合上报成功 HTTP %d, content_len=%lld", status, content_len);

        /* 读取并打印响应体（调试用） */
        char resp_buf[256];
        int read_len = esp_http_client_read(client, resp_buf, sizeof(resp_buf) - 1);
        if (read_len > 0) {
            resp_buf[read_len] = '\0';
            ESP_LOGI(TAG, "聚合上报响应: %s", resp_buf);
        }

        cb_record_success();
    } else {
        int transport_type = -1;
        ESP_LOGE(TAG, "聚合上报失败: %s (errno=%d, transport_type=%d)",
                 esp_err_to_name(err), err,
                 esp_http_client_get_transport_type(client));
        cb_record_failure();
    }
    esp_http_client_cleanup(client);
    xSemaphoreGive(s_upload_mutex);
}

/* ═══════════════════════════════════════════════════════════
 * INA231 初始化（含重试）
 * ═══════════════════════════════════════════════════════════ */

static bool init_ina231(void)
{
    for (int attempt = 1; attempt <= INA231_INIT_MAX_RETRIES; ++attempt) {
        esp_err_t err = ESP_ERR_INVALID_STATE;
        if (xSemaphoreTake(s_i2c_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            err = ina231_write_calibration(I2C_MASTER_PORT, INA231_ADDR,
                                            INA231_MAX_CURR_A, INA231_SHUNT_UOHM);
            if (err == ESP_OK)
                err = ina231_set_bus_conversion(I2C_MASTER_PORT, INA231_ADDR, 3320);
            if (err == ESP_OK)
                err = ina231_set_shunt_conversion(I2C_MASTER_PORT, INA231_ADDR, 3320);
            if (err == ESP_OK)
                err = ina231_set_mode(I2C_MASTER_PORT, INA231_ADDR, INA231_MODE_CONTINUOUS_BOTH);
            if (err == ESP_OK)
                err = ina231_enable_alert_on_conversion(I2C_MASTER_PORT, INA231_ADDR, true);
            xSemaphoreGive(s_i2c_mutex);
        }

        if (err == ESP_OK) {
            if (attempt > 1) ESP_LOGI(TAG, "INA231 重试成功 (attempt=%d)", attempt);
            return true;
        }
        ESP_LOGW(TAG, "INA231 初始化失败 attempt=%d: %s", attempt, esp_err_to_name(err));
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return false;
}

/* ═══════════════════════════════════════════════════════════
 * INA231 读取
 * ═══════════════════════════════════════════════════════════ */

static esp_err_t read_ina231(char *json_out, size_t json_size)
{
    float bus_v = 0, shunt_v = 0, current_ma = 0, power_mw = 0;

    /* INA231 库不使用互斥锁，在此统一保护 */
    esp_err_t ok = ESP_ERR_INVALID_STATE;
    if (xSemaphoreTake(s_i2c_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        ok =
            ina231_read_bus_voltage_v  (I2C_MASTER_PORT, INA231_ADDR, &bus_v)     |
            ina231_read_shunt_voltage_v(I2C_MASTER_PORT, INA231_ADDR, &shunt_v)   |
            ina231_read_current_ma     (I2C_MASTER_PORT, INA231_ADDR,
                                        g_ina231_current_lsb_nA, &current_ma)     |
            ina231_read_power_mw       (I2C_MASTER_PORT, INA231_ADDR,
                                        g_ina231_current_lsb_nA, &power_mw);
        xSemaphoreGive(s_i2c_mutex);
    }
    if (ok != ESP_OK) return ok;

    float bus_v_cal = ina231_bus_voltage_calibrate(bus_v, &g_bus_cal);
    float actual_power_mw = bus_v_cal * current_ma;
    snprintf(json_out, json_size,
        "{\"bus_v\":%.4f,\"bus_v_cal\":%.4f,\"shunt_v\":%.6f,"
        "\"current_ma\":%.3f,\"power_mw\":%.3f}",
        bus_v, bus_v_cal, shunt_v, current_ma, actual_power_mw);
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════
 * SGP30 初始化（含重试）
 * ═══════════════════════════════════════════════════════════ */

static bool init_sgp30(SGP30 &sgp30)
{
    for (int attempt = 1; attempt <= SGP30_INIT_MAX_RETRIES; ++attempt) {
        if (sgp30.init() == ESP_OK) {
            if (attempt > 1) ESP_LOGI(TAG, "SGP30 重试成功 (attempt=%d)", attempt);
            return true;
        }
        ESP_LOGW(TAG, "SGP30 初始化失败 attempt=%d", attempt);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return false;
}

/* ═══════════════════════════════════════════════════════════
 * SGP30 读取
 * ═══════════════════════════════════════════════════════════ */

static esp_err_t read_sgp30(SGP30 &sgp30, char *json_out, size_t json_size)
{
    uint16_t co2 = 0, tvoc = 0;
    esp_err_t err = sgp30.measure(&co2, &tvoc);
    if (err != ESP_OK) return err;
    snprintf(json_out, json_size,
        "{\"co2_ppm\":%u,\"tvoc_ppb\":%u}", co2, tvoc);
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════
 * VEML7700 初始化（含重试）
 * ═══════════════════════════════════════════════════════════ */

static bool init_veml7700(Adafruit_VEML7700 &veml)
{
    for (int attempt = 1; attempt <= VEML7700_INIT_MAX_RETRIES; ++attempt) {
        if (veml.begin(100000)) {
            if (attempt > 1) ESP_LOGI(TAG, "VEML7700 重试成功 (attempt=%d)", attempt);
            return true;
        }
        ESP_LOGW(TAG, "VEML7700 初始化失败 attempt=%d", attempt);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return false;
}

/* ═══════════════════════════════════════════════════════════
 * VEML7700 读取
 * ═══════════════════════════════════════════════════════════ */

static esp_err_t read_veml7700(Adafruit_VEML7700 &veml, char *json_out, size_t json_size)
{
    float lux = veml.readLux(VEML_LUX_AUTO);
    uint8_t gain = veml.getGain();
    uint8_t it = veml.getIntegrationTime();
    snprintf(json_out, json_size,
        "{\"lux\":%.2f,\"gain\":%u,\"integration_time\":%u}", lux, gain, it);
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════
 * PMS9103M 初始化
 * ═══════════════════════════════════════════════════════════ */

static bool init_pms9103m(void)
{
    pms9103m_config_t cfg = {
        .uart_num  = UART_NUM_1,
        .tx_io_num = 17,
        .rx_io_num = 16,
        .baud_rate = 9600,
    };
    return (pms9103m_init(&cfg) == ESP_OK);
}

/* ═══════════════════════════════════════════════════════════
 * PMS9103M 读取
 * ═══════════════════════════════════════════════════════════ */

static esp_err_t read_pms9103m(char *json_out, size_t json_size)
{
    pms9103m_data_t d;
    esp_err_t err = pms9103m_read_frame(&d, pdMS_TO_TICKS(2000));
    if (err != ESP_OK) return err;
    snprintf(json_out, json_size,
        "{\"pm1_0_cf1\":%u,\"pm2_5_cf1\":%u,\"pm10_cf1\":%u,"
        "\"pm1_0_atm\":%u,\"pm2_5_atm\":%u,\"pm10_atm\":%u,"
        "\"cnt_0_3um\":%u,\"cnt_0_5um\":%u,\"cnt_1_0um\":%u,"
        "\"cnt_2_5um\":%u,\"cnt_5_0um\":%u,\"cnt_10um\":%u}",
        d.pm1_0_cf1, d.pm2_5_cf1, d.pm10_cf1,
        d.pm1_0_atm, d.pm2_5_atm, d.pm10_atm,
        d.count_0_3um, d.count_0_5um, d.count_1_0um,
        d.count_2_5um, d.count_5_0um, d.count_10_0um);
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════
 * Sound Meter 初始化
 * ═══════════════════════════════════════════════════════════ */

static bool init_sound_meter(sound_meter_handle_t &handle)
{
    static const sound_meter_db_calibration_point_t cal_pts[] = {
        { .voltage = 0.0f,  .db = 0.0f  },
        { .voltage = 0.5f,  .db = 40.0f },
        { .voltage = 0.62f, .db = 49.0f },
        { .voltage = 1.29f, .db = 60.0f },
        { .voltage = 2.55f, .db = 68.0f },
        { .voltage = 2.58f, .db = 74.0f },
        { .voltage = 2.60f, .db = 77.0f },
    };
    sound_meter_config_t cfg;
    sound_meter_get_default_config(&cfg);
    cfg.use_adc_calibration     = true;
    cfg.calibration_points      = cal_pts;
    cfg.calibration_point_count = sizeof(cal_pts) / sizeof(cal_pts[0]);
    return (sound_meter_init(&handle, &cfg) == ESP_OK);
}

/* ═══════════════════════════════════════════════════════════
 * Sound Meter 读取
 * ═══════════════════════════════════════════════════════════ */

static esp_err_t read_sound_meter(sound_meter_handle_t &handle, char *json_out, size_t json_size)
{
    uint32_t raw = 0;
    float voltage = 0, db = 0;
    if (sound_meter_read_raw(&handle, &raw) != ESP_OK) return ESP_FAIL;
    if (sound_meter_read_voltage(&handle, &voltage) != ESP_OK) return ESP_FAIL;
    if (sound_meter_read_db(&handle, &db) != ESP_OK) return ESP_FAIL;
    snprintf(json_out, json_size,
        "{\"raw\":%u,\"voltage_v\":%.3f,\"db\":%.1f}", (unsigned)raw, voltage, db);
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════
 * UV Meter 初始化
 * ═══════════════════════════════════════════════════════════ */

static bool init_uv_meter(uv_meter_handle_t &handle)
{
    uv_meter_config_t cfg;
    uv_meter_get_default_config(&cfg);
    cfg.channel = ADC_CHANNEL_4;
    return (uv_meter_init(&handle, &cfg) == ESP_OK);
}

/* ═══════════════════════════════════════════════════════════
 * UV Meter 读取
 * ═══════════════════════════════════════════════════════════ */

static esp_err_t read_uv_meter(uv_meter_handle_t &handle, char *json_out, size_t json_size)
{
    uint32_t raw = 0;
    float voltage = 0, uv_index = 0;
    if (uv_meter_read_raw(&handle, &raw) != ESP_OK) return ESP_FAIL;
    if (uv_meter_read_voltage(&handle, &voltage) != ESP_OK) return ESP_FAIL;
    if (uv_meter_read_uv_index(&handle, &uv_index) != ESP_OK) return ESP_FAIL;
    uv_meter_level_t level = uv_meter_get_level(uv_index);
    const char *level_s = uv_meter_level_to_string(level);
    snprintf(json_out, json_size,
        "{\"raw\":%u,\"voltage_v\":%.3f,\"uv_index\":%.2f,\"level\":\"%s\"}",
        (unsigned)raw, voltage, uv_index, level_s);
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════
 * 统一传感器数据采集任务
 *
 * 每分钟顺序采集全部 6 个传感器，聚合后一次性上报。
 * 每个传感器独立错误处理，单一传感器失败不影响其他。
 * ═══════════════════════════════════════════════════════════ */

void task_sensor_collect(void *pvParameter)
{
    (void)pvParameter;

    /* ── 阶段 1: 初始化 INA231 ── */
    g_ina231_ready = init_ina231();
    if (!g_ina231_ready) {
        ESP_LOGE(TAG, "INA231 初始化最终失败，将在后续周期重试");
    }

    /* ── 阶段 2: 初始化 SGP30 ── */
    SemaphoreHandle_t i2c_mutex = sensor_get_i2c_mutex();
    g_sgp30 = new SGP30(I2C_MASTER_PORT, GPIO_NUM_21, GPIO_NUM_20, 100000, i2c_mutex);
    if (g_sgp30) {
        g_sgp30_ready = init_sgp30(*g_sgp30);
        if (!g_sgp30_ready) ESP_LOGE(TAG, "SGP30 初始化失败");
    }

    /* ── 阶段 3: 初始化 VEML7700 ── */
    g_veml7700 = new Adafruit_VEML7700(I2C_MASTER_PORT, GPIO_NUM_21, GPIO_NUM_20,
                                       VEML7700_I2CADDR_DEFAULT, i2c_mutex);
    if (g_veml7700) {
        g_veml7700_ready = init_veml7700(*g_veml7700);
        if (!g_veml7700_ready) ESP_LOGE(TAG, "VEML7700 初始化失败");
    }

    /* ── 阶段 4: 初始化 PMS9103M ── */
    g_pms_ready = init_pms9103m();
    if (!g_pms_ready) ESP_LOGE(TAG, "PMS9103M 初始化失败");

    /* ── 阶段 5: 初始化 Sound Meter ── */
    g_sound_ready = init_sound_meter(g_sound_handle);
    if (!g_sound_ready) ESP_LOGE(TAG, "Sound Meter 初始化失败");

    /* ── 阶段 6: 初始化 UV Meter ── */
    g_uv_ready = init_uv_meter(g_uv_handle);
    if (!g_uv_ready) ESP_LOGE(TAG, "UV Meter 初始化失败");

    ESP_LOGI(TAG, "传感器就绪状态: INA231=%d SGP30=%d VEML7700=%d "
             "PMS9103M=%d Sound=%d UV=%d",
             g_ina231_ready, g_sgp30_ready, g_veml7700_ready,
             g_pms_ready, g_sound_ready, g_uv_ready);

    /* ── 主循环：每分钟采集一次 ── */
    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        ESP_LOGI(TAG, "──── 开始新一轮传感器采集 ────");

        /* 聚合缓冲区，每个传感器最多 256 字节 + 逗号分隔 */
        static char aggregated[2048];
        aggregated[0] = '{';
        aggregated[1] = '\0';
        size_t agg_len = 1;
        bool any_success = false;

        auto append_sensor = [&](const char *name, const char *json, bool success) {
            if (!success) return;
            if (any_success) {
                aggregated[agg_len++] = ',';
                aggregated[agg_len] = '\0';
            }
            int written = snprintf(aggregated + agg_len,
                                   sizeof(aggregated) - agg_len,
                                   "\"%s\":%s", name, json);
            if (written > 0 && (size_t)written < sizeof(aggregated) - agg_len) {
                agg_len += written;
            }
            any_success = true;
        };

        char sensor_json[256];
        bool ok;

        /* ── 1. INA231 ── */
        if (!g_ina231_ready) {
            g_ina231_ready = init_ina231();
        }
        if (g_ina231_ready) {
            ok = (read_ina231(sensor_json, sizeof(sensor_json)) == ESP_OK);
            if (ok) {
                append_sensor("ina231", sensor_json, true);
            } else {
                ESP_LOGW(TAG, "INA231 读取失败");
                g_ina231_ready = false;
            }
        }

        /* ── 2. SGP30 ── */
        if (!g_sgp30_ready && g_sgp30) {
            ESP_LOGI(TAG, "SGP30 未就绪，尝试重新初始化");
            g_sgp30_ready = init_sgp30(*g_sgp30);
        }
        if (g_sgp30_ready && g_sgp30) {
            ok = (read_sgp30(*g_sgp30, sensor_json, sizeof(sensor_json)) == ESP_OK);
            if (ok) {
                append_sensor("sgp30", sensor_json, true);
            } else {
                ESP_LOGW(TAG, "SGP30 读取失败，尝试软复位+重新初始化");
                /* 尝试恢复: 软复位 → init → 再读一次 */
                g_sgp30->soft_reset();
                vTaskDelay(pdMS_TO_TICKS(50));
                if (init_sgp30(*g_sgp30)) {
                    ok = (read_sgp30(*g_sgp30, sensor_json, sizeof(sensor_json)) == ESP_OK);
                    if (ok) {
                        ESP_LOGI(TAG, "SGP30 恢复成功");
                        append_sensor("sgp30", sensor_json, true);
                    } else {
                        ESP_LOGE(TAG, "SGP30 恢复后读取仍然失败");
                        g_sgp30_ready = false;
                    }
                } else {
                    ESP_LOGE(TAG, "SGP30 恢复: 重新初始化失败");
                    g_sgp30_ready = false;
                }
            }
        }

        /* ── 3. VEML7700 ── */
        if (g_veml7700_ready && g_veml7700) {
            ok = (read_veml7700(*g_veml7700, sensor_json, sizeof(sensor_json)) == ESP_OK);
            if (ok) {
                append_sensor("veml7700", sensor_json, true);
            } else {
                ESP_LOGW(TAG, "VEML7700 读取失败");
            }
        }

        /* ── 4. PMS9103M ── */
        if (g_pms_ready) {
            ok = (read_pms9103m(sensor_json, sizeof(sensor_json)) == ESP_OK);
            if (ok) {
                append_sensor("pms9103m", sensor_json, true);
            } else {
                ESP_LOGW(TAG, "PMS9103M 读取失败");
            }
        }

        /* ── 5. Sound Meter ── */
        if (g_sound_ready) {
            ok = (read_sound_meter(g_sound_handle, sensor_json, sizeof(sensor_json)) == ESP_OK);
            if (ok) {
                append_sensor("sound_meter", sensor_json, true);
            } else {
                ESP_LOGW(TAG, "Sound Meter 读取失败");
            }
        }

        /* ── 6. UV Meter ── */
        if (g_uv_ready) {
            ok = (read_uv_meter(g_uv_handle, sensor_json, sizeof(sensor_json)) == ESP_OK);
            if (ok) {
                append_sensor("uv_meter", sensor_json, true);
            } else {
                ESP_LOGW(TAG, "UV Meter 读取失败");
            }
        }

        /* 闭合 JSON 对象 */
        if (agg_len + 2 < sizeof(aggregated)) {
            aggregated[agg_len++] = '}';
            aggregated[agg_len] = '\0';
        }

        /* ── 上报 ── */
        if (any_success) {
            ESP_LOGI(TAG, "聚合数据 (len=%zu): %s", agg_len, aggregated);
            sensor_upload_aggregated(aggregated);
        } else {
            ESP_LOGW(TAG, "本轮采集无有效数据");
        }

        ESP_LOGI(TAG, "──── 本轮采集完成，等待 %d 秒 ────",
                 (int)(SENSOR_COLLECT_INTERVAL_MS / 1000));

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SENSOR_COLLECT_INTERVAL_MS));
    }
}

/* ═══════════════════════════════════════════════════════════
 * 按传感器名称读取一次数据（供 WebSocket on-demand 查询）
 *
 * @param sensor_name  "ina231" / "sgp30" / "veml7700" /
 *                     "pms9103m" / "sound_meter" / "uv_meter"
 * @param json_out     输出缓冲区（不含外层花括号的 JSON 对象）
 * @param json_size    缓冲区大小
 * @return true 表示读取成功
 * ═══════════════════════════════════════════════════════════ */

extern "C" bool sensor_read_one(const char *sensor_name, char *json_out, size_t json_size)
{
    if (!sensor_name || !json_out || !json_size) return false;

    if (strcasecmp(sensor_name, "ina231") == 0) {
        if (!g_ina231_ready) return false;
        return (read_ina231(json_out, json_size) == ESP_OK);
    }

    if (strcasecmp(sensor_name, "sgp30") == 0) {
        if (!g_sgp30_ready || !g_sgp30) return false;
        return (read_sgp30(*g_sgp30, json_out, json_size) == ESP_OK);
    }

    if (strcasecmp(sensor_name, "veml7700") == 0) {
        if (!g_veml7700_ready || !g_veml7700) return false;
        return (read_veml7700(*g_veml7700, json_out, json_size) == ESP_OK);
    }

    if (strcasecmp(sensor_name, "pms9103m") == 0) {
        if (!g_pms_ready) return false;
        return (read_pms9103m(json_out, json_size) == ESP_OK);
    }

    if (strcasecmp(sensor_name, "sound_meter") == 0) {
        if (!g_sound_ready) return false;
        return (read_sound_meter(g_sound_handle, json_out, json_size) == ESP_OK);
    }

    if (strcasecmp(sensor_name, "uv_meter") == 0) {
        if (!g_uv_ready) return false;
        return (read_uv_meter(g_uv_handle, json_out, json_size) == ESP_OK);
    }

    ESP_LOGW(TAG, "sensor_read_one: 未知传感器 '%s'", sensor_name);
    return false;
}
