idf_component_register(SRCS "main.c" "ws_process.c" "remote_log.c"
                             "device_secret.c" "i2c_recovery.c" "tasks.c"
                    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-pointer-sign)
