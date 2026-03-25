import uuid
from fastapi import WebSocket, APIRouter, WebSocketException, Response
from fastapi import status, Path, Query, Depends, Body
from typing import Annotated
from starlette.websockets import WebSocketState

from deviceapi import Device
from schema import CommonOut
import json
import Security
import Db

router = APIRouter(prefix="/viewer")

from Logset import async_log, logger

viewerIdDict = {}


class Viewer:
    def __init__(self, user: Db.M_Users):
        self.id = user.id
        self.websocket = None
        viewerIdDict[self.id] = self
        self.subscribed_device = []

    def unregister(self):
        # 只有字典中还是自己时才删除
        if viewerIdDict.get(self.id) is self:
            viewerIdDict.pop(self.id)
        # 在对应设备上取消订阅
        for i in self.subscribed_device:
            i.subscribers.pop()

    def connect(self, websocket: WebSocket):
        self.websocket = websocket

    def disconnect(self):
        self.websocket = None

    async def subscribe(self, device: Device.Esp32):
        await async_log(logger, "info", device.subscribers)
        if len(device.subscribers) == 0:
            # 确保设备在线且 WebSocket 可用
            if not device.websocket or device.websocket.client_state != WebSocketState.CONNECTED:
                return -1
            try:
                await device.websocket.send_json({"code":1, "item":"status", "key": "status", "values":"stream"})
            except Exception:
                await async_log(logger, "error", f"Device {device.id} WebSocket closed unexpectedly")
                return -1
            
        # 用于向观看者转发
        device.subscribe(self)
        # 用于注销时取消对应设备的转发
        self.subscribed_device.append(device)
        

    def unsubscribe(self, device):
        self.subscribed_device.remove(device)
        device.unsubscribe(self)


@router.post("", response_model=CommonOut[None], deprecated=True)
async def register_viewer(
    response: Response,
    op: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
):
    """
    # 注册观看者身份

    服务器创建临时内存对象
    """
    if op.id in viewerIdDict:
        response.status_code = 400
        return CommonOut(code=400, msg="Viewer has registered.")
    Viewer(op)
    return CommonOut(code=200, msg="viewer register OK.")


@router.delete("", response_model=CommonOut[None], deprecated=True)
async def unregister_viewer(
    response: Response,
    op: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
):
    """
    # 注销观看者身份

    服务器销毁临时内存对象
    """
    if op.id not in viewerIdDict:
        response.status_code = 400
        return CommonOut(code=400, msg="Viewer has not registered.")
    
    viewerIdDict[op.id].unregister()
    return CommonOut(code=200, msg="viewer unregister OK.")


@router.post("/following/{device_id}", response_model=CommonOut[None])
async def subscribe_to_device(
    device_id: Annotated[int, "Device ID"],
    response: Response,
    op: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("subscribe_to_device")),
):
    """
    # 订阅设备推流
    """
    data = Db.GetDevices(db, id=device_id)
    if len(data) == 0:
        response.status_code = 404
        return CommonOut(code=404, msg="Device ID not found.")
    if op.id not in viewerIdDict:
        response.status_code = 404
        return CommonOut(code=404, msg="Viewer has not registerd.")
    
    viewer = viewerIdDict[op.id]
    
    if device_id not in Device.esp32IdDict:
        response.status_code = 400
        return CommonOut(code=400, msg="Device has not connected.")
    
    device = Device.esp32IdDict[device_id]
    if device in viewer.subscribed_device:
        response.status_code = 400
        return CommonOut(code=400, msg="Device has subscribed.")
    
    
    if await viewer.subscribe(device) == -1:
        return CommonOut(code=400, msg="Device has not connected.")
    return CommonOut(code=200, msg="device subscribe OK.")

@router.delete("/following/{device_id}", response_model=CommonOut[None])
async def unsubscribe_to_device(
    device_id: Annotated[int, "Device ID"],
    response: Response,
    op: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("unsubscribe_to_device")),
):
    """
    # 取消订阅设备
    """
    data = Db.GetDevices(db, id=device_id)
    if len(data) == 0:
        response.status_code = 404
        return CommonOut(code=404, msg="Device ID not found.")
    if op.id not in viewerIdDict:
        response.status_code = 404
        return CommonOut(code=404, msg="Viewer has not registerd.")
    
    viewer = viewerIdDict[op.id]
    
    if device_id not in Device.esp32IdDict:
        response.status_code = 400
        return CommonOut(code=400, msg="Device has not connected.")
    
    
    device = Device.esp32IdDict[device_id]
    if device not in viewer.subscribed_device:
        response.status_code = 404
        return CommonOut(code=404, msg="Device has not subscribed.")
    
    viewer.unsubscribe(device)
    return CommonOut(code=200, msg="device unsubscribe OK.")
