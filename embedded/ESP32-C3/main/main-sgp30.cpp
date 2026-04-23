#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sgp30.h"

static const char *TAG = "SGP30_APP";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting SGP30 example");

    SGP30 sensor(I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_20, 100000);
    esp_err_t err = sensor.init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SGP30 init failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Waiting for SGP30 warm-up. This can take 15-30 seconds.");
    for (int i = 0; i < 30; i++) {
        uint16_t co2 = 0;
        uint16_t tvoc = 0;
        err = sensor.measure(&co2, &tvoc);
        if (err == ESP_OK) {
            if (co2 != 400 || tvoc != 0) {
                ESP_LOGI(TAG, "Warm-up complete: CO2 = %d ppm, TVOC = %d ppb", co2, tvoc);
                break;
            }
        } else {
            ESP_LOGW(TAG, "Warm-up read failed: %s", esp_err_to_name(err));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    while (true) {
        uint16_t co2 = 0;
        uint16_t tvoc = 0;
        err = sensor.measure(&co2, &tvoc);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "CO2 = %d ppm, TVOC = %d ppb", co2, tvoc);
        } else {
            ESP_LOGE(TAG, "SGP30 measure failed: %s", esp_err_to_name(err));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
