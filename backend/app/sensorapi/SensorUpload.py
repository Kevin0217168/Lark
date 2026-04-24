from fastapi import APIRouter
from typing import Annotated, List, Optional
from fastapi import Path, Query, Depends, Body
from fastapi.responses import JSONResponse
from datetime import datetime
from Logset import logger

import Db
import Security
from datacontrol.SensorUploadDb import (
    SensorUploadCreate,
    SensorUploadUpdate,
    SensorUploadFilter,
    SensorUploadResult,
    SensorUploadItem,
    AddSensorUpload,
    GetSensorUpload,
    UpdateSensorUpload,
    DeleteSensorUpload,
)
from schema import CommonOut

router = APIRouter(prefix="/sensor-upload", tags=["SensorUpload"])


@router.post(
    "",
    response_model=CommonOut[SensorUploadResult],
    summary="新增传感器上传记录",
    description="设备或客户端上传传感器数据到服务器。\n\n- 使用设备密钥(secret)进行身份验证，只有存在于设备表中的密钥才接受记录\n- 接收完整传感器数据对象，验证数据合法性后将记录插入数据库\n- 若未提供时间戳，则自动使用服务器当前时间(UTC+8)\n- 返回操作结果及新记录ID",
)
async def create_sensor_upload(
    body: Annotated[SensorUploadCreate, Body(description="传感器上传数据，包含设备密钥(secret)、传感器类型、数据类型、数据内容及可选时间戳")],
    db: Db.Session = Depends(Db.GetDb("create_sensor_upload")),
    device_db: Db.Session = Depends(Db.GetDb("create_sensor_upload_device_check")),
):
    result = AddSensorUpload(db, body, device_db)
    if result.success:
        return CommonOut(data=result)
    return JSONResponse(
        status_code=400,
        content=CommonOut(code=400, msg=result.message, data=result).model_dump(),
    )


@router.get(
    "",
    response_model=CommonOut[List[SensorUploadItem]],
    summary="查询传感器上传记录",
    description="支持按设备ID、传感器类型、时间范围等条件进行单条或批量查询，实现分页功能。\n\n- **id**: 按记录ID精确查询\n- **device_id**: 按设备ID查询\n- **sensor_type**: 按传感器类型查询\n- **start_time / end_time**: 按时间范围查询\n- **skip / limit**: 分页参数，默认 skip=0, limit=100\n\n返回符合条件的记录列表，按时间倒序排列。",
)
async def list_sensor_uploads(
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    id: Annotated[Optional[int], Query(title="记录ID", description="按记录ID精确查询")] = None,
    device_id: Annotated[Optional[str], Query(title="设备ID", description="按设备ID模糊查询")] = None,
    sensor_type: Annotated[Optional[str], Query(title="传感器类型", description="按传感器类型查询")] = None,
    start_time: Annotated[Optional[datetime], Query(title="起始时间", description="查询起始时间(包含)")] = None,
    end_time: Annotated[Optional[datetime], Query(title="结束时间", description="查询结束时间(包含)")] = None,
    skip: Annotated[int, Query(ge=0, title="跳过记录数", description="分页偏移量")] = 0,
    limit: Annotated[int, Query(ge=1, le=1000, title="返回记录数", description="每次最多返回1000条")] = 100,
    db: Db.Session = Depends(Db.GetDb("list_sensor_uploads")),
):
    records = GetSensorUpload(
        db,
        id=id,
        device_id=device_id,
        sensor_type=sensor_type,
        start_time=start_time,
        end_time=end_time,
        skip=skip,
        limit=limit,
    )
    return CommonOut(data=records)


@router.put(
    "/{record_id}",
    response_model=CommonOut[SensorUploadResult],
    summary="更新传感器上传记录",
    description="根据主键ID更新指定记录的传感器数据。\n\n- 验证数据合法性\n- 仅更新传入的字段（支持部分更新）\n- 返回更新结果及影响行数",
)
async def update_sensor_upload(
    record_id: Annotated[int, Path(title="记录ID", description="要更新的记录主键ID")],
    body: Annotated[SensorUploadUpdate, Body(description="要更新的字段，支持部分更新（只传需要修改的字段）")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("update_sensor_upload")),
):
    result = UpdateSensorUpload(db, record_id, body)
    if result.success:
        return CommonOut(data=result)
    return JSONResponse(
        status_code=404,
        content=CommonOut(code=404, msg=result.message, data=result).model_dump(),
    )


@router.delete(
    "/{record_id}",
    response_model=CommonOut[SensorUploadResult],
    summary="删除传感器上传记录",
    description="根据主键ID删除指定记录。\n\n- 返回删除结果及影响行数\n- 若记录不存在返回404",
)
async def delete_sensor_upload(
    record_id: Annotated[int, Path(title="记录ID", description="要删除的记录主键ID")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("delete_sensor_upload")),
):
    result = DeleteSensorUpload(db, record_id)
    if result.success:
        return CommonOut(data=result)
    return JSONResponse(
        status_code=404,
        content=CommonOut(code=404, msg=result.message, data=result).model_dump(),
    )
