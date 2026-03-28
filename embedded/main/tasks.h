#ifndef __TASKS_H__
#define __TASKS_H__

#include <stdbool.h>

// I2C 引脚定义（与 sensirion_hw_i2c_implementation.c 保持一致）
#define I2C_SDA_PIN  15
#define I2C_SCL_PIN  14

/**
 * @brief 诊断函数：检查传感器和摄像头是否能被正确初始化
 * @return true 诊断通过，false 诊断失败
 */
bool diagnostic(void);

/** 摄像头图像传输任务 */
void camera_transmit_task(void *pvParameter);

/** 传感器数据采集与上报任务 */
void sensor_data_transmit_task(void *pvParameter);

/** 健康监控任务：WiFi/WS 长时间断连自动重启 */
void health_monitor_task(void *pvParameter);

#endif /* __TASKS_H__ */
