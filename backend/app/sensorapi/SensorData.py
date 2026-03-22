from fastapi import APIRouter
from typing import Annotated, List, Optional
from fastapi import Path, Query, Depends, Body
from fastapi.responses import JSONResponse
from datetime import datetime
from Logset import logger

import Db
import Security
from schema import (
    CommonOut,
    SensorDataFilter,
    SensorDataCreate,
    SensorDataDelete,
    R404_SENSOR_NOT_FOUND,
    R404_DEVICE_NOT_FOUND_BY_SECRET,
    R403_FORBIDDEN,
)

router = APIRouter(prefix="/sensors", tags=["Sensors"])


@router.get(
    "/{device_id}",
    response_model=CommonOut[List[Db.SensorDataItem]],
    responses=R404_SENSOR_NOT_FOUND,
)
async def get_sensors_data_by_device(
    device_id: Annotated[int, Path(title="设备ID", description="数据库设备唯一主键id")],
    start_time: Annotated[Optional[datetime], Query()] = None,
    end_time: Annotated[Optional[datetime], Query()] = None,
    skip: Annotated[int, Query(ge=0)] = 0,
    limit: Annotated[int, Query(ge=1, le=1000)] = 100,
    db: Db.Session = Depends(Db.GetDb("get_sensors_data_by_device")),
):
    """
    # 按设备ID查询传感器数据
    
    - 支持时间范围过滤
    - 支持分页（skip/limit）
    - 若未找到数据返回404
    """
    data_grouped = Db.GetSensorData(
        db,
        device_id=device_id,
        start_time=start_time,
        end_time=end_time,
        skip=skip,
        limit=limit,
    )
    if not data_grouped:
        return JSONResponse(
            status_code=404,
            content=CommonOut(
                code=404, msg="Sensor data not found", data=None
            ).model_dump(),
        )
    # data_grouped 是 List[DeviceSensorData]，取第一个设备的数据列表
    sensor_items = data_grouped[0].data
    return CommonOut(data=sensor_items)


@router.get(
    "",
    response_model=CommonOut[List[Db.DeviceSensorData]],
    responses=R404_SENSOR_NOT_FOUND,
)
async def get_sensors_data(
    start_time: Annotated[Optional[datetime], Query()] = None,
    end_time: Annotated[Optional[datetime], Query()] = None,
    skip: Annotated[int, Query(ge=0)] = 0,
    limit: Annotated[int, Query(ge=1, le=1000)] = 100,
    db: Db.Session = Depends(Db.GetDb("get_sensors_data")),
):
    """
    # 查询所有设备的传感器数据（按设备分组）
    
    - 支持时间范围过滤
    - 支持分页（skip/limit）
    - 若未找到数据返回404
    """
    data = Db.GetSensorData(
        db,
        start_time=start_time,
        end_time=end_time,
        skip=skip,
        limit=limit,
    )
    if not data:
        return JSONResponse(
            status_code=404,
            content=CommonOut(
                code=404, msg="Sensor data not found", data=None
            ).model_dump(),
        )
    return CommonOut(data=data)


@router.post(
    "",
    response_model=CommonOut[Db.SensorDataItem],
    responses=R404_DEVICE_NOT_FOUND_BY_SECRET,
)
async def update_sensors_data(
    body: Annotated[SensorDataCreate, Body()],
    db: Db.Session = Depends(Db.GetDb("update_sensors_data")),
):
    """
    # 设备上报传感器数据
    
    - 通过设备密钥（secret）查找对应设备
    - 若设备不存在返回404
    - 若未提供时间戳，则使用服务器当前时间
    """
    # 1. 根据 secret 查询设备
    devices = Db.GetDevices(db, secret=body.secret)
    if not devices:
        return JSONResponse(
            status_code=404,
            content=CommonOut(
                code=404, msg="Device not found", data=None
            ).model_dump(),
        )
    device = devices[0]  # secret 唯一，取第一个

    # 2. 确定时间戳
    timestamp = body.timestamp if body.timestamp is not None else datetime.now()

    # 3. 插入数据
    new_data = Db.AddSensorData(
        db,
        device_id=device.id,
        timestamp=timestamp,
        temperature=body.temperature,
        humidity=body.humidity,
    )
    # 将 ORM 对象转换为 Pydantic 模型
    sensor_out = Db.SensorDataItem(
        id=new_data.id,
        timestamp=new_data.timestamp,
        temperature=new_data.temperature,
        humidity=new_data.humidity,
    )
    return CommonOut(data=sensor_out)


@router.delete(
    "",
    response_model=CommonOut[int],
    responses={**R404_SENSOR_NOT_FOUND, **R403_FORBIDDEN},
)
async def delete_sensors_data(
    device_id: Annotated[int, Query(title="设备ID")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("delete_sensors_data")),
    start_time: Annotated[Optional[datetime], Query()] = None,
    end_time: Annotated[Optional[datetime], Query()] = None,
):
    """
    # 删除指定设备在时间范围内的传感器数据（仅 root 可操作）
    
    - 若不提供 start_time 和 end_time，则删除该设备所有数据
    - 返回删除的记录数，若无数据返回404
    """
    if current_user.role != "root":
        return JSONResponse(
            status_code=403,
            content=CommonOut(
                code=403, msg="Permission denied.", data=None
            ).model_dump(),
        )

    deleted_count = Db.DeleteSensorData(
        db,
        device_id=device_id,
        start_time=start_time,
        end_time=end_time,
    )
    if deleted_count == 0:
        return JSONResponse(
            status_code=404,
            content=CommonOut(
                code=404, msg="Sensor data not found", data=None
            ).model_dump(),
        )
    return CommonOut(data=deleted_count)