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

typedef enum{
    WS_CLINENT_METHOD_GET,
    WS_CLINENT_METHOD_POST,
    WS_CLINENT_METHOD_PUT,
    WS_CLINENT_METHOD_DELETE,
} WifiSecurityMethod_t;

void WifistaInit();

void obtain_time(void);

void WifiSecurityClientInit();
esp_err_t WifiSecurityRequest(const char *host, const char *path, uint16_t port, WifiSecurityMethod_t method, char *post_data);
void WebsocketStart(const char *host, const char *path, uint16_t port);

#endif