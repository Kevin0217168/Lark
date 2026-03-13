#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_camera.h"
#include "Wifista.h"
#include "nvs_flash.h"

void app_main(void)
{
    printf("Hello ESP-IDF!\n");

    nvs_flash_init();
    WifistaInit();
}