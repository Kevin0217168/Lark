#include "Wifista.h"
#include "esp_camera.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_heap_caps.h"
#include <errno.h>

/* 来自 main 组件的外部符号（避免循环依赖不直接 include） */
extern volatile bool ota_in_progress;
extern void remote_log_pause(void);
extern void remote_log_resume(void);

static const char *TAG = "ota";

// ===================== OTA 服务器配置 =====================
// 生产环境
#define OTA_API_LATEST   "https://lark.mintlab.top/api/firmwares/latest"
#define OTA_API_DOWNLOAD "https://lark.mintlab.top/api/firmwares/download"
// 本地调试（取消注释并注释上面两行即可切换）
// #define OTA_API_LATEST   "http://192.168.1.200:8080/api/firmwares/latest"
// #define OTA_API_DOWNLOAD "http://192.168.1.200:8080/api/firmwares/download"

#define OTA_RECV_TIMEOUT       30000
#define OTA_CHECK_MAX_RETRY    6        // 版本检查最大重试次数
#define OTA_CHECK_RETRY_MS     3000     // 重试间隔（等待其他 TLS 连接释放内部 SRAM）
#define OTA_DOWNLOAD_MAX_RETRY 3        // 固件下载最大重试次数
#define OTA_DOWNLOAD_RETRY_MS  5000     // 下载重试间隔（等待堆内存整理）

#define BUFFSIZE 8196

/* OTA 数据写入缓冲区 */
static char ota_write_data[BUFFSIZE + 1] = {0};

/* 从 main.c 获取设备密钥 */
extern char secret[];

static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

static void __attribute__((noreturn)) task_fatal_error(void)
{
    ESP_LOGE(TAG, "Exiting task due to fatal error...");
    (void)vTaskDelete(NULL);
    while (1) {
        ;
    }
}

// ===================== Step 1: 查询最新固件版本 =====================
/**
 * @brief 向服务器查询最新固件版本信息
 *        POST /api/firmwares/latest   body: {"secret":"..."}
 *
 * @param[out] out_version  接收版本号字符串（调用者需提供至少 32 字节缓冲区）
 * @param version_buf_len   缓冲区长度
 * @return true  查询成功，out_version 已填充
 * @return false 请求失败 / 设备未注册 / 无可用固件
 */
static bool ota_check_latest_version(char *out_version, size_t version_buf_len)
{
    char post_data[128];
    snprintf(post_data, sizeof(post_data), "{\"secret\":\"%s\"}", secret);

    esp_http_client_config_t config = {
        .url = OTA_API_LATEST,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = OTA_RECV_TIMEOUT,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client for version check");
        return false;
    }

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    /* 打开连接并发送 POST body */
    esp_err_t err = esp_http_client_open(client, strlen(post_data));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }

    int wlen = esp_http_client_write(client, post_data, strlen(post_data));
    if (wlen < 0) {
        ESP_LOGE(TAG, "Failed to write POST body");
        http_cleanup(client);
        return false;
    }

    /* 读取响应头 */
    esp_http_client_fetch_headers(client);
    int status_code = esp_http_client_get_status_code(client);

    if (status_code != 200) {
        ESP_LOGW(TAG, "Version check returned HTTP %d (device not found / no firmware)", status_code);
        http_cleanup(client);
        return false;
    }

    /* 读取 JSON 响应体 */
    char resp[512] = {0};
    int total = 0;
    while (total < (int)sizeof(resp) - 1) {
        int n = esp_http_client_read(client, resp + total, sizeof(resp) - 1 - total);
        if (n <= 0) break;
        total += n;
    }
    resp[total] = '\0';
    http_cleanup(client);

    ESP_LOGI(TAG, "Latest firmware response: %s", resp);

    /* 解析 JSON: {"code":200,"msg":"...","data":{"version":"x.y.z",...}} */
    cJSON *root = cJSON_Parse(resp);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse version check JSON");
        return false;
    }

    bool ok = false;
    cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (data) {
        cJSON *ver = cJSON_GetObjectItemCaseSensitive(data, "version");
        if (cJSON_IsString(ver) && ver->valuestring) {
            strncpy(out_version, ver->valuestring, version_buf_len - 1);
            out_version[version_buf_len - 1] = '\0';
            ESP_LOGI(TAG, "Server latest firmware version: %s", out_version);
            ok = true;
        }
    }

    cJSON_Delete(root);
    return ok;
}

// ===================== Step 2: 下载固件并执行 OTA =====================
/**
 * @brief 从服务器下载最新固件并写入 OTA 分区
 *        POST /api/firmwares/download   body: {"secret":"..."}
 *        响应为 application/octet-stream 固件二进制流
 *
 * @param running  当前运行的分区指针
 * @return true    OTA 成功（函数内部会自动 reboot，不会真正返回 true）
 * @return false   下载/验证失败，可由调用方决定是否重试
 */
static bool ota_download_and_update(const esp_partition_t *running)
{
    esp_err_t err;
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;
    bool ota_begun = false;

    char post_data[128];
    snprintf(post_data, sizeof(post_data), "{\"secret\":\"%s\"}", secret);

    esp_http_client_config_t config = {
        .url = OTA_API_DOWNLOAD,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = OTA_RECV_TIMEOUT,
        .keep_alive_enable = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client for firmware download");
        return false;
    }

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    err = esp_http_client_open(client, strlen(post_data));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open download connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }

    int wlen = esp_http_client_write(client, post_data, strlen(post_data));
    if (wlen < 0) {
        ESP_LOGE(TAG, "Failed to write POST body for download");
        http_cleanup(client);
        return false;
    }

    esp_http_client_fetch_headers(client);
    int status_code = esp_http_client_get_status_code(client);
    if (status_code != 200) {
        ESP_LOGE(TAG, "Firmware download returned HTTP %d", status_code);
        http_cleanup(client);
        return false;
    }

    update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%"PRIx32,
             update_partition->subtype, update_partition->address);

    int binary_file_length = 0;
    bool image_header_was_checked = false;

    while (1) {
        int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
        if (data_read < 0) {
            ESP_LOGE(TAG, "Error: data read error");
            http_cleanup(client);
            if (ota_begun) esp_ota_abort(update_handle);
            return false;
        } else if (data_read > 0) {
            if (!image_header_was_checked) {
                esp_app_desc_t new_app_info;
                if (data_read > (int)(sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))) {
                    // 从二进制头部提取版本信息
                    memcpy(&new_app_info,
                           &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)],
                           sizeof(esp_app_desc_t));
                    ESP_LOGI(TAG, "Downloaded firmware version: %s", new_app_info.version);

                    esp_app_desc_t running_app_info;
                    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
                    }

                    // 检查是否与上次回滚的无效固件版本相同
                    const esp_partition_t *last_invalid = esp_ota_get_last_invalid_partition();
                    esp_app_desc_t invalid_info;
                    if (last_invalid != NULL &&
                        esp_ota_get_partition_description(last_invalid, &invalid_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_info.version);
                        if (memcmp(invalid_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
                            ESP_LOGW(TAG, "New version matches previously failed version, aborting OTA");
                            http_cleanup(client);
                            return false;  // 返回主循环，下次再检查
                        }
                    }

                    image_header_was_checked = true;

                    // 摄像头已在 ota_task 入口处关闭
                    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        http_cleanup(client);
                        esp_ota_abort(update_handle);
                        task_fatal_error();
                    }
                    ota_begun = true;
                    ESP_LOGI(TAG, "esp_ota_begin succeeded");
                } else {
                    ESP_LOGE(TAG, "First packet too small for image header");
                    http_cleanup(client);
                    return false;
                }
            }
            err = esp_ota_write(update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK) {
                http_cleanup(client);
                esp_ota_abort(update_handle);
                task_fatal_error();
            }
            binary_file_length += data_read;
            ESP_LOGD(TAG, "Written image length %d", binary_file_length);
            // 让出 CPU 时间给其他任务，避免看门狗超时
            vTaskDelay(1);
        } else if (data_read == 0) {
            if (errno == ECONNRESET || errno == ENOTCONN) {
                ESP_LOGE(TAG, "Connection closed, errno = %d", errno);
                break;
            }
            if (esp_http_client_is_complete_data_received(client) == true) {
                ESP_LOGI(TAG, "Firmware download complete");
                break;
            }
        }
    }

    ESP_LOGI(TAG, "Total firmware binary length: %d", binary_file_length);
    if (esp_http_client_is_complete_data_received(client) != true) {
        ESP_LOGE(TAG, "Incomplete firmware download");
        http_cleanup(client);
        if (ota_begun) esp_ota_abort(update_handle);
        return false;
    }

    http_cleanup(client);

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, firmware is corrupted");
        } else {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
        task_fatal_error();
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        task_fatal_error();
    }

    ESP_LOGI(TAG, "OTA succeeded! Restarting...");
    esp_restart();
    return true;  // 实际不会到达这里
}

// ===================== OTA 主任务 =====================
/**
 * @brief OTA 任务入口：检查一次服务器固件版本，有新版本则下载更新，无论结果如何任务结束后自行退出
 *
 * 流程：
 *   ① POST /api/firmwares/latest  {"secret":"..."}  → 获取最新固件版本号
 *   ② 比较本地版本与服务器版本
 *   ③ 若有更新 → POST /api/firmwares/download {"secret":"..."} → 流式下载固件
 *   ④ 写入 flash → 重启
 *   ⑤ 若无更新 / 失败 → 任务退出，等待下次启动或 WS 命令再触发
 */
void ota_task(void *pvParameter)
{
    ota_context_t *context = (ota_context_t *)pvParameter;
    bool ota_pending = context ? context->ota_pending_flag : false;
    SemaphoreHandle_t ota_sem = context ? context->ota_sem : NULL;

    ESP_LOGI(TAG, "OTA task started (ota_pending=%d)", ota_pending);

    /* 暂停其他 TLS 任务，释放内部 SRAM 给 OTA TLS 握手使用 */
    ota_in_progress = true;
    remote_log_pause();

    /* 关闭摄像头释放内部 SRAM（DMA 描述符等），避免 SPI 总线冲突 */
    ESP_LOGI(TAG, "OTA: 关闭摄像头以释放内部 SRAM...");
    esp_camera_deinit();

    /* 等待其他任务的 TLS 连接释放 & 堆内存整理 */
    vTaskDelay(pdMS_TO_TICKS(3000));

    ESP_LOGI(TAG, "OTA: 内部空闲=%u B, 最大连续块=%u B",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08"PRIx32", but running from offset 0x%08"PRIx32,
                 configured->address, running->address);
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08"PRIx32")",
             running->type, running->subtype, running->address);

    esp_app_desc_t running_info;
    if (esp_ota_get_partition_description(running, &running_info) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get running firmware description");
        if (ota_pending) {
            ESP_LOGE(TAG, "OTA 检查失败, 回滚到上一个版本...");
            esp_ota_mark_app_invalid_rollback_and_reboot();
        }
        goto exit;
    }
    ESP_LOGI(TAG, "Running firmware version: %s", running_info.version);

    ESP_LOGI(TAG, "Checking for firmware update...");

    char latest_version[32] = {0};
    bool version_ok = false;
    for (int attempt = 0; attempt < OTA_CHECK_MAX_RETRY; attempt++) {
        if (ota_check_latest_version(latest_version, sizeof(latest_version))) {
            version_ok = true;
            break;
        }
        if (attempt + 1 < OTA_CHECK_MAX_RETRY) {
            ESP_LOGW(TAG, "Version check failed (%d/%d), %dms 后重试...",
                     attempt + 1, OTA_CHECK_MAX_RETRY, OTA_CHECK_RETRY_MS);
            vTaskDelay(pdMS_TO_TICKS(OTA_CHECK_RETRY_MS));
        }
    }

    if (version_ok) {
        if (strcmp(latest_version, running_info.version) != 0) {
            ESP_LOGI(TAG, "New firmware available: %s (current: %s), starting OTA download...",
                     latest_version, running_info.version);

            bool ok = false;
            for (int dl_attempt = 0; dl_attempt < OTA_DOWNLOAD_MAX_RETRY; dl_attempt++) {
                if (dl_attempt > 0) {
                    ESP_LOGW(TAG, "OTA 下载重试 (%d/%d), 等待 %dms 让堆内存整理...",
                             dl_attempt + 1, OTA_DOWNLOAD_MAX_RETRY, OTA_DOWNLOAD_RETRY_MS);
                    vTaskDelay(pdMS_TO_TICKS(OTA_DOWNLOAD_RETRY_MS));
                    ESP_LOGI(TAG, "重试前内存: 内部空闲=%u B, 最大连续块=%u B",
                             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
                }
                ok = ota_download_and_update(running);
                if (ok) break;  // 成功会 reboot，不会到这里
            }
            if (!ok) {
                ESP_LOGW(TAG, "OTA download failed after %d attempts", OTA_DOWNLOAD_MAX_RETRY);
            }
            // 若 ota_download_and_update 成功会自动 reboot，不会到这里
        } else {
            ESP_LOGI(TAG, "Firmware is up to date (version: %s)", running_info.version);
        }
    } else {
        ESP_LOGW(TAG, "Failed to check latest firmware version");
        if (ota_pending) {
            ESP_LOGE(TAG, "OTA 版本检查失败, 回滚到上一个版本...");
            esp_ota_mark_app_invalid_rollback_and_reboot();
        }
    }

    // OTA 检查任务执行成功，确认新固件有效
    if (ota_pending) {
        ESP_LOGI(TAG, "OTA 诊断全部通过, 确认新固件有效");
        // 摄像头已在 ota_task 入口处关闭，可安全写入 otadata
        esp_ota_mark_app_valid_cancel_rollback();
    }

exit:
    /* 恢复其他 TLS 任务 */
    ota_in_progress = false;
    remote_log_resume();

    ESP_LOGI(TAG, "OTA task finished, deleting self");
    if (ota_sem) {
        xSemaphoreGive(ota_sem);  // 通知主任务 OTA 流程已结束
    }
    vTaskDelete(NULL);
}