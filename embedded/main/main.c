#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "Wifista.h"
#include "nvs_flash.h"
#include "main.h"

static const char* secret = "ae2823b8b2e14205b5e75d30c68d282d";


void app_main(void)
{
    printf("Hello ESP-IDF!\n");
    nvs_flash_init();
    WifistaInit("关闭", "kaiwen0818");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    obtain_time();


    // 创建任务执行 HTTPS 请求
    WifiSecurityClientInit();
    // WifiSecurityRequest("https://file.mintlab.top", "/", 443, WS_CLINENT_METHOD_GET, NULL);

    WifiSecurityRequest("http://192.168.216.109", "/?name=ESP-32", 8080, WS_CLINENT_METHOD_GET, NULL);

    char post_data[128];
    snprintf(post_data, sizeof(post_data), "{\"secret\":\"%s\"}", secret); 
    WifiSecurityRequest("http://192.168.216.109", "/device/register", 8080, WS_CLINENT_METHOD_POST, post_data);
}
