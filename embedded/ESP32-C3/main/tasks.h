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

/** 按传感器名称读取一次数据，写入 json_out（不含外层花括号），返回 true 表示成功 */
bool sensor_read_one(const char *sensor_name, char *json_out, size_t json_size);

void task_sensor_collect(void *pvParameter);

#ifdef __cplusplus
}
#endif
