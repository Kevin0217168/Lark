# Lark 嵌入式固件

目前状态： ESP32-C3固件只预留了接口，硬件和具体功能还未实现， 接口内容可能随时发生变化， 请多加留意和沟通

物联网环境监测与摄像设备嵌入式系统，基于 **ESP-IDF v5.x + FreeRTOS**，包含两个子项目：

| 子项目 | 芯片 | 定位 | 固件版本 |
|--------|------|------|----------|
| [ESP32-camera/](ESP32-camera/) | ESP32 | 摄像设备：推流 / 拍照 / 待机切换 | `0.7.0` |
| [ESP32-C3/](ESP32-C3/) | ESP32-C3 | 传感器集线器：多传感器数据采集上报 | — |

---

## 项目结构

```
embedded/
├── README.md                      # 本文件
├── ESP32-camera/                  # 摄像设备固件
│   ├── CMakeLists.txt             # 项目: lark, 含固件上传自定义命令
│   ├── main/                      # 业务主逻辑
│   │   ├── main.c / main.h        # 启动、NVS、OTA 验证、外设初始化
│   │   ├── tasks.c / tasks.h      # 任务调度（诊断/推流/上报/监控）
│   │   ├── ws_process.c           # WebSocket JSON 命令分发
│   │   ├── i2c_recovery.c/.h      # I2C 总线卡死自动恢复
│   │   └── idf_component.yml      # 组件依赖声明
│   ├── components/
│   │   ├── Camera/                # OV2640 摄像头驱动
│   │   ├── Wifista/               # WiFi / WebSocket / HTTP / OTA / SNTP
│   │   ├── sht4x/                 # SHT4x 温湿度传感器
│   │   ├── device_secret/         # 设备凭证（共享组件）
│   │   └── remote_log/            # 远程日志（vprintf 钩子 + HTTP POST）
│   ├── docs/                      # 文档（见下方链接）
│   └── scripts/                   # 固件上传脚本
│
└── ESP32-C3/                      # 传感器集线器固件
    ├── CMakeLists.txt             # 项目: ina231_example
    ├── main/                      # 业务主逻辑
    │   ├── main.c / main.cpp      # 多传感器初始化与调度
    │   ├── tasks.c / tasks.h      # 任务调度
    │   ├── ws_process.c           # WebSocket JSON 命令分发
    │   └── idf_component.yml      # 组件依赖声明
    ├── components/
    │   ├── Wifista/               # WiFi / WebSocket（共享组件）
    │   ├── remote_log/            # 远程日志（共享组件）
    │   ├── ina231/                # INA231 电压/电流传感器
    │   ├── pms9103m/              # PMS9103M 颗粒物传感器
    │   ├── sgp30/                 # SGP30 空气质量传感器 (eCO₂/TVOC)
    │   ├── sound_meter/           # 噪声传感器
    │   ├── uv_meter/              # 紫外线传感器
    │   ├── veml7700/              # VEML7700 环境光传感器
    │   └── esp_camera/            # 摄像头驱动（预留）
    └── docs/                      # 文档（见下方链接）
```

---

## 文档索引

### 接口协议

| 文档 | 说明 |
|------|------|
| [ESP32-CAM WebSocket 控制接口文档](ESP32-camera/docs/WS_CORESPONSE.md) | 摄像设备 WS 协议：`status` / `camera` / `device` 模块的查询与设定 |
| [ESP32-C3 传感器集线器 WebSocket 控制接口文档](ESP32-C3/docs/WS_CORESPONSE.md) | 传感器集线器 WS 协议：`device` / `light` / `sensor` 模块的查询与设定 |

### 运维参考

| 文档 | 说明 |
|------|------|
| [Lark 嵌入式固件 — 日志参考手册](ESP32-camera/docs/LOG_REFERENCE.md) | 全模块 TAG 索引、日志级别说明、ERROR 日志告警参考（2026-03-28 生成） |

### 学习笔记

| 文档 | 说明 |
|------|------|
| [嵌入式 WebSocket 命令处理的扁平化设计](ESP32-camera/docs/ws_process_design.md) | 映射表 + 分发表替代嵌套 if-else 的设计实战 |
| [ESP32 日志远程传输系统：从设计到实现](ESP32-camera/docs/remote-log-design.md) | vprintf 钩子 + 环形缓冲区 + HTTP POST 批量上传的完整设计演进 |

---

## 共享能力

两个子项目共用以下核心组件：

| 组件 | 功能 |
|------|------|
| `Wifista` | WiFi STA 连接、WebSocket 客户端、HTTPS 请求、OTA 固件升级、SNTP 时间同步 |
| `remote_log` | 零侵入远程日志：vprintf 钩子 → 16KB 环形缓冲区 → HTTP POST 批量上传 |
| `device_secret` | 设备唯一标识与凭证管理 |

---

## 构建

确保已安装 [ESP-IDF v5.x](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/) 并激活环境。

```bash
# 构建摄像设备
cd ESP32-camera
idf.py build

# 构建传感器集线器
cd ESP32-C3
idf.py build
```

摄像设备额外支持一键上传：

```bash
cd ESP32-camera
idf.py upload -- -u <用户名> -p <密码> [-s <服务器地址>]
```