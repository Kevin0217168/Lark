from __future__ import annotations

from fastapi import APIRouter, WebSocket, WebSocketException, WebSocketDisconnect
import json

from stream import Viewer
from deviceapi import Device
from Logset import async_log, logger
import Security
import Db

router = APIRouter(prefix="/stream", tags=["Stream"])
router.include_router(Viewer.router)

# ──────────────────── 设备指令常量 ────────────────────

CMD_STANDBY = {"code": 1, "item": "status", "key": "status", "values": "standby"}
CMD_STREAM  = {"code": 1, "item": "status", "key": "status", "values": "stream"}

# ──────────────────── 消息转发工具 ────────────────────

async def _forward_text(device: Device.Esp32, json_data: dict) -> None:
    """将文本 JSON 消息转发给设备的所有订阅者。"""
    for subscriber in list(device.subscribers):
        try:
            await async_log(logger, "info", f"文本信息转发: 设备{device.id} -> 用户{subscriber.id}")
            await subscriber.websocket.send_json(json_data)
        except Exception as e:
            await async_log(logger, "warning", f"转发文本到用户{subscriber.id}失败: {e}")
    await async_log(logger, "info", "转发结束\n")


async def _forward_bytes(device: Device.Esp32, payload: bytes) -> None:
    """将二进制帧转发给设备的所有订阅者。"""
    for subscriber in list(device.subscribers):
        try:
            await async_log(logger, "info", f"二进制信息转发: 设备{device.id} -> 用户{subscriber.id}")
            await subscriber.websocket.send_bytes(payload)
        except Exception as e:
            await async_log(logger, "warning", f"转发二进制到用户{subscriber.id}失败: {e}")
    await async_log(logger, "info", "转发结束\n")


# ──────────────────── 设备文本消息处理 ────────────────────

async def _handle_device_status_response(device: Device.Esp32, json_data: dict) -> None:
    """处理设备返回的状态响应（stream / standby）。"""
    if not json_data or json_data.get("key") != "status":
        return
    value = json_data.get("values")
    if value == "stream":
        with Db.OpenDb("设备响应, 设置直播状态") as db:
            Db.UpdateDevice(db, id=device.id, status="stream")
    elif value == "standby":
        with Db.OpenDb("设备响应, 设置待机状态") as db:
            Db.UpdateDevice(db, id=device.id, status="standby")


async def _on_device_text(device: Device.Esp32, raw: str) -> None:
    """解析设备文本消息 → 处理状态响应 → 转发给订阅者。"""
    try:
        json_data = json.loads(raw)
    except json.JSONDecodeError:
        await async_log(logger, "warning", f"解码失败, 来自设备{device.id}")
        await async_log(logger, "warning", f"原始信息: {raw}")
        return

    await async_log(logger, "info", f"收到文本信息, 来自设备{device.id} : {json_data}")

    try:
        await _handle_device_status_response(device, json_data)
    except Exception as e:
        await async_log(logger, "warning", f"操作响应数据错误: {e}")
        return

    await _forward_text(device, json_data)


async def _on_device_bytes(device: Device.Esp32, websocket: WebSocket, payload: bytes) -> None:
    """处理设备二进制帧：无订阅者则通知设备待机，否则转发。"""
    size = len(payload)
    await async_log(logger, "info", f"来自设备{device.id} Received {size} bytes")

    if not device.subscribers:
        await async_log(logger, "info", "设备观看者为0")
        await websocket.send_json(CMD_STANDBY)
    else:
        await _forward_bytes(device, payload)


# ──────────────────── 连接初始化工具 ────────────────────

async def _close_old_device_connection(device_id: int) -> None:
    """如果设备已有旧 WebSocket 连接，安全关闭它。"""
    old = Device.esp32IdDict.get(device_id)
    if old and old.websocket:
        try:
            await old.websocket.close()
            await async_log(logger, "warning", f"设备{device_id} 存在旧连接, 已关闭")
        except Exception:
            pass


async def _close_old_viewer_connection(user_id: int) -> None:
    """如果观看者已有旧 WebSocket 连接（旧模式），安全关闭它。"""
    old = Viewer.viewerIdDict.pop(user_id, None)
    if old and old.websocket:
        await old.websocket.close()
        await async_log(logger, "warning", "存在已有连接, 关闭旧连接")


async def _close_old_viewer_device_connection(user_id: int, device_id: int) -> None:
    """如果同一 (user, device) 已有旧 WebSocket 连接（新模式），安全关闭它。"""
    key = (user_id, device_id)
    old = Viewer.viewerWsDict.get(key)
    if old and old.websocket:
        try:
            await old.websocket.close()
            await async_log(logger, "warning", f"用户{user_id} 对设备{device_id} 存在旧连接, 已关闭")
        except Exception:
            pass
        old.unregister()


# ──────────────────── WebSocket 端点 ────────────────────

@router.websocket("/device/ws")
async def device_websocket_endpoint(websocket: WebSocket, secret: str):
    # ── 认证 ──
    with Db.OpenDb("device websocket_endpoint") as db:
        devices = Db.GetDevices(db, secret=secret)
    if not devices:
        raise WebSocketException(code=1008, reason="Device ID not found")
    device_data = devices.pop()

    # ── 处理旧连接 & 创建设备对象 ──
    await _close_old_device_connection(device_data.id)
    device = Device.Esp32(device_data)

    # ── 建立连接 ──
    await websocket.accept()
    await websocket.send_text("Hello ESP32! from Server.")
    device.connected(websocket)
    await websocket.send_json(CMD_STANDBY)

    # ── 消息循环 ──
    try:
        while True:
            data = await websocket.receive()
            if "text" in data:
                await _on_device_text(device, data["text"])
            if "bytes" in data:
                await _on_device_bytes(device, websocket, data["bytes"])
    except (WebSocketDisconnect, RuntimeError) as e:
        await async_log(logger, "info", "设备已断开: ", e)
        for subscriber in list(device.subscribers):
            if subscriber.websocket is None:
                await async_log(logger, "info", f"用户还未连接: {subscriber.id}")
                continue
            try:
                await subscriber.websocket.send_text("设备已断开")
                # 新模式观看者：设备断开时同时关闭其 WebSocket
                if subscriber.ws_per_device:
                    await subscriber.websocket.close()
            except Exception:
                pass
        device.disconnected()


@router.websocket("/viewer/ws")
async def viewer_websocket_endpoint(websocket: WebSocket, token: str):
    """旧模式观看者 WebSocket（已弃用）：一个连接对应多个设备，需配合 HTTP 端点订阅。"""
    # ── 认证 ──
    with Db.OpenDb("viewer websocket_endpoint") as db:
        user = Security.VerifyToken(db, token, False)

    # ── 处理旧连接 & 创建观看者对象 ──
    await _close_old_viewer_connection(user.id)
    viewer = Viewer.Viewer(user)

    # ── 建立连接 ──
    await websocket.accept()
    viewer.connect(websocket)

    # ── 消息循环 ──
    try:
        while True:
            data = await websocket.receive()
            if "text" not in data:
                continue
            try:
                json_data = json.loads(data["text"])
            except json.JSONDecodeError:
                await websocket.send_text("Invalid JSON received")
                continue
            await async_log(logger, "info", f"来自用户{viewer.id} 的信息: {json_data}")
            for d in viewer.subscribed_devices:
                if d.websocket:
                    await async_log(logger, "info", f"信息转发: 用户{viewer.id} -> 转发到设备{d.id}")
                    await d.websocket.send_text(data["text"])
            await async_log(logger, "info", "转发结束\n")
    except (WebSocketDisconnect, RuntimeError):
        viewer.disconnect()
        viewer.unregister()


@router.websocket("/viewer/ws/{device_id}")
async def viewer_device_websocket_endpoint(websocket: WebSocket, device_id: int, token: str):
    """新模式观看者 WebSocket：一个连接对应一个设备，自动处理订阅关系。"""
    # ── 认证 ──
    with Db.OpenDb("viewer_device_ws") as db:
        user = Security.VerifyToken(db, token, False)

    # ── 验证设备存在 ──
    with Db.OpenDb("viewer_device_ws_check") as db:
        if not Db.GetDevices(db, id=device_id):
            raise WebSocketException(code=1008, reason="Device not found")

    device = Device.esp32IdDict.get(device_id)
    if device is None:
        raise WebSocketException(code=1008, reason="Device not connected")

    # ── 处理同一 (user, device) 的旧连接 ──
    await _close_old_viewer_device_connection(user.id, device_id)

    # ── 创建观看者对象 ──
    viewer = Viewer.Viewer(user, ws_per_device=True, device_id=device_id)

    # ── 建立连接 ──
    await websocket.accept()
    viewer.connect(websocket)

    # ── 自动订阅 ──
    if not await viewer.subscribe(device):
        await websocket.send_json({"code": 400, "msg": "Failed to subscribe: device not reachable"})
        await websocket.close()
        viewer.unregister()
        return

    await websocket.send_json({"code": 200, "msg": f"Subscribed to device {device_id}"})

    # ── 消息循环：转发用户消息到设备 ──
    try:
        while True:
            data = await websocket.receive()
            if "text" not in data:
                continue
            try:
                json_data = json.loads(data["text"])
            except json.JSONDecodeError:
                await websocket.send_text("Invalid JSON received")
                continue
            await async_log(logger, "info", f"来自用户{viewer.id} 的信息(设备{device_id}): {json_data}")
            if device.websocket:
                await async_log(logger, "info", f"信息转发: 用户{viewer.id} -> 设备{device.id}")
                await device.websocket.send_text(data["text"])
            await async_log(logger, "info", "转发结束\n")
    except (WebSocketDisconnect, RuntimeError):
        viewer.disconnect()
        viewer.unregister()
