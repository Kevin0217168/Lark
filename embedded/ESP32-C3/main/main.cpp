#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>
#include "nvs_flash.h"
#include "ina231.h"
#include "Wifista.h"
#include "tasks.h"

static const char *TAG = "sensor_hub";

#define I2C_MASTER_PORT    I2C_NUM_0
#define I2C_MASTER_SDA     GPIO_NUM_21
#define I2C_MASTER_SCL     GPIO_NUM_20
#define I2C_MASTER_FREQ_HZ 100000

#define WIFI_SSID "iPhone"
#define WIFI_PASS "1234567890"

extern bool Wifi_isConnected;

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "启动传感器集线器");

    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    WifistaInit((const uint8_t *)WIFI_SSID, (const uint8_t *)WIFI_PASS);
    while (!Wifi_isConnected) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGI(TAG, "WiFi 已连接，开始 SNTP 对时");
    obtain_time();

    ESP_ERROR_CHECK(ina231_init(I2C_MASTER_PORT, I2C_MASTER_SDA,
                                I2C_MASTER_SCL, I2C_MASTER_FREQ_HZ));

    sensor_tasks_init();

    xTaskCreate(task_sensor_collect, "sensor_collect", 10240, NULL, 5, NULL);

    ESP_LOGI(TAG, "统一传感器采集任务已启动 (周期 %d 秒)",
             (int)(SENSOR_COLLECT_INTERVAL_MS / 1000));
}
