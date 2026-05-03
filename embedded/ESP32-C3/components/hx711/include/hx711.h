#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ────────── HX711 增益模式 ────────── */
typedef enum {
    HX711_GAIN_128 = 1,  /**< 通道 A，增益 128（第 25 个脉冲） */
    HX711_GAIN_32  = 2,  /**< 通道 B，增益 32（第 26 个脉冲） */
    HX711_GAIN_64  = 3,  /**< 通道 A，增益 64（第 27 个脉冲） */
} hx711_gain_t;

/* ────────── HX711 配置 ────────── */
typedef struct {
    gpio_num_t dout_gpio;   /**< DOUT 数据引脚 */
    gpio_num_t sck_gpio;    /**< SCK  时钟引脚 */
    hx711_gain_t gain;      /**< 默认增益模式（推荐 GAIN_128） */
    uint8_t sample_count;   /**< 均值采样次数（1-32，默认 8） */
} hx711_config_t;

/* ────────── HX711 句柄 ────────── */
typedef struct {
    hx711_config_t config;
    int32_t offset;           /**< 去皮偏移量（原始值） */
    float   calibration_factor; /**< 标定系数 (raw / factor = grams) */
    float   full_scale_g;     /**< 量程上限（克），0 表示不限 */
    bool    initialized;
} hx711_t;

/* ═══════════════════════════════════════════════════════════
 * 初始化 / 反初始化
 * ═══════════════════════════════════════════════════════════ */

/**
 * @brief 获取默认配置
 *
 * 默认：DOUT=GPIO_NUM_5, SCK=GPIO_NUM_6, GAIN_128, 8次均值
 */
void hx711_get_default_config(hx711_config_t *cfg);

/**
 * @brief 初始化 HX711
 *
 * @param hx711  句柄指针（由调用方分配）
 * @param cfg    配置指针（NULL 则使用默认配置）
 * @return ESP_OK 成功
 */
esp_err_t hx711_init(hx711_t *hx711, const hx711_config_t *cfg);

/**
 * @brief 反初始化，释放 GPIO
 */
void hx711_deinit(hx711_t *hx711);

/* ═══════════════════════════════════════════════════════════
 * 低层原始读取
 * ═══════════════════════════════════════════════════════════ */

/**
 * @brief 读取一次原始 ADC 值（24 位有符号，阻塞等待就绪）
 *
 * @return 原始 ADC 值（-8388608 ~ 8388607）
 */
esp_err_t hx711_read_raw(hx711_t *hx711, int32_t *raw_out);

/**
 * @brief 多次采样取均值（消除抖动）
 *
 * @param hx711   句柄
 * @param count   采样次数（1-32）
 * @param avg_out 平均值输出
 */
esp_err_t hx711_read_average(hx711_t *hx711, uint8_t count, int32_t *avg_out);

/* ═══════════════════════════════════════════════════════════
 * 标定 & 去皮
 * ═══════════════════════════════════════════════════════════ */

/**
 * @brief 去皮（记录当前读数作为零点偏移）
 *
 * 调用前确保称盘为空。
 */
esp_err_t hx711_tare(hx711_t *hx711);

/**
 * @brief 设置去皮偏移量（手动）
 */
void hx711_set_offset(hx711_t *hx711, int32_t offset);

/**
 * @brief 设置标定系数
 *
 * coefficient = raw_reading / known_weight_grams
 * 例如：已知 100g 砝码得到 raw=43000，则 coefficient=43000/100=430
 */
void hx711_set_calibration_factor(hx711_t *hx711, float factor);

/**
 * @brief 便捷标定：根据已知重量自动计算标定系数
 *
 * 放上已知重量砝码后调用此函数。
 *
 * @param hx711          句柄
 * @param known_weight_g 已知砝码重量（克）
 */
esp_err_t hx711_calibrate(hx711_t *hx711, float known_weight_g);

/* ═══════════════════════════════════════════════════════════
 * 重量读取（高层 API）
 * ═══════════════════════════════════════════════════════════ */

/**
 * @brief 读取当前重量（克）
 *
 * 已去皮、已标定。
 *
 * @param hx711      句柄
 * @param weight_g   重量输出（克）
 * @return ESP_OK 成功
 *         ESP_ERR_INVALID_STATE 超量程
 */
esp_err_t hx711_read_weight(hx711_t *hx711, float *weight_g);

/* ═══════════════════════════════════════════════════════════
 * 量程配置（量程可调）
 * ═══════════════════════════════════════════════════════════ */

/**
 * @brief 设置量程上限
 *
 * @param hx711       句柄
 * @param full_scale_g 量程上限（克），0 表示不限制
 *
 * 超过量程时 hx711_read_weight() 返回 ESP_ERR_INVALID_STATE
 */
void hx711_set_full_scale(hx711_t *hx711, float full_scale_g);

/**
 * @brief 获取量程上限
 */
float hx711_get_full_scale(const hx711_t *hx711);

/**
 * @brief 检查是否超量程
 *
 * @return true 表示当前重量超量程
 */
bool hx711_is_overload(const hx711_t *hx711, float weight_g);

/* ═══════════════════════════════════════════════════════════
 * 电源管理
 * ═══════════════════════════════════════════════════════════ */

/**
 * @brief 进入省电模式（拉高 SCK）
 */
void hx711_power_down(hx711_t *hx711);

/**
 * @brief 唤醒（拉低 SCK，等待稳定）
 */
void hx711_power_up(hx711_t *hx711);

/**
 * @brief 是否已就绪（DOUT 为低）
 *
 * @param timeout_ms 超时等待（毫秒），0 表示立即返回
 */
bool hx711_is_ready(hx711_t *hx711, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
