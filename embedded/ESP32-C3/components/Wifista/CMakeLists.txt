idf_component_register(SRCS "OTA.c" "WifiSecurity.c" "Wifista.c" "Websocket.c"
                      INCLUDE_DIRS "include/"
                      REQUIRES "esp_wifi json"
                      PRIV_REQUIRES "nvs_flash esp_http_client mbedtls esp_websocket_client esp_timer app_update esp_driver_gpio esp_netif esp_camera")

add_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-pointer-sign)