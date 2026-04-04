#ifndef __TASKS_H__
#define __TASKS_H__

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// I2C 引脚定义（与 sensirion_hw_i2c_implementation.c 保持一致）
#define I2C_SDA_PIN  15
#define I2C_SCL_PIN  14

/**
 * @brief 诊断函数：检查传感器和摄像头是否能被正确初始化
 * @return true 诊断通过，false 诊断失败
 */
bool diagnostic(void);

/** 摄像头推流信号量（进入推流模式时释放，唤醒采集任务） */
extern SemaphoreHandle_t camera_stream_sem;

/** OTA 进行中标志，其他 TLS 任务应跳过 HTTP 请求以释放内部 SRAM */
extern volatile bool ota_in_progress;

/**
 * @brief 初始化零拷贝帧队列（信号量 + 深度1队列）
 * @note  须在创建 camera_capture_task / camera_send_task 之前调用
 *        相机驱动 fb_count=3 提供三缓冲，本模块不再手动分配 PSRAM
 */
void camera_pipeline_init(void);

/** 摄像头采集任务：fb_get → 放入队列 */
void camera_capture_task(void *pvParameter);

/** 摄像头发送任务：从队列取帧 → WS 发送 → fb_return */
void camera_send_task(void *pvParameter);

/** 传感器数据采集与上报任务 */
void sensor_data_transmit_task(void *pvParameter);

/** 健康监控任务：WiFi/WS 长时间断连自动重启 */
void health_monitor_task(void *pvParameter);

#endif /* __TASKS_H__ */
