idf_component_register(SRCS "main.c" "ws_process.c"
                    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-pointer-sign)
