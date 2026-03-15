#include "Wifista.h"
#include "esp_websocket_client.h"

static const char *TAG = "WebSocket";

esp_websocket_client_handle_t Websocket_client;
esp_websocket_client_config_t websocket_cfg;

bool Websocket_isConnected = false;

// 事件处理函数
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:
        Websocket_isConnected = true;
        ESP_LOGI(TAG, "WebSocket Connected");
        // 连接成功后，可以立即发送一条消息
        char msg[64] = "Hello Server! from ESP32.";
        esp_websocket_client_send_text(data->client, msg, strlen(msg), portMAX_DELAY);
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        Websocket_isConnected = false;
        ESP_LOGE(TAG, "WebSocket Disconnected");
        ESP_LOGW(TAG, "准备自动重连");
        WebsocketStart(websocket_cfg.host, websocket_cfg.path, websocket_cfg.port);
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

    websocket_cfg.uri = host;
    websocket_cfg.path = path;
    websocket_cfg.port = port;
    websocket_cfg.crt_bundle_attach = esp_crt_bundle_attach;

    // 初始化客户端
    Websocket_client = esp_websocket_client_init(&websocket_cfg);
    if (Websocket_client == NULL){
        ESP_LOGE(TAG, "Websocket_client == NULL");
        return;
    }
    // 注册事件处理函数
    ESP_ERROR_CHECK(esp_websocket_register_events(Websocket_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, NULL));
    // 启动客户端，发起WSS连接
    ESP_ERROR_CHECK(esp_websocket_client_start(Websocket_client));
}

bool WebsocketSendbytes(uint8_t *data, int len)
{
    if (Websocket_isConnected)
    {
        int num = esp_websocket_client_send_bin(Websocket_client, data, len, portMAX_DELAY);
        if (num != -1)
        {
            ESP_LOGI(TAG, "发送成功, 共传输: %d bytes", num);
            return true;
        }
        else
        {
            ESP_LOGE(TAG, "发送失败");
            return false;
        }
    }
    else
    {
        ESP_LOGE(TAG, "发送失败, 连接还未建立");
        return false;
    }
}

bool WebsocketIsConnected()
{
    return Websocket_isConnected;
}
