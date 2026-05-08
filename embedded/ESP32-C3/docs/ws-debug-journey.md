# ESP32 WebSocket 连接失败排查全记录

> 从 HTTP 400 到 C++ name mangling，一次跨越配置、协议、编译、链接四层的深度调试之旅。

---

## 起点：硬件迁移

项目原先跑在 **ESP32-C3 (2MB Flash)** 上，后来决定迁移到 **ESP32-S3 (4MB Flash)**，顺手把分区表从单应用改成了双 OTA。架构变更触发了连锁反应，下面按遇到问题的顺序逐一记录。

```
idf.py set-target esp32s3
# sdkconfig 被重置，之前的所有手动配置丢失
```

---

## 问题 1：固件超过 factory 分区大小（编译失败）

### 现象

```
Error: app partition is too small for binary ina231_example.bin size 0x1073e0:
  - Part 'factory' 0/0 @ 0x10000 size 0x100000 (overflow 0x73e0)
```

### 根因

`set-target esp32s3` 把 `sdkconfig` 重置回默认：
- Flash: **2MB**
- 分区表: **`partitions_singleapp.csv`**，`factory` 只有 **1MB**

而 ESP32-S3 编译出的固件 **1.06MB**，刚好溢出。

### 修复

修改 `sdkconfig`：
```
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y          # 2MB → 4MB
CONFIG_PARTITION_TABLE_TWO_OTA_LARGE=y    # singleapp → two_ota_large
CONFIG_LWIP_SNTP_MAX_SERVERS=4            # 代码配了 4 台 NTP 服务器
CONFIG_WS_DYNAMIC_BUFFER=y                # WebSocket 传输层用动态缓冲区
```

双 OTA 大分区表每个槽 **1.66MB**，固件占 **1.06MB**，**38% 空闲**。

---

## 问题 2：SNTP 超时 → DNS 不可用 → WebSocket 连不上

### 现象

```
W (40579) sntp: SNTP 首次同步超时
E (40579) esp-tls: couldn't get hostname for :lark.mintlab.top:
                    getaddrinfo() returns 202
```

### 根因：启动时序错误

```c
// main.cpp 原始代码
while (!Wifi_isConnected) {      // ← WIFI_EVENT_STA_CONNECTED (L2 连接)
    vTaskDelay(pdMS_TO_TICKS(500));
}
ESP_LOGI(TAG, "WiFi 已连接，开始 SNTP 对时");
obtain_time();                    // ← SNTP 需要 DNS！（但 DNS 还没就绪）
```

| 事件 | 含义 | DNS 可用？ |
|------|------|-----------|
| `WIFI_EVENT_STA_CONNECTED` | L2 关联成功 | ❌ |
| `IP_EVENT_STA_GOT_IP` | DHCP 完成，拿到 DNS 服务器 | ✅ |

代码在 L2 刚连上就开始 SNTP（需要 DNS 解析 `ntp.aliyun.com`），此时 DNS 还没拿到。

### 修复

在 `Wifista.c` 中添加 `Wifi_gotIp` 标志：

```c
// IP_EVENT_STA_GOT_IP 事件中
Wifi_gotIp = true;
```

`main.cpp` 改为等待 IP：

```cpp
while (!Wifi_gotIp) {             // ← 等 DHCP 完成
    vTaskDelay(pdMS_TO_TICKS(100));
}
```

此外增加 DNS 解析预检：

```cpp
// 启动 WebSocket 前确认能解析域名
while (dns_retry < 30) {
    struct addrinfo *res = NULL;
    if (getaddrinfo("lark.mintlab.top", NULL, &hints, &res) == 0) {
        freeaddrinfo(res);
        break;  // DNS 就绪
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
}
```

---

## 问题 3：OTA 任务 panic（Load access fault）

### 现象

```
I (100672) ota: OTA task started
E (100682) esp_ota_ops: not found otadata
Guru Meditation Error: Core 0 panic'ed (Load access fault)
```

### 根因

单应用分区表没有 `otadata` 分区，`esp_ota_get_boot_partition()` 返回 `NULL`，代
码直接 `configured->address` 解引用空指针。

### 修复

`OTA.c` 中添加分区表检查：

```c
static bool ota_is_supported(void) {
    // 检查 otadata 是否存在
    const esp_partition_t *otadata = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
    return otadata != NULL;
}

void ota_task(void *pvParameter) {
    if (!ota_is_supported()) {
        ESP_LOGE(TAG, "当前分区表不支持 OTA，任务退出");
        goto exit;
    }
    // ...
}
```

同时对 `configured`、`running`、`update_partition` 增加了 NULL 检查。

---

## 问题 4：WebSocket 初始化失败（Memory exhausted）

### 现象

```
W (49604) websocket_client: network_timeout_ms is not set...
E (49604) websocket_client: esp_websocket_client_init(865): Memory exhausted
```

### 根因

`Websocket.c` 中 `buffer_size = 64 * 1024` — 64KB 的静态接收缓冲区。在 ESP32
上分配 64KB 连续内存 + TLS 协商需要的 ~50KB，直接超出可用堆。

### 修复

```c
// 64KB → 4KB（传感器数据量完全够用）
websocket_cfg.buffer_size = 4096;
websocket_cfg.network_timeout_ms = 10000;  // 消除 warning
```

---

## 问题 5：WebSocket 握手 HTTP 400 — 两种根因

### 现象

```
I (5145) websocket: 启动 WebSocket 连接:
    wss://lark.mintlab.top:443/api/stream/device/ws?secret=�f
                                                            ^^^  乱码！
E (15775) transport_ws: Sec-WebSocket-Accept not found
E (15775) websocket_client: esp_transport_connect() failed
    esp_ws_handshake_status_code=400
```

浏览器测试**同一 URL 连接成功**，说明服务器没问题。

---

### 5a：服务器响应头超过 WS 传输层缓冲区

```
E (17115) transport_ws: Header size exceeded buffer size
```

**修复**: `CONFIG_WS_BUFFER_SIZE` 从 1024 → 4096 → **8192** 字节（含 WebSocket
升级响应头 + 服务器可能附加的 Set-Cookie、安全头等）。

---

### 5b：服务器要求 User-Agent

**修复**: `Websocket.c` 中添加：
```c
websocket_cfg.user_agent = "ESP32";
```

Nginx / Cloudflare 前端在没有 `User-Agent` 的场景下返回 400。

---

## 问题 6：secret 参数变成乱码（最终根因）

### 现象

```
ws?secret=b1f9562544a348c98c57a66b32a92d32  ← main.cpp 拼接
ws?secret=�f                                ← 实际发出的 HTTP 请求
```

### 三层排查

**第一层**: 怀疑 `device_secret.h` 缺少 `extern "C"` → 加上，无效。

**第二层**: 检查 ELF 符号表：

```bash
$ xtensa-esp32s3-elf-objdump -t build/ina231_example.elf | grep -w secret
3fc9beb0  w    O .dram0.data    00000004 secret
                               ^^^^^^^^  只有 4 字节！
```

**第三层**: 查链接映射文件 `ina231_example.map`：

```
.data.secret   0x4  esp-idf/main/libmain.a(tasks.cpp.obj)
                 ^  来自 tasks.cpp，不是 device_secret.c
```

### 真根因：弱符号 + Archive 链接顺序

`tasks.cpp` 中有一段看似无害的"兼容"代码：

```cpp
// tasks.cpp
__attribute__((weak)) const char *secret = "default-secret";
//                         ^^^^^ 4 字节指针     ^^^^^^^^^^^^^^^ 不是 64 字节数组
```

而 `device_secret.c` 中才是真正的定义：

```c
// device_secret.c
char secret[SECRET_MAX_LEN] = "b1f9562544a348c98c57a66b32a92d32";
//         ^^^^^^^^^^^^^^^^ 64 字节数组
```

链接过程：

```
1. main.cpp.obj 引用 secret（未定义）→ 链接器记下需求
2. 扫描 libmain.a → 发现 tasks.cpp.obj 中有 __attribute__((weak)) secret
   → 拉入！引用已满足，是 4 字节指针
3. 扫描 libdevice_secret.a → 没有未解析引用了
   → device_secret.c.obj 根本不会链接进来
```

**结果**: `secret` 指向 `tasks.cpp` 中那个 4 字节的 `"default-secret"` 字符串指针，
`snprintf(ws_path, ..., "secret=%s", secret)` 读到的是随机堆栈数据。

### 修复

```cpp
// tasks.cpp：删除弱符号定义
// __attribute__((weak)) const char *secret = "default-secret";  ← 删掉

// 改为 include 真正的定义
#include "device_secret.h"
```

修复后：

```bash
$ xtensa-esp32s3-elf-objdump -t build/ina231_example.elf | grep -w secret
3fc9bec0 g     O .dram0.data    00000040 secret
         ^                      ^^^^^^^^
         强符号                   64 字节 ✅
```

---

## 补遗：强符号 vs 弱符号 vs Archive 链接

### 强符号覆盖弱符号？

很多人记住的规则是"强符号覆盖弱符号"——这在**两个 `.o` 文件被同时输入给链接器**
时是对的。

### Archive 链接的特殊性

ESP-IDF 的构建系统产生的是**静态库** (`.a`)，即 **Archive**：

```
Archive (.a) = 若干 .o 文件打包在一起
```

Archive 链接遵循的是 **"先到先得"**（精确来说是 archive 的单次扫描规则）：

1. 链接器从左到右扫描命令行中的 `.o` 和 `.a`
2. 遇到 `.a` 时，只从中提取**能满足当前未解析符号**的 `.o` 文件
3. 如果某个弱符号已被拉入并解析了该符号 → 后续 `.a` 中的强符号**不会被看到**

```mermaid
graph LR
    A[main.obj: 引用 secret] --> B[libmain.a]
    B -->|"找到弱 secret (tasks.obj)"| C[secret 已解析 ✓]
    C --> D[libdevice_secret.a]
    D -->|"无未解析引用"| E[device_secret.obj 不加载]
    E --> F["最终: secret = 4 字节弱符号"]
```

### 设计本意

`tasks.cpp` 中 `__attribute__((weak))` 的设计是让没有 `device_secret` 组件的构
建也能通过编译——但它同时犯了两个错误：

1. **类型不匹配**: `const char *` (指针) vs `char[64]` (数组)
2. **作用域期望错误**: C++ weak 不能跨 Archive 边界"让位"给强符号

---

## 时间线总览

| # | 问题 | 根因 | 修复 |
|---|------|------|------|
| 1 | 固件溢出 factory 分区 | `set-target` 重置 sdkconfig | 4MB Flash + 双 OTA 分区表 |
| 2 | SNTP 超时 / DNS 不可用 | 只等 L2 连接，没等 DHCP | 等待 `IP_EVENT_STA_GOT_IP` |
| 3 | OTA panic | 单应用分区表无 otadata | NULL 检查 + 优雅退出 |
| 4 | WebSocket 内存耗尽 | 64KB 静态缓冲区 | 降为 4KB |
| 5a | Header size exceeded | WS 传输层缓冲区不够 | 8KB |
| 5b | HTTP 400 | 缺少 User-Agent | 添加 `"ESP32"` |
| 6 | secret 乱码 | 弱符号指针 + Archive 链接顺序 | 删除弱符号，include 真正定义 |

---

## Key Takeaways

- **`set-target` 后务必检查 `sdkconfig`**：脚本会彻底重置，所有手动修改丢失。
- **WiFi 启动时序**: 网络操作（SNTP, HTTPS, WSS）必须等 `IP_EVENT_STA_GOT_IP`，而非 `WIFI_EVENT_STA_CONNECTED`。
- **Archive 中的弱符号陷阱**: `.a` 中的弱符号可能导致后续 `.a` 中同名强符号被跳过，类型不匹配时链接器也默不作声。
- **善用 ELF 工具**: `nm` 看符号类型/大小，`objdump -t` 查 section，`.map` 追踪来源。

---
*ESP-IDF v5.5.3 · ESP32-S3 · 2026-05-01*
