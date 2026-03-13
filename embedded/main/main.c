#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "Wifista.h"
#include "nvs_flash.h"

// #define SSID "关闭"
// #define PWD "kaiwen0818"

void app_main(void)
{
    printf("Hello ESP-IDF!\n");
    nvs_flash_init();
    WifistaInit("关闭", "kaiwen0818");
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    // 创建任务执行 HTTPS 请求
    xTaskCreate(&https_request_task, "https_task", 8192, NULL, 5, NULL);
}
