# Lark 嵌入式固件 — 日志参考手册

> 自动生成于 2026-03-28，基于当前代码库所有 `ESP_LOG*` 调用。
> 前端可根据 **TAG** 和 **级别** 进行筛选和告警。

---

## 一、TAG 总览

| TAG | 功能模块 | 源文件 | 说明 |
|-----|----------|--------|------|
| `main` | 系统启动 | `main/main.c` | 启动流程、NVS、OTA 验证、传感器/摄像头初始化 |
| `task` | 任务调度 | `main/tasks.c` | 诊断、推流、传感器上报、健康监控 |
| `sys_mon` | 系统监控 | `main/tasks.c` (内联) | 内存/任务状态周期打印（每 60s） |
| `remote_log` | 远程日志 | `main/remote_log.c` | vprintf 钩子 + HTTP POST 批量上传 |
| `ws_cmd` | WS 指令处理 | `main/ws_process.c` | WebSocket JSON 指令解析与执行 |
| `i2c_recovery` | I2C 恢复 | `main/i2c_recovery.c` | I2C 总线卡死恢复（9 脉冲 + STOP） |
| `camera` | 摄像头 | `components/Camera/Camera.c` | OV2640 初始化、帧捕获 |
| `websocket` | WebSocket | `components/Wifista/Websocket.c` | WS 连接/断开/收发 |
| `http_client` | HTTP 客户端 | `components/Wifista/WifiSecurity.c` | HTTPS 请求（传感器上报等） |
| `sntp` | 时间同步 | `components/Wifista/WifiSecurity.c` (内联) | NTP 时间同步 |
| `ota` | OTA 升级 | `components/Wifista/OTA.c` | 固件版本检查、下载、Flash 写入 |
| `wifi` | WiFi 连接 | `components/Wifista/Wifista.c` | STA 模式连接/断开/IP 获取 |
| `sht4x_i2c` | 温湿度 I2C | `components/sht4x/sensirion_hw_i2c_implementation.c` | SHT4x I2C 底层驱动 |

---

## 二、日志级别说明

| 级别 | ESP 宏 | 默认可见 | 含义 |
|------|--------|----------|------|
| **E** (ERROR) | `ESP_LOGE` | ✅ | 需要立即关注的故障 |
| **W** (WARN) | `ESP_LOGW` | ✅ | 异常但可恢复的状态 |
| **I** (INFO) | `ESP_LOGI` | ✅ | 正常运行关键节点 |
| **D** (DEBUG) | `ESP_LOGD` | ❌ | 调试详情，需手动开启 |

开启 DEBUG 日志方法：
```c
// 在 app_main() 中按模块开启
esp_log_level_set("websocket", ESP_LOG_DEBUG);
esp_log_level_set("http_client", ESP_LOG_DEBUG);
```

---

## 三、ERROR 日志详解（前端告警参考）

### TAG: `main` — 系统启动

| 日志内容 | 触发条件 | 严重程度 | 后续动作 |
|----------|----------|----------|----------|
| `WiFi 连接超时 (%d ms), OTA 固件回滚...` | OTA 待验证时 WiFi 30s 未连接 | 🔴 严重 | 自动回滚重启 |
| `硬件诊断失败! 回滚到上一个版本...` | OTA 后 SHT4x 或摄像头不可用 | 🔴 严重 | 自动回滚重启 |
| `WebSocket 连接超时 (%d ms), OTA 固件回滚...` | OTA 待验证时 WS 30s 未连接 | 🔴 严重 | 自动回滚重启 |
| `SHT sensor 初始化失败，已达到最大重试次数 %d` | SHT4x 探测 10 次均失败 | 🟠 高 | 传感器数据不可用 |

### TAG: `task` — 任务调度

| 日志内容 | 触发条件 | 严重程度 | 后续动作 |
|----------|----------|----------|----------|
| `诊断失败: SHT4x 传感器未响应` | OTA 诊断阶段传感器无应答 | 🔴 严重 | 触发固件回滚 |
| `诊断失败: 无法获取摄像头传感器指针` | OTA 诊断阶段摄像头初始化失败 | 🔴 严重 | 触发固件回滚 |
| `传感器数据上报失败, err=%d` | HTTPS POST 传感器数据失败 | 🟡 中 | 下次周期重试 |
| `SHT4x 读取失败` | I2C 读温湿度失败 | 🟡 中 | 下次周期重试 |
| `[健康监控] WiFi 持续断开超过 %d 秒，重启设备...` | WiFi 连续断开 5 分钟 | 🔴 严重 | 自动重启 |
| `[健康监控] WebSocket 持续断开超过 %d 秒，重启设备...` | WS 连续断开 5 分钟 | 🔴 严重 | 自动重启 |

### TAG: `remote_log` — 远程日志

| 日志内容 | 触发条件 | 严重程度 | 后续动作 |
|----------|----------|----------|----------|
| `无法创建环形缓冲区 (%d bytes)` | 内存不足，16KB 分配失败 | 🔴 严重 | 远程日志不可用 |
| `请先调用 remote_log_early_init()` | 调用顺序错误 | 🟠 高 | 编程错误 |
| `无法创建 flush 任务` | FreeRTOS 任务创建失败 | 🔴 严重 | 日志不上传 |

### TAG: `ws_cmd` — WS 指令处理

| 日志内容 | 触发条件 | 严重程度 | 后续动作 |
|----------|----------|----------|----------|
| `相机未初始化` | 收到相机指令但硬件未就绪 | 🟡 中 | 返回错误响应 |
| `JSON 字段缺失或类型错误` | WS 消息格式不符合协议 | 🟡 中 | 返回错误响应 |

### TAG: `camera` — 摄像头

| 日志内容 | 触发条件 | 严重程度 | 后续动作 |
|----------|----------|----------|----------|
| `摄像头初始化失败: 0x%x` | esp_camera_init 返回错误 | 🔴 严重 | 推流不可用 |
| `获取传感器失败` | SCCB 通信异常 | 🔴 严重 | 推流不可用 |
| `获取帧失败` | DMA/PSRAM 问题或总线冲突 | 🟡 中 | 跳过本帧 |

### TAG: `websocket` — WebSocket

| 日志内容 | 触发条件 | 严重程度 | 后续动作 |
|----------|----------|----------|----------|
| `WS 错误` | WebSocket 协议层错误 | 🟠 高 | 自动重连 |
| `WS 客户端初始化失败` | 内存不足或配置错误 | 🔴 严重 | 无法连接服务器 |
| `bin 发送失败: 缓冲区满` | 推流速率超过网络带宽 | 🟡 中 | 丢帧 |
| `text 发送失败` | WS 文本帧发送异常 | 🟡 中 | 指令响应丢失 |

### TAG: `http_client` — HTTP 客户端

| 日志内容 | 触发条件 | 严重程度 | 后续动作 |
|----------|----------|----------|----------|
| `缓冲区扩容失败, 内存不足` | HTTP 响应过大，realloc 失败 | 🟠 高 | 本次请求失败 |
| `HTTP 客户端未初始化` | 未调用 WifiSecurityClientInit | 🔴 严重 | 编程错误 |
| `%s %s 失败: %s` | HTTP 请求执行失败 | 🟡 中 | 下次重试 |

### TAG: `ota` — OTA 升级

| 日志内容 | 触发条件 | 严重程度 | 后续动作 |
|----------|----------|----------|----------|
| `Failed to init HTTP client for version check` | HTTP 客户端初始化失败 | 🟠 高 | OTA 检查中止 |
| `Failed to open HTTP connection: %s` | 无法连接 OTA 服务器 | 🟠 高 | OTA 检查中止 |
| `Firmware download returned HTTP %d` | 服务器返回非 200 状态码 | 🟠 高 | 下载中止 |
| `Error: data read error` | 下载过程中网络中断 | 🟠 高 | 下载中止 |
| `esp_ota_begin failed (%s)` | OTA 分区写入初始化失败 | 🔴 严重 | 致命错误 |
| `Image validation failed, firmware is corrupted` | 固件校验失败(CRC/SHA) | 🔴 严重 | 致命错误 |
| `esp_ota_set_boot_partition failed (%s)` | 启动分区设置失败 | 🔴 严重 | 致命错误 |
| `Failed to get running firmware description` | 当前固件信息读取失败 | 🟠 高 | OTA 中止 |
| `Incomplete firmware download` | 下载数据不完整 | 🟠 高 | 下载中止 |

### TAG: `sht4x_i2c` — I2C 底层

| 日志内容 | 触发条件 | 严重程度 | 后续动作 |
|----------|----------|----------|----------|
| `Too many I2C devices` | 超过 8 个 I2C 设备 | 🔴 严重 | 编程错误 |
| `I2C bus init failed: %s` | I2C 主机初始化失败 | 🔴 严重 | 传感器不可用 |
| `I2C not initialized` | 未调用 init 就读写 | 🟠 高 | 编程错误 |
| `I2C read/write 0x%02x failed: %s` | I2C 通信超时或 NACK | 🟡 中 | 上层重试 |

---

## 四、WARN 日志详解（前端监控参考）

| TAG | 日志内容 | 含义 |
|-----|----------|------|
| `main` | `检测到 OTA 待验证状态，将进行完整诊断` | OTA 刚更新完，正在诊断 |
| `main` | `无法获取 OTA 状态 (rollback 可能未启用)` | 分区表未配置 rollback |
| `main` | `SHT sensor probing failed, retry %d/%d` | SHT4x 探测失败，重试中 |
| `task` | `时间未同步, 由后端补充时间戳` | SNTP 尚未完成同步 |
| `task` | `[健康监控] WiFi 已断开 %d/%d 次` | WiFi 连续断开计数 |
| `task` | `[健康监控] WebSocket 已断开 %d/%d 次` | WS 连续断开计数 |
| `ws_cmd` | `JSON 解析失败` | 收到非法 JSON 数据 |
| `ws_cmd` | `收到重启指令，设备即将重启...` | 远程重启触发 |
| `ws_cmd` | `未处理的请求: item=%s, code=%d` | 不支持的指令 |
| `camera` | `PSRAM 不可用, 缓冲区将使用 DRAM` | 退化为低内存模式 |
| `websocket` | `WS 已断开` | WS 连接断开（会自动重连） |
| `websocket` | `bin/text 发送失败: 未连接` | 发送时 WS 未连接 |
| `http_client` | `缓冲区扩容: %d bytes` | HTTP 响应缓冲区动态扩展 |
| `http_client` | `JSON 解析失败` | HTTP 响应 JSON 格式异常 |
| `sntp` | `SNTP 首次同步超时` | NTP 服务器不可达，后台重试 |
| `ota` | `Version check returned HTTP %d` | 设备未注册或无可用固件 |
| `ota` | `New version matches previously failed version` | 跳过已知失败的固件 |
| `ota` | `OTA download failed` | 下载失败 |

---

## 五、DEBUG 日志参考（默认不输出，需手动开启）

| TAG | 日志内容 | 用途 | 开启方法 |
|-----|----------|------|----------|
| `task` | `时间: %04d-%02d-%02d %02d:%02d:%02d` | 查看传感器上报时间戳 | `esp_log_level_set("task", ESP_LOG_DEBUG)` |
| `ws_cmd` | `收到: %s` | 查看完整 WS JSON 指令 | `esp_log_level_set("ws_cmd", ESP_LOG_DEBUG)` |
| `websocket` | `收到数据: opcode=%d, len=%d` | WS 帧级别调试 | `esp_log_level_set("websocket", ESP_LOG_DEBUG)` |
| `websocket` | `payload: %.*s` | 查看 WS 原始文本 | 同上 |
| `websocket` | `发送 bin/text: %d bytes` | WS 发送追踪 | 同上 |
| `http_client` | `Header: %s: %s` | 查看 HTTP 响应头 | `esp_log_level_set("http_client", ESP_LOG_DEBUG)` |
| `http_client` | `缓冲区: 容量=%d, 已用=%d, ...` | HTTP 缓冲区使用详情 | 同上 |
| `http_client` | `Data (len=%d): %.*s` | HTTP 响应原始数据 | 同上 |
| `http_client` | `POST body (len=%d): %s` | HTTP 请求体内容 | 同上 |
| `sntp` | `等待 SNTP 同步... 重试次数: %d/%d` | SNTP 同步等待过程 | `esp_log_level_set("sntp", ESP_LOG_DEBUG)` |
| `ota` | `Written image length %d` | OTA 写入进度 | `esp_log_level_set("ota", ESP_LOG_DEBUG)` |

---

## 六、前端筛选建议

### 按场景筛选

| 场景 | 推荐筛选 TAG | 推荐级别 |
|------|-------------|----------|
| 设备是否在线 | `wifi`, `websocket` | WARN + ERROR |
| 传感器是否正常 | `task`, `sht4x_i2c` | ERROR |
| 摄像头是否正常 | `camera`, `task` | ERROR |
| OTA 是否成功 | `ota`, `main` | 全部 |
| 系统健康状态 | `task`, `sys_mon` | INFO |
| 网络请求调试 | `http_client` | DEBUG |
| WS 通信调试 | `websocket`, `ws_cmd` | DEBUG |
| 内存泄漏排查 | `sys_mon` | INFO |

### 告警规则建议

| 规则 | 条件 | 优先级 |
|------|------|--------|
| 设备重启 | TAG=`task` & 包含 `重启设备` | P0 |
| OTA 回滚 | TAG=`main` & 包含 `固件回滚` | P0 |
| 摄像头故障 | TAG=`camera` & 级别=ERROR | P1 |
| 传感器故障 | TAG=`sht4x_i2c` & 包含 `failed` | P1 |
| WiFi 断连 | TAG=`wifi` & 包含 `已断开` | P2 |
| WS 断连 | TAG=`websocket` & 包含 `已断开` | P2 |
| OTA 失败 | TAG=`ota` & 级别=ERROR | P1 |

---

## 七、统计

| TAG | ERROR | WARN | INFO | DEBUG | 合计 |
|-----|-------|------|------|-------|------|
| `main` | 4 | 3 | 8 | 0 | **15** |
| `task` | 6 | 4 | 9 | 1 | **20** |
| `sys_mon` | 0 | 0 | 3 | 0 | **3** |
| `remote_log` | 3 | 0 | 5 | 0 | **8** |
| `ws_cmd` | 2 | 3 | 3 | 1 | **9** |
| `i2c_recovery` | 0 | 0 | 3 | 0 | **3** |
| `camera` | 3 | 1 | 2 | 0 | **6** |
| `websocket` | 4 | 3 | 1 | 4 | **12** |
| `http_client` | 3 | 2 | 3 | 5 | **13** |
| `sntp` | 0 | 1 | 2 | 1 | **4** |
| `ota` | 18 | 5 | 16 | 1 | **40** |
| `wifi` | 0 | 1 | 2 | 0 | **3** |
| `sht4x_i2c` | 10 | 0 | 2 | 0 | **12** |
| **总计** | **53** | **23** | **59** | **13** | **148** |
