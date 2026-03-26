from fastapi import APIRouter, WebSocket, WebSocketException, WebSocketDisconnect
from fastapi import status, Path, Query, Depends, Body
import json

from stream import Viewer
from deviceapi import Device
import Security
import Db

from Logset import async_log, logger

router = APIRouter(prefix="/stream", tags=["Stream"])
router.include_router(Viewer.router)


@router.websocket("/device/ws")
async def device_websocket_endpoint(
    websocket: WebSocket,
    secret: str,
):
    with Db.OpenDb("device websocket_endpoint") as db:
        devices = Db.GetDevices(db, secret=secret)
    if len(devices) == 0:
        raise WebSocketException(code=1008, reason="Device ID not found")
    device_data = devices.pop()

    # 移除旧连接（如果存在）
    old_device = Device.esp32IdDict.get(device_data.id)
    if old_device and old_device.websocket:
        try:
            await old_device.websocket.close()
            await async_log(logger, "warning", f"设备{device_data.id} 存在旧连接, 已关闭")
        except Exception:
            pass

    device = Device.Esp32(device_data)

    await websocket.accept()
    await websocket.send_text("Hello ESP32! from Server.")

    # 默认进入standby
    device.connected(websocket)
    await websocket.send_json({"code": 1, "item": "status", "key": "status", "values": "standby"})
    try:
        while True:
            data = await websocket.receive()
            if "text" in data:
                # 试图接收esp32返回的控制信息结果
                try:
                    json_data = json.loads(data["text"])
                except json.JSONDecodeError:
                    # await websocket.send_text("Invalid JSON received")
                    # 直接跳过这次解码环节
                    await async_log(logger, "warning", f"解码失败, 来自设备{device.id}")
                    await async_log(logger, "warning", f"原始信息: {data['text']}")
                    continue
                await async_log(logger, "info", f"收到文本信息, 来自设备{device.id} : {json_data}")
                try:
                    if json_data:
                        # 响应成功
                        if json_data["key"] == "status":
                            # 状态响应
                            if json_data["values"] == "stream":
                                # 切换到stream状态
                                with Db.OpenDb("设备响应, 设置直播状态") as db:
                                    Db.UpdateDevice(db, id=device.id, status="stream")
                            elif json_data["values"] == "standby":
                                # 切换到standby状态
                                with Db.OpenDb("设备响应, 设置待机状态") as db:
                                    Db.UpdateDevice(db, id=device.id, status="standby")
                except Exception as e:
                    await async_log(logger, "warning", f"操作响应数据错误: {e}")
                    continue
                # 给所有订阅者转发消息
                for subscriber in list(device.subscribers):
                    try:
                        await async_log(logger, "info", f"文本信息转发: 设备{device.id} -> 用户{subscriber.id}")
                        await subscriber.websocket.send_json(json_data)
                    except Exception as send_err:
                        await async_log(logger, "warning", f"转发文本到用户{subscriber.id}失败: {send_err}")
                await async_log(logger, "info", f"转发结束\n")

            if "bytes" in data:
                size = len(data["bytes"])
                await async_log(logger, "info", f"来自设备{device.id} Received {size} bytes")
                if len(device.subscribers) == 0:
                    await async_log(logger, "info", "设备观看者为0")
                    # 发送休眠信息
                    await websocket.send_json(
                            {
                                "code": 1,
                                "item": "status",
                                "key": "status",
                                "values": "standby",
                            }
                    )
                else:
                    # 给所有观看者转发信息
                    for subscriber in list(device.subscribers):
                        try:
                            await async_log(logger, "info", f"二进制信息转发: 设备{device.id} -> 用户{subscriber.id}")
                            await subscriber.websocket.send_bytes(data["bytes"])
                        except Exception as send_err:
                            await async_log(logger, "warning", f"转发二进制到用户{subscriber.id}失败: {send_err}")
                    await async_log(logger, "info", f"转发结束\n")
    except (WebSocketDisconnect, RuntimeError) as e:
        await async_log(logger, "info", "设备已断开: ", e)
        for subscriber in device.subscribers:
            if subscriber.websocket is None:
                await async_log(logger, "info", f"用户还未连接: {subscriber.id}")
                continue
            await subscriber.websocket.send_text("设备已断开")
        device.disconnected()


@router.websocket("/viewer/ws")
async def viewer_websocket_endpoint(websocket: WebSocket, token: str):
    # 验证身份
    with Db.OpenDb("viewer websocket_endpoint") as db:
        user = Security.VerifyToken(db, token, False)

    
    # 移除旧对象（如果存在）
    old_viewer = Viewer.viewerIdDict.pop(user.id, None)
    if old_viewer and old_viewer.websocket:
        # 关闭旧连接
        await old_viewer.websocket.close()
        await async_log(logger, "warning", "存在已有连接, 关闭旧连接")
        
    # 自动创建
    viewer = Viewer.Viewer(user)

    await websocket.accept()
    viewer.connect(websocket)

    try:
        while True:
            data = await websocket.receive()
            if "text" in data:
                try:
                    json_data = json.loads(data["text"])
                except json.JSONDecodeError:
                    await websocket.send_text("Invalid JSON received")
                    continue
                await async_log(logger, "info", f"来自用户{viewer.id} 的信息: {json_data}")
                # 直接转发给设备
                for d in viewer.subscribed_device:
                    if d.websocket:
                        await async_log(logger, "info", f"信息转发: 用户{viewer.id} -> 转发到设备{d.id}")
                        await d.websocket.send_text(data["text"])
                await async_log(logger, "info", f"转发结束\n")
    except (WebSocketDisconnect, RuntimeError):
        # 退出连接则自动销毁
        viewer.disconnect()
        viewer.unregister()
