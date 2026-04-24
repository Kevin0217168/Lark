from pydantic import BaseModel, Field
from sqlalchemy import Column, Integer, String, DateTime, Index
from datetime import datetime, timezone, timedelta
from typing import Optional, List
from sqlalchemy.orm import Session, declarative_base

SensorUploadBase = declarative_base()


class M_SensorUpload(SensorUploadBase):
    __tablename__ = "sensor_upload"

    id = Column(Integer, primary_key=True, index=True, autoincrement=True)
    device_id = Column(String(50), nullable=False, index=True)
    sensor_type = Column(String(50), nullable=False, index=True)
    data_type = Column(String(20), nullable=False)
    data = Column(String, nullable=False)
    timestamp = Column(DateTime, nullable=False, default=lambda: datetime.now(timezone(timedelta(hours=8))))

    __table_args__ = (
        Index("ix_device_sensor_time", "device_id", "sensor_type", "timestamp"),
    )


class SensorUploadItem(BaseModel):
    id: int = Field(..., title="记录ID", description="记录唯一主键ID")
    device_id: str = Field(..., title="设备ID", description="设备唯一标识ID")
    sensor_type: str = Field(..., title="传感器类型", description="传感器类型")
    data_type: str = Field(..., title="数据类型", description="数据单位或格式")
    data: str = Field(..., title="传感器数据", description="传感器采集的数据值")
    timestamp: datetime = Field(..., title="数据时间", description="数据采集时间(UTC+8)")

    class Config:
        from_attributes = True


class SensorUploadCreate(BaseModel):
    secret: str = Field(..., min_length=1, max_length=50, title="设备密钥", description="传感器所属设备的唯一密钥(secret)")
    sensor_type: str = Field(..., min_length=1, max_length=50, title="传感器类型", description="传感器类型，如temperature、humidity、pressure等")
    data_type: str = Field(..., min_length=1, max_length=20, title="数据类型", description="数据单位或格式，如°C、%、hPa等")
    data: str = Field(..., min_length=1, title="传感器数据", description="传感器采集的实际数据值")
    timestamp: Optional[datetime] = Field(None, title="数据时间", description="数据采集时间，若不填则使用服务器当前时间(UTC+8)")


class SensorUploadUpdate(BaseModel):
    sensor_type: Optional[str] = Field(None, min_length=1, max_length=50, title="传感器类型", description="传感器类型，如temperature、humidity、pressure等")
    data_type: Optional[str] = Field(None, min_length=1, max_length=20, title="数据类型", description="数据单位或格式，如°C、%、hPa等")
    data: Optional[str] = Field(None, min_length=1, title="传感器数据", description="传感器采集的实际数据值")
    timestamp: Optional[datetime] = Field(None, title="数据时间", description="数据采集时间")


class SensorUploadFilter(BaseModel):
    model_config = {"extra": "forbid"}

    id: Optional[int] = Field(None, title="记录ID")
    device_id: Optional[str] = Field(None, title="设备ID")
    sensor_type: Optional[str] = Field(None, title="传感器类型")
    start_time: Optional[datetime] = Field(None, title="起始时间")
    end_time: Optional[datetime] = Field(None, title="结束时间")
    skip: int = Field(0, ge=0, title="跳过记录数")
    limit: int = Field(100, ge=1, le=1000, title="返回记录数")


class SensorUploadResult(BaseModel):
    success: bool
    message: str
    id: Optional[int] = None
    rows_affected: int = 0


UTC8 = timezone(timedelta(hours=8))


def AddSensorUpload(db: Session, item: SensorUploadCreate, device_db: Session) -> SensorUploadResult:
    """
    新增传感器上传记录。
    使用 secret 进行身份验证，验证通过后存储设备的 id 到 device_id 字段。
    """
    try:
        # 验证 secret 是否存在于设备表中
        from datacontrol.DeviceDb import M_Devices
        device = device_db.query(M_Devices).filter(M_Devices.secret == item.secret).first()
        if not device:
            return SensorUploadResult(
                success=False,
                message="Invalid secret: device not found",
                rows_affected=0,
            )

        timestamp = item.timestamp if item.timestamp else datetime.now(UTC8)
        new_record = M_SensorUpload(
            device_id=str(device.id),
            sensor_type=item.sensor_type,
            data_type=item.data_type,
            data=item.data,
            timestamp=timestamp,
        )
        db.add(new_record)
        db.commit()
        db.refresh(new_record)
        return SensorUploadResult(
            success=True,
            message="Sensor upload record created successfully",
            id=new_record.id,
            rows_affected=1,
        )
    except Exception as e:
        db.rollback()
        return SensorUploadResult(
            success=False,
            message=f"Failed to create sensor upload record: {str(e)}",
            rows_affected=0,
        )


def GetSensorUpload(
    db: Session,
    id: Optional[int] = None,
    device_id: Optional[str] = None,
    sensor_type: Optional[str] = None,
    start_time: Optional[datetime] = None,
    end_time: Optional[datetime] = None,
    skip: int = 0,
    limit: int = 100,
) -> List[SensorUploadItem]:
    query = db.query(M_SensorUpload)

    if id is not None:
        query = query.filter(M_SensorUpload.id == id)
    else:
        if device_id is not None:
            query = query.filter(M_SensorUpload.device_id == device_id)
        if sensor_type is not None:
            query = query.filter(M_SensorUpload.sensor_type == sensor_type)
        if start_time is not None:
            query = query.filter(M_SensorUpload.timestamp >= start_time)
        if end_time is not None:
            query = query.filter(M_SensorUpload.timestamp <= end_time)

    query = query.order_by(M_SensorUpload.timestamp.desc()).offset(skip).limit(limit)
    results = query.all()

    return [SensorUploadItem.model_validate(r) for r in results]


def UpdateSensorUpload(db: Session, id: int, item: SensorUploadUpdate) -> SensorUploadResult:
    try:
        record = db.query(M_SensorUpload).filter(M_SensorUpload.id == id).first()
        if not record:
            return SensorUploadResult(
                success=False,
                message=f"Sensor upload record with id {id} not found",
                rows_affected=0,
            )

        update_data = item.model_dump(exclude_unset=True)
        if not update_data:
            return SensorUploadResult(
                success=False,
                message="No fields to update",
                rows_affected=0,
            )

        for key, value in update_data.items():
            setattr(record, key, value)

        db.commit()
        db.refresh(record)
        return SensorUploadResult(
            success=True,
            message="Sensor upload record updated successfully",
            id=record.id,
            rows_affected=1,
        )
    except Exception as e:
        db.rollback()
        return SensorUploadResult(
            success=False,
            message=f"Failed to update sensor upload record: {str(e)}",
            rows_affected=0,
        )


def DeleteSensorUpload(db: Session, id: int) -> SensorUploadResult:
    try:
        record = db.query(M_SensorUpload).filter(M_SensorUpload.id == id).first()
        if not record:
            return SensorUploadResult(
                success=False,
                message=f"Sensor upload record with id {id} not found",
                rows_affected=0,
            )

        db.delete(record)
        db.commit()
        return SensorUploadResult(
            success=True,
            message="Sensor upload record deleted successfully",
            rows_affected=1,
        )
    except Exception as e:
        db.rollback()
        return SensorUploadResult(
            success=False,
            message=f"Failed to delete sensor upload record: {str(e)}",
            rows_affected=0,
        )


def CleanupOldSensorUploads(db: Session, days: int = 7) -> int:
    """
    删除超过指定天数的传感器上传记录。

    Args:
        db: 数据库会话
        days: 保留天数，默认7天

    Returns:
        删除的记录条数
    """
    from datetime import timedelta
    cutoff_time = datetime.now() - timedelta(days=days)

    count = db.query(M_SensorUpload).filter(M_SensorUpload.timestamp < cutoff_time).delete(synchronize_session=False)
    db.commit()
    return count
