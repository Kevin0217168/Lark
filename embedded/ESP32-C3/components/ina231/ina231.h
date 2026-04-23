#pragma once

#include <driver/i2c.h>
#include <esp_err.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INA231_ADDRESS_DEFAULT 0x40
#define INA231_I2C_DEFAULT_FREQ_HZ 400000

#define INA231_REG_CONFIGURATION     0x00
#define INA231_REG_SHUNT_VOLTAGE     0x01
#define INA231_REG_BUS_VOLTAGE       0x02
#define INA231_REG_POWER             0x03
#define INA231_REG_CURRENT           0x04
#define INA231_REG_CALIBRATION       0x05
#define INA231_REG_MASK_ENABLE       0x06
#define INA231_REG_ALERT_LIMIT       0x07

#define INA231_CONFIG_MODE_MASK      0x0007
#define INA231_CONFIG_BADC_MASK      0x01C0
#define INA231_CONFIG_SADC_MASK      0x0038
#define INA231_ALERT_CONVERSION_BIT  (1 << 10)

#define INA231_BUS_VOLTAGE_LSB_V     0.00125f
#define INA231_SHUNT_VOLTAGE_LSB_V   0.0000025f

typedef struct {
    float measured_v;
    float reference_v;
} bus_calibration_point_t;

typedef struct {
    float gain;
    float offset;
    bool valid;
} bus_calibration_t;

typedef enum {
    INA231_MODE_SHUTDOWN = 0,
    INA231_MODE_TRIGGERED_SHUNT = 1,
    INA231_MODE_TRIGGERED_BUS = 2,
    INA231_MODE_TRIGGERED_BOTH = 3,
    INA231_MODE_POWER_DOWN = 4,
    INA231_MODE_CONTINUOUS_SHUNT = 5,
    INA231_MODE_CONTINUOUS_BUS = 6,
    INA231_MODE_CONTINUOUS_BOTH = 7,
} ina231_mode_t;

bus_calibration_t ina231_bus_calibration_compute(const bus_calibration_point_t *points, size_t count);
float ina231_bus_voltage_calibrate(float raw_bus_v, const bus_calibration_t *cal);
void ina231_dump_ina231_registers(i2c_port_t port, uint8_t addr, uint32_t current_lsb_nA);

esp_err_t ina231_init(i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio, uint32_t freq_hz);
esp_err_t ina231_write_register(i2c_port_t port, uint8_t addr, uint8_t reg, uint16_t value);
esp_err_t ina231_read_register(i2c_port_t port, uint8_t addr, uint8_t reg, uint16_t *value);

uint16_t ina231_rate_for_conversion(uint32_t conv_us);
uint32_t ina231_current_lsb_nA(float max_current_a);
uint16_t ina231_calibration_value(uint32_t current_lsb_nA, uint32_t shunt_uohm);
uint32_t ina231_power_lsb_nW(uint32_t current_lsb_nA);

esp_err_t ina231_write_calibration(i2c_port_t port, uint8_t addr, float max_current_a,
                                   uint32_t shunt_uohm);
esp_err_t ina231_set_bus_conversion(i2c_port_t port, uint8_t addr, uint32_t conv_us);
esp_err_t ina231_set_shunt_conversion(i2c_port_t port, uint8_t addr, uint32_t conv_us);
esp_err_t ina231_set_mode(i2c_port_t port, uint8_t addr, ina231_mode_t mode);
esp_err_t ina231_enable_alert_on_conversion(i2c_port_t port, uint8_t addr, bool enable);

esp_err_t ina231_read_bus_voltage_v(i2c_port_t port, uint8_t addr, float *volts);
esp_err_t ina231_read_shunt_voltage_v(i2c_port_t port, uint8_t addr, float *volts);
esp_err_t ina231_read_current_ma(i2c_port_t port, uint8_t addr, float current_lsb_nA,
                                  float *milliamps);
esp_err_t ina231_read_power_mw(i2c_port_t port, uint8_t addr, uint32_t current_lsb_nA,
                                float *milliwatts);

#ifdef __cplusplus
}
#endif
