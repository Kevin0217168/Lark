#include "Wifista.h"
#include "esp_wifi.h"
#include "esp_log.h"

#define DEFAULT_SSID "关闭"
#define DEFAULT_PWD "kaiwen0818"

const char* TAG = "Wifista";
 
void Wifista_event_handler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    if (event_base == WIFI_EVENT){
        if (event_id == WIFI_EVENT_STA_START){
            esp_wifi_connect();
        }else if (event_id == WIFI_EVENT_STA_CONNECTED){
            ESP_LOGI(TAG, "WIFI connected!");
        }else if (event_id == WIFI_EVENT_STA_DISCONNECTED){
            ESP_LOGE(TAG, "WIFI disconnected!");
            esp_wifi_stop();
        }

    }else if (event_base == IP_EVENT)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "get IP address: %d.%d.%d.%d",
            esp_ip4_addr1_16(&event->ip_info.ip),
            esp_ip4_addr2_16(&event->ip_info.ip),
            esp_ip4_addr3_16(&event->ip_info.ip),
            esp_ip4_addr4_16(&event->ip_info.ip));
    }
    
}

void WifistaInit(){
    esp_netif_init();
    esp_event_loop_create_default();

    // 
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, Wifista_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, Wifista_event_handler, NULL);

    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t wifista_config = {
        .sta = {
            .ssid = DEFAULT_SSID,
            .password = DEFAULT_PWD
        }
    };
    // wifi_config_t wifista_config = { 0 };
    // strncpy((char *)wifista_config.sta.ssid, DEFAULT_SSID, sizeof(wifista_config.sta.ssid));
    // strncpy((char *)wifista_config.sta.password, DEFAULT_PWD, sizeof(wifista_config.sta.password));

    esp_wifi_set_config(WIFI_IF_STA, &wifista_config);

    esp_wifi_start();
}

