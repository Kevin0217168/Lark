from fastapi import APIRouter
from typing import Annotated, List, Optional
from fastapi import Path, Query, Depends, Body
from fastapi.responses import JSONResponse
from datetime import datetime, timedelta
from Logset import logger

import Db
import Security
from schema import (
    CommonOut,
    SensorDataFilter,
    SensorDataCreate,
    SensorDataDelete,
    GroupedDataItem,
    SummaryData,
    StatisticData,
    R200_SENSOR_SUMMARY,
    R200_SENSOR_GROUP, 
    R404_SENSOR_NOT_FOUND,
    R404_DEVICE_NOT_FOUND_BY_SECRET,
    R403_FORBIDDEN,
)

router = APIRouter(prefix="/sensors", tags=["Sensors"])

@router.get("/grouped", response_model=CommonOut[List[GroupedDataItem]], responses=R200_SENSOR_GROUP)
async def get_grouped_data(
    now: Annotated[Optional[datetime], Query()] = None,
    period: Annotated[int, Query(ge=1, description="时间段（秒）", examples=[86400])] = 86400,
    group: Annotated[int, Query(ge=1, le=100, description="分组数", examples=[24])] = 24,
    device_id: Annotated[Optional[int], Query(description="设备ID，不传则计算所有设备")] = None,
    db: Db.Session = Depends(Db.GetDb("get_grouped_data")),
):
    """
    # 获取分组统计的温湿度数据
    
    - 将指定时间范围内的数据按时间平均分成 group 组，返回每组的平均温度和湿度
    - 参数:
        - now: 当前时间（不传则使用服务器当前时间）
        - period: 时间段（秒），以 now 向前计算
        - group: 分组数（1-100）
        - device_id: 设备ID，可选；不传则统计所有设备
    """
    if now is None:
        now = datetime.now()
    start_time = now - timedelta(seconds=period)

    # 获取数据
    data_grouped = Db.GetSensorData(
        db,
        device_id=device_id,
        start_time=start_time,
        end_time=now,
        skip=0,
        limit=100000,
    )

    # 预计算每组的时间区间
    interval = period / group
    group_ranges = []
    for i in range(group):
        g_start = start_time + timedelta(seconds=i * interval)
        g_end = start_time + timedelta(seconds=(i + 1) * interval)
        group_ranges.append((g_start, g_end))

    # 初始化分组统计（计数、温度总和、湿度总和）
    group_stats = [{"count": 0, "temp_sum": 0.0, "hum_sum": 0.0} for _ in range(group)]

    if data_grouped:
        all_records = []
        for device_data in data_grouped:
            all_records.extend(device_data.data)

        for record in all_records:
            # 确定记录所属分组
            offset = (record.timestamp - start_time).total_seconds()
            if offset < 0:
                continue
            group_idx = int(offset // interval)
            if group_idx >= group:
                group_idx = group - 1
            stats = group_stats[group_idx]
            stats["count"] += 1
            stats["temp_sum"] += record.temperature
            stats["hum_sum"] += record.humidity

    # 构建返回结果
    result = []
    for i, (g_start, g_end) in enumerate(group_ranges):
        stats = group_stats[i]
        count = stats["count"]
        if count > 0:
            avg_temp = stats["temp_sum"] / count
            avg_hum = stats["hum_sum"] / count
        else:
            avg_temp = 0.0
            avg_hum = 0.0
        result.append(GroupedDataItem(
            group=i,
            start_time=g_start,
            end_time=g_end,
            total=count,
            avg_temperature=avg_temp,
            avg_humidity=avg_hum,
        ))
    return CommonOut(data=result)


@router.get("/summary", response_model=CommonOut[SummaryData], responses=R200_SENSOR_SUMMARY)
async def get_summary_data(
    now: Annotated[Optional[datetime], Query(description="当前时间（不传则使用服务器时间）")] = None,
    period: Annotated[int, Query(ge=1, description="时间段（秒），以 now 向前计算", examples=[86400])] = 86400,
    device_id: Annotated[Optional[int], Query(description="设备ID，不传则统计所有设备")] = None,
    db: Db.Session = Depends(Db.GetDb("get_summary_data")),
):
    """
    # 获取温湿度数据统计摘要
    
    返回指定时间范围内所有设备（或指定设备）的温度、湿度平均值、最大值、最小值以及总数据条数。
    
    - **now**: 当前时间（可选，默认服务器时间）
    - **period**: 时间段（秒），从 now 向前计算
    - **device_id**: 设备ID（可选，不传则统计所有设备）
    """
    if now is None:
        now = datetime.now()
    start_time = now - timedelta(seconds=period)

    data_grouped = Db.GetSensorData(
        db,
        device_id=device_id,
        start_time=start_time,
        end_time=now,
        skip=0,
        limit=100000,
    )

    # 收集数据
    temps = []
    hums = []
    for device_data in data_grouped:
        for item in device_data.data:
            temps.append(item.temperature)
            hums.append(item.humidity)

    total = len(temps)  # 温度与湿度条数相同
    if total == 0:
        return CommonOut(data=SummaryData(
            total=0,
            temperature=StatisticData(avg=0.0, max=0.0, min=0.0),
            humidity=StatisticData(avg=0.0, max=0.0, min=0.0)
        ))

    temp_avg = sum(temps) / total
    temp_max = max(temps)
    temp_min = min(temps)

    hum_avg = sum(hums) / total
    hum_max = max(hums)
    hum_min = min(hums)

    result = SummaryData(
        total=total,
        temperature=StatisticData(avg=temp_avg, max=temp_max, min=temp_min),
        humidity=StatisticData(avg=hum_avg, max=hum_max, min=hum_min)
    )
    return CommonOut(data=result)


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
            content=CommonOut(code=404, msg="Device not found", data=None).model_dump(),
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