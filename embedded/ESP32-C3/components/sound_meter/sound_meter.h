#pragma once

#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

#define SOUND_METER_MAX_CAL_POINTS 16

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float voltage;
    float db;
} sound_meter_db_calibration_point_t;

typedef struct {
    adc_unit_t unit;
    adc_channel_t channel;
    adc_atten_t atten;
    adc_bitwidth_t bit_width;
    uint16_t sample_count;
    uint16_t discard_low;
    uint16_t discard_high;
    float full_scale_voltage;
    float max_db;
    bool use_adc_calibration;
    const sound_meter_db_calibration_point_t *calibration_points;
    size_t calibration_point_count;
} sound_meter_config_t;

typedef struct {
    sound_meter_config_t config;
    adc_oneshot_unit_handle_t adc_handle;
    adc_cali_handle_t cali_handle;
    sound_meter_db_calibration_point_t calibration_table[SOUND_METER_MAX_CAL_POINTS];
    size_t calibration_point_count;
    uint32_t last_db_raw;
    float last_db;
    bool initialized;
} sound_meter_handle_t;

void sound_meter_get_default_config(sound_meter_config_t *config);
esp_err_t sound_meter_init(sound_meter_handle_t *handle, const sound_meter_config_t *config);
void sound_meter_deinit(sound_meter_handle_t *handle);
esp_err_t sound_meter_read_raw(sound_meter_handle_t *handle, uint32_t *raw_out);
esp_err_t sound_meter_read_voltage(sound_meter_handle_t *handle, float *voltage_out);
esp_err_t sound_meter_read_db(sound_meter_handle_t *handle, float *db_out);

#ifdef __cplusplus
}
#endif
