# 基于 asyncio.Future 实现 WebSocket 请求-响应模式：异步获取 IoT 设备版本号

> 在 FastAPI + WebSocket 架构中，HTTP 接口和 WebSocket 消息循环是两个独立的协程。当 HTTP 接口需要向远端设备"提问"并等待回答时，如何优雅地桥接二者？本文以**获取 ESP32 设备固件版本号**为例，记录一种基于 `asyncio.Future` 的请求-响应实现方案。

## 1. 问题背景

项目中后端通过 WebSocket 长连接与 ESP32-CAM 设备通信，前端通过 HTTP/WebSocket 与后端交互。已有的设备控制指令（重启、OTA）都是"发射后不管"（fire-and-forget）——发完指令就返回，无需等待设备的具体响应内容：

```python
# 重启：发完即返回 "ok"
await device.websocket.send_json(CMD_RESTART)
return CommonOut(msg="Restart command sent.", data={"key": "restart", "values": "ok"})
```

但**获取版本号**不同，它是一个 **请求-响应（Request-Response）** 模式：

```
HTTP 客户端 ──GET /devices/1/version──► 后端 ──ws.send──► ESP32
                                         ▲                    │
                                         │    ws.receive      │
HTTP 客户端 ◄──返回版本号────────────── 后端 ◄─────────────────┘
```

后端必须**等待设备回复**后，才能把版本号返回给前端。

## 2. 为什么不能用简单方案？

### 方案一：在 HTTP 处理函数中直接 `receive()`？

```python
# ❌ 错误方案
async def get_device_version(id):
    await device.websocket.send_json(CMD_VERSION)
    response = await device.websocket.receive_json()  # 阻塞等待
    return response
```

**问题**：WebSocket 连接上的 `receive()` 只能有一个消费者。设备的消息循环（`Stream.py`）已经在持续 `await websocket.receive()` 了。如果 HTTP 处理函数也去 `receive()`：
- 两个协程竞争同一个 WebSocket 的读取权 → **未定义行为**
- 设备可能先发一帧视频流二进制数据，HTTP 侧收到的不是版本号响应 → **消息错乱**
- 消息循环那边丢失了这条消息 → **其他功能被破坏**

### 方案二：轮询共享变量？

```python
# ❌ 低效方案
device.latest_version = None
await device.websocket.send_json(CMD_VERSION)
for _ in range(50):
    if device.latest_version is not None:
        return device.latest_version
    await asyncio.sleep(0.1)
return "timeout"
```

**问题**：
- 轮询间隔固定，要么浪费 CPU 要么响应慢
- 多个请求同时查版本号时，无法区分响应归属
- 需要额外的锁/清理逻辑，实现丑陋且脆弱

### 方案三：回调函数？

回调函数是异步编程中最经典的模式——"事情做完了叫我"。我们先看看如果硬用回调会怎样：

```python
# ⚠️ 尝试用回调实现
class Esp32:
    def __init__(self):
        self.callbacks = {}  # key -> callback function

    def register_callback(self, key: str, callback):
        """注册：等收到这个 key 的响应时，调用 callback"""
        self.callbacks[key] = callback

# 消息循环中触发回调
async def _on_device_text(device, raw):
    json_data = json.loads(raw)
    key = json_data.get("key")
    if key in device.callbacks:
        device.callbacks.pop(key)(json_data)  # 调用回调
        return
    # ...正常转发...
```

看起来还不错？现在问题来了——HTTP 处理函数怎么写：

```python
# ❌ 回调的死胡同
@router.get("/{id}/version")
async def get_device_version(id):
    result = None

    def on_version(data):
        nonlocal result
        result = data   # 回调把结果写入 result

    device.register_callback("version", on_version)
    await device.websocket.send_json(CMD_VERSION)

    # 但是...现在 result 还是 None！
    # 回调还没被调用（设备还没响应）
    # 而 HTTP 函数必须现在就 return 一个值
    return CommonOut(data=result)  # ← None，错误！
```

**核心矛盾**：

```
时间线：
  T0  HTTP handler: 注册回调, 发送指令
  T1  HTTP handler: 函数执行到末尾, 必须 return  ← 此时设备还没响应！
  T2  WS 消息循环: 收到设备响应, 触发回调
  T3  回调执行: result = data  ← 但 HTTP 响应早就在 T1 返回了，来不及了
```

回调函数能在"将来某个时刻"处理数据，但 **HTTP 处理函数需要在当前执行流中返回结果**。回调和 HTTP handler 的生命周期是割裂的。

那加个 `await asyncio.sleep()` 等等？那就退化成方案二的轮询了。

**真正的解法**：我们需要一种方式让 HTTP handler **挂起（暂停执行），直到回调被触发后再继续**。这恰好就是 `asyncio.Future` 做的事——它把回调模式和 await 挂起机制粘合在一起：

```python
# ✅ 回调 + Future = 最终方案
async def get_device_version(id):
    future = loop.create_future()                       # 创建占位符
    device.register_callback("version", future.set_result)  # 回调 = 填充 Future
    await device.websocket.send_json(CMD_VERSION)
    result = await future   # 挂起，直到回调执行 set_result()
    return CommonOut(data=result)
```

`future.set_result` 本身就是一个函数引用——它就是那个"回调"。只不过我们不直接用回调返回数据，而是用它来 **填充 Future**。HTTP handler 那边 `await future` 就能在回调执行后被唤醒继续执行。

这正是我们最终方案的本质——**把回调的"通知"能力和 Future 的"等待"能力结合起来**。

## 3. 最终方案：`asyncio.Future` 桥接两个协程

### 3.1 核心思想

`asyncio.Future` 是 asyncio 中最底层的可等待对象。它本质上是一个 **"还没有结果的占位符"**：

- 创建时：pending 状态（空盒子）
- `set_result(value)`：filled 状态（盒子里放了东西）
- `await future`：如果还是空的就挂起等待；如果已经有值就立即返回

利用这个特性，我们可以在协程 A 中创建 Future 并等待，在协程 B 中填入结果：

```
协程A (HTTP handler)          共享的 Future           协程B (WS 消息循环)
    │                              │                       │
    ├─ create future ────────────► │                       │
    ├─ send command to device      │                       │
    ├─ await future (挂起) ◄───── pending                  │
    │                              │     收到设备响应 ────── │
    │                              │ ◄── set_result(data)   │
    ├─ await 返回, 拿到 data ◄── filled                    │
    └─ return response             │                       │
```

### 3.2 实现代码

#### Esp32 类中的两个方法

```python
class Esp32:
    def __init__(self, device):
        # ...
        self.pending_responses: dict[str, asyncio.Future] = {}
        #    key: 指令的 key 字段 (如 "version")
        #    value: 等待该响应的 Future

    async def send_and_wait(self, command: dict, key: str, timeout: float = 5.0) -> dict | None:
        """发送指令并等待设备返回匹配 key 的响应。超时返回 None。"""
        loop = asyncio.get_event_loop()
        future = loop.create_future()          # ① 创建空 Future
        self.pending_responses[key] = future   # ② 注册，供消息循环查找
        try:
            await self.websocket.send_json(command)  # ③ 发送指令
            return await asyncio.wait_for(future, timeout=timeout)  # ④ 挂起等待
        except asyncio.TimeoutError:
            return None                        # ⑤ 超时 → 返回 None
        finally:
            self.pending_responses.pop(key, None)  # ⑥ 清理

    def resolve_pending(self, key: str, data: dict) -> bool:
        """由消息循环调用：尝试匹配并填充一个挂起的 Future。"""
        future = self.pending_responses.get(key)
        if future and not future.done():
            future.set_result(data)   # 唤醒 send_and_wait 中的 await
            return True
        return False
```

#### 消息循环中的拦截逻辑

```python
# Stream.py
async def _on_device_text(device: Device.Esp32, raw: str) -> None:
    json_data = json.loads(raw)

    # 优先检查：是否有 HTTP 请求在等这条响应？
    resp_key = json_data.get("key")
    if resp_key and device.resolve_pending(resp_key, json_data):
        return  # 已消费，不转发给前端观看者

    # 常规流程：状态处理 + 转发给订阅者
    await _handle_device_status_response(device, json_data)
    await _forward_text(device, json_data)
```

#### HTTP 接口

```python
@router.get("/{id}/version", summary="查询设备固件版本")
async def get_device_version(id, db):
    # ...校验省略...
    device = esp32IdDict.get(id)

    result = await device.send_and_wait(CMD_VERSION, "version", timeout=5.0)
    if result is None:
        return JSONResponse(status_code=504, ...)  # 超时

    return CommonOut(data={"key": "version", "values": result.get("values", "")})
```

### 3.3 逐步执行流程

以 `GET /devices/1/version` 为例：

| 时间 | 事件循环调度 | 动作 |
|------|------------|------|
| T0 | HTTP handler | 创建 `Future`，存入 `pending_responses["version"]` |
| T1 | HTTP handler | `send_json(CMD_VERSION)` → 指令发往 ESP32 |
| T2 | HTTP handler | `await wait_for(future, 5.0)` → **挂起**，让出控制权 |
| T3 | WS 消息循环 | `await websocket.receive()` → 收到设备响应 |
| T4 | WS 消息循环 | `resolve_pending("version", data)` → `future.set_result(data)` |
| T5 | HTTP handler | **被唤醒**，`await` 返回 `data` |
| T6 | HTTP handler | 包装成 JSON 返回给前端 |

如果 T3 在 T2 之后 5 秒仍未发生 → `TimeoutError` → 返回 504。

## 4. 与回调函数的深度类比

### 4.1 什么是回调函数

回调的思想很简单：**"我现在没法处理，但你帮我做完后，调用我给你的这个函数"**。

生活类比：你去餐厅点餐，前台给你一个取餐呼叫器。你不用站在柜台前等，呼叫器响了你再去取。这里"呼叫器响了"就是回调被触发的时刻。

```python
# 最朴素的回调模式
def 点餐(菜名, 做好了之后):
    厨房.开始做(菜名)
    厨房.做好时调用(做好了之后)  # 注册回调

def 吃饭(菜):
    print(f"开吃: {菜}")

点餐("红烧肉", 吃饭)   # 传入回调函数
# 这里程序继续往下跑，不会等在这里
```

### 4.2 用回调实现设备版本查询——完整示例

为了更好理解 Future 解决了什么问题，我们先用纯回调硬写一遍：

```python
# ========= 纯回调实现（在 FastAPI 中行不通的版本）=========

# 1. 设备对象维护回调注册表
class Esp32:
    def __init__(self):
        self.callbacks: dict[str, callable] = {}

    def on_response(self, key, callback):
        self.callbacks[key] = callback

# 2. 消息循环：收到消息后查找并触发回调
async def _on_device_text(device, raw):
    json_data = json.loads(raw)
    key = json_data.get("key")
    cb = device.callbacks.pop(key, None)
    if cb:
        cb(json_data)  # 触发回调！
        return
    # ...正常转发...

# 3. HTTP 接口中使用
@router.get("/{id}/version")
async def get_device_version(id):
    device = esp32IdDict[id]

    # 问题在这里 ↓
    received = {}
    def my_callback(data):
        received["result"] = data   # 回调把数据存到外部字典

    device.on_response("version", my_callback)
    await device.websocket.send_json(CMD_VERSION)

    # ❌ 此刻 received 是空的！因为设备还没响应
    # ❌ 但 HTTP 函数必须返回值
    return received.get("result")  # → None
```

画成时序图看得更清楚：

```
HTTP handler 执行流         回调函数           WS 消息循环
     │                        │                    │
  T0 ├── on_response(cb) ────►│                    │
  T1 ├── send_json(CMD) ──────┼───────────────────►│
  T2 ├── return received ──►  │                    │  ← HTTP 响应已发出（None）!
     │   （函数结束了）          │                    │
  T3 │                        │    receive(data) ──┤
  T4 │  （已经不存在了）   ◄── cb(data)             │  ← 回调终于执行了
     │                        │                    │     但 HTTP 响应已经返回了
```

**根本矛盾**：回调是"事后"执行的，但 HTTP handler 的 `return` 是"当下"就要的。这两个时间点对不上。

### 4.3 回调在 Node.js/Express 中为什么能用？

你可能会问：JavaScript 的 Express 框架不就是用回调的吗？

```javascript
// Node.js Express — 回调是可以的
app.get('/version', (req, res) => {
    device.send({ key: 'version' });
    device.once('message:version', (data) => {
        res.json(data);   // ← 在回调里发送 HTTP 响应
    });
    // 注意：这里没有 return 响应内容
    // Express 不要求函数返回值，它靠 res.json() 来发响应
});
```

区别在于 **Express 的 HTTP 响应是通过 `res.send()/res.json()` 主动发送的**，不依赖函数返回值。所以回调里可以随时调 `res.json()`。

但 **FastAPI 的路由处理函数必须通过 `return` 返回响应**。函数 return 了就结束了，你在回调里没法 "return 到一个已经结束的函数"。

```python
# FastAPI：响应 = 函数返回值
@router.get("/version")
async def handler():
    return CommonOut(data=...)  # ← 只能在这里决定返回什么

# Express：响应 = 手动调用 res
app.get('/version', (req, res) => {
    // 可以在任何时候、任何回调里调用 res.json()
    setTimeout(() => res.json({...}), 1000);
});
```

### 4.4 Future = 回调的"时空传送门"

`asyncio.Future` 解决了这个时间差问题。它的作用就像一个**信封**：

1. HTTP handler 创建一个空信封（Future），然后**在信封前等着**（`await future`）—— 此时函数 **没有 return**，它挂起了
2. 回调往信封里塞了数据（`set_result(data)`）
3. HTTP handler 拆开信封拿到数据，**继续执行到 return**

```python
# 回调                              Future
# ────                              ──────

def on_version(data):               future = loop.create_future()
    # data 在这里，但没法                
    # 传回 HTTP handler              pending["version"] = future
    ???                              await ws.send_json(CMD_VERSION)
                                     
                                     data = await future  # 暂停在这里
                                     # ↑ 等价于：注册了一个回调说
                                     # "有人 set_result 时唤醒我"

# 消息循环触发回调：                    # 消息循环填充 Future：
cb(json_data)                        future.set_result(json_data)
# ↓                                  # ↓
# 回调执行了，但 HTTP handler          # await 被唤醒，data 有值了
# 已经 return 了                      # HTTP handler 继续执行到 return
```

`await future` 做了两件事：
- **暂停当前函数**（不是 return，是 suspend，函数还活着）
- **等 `set_result()` 后恢复执行**

这就把回调的"通知能力"和 HTTP handler 的"需要 return 值"完美衔接了。

### 4.5 对照表：回调 vs Future vs Promise

| 维度 | 回调函数 | asyncio.Future | JS Promise |
|------|---------|---------------|------------|
| 注册"事后操作" | 传入回调函数 | `pending[key] = future` | `.then(callback)` |
| 触发"事后操作" | 直接调用 `cb(data)` | `future.set_result(data)` | `resolve(data)` |
| 等待结果 | 没有原生支持 | `await future` | `await promise` |
| 错误传递 | 手动传 `err` 参数 | `future.set_exception(e)` + try/except | `reject(e)` + `.catch()` |
| 超时控制 | 手动 `setTimeout` + 清理 | `asyncio.wait_for(future, timeout)` | `Promise.race([p, timeout])` |
| 多个结果汇聚 | 嵌套回调（回调地狱） | `asyncio.gather(f1, f2)` | `Promise.all([p1, p2])` |
| 能否挂起调用方 | ❌ 不能 | ✅ `await` 挂起 | ✅ `await` 挂起 |
| 本质 | "做完了叫我" | "给我一个承诺，我等它兑现" | "给我一个承诺，我等它兑现" |

### 4.6 一句话总结关系

```
回调:    "嘿，做完了调用这个函数"    → 通知能力 ✓，挂起等待 ✗
Future:  "嘿，做完了往这个盒子里放结果，我在盒子前 await 着"  → 通知能力 ✓，挂起等待 ✓
```

**`asyncio.Future` 不是替代了回调，而是包装了回调。** `set_result()` 就是回调被触发的那个动作，只不过触发后的效果不是"跑一个函数"，而是 "唤醒一个正在 await 的协程"。

底层实现上，`await future` 其实就是向事件循环注册了一个回调："当这个 future 完成时，恢复执行我这个协程"。async/await 只是把这个注册过程藏在了语法糖背后，让你写出来的代码 **看起来像同步的，跑起来是异步的**。

> 如果你了解 JavaScript 的 `Promise`，`asyncio.Future` 就是 Python 中的等价物。`set_result()` 对应 `resolve()`，`await future` 对应 `await promise`。历史上 Python 也是受 JS Promise 启发才在 asyncio 中引入了 Future + async/await 语法。

## 5. 为什么必须使用异步方案？

回到我们的场景——**FastAPI + 单线程事件循环 + WebSocket 长连接**：

### 5.1 WebSocket 消息流是独占的

设备的 WebSocket 连接上跑着一个持续的消息循环：

```python
while True:
    data = await websocket.receive()  # 独占读取
    if "text" in data:
        await _on_device_text(device, data["text"])
    if "bytes" in data:
        await _on_device_bytes(device, websocket, data["bytes"])
```

你不可能从另一个协程去 `receive()` 同一个 WebSocket——这会导致**竞态条件**。所以响应 **只能从消息循环里获取**，然后通过某种机制传给等待方。

### 5.2 HTTP 处理函数需要"等"

FastAPI 的路由处理函数必须返回一个值。对于版本号查询，你不能"先返回再补数据"——HTTP 是请求-响应模型。所以处理函数 **必须挂起等待**，直到拿到结果或超时。

### 5.3 不能阻塞事件循环

如果用 `time.sleep()` 或忙等待来"等"，整个事件循环都会冻结——**所有其他请求、所有 WebSocket 通信全部停摆**。必须用 `await` 让出控制权，让事件循环在等待期间继续处理其他任务。

`asyncio.Future` + `await` 完美满足这三个约束：
- 不触碰 WebSocket 的读取权 ✓
- 让 HTTP 处理函数挂起等待 ✓  
- 不阻塞事件循环 ✓

## 6. 扩展思考

### 6.1 并发安全

如果两个前端同时查同一设备的版本号怎么办？当前实现中 `pending_responses` 用 `key` 作为字典键，后注册的会覆盖先注册的 Future。

如需支持并发，可以改为用唯一请求 ID：

```python
import uuid

async def send_and_wait(self, command, key, timeout=5.0):
    request_id = f"{key}:{uuid.uuid4().hex[:8]}"
    command["_req_id"] = request_id  # 需要设备原样返回此字段
    future = loop.create_future()
    self.pending_responses[request_id] = future
    # ...
```

但对于嵌入式设备场景（ESP32 资源有限，协议简单），当前按 `key` 匹配的方案已经够用。

### 6.2 方案适用范围

这个模式适用于任何需要"通过一个共享通道发送请求、从同一通道接收响应"的场景：

- IoT 设备控制（本文场景）
- 消息队列的 RPC 模式（通过 MQ 发请求、等回复）
- 多路复用的 TCP 连接上的请求-响应匹配

核心思路始终是：**创建 Future → 注册到共享字典 → 发送请求 → 等待 Future → 另一端填充结果**。

---

*基于 FastAPI + asyncio + WebSocket + ESP32-CAM 的实际项目经验整理。*
