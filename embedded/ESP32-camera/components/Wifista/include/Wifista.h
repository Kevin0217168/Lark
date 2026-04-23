#ifndef __WIFISTA_H_
#define __WIFISTA_H_

#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"

typedef enum{
    WS_CLINENT_METHOD_GET,
    WS_CLINENT_METHOD_POST,
    WS_CLINENT_METHOD_PUT,
    WS_CLINENT_METHOD_DELETE,
} WifiSecurityMethod_t;

typedef struct 
{
    esp_http_client_handle_t* client;   // 指向全局客户端句柄
    bool is_json;                        // 是否为 JSON 响应
    cJSON* json;                          // 解析后的 JSON 对象（若成功）
    char *buffer;                         // 动态累积响应数据的缓冲区
    size_t buffer_size;                   // 当前已接收数据长度
    size_t buffer_capacity;                // 缓冲区总容量
} RequestContext_t;

typedef struct 
{
    void (* ws_disconnected_handler)(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
    void (* ws_text_handler)(void *handler_args, int len, const char *data_ptr);
}WS_Context_t;

void WifistaInit();
void obtain_time(void);
bool is_time_synced(void);
bool is_time_valid(void);
void WifiSecurityClientInit();
esp_err_t WifiSecurityRequest(const char *host, const char *path, uint16_t port, WifiSecurityMethod_t method,
     char *post_data, void (ResponseUserHandler)(RequestContext_t*));

void WebsocketStart(const char *host, const char *path, uint16_t port);
void Websocket_event_handler_register(void (*ws_disconnected_handler)(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data),
                                       void (* ws_text_handler)(void *handler_args, int len, const char *data_ptr));
void ws_text_handler(void *handler_args, int len, const char *data_ptr);
bool WebsocketIsConnected();
bool WebsocketSendbytes(uint8_t *data, int len);
bool WebsocketSendText(uint8_t *data, int len);

void ota_task(void *pvParameter);

#endif