#include <stdio.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_sntp.h"

#include "ina231.h"
#include "pms9103m.h"
#include "sgp30.h"
#include "sound_meter.h"
#include "uv_meter.h"
#include "Adafruit_VEML7700_espidf.h"
#include "device_secret.h"

#include "tasks.h"

static const char *TAG = "sensor_task";

/* ────────── 全局共享的 INA231 校准参数 ────────── */
#define I2C_MASTER_PORT    I2C_NUM_0
#define INA231_ADDR        INA231_ADDRESS_DEFAULT
#define INA231_MAX_CURR_A  1.0f
#define INA231_SHUNT_UOHM  100000UL

static uint32_t g_ina231_current_lsb_nA = 0;
static bus_calibration_t g_bus_cal = {};
static bool g_ina231_ready = false;

/* ────────── 上报互斥锁（防止多任务并发 HTTP 请求） ────────── */
static SemaphoreHandle_t s_upload_mutex = NULL;

/* ────────── 上报服务器配置 ────────── */
#define UPLOAD_URL   "http://172.20.10.3:8080/api/sensor-upload"

/* ────────── Keep-alive HTTP 客户端（复用 TCP 连接） ────────── */
static esp_http_client_handle_t s_http_client = NULL;

/* ────────── 采集周期 ────────── */
#define INTERVAL_INA231_MS       2000
#define INTERVAL_SGP30_MS        5000
#define INTERVAL_VEML7700_MS     5000
#define INTERVAL_PMS9103M_MS    10000
#define INTERVAL_SOUND_MS        2000
#define INTERVAL_UV_MS           2000

/* ═══════════════════════════════════════════════════════════
 * 公共初始化
 * ═══════════════════════════════════════════════════════════ */

static void http_client_init_once(void)
{
    if (s_http_client != NULL) return;
    esp_http_client_config_t cfg = {};
    cfg.url            = UPLOAD_URL;
    cfg.method         = HTTP_METHOD_POST;
    cfg.timeout_ms     = 5000;
    cfg.keep_alive_enable = true;
    cfg.keep_alive_idle    = 5;
    cfg.keep_alive_interval = 5;
    cfg.keep_alive_count   = 3;
    s_http_client = esp_http_client_init(&cfg);
    configASSERT(s_http_client);
}

static esp_err_t init_ina231_with_retry(void)
{
    for (int attempt = 1; attempt <= 3; ++attempt) {
        esp_err_t err = ina231_write_calibration(I2C_MASTER_PORT, INA231_ADDR,
                                                 INA231_MAX_CURR_A, INA231_SHUNT_UOHM);
        if (err == ESP_OK) {
            err = ina231_set_bus_conversion(I2C_MASTER_PORT, INA231_ADDR, 3320);
        }
        if (err == ESP_OK) {
            err = ina231_set_shunt_conversion(I2C_MASTER_PORT, INA231_ADDR, 3320);
        }
        if (err == ESP_OK) {
            err = ina231_set_mode(I2C_MASTER_PORT, INA231_ADDR, INA231_MODE_CONTINUOUS_BOTH);
        }
        if (err == ESP_OK) {
            err = ina231_enable_alert_on_conversion(I2C_MASTER_PORT, INA231_ADDR, true);
        }

        if (err == ESP_OK) {
            if (attempt > 1) {
                ESP_LOGI(TAG, "INA231 初始化重试成功 (attempt=%d)", attempt);
            }
            return ESP_OK;
        }

        ESP_LOGW(TAG, "INA231 初始化失败 attempt=%d: %s", attempt, esp_err_to_name(err));
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return ESP_ERR_TIMEOUT;
}

void sensor_tasks_init(void)
{
    s_upload_mutex = xSemaphoreCreateMutex();
    configASSERT(s_upload_mutex);

    /* INA231 初始化（I2C 总线由 main 在进入任务前完成） */
    g_ina231_current_lsb_nA = ina231_current_lsb_nA(INA231_MAX_CURR_A);
    g_ina231_ready = (init_ina231_with_retry() == ESP_OK);
    if (!g_ina231_ready) {
        ESP_LOGE(TAG, "INA231 初始化失败，任务将后台重试，不阻塞系统启动");
    }

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

/* ═══════════════════════════════════════════════════════════
 * 统一 JSON 上报
 * ═══════════════════════════════════════════════════════════ */

void sensor_upload(const char *sensor_type, const char *data_type, const char *data_json)
{
    /* 序列化 HTTP 请求（单次只允许一个任务占用） */
    if (xSemaphoreTake(s_upload_mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        ESP_LOGW(TAG, "[%s] 上报互斥锁超时，跳过本次", sensor_type);
        return;
    }

    http_client_init_once();

    /* 生成 ISO 8601 时间戳，带毫秒 */
    char timestamp[40];
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    time_t now_sec = tv_now.tv_sec;
    struct tm tm_info;
    gmtime_r(&now_sec, &tm_info);
    int millis = (int)(tv_now.tv_usec / 1000);
    int ts_len = snprintf(timestamp, sizeof(timestamp),
                          "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
                          tm_info.tm_year + 1900,
                          tm_info.tm_mon + 1,
                          tm_info.tm_mday,
                          tm_info.tm_hour,
                          tm_info.tm_min,
                          tm_info.tm_sec,
                          millis);
    if (ts_len <= 0 || ts_len >= (int)sizeof(timestamp)) {
        ESP_LOGE(TAG, "[%s] 时间戳生成失败，跳过上报", sensor_type);
        xSemaphoreGive(s_upload_mutex);
        return;
    }

    char data_escaped[512];
    size_t src_idx = 0;
    size_t dst_idx = 0;
    while (data_json[src_idx] != '\0' && dst_idx + 2 < sizeof(data_escaped)) {
        char ch = data_json[src_idx++];
        if (ch == '"' || ch == '\\') {
            data_escaped[dst_idx++] = '\\';
        }
        data_escaped[dst_idx++] = ch;
    }
    data_escaped[dst_idx] = '\0';
    if (data_json[src_idx] != '\0') {
        ESP_LOGE(TAG, "[%s] data 字段转义后过长，跳过上报", sensor_type);
        xSemaphoreGive(s_upload_mutex);
        return;
    }

    char body[1024];
    int body_len = snprintf(body, sizeof(body),
                            "{\"secret\":\"%s\",\"sensor_type\":\"%s\",\"data_type\":\"%s\",\"data\":\"%s\",\"timestamp\":\"%s\"}",
                            secret, sensor_type, data_type, data_escaped, timestamp);
    if (body_len <= 0 || body_len >= (int)sizeof(body)) {
        ESP_LOGE(TAG, "[%s] 上报报文过长，跳过上报", sensor_type);
        xSemaphoreGive(s_upload_mutex);
        return;
    }

    esp_http_client_set_url(s_http_client, UPLOAD_URL);
    esp_http_client_set_method(s_http_client, HTTP_METHOD_POST);
    esp_http_client_set_header(s_http_client, "Content-Type", "application/json");
    esp_http_client_set_header(s_http_client, "Connection", "keep-alive");
    esp_http_client_set_post_field(s_http_client, body, body_len);

    esp_err_t err = esp_http_client_perform(s_http_client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(s_http_client);
        ESP_LOGI(TAG, "[%s/%s] 上报成功 HTTP %d", sensor_type, data_type, status);
    } else {
        ESP_LOGE(TAG, "[%s/%s] 上报失败: %s", sensor_type, data_type, esp_err_to_name(err));
        /* 连接失败时关闭重建，下次自动重连 */
        esp_http_client_cleanup(s_http_client);
        s_http_client = NULL;
    }

    xSemaphoreGive(s_upload_mutex);
}

/* ═══════════════════════════════════════════════════════════
 * INA231 — 电压 / 电流 / 功率
 * ═══════════════════════════════════════════════════════════ */

void task_ina231(void *pvParameter)
{
    (void)pvParameter;
    while (1) {
        if (!g_ina231_ready) {
            g_ina231_ready = (init_ina231_with_retry() == ESP_OK);
            if (!g_ina231_ready) {
                vTaskDelay(pdMS_TO_TICKS(INTERVAL_INA231_MS));
                continue;
            }
        }

        float bus_v = 0, shunt_v = 0, current_ma = 0, power_mw = 0;
        esp_err_t ok =
            ina231_read_bus_voltage_v  (I2C_MASTER_PORT, INA231_ADDR, &bus_v)         |
            ina231_read_shunt_voltage_v(I2C_MASTER_PORT, INA231_ADDR, &shunt_v)       |
            ina231_read_current_ma     (I2C_MASTER_PORT, INA231_ADDR,
                                        g_ina231_current_lsb_nA, &current_ma)         |
            ina231_read_power_mw       (I2C_MASTER_PORT, INA231_ADDR,
                                        g_ina231_current_lsb_nA, &power_mw);
        if (ok == ESP_OK) {
            float bus_v_cal = ina231_bus_voltage_calibrate(bus_v, &g_bus_cal);
            ESP_LOGI(TAG, "INA231: BUS=%.3fV(cal=%.3fV) SHUNT=%.6fV CURR=%.3fmA PWR=%.3fmW",
                     bus_v, bus_v_cal, shunt_v, current_ma, power_mw);

            char data[128];
            snprintf(data, sizeof(data),
                     "{\"bus_v\":%.4f,\"bus_v_cal\":%.4f,"
                     "\"shunt_v\":%.6f,\"current_ma\":%.3f,\"power_mw\":%.3f}",
                     bus_v, bus_v_cal, shunt_v, current_ma, power_mw);
            sensor_upload("ina231", "power", data);
        } else {
            ESP_LOGW(TAG, "INA231 读取失败");
            g_ina231_ready = false;
        }
        vTaskDelay(pdMS_TO_TICKS(INTERVAL_INA231_MS));
    }
}

/* ═══════════════════════════════════════════════════════════
 * SGP30 — CO₂ / TVOC
 * ═══════════════════════════════════════════════════════════ */

void task_sgp30(void *pvParameter)
{
    (void)pvParameter;

    /* SGP30 实例存放在任务栈（较小，仅句柄）*/
    SGP30 sgp30(I2C_MASTER_PORT,
                (gpio_num_t)21, (gpio_num_t)20,   /* SDA / SCL，与 main 相同 */
                100000);
    if (sgp30.init() != ESP_OK) {
        ESP_LOGE(TAG, "SGP30 初始化失败，任务退出");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        uint16_t co2 = 0, tvoc = 0;
        if (sgp30.measure(&co2, &tvoc) == ESP_OK) {
            ESP_LOGI(TAG, "SGP30: CO2=%u ppm  TVOC=%u ppb", co2, tvoc);

            char data[64];
            snprintf(data, sizeof(data),
                     "{\"co2_ppm\":%u,\"tvoc_ppb\":%u}", co2, tvoc);
            sensor_upload("sgp30", "air_quality", data);
        } else {
            ESP_LOGW(TAG, "SGP30 测量失败");
        }
        vTaskDelay(pdMS_TO_TICKS(INTERVAL_SGP30_MS));
    }
}

/* ═══════════════════════════════════════════════════════════
 * VEML7700 — 环境光照度
 * ═══════════════════════════════════════════════════════════ */

void task_veml7700(void *pvParameter)
{
    (void)pvParameter;

    Adafruit_VEML7700 veml(I2C_MASTER_PORT,
                            (gpio_num_t)21, (gpio_num_t)20);
    if (!veml.begin(100000)) {
        ESP_LOGE(TAG, "VEML7700 初始化失败，任务退出");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        float lux  = veml.readLux(VEML_LUX_AUTO);
        uint8_t gain = veml.getGain();
        uint8_t it   = veml.getIntegrationTime();
        ESP_LOGI(TAG, "VEML7700: Lux=%.2f gain=%u it=%u", lux, gain, it);

        char data[64];
        snprintf(data, sizeof(data),
                 "{\"lux\":%.2f,\"gain\":%u,\"integration_time\":%u}",
                 lux, gain, it);
        sensor_upload("veml7700", "light", data);

        vTaskDelay(pdMS_TO_TICKS(INTERVAL_VEML7700_MS));
    }
}

/* ═══════════════════════════════════════════════════════════
 * PMS9103M — 颗粒物浓度
 * ═══════════════════════════════════════════════════════════ */

void task_pms9103m(void *pvParameter)
{
    (void)pvParameter;

    pms9103m_config_t cfg = {
        .uart_num  = UART_NUM_1,
        .tx_io_num = 17,
        .rx_io_num = 16,
        .baud_rate = 9600,
    };
    if (pms9103m_init(&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "PMS9103M 初始化失败，任务退出");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        pms9103m_data_t d;
        if (pms9103m_read_frame(&d, pdMS_TO_TICKS(2000)) == ESP_OK) {
            ESP_LOGI(TAG,
                     "PMS9103M: PM1.0=%u PM2.5=%u PM10=%u "
                     "PM1.0_ATM=%u PM2.5_ATM=%u PM10_ATM=%u",
                     d.pm1_0_cf1, d.pm2_5_cf1, d.pm10_cf1,
                     d.pm1_0_atm, d.pm2_5_atm, d.pm10_atm);

            char data[256];
            snprintf(data, sizeof(data),
                     "{\"pm1_0_cf1\":%u,\"pm2_5_cf1\":%u,\"pm10_cf1\":%u,"
                     "\"pm1_0_atm\":%u,\"pm2_5_atm\":%u,\"pm10_atm\":%u,"
                     "\"cnt_0_3um\":%u,\"cnt_0_5um\":%u,\"cnt_1_0um\":%u,"
                     "\"cnt_2_5um\":%u,\"cnt_5_0um\":%u,\"cnt_10um\":%u}",
                     d.pm1_0_cf1, d.pm2_5_cf1, d.pm10_cf1,
                     d.pm1_0_atm, d.pm2_5_atm, d.pm10_atm,
                     d.count_0_3um, d.count_0_5um, d.count_1_0um,
                     d.count_2_5um, d.count_5_0um, d.count_10_0um);
            sensor_upload("pms9103m", "particulate", data);
        } else {
            ESP_LOGW(TAG, "PMS9103M 读取失败");
        }
        vTaskDelay(pdMS_TO_TICKS(INTERVAL_PMS9103M_MS));
    }
}

/* ═══════════════════════════════════════════════════════════
 * Sound Meter — 声压级
 * ═══════════════════════════════════════════════════════════ */

void task_sound_meter(void *pvParameter)
{
    (void)pvParameter;

    static const sound_meter_db_calibration_point_t cal_pts[] = {
        { .voltage = 0.0f,  .db = 0.0f  },
        { .voltage = 0.5f,  .db = 40.0f },
        { .voltage = 0.62f, .db = 49.0f },
        { .voltage = 1.29f, .db = 60.0f },
        { .voltage = 2.55f, .db = 68.0f },
        { .voltage = 2.58f, .db = 74.0f },
        { .voltage = 2.60f, .db = 77.0f },
    };

    sound_meter_handle_t handle = {};
    sound_meter_config_t cfg;
    sound_meter_get_default_config(&cfg);
    cfg.use_adc_calibration     = true;
    cfg.calibration_points      = cal_pts;
    cfg.calibration_point_count = sizeof(cal_pts) / sizeof(cal_pts[0]);

    if (sound_meter_init(&handle, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Sound Meter 初始化失败，任务退出");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        uint32_t raw = 0;
        float voltage = 0, db = 0;
        if (sound_meter_read_raw    (&handle, &raw)     == ESP_OK &&
            sound_meter_read_voltage(&handle, &voltage) == ESP_OK &&
            sound_meter_read_db     (&handle, &db)      == ESP_OK) {
            ESP_LOGI(TAG, "Sound: raw=%u voltage=%.3fV db=%.1f", raw, voltage, db);

            char data[64];
            snprintf(data, sizeof(data),
                     "{\"raw\":%u,\"voltage_v\":%.3f,\"db\":%.1f}",
                     (unsigned)raw, voltage, db);
            sensor_upload("sound_meter", "sound_level", data);
        } else {
            ESP_LOGW(TAG, "Sound Meter 读取失败");
        }
        vTaskDelay(pdMS_TO_TICKS(INTERVAL_SOUND_MS));
    }
}

/* ═══════════════════════════════════════════════════════════
 * UV Meter — 紫外线指数
 * ═══════════════════════════════════════════════════════════ */

void task_uv_meter(void *pvParameter)
{
    (void)pvParameter;

    uv_meter_handle_t handle = {};
    uv_meter_config_t cfg;
    uv_meter_get_default_config(&cfg);
    cfg.channel = ADC_CHANNEL_4;

    if (uv_meter_init(&handle, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "UV Meter 初始化失败，任务退出");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        uint32_t raw = 0;
        float voltage = 0, uv_index = 0;
        if (uv_meter_read_raw      (&handle, &raw)       == ESP_OK &&
            uv_meter_read_voltage  (&handle, &voltage)   == ESP_OK &&
            uv_meter_read_uv_index (&handle, &uv_index)  == ESP_OK) {
            uv_meter_level_t level    = uv_meter_get_level(uv_index);
            const char      *level_s  = uv_meter_level_to_string(level);
            ESP_LOGI(TAG, "UV: raw=%u voltage=%.3fV index=%.2f level=%s",
                     (unsigned)raw, voltage, uv_index, level_s);

            char data[80];
            snprintf(data, sizeof(data),
                     "{\"raw\":%u,\"voltage_v\":%.3f,\"uv_index\":%.2f,\"level\":\"%s\"}",
                     (unsigned)raw, voltage, uv_index, level_s);
            sensor_upload("uv_meter", "uv_radiation", data);
        } else {
            ESP_LOGW(TAG, "UV Meter 读取失败");
        }
        vTaskDelay(pdMS_TO_TICKS(INTERVAL_UV_MS));
    }
}
