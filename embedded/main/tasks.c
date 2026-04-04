#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
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
#include "remote_log.h"

static const char *TAG = "task";

// 健康监控配置
#define HEALTH_CHECK_INTERVAL_MS    10000   // 健康检查间隔 10 秒
#define HEALTH_MAX_DISCONNECT_COUNT 30      // 最大连续断连次数（30 × 10s = 5 分钟）

extern Device_t device;
extern bool Wifi_isConnected;

/* ───── 摄像头推流信号量 ───── */
SemaphoreHandle_t camera_stream_sem = NULL;

/* ───── OTA 进行中标志 ───── */
volatile bool ota_in_progress = false;

void camera_stream_sem_init(void)
{
    camera_stream_sem = xSemaphoreCreateBinary();
    configASSERT(camera_stream_sem);
}

/* ───────────────── 诊断 ───────────────── */

bool diagnostic(void)
{
    ESP_LOGI(TAG, "固件诊断开始");

    // 1. I2C 总线恢复 + 检查 SHT4x 传感器
    i2c_bus_recovery(I2C_SDA_PIN, I2C_SCL_PIN);
    sensirion_i2c_init();
    if (sht4x_probe() != STATUS_OK)
    {
        ESP_LOGE(TAG, "诊断失败: SHT4x 传感器未响应");
        return false;
    }
    ESP_LOGI(TAG, "SHT4x 探测成功");

    // 2. 检查摄像头初始化
    CameraInit();
    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL)
    {
        ESP_LOGE(TAG, "诊断失败: 无法获取摄像头传感器指针");
        return false;
    }
    ESP_LOGI(TAG, "摄像头初始化成功");
    return true;
}

/* ───────────────── 摄像头图像传输 ───────────────── */

/* 推流发送超时（ms）——超过则丢弃当前帧，保持流畅 */
#define STREAM_SEND_TIMEOUT_MS  2000

static void PhotoTransmit(camera_fb_t *fb)
{
    if (!WebsocketSendbytesTimeout(fb->buf, fb->len, pdMS_TO_TICKS(STREAM_SEND_TIMEOUT_MS))) {
        ESP_LOGW(TAG, "帧发送超时/失败 (size=%u), 丢弃", (unsigned)fb->len);
    }
}

/* 推流帧间延迟（ms）——让 WS 任务处理 PING/PONG */
#define STREAM_INTER_FRAME_DELAY_MS  1

void camera_transmit_task(void *pvParameter)
{
    (void)pvParameter;
    while (1)
    {
        /* 阻塞等待信号量，直到被唤醒才开始推流，不占用 CPU */
        ESP_LOGI(TAG, "摄像头待机, 等待推流信号");
        xSemaphoreTake(camera_stream_sem, portMAX_DELAY);

        ESP_LOGI(TAG, "开始图像推流");
        /* 持续推流，直到状态不再是 DEVICE_ON_STREAM */
        while (device.status == DEVICE_ON_STREAM)
        {
            /* 检查 WebSocket 是否仍在线 */
            if (!WebsocketIsConnected()) {
                ESP_LOGW(TAG, "推流中断: WebSocket 已断开, 等待重连...");
                /* 等待 WS 重连，但每秒检查一次状态是否已切换 */
                while (!WebsocketIsConnected() && device.status == DEVICE_ON_STREAM) {
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
                if (device.status != DEVICE_ON_STREAM) break;
                ESP_LOGI(TAG, "WebSocket 已重连, 继续推流");
            }

            CameraTakePhoto(PhotoTransmit);

            /* 帧间延迟：让出 CPU，让 WS 任务处理 PING/PONG 和其他协议帧 */
            vTaskDelay(pdMS_TO_TICKS(STREAM_INTER_FRAME_DELAY_MS));
        }
        ESP_LOGI(TAG, "推流已停止");
        /* 状态已切换，回到外层循环重新等待信号量 */
    }
}

/* ───────────────── 传感器数据采集与上报 ───────────────── */

// 传感器上报重试配置
#define SENSOR_POST_MAX_RETRY   3       // 每轮最大重试次数
#define SENSOR_POST_RETRY_MS    3000    // 重试间隔 3 秒（等待其他 TLS 连接释放内部 SRAM）
#define SENSOR_CONSEC_FAIL_MAX  5       // 连续多轮全失败后重启（即连续5分钟全失败）

void sensor_data_transmit_task(void *pvParameter)
{
    (void)pvParameter;
    time_t now = 0;
    struct tm timeinfo = {0};
    int consec_fail_rounds = 0;  // 连续多轮全失败计数

    // 启动后延迟 5 秒，错开 OTA/rlog 首次 TLS 握手的内存高峰
    vTaskDelay(pdMS_TO_TICKS(5000));

    while (1)
    {
        int32_t temperature, humidity;
        int8_t ret = sht4x_measure_blocking_read(&temperature, &humidity);
        if (ret == STATUS_OK)
        {
            float temp_c = temperature / 1000.0f;
            float hum_pct = humidity / 1000.0f;
            ESP_LOGI(TAG, "温度: %.2f°C, 湿度: %.1f%%",
                     temp_c, hum_pct);

            char post_data[256];
            if (is_time_valid()) {
                time(&now);
                setenv("TZ", "CST-8", 1);
                localtime_r(&now, &timeinfo);
                ESP_LOGD(TAG, "时间: %04d-%02d-%02d %02d:%02d:%02d",
                         timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                         timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

                char timestamp_str[32];
                strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%dT%H:%M:%S+08:00", &timeinfo);

                snprintf(post_data, sizeof(post_data),
                         "{\"secret\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f,\"timestamp\":\"%s\"}",
                         secret, temp_c, hum_pct, timestamp_str);
            } else {
                ESP_LOGW(TAG, "时间未同步, 由后端补充时间戳");
                snprintf(post_data, sizeof(post_data),
                         "{\"secret\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f}",
                         secret, temp_c, hum_pct);
            }

            // 带重试的 HTTPS POST（并发 TLS 握手可能偶发失败）
            int ret_code = ESP_FAIL;
            if (ota_in_progress) {
                ESP_LOGW(TAG, "OTA 进行中，跳过传感器上报");
            } else {
            for (int attempt = 0; attempt < SENSOR_POST_MAX_RETRY; attempt++) {
                if (ota_in_progress) {
                    ESP_LOGW(TAG, "OTA 进行中，取消传感器上报重试");
                    break;
                }
                ret_code = WifiSecurityRequest("https://lark.mintlab.top", "/api/sensors", 443,
                                               WS_CLINENT_METHOD_POST, post_data, NULL);
                if (ret_code == ESP_OK) {
                    break;
                }
                ESP_LOGW(TAG, "传感器上报失败 (%d/%d), %dms 后重试...",
                         attempt + 1, SENSOR_POST_MAX_RETRY, SENSOR_POST_RETRY_MS);
                vTaskDelay(pdMS_TO_TICKS(SENSOR_POST_RETRY_MS));
            }
            }
            if (ret_code != ESP_OK)
            {
                consec_fail_rounds++;
                ESP_LOGE(TAG, "传感器数据上报失败 (已重试 %d 次), 连续失败轮数=%d/%d",
                         SENSOR_POST_MAX_RETRY, consec_fail_rounds, SENSOR_CONSEC_FAIL_MAX);
                if (consec_fail_rounds >= SENSOR_CONSEC_FAIL_MAX) {
                    ESP_LOGE(TAG, "传感器上报连续 %d 轮失败，可能内存碎片化或网络故障，重启设备...",
                             consec_fail_rounds);
                    remote_log_flush_sync();
                    esp_restart();
                }
            }
            else
            {
                consec_fail_rounds = 0;
            }
        }
        else
        {
            ESP_LOGE(TAG, "SHT4x 读取失败");
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

    size_t largest_internal = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);

    ESP_LOGI("sys_mon", "[内存] 堆=%u/%uB(%d%%) 最低=%uB | 内部=%u/%uB(%d%%) 最大连续=%uB | PSRAM=%u/%uB(%d%%)",
        (unsigned)free_heap, (unsigned)(total_internal + total_spiram), pct_heap, (unsigned)min_free_heap,
        (unsigned)free_internal, (unsigned)total_internal, pct_internal, (unsigned)largest_internal,
        (unsigned)free_spiram, (unsigned)total_spiram, pct_spiram);

    /* ── 任务列表（逐条打印，每条 < 120 字节，远程日志不会截断） ── */
    /* 使用静态数组避免反复 malloc/free 加剧内存碎片化 */
    #define MAX_MONITORED_TASKS 20
    static TaskStatus_t task_array[MAX_MONITORED_TASKS];
    UBaseType_t task_count = uxTaskGetNumberOfTasks();
    if (task_count > MAX_MONITORED_TASKS) task_count = MAX_MONITORED_TASKS;

    UBaseType_t got = uxTaskGetSystemState(task_array, task_count, NULL);

    static const char *state_str[] = {"运行","就绪","阻塞","挂起","删除","未知"};

    ESP_LOGI("sys_mon", "[任务×%u] %-20s %-6s %-4s %-8s",
             (unsigned)got, "名称", "状态", "优先", "栈剩(B)");

    for (UBaseType_t i = 0; i < got; i++) {
        int st = task_array[i].eCurrentState;
        if (st < 0 || st > 4) st = 5;
        ESP_LOGI("sys_mon", "  %-20s %-6s %-4u %-8u",
                 task_array[i].pcTaskName,
                 state_str[st],
                 (unsigned)task_array[i].uxCurrentPriority,
                 (unsigned)(task_array[i].usStackHighWaterMark * sizeof(StackType_t)));
    }

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
            remote_log_flush_sync();
            esp_restart();
        }
        if (ws_disconnect_count >= HEALTH_MAX_DISCONNECT_COUNT) {
            ESP_LOGE(TAG, "[健康监控] WebSocket 持续断开超过 %d 秒，重启设备...",
                     HEALTH_MAX_DISCONNECT_COUNT * HEALTH_CHECK_INTERVAL_MS / 1000);
            remote_log_flush_sync();
            esp_restart();
        }

        // 内存碎片化监控（仅记录日志，不主动重启）
        // 实际重启由传感器上报连续失败 / WiFi/WS 断连检测触发
        {
            size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
            size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
            if (largest_block < 12288) {
                ESP_LOGW(TAG, "[健康监控] 内存碎片化: 内部空闲=%u B, 最大连续块=%u B",
                         (unsigned)free_internal, (unsigned)largest_block);
            }
        }
    }
}
