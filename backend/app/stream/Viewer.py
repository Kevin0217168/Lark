import uuid
from fastapi import WebSocket, APIRouter, WebSocketException, Response
from fastapi import status, Path, Query, Depends, Body
from typing import Annotated

from deviceapi import Device
from schema import CommonOut
import Security
import Db

router = APIRouter(prefix="/viewer")

# TODO: 回收机制
viewerIdDict = {}


class Viewer:
    def __init__(self, user: Db.M_Users):
        self.id = user.id
        self.websocket = None
        viewerIdDict[self.id] = self
        self.subscribed_device = []

    def unregister(self):
        viewerIdDict.pop(self.id, None)
        del self

    def connect(self, websocket: WebSocket):
        self.websocket = websocket

    def disconnect(self):
        self.websocket = None

    def subscribe(self, device):
        self.subscribed_device.append(device)
        device.subscribe(self)

    def unsubscribe(self, device):
        self.subscribed_device.remove(device)
        device.unsubscribe(self)


@router.post("", response_model=CommonOut[None])
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


@router.delete("", response_model=CommonOut[None])
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
    
    device = data.pop()
    viewer = viewerIdDict[op.id]
    viewer.subscribe(device)
    return CommonOut(code=200, msg="viewer unregister OK.")

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
    
    device = data.pop()
    viewer = viewerIdDict[op.id]
    viewer.unsubscribe(device)
    return CommonOut(code=200, msg="viewer unregister OK.")
