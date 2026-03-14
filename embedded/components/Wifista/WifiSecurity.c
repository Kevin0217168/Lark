#include "Wifista.h"

const char *WS_TAG = "WifiSecurity";

esp_http_client_handle_t WifiSecurityClient;
esp_http_client_config_t config;

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

void WifiSecurityClientInit()
{

    config.event_handler = http_event_handler;
    config.url = "https://www.mintlab.top";           // 这里必须给初值, 否则初始化失败
    config.buffer_size = 4096;
    config.timeout_ms = 2000;
    config.crt_bundle_attach = esp_crt_bundle_attach; // 使用证书包
                                                      // 或者使用手动根证书: config.cert_pem = root_cert_pem
    config.skip_cert_common_name_check = false;

    WifiSecurityClient = esp_http_client_init(&config);
}

esp_err_t WifiSecurityRequest(const char *host, const char *path, uint16_t port, WifiSecurityMethod_t method, char *post_data)
{
    char url[256];

    snprintf(url, sizeof(url), "%s:%d%s", host, port, path);

    if (WifiSecurityClient == NULL)
        ESP_LOGE(WS_TAG, "WifiSecurityClient == NULL");

    esp_http_client_set_url(WifiSecurityClient, url);

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

    // esp_http_client_cleanup(WifiSecurityClient);
    return err;
}