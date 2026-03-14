#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "Wifista.h"
#include "nvs_flash.h"
#include "main.h"

static const char* TAG = "main";
static const char *secret = "ae2823b8b2e14205b5e75d30c68d282d";
extern bool Wifi_isConnected;

DeviceStatus_t deviceStatus = {
    .name = "ESP32 - Default",
    .uuid = "",
    .isOnline = false,
}; 

void register_handler(RequestContext_t *ctx)
{
    if (esp_http_client_get_status_code(*(ctx->client)) != 200) {
        ESP_LOGE(TAG, "注册失败, 状态码: %d", esp_http_client_get_status_code(*(ctx->client)));
        return;
    }

    if (ctx->is_json && ctx->json != NULL) {
        // 根据实际返回字段名修改
        cJSON *id_item = cJSON_GetObjectItemCaseSensitive(ctx->json, "id");
        cJSON *name_item = cJSON_GetObjectItemCaseSensitive(ctx->json, "name");
        cJSON *status_item = cJSON_GetObjectItemCaseSensitive(ctx->json, "status");

        if (cJSON_IsString(id_item) && cJSON_IsString(name_item) && cJSON_IsString(status_item)) {
            // 注意：需要拷贝字符串，因为 ctx->json 即将被释放
            strncpy(deviceStatus.uuid, id_item->valuestring, sizeof(deviceStatus.uuid) - 1);
            deviceStatus.uuid[sizeof(deviceStatus.uuid) - 1] = '\0';
            strncpy(deviceStatus.name, name_item->valuestring, sizeof(deviceStatus.name) - 1);
            deviceStatus.name[sizeof(deviceStatus.name) - 1] = '\0';
            deviceStatus.isOnline = (strcasecmp(status_item->valuestring, "online") == 0);

            ESP_LOGI(TAG, "注册成功, 与服务器同步数据:\n名称: %s\nID: %s\n状态: %s",
                     deviceStatus.name, deviceStatus.uuid,
                     deviceStatus.isOnline ? "online" : "offline");
        } else {
            ESP_LOGE(TAG, "返回的 JSON 缺少必要字段或类型错误");
        }
    } else {
        ESP_LOGW(TAG, "响应不是有效的 JSON 对象");
    }
}

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

    WifiSecurityRequest("http://192.168.216.109", "/?name=ESP-32", 8080, WS_CLINENT_METHOD_GET, NULL, NULL);

    char post_data[128];
    snprintf(post_data, sizeof(post_data), "{\"secret\":\"%s\"}", secret);
    WifiSecurityRequest("http://192.168.216.109", "/device/register", 8080,
         WS_CLINENT_METHOD_POST, post_data, register_handler);
}
