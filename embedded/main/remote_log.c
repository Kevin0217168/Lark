/**
 * @file remote_log.c
 * @brief 远程日志上传模块 —— 两阶段初始化，通过 HTTP POST 批量上传 ESP_LOG 日志
 *
 * 架构:
 *   ESP_LOGx ──► 自定义 vprintf 钩子 (阶段一安装)
 *                   ├──► vprintf (串口照常输出)
 *                   └──► 环形缓冲区 16KB (非阻塞写入)
 *                            │
 *                     flush 任务 (阶段二启动, 每 RLOG_FLUSH_INTERVAL_MS)
 *                            │
 *                     HTTP POST ──► /api/logs  (独立 http client, 线程安全)
 *
 * 时序:
 *   remote_log_early_init()   ← NVS 之后, WiFi 之前
 *     → 缓冲区 + 钩子就绪, 日志开始积攒
 *   [WiFi 连接 / SNTP / OTA / WS 握手]  ← 日志全部缓存
 *   remote_log_start()        ← 网络就绪后
 *     → 创建独立 HTTP client + flush 任务, 积攒日志一次性刷出
 *
 * 设计要点:
 *   1. vprintf 钩子中不能阻塞，写缓冲区使用 0-tick 超时
 *   2. 钩子内部不得调用 ESP_LOGx，否则无限递归
 *   3. 使用独立的 esp_http_client (非全局 WifiSecurityClient)，线程安全
 *   4. WS 未就绪时数据保留在缓冲区（不丢弃），满了则新日志丢弃
 *   5. 后端认证: secret 放在 Authorization header
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "remote_log.h"

/* ======================== 配置 ======================== */
#define RLOG_RINGBUF_SIZE       (12 * 1024)  // 环形缓冲区 12KB（覆盖启动阶段日志）
#define RLOG_FLUSH_INTERVAL_MS  1000         // 刷新间隔 1 秒
#define RLOG_FLUSH_BUF_SIZE     (4 * 1024)   // 单次发送最大长度
#define RLOG_LINE_BUF_SIZE      256          // 单行日志最大长度
#define RLOG_TASK_STACK_SIZE    6144         // flush 任务栈（TLS POST 需要较大栈）
#define RLOG_TASK_PRIORITY      1            // flush 任务优先级
#define RLOG_HTTP_TIMEOUT_MS    5000         // HTTP 请求超时
#define RLOG_STATS_INTERVAL     10           // 每 N 次 flush 打印一次统计 (即每 10 秒)

/* ======================== 内部状态 ======================== */
static const char *TAG = "remote_log";

static RingbufHandle_t s_ringbuf = NULL;
/*
 * 环形缓冲区采用 Static 方式创建:
 *   - 控制结构 (StaticRingbuffer_t) 留在内部 SRAM —— 含 FreeRTOS 互斥锁/信号量，
 *     ISR 和调度器需要快速访问，不能放 PSRAM
 *   - 数据存储区 (12 KB) 放在 PSRAM —— 释放宝贵的内部 SRAM
 *
 * SPI 总线安全:
 *   ESP32 上 PSRAM 和 Flash 共享 SPI 总线。Flash 擦写期间 cache 被临时禁用，
 *   此时不能访问 PSRAM。但 ESP-IDF 在 spi_flash_write 时会 stall 双核，
 *   所有任务冻结，vprintf 钩子不会被调用，因此 PSRAM 缓冲区安全。
 *   CONFIG_SPIRAM_CACHE_WORKAROUND=y (已启用) 提供额外保护。
 */
static StaticRingbuffer_t s_ringbuf_struct;
static uint8_t *s_ringbuf_storage = NULL;
static TaskHandle_t s_flush_task_handle = NULL;
static vprintf_like_t s_original_vprintf = NULL;
static volatile bool s_hook_installed = false;  // 阶段一完成
static volatile bool s_upload_started = false;  // 阶段二完成
static volatile bool s_paused = false;          // OTA 暂停标志

/* HTTP POST 配置（阶段二设置） */
static char s_upload_url[192] = {0};
static char s_secret[64] = {0};

/* 诊断统计 */
static volatile uint32_t s_total_bytes_buffered = 0;
static volatile uint32_t s_total_bytes_dropped = 0;
static volatile uint32_t s_total_bytes_sent = 0;
static volatile uint32_t s_total_bytes_send_fail = 0;
static volatile uint32_t s_drop_count = 0;
static volatile uint32_t s_consecutive_fail = 0;  // 连续发送失败计数

#define RLOG_MAX_BACKOFF_MS   30000  // 最大退避 30 秒

/* 用于防止钩子内递归 */
static _Thread_local bool s_in_hook = false;

/* ======================== vprintf 钩子 ======================== */
static int remote_log_vprintf(const char *fmt, va_list args)
{
    /* 1. 先输出到串口（原始行为） */
    int ret = 0;
    if (s_original_vprintf) {
        va_list args_copy;
        va_copy(args_copy, args);
        ret = s_original_vprintf(fmt, args_copy);
        va_end(args_copy);
    }

    /* 2. 防递归 & 未初始化保护 */
    if (s_in_hook || !s_hook_installed || s_ringbuf == NULL) {
        return ret;
    }
    s_in_hook = true;

    /* 3. 格式化日志到临时缓冲区 */
    char line_buf[RLOG_LINE_BUF_SIZE];
    int len = vsnprintf(line_buf, sizeof(line_buf), fmt, args);
    if (len > 0) {
        if (len >= (int)sizeof(line_buf)) {
            len = sizeof(line_buf) - 1;
        }
        /* 4. 尝试写入环形缓冲区，0-tick 超时（不阻塞） */
        BaseType_t ok = xRingbufferSend(s_ringbuf, line_buf, len, 0);
        if (ok == pdTRUE) {
            s_total_bytes_buffered += len;
        } else {
            s_total_bytes_dropped += len;
            s_drop_count++;
        }
    }

    s_in_hook = false;
    return ret;
}

/* ======================== 持久 HTTP Client ======================== */
static esp_http_client_handle_t s_persistent_client = NULL;

/**
 * @brief 获取或创建持久 HTTP client（keep-alive 复用 TLS 连接）
 *
 * 首次调用时创建 client 并完成 TLS 握手，后续请求复用同一连接，
 * 大幅减少 TLS 握手次数，降低内部 SRAM 峰值占用，避免与其他任务
 * 的并发 TLS 握手冲突。
 */
static esp_http_client_handle_t rlog_get_client(void)
{
    if (s_persistent_client != NULL) {
        return s_persistent_client;
    }

    esp_http_client_config_t cfg = {
        .url = s_upload_url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = RLOG_HTTP_TIMEOUT_MS,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .buffer_size = 1024,
        .keep_alive_enable = true,
    };

    s_persistent_client = esp_http_client_init(&cfg);
    if (s_persistent_client != NULL) {
        esp_http_client_set_header(s_persistent_client, "Content-Type", "text/plain");
        esp_http_client_set_header(s_persistent_client, "Authorization", s_secret);
    }
    return s_persistent_client;
}

/**
 * @brief 销毁持久 HTTP client（模块关闭时调用）
 */
static void rlog_destroy_client(void)
{
    if (s_persistent_client != NULL) {
        esp_http_client_close(s_persistent_client);
        esp_http_client_cleanup(s_persistent_client);
        s_persistent_client = NULL;
    }
}

/* ======================== HTTP POST 发送 ======================== */
/**
 * @brief 使用持久 HTTP client 发送日志文本（keep-alive 复用连接）
 *
 * 复用 TLS 连接避免每次 flush 都做 TLS 握手，减少内部 SRAM 峰值占用。
 * 连接断开时自动重建。
 *
 * @return ESP_OK 成功，其它失败
 */
static esp_err_t remote_log_http_post(const char *data, int len)
{
    esp_http_client_handle_t client = rlog_get_client();
    if (client == NULL) {
        return ESP_FAIL;
    }

    /* 设置 body */
    esp_http_client_set_post_field(client, data, len);

    /* 执行请求（keep-alive 下复用已有连接，无需重新 TLS 握手） */
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        if (status != 200 && status != 201 && status != 204) {
            err = ESP_FAIL;
        }
    } else {
        /* 连接级错误（超时、TLS 断开等），销毁 client 下次重建 */
        ESP_LOGW(TAG, "持久连接失败, 将重建: %s", esp_err_to_name(err));
        rlog_destroy_client();
    }

    return err;
}

/* ======================== Flush 任务 ======================== */
static void remote_log_flush_task(void *pvParameters)
{
    /* flush 缓冲区放 PSRAM，节省 4 KB 内部 SRAM（仅在任务上下文中使用，无 DMA） */
    char *flush_buf = heap_caps_malloc(RLOG_FLUSH_BUF_SIZE, MALLOC_CAP_SPIRAM);
    if (flush_buf == NULL) {
        vTaskDelete(NULL);
        return;
    }

    int stats_counter = 0;

    while (s_upload_started) {
        /* OTA 暂停期间：跳过 HTTP 发送，日志留在缓冲区，同时释放持久连接 */
        if (s_paused) {
            rlog_destroy_client();
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        // 动态退避：连续失败时逐渐增大间隔，避免网络故障时狂刷 HTTP
        uint32_t delay_ms = RLOG_FLUSH_INTERVAL_MS;
        if (s_consecutive_fail > 0) {
            delay_ms = RLOG_FLUSH_INTERVAL_MS * (1u << (s_consecutive_fail > 4 ? 4 : s_consecutive_fail));
            if (delay_ms > RLOG_MAX_BACKOFF_MS) delay_ms = RLOG_MAX_BACKOFF_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(delay_ms));

        if (s_ringbuf == NULL) {
            continue;
        }

        /* 从环形缓冲区批量读取 */
        int total_len = 0;
        size_t item_size = 0;
        void *item = NULL;

        while (total_len < RLOG_FLUSH_BUF_SIZE - 1) {
            size_t remain = RLOG_FLUSH_BUF_SIZE - 1 - total_len;
            item = xRingbufferReceiveUpTo(s_ringbuf, &item_size, 0, remain);
            if (item == NULL || item_size == 0) {
                if (item) vRingbufferReturnItem(s_ringbuf, item);
                break;
            }
            memcpy(flush_buf + total_len, item, item_size);
            total_len += item_size;
            vRingbufferReturnItem(s_ringbuf, item);
        }

        /* 记录发送前的缓冲区使用量（用于统计） */
        size_t pre_flush_free = xRingbufferGetCurFreeSize(s_ringbuf);
        size_t pre_flush_used = RLOG_RINGBUF_SIZE - pre_flush_free;

        if (total_len > 0) {
            flush_buf[total_len] = '\0';
            esp_err_t err = remote_log_http_post(flush_buf, total_len);
            if (err == ESP_OK) {
                s_total_bytes_sent += total_len;
                s_consecutive_fail = 0;
            } else {
                s_total_bytes_send_fail += total_len;
                s_consecutive_fail++;
                // 发送失败时尝试将数据写回缓冲区，下次重试
                // 使用 0-tick 超时，缓冲区满则丢弃（避免阻塞）
                xRingbufferSend(s_ringbuf, flush_buf, total_len, 0);
            }
        }

        /* 定期打印诊断统计 */
        stats_counter++;
        if (stats_counter >= RLOG_STATS_INTERVAL) {
            stats_counter = 0;
            ESP_LOGI(TAG, "[stats] flushed=%d B, pending=%u/%u bytes (%.0f%%), sent=%u B, drop=%u B (%u times), send_fail=%u B, consec_fail=%u",
                     total_len,
                     (unsigned)(pre_flush_used + total_len), RLOG_RINGBUF_SIZE,
                     (float)(pre_flush_used + total_len) / RLOG_RINGBUF_SIZE * 100.0f,
                     (unsigned)s_total_bytes_sent,
                     (unsigned)s_total_bytes_dropped, (unsigned)s_drop_count,
                     (unsigned)s_total_bytes_send_fail,
                     (unsigned)s_consecutive_fail);
        }
    }

    free(flush_buf);
    vTaskDelete(NULL);
}

/* ======================== 公共 API ======================== */

esp_err_t remote_log_early_init(void)
{
    if (s_hook_installed) {
        return ESP_ERR_INVALID_STATE;
    }

    /* 1. 创建环形缓冲区 —— 数据区域放 PSRAM，控制结构留内部 SRAM */
    s_ringbuf_storage = heap_caps_malloc(RLOG_RINGBUF_SIZE, MALLOC_CAP_SPIRAM);
    if (s_ringbuf_storage == NULL) {
        ESP_LOGE(TAG, "无法从 PSRAM 分配环形缓冲区存储 (%d bytes)", RLOG_RINGBUF_SIZE);
        return ESP_ERR_NO_MEM;
    }
    s_ringbuf = xRingbufferCreateStatic(RLOG_RINGBUF_SIZE, RINGBUF_TYPE_BYTEBUF,
                                         s_ringbuf_storage, &s_ringbuf_struct);
    if (s_ringbuf == NULL) {
        ESP_LOGE(TAG, "无法创建环形缓冲区");
        heap_caps_free(s_ringbuf_storage);
        s_ringbuf_storage = NULL;
        return ESP_ERR_NO_MEM;
    }

    /* 2. 安装 vprintf 钩子 */
    s_hook_installed = true;
    s_original_vprintf = esp_log_set_vprintf(remote_log_vprintf);

    ESP_LOGI(TAG, "阶段一完成: vprintf 钩子已安装, 缓冲区 %d KB", RLOG_RINGBUF_SIZE / 1024);
    return ESP_OK;
}

esp_err_t remote_log_start(const char *base_url, const char *path,
                           uint16_t port, const char *secret)
{
    if (!s_hook_installed) {
        ESP_LOGE(TAG, "请先调用 remote_log_early_init()");
        return ESP_ERR_INVALID_STATE;
    }
    if (s_upload_started) {
        return ESP_ERR_INVALID_STATE;
    }

    /* 1. 构建完整 URL 和保存 secret */
    snprintf(s_upload_url, sizeof(s_upload_url), "%s:%d%s", base_url, port, path);
    strncpy(s_secret, secret, sizeof(s_secret) - 1);
    s_secret[sizeof(s_secret) - 1] = '\0';

    ESP_LOGI(TAG, "日志上传地址: %s", s_upload_url);

    /* 2. 启动 flush 任务（栈放 PSRAM，节省内部 SRAM） */
    s_upload_started = true;
    BaseType_t ret = xTaskCreateWithCaps(
        remote_log_flush_task, "rlog_flush",
        RLOG_TASK_STACK_SIZE, NULL,
        RLOG_TASK_PRIORITY, &s_flush_task_handle,
        MALLOC_CAP_SPIRAM);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "无法创建 flush 任务");
        s_upload_started = false;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "阶段二完成: flush 任务已启动 (POST, 每 %d ms)", RLOG_FLUSH_INTERVAL_MS);
    return ESP_OK;
}

bool remote_log_is_connected(void)
{
    return s_upload_started;
}

void remote_log_pause(void)
{
    s_paused = true;
    ESP_LOGI(TAG, "远程日志上传已暂停 (释放 TLS 连接)");
}

void remote_log_resume(void)
{
    s_paused = false;
    ESP_LOGI(TAG, "远程日志上传已恢复");
}

void remote_log_flush_sync(void)
{
    if (!s_upload_started || s_ringbuf == NULL || s_paused) {
        return;
    }

    /* 临时 flush 缓冲区（复用栈上的小块 + 多次循环，避免额外 malloc） */
    char tmp[512];
    int rounds = 0;
    const int max_rounds = (RLOG_RINGBUF_SIZE / sizeof(tmp)) + 2; // 防止无限循环

    while (rounds < max_rounds) {
        int total_len = 0;
        size_t item_size = 0;
        void *item = NULL;

        while (total_len < (int)sizeof(tmp) - 1) {
            size_t remain = sizeof(tmp) - 1 - total_len;
            item = xRingbufferReceiveUpTo(s_ringbuf, &item_size, 0, remain);
            if (item == NULL || item_size == 0) {
                if (item) vRingbufferReturnItem(s_ringbuf, item);
                break;
            }
            memcpy(tmp + total_len, item, item_size);
            total_len += item_size;
            vRingbufferReturnItem(s_ringbuf, item);
        }

        if (total_len == 0) break; // 缓冲区已空

        tmp[total_len] = '\0';
        remote_log_http_post(tmp, total_len);
        rounds++;
    }
}

void remote_log_deinit(void)
{
    /* 1. 先恢复原始 vprintf */
    if (s_original_vprintf) {
        esp_log_set_vprintf(s_original_vprintf);
        s_original_vprintf = NULL;
    }
    s_hook_installed = false;

    /* 2. 停止 flush 任务 */
    s_upload_started = false;
    if (s_flush_task_handle) {
        vTaskDelay(pdMS_TO_TICKS(RLOG_FLUSH_INTERVAL_MS + 500));
        s_flush_task_handle = NULL;
    }

    /* 3. 销毁持久 HTTP client */
    rlog_destroy_client();

    /* 4. 释放缓冲区（静态创建的 ring buffer 不能调用 vRingbufferDelete） */
    s_ringbuf = NULL;
    if (s_ringbuf_storage) {
        heap_caps_free(s_ringbuf_storage);
        s_ringbuf_storage = NULL;
    }

    ESP_LOGI(TAG, "远程日志模块已停止");
}
