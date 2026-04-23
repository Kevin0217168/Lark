#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "sound_meter.h"

static const char *TAG = "soundmeter_app";

static const sound_meter_db_calibration_point_t calibration_points[] = {
    { .voltage = 0.0f, .db = 0.0f },
    { .voltage = 0.5f, .db = 40.0f },
    { .voltage = 0.62f, .db = 49.0f },
    { .voltage = 1.29f, .db = 60.0f },
    { .voltage = 2.55f, .db = 68.0f },
    { .voltage = 2.58f, .db = 74.0f },
    { .voltage = 2.60f, .db = 77.0f },
};

void app_main(void)
{
    sound_meter_handle_t meter = {0};
    sound_meter_config_t config;
    sound_meter_get_default_config(&config);
    config.use_adc_calibration = true;
    config.calibration_points = calibration_points;
    config.calibration_point_count = sizeof(calibration_points) / sizeof(calibration_points[0]);

    ESP_LOGI(TAG, "Using ADC1 channel %d on GPIO4 with attenuation 12dB", config.channel);
    ESP_LOGI(TAG, "ADC calibration enabled, using %u calibration points", (unsigned)config.calibration_point_count);

    esp_err_t err = sound_meter_init(&meter, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "sound_meter_init failed (%s)", esp_err_to_name(err));
        return;
    }

    while (1) {
        float voltage = 0.0f;
        float db = 0.0f;
        uint32_t raw = 0;

        err = sound_meter_read_raw(&meter, &raw);
        if (err == ESP_OK) {
            err = sound_meter_read_voltage(&meter, &voltage);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "voltage read failed (%s)", esp_err_to_name(err));
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }
            err = sound_meter_read_db(&meter, &db);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "db read failed (%s)", esp_err_to_name(err));
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }
            ESP_LOGI(TAG, "raw=%u, voltage=%.3f V, calibrated db=%.1f", raw, voltage, db);
        } else {
            ESP_LOGE(TAG, "read failed (%s)", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
