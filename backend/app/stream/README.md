# Stream 模块文档

## 概述

Stream 模块负责设备推流与观看者之间的 **WebSocket 双向通信**，包括：

- 设备（ESP32）WebSocket 连接管理
- 观看者 WebSocket 连接管理
- 设备 → 观看者的音视频 / 文本消息转发
- 观看者 → 设备的控制指令转发

---

## 架构

```
┌─────────────┐    WebSocket     ┌──────────┐    WebSocket     ┌─────────────┐
│  ESP32 设备  │ ◄─────────────► │  Server  │ ◄─────────────► │  观看者客户端 │
└─────────────┘   /device/ws     │ (Stream) │   /viewer/ws/   └─────────────┘
                                 └──────────┘   {device_id}
```

### 核心对象

| 对象 | 位置 | 说明 |
|------|------|------|
| `Device.Esp32` | `deviceapi/Device.py` | 设备实例，维护 WebSocket 连接与订阅者列表 |
| `Viewer.Viewer` | `stream/Viewer.py` | 观看者实例，维护 WebSocket 连接与订阅设备列表 |

### 全局注册表

| 变量 | 键类型 | 说明 |
|------|--------|------|
| `Device.esp32IdDict` | `device_id` | 在线设备查找表 |
| `Viewer.viewerIdDict` | `user_id` | **旧模式**：用户 → Viewer（已弃用） |
| `Viewer.viewerWsDict` | `(user_id, device_id)` | **新模式**：(用户, 设备) → Viewer |

---

## 观看者模式

### 新模式（推荐）— 一个 WebSocket 对应一个设备

**一个用户可以同时开启多个 WebSocket 连接，每个连接订阅一个不同的设备。**

#### 连接流程

```
客户端                                              服务器
  │                                                    │
  │  ws://host/api/stream/viewer/ws/{device_id}        │
  │         ?token=<JWT>                               │
  ├───────────────────────────────────────────────────►│
  │                                                    ├─ 验证 token
  │                                                    ├─ 验证设备存在 & 在线
  │                                                    ├─ 关闭同 (user, device) 旧连接
  │                                                    ├─ 创建 Viewer 对象
  │                                                    ├─ 自动订阅该设备
  │◄───────────────────────────────────────────────────┤
  │  {"code": 200, "msg": "Subscribed to device X"}    │
  │                                                    │
  │  ── 双向消息通信 ──                                  │
  │                                                    │
  │  客户端 → 服务器 (JSON 文本)                         │
  ├───────────────────────────────────────────────────►│──► 转发到设备
  │                                                    │
  │  设备 → 服务器 (文本 / 二进制)                       │
  │◄───────────────────────────────────────────────────┤◄── 来自设备
  │                                                    │
  │  ── 断开 ──                                         │
  │  (客户端关闭或设备断开)                               │
  │                                                    ├─ 自动取消订阅
  │                                                    ├─ 清理 Viewer 对象
```

#### 关键特性

- **自动订阅**：WebSocket 连接建立后自动订阅目标设备，无需额外 HTTP 调用
- **自动取消订阅**：WebSocket 断开时自动清理订阅关系
- **设备断开通知**：设备离线时，服务器向观看者发送 `"设备已断开"` 文本消息，随后关闭观看者 WebSocket
- **多设备并行观看**：同一用户可同时打开多个 WebSocket，每个连接观看不同设备
- **连接去重**：同一 (user, device) 对若已有连接，旧连接会被自动关闭

#### WebSocket URL

```
ws://host/api/stream/viewer/ws/{device_id}?token=<JWT>
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `device_id` | int (路径参数) | 要订阅的设备 ID |
| `token` | string (查询参数) | 用户 JWT 认证令牌 |

#### 客户端发送格式

向设备转发 JSON 控制指令：

```json
{"code": 1, "item": "status", "key": "status", "values": "stream"}
```

#### 服务器推送格式

- **JSON 文本**：来自设备的状态响应或其他文本消息
- **二进制帧**：来自设备的音视频流数据
- **`"设备已断开"`**：设备离线通知（收到后连接将被关闭）

#### 错误情况

| 错误码 | 说明 |
|--------|------|
| WebSocket 1008 | `Device not found` — 设备 ID 不存在 |
| WebSocket 1008 | `Device not connected` — 设备不在线 |
| `{"code": 400, ...}` | 订阅失败（设备 WebSocket 不可达），连接随后关闭 |

---

### 旧模式（已弃用）— 一个 WebSocket 对应多个设备

> ⚠️ 以下接口已标记为 `deprecated`，仍可使用但建议迁移到新模式。

旧模式需要三步操作：

1. **注册观看者** — `POST /api/stream/viewer`
2. **订阅设备** — `POST /api/stream/viewer/following/{device_id}`
3. **建立 WebSocket** — `ws://host/api/stream/viewer/ws?token=<JWT>`

取消订阅和注销同样需要额外的 HTTP 调用。

#### 已弃用端点

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/stream/viewer` | 注册观看者身份 |
| DELETE | `/api/stream/viewer` | 注销观看者身份 |
| POST | `/api/stream/viewer/following/{device_id}` | 订阅设备推流 |
| DELETE | `/api/stream/viewer/following/{device_id}` | 取消订阅设备 |
| WebSocket | `/api/stream/viewer/ws?token=<JWT>` | 旧模式 WebSocket 连接 |

---

## 设备端 WebSocket

### 连接 URL

```
ws://host/api/stream/device/ws?secret=<设备密钥>
```

### 行为

1. 设备通过 `secret` 认证后建立连接
2. 服务器发送 standby 指令，设备默认进入待机状态
3. 当首个观看者订阅时，服务器发送 stream 指令，设备开始推流
4. 当所有观看者取消订阅后，服务器发送 standby 指令，设备返回待机
5. 设备断开时，通知所有订阅者

---

## 文件结构

```
stream/
├── Stream.py      # WebSocket 端点 & 消息处理逻辑
├── Viewer.py      # Viewer 类定义 & HTTP 端点（已弃用）
└── README.md      # 本文档
```

### Stream.py 内部结构

| 区域 | 内容 |
|------|------|
| 设备指令常量 | `CMD_STANDBY`, `CMD_STREAM` |
| 消息转发工具 | `_forward_text()`, `_forward_bytes()` |
| 设备文本消息处理 | `_handle_device_status_response()`, `_on_device_text()`, `_on_device_bytes()` |
| 连接初始化工具 | `_close_old_device_connection()`, `_close_old_viewer_connection()`, `_close_old_viewer_device_connection()` |
| WebSocket 端点 | `device_websocket_endpoint`, `viewer_websocket_endpoint`(旧), `viewer_device_websocket_endpoint`(新) |

### Viewer.py 内部结构

| 区域 | 内容 |
|------|------|
| 全局注册表 | `viewerIdDict`(旧), `viewerWsDict`(新) |
| Viewer 类 | 连接管理、订阅管理、双模式注册/注销 |
| 内部工具函数 | `_is_device_ws_alive()`, `_resolve_viewer_and_device()` |
| HTTP 端点 | 注册/注销观看者、订阅/取消订阅设备（均已弃用） |
