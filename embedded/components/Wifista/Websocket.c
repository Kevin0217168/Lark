#include "Wifista.h"
#include "esp_websocket_client.h"
#include "esp_timer.h"

static const char *TAG = "websocket";

esp_websocket_client_handle_t Websocket_client;
esp_websocket_client_config_t websocket_cfg;
WS_Context_t ws_context;

bool Websocket_isConnected = false;

// static void reconnect_timer_callback(void *arg)
// {
//     ESP_LOGI(TAG, "Attempting to reconnect WebSocket...");
//     // 销毁旧客户端（如果还存在）
//     if (Websocket_client != NULL)
//     {
//         esp_websocket_client_stop(Websocket_client);
//         esp_websocket_client_destroy(Websocket_client);
//         Websocket_client = NULL;
//     }
//     // 重新启动连接
//     WebsocketStart(websocket_cfg.host, websocket_cfg.path, websocket_cfg.port);
// }

void Websocket_event_handler_register(void (*ws_disconnected_handler)(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data),
                                       void (* ws_text_handler)(void *handler_args, int len, const char *data_ptr))
{
    ws_context.ws_text_handler = ws_text_handler;
    ws_context.ws_disconnected_handler = ws_disconnected_handler;
}

// 事件处理函数
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // 加载自定义上下文
    WS_Context_t *cxt = (WS_Context_t *)handler_args;
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:
        Websocket_isConnected = true;
        ESP_LOGI(TAG, "WS 已连接");
        // 连接成功后，可以立即发送一条消息
        char msg[64] = "Hello Server! from ESP32.";
        esp_websocket_client_send_text(data->client, msg, strlen(msg), portMAX_DELAY);
        break;

    case WEBSOCKET_EVENT_DISCONNECTED:
        Websocket_isConnected = false;
        ESP_LOGW(TAG, "WS 已断开");
        // ESP_LOGW(TAG, "Schedule reconnect in 3 seconds...");
        // // 创建一次性定时器，3秒后执行重连
        // const esp_timer_create_args_t timer_args = {
        //     .callback = &reconnect_timer_callback,
        //     .name = "ws_reconnect"};
        // esp_timer_handle_t reconnect_timer;
        // ESP_ERROR_CHECK(esp_timer_create(&timer_args, &reconnect_timer));
        // ESP_ERROR_CHECK(esp_timer_start_once(reconnect_timer, 3000 * 1000)); // 3秒
        break;

    case WEBSOCKET_EVENT_DATA:
        ESP_LOGD(TAG, "收到数据: opcode=%d, len=%d", data->op_code, data->data_len);
        if (data->op_code == 0x1)
        { // 文本帧
            ESP_LOGD(TAG, "payload: %.*s", data->data_len, data->data_ptr);
            if (cxt != NULL)
            {
                cxt->ws_text_handler(handler_args, data->data_len, data->data_ptr);
            }
        }

        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(TAG, "WS 错误");
        break;
    }
}

void WebsocketStart(const char *host, const char *path, uint16_t port)
{

    websocket_cfg.uri = host;
    websocket_cfg.path = path;
    websocket_cfg.port = port;
    websocket_cfg.crt_bundle_attach = esp_crt_bundle_attach;
    // websocket_cfg.disable_auto_reconnect = true;
    websocket_cfg.buffer_size = 2048;          // 收发缓冲区 2KB（收 JSON 指令足够，发送由 transport 层分片）
    websocket_cfg.task_stack = 6144;
    websocket_cfg.reconnect_timeout_ms = 5000; // 断连后 5 秒重连
    websocket_cfg.network_timeout_ms = 10000;  // 传输层读写超时 10 秒
    websocket_cfg.ping_interval_sec = 20;      // PING 间隔 20s——推流时发送密集，无需频繁 PING
    websocket_cfg.pingpong_timeout_sec = 40;   // PONG 等待 40s——推流密集写入可能延迟 PONG 处理
    websocket_cfg.enable_close_reconnect = true;
    websocket_cfg.task_prio = 6;               // WS 任务优先级 6

    // 初始化客户端
    Websocket_client = esp_websocket_client_init(&websocket_cfg);
    if (Websocket_client == NULL)
    {
        ESP_LOGE(TAG, "WS 客户端初始化失败");
        return;
    }
    // 注册事件处理函数
    ESP_ERROR_CHECK(esp_websocket_register_events(Websocket_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)&ws_context));
    // 启动客户端，发起WSS连接
    ESP_ERROR_CHECK(esp_websocket_client_start(Websocket_client));
}

bool WebsocketSendbytes(uint8_t *data, int len)
{
    return WebsocketSendbytesTimeout(data, len, pdMS_TO_TICKS(15000));
}

bool WebsocketSendbytesTimeout(uint8_t *data, int len, TickType_t timeout)
{
    if (Websocket_isConnected)
    {
        int num = esp_websocket_client_send_bin(Websocket_client, data, len, timeout);
        if (num != -1)
        {
            ESP_LOGD(TAG, "发送 bin: %d bytes", num);
            return true;
        }
        else
        {
            ESP_LOGD(TAG, "bin 发送超时/失败 (len=%d)", len);
            return false;
        }
    }
    else
    {
        ESP_LOGW(TAG, "bin 发送失败: 未连接");
        return false;
    }
}

bool WebsocketSendText(uint8_t *data, int len)
{
    if (Websocket_isConnected)
    {
        int num = esp_websocket_client_send_text(Websocket_client, data, len, portMAX_DELAY);
        if (num != -1)
        {
            ESP_LOGD(TAG, "发送 text: %d bytes", num);
            return true;
        }
        else
        {
            ESP_LOGE(TAG, "text 发送失败");
            return false;
        }
    }
    else
    {
        ESP_LOGW(TAG, "text 发送失败: 未连接");
        return false;
    }
}

bool WebsocketIsConnected()
{
    return Websocket_isConnected;
}
