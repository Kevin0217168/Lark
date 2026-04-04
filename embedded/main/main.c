#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_app_format.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "esp_heap_caps.h"

#include "Wifista.h"
#include "Camera.h"
#include "main.h"
#include "sht4x.h"
#include "device_secret.h"
#include "i2c_recovery.h"
#include "tasks.h"
#include "remote_log.h"

// OTA 诊断超时配置
#define WIFI_CONNECT_TIMEOUT_MS     30000   // WiFi 连接超时 30 秒
#define WS_CONNECT_TIMEOUT_MS       30000   // WebSocket 连接超时 30 秒

static const char *TAG = "main";

extern bool Wifi_isConnected;

Device_t device = {
    .name = "ESP32 - Default",
    .uuid = "",
    .isOnline = false,
    .status = DEVICE_OFFLINE,
};

void app_main(void)
{
    // ---------------------WIFI连接配置-------------------------
    ESP_LOGI(TAG, "系统启动");
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // 优先从 NVS 读取 secret
    char nvs_secret[SECRET_MAX_LEN] = {0};
    if (load_secret_from_nvs(nvs_secret, sizeof(nvs_secret)) == ESP_OK && nvs_secret[0] != '\0') {
        strncpy(secret, nvs_secret, sizeof(secret) - 1);
        secret[sizeof(secret) - 1] = '\0';
        ESP_LOGI(TAG, "NVS 密钥已加载 (..%s)", secret + (strlen(secret) > 4 ? strlen(secret) - 4 : 0));
    } else {
        // 第一次启动或未写入过，写入默认密钥
        save_secret_to_nvs(secret);
        ESP_LOGI(TAG, "默认密钥已写入 NVS");
    }

    // ----------------------远程日志 阶段一：安装钩子--------------------------
    // 在 WiFi 之前安装 vprintf 钩子，WiFi/SNTP/OTA 日志全部缓存到 16KB 环形缓冲区
    remote_log_early_init();

    // ----------------------OTA 状态早期检测--------------------------
    // 在 WiFi 之前检测 OTA 状态，用于后续网络连接超时时自动回滚
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state = ESP_OTA_IMG_UNDEFINED;
    bool ota_state_valid = (esp_ota_get_state_partition(running, &ota_state) == ESP_OK);
    bool ota_pending = ota_state_valid && (ota_state == ESP_OTA_IMG_PENDING_VERIFY);
    if (ota_pending) {
        ESP_LOGW(TAG, "检测到 OTA 待验证状态，将进行完整诊断（硬件 + 网络）");
    }

    WifistaInit("关闭", "kaiwen0818");
    // 等待wifi连接（OTA 待验证时设置超时，防止新固件 WiFi 故障导致死锁）
    {
        int wifi_wait_ticks = 0;
        const int wifi_timeout_ticks = WIFI_CONNECT_TIMEOUT_MS / 500;
        while (!Wifi_isConnected)
        {
            vTaskDelay(500 / portTICK_PERIOD_MS);
            wifi_wait_ticks++;
            if (ota_pending && wifi_wait_ticks >= wifi_timeout_ticks) {
                ESP_LOGE(TAG, "WiFi 连接超时 (%d ms), OTA 固件回滚...", WIFI_CONNECT_TIMEOUT_MS);
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }
    // SNTP获取时间
    obtain_time();

    // ----------------------远程日志 阶段二：启动 POST 上传--------------------------
    // 网络已就绪，启动 flush 任务，通过独立 HTTP POST 批量上传日志
    remote_log_start("https://lark.mintlab.top", "/api/logs", 443, secret);
    // remote_log_start("http://192.168.1.200", "/api/logs", 8080, secret);


    // ---------------------版本更新检测--------------------------
    const esp_app_desc_t* desc;
    desc = esp_app_get_description();
    ESP_LOGI(TAG, "固件版本: %s", desc->version);

    // OTA 硬件诊断（仅在 OTA 待验证时执行，网络验证推迟到 WS 连接后）
    if (ota_pending) {
        ESP_LOGI(TAG, "新版本固件诊断中...");
        if (!diagnostic()) {
            ESP_LOGE(TAG, "硬件诊断失败! 回滚到上一个版本...");
            esp_ota_mark_app_invalid_rollback_and_reboot();
        }
        ESP_LOGI(TAG, "硬件诊断通过, 继续验证网络...");
    } else if (!ota_state_valid) {
        ESP_LOGW(TAG, "无法获取 OTA 状态 (rollback 可能未启用), 跳过诊断");
    }

    // ---------------------连接服务器----------------------------
    WifiSecurityClientInit();

    // 通过id开启ws连接/stream/viewer/ws
    char path_data[128];
    snprintf(path_data, sizeof(path_data), "/api/stream/device/ws?secret=%s", secret);
    // 注册回调函数
    Websocket_event_handler_register(NULL, ws_text_handler);
    WebsocketStart("wss://lark.mintlab.top", path_data, 443);
    // WebsocketStart("ws://192.168.1.200", path_data, 8080);

    // 等待ws连接成功（OTA 待验证时设置超时，确保新固件能正常联网）
    {
        int ws_wait_ticks = 0;
        const int ws_timeout_ticks = WS_CONNECT_TIMEOUT_MS / 500;
        while (!WebsocketIsConnected())
        {
            vTaskDelay(500 / portTICK_PERIOD_MS);
            ws_wait_ticks++;
            if (ota_pending && ws_wait_ticks >= ws_timeout_ticks) {
                ESP_LOGE(TAG, "WebSocket 连接超时 (%d ms), OTA 固件回滚...", WS_CONNECT_TIMEOUT_MS);
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }

    // ----------------------OTA 验证（网络部分）推迟到 ota_task 版本检查通过后确认--------------------------


    // ----------------------初始化传感器-----------------------------
    // 如果不是第一次 OTA 运行（或者已经诊断过），则在这里正常初始化
    // 第一次运行已经在 diagnostic() 里面初始化过了
    bool need_sensor_init = !ota_pending;
    if (need_sensor_init) {
        // I2C 总线恢复：WDT/软重启后 SHT4x 可能卡住 SDA，需先恢复总线
        i2c_bus_recovery(I2C_SDA_PIN, I2C_SCL_PIN);

        sensirion_i2c_init();
        int sht_retry = 0;
        const int sht_max_retry = 10;
        while (sht4x_probe() != STATUS_OK && sht_retry < sht_max_retry)
        {
            sht_retry++;
            ESP_LOGW(TAG, "SHT sensor probing failed, retry %d/%d", sht_retry, sht_max_retry);
            // 每次失败后再尝试一次总线恢复
            sensirion_i2c_release();
            i2c_bus_recovery(I2C_SDA_PIN, I2C_SCL_PIN);
            sensirion_i2c_init();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        if (sht_retry >= sht_max_retry) {
            ESP_LOGE(TAG, "SHT sensor 初始化失败，已达到最大重试次数 %d", sht_max_retry);
        } else {
            ESP_LOGI(TAG, "SHT sensor probing successful");
        }

        // 摄像头 SCCB I2C (GPIO 26/27) 总线恢复
        i2c_bus_recovery(26, 27);
        CameraInit();
    }
    
    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL) {
        s->set_framesize(s, FRAMESIZE_SVGA);
        s->set_vflip(s, 1);
    }

    // ----------------------OTA 任务（在传感器/摄像头初始化完成后启动）-----------
    // 注意：OTA 必须在摄像头初始化之后启动，因为 OTA flash 擦写 与
    // 摄像头 PSRAM DMA 共享 SPI 总线，同时运行会触发 WDT
    // 将 ota_pending 传给 ota_task，版本检查通过后在任务内确认固件有效
    SemaphoreHandle_t ota_sem = NULL;
    ota_sem = xSemaphoreCreateBinary();
    configASSERT(ota_sem);

    TaskHandle_t ota_task_Handle = NULL;
    
    ota_context_t ota_context = {
        .ota_pending_flag = ota_pending,
        .ota_sem = ota_sem
    };
    
    xTaskCreate(ota_task, "ota_task", 8192, &ota_context, 1, &ota_task_Handle);
    configASSERT(ota_task_Handle);

    // -------------------------开启任务-------------------------------
    ESP_LOGI(TAG, "等待 OTA 任务...");
    xSemaphoreTake(ota_sem, portMAX_DELAY);
    ESP_LOGI(TAG, "OTA 任务已完成，继续启动其他任务...");

    // OTA 待验证流程中，摄像头在 OTA 任务写 flash 前被关闭，需要重新初始化
    if (ota_pending) {
        ESP_LOGI(TAG, "重新初始化摄像头...");
        i2c_bus_recovery(26, 27);
        CameraInit();
        sensor_t *s2 = esp_camera_sensor_get();
        if (s2 != NULL) {
            s2->set_framesize(s2, FRAMESIZE_SVGA);
            s2->set_vflip(s2, 1);
        }
    }

    // 开启图像传输任务
    // 用户任务栈显式分配到 PSRAM，释放内部 SRAM 给 TLS / DMA 使用
    camera_stream_sem_init();   // 必须在创建任务之前初始化信号量
    static uint8_t ucParameterToPass;
    TaskHandle_t xHandle = NULL;
    xTaskCreateWithCaps(camera_transmit_task, "camera_transmit", 4096, &ucParameterToPass, 1, &xHandle, MALLOC_CAP_SPIRAM);
    configASSERT(xHandle);

    // 开启传感器传输任务（TLS HTTPS POST 需要较大栈空间）
    static uint8_t sensor_data_transmit_task_Handle_ParameterToPass;
    TaskHandle_t sensor_data_transmit_task_Handle = NULL;
    xTaskCreateWithCaps(sensor_data_transmit_task, "sensor_data_tra", 6144, &sensor_data_transmit_task_Handle_ParameterToPass, 1, &sensor_data_transmit_task_Handle, MALLOC_CAP_SPIRAM);
    configASSERT(sensor_data_transmit_task_Handle);

    // 开启健康监控任务（检测 WiFi/WS 长时间断连自动重启）
    // 注意：log_system_status() 内有大量 snprintf 格式化，栈需求较大
    static uint8_t health_monitor_param;
    TaskHandle_t health_monitor_handle = NULL;
    xTaskCreateWithCaps(health_monitor_task, "health_monitor", 4096, &health_monitor_param, 1, &health_monitor_handle, MALLOC_CAP_SPIRAM);
    configASSERT(health_monitor_handle);
}
