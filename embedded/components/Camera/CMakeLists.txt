idf_component_register(SRCS "Camera.c"
                      INCLUDE_DIRS "include/"
                      PRIV_REQUIRES "esp32-camera esp_psram esp_timer")

# 添加 esp_hw_support 的 port/include 目录
target_include_directories(${COMPONENT_LIB} PRIVATE
    ${IDF_PATH}/components/esp_hw_support/port/include)

add_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-pointer-sign)