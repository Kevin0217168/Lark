idf_component_register(
    SRCS
        "sensirion_common.c"
        "sensirion_hw_i2c_implementation.c"
        "sht4x.c"
        "sht_git_version.c"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        "driver"
)

# 可选的编译选项
target_compile_options(${COMPONENT_LIB} PRIVATE -ffast-math -O3 -Wno-error=format -Wno-pointer-sign)

# add_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)
# target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-pointer-sign)