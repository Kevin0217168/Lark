idf_component_register(SRCS "Wifista.c"
                      INCLUDE_DIRS "include/"
                      PRIV_REQUIRES "esp_wifi")

add_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-pointer-sign)
