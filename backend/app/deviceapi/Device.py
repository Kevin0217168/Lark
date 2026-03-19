from fastapi import APIRouter
from typing import Annotated, List
from fastapi import status, Path, Query, Depends, Body, WebSocket
from fastapi.responses import JSONResponse

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

class Esp32:
    def __init__(self, device:Db.M_Devices):
        self.id = device.id
        self.websocket = None
        esp32IdDict[self.id] = self

    # def __delete__(self, instance):

    def connected(self, db:Db.Session, websocket: WebSocket):
        esp32IdDict[self.id] = self
        self.websocket = websocket
        Db.UpdateDevice(db, id=self.id, isOnline=True, status="stream")

    def disconnected(self):
        self.websocket = None
        esp32IdDict.pop(key=self.id)
        del self

    def subscribe(self, subscriber):
        self.subscribers.append(subscriber)

    def unsubscribe(self, subscriber):
        self.subscribers.remove(subscriber)


@router.get(
    "",
    response_model=CommonOut[List[Db.DeviceOut]],
    responses=R404_DEVICE_NOT_FOUND,
)
async def get_devices(
    filter_query: Annotated[DeviceFilter, Query()],
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
)
async def get_device(
    id: Annotated[int, Path(title="设备id", description="数据库设备唯一主键id")],
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
    responses=R400_DEVICE_ALREADY_EXIST,
)
async def register_device(
    body: Annotated[DeviceItem, Body()],
    db: Db.Session = Depends(Db.GetDb("RegisterDevice")),
):
    """
    # 注册新设备（设备密钥唯一）
    
    区域+编号不可出现重复
    """
    # 检查密钥和区域+编号
    s = Db.GetDevices(db, secret=body.secret)
    an = Db.GetDevices(db, area=body.area, number=body.number)
    if s or an:
        return JSONResponse(
            status_code=400,
            content=CommonOut(
                code=400, msg="Device already exist.", data=None
            ).model_dump(),
        )
    new_device = Db.RegisterDevice(db, **body.model_dump(exclude_unset=True))
    return CommonOut(data=new_device)


@router.put(
    "/{id}",
    response_model=CommonOut[Db.DeviceOut],
    responses=R404_DEVICE_NOT_FOUND,
)
async def update_device(
    id: Annotated[int, Path(title="设备id", description="数据库设备唯一主键id")],
    body: Annotated[DeviceUpdateItem, Body()],
    db: Db.Session = Depends(Db.GetDb("UpdateDevice")),
):
    """
    # 使用唯一id更新设备信息
    
    不允许更新设备密钥 返回更新后的设备对象; 若设备不存在则返回404
    ## 字段更新规则：
    - 字符串字段：若传入 NULL 则不更新；若传入空字符串则置为 NULL; 否则更新为新值。
    - 布尔/整数字段：仅当传入非 NULL 时更新
    """
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
