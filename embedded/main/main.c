#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "Wifista.h"
#include "Camera.h"
#include "main.h"
#include "sht4x.h"

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

void camera_transmit_task();

extern void i2c_scan(void);

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

    // // 通过id开启ws连接/stream/viewer/ws
    // char path_data[128];
    // snprintf(path_data, sizeof(path_data), "/api/stream/device/ws?secret=%s", secret);
    // // 注册回调函数
    // Websocket_event_handler_register(NULL, ws_text_handler);

    // WebsocketStart("wss://lark.mintlab.top", path_data, 443);
    // WebsocketStart("ws://192.168.1.199", path_data, 8080);

    // 等待ws连接成功
    // while (!WebsocketIsConnected())
    // {
    //     vTaskDelay(500 / portTICK_PERIOD_MS);
    // }

    // CameraInit();
    // sensor_t *s = esp_camera_sensor_get();
    // s->set_framesize(s, FRAMESIZE_SVGA);
    // s->set_vflip(s, 1);

    // static uint8_t ucParameterToPass;
    // TaskHandle_t xHandle = NULL;
    // xTaskCreate(camera_transmit_task, "camera_transmit_task", 4096, &ucParameterToPass, 1, &xHandle);
    // configASSERT(xHandle);

    i2c_scan();

    /* Initialize the i2c bus for the current platform */
    sensirion_i2c_init();

    /* Busy loop for initialization, because the main loop does not work without
     * a sensor.
     */
    while (sht4x_probe() != STATUS_OK)
    {
        printf("SHT sensor probing failed\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("SHT sensor probing successful\n");

    while (1)
    {
        int32_t temperature, humidity;
        /* Measure temperature and relative humidity and store into variables
         * temperature, humidity (each output multiplied by 1000).
         */
        int8_t ret = sht4x_measure_blocking_read(&temperature, &humidity);
        if (ret == STATUS_OK)
        {
            printf("measured temperature: %0.2f degreeCelsius, "
                   "measured humidity: %0.2f percentRH\n",
                   temperature / 1000.0f, humidity / 1000.0f);
        }
        else
        {
            printf("error reading measurement\n");
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS); /* sleep 1s */
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
