#include "Wifista.h"

const char *WS_TAG = "WifiSecurity";

esp_http_client_handle_t WifiSecurityClient;
esp_http_client_config_t config;

void obtain_time(void)
{
    // 设置时区（中国标准时间 UTC+8）
    setenv("TZ", "CST-8", 1);
    tzset();

    // 初始化 SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    // 等待系统时间有效（超过 2020-01-01 00:00:00 的时间戳）
    int retry = 0;
    const int max_retry = 20;   // 最多等待 40 秒
    bool synced = false;
    while (retry < max_retry) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time_t now = time(NULL);
        // 如果时间大于 2020-01-01 的 Unix 时间戳，视为有效
        if (now > 1577836800) {
            synced = true;
            break;
        }
        retry++;
        ESP_LOGD("obtain_time", "等待 SNTP 同步... 重试次数: %d", retry);
    }

    if (synced) {
        ESP_LOGI("obtain_time", "SNTP 同步成功");
    } else {
        ESP_LOGW("obtain_time", "SNTP 同步超时，可能无法获取正确时间");
    }

    // 停止 SNTP 服务（可选，如果后续不再需要可停止以节省资源）
    esp_sntp_stop();

    // 获取最终时间并打印
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    if (now > 0) {
        ESP_LOGI("obtain_time", "当前时间: %04d-%02d-%02d %02d:%02d:%02d",
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
        ESP_LOGE("obtain_time", "获取系统时间失败，时间为 1970-01-01");
    }
}


static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    RequestContext_t *ctx = (RequestContext_t *)evt->user_data;

    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(WS_TAG, "Header: %s: %s", evt->header_key, evt->header_value);
        if (strcasecmp(evt->header_key, "Content-Type") == 0)
        {
            ctx->is_json = (strstr(evt->header_value, "application/json") != NULL);
        }
        break;

    case HTTP_EVENT_ON_DATA:
        if (evt->data_len > 0)
        {
            // 确保缓冲区足够大
            size_t new_size = ctx->buffer_size + evt->data_len;
            ESP_LOGI(WS_TAG, "数据缓冲区总大小: %d bytes, 已使用: %d bytes, 本次接收数据长度: %d bytes",
                     ctx->buffer_capacity, ctx->buffer_size, evt->data_len);
            if (new_size > ctx->buffer_capacity)
            {
                // 多预留空间
                size_t new_cap = new_size + (new_size - ctx->buffer_capacity) + 128;
                ESP_LOGW(WS_TAG, "数据缓冲区空间不足, 重新分配大小为: %d bytes", new_cap);

                char *new_buf = realloc(ctx->buffer, new_cap);
                if (new_buf)
                {
                    ctx->buffer = new_buf;
                    ctx->buffer_capacity = new_cap;
                }
                else
                {
                    ESP_LOGE(WS_TAG, "分配失败, 内存不足, 无法接收数据");
                    return ESP_FAIL;
                }
            }
            memcpy(ctx->buffer + ctx->buffer_size, evt->data, evt->data_len);
            ctx->buffer_size = new_size;

            ESP_LOGI(WS_TAG, "分配成功, 新缓冲区总大小: %d bytes, 已使用: %d bytes", ctx->buffer_capacity, ctx->buffer_size);
        }
        ESP_LOGI(WS_TAG, "Data (len=%d): %.*s\n", evt->data_len, evt->data_len, (char *)evt->data);
        break;

    case HTTP_EVENT_ON_FINISH:
        if (ctx->is_json)
        {
            // ctx->json = cJSON_Parse(ctx->buffer); // 假设数据完整且以 \0 结尾
            ctx->json = cJSON_ParseWithLength(ctx->buffer, ctx->buffer_size);
            if (ctx->json != NULL)
            {
                char *string = cJSON_Print(ctx->json);
                ESP_LOGI(WS_TAG, "收到 JSON 数据, 解析成功: \n%s\n", string);
                cJSON_free(string);
                // cJSON_Delete(json); // 由请求函数释放
            }
            else
            {
                ESP_LOGI(WS_TAG, "收到 JSON 数据, 解析失败");
            }
        }
    default:
        break;
    }
    return ESP_OK;
}

void WifiSecurityClientInit()
{

    config.event_handler = http_event_handler;
    config.url = "https://www.mintlab.top"; // 这里必须给初值, 否则初始化失败
    config.buffer_size = 4096;
    config.timeout_ms = 10000;
    config.crt_bundle_attach = esp_crt_bundle_attach; // 使用证书包
                                                      // 或者使用手动根证书: config.cert_pem = root_cert_pem
    config.skip_cert_common_name_check = false;

    WifiSecurityClient = esp_http_client_init(&config);
}

esp_err_t WifiSecurityRequest(const char *host, const char *path, uint16_t port, WifiSecurityMethod_t method,
                              char *post_data, void(ResponseUserHandler)(RequestContext_t *))
{
    // 检查客户端句柄非空
    if (WifiSecurityClient == NULL)
        ESP_LOGE(WS_TAG, "WifiSecurityClient == NULL");

    // 准备请求url
    char url[256];
    snprintf(url, sizeof(url), "%s:%d%s", host, port, path);
    esp_http_client_set_url(WifiSecurityClient, url);

    // 准备上下文
    RequestContext_t req_ctx = {
        .is_json = false,
        .json = NULL,
        .client = &WifiSecurityClient,
        .buffer = NULL,
        .buffer_capacity = 0,
        .buffer_size = 0,
    };
    esp_http_client_set_user_data(WifiSecurityClient, &req_ctx);

    // 区分请求方法
    char *method_name = "";
    switch (method)
    {
    case WS_CLINENT_METHOD_GET:
        method_name = "GET";
        esp_http_client_set_method(WifiSecurityClient, HTTP_METHOD_GET);
        break;
    case WS_CLINENT_METHOD_POST:
        method_name = "POST";
        esp_http_client_set_method(WifiSecurityClient, HTTP_METHOD_POST);
        esp_http_client_set_header(WifiSecurityClient, "Content-Type", "application/json");
        esp_http_client_set_post_field(WifiSecurityClient, post_data, strlen(post_data));
        ESP_LOGI(WS_TAG, "准备好Body(len=%d): %s", strlen(post_data), post_data);
        break;
    case WS_CLINENT_METHOD_PUT:
        method_name = "PUT";
        break;
    case WS_CLINENT_METHOD_DELETE:
        method_name = "DELETE";
        break;
    default:
        break;
    }

    // 发起请求
    ESP_LOGI(WS_TAG, "准备好请求: %s | %s ", method_name, url);
    esp_err_t err = esp_http_client_perform(WifiSecurityClient);

    if (err == ESP_OK)
    {
        ESP_LOGI(WS_TAG, "%s | %s 请求成功，状态码: %d\n", method_name, url, esp_http_client_get_status_code(WifiSecurityClient));
    }
    else
    {
        ESP_LOGI(WS_TAG, "%s | %s 请求失败: %s\n", method_name, url, esp_err_to_name(err));
    }

    // 调用用户回调函数处理返回结果
    if (ResponseUserHandler != NULL)
    {
        ResponseUserHandler(&req_ctx);
    }
    else
    {
        // 释放 JSON 对象（如果有）
        if (req_ctx.is_json && req_ctx.json != NULL)
        {
            cJSON_Delete(req_ctx.json);
        }
    }
    // 释放动态分配的接收缓冲区
    if (req_ctx.buffer != NULL)
    {
        free(req_ctx.buffer);
    }

    // 关键：关闭当前连接，下次请求会重新建立
    esp_http_client_close(WifiSecurityClient);

    // 清除 user_data，避免残留指针
    esp_http_client_set_user_data(WifiSecurityClient, NULL);

    // esp_http_client_cleanup(WifiSecurityClient);
    return err;
}