#pragma once

#include <stdint.h>
#include <esp_err.h>
#include <driver/i2c.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class SGP30 {
public:
    static constexpr uint8_t I2C_ADDRESS = 0x58;
    static constexpr int I2C_TIMEOUT_MS = 1000;

    SGP30(i2c_port_t i2c_num = I2C_NUM_0,
          gpio_num_t sda_gpio = GPIO_NUM_21,
          gpio_num_t scl_gpio = GPIO_NUM_20,
          uint32_t clk_speed = 100000,
          SemaphoreHandle_t i2c_mutex = NULL);

    esp_err_t init();
    esp_err_t measure(uint16_t *co2_ppm, uint16_t *tvoc_ppb);
    esp_err_t soft_reset(void);

private:
    i2c_port_t i2c_num;
    gpio_num_t sda_gpio;
    gpio_num_t scl_gpio;
    uint32_t clk_speed;
    SemaphoreHandle_t _i2c_mutex;

    esp_err_t i2c_master_init();
    esp_err_t write_command(const uint8_t *cmd, size_t cmd_len);
    esp_err_t write_raw_command(uint8_t addr, const uint8_t *cmd, size_t cmd_len);
    esp_err_t read_raw(uint8_t *data, size_t len);
    static uint8_t crc8(const uint8_t *data, size_t len);
};
