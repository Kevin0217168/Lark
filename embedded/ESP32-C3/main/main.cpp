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

    /* ── I2C 总线初始化（所有 I2C 传感器共用） ── */
    ESP_ERROR_CHECK(ina231_init(I2C_MASTER_PORT, I2C_MASTER_SDA,
                                I2C_MASTER_SCL, I2C_MASTER_FREQ_HZ));

    /* ── 传感器任务公共资源初始化（互斥锁、INA231 校准等） ── */
    sensor_tasks_init();

    /* ── 创建各传感器独立任务 ── */
    xTaskCreate(task_ina231,      "ina231",      4096, NULL, 5, NULL);
    xTaskCreate(task_sgp30,       "sgp30",       6144, NULL, 5, NULL);
    xTaskCreate(task_veml7700,    "veml7700",    6144, NULL, 5, NULL);
    xTaskCreate(task_pms9103m,    "pms9103m",    4096, NULL, 5, NULL);
    xTaskCreate(task_sound_meter, "sound_meter", 8192, NULL, 5, NULL);
    xTaskCreate(task_uv_meter,    "uv_meter",    6144, NULL, 5, NULL);

    ESP_LOGI(TAG, "所有传感器任务已启动");
}
