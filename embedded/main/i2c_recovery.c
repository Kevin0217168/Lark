#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
#include "i2c_recovery.h"

static const char *TAG = "i2c_recovery";

void i2c_bus_recovery(int sda_pin, int scl_pin)
{
    ESP_LOGI(TAG, "尝试 I2C 总线恢复 (SDA=%d, SCL=%d)...", sda_pin, scl_pin);

    // 先将两个引脚配置为普通 GPIO 输出（开漏模式 + 上拉）
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << sda_pin) | (1ULL << scl_pin),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,  // 开漏，兼容 I2C 标准
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // 释放 SDA 和 SCL
    gpio_set_level(sda_pin, 1);
    gpio_set_level(scl_pin, 1);
    esp_rom_delay_us(10);

    // 发送最多 9 个 SCL 脉冲以释放被卡住的从机
    for (int i = 0; i < 9; i++) {
        // 如果 SDA 已经恢复高电平，从机已释放
        if (gpio_get_level(sda_pin) == 1) {
            ESP_LOGI(TAG, "SDA 在第 %d 个脉冲后恢复正常", i);
            break;
        }
        gpio_set_level(scl_pin, 0);
        esp_rom_delay_us(5);
        gpio_set_level(scl_pin, 1);
        esp_rom_delay_us(5);
    }

    // 发送 STOP 条件: SDA 从低到高（在 SCL 高时）
    gpio_set_level(sda_pin, 0);
    esp_rom_delay_us(5);
    gpio_set_level(scl_pin, 1);
    esp_rom_delay_us(5);
    gpio_set_level(sda_pin, 1);
    esp_rom_delay_us(5);

    // 恢复 GPIO 为默认状态，让 I2C 驱动正常接管
    gpio_reset_pin(sda_pin);
    gpio_reset_pin(scl_pin);

    ESP_LOGI(TAG, "I2C 总线恢复完成");
}
