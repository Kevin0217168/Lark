idf_component_register(SRCS "main.c" "ws_process.c"
                             "i2c_recovery.c" "tasks.c"
                    INCLUDE_DIRS "."
                    REQUIRES remote_log device_secret Wifista Camera sht4x
                    PRIV_REQUIRES bootloader_support app_update nvs_flash)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-pointer-sign)
