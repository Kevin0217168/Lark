from fastapi import APIRouter, WebSocket, WebSocketException, WebSocketDisconnect
from fastapi import status, Path, Query, Depends, Body
import json

from stream import Viewer
from deviceapi import Device
import Security
import Db

router = APIRouter(prefix="/stream", tags=["Stream"])
router.include_router(Viewer.router)


@router.websocket("/device/ws")
async def websocket_endpoint(
    websocket: WebSocket,
    secret: str,
):
    with Db.OpenDb("device websocket_endpoint") as db:
        devices = Db.GetDevices(db, secret=secret)
    if len(devices) == 0:
        raise WebSocketException(code=1008, reason="Device ID not found")
    device = Device.Esp32(devices.pop())

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
                print(data["text"])
                try:
                    json_data = json.loads(data["text"])
                except json.JSONDecodeError:
                    # await websocket.send_text("Invalid JSON received")
                    # 直接跳过这次解码环节
                    print("解码失败")
                    continue
                print(json_data)
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
                    print(e)
                    continue
                # 给所有订阅者转发消息
                for subscriber in device.subscribers:
                    await subscriber.websocket.send_json(json_data)

            if "bytes" in data:
                size = len(data["bytes"])
                print(f"Received {size} bytes")
                if len(device.subscribers) == 0:
                    print("设备观看者为0")
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
                    for subscriber in device.subscribers:
                        await subscriber.websocket.send_bytes(data["bytes"])
    except (WebSocketDisconnect, RuntimeError) as e:
        print("设备已断开: ", e)
        for subscriber in device.subscribers:
            if subscriber.websocket == None:
                print(f"用户还未连接: {subscriber.id}")
                continue
            await subscriber.websocket.send_text("设备已断开")
        device.disconnected()


@router.websocket("/viewer/ws")
async def websocket_endpoint(websocket: WebSocket, token: str):
    # 验证身份
    with Db.OpenDb("viewer websocket_endpoint") as db:
        user = Security.VerifyToken(db, token, False)

    print(Viewer.viewerIdDict)
    
    # 移除旧对象（如果存在）
    old_viewer = Viewer.viewerIdDict.pop(user.id, None)
    if old_viewer and old_viewer.websocket:
        # 关闭旧连接
        await old_viewer.websocket.close()
        
    # 自动创建
    viewer = Viewer.Viewer(user)
    print(Viewer.viewerIdDict)

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
                print(json_data)
    except (WebSocketDisconnect, RuntimeError):
        # 退出连接则自动销毁
        viewer.disconnect()
        viewer.unregister()
