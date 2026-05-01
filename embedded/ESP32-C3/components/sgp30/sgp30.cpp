#include "sgp30.h"
#include <string.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "SGP30";

SGP30::SGP30(i2c_port_t i2c_num, gpio_num_t sda_gpio, gpio_num_t scl_gpio, uint32_t clk_speed, SemaphoreHandle_t i2c_mutex)
    : i2c_num(i2c_num), sda_gpio(sda_gpio), scl_gpio(scl_gpio), clk_speed(clk_speed), _i2c_mutex(i2c_mutex) {}

esp_err_t SGP30::i2c_master_init()
{
    /* I2C 驱动由 main 中的 ina231_init 统一安装，此处直接复用 */
    return ESP_OK;
}

esp_err_t SGP30::init()
{
    esp_err_t err = i2c_master_init();
    if (err != ESP_OK) {
        return err;
    }

    const uint8_t cmd[] = {0x20, 0x03};
    err = write_command(cmd, sizeof(cmd));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SGP30 initialization command failed");
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG, "SGP30 initialized");
    return ESP_OK;
}

esp_err_t SGP30::write_command(const uint8_t *cmd, size_t cmd_len)
{
    return write_raw_command(I2C_ADDRESS, cmd, cmd_len);
}

esp_err_t SGP30::write_raw_command(uint8_t addr, const uint8_t *cmd, size_t cmd_len)
{
    i2c_cmd_handle_t handle = i2c_cmd_link_create();
    i2c_master_start(handle);
    i2c_master_write_byte(handle, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(handle, cmd, cmd_len, true);
    i2c_master_stop(handle);

    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (_i2c_mutex) {
        if (xSemaphoreTake(_i2c_mutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) == pdTRUE) {
            err = i2c_master_cmd_begin(i2c_num, handle, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
            xSemaphoreGive(_i2c_mutex);
        }
    } else {
        err = i2c_master_cmd_begin(i2c_num, handle, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    }

    i2c_cmd_link_delete(handle);
    return err;
}

esp_err_t SGP30::read_raw(uint8_t *data, size_t len)
{
    i2c_cmd_handle_t handle = i2c_cmd_link_create();
    i2c_master_start(handle);
    i2c_master_write_byte(handle, (I2C_ADDRESS << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(handle, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(handle, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(handle);

    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (_i2c_mutex) {
        if (xSemaphoreTake(_i2c_mutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) == pdTRUE) {
            err = i2c_master_cmd_begin(i2c_num, handle, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
            xSemaphoreGive(_i2c_mutex);
        }
    } else {
        err = i2c_master_cmd_begin(i2c_num, handle, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    }

    i2c_cmd_link_delete(handle);
    return err;
}

esp_err_t SGP30::measure(uint16_t *co2_ppm, uint16_t *tvoc_ppb)
{
    if (co2_ppm == nullptr || tvoc_ppb == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t cmd[] = {0x20, 0x08};
    esp_err_t err = write_command(cmd, sizeof(cmd));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SGP30 measure command failed: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(15));

    uint8_t data[6];
    err = read_raw(data, sizeof(data));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SGP30 read failed: %s", esp_err_to_name(err));
        return err;
    }

    if (crc8(data, 2) != data[2] || crc8(data + 3, 2) != data[5]) {
        ESP_LOGW(TAG, "SGP30 CRC mismatch");
        // 仍然尝试返回数据
    }

    *co2_ppm = (uint16_t)data[0] << 8 | data[1];
    *tvoc_ppb = (uint16_t)data[3] << 8 | data[4];
    return ESP_OK;
}

esp_err_t SGP30::soft_reset(void)
{
    /* SGP30 通用呼叫复位: 向地址 0x00 写入 0x0006 */
    const uint8_t cmd[] = {0x00, 0x06};
    esp_err_t err = write_raw_command(0x00, cmd, sizeof(cmd));
    if (err == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(10));
        ESP_LOGI(TAG, "SGP30 soft reset OK");
    } else {
        ESP_LOGW(TAG, "SGP30 soft reset failed: %s", esp_err_to_name(err));
    }
    return err;
}

uint8_t SGP30::crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}
