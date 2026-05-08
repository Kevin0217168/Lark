#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

#include "ina231.h"
#include "pms9103m.h"
#include "sound_meter.h"
#include "uv_meter.h"

#include "tasks.h"

static const char *TAG = "sensor_task";

#define I2C_MASTER_PORT    I2C_NUM_0
#define INA231_ADDR        INA231_ADDRESS_DEFAULT
#define INA231_MAX_CURR_A  1.0f
#define INA231_SHUNT_UOHM  100000UL

#define UPLOAD_HOST  "https://lark.mintlab.top"
#define UPLOAD_PATH  "/api/sensors"
#define UPLOAD_PORT  443

#define INTERVAL_INA231_MS       2000
#define INTERVAL_SGP30_MS        5000
#define INTERVAL_VEML7700_MS     5000
#define INTERVAL_PMS9103M_MS    10000
#define INTERVAL_SOUND_MS        2000
#define INTERVAL_UV_MS           2000

static uint32_t g_ina231_current_lsb_nA = 0;
static bus_calibration_t g_bus_cal = {0};
static SemaphoreHandle_t s_upload_mutex = NULL;

void sensor_tasks_init(void)
{
    s_upload_mutex = xSemaphoreCreateMutex();
    configASSERT(s_upload_mutex);

    g_ina231_current_lsb_nA = ina231_current_lsb_nA(INA231_MAX_CURR_A);
    ESP_ERROR_CHECK(ina231_write_calibration(I2C_MASTER_PORT, INA231_ADDR,
                                             INA231_MAX_CURR_A, INA231_SHUNT_UOHM));
    ESP_ERROR_CHECK(ina231_set_bus_conversion(I2C_MASTER_PORT, INA231_ADDR, 3320));
    ESP_ERROR_CHECK(ina231_set_shunt_conversion(I2C_MASTER_PORT, INA231_ADDR, 3320));
    ESP_ERROR_CHECK(ina231_set_mode(I2C_MASTER_PORT, INA231_ADDR, INA231_MODE_CONTINUOUS_BOTH));
    ESP_ERROR_CHECK(ina231_enable_alert_on_conversion(I2C_MASTER_PORT, INA231_ADDR, true));

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

void sensor_upload(const char *sensor_type, const char *data_type, const char *data_json)
{
    char body[512];
    int len = snprintf(body, sizeof(body),
                       "{\"sensor\":\"%s\",\"data_type\":\"%s\",\"data\":%s}",
                       sensor_type, data_type, data_json);
    if (len <= 0 || len >= (int)sizeof(body)) {
        ESP_LOGE(TAG, "[%s] JSON 报文过长，跳过上报", sensor_type);
        return;
    }

    if (xSemaphoreTake(s_upload_mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        ESP_LOGW(TAG, "[%s] 上报互斥锁超时，跳过本次", sensor_type);
        return;
    }

    esp_http_client_config_t cfg = {0};
    cfg.url = UPLOAD_HOST UPLOAD_PATH;
    cfg.port = UPLOAD_PORT;
    cfg.method = HTTP_METHOD_POST;
    cfg.crt_bundle_attach = esp_crt_bundle_attach;
    cfg.timeout_ms = 5000;

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "[%s/%s] 创建 HTTP 客户端失败", sensor_type, data_type);
        xSemaphoreGive(s_upload_mutex);
        return;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, body, len);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "[%s/%s] 上报成功 HTTP %d", sensor_type, data_type, status);
    } else {
        ESP_LOGE(TAG, "[%s/%s] 上报失败: %s", sensor_type, data_type, esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    xSemaphoreGive(s_upload_mutex);
}

void task_ina231(void *pvParameter)
{
    (void)pvParameter;

    while (1) {
        float bus_v = 0.0f;
        float shunt_v = 0.0f;
        float current_ma = 0.0f;
        float power_mw = 0.0f;

        esp_err_t ok = ESP_OK;
        ok |= ina231_read_bus_voltage_v(I2C_MASTER_PORT, INA231_ADDR, &bus_v);
        ok |= ina231_read_shunt_voltage_v(I2C_MASTER_PORT, INA231_ADDR, &shunt_v);
        ok |= ina231_read_current_ma(I2C_MASTER_PORT, INA231_ADDR, g_ina231_current_lsb_nA, &current_ma);
        ok |= ina231_read_power_mw(I2C_MASTER_PORT, INA231_ADDR, g_ina231_current_lsb_nA, &power_mw);

        if (ok == ESP_OK) {
            float bus_v_cal = ina231_bus_voltage_calibrate(bus_v, &g_bus_cal);
            char data[128];
            snprintf(data, sizeof(data),
                     "{\"bus_v\":%.4f,\"bus_v_cal\":%.4f,\"shunt_v\":%.6f,\"current_ma\":%.3f,\"power_mw\":%.3f}",
                     bus_v, bus_v_cal, shunt_v, current_ma, power_mw);
            sensor_upload("ina231", "power", data);
        } else {
            ESP_LOGW(TAG, "INA231 读取失败");
        }

        vTaskDelay(pdMS_TO_TICKS(INTERVAL_INA231_MS));
    }
}

void task_sgp30(void *pvParameter)
{
    (void)pvParameter;

    ESP_LOGW(TAG, "SGP30 组件仅提供 C++ 接口，当前 C 任务仅保留占位");
    while (1) {
        sensor_upload("sgp30", "air_quality", "{\"status\":\"not_supported_in_c\"}");
        vTaskDelay(pdMS_TO_TICKS(INTERVAL_SGP30_MS));
    }
}

void task_veml7700(void *pvParameter)
{
    (void)pvParameter;

    ESP_LOGW(TAG, "VEML7700 组件仅提供 C++ 接口，当前 C 任务仅保留占位");
    while (1) {
        sensor_upload("veml7700", "light", "{\"status\":\"not_supported_in_c\"}");
        vTaskDelay(pdMS_TO_TICKS(INTERVAL_VEML7700_MS));
    }
}

void task_pms9103m(void *pvParameter)
{
    (void)pvParameter;

    pms9103m_config_t cfg = {
        .uart_num = UART_NUM_1,
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
            char data[256];
            snprintf(data, sizeof(data),
                     "{\"pm1_0_cf1\":%u,\"pm2_5_cf1\":%u,\"pm10_cf1\":%u,\"pm1_0_atm\":%u,\"pm2_5_atm\":%u,\"pm10_atm\":%u,\"cnt_0_3um\":%u,\"cnt_0_5um\":%u,\"cnt_1_0um\":%u,\"cnt_2_5um\":%u,\"cnt_5_0um\":%u,\"cnt_10um\":%u}",
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

void task_sound_meter(void *pvParameter)
{
    (void)pvParameter;

    static const sound_meter_db_calibration_point_t cal_pts[] = {
        { .voltage = 0.0f, .db = 0.0f },
        { .voltage = 0.5f, .db = 40.0f },
        { .voltage = 0.62f, .db = 49.0f },
        { .voltage = 1.29f, .db = 60.0f },
        { .voltage = 2.55f, .db = 68.0f },
        { .voltage = 2.58f, .db = 74.0f },
        { .voltage = 2.60f, .db = 77.0f },
    };

    sound_meter_handle_t handle = {0};
    sound_meter_config_t cfg;
    sound_meter_get_default_config(&cfg);
    cfg.use_adc_calibration = true;
    cfg.calibration_points = cal_pts;
    cfg.calibration_point_count = sizeof(cal_pts) / sizeof(cal_pts[0]);

    if (sound_meter_init(&handle, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Sound Meter 初始化失败，任务退出");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        uint32_t raw = 0;
        float voltage = 0.0f;
        float db = 0.0f;
        if (sound_meter_read_raw(&handle, &raw) == ESP_OK &&
            sound_meter_read_voltage(&handle, &voltage) == ESP_OK &&
            sound_meter_read_db(&handle, &db) == ESP_OK) {
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

void task_uv_meter(void *pvParameter)
{
    (void)pvParameter;

    uv_meter_handle_t handle = {0};
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
        float voltage = 0.0f;
        float uv_index = 0.0f;

        if (uv_meter_read_raw(&handle, &raw) == ESP_OK &&
            uv_meter_read_voltage(&handle, &voltage) == ESP_OK &&
            uv_meter_read_uv_index(&handle, &uv_index) == ESP_OK) {
            uv_meter_level_t level = uv_meter_get_level(uv_index);
            const char *level_s = uv_meter_level_to_string(level);

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
