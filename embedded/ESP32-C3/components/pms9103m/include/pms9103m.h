#pragma once

#include <stdint.h>
#include "driver/uart.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PMS9103M_MODE_ACTIVE = 0,
    PMS9103M_MODE_PASSIVE = 1,
} pms9103m_mode_t;

typedef struct {
    uart_port_t uart_num;
    int tx_io_num;
    int rx_io_num;
    int baud_rate;
} pms9103m_config_t;

typedef struct {
    uint16_t pm1_0_cf1;
    uint16_t pm2_5_cf1;
    uint16_t pm10_cf1;
    uint16_t pm1_0_atm;
    uint16_t pm2_5_atm;
    uint16_t pm10_atm;
    uint16_t count_0_3um;
    uint16_t count_0_5um;
    uint16_t count_1_0um;
    uint16_t count_2_5um;
    uint16_t count_5_0um;
    uint16_t count_10_0um;
    uint8_t version;
    uint8_t error_code;
} pms9103m_data_t;

esp_err_t pms9103m_init(const pms9103m_config_t *config);
esp_err_t pms9103m_read_frame(pms9103m_data_t *data, TickType_t ticks_to_wait);
esp_err_t pms9103m_request_passive_data(pms9103m_data_t *data, TickType_t ticks_to_wait);
esp_err_t pms9103m_set_mode(pms9103m_mode_t mode, TickType_t ticks_to_wait);

#ifdef __cplusplus
}
#endif
