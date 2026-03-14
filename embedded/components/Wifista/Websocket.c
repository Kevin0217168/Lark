#include "Wifista.h"
#include "esp_websocket_client.h"

static const char *TAG = "WebSocket";

esp_websocket_client_handle_t Websocket_client;

// 事件处理函数
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WebSocket Connected");
        // 连接成功后，可以立即发送一条消息
        char msg[64] = "Hello Server! from ESP32.";
        esp_websocket_client_send_text(data->client, msg, strlen(msg), portMAX_DELAY);
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WebSocket Disconnected");
        break;
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(TAG, "Received data: opcode=%d, len=%d", data->op_code, data->data_len);
        if (data->op_code == 0x1)
        { // 文本帧
            ESP_LOGI(TAG, "Payload: %.*s", data->data_len, data->data_ptr);
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(TAG, "WebSocket Error");
        break;
    }
}

void WebsocketStart(const char *host, const char *path, uint16_t port)
{

    esp_websocket_client_config_t websocket_cfg = {
        .uri = host,
        .path = path,
        .port = port,
        .crt_bundle_attach = esp_crt_bundle_attach};

    // 初始化客户端
    Websocket_client = esp_websocket_client_init(&websocket_cfg);
    // 注册事件处理函数
    esp_websocket_register_events(Websocket_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, NULL);
    // 启动客户端，发起WSS连接
    ESP_ERROR_CHECK(esp_websocket_client_start(Websocket_client));

    while (1)
    {
        char msg[64] = "Hello Server! from ESP32.";
        esp_websocket_client_send_text(Websocket_client, msg, strlen(msg), portMAX_DELAY);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}