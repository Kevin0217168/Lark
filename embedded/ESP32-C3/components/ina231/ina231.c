#include "ina231.h"
#include <driver/i2c.h>
#include <esp_err.h>
#include <esp_log.h>
#include <stdbool.h>
#include <string.h>

static const char *TAG = "ina231";

static esp_err_t ina231_i2c_write16(i2c_port_t port, uint8_t addr, uint8_t reg, uint16_t value) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        return ESP_ERR_NO_MEM;
    }
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, (value >> 8) & 0xFF, true);
    i2c_master_write_byte(cmd, value & 0xFF, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return err;
}

static esp_err_t ina231_i2c_read16(i2c_port_t port, uint8_t addr, uint8_t reg, uint16_t *value) {
    if (value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        return ESP_ERR_NO_MEM;
    }

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);

    uint8_t data[2];
    i2c_master_read(cmd, data, 2, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    if (err == ESP_OK) {
        *value = ((uint16_t)data[0] << 8) | data[1];
    }
    return err;
}

esp_err_t ina231_init(i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio, uint32_t freq_hz) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_gpio,
        .scl_io_num = scl_gpio,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = freq_hz,
    };
    esp_err_t err = i2c_param_config(port, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(err));
        return err;
    }
    err = i2c_driver_install(port, I2C_MODE_MASTER, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t ina231_write_register(i2c_port_t port, uint8_t addr, uint8_t reg, uint16_t value) {
    return ina231_i2c_write16(port, addr, reg, value);
}

esp_err_t ina231_read_register(i2c_port_t port, uint8_t addr, uint8_t reg, uint16_t *value) {
    return ina231_i2c_read16(port, addr, reg, value);
}

uint16_t ina231_rate_for_conversion(uint32_t conv_us) {
    if (conv_us >= 8244) return 7;
    if (conv_us >= 4156) return 6;
    if (conv_us >= 2116) return 5;
    if (conv_us >= 1100) return 4;
    if (conv_us >= 588)  return 3;
    if (conv_us >= 332)  return 2;
    if (conv_us >= 204)  return 1;
    return 0;
}

uint32_t ina231_current_lsb_nA(float max_current_a) {
    if (max_current_a <= 0.0f) {
        return 0;
    }
    return (uint32_t)((max_current_a * 1000000000.0f) / 32767.0f);
}

uint16_t ina231_calibration_value(uint32_t current_lsb_nA, uint32_t shunt_uohm) {
    if (current_lsb_nA == 0 || shunt_uohm == 0) {
        return 0;
    }
    uint64_t denom = (uint64_t)current_lsb_nA * shunt_uohm;
    return (uint16_t)((51200000ULL * 100000ULL) / denom);
}

uint32_t ina231_power_lsb_nW(uint32_t current_lsb_nA) {
    return current_lsb_nA * 25U;
}

esp_err_t ina231_write_calibration(i2c_port_t port, uint8_t addr, float max_current_a,
                                   uint32_t shunt_uohm) {
    uint32_t current_lsb_nA = ina231_current_lsb_nA(max_current_a);
    uint16_t calibration = ina231_calibration_value(current_lsb_nA, shunt_uohm);
    return ina231_i2c_write16(port, addr, INA231_REG_CALIBRATION, calibration);
}

static esp_err_t ina231_update_config(i2c_port_t port, uint8_t addr, uint16_t mask,
                                      uint16_t value) {
    uint16_t config;
    esp_err_t err = ina231_i2c_read16(port, addr, INA231_REG_CONFIGURATION, &config);
    if (err != ESP_OK) {
        return err;
    }
    config &= ~mask;
    config |= value;
    return ina231_i2c_write16(port, addr, INA231_REG_CONFIGURATION, config);
}

esp_err_t ina231_set_bus_conversion(i2c_port_t port, uint8_t addr, uint32_t conv_us) {
    uint16_t rate = ina231_rate_for_conversion(conv_us);
    return ina231_update_config(port, addr, INA231_CONFIG_BADC_MASK, rate << 7);
}

esp_err_t ina231_set_shunt_conversion(i2c_port_t port, uint8_t addr, uint32_t conv_us) {
    uint16_t rate = ina231_rate_for_conversion(conv_us);
    return ina231_update_config(port, addr, INA231_CONFIG_SADC_MASK, rate << 3);
}

esp_err_t ina231_set_mode(i2c_port_t port, uint8_t addr, ina231_mode_t mode) {
    return ina231_update_config(port, addr, INA231_CONFIG_MODE_MASK, mode & INA231_CONFIG_MODE_MASK);
}

esp_err_t ina231_enable_alert_on_conversion(i2c_port_t port, uint8_t addr, bool enable) {
    uint16_t mask_enable;
    esp_err_t err = ina231_i2c_read16(port, addr, INA231_REG_MASK_ENABLE, &mask_enable);
    if (err != ESP_OK) {
        return err;
    }
    if (enable) {
        mask_enable |= INA231_ALERT_CONVERSION_BIT;
    } else {
        mask_enable &= ~INA231_ALERT_CONVERSION_BIT;
    }
    return ina231_i2c_write16(port, addr, INA231_REG_MASK_ENABLE, mask_enable);
}

esp_err_t ina231_read_bus_voltage_v(i2c_port_t port, uint8_t addr, float *volts) {
    if (volts == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t raw;
    esp_err_t err = ina231_i2c_read16(port, addr, INA231_REG_BUS_VOLTAGE, &raw);
    if (err != ESP_OK) {
        return err;
    }
    *volts = raw * INA231_BUS_VOLTAGE_LSB_V;
    return ESP_OK;
}

esp_err_t ina231_read_shunt_voltage_v(i2c_port_t port, uint8_t addr, float *volts) {
    if (volts == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    int16_t raw;
    esp_err_t err = ina231_i2c_read16(port, addr, INA231_REG_SHUNT_VOLTAGE, (uint16_t *)&raw);
    if (err != ESP_OK) {
        return err;
    }
    *volts = raw * INA231_SHUNT_VOLTAGE_LSB_V;
    return ESP_OK;
}

esp_err_t ina231_read_current_ma(i2c_port_t port, uint8_t addr, float current_lsb_nA,
                                  float *milliamps) {
    if (milliamps == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    int16_t raw;
    esp_err_t err = ina231_i2c_read16(port, addr, INA231_REG_CURRENT, (uint16_t *)&raw);
    if (err != ESP_OK) {
        return err;
    }
    *milliamps = raw * current_lsb_nA / 1000000.0f;
    return ESP_OK;
}

esp_err_t ina231_read_power_mw(i2c_port_t port, uint8_t addr, uint32_t current_lsb_nA,
                                float *milliwatts) {
    if (milliwatts == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    int16_t raw;
    esp_err_t err = ina231_i2c_read16(port, addr, INA231_REG_POWER, (uint16_t *)&raw);
    if (err != ESP_OK) {
        return err;
    }
    uint32_t power_lsb_nW = ina231_power_lsb_nW(current_lsb_nA);
    *milliwatts = raw * power_lsb_nW / 1000000.0f;
    return ESP_OK;
}
