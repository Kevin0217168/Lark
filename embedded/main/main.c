#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "Wifista.h"
#include "Camera.h"
#include "main.h"

static const char *TAG = "main";
static const char *secret = "b1f9562544a348c98c57a66b32a92d32";
extern bool Wifi_isConnected;

Device_t device = {
    .name = "ESP32 - Default",
    .uuid = "",
    .isOnline = false,
    .status = DEVICE_OFFLINE,
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

void ws_text_handler(void *handler_args, int len, const char *data_ptr)
{
    cJSON *json = cJSON_ParseWithLength(data_ptr, len);
    if (json != NULL)
    {
        char *string = cJSON_Print(json);
        ESP_LOGI(TAG, "收到 JSON 数据, 解析成功: \n%s\n", string);
        cJSON_free(string);

        // 根据实际返回字段名修改
        cJSON *code_item = cJSON_GetObjectItemCaseSensitive(json, "code");
        cJSON *item_item = cJSON_GetObjectItemCaseSensitive(json, "item");
        cJSON *key_item = cJSON_GetObjectItemCaseSensitive(json, "key");
        cJSON *values_item = cJSON_GetObjectItemCaseSensitive(json, "values");
        
        if (cJSON_IsNumber(code_item) && cJSON_IsString(item_item) && cJSON_IsString(key_item) && cJSON_IsString(values_item))
        {
            // 注意：需要拷贝字符串，因为 json 即将被释放
            if (code_item->valueint){
                // code: 1 修改操作
                if (strcasecmp(item_item->valuestring, "status") == 0){
                    // 修改状态
                    if (strcasecmp(values_item->valuestring, "stream") == 0){
                        ESP_LOGI(TAG, "进入推流模式");
                        device.status = DEVICE_ON_STREAM;
                        char return_data[128] = "{\"code\":1,\"msg\":\"进入推流模式.\",\"key\":\"status\",\"values\":\"stream\"}";
                        WebsocketSendText(return_data, strlen(return_data));
                    }else if (strcasecmp(values_item->valuestring, "standby") == 0){
                        ESP_LOGI(TAG, "进入待机模式");
                        device.status = DEVICE_STANDBY;
                        char return_data[128] = "{\"code\":1,\"msg\":\"进入待机模式.\",\"key\":\"status\",\"values\":\"standby\"}";
                        WebsocketSendText(return_data, strlen(return_data));
                    }                    
                }
            }else{
                // code: 0 读取操作
            }
            
        }
        else
        {
            ESP_LOGE(TAG, "返回的 JSON 缺少必要字段或类型错误");
            // 反馈状态
            char return_data[128] = "{\"code\":0,\"msg\":\"返回的 JSON 缺少必要字段或类型错误\",\"key\":\"\",\"values\":\"\"}";
            WebsocketSendText(return_data, strlen(return_data));
        }

        cJSON_Delete(json);
    }
    else
    {
        ESP_LOGI(TAG, "收到 JSON 数据, 解析失败");
        // 反馈状态
        // uint8_t return_data[128];
        // snprintf(return_data, sizeof(return_data), "{\"code\":0,\"msg\":\"JSON 数据解析失败\",\"key\":\"\",\"values\":\"%s\"}", data_ptr);
        char return_data[128] = "{\"code\":0,\"msg\":\"JSON 数据解析失败\",\"key\":\"\",\"values\":\"\"}";
        WebsocketSendText(return_data, strlen(return_data));
    }
}

void camera_transmit_task();

void app_main(void)
{
    printf("Hello ESP-IDF!\n");
    nvs_flash_init();
    WifistaInit("关闭", "kaiwen0818");
    // 等待wifi连接
    while (!Wifi_isConnected)
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    obtain_time();

    // 创建任务执行 HTTPS 请求
    WifiSecurityClientInit();
    // WifiSecurityRequest("https://file.mintlab.top", "/", 443, WS_CLINENT_METHOD_GET, NULL);

    // WifiSecurityRequest("http://192.168.216.109", "/?name=ESP-32", 8080, WS_CLINENT_METHOD_GET, NULL, NULL);

    // // 注册设备, 获得id
    // char post_data[64];
    // snprintf(post_data, sizeof(post_data), "{\"secret\":\"%s\"}", secret);
    // WifiSecurityRequest("http://192.168.216.109", "/device/register", 8080,
    //      WS_CLINENT_METHOD_POST, post_data, register_handler);

    // // 通过id开启ws连接
    // char path_data[128];
    // snprintf(path_data, sizeof(path_data), "/stream/device/ws?id=%s", device.uuid);
    // WebsocketStart("ws://192.168.216.109", path_data, 8080);

    // 通过id开启ws连接/stream/viewer/ws
    char path_data[128];
    snprintf(path_data, sizeof(path_data), "/stream/device/ws?secret=%s", secret);
    // 注册回调函数
    Websocket_event_handler_register(NULL, ws_text_handler);

    WebsocketStart("wss://lark.mintlab.top", path_data, 443);

    // 等待ws连接成功
    while (!WebsocketIsConnected())
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    CameraInit();
    sensor_t *s = esp_camera_sensor_get();
    s->set_framesize(s, FRAMESIZE_SVGA);

    static uint8_t ucParameterToPass;
    TaskHandle_t xHandle = NULL;
    xTaskCreate(camera_transmit_task, "camera_transmit_task", 4096, &ucParameterToPass, 1, &xHandle);
    configASSERT(xHandle);
}

void camera_transmit_task(){
    while (1)
    {
        if (device.status == DEVICE_ON_STREAM){
            CameraTakePhoto(PhotoTransmit);
        }else{
            ESP_LOGI("camera_transmit_task", "待机模式");
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
    }
}
