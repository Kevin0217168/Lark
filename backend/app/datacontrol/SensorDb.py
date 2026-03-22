from pydantic import BaseModel
from sqlalchemy import Column, Integer, Float, DateTime, Index, String
from datetime import datetime
from typing import Optional, Literal, List, Dict, Any
from sqlalchemy.orm import sessionmaker, Session, declarative_base

SensorDataBase = declarative_base()


class M_SensorData(SensorDataBase):
    __tablename__ = "sensor_data"

    id = Column(Integer, primary_key=True, index=True)  
    device_id = Column(Integer, nullable=False, index=True)  # 所属设备id
    timestamp = Column(DateTime, nullable=False, index=True)  # 记录时间戳
    temperature = Column(Float, nullable=False)  # 温度
    humidity = Column(Float, nullable=False)  # 湿度
    
    # 联合索引，优化按设备+时间的查询
    __table_args__ = (
        Index("ix_device_timestamp", "device_id", "timestamp"),
    )


class SensorDataItem(BaseModel):
    id: int
    timestamp: datetime
    temperature: float
    humidity: float

    class Config:
        from_attributes = True

class DeviceSensorData(BaseModel):
    device_id: int
    data: List[SensorDataItem]


def GetSensorData(
    db: Session,
    id: Optional[int] = None,
    device_id: Optional[int] = None,
    start_time: Optional[datetime] = None,
    end_time: Optional[datetime] = None,
    skip: int = 0,
    limit: int = 100,
) -> List[DeviceSensorData]:   # 返回类型改为 List[DeviceSensorData]
    """
    根据条件查询传感器数据，支持分页。
    若指定 id，则直接返回该条记录（忽略其他条件）；
    否则按 device_id、时间范围组合过滤。
    返回结构为：
    [
        {
            "device_id": int,
            "data": [
                {
                    "id": int,
                    "timestamp": datetime,
                    "temperature": float,
                    "humidity": float
                },
                ...
            ]
        },
        ...
    ]
    """
    query = db.query(M_SensorData)

    # 如果提供了 id，则直接按 id 查询，忽略其他条件
    if id is not None:
        query = query.filter(M_SensorData.id == id)
    else:
        if device_id is not None:
            query = query.filter(M_SensorData.device_id == device_id)
        if start_time is not None:
            query = query.filter(M_SensorData.timestamp >= start_time)
        if end_time is not None:
            query = query.filter(M_SensorData.timestamp <= end_time)

    # 分页
    query = query.order_by(M_SensorData.timestamp).offset(skip).limit(limit)

    results = query.all()

    grouped = {}
    for record in results:
        grouped.setdefault(record.device_id, []).append({
            "id": record.id,
            "timestamp": record.timestamp,
            "temperature": record.temperature,
            "humidity": record.humidity,
        })
    return [DeviceSensorData(device_id=dev_id, data=data_list) for dev_id, data_list in grouped.items()]


def AddSensorData(
    db: Session,
    device_id: int,
    timestamp: datetime,
    temperature: float,
    humidity: float,
) -> M_SensorData:
    """
    添加一条传感器数据记录。
    返回新创建的 M_SensorData 对象。
    """
    new_data = M_SensorData(
        device_id=device_id,
        timestamp=timestamp,
        temperature=temperature,
        humidity=humidity,
    )
    db.add(new_data)
    db.commit()
    db.refresh(new_data)
    return new_data


def DeleteSensorData(
    db: Session,
    device_id: int,
    start_time: Optional[datetime] = None,
    end_time: Optional[datetime] = None,
) -> int:
    """
    删除指定设备在时间范围内的传感器数据。
    如果 start_time 和 end_time 均为 None，则删除该设备的所有数据。
    返回删除的记录数。
    """
    query = db.query(M_SensorData).filter(M_SensorData.device_id == device_id)
    if start_time is not None:
        query = query.filter(M_SensorData.timestamp >= start_time)
    if end_time is not None:
        query = query.filter(M_SensorData.timestamp <= end_time)

    deleted_count = query.delete(synchronize_session=False)
    db.commit()
    return deleted_count