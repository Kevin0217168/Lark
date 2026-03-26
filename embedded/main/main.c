#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_app_format.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs_flash.h"

#include "Wifista.h"
#include "Camera.h"
#include "main.h"
#include "sht4x.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

// I2C 引脚定义（与 sensirion_hw_i2c_implementation.c 保持一致）
#define I2C_SDA_PIN  15
#define I2C_SCL_PIN  14

/**
 * @brief I2C 总线恢复：在 WDT/软重启后，从机可能卡住 SDA 线
 *        通过手动发送 9 个 SCL 时钟脉冲 + STOP 条件来恢复总线
 */
static void i2c_bus_recovery(int sda_pin, int scl_pin)
{
    ESP_LOGI("i2c_recovery", "尝试 I2C 总线恢复 (SDA=%d, SCL=%d)...", sda_pin, scl_pin);

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
            ESP_LOGI("i2c_recovery", "SDA 在第 %d 个脉冲后恢复正常", i);
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

    ESP_LOGI("i2c_recovery", "I2C 总线恢复完成");
}

#define SECRET_NAMESPACE "device"
#define SECRET_KEY "secret"
#define SECRET_MAX_LEN 64

static const char *TAG = "main";
// 默认密钥（首次烧录/未写入时使用）
static char secret[SECRET_MAX_LEN] = "2d3173bfc1f64da0894a3257e1636d72";

// 设备1
// static const char *secret = "b1f9562544a348c98c57a66b32a92d32";

// 写入 secret 到 NVS
esp_err_t save_secret_to_nvs(const char *secret_val) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(SECRET_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    err = nvs_set_str(handle, SECRET_KEY, secret_val);
    if (err == ESP_OK) nvs_commit(handle);
    nvs_close(handle);
    return err;
}

// 从 NVS 读取 secret
esp_err_t load_secret_from_nvs(char *secret_buf, size_t buf_len) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(SECRET_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return err;
    size_t required_size = buf_len;
    err = nvs_get_str(handle, SECRET_KEY, secret_buf, &required_size);
    nvs_close(handle);
    return err;
}
extern bool Wifi_isConnected;

Device_t device = {
    .name = "ESP32 - Default",
    .uuid = "",
    .isOnline = false,
    .status = DEVICE_OFFLINE,
    //TODO: 存储版本信息
};

void register_handler(RequestContext_t *ctx)
{
    if (esp_http_client_get_status_code(*(ctx->client)) != 200)
    {
        ESP_LOGE(TAG, "注册失败, 状态码: %d", esp_http_client_get_status_code(*(ctx->client)));
        return;
    }

    if (ctx->is_json && ctx->json != NULL)
    {
        // 根据实际返回字段名修改
        cJSON *id_item = cJSON_GetObjectItemCaseSensitive(ctx->json, "id");
        cJSON *name_item = cJSON_GetObjectItemCaseSensitive(ctx->json, "name");
        cJSON *status_item = cJSON_GetObjectItemCaseSensitive(ctx->json, "status");

        if (cJSON_IsString(id_item) && cJSON_IsString(name_item) && cJSON_IsString(status_item))
        {
            // 注意：需要拷贝字符串，因为 ctx->json 即将被释放
            strncpy(device.uuid, id_item->valuestring, sizeof(device.uuid) - 1);
            device.uuid[sizeof(device.uuid) - 1] = '\0';
            strncpy(device.name, name_item->valuestring, sizeof(device.name) - 1);
            device.name[sizeof(device.name) - 1] = '\0';
            device.isOnline = (strcasecmp(status_item->valuestring, "online") == 0);

            ESP_LOGI(TAG, "注册成功, 与服务器同步数据:\n名称: %s\nID: %s\n状态: %s",
                     device.name, device.uuid,
                     device.isOnline ? "online" : "offline");
        }
        else
        {
            ESP_LOGE(TAG, "返回的 JSON 缺少必要字段或类型错误");
        }
    }
    else
    {
        ESP_LOGW(TAG, "响应不是有效的 JSON 对象");
    }
}

void PhotoTransmit(camera_fb_t *fb)
{
    WebsocketSendbytes(fb->buf, fb->len);
}

/**
 * @brief 诊断函数：检查传感器是否能被正确初始化
 * 
 * @return true 诊断通过
 * @return false 诊断失败
 */
bool diagnostic(void)
{
    ESP_LOGI(TAG, "开始运行固件诊断...");

    // 1. I2C 总线恢复 + 检查 SHT4x 传感器
    i2c_bus_recovery(I2C_SDA_PIN, I2C_SCL_PIN);
    sensirion_i2c_init();
    if (sht4x_probe() != STATUS_OK)
    {
        ESP_LOGE(TAG, "诊断失败: SHT4x 传感器未响应");
        return false;
    }
    ESP_LOGI(TAG, "SHT4x 传感器探测成功");

    // 2. 检查摄像头初始化
    // 注意: CameraInit() 目前内部出错只是 LOGE 并没有返回错误码，
    // 我们需要通过 esp_camera_sensor_get() 或者重新检查初始化状态
    CameraInit();
    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL)
    {
        ESP_LOGE(TAG, "诊断失败: 无法获取摄像头传感器指针");
        return false;
    }
    ESP_LOGI(TAG, "摄像头传感器初始化成功");

    ESP_LOGI(TAG, "固件诊断通过!");
    return true;
}

void camera_transmit_task();
void sensor_data_transmit_task();

void app_main(void)
{
    // ---------------------WIFI连接配置-------------------------
    printf("Hello ESP-IDF!\n");
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // 优先从 NVS 读取 secret
    char nvs_secret[SECRET_MAX_LEN] = {0};
    if (load_secret_from_nvs(nvs_secret, sizeof(nvs_secret)) == ESP_OK && nvs_secret[0] != '\0') {
        strncpy(secret, nvs_secret, sizeof(secret) - 1);
        secret[sizeof(secret) - 1] = '\0';
        ESP_LOGI(TAG, "Loaded secret from NVS: %s", secret);
    } else {
        // 第一次启动或未写入过，写入默认密钥
        save_secret_to_nvs(secret);
        ESP_LOGI(TAG, "Saved default secret to NVS: %s", secret);
    }

    WifistaInit("关闭", "kaiwen0818");
    // 等待wifi连接
    while (!Wifi_isConnected)
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    // SNTP获取时间
    obtain_time();

    // ---------------------版本更新检测--------------------------
    const esp_app_desc_t* desc;
    desc = esp_app_get_description();
    ESP_LOGI(TAG, "---------------------固件信息---------------------------------");

    ESP_LOGI(TAG, "当前运行的版本: Version: %s\n", desc->version);

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state = ESP_OTA_IMG_UNDEFINED;
    bool ota_state_valid = (esp_ota_get_state_partition(running, &ota_state) == ESP_OK);
    if (ota_state_valid && ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
        // 固件第一次运行, 执行诊断
        ESP_LOGI(TAG, "新版本喵");
        ESP_LOGI(TAG, "[DIAGNOSTIC] 开始运行固件诊断...");
        if (diagnostic()) {
            ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
            esp_ota_mark_app_valid_cancel_rollback();
        } else {
            ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
            esp_ota_mark_app_invalid_rollback_and_reboot();
        }
    } else if (!ota_state_valid) {
        ESP_LOGW(TAG, "无法获取 OTA 状态 (rollback 可能未启用), 跳过诊断");
    }

    // ---------------------连接服务器----------------------------
    WifiSecurityClientInit();

    // 通过id开启ws连接/stream/viewer/ws
    char path_data[128];
    snprintf(path_data, sizeof(path_data), "/api/stream/device/ws?secret=%s", secret);
    // 注册回调函数
    Websocket_event_handler_register(NULL, ws_text_handler);
    WebsocketStart("wss://lark.mintlab.top", path_data, 443);
    // WebsocketStart("ws://192.168.1.199", path_data, 8080);

    // 等待ws连接成功
    while (!WebsocketIsConnected())
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    // ----------------------初始化传感器-----------------------------
    // 如果不是第一次 OTA 运行（或者已经诊断过），则在这里正常初始化
    // 第一次运行已经在 diagnostic() 里面初始化过了
    bool need_sensor_init = !ota_state_valid || (ota_state != ESP_OTA_IMG_PENDING_VERIFY);
    if (need_sensor_init) {
        // I2C 总线恢复：WDT/软重启后 SHT4x 可能卡住 SDA，需先恢复总线
        i2c_bus_recovery(I2C_SDA_PIN, I2C_SCL_PIN);

        sensirion_i2c_init();
        int sht_retry = 0;
        const int sht_max_retry = 10;
        while (sht4x_probe() != STATUS_OK && sht_retry < sht_max_retry)
        {
            sht_retry++;
            ESP_LOGW(TAG, "SHT sensor probing failed, retry %d/%d", sht_retry, sht_max_retry);
            // 每次失败后再尝试一次总线恢复
            sensirion_i2c_release();
            i2c_bus_recovery(I2C_SDA_PIN, I2C_SCL_PIN);
            sensirion_i2c_init();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        if (sht_retry >= sht_max_retry) {
            ESP_LOGE(TAG, "SHT sensor 初始化失败，已达到最大重试次数 %d", sht_max_retry);
        } else {
            ESP_LOGI(TAG, "SHT sensor probing successful");
        }

        // 摄像头 SCCB I2C (GPIO 26/27) 总线恢复
        i2c_bus_recovery(26, 27);
        CameraInit();
    }
    
    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL) {
        s->set_framesize(s, FRAMESIZE_SVGA);
        s->set_vflip(s, 1);
    }

    // ----------------------OTA 任务（在传感器/摄像头初始化完成后启动）-----------
    // 注意：OTA 必须在摄像头初始化之后启动，因为 OTA flash 擦写 与
    // 摄像头 PSRAM DMA 共享 SPI 总线，同时运行会触发 WDT
    static uint8_t ota_task_Handle_ParameterToPass;
    TaskHandle_t ota_task_Handle = NULL;
    xTaskCreate(ota_task, "ota_task", 8192, &ota_task_Handle_ParameterToPass, 1, &ota_task_Handle);
    configASSERT(ota_task_Handle);

    // -------------------------开启任务-------------------------------

    // 开启图像传输任务
    static uint8_t ucParameterToPass;
    TaskHandle_t xHandle = NULL;
    xTaskCreate(camera_transmit_task, "camera_transmit_task", 4096, &ucParameterToPass, 1, &xHandle);
    configASSERT(xHandle);

    // 开启传感器传输任务（TLS HTTPS POST 需要至少 8192 字节栈空间）
    static uint8_t sensor_data_transmit_task_Handle_ParameterToPass;
    TaskHandle_t sensor_data_transmit_task_Handle = NULL;
    xTaskCreate(sensor_data_transmit_task, "sensor_data_transmit_task", 8192, &sensor_data_transmit_task_Handle_ParameterToPass, 1, &sensor_data_transmit_task_Handle);
    configASSERT(sensor_data_transmit_task_Handle);
}

void sensor_data_transmit_task()
{
    time_t now = 0;
    struct tm timeinfo = {0};
    while (1)
    {
        int32_t temperature, humidity;
        int8_t ret = sht4x_measure_blocking_read(&temperature, &humidity);
        if (ret == STATUS_OK)
        {
            float temp_c = temperature / 1000.0f;
            float hum_pct = humidity / 1000.0f;
            ESP_LOGI(TAG, "measured temperature: %0.2f degreeCelsius, "
                          "measured humidity: %0.2f percentRH\n",
                     temp_c, hum_pct);

            // 检查时间是否已同步，如果无效则不附带时间戳，让后端使用服务器时间
            char post_data[256];
            if (is_time_valid()) {
                time(&now);
                setenv("TZ", "CST-8", 1);
                localtime_r(&now, &timeinfo);
                ESP_LOGI(TAG, "当前时间: %04d-%02d-%02d %02d:%02d:%02d+08:00",
                         timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                         timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

                // 构建 ISO 8601 格式时间戳
                char timestamp_str[32];
                strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%dT%H:%M:%S+08:00", &timeinfo);

                snprintf(post_data, sizeof(post_data),
                         "{\"secret\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f,\"timestamp\":\"%s\"}",
                         secret, temp_c, hum_pct, timestamp_str);
            } else {
                // 时间未同步，不传时间戳，让后端使用服务器当前时间
                ESP_LOGW(TAG, "时间尚未同步，不附带时间戳，由后端补充");
                snprintf(post_data, sizeof(post_data),
                         "{\"secret\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f}",
                         secret, temp_c, hum_pct);
            }

            // 发送 POST 请求到后端
            // int ret_code = WifiSecurityRequest("http://192.168.1.199", "/sensors", 8080,
            //                                    WS_CLINENT_METHOD_POST, post_data, NULL);
            int ret_code = WifiSecurityRequest("https://lark.mintlab.top", "/api/sensors", 443,
                                               WS_CLINENT_METHOD_POST, post_data, NULL);
            if (ret_code != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to send sensor data, error=%d", ret_code);
            }
        }
        else
        {
            ESP_LOGE(TAG, "error reading measurement\n");
        }

        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
}

void camera_transmit_task()
{
    while (1)
    {
        if (device.status == DEVICE_ON_STREAM)
        {
            CameraTakePhoto(PhotoTransmit);
        }
        else
        {
            ESP_LOGI("camera_transmit_task", "待机模式");
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
    }
}
