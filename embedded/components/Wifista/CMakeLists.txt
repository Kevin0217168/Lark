idf_component_register(SRCS "WifiSecurity.c" "Wifista.c" "Websocket.c"
                      INCLUDE_DIRS "include/"
                      PRIV_REQUIRES "esp_wifi nvs_flash esp_http_client mbedtls esp_websocket_client json esp_timer")

add_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-pointer-sign)