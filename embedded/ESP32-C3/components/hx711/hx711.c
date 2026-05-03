#include "hx711.h"

#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <string.h>

static const char *TAG = "hx711";

/* ────────── HX711 时序常量 ────────── */
#define HX711_DELAY_US  1       /**< SCK 高低电平保持时间（µs），≥0.2µs */
#define HX711_POWER_ON_DELAY_MS 60 /**< 上电稳定等待 */

/* ────────── 默认配置 ────────── */
#define HX711_DEFAULT_DOUT     GPIO_NUM_5
#define HX711_DEFAULT_SCK      GPIO_NUM_6
#define HX711_DEFAULT_GAIN     HX711_GAIN_128
#define HX711_DEFAULT_SAMPLES  8

/* ═══════════════════════════════════════════════════════════
 * 获取默认配置
 * ═══════════════════════════════════════════════════════════ */

void hx711_get_default_config(hx711_config_t *cfg)
{
    if (!cfg) return;
    cfg->dout_gpio    = HX711_DEFAULT_DOUT;
    cfg->sck_gpio     = HX711_DEFAULT_SCK;
    cfg->gain         = HX711_DEFAULT_GAIN;
    cfg->sample_count = HX711_DEFAULT_SAMPLES;
}

/* ═══════════════════════════════════════════════════════════
 * 初始化
 * ═══════════════════════════════════════════════════════════ */

esp_err_t hx711_init(hx711_t *hx711, const hx711_config_t *cfg)
{
    if (!hx711) return ESP_ERR_INVALID_ARG;

    hx711_config_t default_cfg;
    if (cfg) {
        memcpy(&hx711->config, cfg, sizeof(hx711_config_t));
    } else {
        hx711_get_default_config(&default_cfg);
        memcpy(&hx711->config, &default_cfg, sizeof(hx711_config_t));
    }

    /* 参数校验 */
    if (hx711->config.sample_count < 1 || hx711->config.sample_count > 32) {
        ESP_LOGW(TAG, "sample_count=%d 不合法，已修正为 %d",
                 hx711->config.sample_count, HX711_DEFAULT_SAMPLES);
        hx711->config.sample_count = HX711_DEFAULT_SAMPLES;
    }

    /* 初始化 GPIO */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << hx711->config.dout_gpio),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "DOUT GPIO 配置失败: %s", esp_err_to_name(ret));
        return ret;
    }

    io_conf.pin_bit_mask = (1ULL << hx711->config.sck_gpio);
    io_conf.mode         = GPIO_MODE_OUTPUT;
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SCK GPIO 配置失败: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 上电：拉低 SCK，唤醒芯片 */
    gpio_set_level(hx711->config.sck_gpio, 0);
    vTaskDelay(pdMS_TO_TICKS(HX711_POWER_ON_DELAY_MS));

    /* 初始化状态 */
    hx711->offset              = 0;
    hx711->calibration_factor  = 0;
    hx711->full_scale_g        = 0;
    hx711->initialized         = true;

    ESP_LOGI(TAG, "HX711 初始化完成: DOUT=GPIO%d SCK=GPIO%d gain=%d samples=%d",
             hx711->config.dout_gpio, hx711->config.sck_gpio,
             hx711->config.gain, hx711->config.sample_count);
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════
 * 反初始化
 * ═══════════════════════════════════════════════════════════ */

void hx711_deinit(hx711_t *hx711)
{
    if (!hx711 || !hx711->initialized) return;
    gpio_reset_pin(hx711->config.dout_gpio);
    gpio_reset_pin(hx711->config.sck_gpio);
    hx711->initialized = false;
}

/* ═══════════════════════════════════════════════════════════
 * 判断数据是否就绪
 * ═══════════════════════════════════════════════════════════ */

bool hx711_is_ready(hx711_t *hx711, uint32_t timeout_ms)
{
    if (!hx711 || !hx711->initialized) return false;

    TickType_t start = xTaskGetTickCount();
    while (gpio_get_level(hx711->config.dout_gpio) == 1) {
        if (timeout_ms > 0 &&
            xTaskGetTickCount() - start >= pdMS_TO_TICKS(timeout_ms)) {
            ESP_LOGW(TAG, "HX711 等待就绪超时 (%" PRIu32 "ms)", timeout_ms);
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(1)); /* 1ms 粒度轮询 */
    }
    return true;
}

/* ═══════════════════════════════════════════════════════════
 * 读取一次原始值
 *
 * 协议：
 *   1. 等待 DOUT 变低（数据就绪）
 *   2. 在 SCK 上升沿读取 DOUT，连续读 24 位（MSB 在前）
 *   3. 第 25-27 个脉冲设置下次转换增益
 *   4. 数据是偏移二进制，异或 0x800000 转有符号数
 * ═══════════════════════════════════════════════════════════ */

esp_err_t hx711_read_raw(hx711_t *hx711, int32_t *raw_out)
{
    if (!hx711 || !hx711->initialized || !raw_out) {
        return ESP_ERR_INVALID_ARG;
    }

    gpio_num_t dout = hx711->config.dout_gpio;
    gpio_num_t sck  = hx711->config.sck_gpio;

    /* 等待数据就绪（无超时，阻塞） */
    while (gpio_get_level(dout) == 1) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    /* 读取 24 位数据，MSB 在前 */
    uint32_t count = 0;
    for (int i = 0; i < 24; i++) {
        gpio_set_level(sck, 1);
        esp_rom_delay_us(HX711_DELAY_US);
        count = count << 1;
        if (gpio_get_level(dout)) {
            count++;
        }
        gpio_set_level(sck, 0);
        esp_rom_delay_us(HX711_DELAY_US);
    }

    /* 发送增益选择脉冲 */
    for (int i = 0; i < hx711->config.gain; i++) {
        gpio_set_level(sck, 1);
        esp_rom_delay_us(HX711_DELAY_US);
        gpio_set_level(sck, 0);
        esp_rom_delay_us(HX711_DELAY_US);
    }

    /* 偏移二进制 → 有符号数 */
    count ^= 0x800000;

    *raw_out = (int32_t)count;
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════
 * 多次采样取均值
 * ═══════════════════════════════════════════════════════════ */

esp_err_t hx711_read_average(hx711_t *hx711, uint8_t count, int32_t *avg_out)
{
    if (!hx711 || !hx711->initialized || !avg_out || count == 0 || count > 32) {
        return ESP_ERR_INVALID_ARG;
    }

    int64_t sum = 0;
    for (uint8_t i = 0; i < count; i++) {
        int32_t raw;
        esp_err_t ret = hx711_read_raw(hx711, &raw);
        if (ret != ESP_OK) return ret;
        sum += raw;
    }
    *avg_out = (int32_t)(sum / count);
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════
 * 去皮（记录当前读数作为零点偏移）
 * ═══════════════════════════════════════════════════════════ */

esp_err_t hx711_tare(hx711_t *hx711)
{
    if (!hx711 || !hx711->initialized) return ESP_ERR_INVALID_ARG;

    int32_t avg;
    esp_err_t ret = hx711_read_average(hx711, hx711->config.sample_count, &avg);
    if (ret != ESP_OK) return ret;

    hx711->offset = avg;
    ESP_LOGI(TAG, "去皮完成: offset=%" PRId32, hx711->offset);
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════
 * 手动设置偏移量
 * ═══════════════════════════════════════════════════════════ */

void hx711_set_offset(hx711_t *hx711, int32_t offset)
{
    if (!hx711) return;
    hx711->offset = offset;
}

/* ═══════════════════════════════════════════════════════════
 * 设置标定系数
 * ═══════════════════════════════════════════════════════════ */

void hx711_set_calibration_factor(hx711_t *hx711, float factor)
{
    if (!hx711) return;
    hx711->calibration_factor = factor;
}

/* ═══════════════════════════════════════════════════════════
 * 便捷标定：自动计算标定系数
 * ═══════════════════════════════════════════════════════════ */

esp_err_t hx711_calibrate(hx711_t *hx711, float known_weight_g)
{
    if (!hx711 || !hx711->initialized || known_weight_g <= 0) {
        return ESP_ERR_INVALID_ARG;
    }

    /* 读取当前原始值（多次采样取均值） */
    int32_t avg;
    esp_err_t ret = hx711_read_average(hx711, hx711->config.sample_count, &avg);
    if (ret != ESP_OK) return ret;

    int32_t net = avg - hx711->offset;
    if (net <= 0) {
        ESP_LOGE(TAG, "标定失败：净读数无效 (net=%" PRId32 ")，请先放置砝码", net);
        return ESP_ERR_INVALID_STATE;
    }

    /* factor = raw_net / known_weight_g */
    hx711->calibration_factor = (float)net / known_weight_g;

    ESP_LOGI(TAG, "标定完成: 砝码=%.1fg 原始值=%.1f→标定系数=%.3f",
             known_weight_g, (float)net, hx711->calibration_factor);
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════
 * 量程配置
 * ═══════════════════════════════════════════════════════════ */

void hx711_set_full_scale(hx711_t *hx711, float full_scale_g)
{
    if (!hx711) return;
    hx711->full_scale_g = full_scale_g;
    ESP_LOGI(TAG, "量程设置: %.0f g", full_scale_g);
}

float hx711_get_full_scale(const hx711_t *hx711)
{
    if (!hx711) return 0;
    return hx711->full_scale_g;
}

bool hx711_is_overload(const hx711_t *hx711, float weight_g)
{
    if (!hx711 || hx711->full_scale_g <= 0) return false;
    return (weight_g > hx711->full_scale_g);
}

/* ═══════════════════════════════════════════════════════════
 * 读取当前重量（克）
 * ═══════════════════════════════════════════════════════════ */

esp_err_t hx711_read_weight(hx711_t *hx711, float *weight_g)
{
    if (!hx711 || !hx711->initialized || !weight_g) {
        return ESP_ERR_INVALID_ARG;
    }

    if (hx711->calibration_factor <= 0) {
        ESP_LOGE(TAG, "未标定，请先调用 hx711_calibrate() 或 hx711_set_calibration_factor()");
        return ESP_ERR_INVALID_STATE;
    }

    /* 读取均值 */
    int32_t avg;
    esp_err_t ret = hx711_read_average(hx711, hx711->config.sample_count, &avg);
    if (ret != ESP_OK) return ret;

    /* 去皮 */
    int32_t net = avg - hx711->offset;
    if (net < 0) net = 0;

    /* 计算重量 */
    *weight_g = (float)net / hx711->calibration_factor;

    /* 超量程检查 */
    if (hx711->full_scale_g > 0 && *weight_g > hx711->full_scale_g) {
        ESP_LOGW(TAG, "超量程: %.1f g > %.0f g", *weight_g, hx711->full_scale_g);
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════
 * 电源管理
 * ═══════════════════════════════════════════════════════════ */

void hx711_power_down(hx711_t *hx711)
{
    if (!hx711 || !hx711->initialized) return;
    /* 拉高 SCK 并保持 > 60µs 进入省电模式 */
    gpio_set_level(hx711->config.sck_gpio, 1);
    esp_rom_delay_us(100);
    ESP_LOGI(TAG, "HX711 进入省电模式");
}

void hx711_power_up(hx711_t *hx711)
{
    if (!hx711 || !hx711->initialized) return;
    /* 拉低 SCK 唤醒芯片 */
    gpio_set_level(hx711->config.sck_gpio, 0);
    vTaskDelay(pdMS_TO_TICKS(HX711_POWER_ON_DELAY_MS));
    ESP_LOGI(TAG, "HX711 唤醒");
}
