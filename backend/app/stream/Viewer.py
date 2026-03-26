from __future__ import annotations

from fastapi import WebSocket, APIRouter, Response, Depends
from typing import Annotated, Optional
from starlette.websockets import WebSocketState

from deviceapi import Device
from schema import CommonOut
from Logset import async_log, logger
import Security
import Db

router = APIRouter(prefix="/viewer")

# ──────────────────── 全局观看者注册表 ────────────────────

# 旧模式（已弃用）：一个用户对应一个 Viewer，通过 HTTP 端点手动管理
viewerIdDict: dict[int, "Viewer"] = {}

# 新模式：(user_id, device_id) 对应一个 Viewer，通过 WebSocket 连接自动管理
viewerWsDict: dict[tuple[int, int], "Viewer"] = {}

# ──────────────────── 设备指令常量 ────────────────────
CMD_STREAM = {"code": 1, "item": "status", "key": "status", "values": "stream"}


# ──────────────────── Viewer 类 ────────────────────
class Viewer:
    """代表一个正在观看设备推流的用户，管理 WebSocket 连接与设备订阅关系。

    支持两种模式：
    - 旧模式 (ws_per_device=False): 一个用户一个 Viewer，可订阅多个设备（已弃用）
    - 新模式 (ws_per_device=True):  每个 (用户, 设备) 对应一个 Viewer，WebSocket 自动管理订阅
    """

    def __init__(
        self,
        user: Db.M_Users,
        *,
        ws_per_device: bool = False,
        device_id: Optional[int] = None,
    ):
        self.id: int = user.id
        self.websocket: Optional[WebSocket] = None
        self.subscribed_devices: list[Device.Esp32] = []
        self.ws_per_device: bool = ws_per_device
        self.device_id: Optional[int] = device_id

        # 注册到对应的全局字典
        if ws_per_device and device_id is not None:
            viewerWsDict[(self.id, device_id)] = self
        else:
            viewerIdDict[self.id] = self

    # ── 生命周期 ──

    def connect(self, websocket: WebSocket) -> None:
        self.websocket = websocket

    def disconnect(self) -> None:
        self.websocket = None

    def unregister(self) -> None:
        """从全局注册表移除自身，并取消所有设备订阅。"""
        if self.ws_per_device and self.device_id is not None:
            key = (self.id, self.device_id)
            if viewerWsDict.get(key) is self:
                viewerWsDict.pop(key)
        else:
            if viewerIdDict.get(self.id) is self:
                viewerIdDict.pop(self.id)
        for device in self.subscribed_devices:
            if self in device.subscribers:
                device.subscribers.remove(self)

    # ── 订阅管理 ──

    async def subscribe(self, device: Device.Esp32) -> bool:
        """
        订阅设备推流。当设备无订阅者时，通知设备切换到 stream 模式。

        Returns:
            True  — 订阅成功
            False — 设备不可达
        """
        await async_log(logger, "info", device.subscribers)

        # 首个订阅者需要唤醒设备
        if len(device.subscribers) == 0:
            if not _is_device_ws_alive(device):
                return False
            try:
                await device.websocket.send_json(CMD_STREAM)
            except Exception:
                await async_log(logger, "error", f"Device {device.id} WebSocket closed unexpectedly")
                return False

        device.subscribe(self)
        self.subscribed_devices.append(device)
        return True

    def unsubscribe(self, device: Device.Esp32) -> None:
        self.subscribed_devices.remove(device)
        device.unsubscribe(self)


# ──────────────────── 内部工具函数 ────────────────────

def _is_device_ws_alive(device: Device.Esp32) -> bool:
    """检查设备的 WebSocket 是否仍处于连接状态。"""
    return bool(device.websocket and device.websocket.client_state == WebSocketState.CONNECTED)


def _resolve_viewer_and_device(
    db: Db.Session, device_id: int, user_id: int, response: Response
) -> tuple[Optional["Viewer"], Optional[Device.Esp32], Optional[CommonOut]]:
    """
    公共校验逻辑：验证设备存在 → 观看者已注册 → 设备在线。

    Returns:
        (viewer, device, None)      — 校验通过
        (None,   None,   CommonOut) — 校验失败，直接返回错误响应
    """
    if not Db.GetDevices(db, id=device_id):
        response.status_code = 404
        return None, None, CommonOut(code=404, msg="Device ID not found.")

    viewer = viewerIdDict.get(user_id)
    if viewer is None:
        response.status_code = 404
        return None, None, CommonOut(code=404, msg="Viewer has not registered.")

    device = Device.esp32IdDict.get(device_id)
    if device is None:
        response.status_code = 400
        return None, None, CommonOut(code=400, msg="Device has not connected.")

    return viewer, device, None


# ──────────────────── HTTP 端点 ────────────────────

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


@router.post("/following/{device_id}", response_model=CommonOut[None], deprecated=True)
async def subscribe_to_device(
    device_id: Annotated[int, "Device ID"],
    response: Response,
    op: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("subscribe_to_device")),
):
    """# 订阅设备推流"""
    viewer, device, err = _resolve_viewer_and_device(db, device_id, op.id, response)
    if err:
        return err

    if device in viewer.subscribed_devices:
        response.status_code = 400
        return CommonOut(code=400, msg="Device has subscribed.")

    if not await viewer.subscribe(device):
        return CommonOut(code=400, msg="Device has not connected.")
    return CommonOut(code=200, msg="device subscribe OK.")


@router.delete("/following/{device_id}", response_model=CommonOut[None], deprecated=True)
async def unsubscribe_to_device(
    device_id: Annotated[int, "Device ID"],
    response: Response,
    op: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("unsubscribe_to_device")),
):
    """# 取消订阅设备"""
    viewer, device, err = _resolve_viewer_and_device(db, device_id, op.id, response)
    if err:
        return err

    if device not in viewer.subscribed_devices:
        response.status_code = 404
        return CommonOut(code=404, msg="Device has not subscribed.")

    viewer.unsubscribe(device)
    return CommonOut(code=200, msg="device unsubscribe OK.")
