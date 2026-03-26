#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_app_format.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "Wifista.h"
#include "Camera.h"
#include "main.h"
#include "sht4x.h"

static const char *TAG = "main";
// 设备2
static const char *secret = "2d3173bfc1f64da0894a3257e1636d72";
// 设备1
// static const char *secret = "b1f9562544a348c98c57a66b32a92d32";
extern bool Wifi_isConnected;

Device_t device = {
    .name = "ESP32 - Default",
    .uuid = "",
    .isOnline = false,
    .status = DEVICE_OFFLINE,
    //TODO: 存储版本信息
};

void register_handler(RequestContext_t *ctx)
{
    if (esp_http_client_get_status_code(*(ctx->client)) != 200)
    {
        ESP_LOGE(TAG, "注册失败, 状态码: %d", esp_http_client_get_status_code(*(ctx->client)));
        return;
    }

    if (ctx->is_json && ctx->json != NULL)
    {
        // 根据实际返回字段名修改
        cJSON *id_item = cJSON_GetObjectItemCaseSensitive(ctx->json, "id");
        cJSON *name_item = cJSON_GetObjectItemCaseSensitive(ctx->json, "name");
        cJSON *status_item = cJSON_GetObjectItemCaseSensitive(ctx->json, "status");

        if (cJSON_IsString(id_item) && cJSON_IsString(name_item) && cJSON_IsString(status_item))
        {
            // 注意：需要拷贝字符串，因为 ctx->json 即将被释放
            strncpy(device.uuid, id_item->valuestring, sizeof(device.uuid) - 1);
            device.uuid[sizeof(device.uuid) - 1] = '\0';
            strncpy(device.name, name_item->valuestring, sizeof(device.name) - 1);
            device.name[sizeof(device.name) - 1] = '\0';
            device.isOnline = (strcasecmp(status_item->valuestring, "online") == 0);

            ESP_LOGI(TAG, "注册成功, 与服务器同步数据:\n名称: %s\nID: %s\n状态: %s",
                     device.name, device.uuid,
                     device.isOnline ? "online" : "offline");
        }
        else
        {
            ESP_LOGE(TAG, "返回的 JSON 缺少必要字段或类型错误");
        }
    }
    else
    {
        ESP_LOGW(TAG, "响应不是有效的 JSON 对象");
    }
}

void PhotoTransmit(camera_fb_t *fb)
{
    WebsocketSendbytes(fb->buf, fb->len);
}

void camera_transmit_task();
void sensor_data_transmit_task();

extern void i2c_scan(void);

void app_main(void)
{
    // ---------------------WIFI连接配置-------------------------
    printf("Hello ESP-IDF!\n");
    nvs_flash_init();
    WifistaInit("关闭", "kaiwen0818");
    // 等待wifi连接
    while (!Wifi_isConnected)
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    // SNTP获取时间
    obtain_time();

    // ---------------------版本更新检测--------------------------
    const esp_app_desc_t* desc;
    desc = esp_app_get_description();
    ESP_LOGI(TAG, "当前运行的版本: Version: %s\n", desc->version);

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // 固件第一次运行, 执行诊断
            // run diagnostic function ...
            // bool diagnostic_is_ok = diagnostic();
            // TODO: 检查后续的连接和传感器初始化是否正常
            bool diagnostic_is_ok = true;
            if (diagnostic_is_ok) {
                ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
                esp_ota_mark_app_valid_cancel_rollback();
            } else {
                ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }

    static uint8_t ota_task_Handle_ParameterToPass;
    TaskHandle_t ota_task_Handle = NULL;
    xTaskCreate(ota_task, "ota_task", 8192, &ota_task_Handle_ParameterToPass, 1, &ota_task_Handle);
    configASSERT(ota_task_Handle);

    // ---------------------连接服务器----------------------------
    // 创建任务执行 HTTPS 请求
    WifiSecurityClientInit();

    // 通过id开启ws连接/stream/viewer/ws
    char path_data[128];
    snprintf(path_data, sizeof(path_data), "/api/stream/device/ws?secret=%s", secret);
    // 注册回调函数
    Websocket_event_handler_register(NULL, ws_text_handler);
    WebsocketStart("wss://lark.mintlab.top", path_data, 443);
    // WebsocketStart("ws://192.168.1.199", path_data, 8080);

    // 等待ws连接成功
    while (!WebsocketIsConnected())
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    // ----------------------初始化传感器-----------------------------
    // 传感器初始化
    i2c_scan();
    sensirion_i2c_init();
    while (sht4x_probe() != STATUS_OK)
    {
        printf("SHT sensor probing failed\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("SHT sensor probing successful\n");

    // 摄像头初始化
    CameraInit();
    sensor_t *s = esp_camera_sensor_get();
    s->set_framesize(s, FRAMESIZE_SVGA);
    s->set_vflip(s, 1);

    // -------------------------开启任务-------------------------------

    // 开启图像传输任务
    static uint8_t ucParameterToPass;
    TaskHandle_t xHandle = NULL;
    xTaskCreate(camera_transmit_task, "camera_transmit_task", 4096, &ucParameterToPass, 1, &xHandle);
    configASSERT(xHandle);

    // 开启传感器传输任务
    static uint8_t sensor_data_transmit_task_Handle_ParameterToPass;
    TaskHandle_t sensor_data_transmit_task_Handle = NULL;
    xTaskCreate(sensor_data_transmit_task, "sensor_data_transmit_task", 4096, &sensor_data_transmit_task_Handle_ParameterToPass, 1, &sensor_data_transmit_task_Handle);
    configASSERT(sensor_data_transmit_task_Handle);
}

void sensor_data_transmit_task()
{
    time_t now = 0;
    struct tm timeinfo = {0};
    while (1)
    {
        int32_t temperature, humidity;
        int8_t ret = sht4x_measure_blocking_read(&temperature, &humidity);
        if (ret == STATUS_OK)
        {
            float temp_c = temperature / 1000.0f;
            float hum_pct = humidity / 1000.0f;
            ESP_LOGI(TAG, "measured temperature: %0.2f degreeCelsius, "
                          "measured humidity: %0.2f percentRH\n",
                     temp_c, hum_pct);

            time(&now);
            setenv("TZ", "CST-8", 1);
            localtime_r(&now, &timeinfo);
            ESP_LOGI(TAG, "当前时间: %04d-%02d-%02d %02d:%02d:%02d+08:00",
                     timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

            // 构建 ISO 8601 格式时间戳（可选，不传则后端使用当前时间）
            char timestamp_str[32];
            strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%dT%H:%M:%S+08:00", &timeinfo);

            char post_data[256];
            snprintf(post_data, sizeof(post_data),
                     "{\"secret\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f,\"timestamp\":\"%s\"}",
                     secret, temp_c, hum_pct, timestamp_str);

            // 发送 POST 请求到后端
            // int ret_code = WifiSecurityRequest("http://192.168.1.199", "/sensors", 8080,
            //                                    WS_CLINENT_METHOD_POST, post_data, NULL);
            int ret_code = WifiSecurityRequest("https://lark.mintlab.top", "/api/sensors", 443,
                                               WS_CLINENT_METHOD_POST, post_data, NULL);
            if (ret_code != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to send sensor data, error=%d", ret_code);
            }
        }
        else
        {
            ESP_LOGE(TAG, "error reading measurement\n");
        }

        vTaskDelay(60000 / portTICK_PERIOD_MS); /* sleep 10s */
    }
}

void camera_transmit_task()
{
    while (1)
    {
        if (device.status == DEVICE_ON_STREAM)
        {
            CameraTakePhoto(PhotoTransmit);
        }
        else
        {
            ESP_LOGI("camera_transmit_task", "待机模式");
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
    }
}
