from fastapi import APIRouter
from typing import Annotated, List
from fastapi import status, Path, Query, Depends, Body, WebSocket
from fastapi.responses import JSONResponse
from starlette.websockets import WebSocketState
from Logset import async_log, logger

import asyncio
import json
import Db
import Security
from schema import (
    DeviceFilter,
    DeviceItem,
    DeviceUpdateItem,
    CommonOut,
    R404_DEVICE_NOT_FOUND,
    R400_DEVICE_ALREADY_EXIST,
    R403_FORBIDDEN,
)

router = APIRouter(prefix="/devices", tags=["Devices"])

esp32IdDict = {}

# ──────────────────── 设备指令常量 ────────────────────
CMD_RESTART  = {"code": 1, "item": "device", "key": "restart", "values": ""}
CMD_OTA      = {"code": 1, "item": "device", "key": "ota", "values": ""}
CMD_VERSION  = {"code": 0, "item": "device", "key": "version", "values": ""}
CMD_FEEDING  = {"code": 1, "item": "servo", "key": "feeding", "values": ""}

class Esp32:
    def __init__(self, device:Db.M_Devices):
        self.id = device.id
        self.websocket = None
        esp32IdDict[self.id] = self
        self.subscribers = []
        self.pending_responses: dict[str, asyncio.Future] = {}

    # def __delete__(self, instance):

    def connected(self, websocket: WebSocket):
        esp32IdDict[self.id] = self
        self.websocket = websocket
        with Db.OpenDb("Device Connected") as db:
            Db.UpdateDevice(db, id=self.id, isOnline=True, status="standby")

    def disconnected(self):
        self.websocket = None
        # 只有字典中还是自己时才删除和更新状态，避免旧连接断开时误清新连接
        if esp32IdDict.get(self.id) is self:
            esp32IdDict.pop(self.id, None)
            with Db.OpenDb("Device Disconnected") as db:
                Db.UpdateDevice(db, id=self.id, isOnline=False, status="none")
        else:
            logger.warning(f"设备{self.id} 旧连接断开, 新连接仍在线, 跳过状态更新")

    def subscribe(self, subscriber):
        self.subscribers.append(subscriber)

    def unsubscribe(self, subscriber):
        self.subscribers.remove(subscriber)

    async def send_and_wait(self, command: dict, key: str, timeout: float = 5.0) -> dict | None:
        """发送指令并等待设备返回匹配 key 的响应。超时返回 None。"""
        loop = asyncio.get_event_loop()
        future = loop.create_future()
        self.pending_responses[key] = future
        try:
            await self.websocket.send_json(command)
            return await asyncio.wait_for(future, timeout=timeout)
        except asyncio.TimeoutError:
            return None
        finally:
            self.pending_responses.pop(key, None)

    def resolve_pending(self, key: str, data: dict) -> bool:
        """解析并完成一个挂起的 Future。返回是否成功匹配。"""
        future = self.pending_responses.get(key)
        if future and not future.done():
            future.set_result(data)
            return True
        return False


@router.get(
    "",
    response_model=CommonOut[List[Db.DeviceOut]],
    responses=R404_DEVICE_NOT_FOUND,
    summary="查询设备列表",
)
async def get_devices(
    filter_query: Annotated[DeviceFilter, Query()],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("GetDevices")),
):
    """
    # 查询符合条件的设备列表
    
    结果为空返回404
    """
    data = Db.GetDevices(db, **filter_query.model_dump(exclude_unset=True))
    if not data:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Device not found", data=None).model_dump(),
        )
    return CommonOut(data=data)


@router.get(
    "/{id}",
    response_model=CommonOut[List[Db.DeviceOut]],
    responses=R404_DEVICE_NOT_FOUND,
    summary="根据ID查询单个设备",
)
async def get_device(
    id: Annotated[int, Path(title="设备id", description="数据库设备唯一主键id")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("GetDevice")),
):
    """
    # 根据ID获取单个设备
    
    结果为空返回404
    """
    data = Db.GetDevices(db, id=id)
    if not data:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Device not found", data=None).model_dump(),
        )
    return CommonOut(data=data)


@router.post(
    "",
    response_model=CommonOut[Db.DeviceOut],
    responses={**R400_DEVICE_ALREADY_EXIST, **R403_FORBIDDEN},
    summary="注册新设备",
)
async def register_device(
    body: Annotated[DeviceItem, Body()],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("RegisterDevice")),
):
    """
    # 注册新设备（设备密钥唯一）
    
    同一个鸟笼（区域+编号）中，每种设备类型（ESP32-CAM / ESP32-C3）最多只能存在一台。
    
    ## 权限要求
    仅 root 用户可操作
    """
    # 检查权限
    if current_user.role != "root":
        return JSONResponse(
            status_code=403,
            content=CommonOut(
                code=403, msg="Permission denied.", data=None
            ).model_dump(),
        )
    
    # 检查密钥唯一性
    s = Db.GetDevices(db, secret=body.secret)
    if s:
        return JSONResponse(
            status_code=400,
            content=CommonOut(
                code=400, msg="Device secret already exist.", data=None
            ).model_dump(),
        )
    
    # 检查鸟笼约束：同一区域+编号下，每种设备类型最多一台
    constraint_err = Db.ValidateBirdcageConstraint(
        db, body.area, body.number, body.device_type
    )
    if constraint_err:
        return JSONResponse(
            status_code=400,
            content=CommonOut(code=400, msg=constraint_err, data=None).model_dump(),
        )
    
    new_device = Db.RegisterDevice(db, **body.model_dump(exclude_unset=True))
    return CommonOut(data=new_device)


@router.put(
    "/{id}",
    response_model=CommonOut[Db.DeviceOut],
    responses={**R404_DEVICE_NOT_FOUND, **R403_FORBIDDEN},
    summary="更新设备信息",
)
async def update_device(
    id: Annotated[int, Path(title="设备id", description="数据库设备唯一主键id")],
    body: Annotated[DeviceUpdateItem, Body()],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("UpdateDevice")),
):
    """
    # 使用唯一id更新设备信息
    
    不允许更新设备密钥 返回更新后的设备对象; 若设备不存在则返回404
    ## 字段更新规则：
    - 字符串字段：若传入 NULL 则不更新；若传入空字符串则置为 NULL; 否则更新为新值。
    - 布尔/整数字段：仅当传入非 NULL 时更新
    
    ## 权限要求
    仅 root 用户可操作
    """
    # 检查权限
    if current_user.role != "root":
        return JSONResponse(
            status_code=403,
            content=CommonOut(
                code=403, msg="Permission denied.", data=None
            ).model_dump(),
        )
    
    # 检查设备是否存在
    existing = db.query(Db.M_Devices).filter(Db.M_Devices.id == id).first()
    if not existing:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Device not found", data=None).model_dump(),
        )
    
    # 如果更新了 area/number/device_type，检查鸟笼约束
    new_area = body.area if body.area is not None else existing.area
    new_number = body.number if body.number is not None else existing.number
    new_type = body.device_type if body.device_type is not None else existing.device_type
    
    if (body.area is not None or body.number is not None or body.device_type is not None):
        constraint_err = Db.ValidateBirdcageConstraint(
            db, new_area, new_number, new_type, exclude_device_id=id
        )
        if constraint_err:
            return JSONResponse(
                status_code=400,
                content=CommonOut(code=400, msg=constraint_err, data=None).model_dump(),
            )
    
    updated = Db.UpdateDevice(db, id=id, **body.model_dump(exclude_unset=True))
    if not updated:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Device not found", data=None).model_dump(),
        )
    return CommonOut(data=updated)


@router.delete(
    "/{id}",
    response_model=CommonOut[None],
    responses={**R404_DEVICE_NOT_FOUND, **R403_FORBIDDEN},
    summary="删除设备",
)
async def delete_device(
    id: Annotated[int, Path(title="设备id", description="数据库设备唯一主键id")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("DeleteDevice")),
):
    """
    # 使用唯一id删除设备（仅 root 可操作）
    """
    if current_user.role != "root":
        return JSONResponse(
            status_code=403,
            content=CommonOut(
                code=403, msg="Permission denied.", data=None
            ).model_dump(),
        )
    result = Db.DeleteDevice(db, id=id)
    if not result:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Device not found", data=None).model_dump(),
        )
    return CommonOut(data=None)


@router.get(
    "/birdcage-groups",
    response_model=CommonOut[List[dict]],
    summary="获取鸟笼设备分组",
)
async def get_birdcage_groups(
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("GetBirdcageGroups")),
):
    groups = Db.GetBirdcageGroups(db)
    return CommonOut(data=groups)


# ──────────────────── 设备 WebSocket 指令工具 ────────────────────

def _is_device_ws_alive(device: "Esp32") -> bool:
    """检查设备的 WebSocket 是否仍处于连接状态。"""
    return bool(device.websocket and device.websocket.client_state == WebSocketState.CONNECTED)


async def _send_device_command(device_id: int, command: dict) -> CommonOut:
    """
    向在线设备发送 WebSocket 指令并等待响应。

    Returns:
        CommonOut 封装的响应
    """
    device = esp32IdDict.get(device_id)
    if device is None or not _is_device_ws_alive(device):
        return None, JSONResponse(
            status_code=503,
            content=CommonOut(code=503, msg="Device is not online", data=None).model_dump(),
        )

    try:
        await device.websocket.send_json(command)
    except Exception as e:
        await async_log(logger, "error", f"发送指令到设备{device_id}失败: {e}")
        return None, JSONResponse(
            status_code=502,
            content=CommonOut(code=502, msg=f"Failed to send command: {e}", data=None).model_dump(),
        )

    return device, None


@router.post(
    "/{id}/restart",
    response_model=CommonOut[dict],
    responses={**R404_DEVICE_NOT_FOUND},
    summary="重启设备",
)
async def restart_device(
    id: Annotated[int, Path(title="设备id", description="数据库设备唯一主键id")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("RestartDevice")),
):
    """
    # 通过 WebSocket 向设备发送重启指令

    设备收到指令后约 500ms 执行重启，WebSocket 连接将断开。
    """
    # 检查设备是否存在
    data = Db.GetDevices(db, id=id)
    if not data:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Device not found", data=None).model_dump(),
        )

    device, err = await _send_device_command(id, CMD_RESTART)
    if err:
        return err

    await async_log(logger, "info", f"已向设备{id}发送重启指令")
    return CommonOut(msg="Restart command sent.", data={"key": "restart", "values": "ok"})


@router.post(
    "/{id}/ota",
    response_model=CommonOut[dict],
    responses={**R404_DEVICE_NOT_FOUND},
    summary="设备OTA更新",
)
async def ota_device(
    id: Annotated[int, Path(title="设备id", description="数据库设备唯一主键id")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("OtaDevice")),
):
    """
    # 通过 WebSocket 向设备发送 OTA 固件更新指令

    设备会自动从服务器检查新版本并下载。如果有新版本，下载完成后设备将自动重启。
    OTA 过程中摄像头会被临时关闭以避免 SPI 总线冲突。
    """
    # 检查设备是否存在
    data = Db.GetDevices(db, id=id)
    if not data:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Device not found", data=None).model_dump(),
        )

    device, err = await _send_device_command(id, CMD_OTA)
    if err:
        return err

    await async_log(logger, "info", f"已向设备{id}发送OTA更新指令")
    return CommonOut(msg="OTA command sent.", data={"key": "ota", "values": "ok"})


@router.get(
    "/{id}/version",
    response_model=CommonOut[dict],
    responses={**R404_DEVICE_NOT_FOUND},
    summary="查询设备固件版本",
)
async def get_device_version(
    id: Annotated[int, Path(title="设备id", description="数据库设备唯一主键id")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("GetDeviceVersion")),
):
    """
    # 通过 WebSocket 查询设备当前固件版本号

    向设备发送版本查询指令并等待响应（超时 5 秒）。
    """
    # 检查设备是否存在
    data = Db.GetDevices(db, id=id)
    if not data:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Device not found", data=None).model_dump(),
        )

    device = esp32IdDict.get(id)
    if device is None or not _is_device_ws_alive(device):
        return JSONResponse(
            status_code=503,
            content=CommonOut(code=503, msg="Device is not online", data=None).model_dump(),
        )

    result = await device.send_and_wait(CMD_VERSION, "version", timeout=5.0)
    if result is None:
        return JSONResponse(
            status_code=504,
            content=CommonOut(code=504, msg="Device response timed out", data=None).model_dump(),
        )

    await async_log(logger, "info", f"设备{id}固件版本: {result.get('values')}")
    return CommonOut(data={"key": "version", "values": result.get("values", "")})
