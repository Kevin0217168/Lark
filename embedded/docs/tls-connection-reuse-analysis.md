# ESP32 并发 TLS 握手导致间歇性连接失败的排查与优化

> ESP32 上多个 HTTPS 任务并发 TLS 握手时，偶发 `PK verify failed` 签名验证失败。本文从内部 SRAM 峰值争抢的角度定位根因，并通过 HTTP 连接持久化复用将 TLS 握手频率降低 99.9%，彻底消除问题。

> **环境**: ESP-IDF v5.5 / ESP32 (4MB PSRAM) / mbedTLS  
> **现象**: HTTPS POST 传感器数据时随机出现 `PK verify failed with error 0x4290`，但同一请求有时成功有时失败  
> **根因**: 多任务并发 TLS 握手争抢内部 SRAM，RSA 签名验证因内存不足而失败  
> **修复**: HTTP 连接持久化复用 + 请求重试

---

## 一、问题现象

设备运行期间，传感器数据每 60 秒上报一次，日志中周期性出现以下错误：

```
E esp-x509-crt-bundle: PK verify failed with error 0x4290
E esp-x509-crt-bundle: Certificate matched but signature verification failed
E esp-x509-crt-bundle: Failed to verify certificate
E esp-tls-mbedtls: mbedtls_ssl_handshake returned -0x3000
E esp-tls: Failed to open new connection
E HTTP_CLIENT: Connection failed, sock < 0
```

奇怪的是，**同一个接口、同样的数据、同一台服务器**——上一分钟还成功返回 200，下一分钟就 TLS 握手失败。跟服务器证书本身没有关系（证书有效期内，其他时段正常）。

---

## 二、系统架构与并发模型

设备上同时运行着多个需要 HTTPS 的任务：

```
┌─────────────────────┐     ┌──────────────────────┐     ┌──────────────────┐
│  remote_log (rlog)  │     │ sensor_data_transmit │     │   websocket_task │
│  每 ~1s POST 日志    │     │  每 60s POST 传感器   │     │   持久 WSS 连接   │
│  独立 HTTP client   │     │  全局 HTTP client     │     │                  │
└────────┬────────────┘     └────────┬─────────────┘     └──────────────────┘
         │                           │
         ▼                           ▼
   ┌───────────┐              ┌───────────┐
   │ TLS 握手   │              │ TLS 握手  │
   │ (mbedTLS) │              │ (mbedTLS) │
   └─────┬─────┘              └─────┬─────┘
         │                          │
         ▼                          ▼
   ┌────────────────────────────────────────┐
   │        ESP32 内部 SRAM (~307 KB)        │
   │     空闲仅 35~70 KB (11%~22%)           │
   └────────────────────────────────────────┘
```

关键数据来自健康监控日志：

```
[内存] 堆=3318188/4501391B(73%) 最低=3279536B
       内部=35691/307087B(11%) | PSRAM=3285836/4194304B(78%)
```

虽然总堆空间（含 PSRAM）还剩 73%，但**内部 SRAM 只剩 11%（~35KB）**。而 mbedTLS 的密钥运算必须使用内部 SRAM（不能用 PSRAM），一次 TLS 握手的 RSA 签名验证临时需要 20~40KB 内部 SRAM。

---

## 三、根因分析

### 3.1 旧代码：每次 flush 都新建 TLS 连接

修改前的 `remote_log_http_post()` 实现：

```c
// ❌ 旧代码：每次调用都 init → TLS握手 → perform → close → cleanup
static esp_err_t remote_log_http_post(const char *data, int len)
{
    esp_http_client_config_t cfg = {
        .url = s_upload_url,
        .method = HTTP_METHOD_POST,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .keep_alive_enable = true,   // 虽然开了 keep-alive，但...
    };

    // 每次都创建新 client
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    
    esp_http_client_set_header(client, "Content-Type", "text/plain");
    esp_http_client_set_header(client, "Authorization", s_secret);
    esp_http_client_set_post_field(client, data, len);
    
    // perform 内部会做完整 TLS 握手（TCP → TLS → HTTP）
    esp_err_t err = esp_http_client_perform(client);

    // 用完直接销毁
    esp_http_client_close(client);
    esp_http_client_cleanup(client);  // ← 连接和 TLS 上下文全部释放
    return err;
}
```

**问题**：虽然配置了 `keep_alive_enable = true`，但每次调用结束都 `cleanup` 销毁了 client 对象，**下次调用只能重新创建、重新握手**。`keep_alive` 形同虚设。

由于 flush 任务每秒运行一次，这意味着**每秒都会进行一次完整的 TLS 握手**。

### 3.2 时间线碰撞

```
时间轴 ──────────────────────────────────────►

rlog:   [TLS握手][TLS握手][TLS握手]...[TLS握手][TLS握手]...
               每 ~1-2 秒一次             ↑
                                         │ 恰好重叠
sensor:                                  │
         ────────── 60s ────────── [TLS握手] ← 失败!
```

每分钟总有一次传感器 POST 的 TLS 握手和 rlog 的握手**时间重叠**。两个并发握手同时向内部 SRAM 申请临时缓冲区：

| 资源                | 需求         |
|---------------------|-------------|
| 一次 TLS 握手 RSA 验签 | ~20-40 KB   |
| 内部 SRAM 空闲      | ~35-70 KB |
| 两次并发握手总需求   | ~40-80 KB   |

当内部 SRAM 处于低位（35KB）时，两个并发握手总需求超过可用空间，后发起的那个就会在 RSA 签名验证阶段因 `malloc` 失败而报错 `0x4290`。

### 3.3 为什么"有时成功有时失败"?

这是**概率性**的，取决于两个条件：

1. **时间是否重叠**：rlog 每 ~1s 一次握手，sensor 每 60s 一次。两者碰撞概率大约 30-50%（取决于握手耗时 ~0.5-1s）
2. **内部 SRAM 当时的空闲量**：其他任务（WiFi、WebSocket、Camera）的内存占用是波动的，空闲量在 35-70KB 间浮动

两个条件同时满足（重叠 + 内存低位）时失败，否则成功。

---

## 四、修复方案

### 4.1 核心修复：HTTP 连接持久化复用

```c
// ✅ 新代码：持久化 client，复用 TLS 连接
static esp_http_client_handle_t s_persistent_client = NULL;

static esp_http_client_handle_t rlog_get_client(void)
{
    if (s_persistent_client != NULL) {
        return s_persistent_client;  // 直接返回已有 client
    }

    // 仅首次（或重建时）创建
    esp_http_client_config_t cfg = {
        .url = s_upload_url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = RLOG_HTTP_TIMEOUT_MS,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .buffer_size = 1024,
        .keep_alive_enable = true,
    };

    s_persistent_client = esp_http_client_init(&cfg);
    if (s_persistent_client != NULL) {
        esp_http_client_set_header(s_persistent_client, "Content-Type", "text/plain");
        esp_http_client_set_header(s_persistent_client, "Authorization", s_secret);
    }
    return s_persistent_client;
}

static esp_err_t remote_log_http_post(const char *data, int len)
{
    esp_http_client_handle_t client = rlog_get_client();
    if (client == NULL) return ESP_FAIL;

    esp_http_client_set_post_field(client, data, len);

    // perform 在 keep-alive 连接上直接发 HTTP 请求，无需 TLS 握手
    esp_err_t err = esp_http_client_perform(client);

    if (err != ESP_OK) {
        // 连接断开时销毁，下次调用会自动重建
        ESP_LOGW(TAG, "持久连接失败, 将重建: %s", esp_err_to_name(err));
        rlog_destroy_client();
    }
    // 注意：成功时不 close/cleanup，连接保持
    return err;
}
```

### 4.2 关键区别对比

| 维度 | 旧代码（每次新建） | 新代码（持久复用） |
|------|--------------------|--------------------|
| **client 生命周期** | 每次 flush 创建并销毁 | 首次创建，长期持有 |
| **TLS 握手频率** | 每秒 1 次 | 仅首次 + 断线重连 |
| **内部 SRAM 峰值** | 每秒出现 20-40KB 尖峰 | 稳态仅 ~5KB（TLS 上下文） |
| **与 sensor 任务碰撞概率** | ~30-50% | ≈0%（几乎无握手） |
| **`Certificate validated` 日志** | 每 2 秒一条（刷屏） | 仅启动时 1 条 |
| **网络开销** | 每次 TCP+TLS 三次握手 | HTTP keep-alive 复用 |

### 4.3 `esp_http_client_perform` 的内部行为

理解这个修复的关键在于 `esp_http_client_perform()` 的内部逻辑：

```
esp_http_client_perform(client)
    │
    ├── 连接未建立？
    │   ├── DNS 解析
    │   ├── TCP connect (三次握手)
    │   ├── TLS handshake ← 内存密集操作!
    │   │   ├── ClientHello → ServerHello
    │   │   ├── 服务器证书验证 (x509 bundle)
    │   │   ├── RSA/ECDSA 签名验证 ← 需要 20-40KB 内部 SRAM
    │   │   └── 密钥交换 → Finished
    │   └── 连接就绪
    │
    ├── 连接已建立且 keep-alive？
    │   └── 直接跳到 HTTP 请求 ← 零额外内存开销!
    │
    ├── 发送 HTTP 请求
    ├── 接收 HTTP 响应
    └── 返回结果
```

旧代码每次都走左边分支（完整握手），新代码只有首次走左边，后续全部走右边（直接发请求）。

### 4.4 辅助修复：传感器 POST 增加重试

即使持久化连接大幅降低了碰撞概率，极端情况下仍可能遇到（比如持久连接断线重建时恰好碰上 sensor POST）。因此增加重试兜底：

```c
#define SENSOR_POST_MAX_RETRY   3
#define SENSOR_POST_RETRY_MS    3000  // 重试间隔 3 秒

// 带重试的 HTTPS POST
int ret_code = ESP_FAIL;
for (int attempt = 0; attempt < SENSOR_POST_MAX_RETRY; attempt++) {
    ret_code = WifiSecurityRequest("https://lark.mintlab.top",
                                   "/api/sensors", 443,
                                   WS_CLINENT_METHOD_POST, post_data, NULL);
    if (ret_code == ESP_OK) break;
    
    ESP_LOGW(TAG, "传感器上报失败 (%d/%d), %dms 后重试...",
             attempt + 1, SENSOR_POST_MAX_RETRY, SENSOR_POST_RETRY_MS);
    vTaskDelay(pdMS_TO_TICKS(SENSOR_POST_RETRY_MS));
}
```

3 秒的重试间隔不是随意选的——它足够让另一个 TLS 握手完成并释放内部 SRAM。

---

## 五、实测验证

部署修复后的固件 (v0.7.1)，通过 OTA 升级并观察日志。

### 5.1 启动阶段：仍有碰撞，但重试兜住了

OTA 重启后，前 5 秒内 OTA 检查、rlog 首次 flush、sensor 首次 POST **三个任务几乎同时发起 TLS 握手**：

```
(5050) ota:     Checking for firmware update    → TLS 握手
(5070) sensor:  POST /api/sensors               → TLS 握手
(5070) rlog:    首次 flush                       → TLS 握手（持久 client 首次创建）
```

sensor 的前两次握手失败（`0x4290` 和 `0x7F00`），第三次重试成功——此时 OTA 和 rlog 已完成握手释放了内部 SRAM：

```
(5240)  task: 传感器上报失败 (1/3), 3000ms 后重试...
(8320)  task: 传感器上报失败 (2/3), 3000ms 后重试...
(12190) http_client: POST .../api/sensors -> 200  ✅ 第三次成功
```

**重试机制达到预期效果**。

为进一步消除启动碰撞，在 sensor 任务入口增加 5 秒延迟，错开与 OTA/rlog 的首次握手窗口：

```c
void sensor_data_transmit_task(void *pvParameter)
{
    // 启动后延迟 5 秒，错开 OTA/rlog 首次 TLS 握手的内存高峰
    vTaskDelay(pdMS_TO_TICKS(5000));
    // ...
}
```

### 5.2 稳态运行：问题彻底消除

启动阶段过后，对比效果显著：

| 指标 | 修复前 (v0.7.0) | 修复后 (v0.7.1) |
|------|----------------|----------------|
| `Certificate validated` 频率 | 每 ~2 秒一条（刷屏） | 启动后消失 |
| sensor POST 成功率 | ~60-70%（每分钟偶发失败） | 100%（稳态零重试） |
| rlog `consec_fail` | 偶发 1+ | 稳定 0 |
| rlog stats 间隔 | ~20 秒（退避中） | ~10 秒（正常，无退避） |

关键证据——稳态下 `Certificate validated` 日志**完全消失**，证明 rlog 持久连接复用生效，不再每秒做 TLS 握手：

```
00:29:28  remote_log: [stats] sent=6746 B, consec_fail=0
00:29:38  remote_log: [stats] sent=6879 B, consec_fail=0   ← +133B, 无握手
00:29:48  remote_log: [stats] sent=7012 B, consec_fail=0   ← +133B, 无握手
00:30:22  http_client: POST .../api/sensors -> 200          ← 一次成功，无重试
```

### 5.3 实际数据对比

**TLS 握手次数**：

```
修复前: ~1次/秒 × 3600秒/小时 = 3600 次/小时
修复后: 1次/启动 + 偶发重建 ≈ 1-5 次/小时
减少: 99.9%
```

**内部 SRAM**（来自 sys_mon 实测）：

```
修复前: 内部=35691/307087B (11%)  ← TLS 握手后的谷值
修复后: 内部=38731/307079B (12%)  ← 稳态值，无尖峰波动
```

稳态下内部 SRAM 不再出现周期性尖峰，彻底消除了并发碰撞的条件。

---

## 六、经验总结

### 6.1 `keep_alive_enable = true` 不是万能的

ESP-IDF 的 `keep_alive_enable` 只是在 HTTP 层面告诉服务器"不要关闭连接"，但如果你在客户端代码里每次都 `cleanup` 销毁了 client 对象，keep-alive 就毫无意义——对象都没了，哪来的连接可以复用？

**正确用法**: 创建一次 client，多次调用 `perform`，只在不再需要时 `cleanup`。

### 6.2 ESP32 的"两种内存"陷阱

ESP32 有 PSRAM 时总堆空间看起来很大（4.5MB），但 mbedTLS 加密运算只能用**内部 SRAM**（~307KB），而系统本身（WiFi、freeRTOS、任务栈等）已占用大部分。实际可用于 TLS 握手的空间非常紧张。

`esp_get_free_heap_size()` 返回的数字包含 PSRAM，容易给人"内存充裕"的错觉。排查 TLS 问题时要看 `heap_caps_get_free_size(MALLOC_CAP_INTERNAL)`。

### 6.3 间歇性 TLS 失败的排查思路

当 TLS 握手"有时成功有时失败"时：

1. **不是证书问题**——证书问题是 100% 失败，不会间歇
2. **不是网络问题**——网络问题表现为超时，不是签名验证失败
3. **大概率是内存问题**——特别是 `PK verify failed`，说明 RSA 运算分配不到足够内存
4. **检查并发**——找出所有同时做 TLS 的任务，计算峰值内存需求

### 6.4 连接复用是嵌入式 HTTPS 的最佳实践

在资源受限的嵌入式设备上，每个 TLS 连接的建立成本很高：

| 成本维度 | 数值 |
|---------|------|
| 时间 | 1-3 秒（含 TCP + TLS 握手） |
| 内部 SRAM 峰值 | 20-40 KB |
| CPU | RSA-2048 验签约 0.5-1 秒 |

对于周期性请求同一服务器的场景，**连接复用**应该是默认选择，而不是每次新建。
