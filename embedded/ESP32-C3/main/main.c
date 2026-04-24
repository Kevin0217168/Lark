#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "ina231.h"
#include "tasks.h"

static const char *TAG = "sensor_hub";

#define I2C_MASTER_PORT    I2C_NUM_0
#define I2C_MASTER_SDA     GPIO_NUM_21
#define I2C_MASTER_SCL     GPIO_NUM_20
#define I2C_MASTER_FREQ_HZ 100000

void app_main(void)
{
    ESP_LOGI(TAG, "启动传感器集线器(C)");

    ESP_ERROR_CHECK(ina231_init(I2C_MASTER_PORT, I2C_MASTER_SDA,
                                I2C_MASTER_SCL, I2C_MASTER_FREQ_HZ));

    sensor_tasks_init();

    xTaskCreate(task_ina231,      "ina231",      4096, NULL, 5, NULL);
    xTaskCreate(task_sgp30,       "sgp30",       4096, NULL, 5, NULL);
    xTaskCreate(task_veml7700,    "veml7700",    4096, NULL, 5, NULL);
    xTaskCreate(task_pms9103m,    "pms9103m",    4096, NULL, 5, NULL);
    xTaskCreate(task_sound_meter, "sound_meter", 4096, NULL, 5, NULL);
    xTaskCreate(task_uv_meter,    "uv_meter",    4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "所有传感器任务已启动");
}
