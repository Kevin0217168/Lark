from fastapi import APIRouter, WebSocket, WebSocketException
import json

from stream import device

router = APIRouter(prefix="/stream", tags=["stream"])

@router.websocket("/device/ws?id={id}")
async def websocket_endpoint(websocket: WebSocket, id: str):
  if id not in device.esp32IdDict:
      raise WebSocketException(code=1008, reason="Device ID not found")
  await websocket.accept()
  device = device.esp32IdDict[id]
  device.connected(websocket)

  while True:
    data = await websocket.receive()
    if "text" in data:
      try:
        json_data = json.loads(data["text"])
      except json.JSONDecodeError:
        await websocket.send_text("Invalid JSON received")
        continue
      print(json_data)

    if "bytes" in data:
       for subscriber in device.subsrcibers:
         await subscriber.websocket.send_bytes(data["bytes"])

@router.websocket("/viewer/ws?id={id}")
async def websocket_endpoint(websocket: WebSocket, id: str):
  if id not in viewer.viewerIdDict:
      raise WebSocketException(code=1008, reason="Viewer ID not found")
  await websocket.accept()
  viewer = viewer.viewerIdDict[id]
  viewer.connected(websocket)

  while True:
    data = await websocket.receive()
    if "text" in data:
      try:
        json_data = json.loads(data["text"])
      except json.JSONDecodeError:
        await websocket.send_text("Invalid JSON received")
        continue
      print(json_data)
    
