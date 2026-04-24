#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化传感器上报所需的公共资源（互斥锁等）
 */
void sensor_tasks_init(void);

/**
 * @brief 统一 JSON 上报函数
 *
 * 上报格式：
 * {
 *   "sensor":    "<sensor_type>",
 *   "data_type": "<data_type>",
 *   "data":      { ... }
 * }
 *
 * @param sensor_type  传感器名称，如 "ina231"
 * @param data_type    数据类型，如 "power" / "env" / "air_quality" 等
 * @param data_json    data 字段的 JSON 字符串（不含外层花括号）
 */
void sensor_upload(const char *sensor_type, const char *data_type, const char *data_json);

/* ── 各传感器任务 ── */
void task_ina231(void *pvParameter);
void task_sgp30(void *pvParameter);
void task_veml7700(void *pvParameter);
void task_pms9103m(void *pvParameter);
void task_sound_meter(void *pvParameter);
void task_uv_meter(void *pvParameter);

#ifdef __cplusplus
}
#endif
