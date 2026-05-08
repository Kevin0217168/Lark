#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>
#include "nvs_flash.h"
#include "ina231.h"
#include "Wifista.h"
#include "device_secret.h"
extern "C" {
#include "remote_log.h"
}
#include "tasks.h"

static const char *TAG = "sensor_hub";

#define I2C_MASTER_PORT    I2C_NUM_0
#define I2C_MASTER_SDA     GPIO_NUM_21
#define I2C_MASTER_SCL     GPIO_NUM_20
#define I2C_MASTER_FREQ_HZ 100000

#define WIFI_SSID "关闭"
#define WIFI_PASS "kaiwen0818"

extern bool Wifi_isConnected;

/* ws_process.c 中定义的弱符号函数（C 链接） */
extern "C" void auto_feed_task(void *pvParameter);

extern "C" void app_main(void)
{
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // ── 远程日志 阶段一：安装 vprintf 钩子（WiFi 之前）──
    remote_log_early_init();

    WifistaInit((const uint8_t *)WIFI_SSID, (const uint8_t *)WIFI_PASS);
    while (!Wifi_isConnected) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGI(TAG, "WiFi 已连接，开始 SNTP 对时");
    obtain_time();

    // ── WebSocket 连接 ──
    WifiSecurityClientInit();

    Websocket_event_handler_register(NULL, ws_text_handler);

    char ws_path[256];
    snprintf(ws_path, sizeof(ws_path), "/api/stream/device/ws?secret=%s", secret);
    WebsocketStart("wss://lark.mintlab.top", ws_path, 443);

    // 等待 WS 连接
    {
        int ws_wait_ticks = 0;
        const int ws_timeout_ticks = 60000 / 500;  // 60s 超时
        while (!WebsocketIsConnected()) {
            vTaskDelay(500 / portTICK_PERIOD_MS);
            ws_wait_ticks++;
            if (ws_wait_ticks >= ws_timeout_ticks) {
                ESP_LOGW(TAG, "WebSocket 连接超时，继续启动传感器任务");
                break;
            }
        }
    }
    if (WebsocketIsConnected()) {
        ESP_LOGI(TAG, "WebSocket 已连接");
    }

    // ── 远程日志 阶段二：启动 HTTP POST 上传（网络已就绪）──
    remote_log_start("https://lark.mintlab.top", "/api/logs", 443, secret);

    // ── INA231 初始化 ──

    ESP_LOGI(TAG, "启动传感器集线器");
    ESP_ERROR_CHECK(ina231_init(I2C_MASTER_PORT, I2C_MASTER_SDA,
                                I2C_MASTER_SCL, I2C_MASTER_FREQ_HZ));

    sensor_tasks_init();

    xTaskCreate(task_sensor_collect, "sensor_collect", 10240, NULL, 5, NULL);

    ESP_LOGI(TAG, "统一传感器采集任务已启动 (周期 %d 秒)",
             (int)(SENSOR_COLLECT_INTERVAL_MS / 1000));

    // ── OTA 任务 ──
    static uint8_t ota_param;
    TaskHandle_t ota_handle = NULL;
    xTaskCreate(ota_task, "ota_task", 8192, &ota_param, 1, &ota_handle);
    configASSERT(ota_handle);

    // ── 自动上粮任务（预留接口，默认弱符号实现会自行退出）──
    static uint8_t feed_param;
    TaskHandle_t feed_handle = NULL;
    xTaskCreate(auto_feed_task, "auto_feed", 4096, &feed_param, 1, &feed_handle);
    configASSERT(feed_handle);

    ESP_LOGI(TAG, "所有任务已启动");
}
