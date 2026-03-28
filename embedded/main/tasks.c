#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

#include "Wifista.h"
#include "Camera.h"
#include "main.h"
#include "sht4x.h"
#include "device_secret.h"
#include "i2c_recovery.h"
#include "tasks.h"

static const char *TAG = "tasks";

// 健康监控配置
#define HEALTH_CHECK_INTERVAL_MS    10000   // 健康检查间隔 10 秒
#define HEALTH_MAX_DISCONNECT_COUNT 30      // 最大连续断连次数（30 × 10s = 5 分钟）

extern Device_t device;
extern bool Wifi_isConnected;

/* ───────────────── 诊断 ───────────────── */

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

/* ───────────────── 摄像头图像传输 ───────────────── */

static void PhotoTransmit(camera_fb_t *fb)
{
    WebsocketSendbytes(fb->buf, fb->len);
}

void camera_transmit_task(void *pvParameter)
{
    (void)pvParameter;
    while (1)
    {
        if (device.status == DEVICE_ON_STREAM)
        {
            CameraTakePhoto(PhotoTransmit);
        }
        else
        {
            ESP_LOGI(TAG, "摄像头待机模式");
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
    }
}

/* ───────────────── 传感器数据采集与上报 ───────────────── */

void sensor_data_transmit_task(void *pvParameter)
{
    (void)pvParameter;
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
            ESP_LOGI(TAG, "measured temperature: %0.2f °C, humidity: %0.2f %%RH",
                     temp_c, hum_pct);

            char post_data[256];
            if (is_time_valid()) {
                time(&now);
                setenv("TZ", "CST-8", 1);
                localtime_r(&now, &timeinfo);
                ESP_LOGI(TAG, "当前时间: %04d-%02d-%02d %02d:%02d:%02d+08:00",
                         timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                         timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

                char timestamp_str[32];
                strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%dT%H:%M:%S+08:00", &timeinfo);

                snprintf(post_data, sizeof(post_data),
                         "{\"secret\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f,\"timestamp\":\"%s\"}",
                         secret, temp_c, hum_pct, timestamp_str);
            } else {
                ESP_LOGW(TAG, "时间尚未同步，不附带时间戳，由后端补充");
                snprintf(post_data, sizeof(post_data),
                         "{\"secret\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f}",
                         secret, temp_c, hum_pct);
            }

            int ret_code = WifiSecurityRequest("https://lark.mintlab.top", "/api/sensors", 443,
                                               WS_CLINENT_METHOD_POST, post_data, NULL);
            if (ret_code != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to send sensor data, error=%d", ret_code);
            }
        }
        else
        {
            ESP_LOGE(TAG, "error reading measurement");
        }

        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
}

/* ───────────────── 健康监控 ───────────────── */

// 每隔多少个健康检查周期输出一次系统状态（6 × 10s = 60s）
#define SYSTEM_STATUS_LOG_INTERVAL  6

/**
 * @brief 打印所有任务状态及系统内存使用情况（紧凑格式，适合远程日志）
 *
 * 内存说明：
 *   - "堆" = esp_get_free_heap_size()，包含内部 SRAM + PSRAM 的总空闲
 *   - "内部" = MALLOC_CAP_INTERNAL，仅芯片内置 SRAM（约 320 KB）
 *   - "PSRAM" = MALLOC_CAP_SPIRAM，外挂 SPI RAM（板载 4 MB）
 *   - xTaskCreate() 分配的任务栈来自 **内部 SRAM**
 *     （如需放到 PSRAM，须用 xTaskCreateWithCaps + MALLOC_CAP_SPIRAM）
 *   - "历史最低" 反映运行以来堆空闲的最小值，是内存泄漏的关键指标
 */
static void log_system_status(void)
{
    /* ── 内存概览（每条独立输出，避免单条日志过长被截断或被其他任务打断） ── */
    size_t free_heap      = esp_get_free_heap_size();
    size_t min_free_heap  = esp_get_minimum_free_heap_size();
    size_t total_internal = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    size_t free_internal  = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t total_spiram   = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    size_t free_spiram    = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    int pct_internal = total_internal ? (int)(free_internal * 100 / total_internal) : 0;
    int pct_spiram   = total_spiram   ? (int)(free_spiram   * 100 / total_spiram)   : 0;
    int pct_heap     = (total_internal + total_spiram) ?
        (int)(free_heap * 100 / (total_internal + total_spiram)) : 0;

    ESP_LOGI("trace", "[内存] 堆=%u/%uB(%d%%) 最低=%uB | 内部=%u/%uB(%d%%) | PSRAM=%u/%uB(%d%%)",
        (unsigned)free_heap, (unsigned)(total_internal + total_spiram), pct_heap, (unsigned)min_free_heap,
        (unsigned)free_internal, (unsigned)total_internal, pct_internal,
        (unsigned)free_spiram, (unsigned)total_spiram, pct_spiram);

    /* ── 任务列表（逐条打印，每条 < 120 字节，远程日志不会截断） ── */
    UBaseType_t task_count = uxTaskGetNumberOfTasks();
    TaskStatus_t *task_array = malloc(task_count * sizeof(TaskStatus_t));
    if (task_array == NULL) {
        ESP_LOGW(TAG, "[系统状态] 任务快照分配失败");
        return;
    }

    UBaseType_t got = uxTaskGetSystemState(task_array, task_count, NULL);

    static const char *state_str[] = {"运行","就绪","阻塞","挂起","删除","未知"};

    ESP_LOGI("trace", "[任务×%u] %-20s %-6s %-4s %-8s",
             (unsigned)got, "名称", "状态", "优先", "栈剩(B)");

    for (UBaseType_t i = 0; i < got; i++) {
        int st = task_array[i].eCurrentState;
        if (st < 0 || st > 4) st = 5;
        ESP_LOGI("trace", "  %-20s %-6s %-4u %-8u",
                 task_array[i].pcTaskName,
                 state_str[st],
                 (unsigned)task_array[i].uxCurrentPriority,
                 (unsigned)(task_array[i].usStackHighWaterMark * sizeof(StackType_t)));
    }

    free(task_array);
}

void health_monitor_task(void *pvParameter)
{
    (void)pvParameter;
    int wifi_disconnect_count = 0;
    int ws_disconnect_count = 0;
    int status_log_counter = 0;

    // 启动后等待一段时间，让系统完成初始化
    vTaskDelay(pdMS_TO_TICKS(HEALTH_CHECK_INTERVAL_MS));

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(HEALTH_CHECK_INTERVAL_MS));

        // 定期输出系统状态（任务列表 + 内存）
        status_log_counter++;
        if (status_log_counter >= SYSTEM_STATUS_LOG_INTERVAL) {
            status_log_counter = 0;
            log_system_status();
        }

        // 检查 WiFi 连接
        if (!Wifi_isConnected) {
            wifi_disconnect_count++;
            if (wifi_disconnect_count % 6 == 1) {
                ESP_LOGW(TAG, "[健康监控] WiFi 已断开 %d/%d 次 (~%ds)",
                         wifi_disconnect_count, HEALTH_MAX_DISCONNECT_COUNT,
                         wifi_disconnect_count * HEALTH_CHECK_INTERVAL_MS / 1000);
            }
        } else {
            if (wifi_disconnect_count > 0) {
                ESP_LOGI(TAG, "[健康监控] WiFi 已恢复连接");
            }
            wifi_disconnect_count = 0;
        }

        // 检查 WebSocket 连接
        if (!WebsocketIsConnected()) {
            ws_disconnect_count++;
            if (ws_disconnect_count % 6 == 1) {
                ESP_LOGW(TAG, "[健康监控] WebSocket 已断开 %d/%d 次 (~%ds)",
                         ws_disconnect_count, HEALTH_MAX_DISCONNECT_COUNT,
                         ws_disconnect_count * HEALTH_CHECK_INTERVAL_MS / 1000);
            }
        } else {
            if (ws_disconnect_count > 0) {
                ESP_LOGI(TAG, "[健康监控] WebSocket 已恢复连接");
            }
            ws_disconnect_count = 0;
        }

        // 连续断连超过阈值，自动重启
        if (wifi_disconnect_count >= HEALTH_MAX_DISCONNECT_COUNT) {
            ESP_LOGE(TAG, "[健康监控] WiFi 持续断开超过 %d 秒，重启设备...",
                     HEALTH_MAX_DISCONNECT_COUNT * HEALTH_CHECK_INTERVAL_MS / 1000);
            esp_restart();
        }
        if (ws_disconnect_count >= HEALTH_MAX_DISCONNECT_COUNT) {
            ESP_LOGE(TAG, "[健康监控] WebSocket 持续断开超过 %d 秒，重启设备...",
                     HEALTH_MAX_DISCONNECT_COUNT * HEALTH_CHECK_INTERVAL_MS / 1000);
            esp_restart();
        }
    }
}
