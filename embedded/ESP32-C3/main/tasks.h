#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SENSOR_COLLECT_INTERVAL_MS  60000

void sensor_tasks_init(void);
SemaphoreHandle_t sensor_get_i2c_mutex(void);
void sensor_upload(const char *sensor_type, const char *data_type, const char *data_json);
void sensor_upload_aggregated(const char *aggregated_json);

void task_sensor_collect(void *pvParameter);

#ifdef __cplusplus
}
#endif
