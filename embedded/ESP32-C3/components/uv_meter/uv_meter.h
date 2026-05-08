#pragma once

#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UV_METER_MAX_CAL_POINTS 16

typedef struct {
    float voltage_mv;
    float uv_index;
} uv_meter_calibration_point_t;

typedef enum {
    UV_METER_LEVEL_LOW = 0,
    UV_METER_LEVEL_MODERATE,
    UV_METER_LEVEL_HIGH,
    UV_METER_LEVEL_VERY_HIGH,
    UV_METER_LEVEL_EXTREME,
} uv_meter_level_t;

typedef struct {
    adc_unit_t unit;
    adc_channel_t channel;
    adc_atten_t atten;
    adc_bitwidth_t bit_width;
    uint16_t sample_count;
    uint16_t discard_low;
    uint16_t discard_high;
    float full_scale_voltage;
    bool use_adc_calibration;
    const uv_meter_calibration_point_t *calibration_points;
    size_t calibration_point_count;
} uv_meter_config_t;

typedef struct {
    uv_meter_config_t config;
    adc_oneshot_unit_handle_t adc_handle;
    adc_cali_handle_t cali_handle;
    uv_meter_calibration_point_t calibration_table[UV_METER_MAX_CAL_POINTS];
    size_t calibration_point_count;
    uint32_t last_raw;
    float last_uv_index;
    bool initialized;
} uv_meter_handle_t;

void uv_meter_get_default_config(uv_meter_config_t *config);
esp_err_t uv_meter_init(uv_meter_handle_t *handle, const uv_meter_config_t *config);
void uv_meter_deinit(uv_meter_handle_t *handle);
esp_err_t uv_meter_read_raw(uv_meter_handle_t *handle, uint32_t *raw_out);
esp_err_t uv_meter_read_voltage(uv_meter_handle_t *handle, float *voltage_out);
esp_err_t uv_meter_read_uv_index(uv_meter_handle_t *handle, float *uv_index_out);
uv_meter_level_t uv_meter_get_level(float uv_index);
const char *uv_meter_level_to_string(uv_meter_level_t level);

#ifdef __cplusplus
}
#endif
