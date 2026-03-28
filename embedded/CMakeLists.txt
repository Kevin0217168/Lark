# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(PROJECT_VER "0.6.11")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(lark)

# ── 自定义命令: 上传固件到后端服务器 ──
# 用法: idf.py upload -- -u <用户名> -p <密码> [-s <服务器地址>]
# 示例: idf.py upload -- -u root -p 123456
#       idf.py upload -- -u root -p 123456 -s https://lark.mintlab.top/api
add_custom_target(upload
    COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/upload_firmware.sh
        -f ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
        \${UPLOAD_ARGS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "上传固件到 Lark 后端服务器"
    VERBATIM
)
