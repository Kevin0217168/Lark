#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "ina231.h"

static const char *TAG = "ina231_demo";

#define I2C_MASTER_PORT I2C_NUM_0
#define I2C_MASTER_SDA  GPIO_NUM_21
#define I2C_MASTER_SCL  GPIO_NUM_20
#define INA231_ADDR     INA231_ADDRESS_DEFAULT

typedef struct {
    float measured_v;
    float reference_v;
} bus_calibration_point_t;

typedef struct {
    float gain;
    float offset;
    bool valid;
} bus_calibration_t;

static bus_calibration_t bus_calibration_compute(const bus_calibration_point_t *points, size_t count)
{
    bus_calibration_t cal = { .gain = 1.0f, .offset = 0.0f, .valid = false };
    if (points == NULL || count == 0) {
        return cal;
    }
    if (count == 1) {
        float measured = points[0].measured_v;
        cal.gain = (measured == 0.0f) ? 1.0f : (points[0].reference_v / measured);
        cal.offset = 0.0f;
        cal.valid = true;
        return cal;
    }

    float sum_x = 0.0f;
    float sum_y = 0.0f;
    float sum_xy = 0.0f;
    float sum_x2 = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        float x = points[i].measured_v;
        float y = points[i].reference_v;
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }

    float mean_x = sum_x / count;
    float mean_y = sum_y / count;
    float denom = sum_x2 - sum_x * mean_x;
    if (denom == 0.0f) {
        cal.gain = 1.0f;
        cal.offset = 0.0f;
        cal.valid = false;
        return cal;
    }

    cal.gain = (sum_xy - sum_x * mean_y) / denom;
    cal.offset = mean_y - cal.gain * mean_x;
    cal.valid = true;
    return cal;
}

static float bus_voltage_calibrate(float raw_bus_v, const bus_calibration_t *cal)
{
    if (cal == NULL || !cal->valid) {
        return raw_bus_v;
    }
    return raw_bus_v * cal->gain + cal->offset;
}

static void dump_registers(i2c_port_t port, uint8_t addr, uint32_t current_lsb_nA)
{
    uint16_t config = 0;
    uint16_t shunt = 0;
    uint16_t bus = 0;
    uint16_t power = 0;
    uint16_t current = 0;
    uint16_t calibration = 0;
    uint16_t mask = 0;

    ina231_read_register(port, addr, INA231_REG_CONFIGURATION, &config);
    ina231_read_register(port, addr, INA231_REG_SHUNT_VOLTAGE, &shunt);
    ina231_read_register(port, addr, INA231_REG_BUS_VOLTAGE, &bus);
    ina231_read_register(port, addr, INA231_REG_POWER, &power);
    ina231_read_register(port, addr, INA231_REG_CURRENT, &current);
    ina231_read_register(port, addr, INA231_REG_CALIBRATION, &calibration);
    ina231_read_register(port, addr, INA231_REG_MASK_ENABLE, &mask);

    ESP_LOGI(TAG, "REG dump: CONFIG=0x%04X SHUNT=0x%04X BUS=0x%04X POWER=0x%04X CURRENT=0x%04X CAL=0x%04X MASK=0x%04X",
             config, shunt, bus, power, current, calibration, mask);
    ESP_LOGI(TAG, "REG values: BUS=%.3f V, SHUNT=%.6f V, CURRENT_RAW=%d, POWER_RAW=%d, CAL=%d",
             bus * INA231_BUS_VOLTAGE_LSB_V,
             (int16_t)shunt * INA231_SHUNT_VOLTAGE_LSB_V,
             (int16_t)current,
             power,
             calibration);

    if (current != 0) {
        float current_ma = (int16_t)current * current_lsb_nA / 1000000.0f;
        ESP_LOGI(TAG, "CALCULATED CURRENT=%f mA from RAW=%d", current_ma, (int16_t)current);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(ina231_init(I2C_MASTER_PORT, I2C_MASTER_SDA, I2C_MASTER_SCL, INA231_I2C_DEFAULT_FREQ_HZ));

    const float max_current_a = 1.0f;
    const uint32_t shunt_uohm = 100000; // 0.1 ohm = 100000 micro-ohm
    const uint32_t current_lsb_nA = ina231_current_lsb_nA(max_current_a);

    ESP_LOGI(TAG, "max_current_a=%.3f A, shunt=%u uOhm, current_lsb=%u nA", max_current_a, shunt_uohm,
             current_lsb_nA);

    uint16_t calibration_value = ina231_calibration_value(current_lsb_nA, shunt_uohm);
    ESP_LOGI(TAG, "computed calibration value: 0x%04X (%u)", calibration_value, calibration_value);

    ESP_ERROR_CHECK(ina231_write_calibration(I2C_MASTER_PORT, INA231_ADDR, max_current_a, shunt_uohm));
    ESP_ERROR_CHECK(ina231_set_bus_conversion(I2C_MASTER_PORT, INA231_ADDR, 3320));
    ESP_ERROR_CHECK(ina231_set_shunt_conversion(I2C_MASTER_PORT, INA231_ADDR, 3320));
    ESP_ERROR_CHECK(ina231_set_mode(I2C_MASTER_PORT, INA231_ADDR, INA231_MODE_CONTINUOUS_BOTH));
    ESP_ERROR_CHECK(ina231_enable_alert_on_conversion(I2C_MASTER_PORT, INA231_ADDR, true));

    const bus_calibration_point_t bus_cal_points[] = {
        { .measured_v = 3.115f,  .reference_v = 3.3008f },
        { .measured_v = 4.918f,  .reference_v = 5.011f },
        { .measured_v = 11.866f, .reference_v = 12.010f },
        { .measured_v = 15.869f, .reference_v = 16.000f },
        { .measured_v = 17.887f, .reference_v = 18.009f },
    };
    // 电压(V)
    // 实际       读数     误差
    // 3.3008f - 3.286f = 0.015f
    // 5.000f - 5.011f = 0.011f
    // 11.998f - 11.923f = 0.075f
    // 15.999f - 15.997f = 0.002f
    // 17.999f - 17.988f = 0.001f
    
    // 电流(ma)
    // 3.157f - 3.173f = -0.016f
    // 5.015f - 5.065f = -0.050f
    // 12.031f - 12.115f = -0.084f
    // 16.143f - 16.235f = -0.092f
    // 18.232f - 18.371f = -0.139f

    const size_t bus_cal_point_count = sizeof(bus_cal_points) / sizeof(bus_cal_points[0]);
    bus_calibration_t bus_cal = bus_calibration_compute(bus_cal_points, bus_cal_point_count);
    ESP_LOGI(TAG, "BUS calibration: gain=%.6f offset=%.6f valid=%d",
             bus_cal.gain, bus_cal.offset, bus_cal.valid);

    dump_registers(I2C_MASTER_PORT, INA231_ADDR, current_lsb_nA);

    while (true) {
        float bus_v = 0.0f;
        float shunt_v = 0.0f;
        float current_ma = 0.0f;
        float power_mw = 0.0f;

        if (ina231_read_bus_voltage_v(I2C_MASTER_PORT, INA231_ADDR, &bus_v) != ESP_OK) {
            ESP_LOGE(TAG, "failed to read bus voltage");
        }
        if (ina231_read_shunt_voltage_v(I2C_MASTER_PORT, INA231_ADDR, &shunt_v) != ESP_OK) {
            ESP_LOGE(TAG, "failed to read shunt voltage");
        }
        if (ina231_read_current_ma(I2C_MASTER_PORT, INA231_ADDR, current_lsb_nA, &current_ma) != ESP_OK) {
            ESP_LOGE(TAG, "failed to read current");
        }
        if (ina231_read_power_mw(I2C_MASTER_PORT, INA231_ADDR, current_lsb_nA, &power_mw) != ESP_OK) {
            ESP_LOGE(TAG, "failed to read power");
        }

        float bus_v_calibrated = bus_voltage_calibrate(bus_v, &bus_cal);
        ESP_LOGI(TAG, "BUS: %.3f V (calibrated: %.3f V), SHUNT: %.6f V, CURRENT: %.3f mA, POWER: %.3f mW",
                 bus_v, bus_v_calibrated, shunt_v, current_ma, power_mw);
        dump_registers(I2C_MASTER_PORT, INA231_ADDR, current_lsb_nA);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
