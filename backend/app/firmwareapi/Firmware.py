from fastapi import APIRouter, UploadFile, File, Form
from typing import Annotated, List
from fastapi import status, Path, Query, Depends, Body
from fastapi.responses import JSONResponse, FileResponse
from Logset import async_log, logger

import os
import Db
import Security
from schema import (
    FirmwareOut,
    FirmwareItem,
    FirmwareSecret,
    CommonOut,
    R404_FIRMWARE_NOT_FOUND,
    R400_FIRMWARE_ALREADY_EXIST,
    R404_DEVICE_NOT_FOUND,
    R403_FORBIDDEN,
)

router = APIRouter(prefix="/firmwares", tags=["Firmwares"])

FIRMWARE_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "static", "firmwares")
# 确保固件存放目录存在
os.makedirs(FIRMWARE_DIR, exist_ok=True)


@router.get(
    "",
    response_model=CommonOut[List[FirmwareOut]],
    responses=R404_FIRMWARE_NOT_FOUND,
    summary="查询固件列表",
)
async def get_firmwares(
    device_type: str = Query(default=None, title="设备类型", description="按设备类型筛选"),
    db: Db.Session = Depends(Db.GetDb("GetFirmwares")),
):
    """
    # 查询固件列表

    可选按设备类型筛选，结果为空返回404
    """
    data = Db.GetFirmwares(db, device_type=device_type)
    if not data:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Firmware not found", data=None).model_dump(),
        )
    return CommonOut(data=data)


@router.post(
    "/upload",
    response_model=CommonOut[FirmwareOut],
    responses={**R400_FIRMWARE_ALREADY_EXIST, **R403_FORBIDDEN},
    summary="上传固件",
)
async def upload_firmware(
    device_type: str = Form(..., title="设备类型", description="固件适用的设备类型"),
    name: str = Form(..., title="固件名称", description="固件名称"),
    version: str = Form(..., title="固件版本", description="固件版本号"),
    file: UploadFile = File(..., description="固件文件"),
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)] = None,
    db: Db.Session = Depends(Db.GetDb("UploadFirmware")),
):
    """
    # 管理员上传固件文件（仅 root 可操作）

    固件文件存放在 static/firmwares 目录下，文件名格式: {设备类型}_{版本号}_{原始文件名}
    """
    if current_user.role != "root":
        return JSONResponse(
            status_code=403,
            content=CommonOut(
                code=403, msg="Permission denied.", data=None
            ).model_dump(),
        )

    # 检查是否已存在相同设备类型+版本的固件
    existing = Db.GetFirmwares(db, device_type=device_type, version=version)
    if existing:
        return JSONResponse(
            status_code=400,
            content=CommonOut(
                code=400, msg="Firmware already exist.", data=None
            ).model_dump(),
        )

    # 保存固件文件
    filename = f"{device_type}_{version}_{file.filename}"
    filepath = os.path.join(FIRMWARE_DIR, filename)
    content = await file.read()
    with open(filepath, "wb") as f:
        f.write(content)

    await async_log(logger, "info", f"固件文件已保存: {filepath}")

    # 存储相对路径到数据库
    relative_path = f"static/firmwares/{filename}"
    new_firmware = Db.CreateFirmware(
        db,
        device_type=device_type,
        name=name,
        version=version,
        filepath=relative_path,
    )
    return CommonOut(data=new_firmware)


@router.delete(
    "/{id}",
    response_model=CommonOut[None],
    responses={**R404_FIRMWARE_NOT_FOUND, **R403_FORBIDDEN},
    summary="删除固件",
)
async def delete_firmware(
    id: Annotated[int, Path(title="固件id", description="数据库固件唯一主键id")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)] = None,
    db: Db.Session = Depends(Db.GetDb("DeleteFirmware")),
):
    """
    # 使用唯一id删除固件（仅 root 可操作）

    同时删除服务器上的固件文件
    """
    if current_user.role != "root":
        return JSONResponse(
            status_code=403,
            content=CommonOut(
                code=403, msg="Permission denied.", data=None
            ).model_dump(),
        )

    # 先查询固件信息以获取文件路径
    firmwares = Db.GetFirmwares(db, id=id)
    if firmwares:
        firmware = firmwares[0]
        # 删除服务器上的固件文件
        abs_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), firmware.filepath)
        if os.path.exists(abs_path):
            os.remove(abs_path)
            await async_log(logger, "info", f"固件文件已删除: {abs_path}")

    result = Db.DeleteFirmware(db, id=id)
    if not result:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Firmware not found", data=None).model_dump(),
        )
    return CommonOut(data=None)


@router.post(
    "/latest",
    response_model=CommonOut[FirmwareOut],
    responses={**R404_DEVICE_NOT_FOUND, **R404_FIRMWARE_NOT_FOUND},
    summary="设备获取最新固件",
)
async def get_latest_firmware(
    body: Annotated[FirmwareSecret, Body()],
    db: Db.Session = Depends(Db.GetDb("GetLatestFirmware")),
):
    """
    # 设备通过密钥获取最新固件信息

    接收设备密钥，在数据库中查询设备类型，返回对应设备类型的最新固件信息
    """
    # 根据密钥查找设备
    devices = Db.GetDevices(db, secret=body.secret)
    if not devices:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Device not found", data=None).model_dump(),
        )
    device = devices[0]

    if not device.device_type:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Device type not set", data=None).model_dump(),
        )

    # 获取该设备类型的最新固件
    firmware = Db.GetLatestFirmware(db, device_type=device.device_type)
    if not firmware:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Firmware not found", data=None).model_dump(),
        )

    return CommonOut(data=firmware)


@router.post(
    "/download",
    responses={**R404_DEVICE_NOT_FOUND, **R404_FIRMWARE_NOT_FOUND},
    summary="设备下载最新固件",
)
async def download_latest_firmware(
    body: Annotated[FirmwareSecret, Body()],
    db: Db.Session = Depends(Db.GetDb("DownloadLatestFirmware")),
):
    """
    # 设备通过密钥下载最新固件文件

    接收设备密钥，查询设备类型，返回对应的固件文件供设备OTA下载
    """
    # 根据密钥查找设备
    devices = Db.GetDevices(db, secret=body.secret)
    if not devices:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Device not found", data=None).model_dump(),
        )
    device = devices[0]

    if not device.device_type:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Device type not set", data=None).model_dump(),
        )

    # 获取该设备类型的最新固件
    firmware = Db.GetLatestFirmware(db, device_type=device.device_type)
    if not firmware:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Firmware not found", data=None).model_dump(),
        )

    # 返回固件文件
    abs_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), firmware.filepath)
    if not os.path.exists(abs_path):
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Firmware file not found on server", data=None).model_dump(),
        )

    return FileResponse(
        path=abs_path,
        filename=os.path.basename(abs_path),
        media_type="application/octet-stream",
    )
