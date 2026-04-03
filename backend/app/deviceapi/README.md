## 异步获取设备版本号 — 实现解析

### 核心问题

获取版本号与重启/OTA 不同——后者是"发射后不管"（fire-and-forget），而版本号查询需要 **发送请求 → 等待设备响应 → 将结果返回给 HTTP 调用者**。

挑战在于：设备的 WebSocket 消息循环运行在 Stream.py 中，而 HTTP 请求处理在 Device.py 中，二者是两个不同的异步协程。需要一个跨协程的通信机制。

---

### 整体架构

```
HTTP 请求 (GET /devices/{id}/version)
    │
    ├─1─► 创建 asyncio.Future, 存入 device.pending_responses["version"]
    ├─2─► 通过 WebSocket 发送查询指令给设备
    ├─3─► await asyncio.wait_for(future, timeout=5.0) ── 挂起等待
    │                                                        │
    │     ┌─────── 设备 WebSocket 消息循环 (Stream.py) ───────┐
    │     │ 收到设备文本消息                                   │
    │     │   ├── json_data.get("key") == "version"           │
    │     │   ├── device.resolve_pending("version", json_data)│
    │     │   └── future.set_result(json_data) ◄──────────────┘
    │                    │
    └─4─► future 被 resolve, await 返回结果
    │
    └─5─► 将版本号包在 CommonOut 中返回给前端
```

---

### 第一步：`asyncio.Future` — 跨协程的"承诺"

```python
# Device.py - Esp32 类
class Esp32:
    def __init__(self, device: Db.M_Devices):
        # ...
        self.pending_responses: dict[str, asyncio.Future] = {}
```

`pending_responses` 是一个字典，key 是指令的 `key` 字段（如 `"version"`），value 是一个 `asyncio.Future` 对象。

`asyncio.Future` 是 asyncio 中最底层的可等待对象（awaitable）。它代表一个**尚未完成的异步操作的结果**：
- 创建时处于 **pending** 状态
- 调用 `set_result(value)` 后变为 **resolved**，所有 `await` 它的协程会被唤醒并拿到 value
- 本质是一个**跨协程的同步原语**，相当于"回调"的结构化替代品

---

### 第二步：`send_and_wait()` — 发送指令并挂起等待

```python
async def send_and_wait(self, command: dict, key: str, timeout: float = 5.0) -> dict | None:
    """发送指令并等待设备返回匹配 key 的响应。超时返回 None。"""
    loop = asyncio.get_event_loop()
    future = loop.create_future()          # ① 创建 Future
    self.pending_responses[key] = future   # ② 注册到字典，供消息循环查找
    try:
        await self.websocket.send_json(command)  # ③ 发送指令给设备
        return await asyncio.wait_for(future, timeout=timeout)  # ④ 挂起，等 Future 被 resolve
    except asyncio.TimeoutError:
        return None                        # ⑤ 超时返回 None
    finally:
        self.pending_responses.pop(key, None)  # ⑥ 无论成功/超时，清理注册
```

**逐行解析：**

| 步骤 | 代码 | 作用 |
|------|------|------|
| ① | `loop.create_future()` | 在当前事件循环上创建一个空的 Future 对象 |
| ② | `self.pending_responses[key] = future` | 把 Future 按 key 存入字典，这样 Stream.py 的消息循环收到响应时能找到它 |
| ③ | `await self.websocket.send_json(command)` | 通过 WebSocket 发送 JSON 指令到 ESP32 设备 |
| ④ | `await asyncio.wait_for(future, timeout)` | **关键行**：当前协程挂起（让出控制权给事件循环），直到 future 被 set_result 或超时 |
| ⑤ | `except asyncio.TimeoutError` | 5 秒内设备没响应，`wait_for` 抛出超时异常，返回 None |
| ⑥ | `finally: pop` | 确保不论成功或失败都清理 pending_responses，防止内存泄漏 |

`asyncio.wait_for(future, timeout)` 的行为：
- 它包装了 `future`，加上超时机制
- 如果 timeout 秒内 future 被 resolve → 返回 future 的 result
- 如果超时 → 取消 future 并抛出 `asyncio.TimeoutError`

---

### 第三步：`resolve_pending()` — 消息循环侧解析响应

```python
def resolve_pending(self, key: str, data: dict) -> bool:
    """解析并完成一个挂起的 Future。返回是否成功匹配。"""
    future = self.pending_responses.get(key)
    if future and not future.done():
        future.set_result(data)  # 唤醒 send_and_wait 中的 await
        return True
    return False
```

这是一个**同步方法**（非 async），因为 `set_result()` 本身是同步的——它只是标记 Future 的状态为完成，事件循环会在下一轮调度时唤醒等待它的协程。

---

### 第四步：Stream.py 中的消息循环 — 桥接两端

```python
async def _on_device_text(device: Device.Esp32, raw: str) -> None:
    """解析设备文本消息 → 解析挂起请求 → 处理状态响应 → 转发给订阅者。"""
    try:
        json_data = json.loads(raw)
    except json.JSONDecodeError:
        # ...
        return

    # 检查是否有挂起的请求等待此响应
    resp_key = json_data.get("key")
    if resp_key and device.resolve_pending(resp_key, json_data):
        await async_log(logger, "info", f"设备{device.id} 响应已匹配挂起请求: {resp_key}")
        return  # 已消费，不转发给订阅者

    # 常规流程：处理状态变更 + 转发
    await _handle_device_status_response(device, json_data)
    await _forward_text(device, json_data)
```

**关键逻辑：**
1. 设备 WebSocket 消息循环收到一条文本消息
2. 解析 JSON 取出 `key` 字段
3. 调用 `device.resolve_pending(key, json_data)` 尝试匹配挂起的 Future
4. 如果匹配成功（`return True`）→ **直接 return，不再转发给订阅者**（这是版本号查询的私有响应）
5. 如果没有匹配 → 走常规流程（状态处理 + 转发）

---

### 第五步：HTTP 接口 — 入口

```python
@router.get("/{id}/version", ...)
async def get_device_version(id, db):
    # 1. 数据库校验设备存在
    data = Db.GetDevices(db, id=id)
    if not data:
        return JSONResponse(status_code=404, ...)

    # 2. 内存中查找设备对象，检查 WebSocket 在线
    device = esp32IdDict.get(id)
    if device is None or not _is_device_ws_alive(device):
        return JSONResponse(status_code=503, ...)

    # 3. 发送指令并等待响应（最多 5 秒）
    result = await device.send_and_wait(CMD_VERSION, "version", timeout=5.0)
    if result is None:
        return JSONResponse(status_code=504, ...)  # 超时

    # 4. 返回版本号
    return CommonOut(data={"key": "version", "values": result.get("values", "")})
```

---

### asyncio 并发模型说明

```
事件循环 (单线程)
  │
  ├── 协程A: HTTP 请求处理 (get_device_version)
  │     await send_and_wait() ──挂起──┐
  │                                    │ (控制权还给事件循环)
  ├── 协程B: 设备 WebSocket 消息循环    │
  │     await websocket.receive()      │
  │     收到 {"key":"version",...}      │
  │     device.resolve_pending() ──────┘ (set_result 唤醒协程A)
  │                                    
  ├── 协程A: 被唤醒，拿到 result，返回 HTTP 响应
```

核心原理：FastAPI 基于 uvicorn/asyncio 事件循环，所有协程在**同一线程的同一事件循环**中交替执行。当协程 A `await future` 挂起后，事件循环可以调度协程 B（WebSocket 消息循环）运行。协程 B 调用 `set_result()` 后，事件循环在下一轮会唤醒协程 A 继续执行。这就是为什么整个过程无需多线程也能实现"等待远端响应"。