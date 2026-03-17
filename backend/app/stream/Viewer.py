import uuid
from fastapi import WebSocket, APIRouter, WebSocketException
from typing import Annotated

from stream import Device

router = APIRouter(prefix="/viewer", tags=["viewer"])

# TODO: 回收机制
viewerIdDict = {}

class Viewer:
    def __init__(self):
        self.id = str(uuid.uuid4().hex)
        self.websocket = None
        viewerIdDict[self.id] = self
        self.subscribed_device = []
    
    def __delete__(self, instance):
        viewerIdDict.pop(self.id, None)

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

@router.get("/register")
async def register_viewer():
    viewer = Viewer()
    return viewer.id

@router.get("/subscribe")
async def subscribe_to_device(id: Annotated[str, "Viewer ID", ],
                              device_id: Annotated[str, "Device ID"]):
    if id not in viewerIdDict:
        raise WebSocketException(code=1008, reason="Viewer ID not found")
    if device_id not in Device.esp32IdDict:
        raise WebSocketException(code=1008, reason="Device ID not found")
    viewer = viewerIdDict[id]
    device = Device.esp32IdDict[device_id]
    viewer.subscribe(device)
    return {"code": 200, "message": f"Subscribed to device {device_id}"}