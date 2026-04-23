#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "uv_meter.h"

static const char *TAG = "uvmeter_app";

void app_main(void)
{
    uv_meter_handle_t meter = {0};
    uv_meter_config_t config;
    uv_meter_get_default_config(&config);

    ESP_LOGI(TAG, "Using ADC1 channel %d on GPIO4 with attenuation 12dB", config.channel);
    ESP_LOGI(TAG, "ADC calibration enabled: %s", config.use_adc_calibration ? "true" : "false");

    esp_err_t err = uv_meter_init(&meter, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uv_meter_init failed (%s)", esp_err_to_name(err));
        return;
    }

    while (1) {
        float voltage = 0.0f;
        float uv_index = 0.0f;
        uint32_t raw = 0;

        err = uv_meter_read_raw(&meter, &raw);
        if (err == ESP_OK) {
            err = uv_meter_read_voltage(&meter, &voltage);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "voltage read failed (%s)", esp_err_to_name(err));
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }
            err = uv_meter_read_uv_index(&meter, &uv_index);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "uv index read failed (%s)", esp_err_to_name(err));
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }
            uv_meter_level_t level = uv_meter_get_level(uv_index);
            const char *level_str = uv_meter_level_to_string(level);
            ESP_LOGI(TAG, "raw=%u, voltage=%.3f V, uv_index=%.2f, level=%s", raw, voltage, uv_index, level_str);
        } else {
            ESP_LOGE(TAG, "read failed (%s)", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
