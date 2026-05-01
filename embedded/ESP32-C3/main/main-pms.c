#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "pms9103m.h"

static const char *TAG = "PMS9103M_EXAMPLE";

void app_main(void)
{
    pms9103m_config_t config = {
        .uart_num = UART_NUM_1,
        .tx_io_num = 17,
        .rx_io_num = 16,
        .baud_rate = 9600,
    };

    esp_err_t err = pms9103m_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PMS9103M: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "PMS9103M UART initialized on UART%d", config.uart_num);

    pms9103m_data_t data;
    while (true) {
        err = pms9103m_read_frame(&data, pdMS_TO_TICKS(5000));
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "PM1.0(CF=1)=%u µg/m3  PM2.5(CF=1)=%u µg/m3  PM10(CF=1)=%u µg/m3",
                     data.pm1_0_cf1, data.pm2_5_cf1, data.pm10_cf1);
            ESP_LOGI(TAG, "PM1.0(ATM)=%u µg/m3  PM2.5(ATM)=%u µg/m3  PM10(ATM)=%u µg/m3",
                     data.pm1_0_atm, data.pm2_5_atm, data.pm10_atm);
            ESP_LOGI(TAG, "Count>0.3µm=%u  Count>0.5µm=%u  Count>1.0µm=%u  Count>2.5µm=%u  Count>5.0µm=%u  Count>10µm=%u",
                     data.count_0_3um, data.count_0_5um, data.count_1_0um,
                     data.count_2_5um, data.count_5_0um, data.count_10_0um);
            ESP_LOGI(TAG, "Version=%u  ErrorCode=%u", data.version, data.error_code);
        } else {
            ESP_LOGW(TAG, "Read failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
