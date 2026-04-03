#include "sensirion_arch_config.h"
#include "sensirion_common.h"
#include "sensirion_i2c.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "sht4x_i2c";

// 配置
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SCL_IO   14
#define I2C_MASTER_SDA_IO   15
#define I2C_MASTER_FREQ_HZ  400000
#define I2C_MASTER_TIMEOUT_MS 1000
#define MAX_I2C_DEVICES     8

static i2c_master_bus_handle_t g_i2c_bus_handle = NULL;
static bool g_i2c_initialized = false;

typedef struct {
    uint8_t address;
    i2c_master_dev_handle_t handle;
} i2c_dev_entry_t;
static i2c_dev_entry_t g_devices[MAX_I2C_DEVICES];
static int g_dev_count = 0;

static i2c_master_dev_handle_t get_or_create_device(uint8_t address) {
    for (int i = 0; i < g_dev_count; i++) {
        if (g_devices[i].address == address) {
            return g_devices[i].handle;
        }
    }
    if (g_dev_count >= MAX_I2C_DEVICES) {
        ESP_LOGE(TAG, "Too many I2C devices");
        return NULL;
    }
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        .flags = { .disable_ack_check = false },
    };
    esp_err_t ret = i2c_master_bus_add_device(g_i2c_bus_handle, &dev_config,
                                              &g_devices[g_dev_count].handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Add device 0x%02x failed: %s", address, esp_err_to_name(ret));
        return NULL;
    }
    g_devices[g_dev_count].address = address;
    ESP_LOGI(TAG, "Added device 0x%02x", address);
    return g_devices[g_dev_count++].handle;
}

int16_t sensirion_i2c_select_bus(uint8_t bus_idx) {
    (void)bus_idx;
    return STATUS_OK;
}

void sensirion_i2c_init(void) {
    if (g_i2c_initialized) return;
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = { .enable_internal_pullup = true },
    };
    esp_err_t ret = i2c_new_master_bus(&bus_config, &g_i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus init failed: %s", esp_err_to_name(ret));
        return;
    }
    g_i2c_initialized = true;
    g_dev_count = 0;
    ESP_LOGI(TAG, "I2C bus initialized");
}

void sensirion_i2c_release(void) {
    if (!g_i2c_initialized) return;
    for (int i = 0; i < g_dev_count; i++) {
        if (g_devices[i].handle) {
            i2c_master_bus_rm_device(g_devices[i].handle);
        }
    }
    g_dev_count = 0;
    if (g_i2c_bus_handle) {
        i2c_del_master_bus(g_i2c_bus_handle);
        g_i2c_bus_handle = NULL;
    }
    g_i2c_initialized = false;
}

int8_t sensirion_i2c_read(uint8_t address, uint8_t* data, uint16_t count) {
    if (!g_i2c_initialized) {
        ESP_LOGE(TAG, "I2C not initialized");
        return STATUS_FAIL;
    }
    if (data == NULL || count == 0) {
        ESP_LOGE(TAG, "Invalid read: data=%p, count=%u", data, count);
        return STATUS_FAIL;
    }

    i2c_master_dev_handle_t dev = get_or_create_device(address);
    if (dev == NULL) {
        ESP_LOGE(TAG, "No device handle for 0x%02x", address);
        return STATUS_FAIL;
    }

    // 使用专门的读函数
    esp_err_t ret = i2c_master_receive(
        dev,
        data, count,
        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS)
    );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C read 0x%02x failed: %s", address, esp_err_to_name(ret));
    }
    return (ret == ESP_OK) ? STATUS_OK : STATUS_FAIL;
}

int8_t sensirion_i2c_write(uint8_t address, const uint8_t* data, uint16_t count) {
    if (!g_i2c_initialized) {
        ESP_LOGE(TAG, "I2C not initialized");
        return STATUS_FAIL;
    }
    if (data == NULL || count == 0) {
        ESP_LOGE(TAG, "Invalid write: data=%p, count=%u", data, count);
        return STATUS_FAIL;
    }

    i2c_master_dev_handle_t dev = get_or_create_device(address);
    if (dev == NULL) {
        ESP_LOGE(TAG, "No device handle for 0x%02x", address);
        return STATUS_FAIL;
    }

    esp_err_t ret = i2c_master_transmit(
        dev,
        data, count,
        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS)
    );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C write 0x%02x failed: %s", address, esp_err_to_name(ret));
    }
    return (ret == ESP_OK) ? STATUS_OK : STATUS_FAIL;
}

void sensirion_sleep_usec(uint32_t useconds) {
    esp_rom_delay_us(useconds);
}

void i2c_scan(void) {
    if (!g_i2c_initialized) {
        printf("I2C not initialized\n");
        return;
    }
    printf("Scanning I2C bus...\n");
    uint8_t address;
    for (address = 1; address < 127; address++) {
        i2c_master_dev_handle_t dev;
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = address,
            .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        };
        esp_err_t ret = i2c_master_bus_add_device(g_i2c_bus_handle, &dev_cfg, &dev);
        if (ret == ESP_OK) {
            // 尝试读取一个字节
            uint8_t dummy;
            ret = i2c_master_transmit_receive(dev, NULL, 0, &dummy, 1, pdMS_TO_TICKS(100));
            if (ret == ESP_OK) {
                printf("Device found at 0x%02x\n", address);
            }
            i2c_master_bus_rm_device(dev);
        }
    }
    printf("Scan complete\n");
}