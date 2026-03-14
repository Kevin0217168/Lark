import json
import uuid
from fastapi import APIRouter, Response, WebSocket
from pydantic import BaseModel, Field
from typing import Optional, Annotated

router = APIRouter(prefix="/device", tags=["device"])

esp32IdDict = {}

esp32TokenList = [
  "ae2823b8b2e14205b5e75d30c68d282d",
]

class Esp32:
  def __init__(self, name=None):
    self.id = str(uuid.uuid4().hex)
    self.name = name if name else "ESP32 - " + str(len(esp32IdDict) + 1)
    self.status = "offline"
    self.websocket = None
    self.subsrcibers = []
    esp32IdDict[self.id] = self

  def __delete__(self, instance):
        esp32IdDict.pop(self.id, None)

  def connected(self, websocket: WebSocket):
    self.status = "online"
    self.websocket = websocket

  def disconnected(self):
    self.status = "offline"
    self.websocket = None

  def subscribe(self, subscriber):
    self.subsrcibers.append(subscriber)

  def unsubscribe(self, subscriber):
    self.subsrcibers.remove(subscriber)

# 定义请求体模型
class RegisterItem(BaseModel):
  secret: str = Field(..., 
                      description="设备密钥, 须与服务端一致", 
                      examples=["ae2823b8b2e14205b5e75d30c68d282d"])

@router.post("/register")
async def register_device(item: RegisterItem):
  if item.secret not in esp32TokenList:
    return Response(status_code=403, 
                    content=json.dumps({"error": "Invalid token"}), 
                    media_type="application/json")
  device = Esp32()
  return Response(status_code=200, 
                  content=json.dumps({"id": device.id, "name": device.name, "status": device.status}), 
                  media_type="application/json")


@router.get("/list")
async def list_devices():
  devices = []
  for device in esp32IdDict.values():
    devices.append({"id": device.id, "name": device.name, "status": device.status})
  return Response(status_code=200, 
                  content=json.dumps({"devices": devices}), 
                  media_type="application/json")


# 定义请求体模型
class Item(BaseModel):
  id: str = Field(..., description="设备ID")
  name: Optional[str] = Field(None, description="新设备名称")


@router.post("/update")
async def update_device_status(item: Annotated[Item, "设备信息"]):
  if item.id not in esp32IdDict:
    return Response(status_code=404, 
                    content=json.dumps({"error": "Device ID not found"}), 
                    media_type="application/json")
  device = esp32IdDict.get(item.id)
  device.name = item.name if item.name else device.name
  return Response(status_code=200, 
                  content=json.dumps({"id": device.id, "name": device.name, "status": device.status}), 
                  media_type="application/json")
  
