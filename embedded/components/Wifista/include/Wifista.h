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

void WifistaInit();

void obtain_time(void);
void https_request_task(void *pvParameters);
esp_err_t WifiSecurityRequest(const char *host, const char *path, uint16_t port);

#endif