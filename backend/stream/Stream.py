from fastapi import APIRouter, WebSocket, WebSocketException
import json

from stream import Device, Viewer

router = APIRouter(prefix="/stream", tags=["stream"])

@router.websocket("/device/ws")
async def websocket_endpoint(websocket: WebSocket, id: str):
  if id not in Device.esp32IdDict:
      raise WebSocketException(code=1008, reason="Device ID not found")
  await websocket.accept()
  device = Device.esp32IdDict[id]
  device.connected(websocket)

  while True:
    data = await websocket.receive()
    if "text" in data:
      print(data["text"])
      # try:
      #   json_data = json.loads(data["text"])
      # except json.JSONDecodeError:
      #   await websocket.send_text("Invalid JSON received")
      #   continue
      # print(json_data)
      await websocket.send_text("Hello ESP32! from Server.")

    if "bytes" in data:
      size = len(data["bytes"])
      print(f"Received {size} bytes")
      for subscriber in device.subsrcibers:
        if subscriber.websocket == None:
          print(f"用户还未连接: {subscriber.id}")
          continue
        await subscriber.websocket.send_bytes(data["bytes"])

@router.websocket("/viewer/ws")
async def websocket_endpoint(websocket: WebSocket, id: str):
  if id not in Viewer.viewerIdDict:
      raise WebSocketException(code=1008, reason="Viewer ID not found")
  await websocket.accept()
  viewer = Viewer.viewerIdDict[id]
  viewer.connect(websocket)

  while True:
    data = await websocket.receive()
    if "text" in data:
      try:
        json_data = json.loads(data["text"])
      except json.JSONDecodeError:
        await websocket.send_text("Invalid JSON received")
        continue
      print(json_data)
    
