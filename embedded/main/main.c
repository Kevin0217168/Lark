#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "Wifista.h"

#define SSID "关闭"
#define PWD "kaiwen0818"

typedef struct
{
    const char *ssid;
    const char *pwd;
} Wifi_config_t;

const char *WIFI_TAG = "WIFI_task";

void WIFI_task(void *pvParam)
{
    Wifi_config_t *wifi_cfg = (Wifi_config_t *)pvParam;

    nvs_flash_init();
    WifistaInit(wifi_cfg->ssid, wifi_cfg->pwd);

    // 删除自己
    vTaskDelete(NULL);
}

static Wifi_config_t wifi_cfg = {
    .ssid = SSID,
    .pwd = PWD
};

void app_main(void)
{
    printf("Hello ESP-IDF!\n");
    
    TaskHandle_t WIFI_task_handle = NULL;
    xTaskCreate(WIFI_task, "WIFI_task", 4096, &wifi_cfg, 1, &WIFI_task_handle);
}
