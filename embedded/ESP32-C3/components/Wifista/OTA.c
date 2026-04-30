#include "Wifista.h"
#include "esp_camera.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include <errno.h>

static const char *TAG = "ota";

// ===================== OTA 服务器配置 =====================
// 生产环境
#define OTA_API_LATEST   "https://lark.mintlab.top/api/firmwares/latest"
#define OTA_API_DOWNLOAD "https://lark.mintlab.top/api/firmwares/download"
// 本地调试（取消注释并注释上面两行即可切换）
// #define OTA_API_LATEST   "http://192.168.1.200:8080/api/firmwares/latest"
// #define OTA_API_DOWNLOAD "http://192.168.1.200:8080/api/firmwares/download"

#define OTA_RECV_TIMEOUT       30000

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
    if (update_partition == NULL) {
        ESP_LOGE(TAG, "找不到可用的 OTA 更新分区");
        http_cleanup(client);
        return false;
    }
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

                    // OTA flash 擦写与摄像头 PSRAM DMA 共享 SPI 总线，同时运行会触发 WDT
                    ESP_LOGI(TAG, "OTA 更新开始，关闭摄像头以避免 SPI 总线冲突...");
                    esp_camera_deinit();

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
 * @brief 检查当前分区表是否支持 OTA（存在 otadata 和至少两个 APP 分区）
 * @return true  OTA 可用
 * @return false 不支持 OTA（单分区模式）
 */
static bool ota_is_supported(void)
{
    const esp_partition_t *otadata = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
    if (otadata == NULL) {
        ESP_LOGW(TAG, "分区表无 otadata，OTA 不可用（当前为单应用分区表）");
        return false;
    }
    /* 确保至少有两个 OTA 应用槽 */
    const esp_partition_t *ota0 = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    const esp_partition_t *ota1 = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
    if (ota0 == NULL || ota1 == NULL) {
        ESP_LOGW(TAG, "OTA 应用槽不足，OTA 不可用");
        return false;
    }
    return true;
}

void ota_task(void *pvParameter)
{
    ESP_LOGI(TAG, "OTA task started");

    /* ── 前置检查：分区表是否支持 OTA ── */
    if (!ota_is_supported()) {
        ESP_LOGE(TAG, "当前分区表不支持 OTA，任务退出。"
                 "请切换到双 OTA 分区表（如 partitions_two_ota.csv）");
        goto exit;
    }

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured == NULL || running == NULL) {
        ESP_LOGE(TAG, "获取分区信息失败 (configured=%p, running=%p)，任务退出",
                 (void *)configured, (void *)running);
        goto exit;
    }

    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08"PRIx32
                 ", but running from offset 0x%08"PRIx32,
                 configured->address, running->address);
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08"PRIx32")",
             running->type, running->subtype, running->address);

    esp_app_desc_t running_info;
    if (esp_ota_get_partition_description(running, &running_info) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get running firmware description");
        goto exit;
    }
    ESP_LOGI(TAG, "Running firmware version: %s", running_info.version);

    ESP_LOGI(TAG, "Checking for firmware update...");

    char latest_version[32] = {0};
    if (ota_check_latest_version(latest_version, sizeof(latest_version))) {
        if (strcmp(latest_version, running_info.version) != 0) {
            ESP_LOGI(TAG, "New firmware available: %s (current: %s), starting OTA download...",
                     latest_version, running_info.version);
            bool ok = ota_download_and_update(running);
            if (!ok) {
                ESP_LOGW(TAG, "OTA download failed");
            }
            // 若 ota_download_and_update 成功会自动 reboot，不会到这里
        } else {
            ESP_LOGI(TAG, "Firmware is up to date (version: %s)", running_info.version);
        }
    } else {
        ESP_LOGW(TAG, "Failed to check latest firmware version");
    }

exit:
    ESP_LOGI(TAG, "OTA task finished, deleting self");
    vTaskDelete(NULL);
}