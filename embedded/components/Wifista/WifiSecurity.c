#include "Wifista.h"

const char *WS_TAG = "WifiSecurity";

void obtain_time(void)
{
    // 初始化 SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    // 等待时间同步
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2026 - 1900) && ++retry < retry_count)
    {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        setenv("TZ", "CST-8", 1);
        localtime_r(&now, &timeinfo);
    }
    esp_sntp_stop();

    time(&now);
    setenv("TZ", "CST-8", 1);
    localtime_r(&now, &timeinfo);
    ESP_LOGI(WS_TAG, "当前时间: %04d-%02d-%02d %02d:%02d:%02d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI("HTTP", "Header: %s: %s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI("HTTP", "Data (len=%d): %.*s", evt->data_len, evt->data_len, (char *)evt->data);
        break;
    default:
        break;
    }
    return ESP_OK;
}

esp_err_t WifiSecurityRequest(const char *host, const char *path, uint16_t port)
{
    char url[256];

    snprintf(url, sizeof(url), "%s:%d%s", host, port, path);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .buffer_size = 4096,
        .timeout_ms = 2000,
        .crt_bundle_attach = esp_crt_bundle_attach, // 使用证书包
        // 或者使用手动根证书: .cert_pem = root_cert_pem
        .skip_cert_common_name_check = false};

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(WS_TAG, "HTTPS 请求成功，状态码 = %d", esp_http_client_get_status_code(client));
    }
    else
    {
        ESP_LOGI(WS_TAG, "HTTPS 请求失败: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}