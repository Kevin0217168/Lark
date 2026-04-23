#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>
#include "ina231.h"
#include "pms9103m.h"
#include "sgp30.h"
#include "sound_meter.h"
#include "uv_meter.h"
#include "Adafruit_VEML7700_espidf.h"

static const char *TAG = "sensor_hub";

#define I2C_MASTER_PORT I2C_NUM_0
#define I2C_MASTER_SDA  GPIO_NUM_21
#define I2C_MASTER_SCL  GPIO_NUM_20
#define I2C_MASTER_FREQ_HZ 100000
#define INA231_ADDR INA231_ADDRESS_DEFAULT



extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting integrated sensor hub");

    ESP_ERROR_CHECK(ina231_init(I2C_MASTER_PORT, I2C_MASTER_SDA, I2C_MASTER_SCL, I2C_MASTER_FREQ_HZ));

    const float max_current_a = 1.0f;
    const uint32_t shunt_uohm = 100000;
    const uint32_t current_lsb_nA = ina231_current_lsb_nA(max_current_a);

    ESP_LOGI(TAG, "INA231 config: max_current=%.3f A, shunt=%u uOhm, current_lsb=%u nA",
             max_current_a, shunt_uohm, current_lsb_nA);
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
    bus_calibration_t bus_cal = ina231_bus_calibration_compute(bus_cal_points, sizeof(bus_cal_points) / sizeof(bus_cal_points[0]));
    ESP_LOGI(TAG, "INA231 bus calibration: gain=%.6f offset=%.6f valid=%d",
             bus_cal.gain, bus_cal.offset, bus_cal.valid);

    ina231_dump_ina231_registers(I2C_MASTER_PORT, INA231_ADDR, current_lsb_nA);

    SGP30 sgp30(I2C_MASTER_PORT, I2C_MASTER_SDA, I2C_MASTER_SCL, I2C_MASTER_FREQ_HZ);
    bool sgp30_ready = (sgp30.init() == ESP_OK);
    if (!sgp30_ready) {
        ESP_LOGW(TAG, "SGP30 initialization failed");
    }

    Adafruit_VEML7700 veml(I2C_MASTER_PORT, I2C_MASTER_SDA, I2C_MASTER_SCL);
    bool veml_ready = veml.begin(I2C_MASTER_FREQ_HZ);
    if (!veml_ready) {
        ESP_LOGW(TAG, "VEML7700 initialization failed");
    }

    pms9103m_config_t pms_cfg = {
        .uart_num = UART_NUM_1,
        .tx_io_num = 17,
        .rx_io_num = 16,
        .baud_rate = 9600,
    };
    bool pms_ready = (pms9103m_init(&pms_cfg) == ESP_OK);
    if (!pms_ready) {
        ESP_LOGW(TAG, "PMS9103M initialization failed");
    }

    static const sound_meter_db_calibration_point_t sound_calibration_points[] = {
        { .voltage = 0.0f, .db = 0.0f },
        { .voltage = 0.5f, .db = 40.0f },
        { .voltage = 0.62f, .db = 49.0f },
        { .voltage = 1.29f, .db = 60.0f },
        { .voltage = 2.55f, .db = 68.0f },
        { .voltage = 2.58f, .db = 74.0f },
        { .voltage = 2.60f, .db = 77.0f },
    };

    sound_meter_handle_t sound_meter = {};
    sound_meter_config_t sound_cfg;
    sound_meter_get_default_config(&sound_cfg);
    sound_cfg.use_adc_calibration = true;
    sound_cfg.calibration_points = sound_calibration_points;
    sound_cfg.calibration_point_count = sizeof(sound_calibration_points) / sizeof(sound_calibration_points[0]);
    bool sound_ready = (sound_meter_init(&sound_meter, &sound_cfg) == ESP_OK);
    if (!sound_ready) {
        ESP_LOGW(TAG, "Sound meter initialization failed");
    }

    uv_meter_handle_t uv_meter = {};
    uv_meter_config_t uv_cfg;
    uv_meter_get_default_config(&uv_cfg);
    uv_cfg.channel = ADC_CHANNEL_4; // use a different ADC channel from the sound meter if both are present
    bool uv_ready = (uv_meter_init(&uv_meter, &uv_cfg) == ESP_OK);
    if (!uv_ready) {
        ESP_LOGW(TAG, "UV meter initialization failed");
    }

    while (true) {
        float bus_v = 0.0f;
        float shunt_v = 0.0f;
        float current_ma = 0.0f;
        float power_mw = 0.0f;

        if (ina231_read_bus_voltage_v(I2C_MASTER_PORT, INA231_ADDR, &bus_v) == ESP_OK &&
            ina231_read_shunt_voltage_v(I2C_MASTER_PORT, INA231_ADDR, &shunt_v) == ESP_OK &&
            ina231_read_current_ma(I2C_MASTER_PORT, INA231_ADDR, current_lsb_nA, &current_ma) == ESP_OK &&
            ina231_read_power_mw(I2C_MASTER_PORT, INA231_ADDR, current_lsb_nA, &power_mw) == ESP_OK) {
            float bus_v_cal = ina231_bus_voltage_calibrate(bus_v, &bus_cal);
            ESP_LOGI(TAG, "INA231: BUS=%.3fV (cal=%.3fV), SHUNT=%.6fV, CURRENT=%.3fmA, POWER=%.3fmW",
                     bus_v, bus_v_cal, shunt_v, current_ma, power_mw);
        } else {
            ESP_LOGW(TAG, "INA231 read failed");
        }

        if (sgp30_ready) {
            uint16_t co2 = 0;
            uint16_t tvoc = 0;
            if (sgp30.measure(&co2, &tvoc) == ESP_OK) {
                ESP_LOGI(TAG, "SGP30: CO2=%d ppm, TVOC=%d ppb", co2, tvoc);
            } else {
                ESP_LOGW(TAG, "SGP30 measure failed");
            }
        }

        if (veml_ready) {
            float lux = veml.readLux(VEML_LUX_AUTO);
            uint8_t gain = veml.getGain();
            uint8_t it = veml.getIntegrationTime();
            ESP_LOGI(TAG, "VEML7700: Lux=%.2f gain=%u int_time=%u", lux, gain, it);
        }

        if (pms_ready) {
            pms9103m_data_t data;
            if (pms9103m_read_frame(&data, pdMS_TO_TICKS(2000)) == ESP_OK) {
                ESP_LOGI(TAG, "PMS9103M: PM1.0=%u PM2.5=%u PM10=%u PM1.0_ATM=%u PM2.5_ATM=%u PM10_ATM=%u",
                         data.pm1_0_cf1, data.pm2_5_cf1, data.pm10_cf1,
                         data.pm1_0_atm, data.pm2_5_atm, data.pm10_atm);
            } else {
                ESP_LOGW(TAG, "PMS9103M read failed");
            }
        }

        if (sound_ready) {
            float voltage = 0.0f;
            float db = 0.0f;
            uint32_t raw = 0;
            if (sound_meter_read_raw(&sound_meter, &raw) == ESP_OK &&
                sound_meter_read_voltage(&sound_meter, &voltage) == ESP_OK &&
                sound_meter_read_db(&sound_meter, &db) == ESP_OK) {
                ESP_LOGI(TAG, "Sound: raw=%u voltage=%.3fV db=%.1f", raw, voltage, db);
            } else {
                ESP_LOGW(TAG, "Sound meter read failed");
            }
        }

        if (uv_ready) {
            float voltage = 0.0f;
            float uv_index = 0.0f;
            uint32_t raw = 0;
            if (uv_meter_read_raw(&uv_meter, &raw) == ESP_OK &&
                uv_meter_read_voltage(&uv_meter, &voltage) == ESP_OK &&
                uv_meter_read_uv_index(&uv_meter, &uv_index) == ESP_OK) {
                uv_meter_level_t level = uv_meter_get_level(uv_index);
                const char *level_str = uv_meter_level_to_string(level);
                ESP_LOGI(TAG, "UV: raw=%u voltage=%.3fV index=%.2f level=%s", raw, voltage, uv_index, level_str);
            } else {
                ESP_LOGW(TAG, "UV meter read failed");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
