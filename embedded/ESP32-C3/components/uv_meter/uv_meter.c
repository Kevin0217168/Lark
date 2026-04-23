#include "uv_meter.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "uv_meter";
static const uint16_t UV_METER_MAX_SAMPLES = 128;

static const uv_meter_calibration_point_t uv_meter_default_points[] = {
    { .voltage_mv = 0.0f,   .uv_index = 0.0f },
    { .voltage_mv = 50.0f,  .uv_index = 0.0f },
    { .voltage_mv = 227.0f, .uv_index = 1.0f },
    { .voltage_mv = 318.0f, .uv_index = 2.0f },
    { .voltage_mv = 408.0f, .uv_index = 3.0f },
    { .voltage_mv = 503.0f, .uv_index = 4.0f },
    { .voltage_mv = 606.0f, .uv_index = 5.0f },
    { .voltage_mv = 696.0f, .uv_index = 6.0f },
    { .voltage_mv = 795.0f, .uv_index = 7.0f },
    { .voltage_mv = 881.0f, .uv_index = 8.0f },
    { .voltage_mv = 976.0f, .uv_index = 9.0f },
    { .voltage_mv = 1079.0f, .uv_index = 10.0f },
    { .voltage_mv = 1170.0f, .uv_index = 11.0f },
};

static inline uint32_t uv_meter_max_raw(adc_bitwidth_t width)
{
    switch (width) {
    case ADC_BITWIDTH_9:
        return 0x01FF;
    case ADC_BITWIDTH_10:
        return 0x03FF;
    case ADC_BITWIDTH_11:
        return 0x07FF;
    case ADC_BITWIDTH_12:
    default:
        return 0x0FFF;
    }
}

void uv_meter_get_default_config(uv_meter_config_t *config)
{
    if (config == NULL) {
        return;
    }
    config->unit = ADC_UNIT_1;
    config->channel = ADC_CHANNEL_3; // GPIO4 on ESP32-S3
    config->atten = ADC_ATTEN_DB_12;
    config->bit_width = ADC_BITWIDTH_12;
    config->sample_count = 60;
    config->discard_low = 8;
    config->discard_high = 8;
    config->full_scale_voltage = 3.3f;
    config->use_adc_calibration = true;
    config->calibration_points = uv_meter_default_points;
    config->calibration_point_count = sizeof(uv_meter_default_points) / sizeof(uv_meter_default_points[0]);
}

static esp_err_t uv_meter_validate_config(const uv_meter_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (config->sample_count < 16 || config->sample_count > UV_METER_MAX_SAMPLES) {
        return ESP_ERR_INVALID_ARG;
    }
    if (config->discard_low + config->discard_high >= config->sample_count) {
        return ESP_ERR_INVALID_ARG;
    }
    if (config->full_scale_voltage <= 0.0f) {
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static esp_err_t uv_meter_create_cali_handle(uv_meter_handle_t *handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = handle->config.unit,
        .chan = handle->config.channel,
        .atten = handle->config.atten,
        .bitwidth = handle->config.bit_width,
    };
    return adc_cali_create_scheme_curve_fitting(&cali_cfg, &handle->cali_handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id = handle->config.unit,
        .atten = handle->config.atten,
        .bitwidth = handle->config.bit_width,
    };
    return adc_cali_create_scheme_line_fitting(&cali_cfg, &handle->cali_handle);
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static void uv_meter_delete_cali_handle(uv_meter_handle_t *handle)
{
    if (handle == NULL || handle->cali_handle == NULL) {
        return;
    }
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_delete_scheme_curve_fitting(handle->cali_handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_delete_scheme_line_fitting(handle->cali_handle);
#endif
    handle->cali_handle = NULL;
}

static float uv_meter_map_voltage_to_uv_index(const uv_meter_handle_t *handle, float voltage_mv)
{
    if (handle == NULL) {
        return 0.0f;
    }
    size_t count = handle->calibration_point_count;
    if (count < 2) {
        return 0.0f;
    }
    const uv_meter_calibration_point_t *points = handle->calibration_table;

    if (voltage_mv <= points[0].voltage_mv) {
        return points[0].uv_index;
    }
    if (voltage_mv >= points[count - 1].voltage_mv) {
        return points[count - 1].uv_index;
    }

    for (size_t i = 1; i < count; i++) {
        if (voltage_mv <= points[i].voltage_mv) {
            float x0 = points[i - 1].voltage_mv;
            float y0 = points[i - 1].uv_index;
            float x1 = points[i].voltage_mv;
            float y1 = points[i].uv_index;
            if (x1 <= x0) {
                return y0;
            }
            float ratio = (voltage_mv - x0) / (x1 - x0);
            return y0 + ratio * (y1 - y0);
        }
    }
    return points[count - 1].uv_index;
}

uv_meter_level_t uv_meter_get_level(float uv_index)
{
    if (uv_index < 3.0f) {
        return UV_METER_LEVEL_LOW;
    }
    if (uv_index < 6.0f) {
        return UV_METER_LEVEL_MODERATE;
    }
    if (uv_index < 8.0f) {
        return UV_METER_LEVEL_HIGH;
    }
    if (uv_index < 11.0f) {
        return UV_METER_LEVEL_VERY_HIGH;
    }
    return UV_METER_LEVEL_EXTREME;
}

const char *uv_meter_level_to_string(uv_meter_level_t level)
{
    switch (level) {
    case UV_METER_LEVEL_LOW:
        return "Low";
    case UV_METER_LEVEL_MODERATE:
        return "Moderate";
    case UV_METER_LEVEL_HIGH:
        return "High";
    case UV_METER_LEVEL_VERY_HIGH:
        return "Very High";
    case UV_METER_LEVEL_EXTREME:
        return "Extreme";
    default:
        return "Unknown";
    }
}

esp_err_t uv_meter_init(uv_meter_handle_t *handle, const uv_meter_config_t *config)
{
    if (handle == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = uv_meter_validate_config(config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "invalid config");
        return err;
    }

    *handle = (uv_meter_handle_t){0};
    handle->config = *config;
    handle->last_raw = 0;
    handle->last_uv_index = 0.0f;
    handle->initialized = false;
    handle->adc_handle = NULL;
    handle->cali_handle = NULL;
    handle->calibration_point_count = 0;

    if (handle->config.unit != ADC_UNIT_1) {
        ESP_LOGE(TAG, "Only ADC_UNIT_1 is supported");
        return ESP_ERR_NOT_SUPPORTED;
    }

    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = handle->config.unit,
        .clk_src = 0,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    err = adc_oneshot_new_unit(&init_cfg, &handle->adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed (%s)", esp_err_to_name(err));
        return err;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = handle->config.atten,
        .bitwidth = handle->config.bit_width,
    };
    err = adc_oneshot_config_channel(handle->adc_handle, handle->config.channel, &chan_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_config_channel failed (%s)", esp_err_to_name(err));
        adc_oneshot_del_unit(handle->adc_handle);
        return err;
    }

    if (handle->config.use_adc_calibration) {
        err = uv_meter_create_cali_handle(handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "adc calibration handle creation failed (%s)", esp_err_to_name(err));
            adc_oneshot_del_unit(handle->adc_handle);
            return err;
        }
    }

    if (handle->config.calibration_points != NULL && handle->config.calibration_point_count > 0) {
        if (handle->config.calibration_point_count > UV_METER_MAX_CAL_POINTS) {
            ESP_LOGE(TAG, "too many calibration points (%u)", (unsigned)handle->config.calibration_point_count);
            uv_meter_delete_cali_handle(handle);
            adc_oneshot_del_unit(handle->adc_handle);
            return ESP_ERR_INVALID_ARG;
        }
        handle->calibration_point_count = handle->config.calibration_point_count;
        for (size_t i = 0; i < handle->calibration_point_count; i++) {
            handle->calibration_table[i] = handle->config.calibration_points[i];
        }
        for (size_t i = 0; i + 1 < handle->calibration_point_count; i++) {
            for (size_t j = i + 1; j < handle->calibration_point_count; j++) {
                if (handle->calibration_table[j].voltage_mv < handle->calibration_table[i].voltage_mv) {
                    uv_meter_calibration_point_t tmp = handle->calibration_table[i];
                    handle->calibration_table[i] = handle->calibration_table[j];
                    handle->calibration_table[j] = tmp;
                }
            }
        }
    }

    handle->initialized = true;
    return ESP_OK;
}

void uv_meter_deinit(uv_meter_handle_t *handle)
{
    if (handle == NULL) {
        return;
    }
    if (handle->adc_handle != NULL) {
        adc_oneshot_del_unit(handle->adc_handle);
        handle->adc_handle = NULL;
    }
    uv_meter_delete_cali_handle(handle);
    handle->initialized = false;
}

esp_err_t uv_meter_read_raw(uv_meter_handle_t *handle, uint32_t *raw_out)
{
    if (handle == NULL || raw_out == NULL || !handle->initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t count = handle->config.sample_count;
    uint32_t readings[UV_METER_MAX_SAMPLES];
    uint64_t sum = 0;

    for (uint16_t i = 0; i < count; i++) {
        int raw = 0;
        esp_err_t err = adc_oneshot_read(handle->adc_handle, handle->config.channel, &raw);
        if (err != ESP_OK) {
            return err;
        }
        readings[i] = (uint32_t)raw;
        sum += readings[i];
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    for (uint16_t i = 0; i < count - 1; i++) {
        for (uint16_t j = 0; j < count - 1 - i; j++) {
            if (readings[j] > readings[j + 1]) {
                uint32_t tmp = readings[j];
                readings[j] = readings[j + 1];
                readings[j + 1] = tmp;
            }
        }
    }

    uint64_t trimmed = sum;
    for (uint16_t i = 0; i < handle->config.discard_low; i++) {
        trimmed -= readings[i];
    }
    for (uint16_t i = 0; i < handle->config.discard_high; i++) {
        trimmed -= readings[count - 1 - i];
    }

    uint32_t average = (uint32_t)(trimmed / (count - handle->config.discard_low - handle->config.discard_high));
    *raw_out = average;
    handle->last_raw = average;
    return ESP_OK;
}

esp_err_t uv_meter_read_voltage(uv_meter_handle_t *handle, float *voltage_out)
{
    if (handle == NULL || voltage_out == NULL || !handle->initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    if (handle->last_raw == 0) {
        uint32_t raw;
        esp_err_t err = uv_meter_read_raw(handle, &raw);
        if (err != ESP_OK) {
            return err;
        }
    }

    uint32_t raw = handle->last_raw;
    if (handle->cali_handle != NULL) {
        int voltage_mv = 0;
        esp_err_t err = adc_cali_raw_to_voltage(handle->cali_handle, (int)raw, &voltage_mv);
        if (err != ESP_OK) {
            return err;
        }
        *voltage_out = voltage_mv * 0.001f;
        return ESP_OK;
    }

    uint32_t max_raw = uv_meter_max_raw(handle->config.bit_width);
    *voltage_out = ((float)raw / (float)max_raw) * handle->config.full_scale_voltage;
    return ESP_OK;
}

esp_err_t uv_meter_read_uv_index(uv_meter_handle_t *handle, float *uv_index_out)
{
    if (handle == NULL || uv_index_out == NULL || !handle->initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    float voltage = 0.0f;
    esp_err_t err = uv_meter_read_voltage(handle, &voltage);
    if (err != ESP_OK) {
        return err;
    }
    float uv_index = uv_meter_map_voltage_to_uv_index(handle, voltage * 1000.0f);
    handle->last_uv_index = uv_index;
    *uv_index_out = uv_index;
    return ESP_OK;
}
