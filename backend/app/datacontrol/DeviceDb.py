from pydantic import BaseModel
from sqlalchemy import Column, Integer, String, Boolean
from typing import Optional, Literal, List
from sqlalchemy.orm import sessionmaker, Session, declarative_base

DeviceBase = declarative_base()


class M_Devices(DeviceBase):
    __tablename__ = "devices"

    id = Column(
        Integer, primary_key=True, index=True
    )  # index = True 创建索引, 方便查询
    secret = Column(String, unique=True, index=True)  # 设备密钥
    name = Column(String)  # 设备名称
    area = Column(String)  # 设备所在区域
    number = Column(Integer)  # 设备所在区域的编号
    isOnline = Column(Boolean)  # 是否连接
    status = Column(String)  # 状态 stream standby warning error none


class DeviceOut(BaseModel):
    id: int
    name: str
    area: str
    number: int
    isOnline: bool
    status: Literal["stream", "standby", "error", "warning", "none"]

    class Config:
        from_attributes = True


def GetDevices(
    db: Session,
    id: Optional[int] = None,
    name: Optional[str] = None,
    area: Optional[str] = None,
    number: Optional[int] = None,
    isOnline: Optional[bool] = None,
    status: Optional[str] = None,
) -> List[M_Devices]:
    """
    根据传入的条件查询设备，返回符合条件的设备列表。
    所有参数均为可选，仅当传入非 None 时才加入过滤条件。
    """
    conditions = []
    if id is not None:
        conditions.append(M_Devices.id == id)
    if name is not None:
        conditions.append(M_Devices.name == name)
    if area is not None:
        conditions.append(M_Devices.area == area)
    if number is not None:
        conditions.append(M_Devices.number == number)
    if isOnline is not None:
        conditions.append(M_Devices.isOnline == isOnline)
    if status is not None:
        conditions.append(M_Devices.status == status)

    devices = db.query(M_Devices).filter(*conditions).all()
    return devices


def RegisterDevice(
    db: Session,
    secret: str,
    name: str,
    area: str,
    number: int,
    isOnline: bool = False,
    status: str = "none",
) -> M_Devices:
    """
    注册新设备。
    返回新创建的设备对象。
    """
    new_device = M_Devices(
        secret=secret,
        name=name,
        area=area,
        number=number,
        isOnline=isOnline,
        status=status,
    )
    db.add(new_device)
    db.commit()
    db.refresh(new_device)
    return new_device


def UpdateDevice(
    db: Session,
    id: int,
    name: Optional[str] = None,
    area: Optional[str] = None,
    number: Optional[int] = None,
    isOnline: Optional[bool] = None,
    status: Optional[str] = None,
) -> Optional[M_Devices]:
    """
    使用唯一id更新设备信息。
    不允许更新设备密钥
    返回更新后的设备对象；若设备不存在则返回 None。
    字段更新规则：
      - 字符串字段：若传入 None 则不更新；若传入空字符串则置为 None；否则更新为新值。
      - 布尔/整数字段：仅当传入非 None 时更新。
    """
    device = db.query(M_Devices).filter(M_Devices.id == id).first()
    if not device:
        return None

    # 名称：非空字符串才更新
    if name is not None:
        device.name = name if name != "" else None

    # 区域：非空字符串才更新
    if area is not None:
        device.area = area if area != "" else None

    # 编号：仅当传入非 None 时更新（编号为整数，不允许清空）
    if number is not None:
        device.number = number

    # 在线状态：仅当传入非 None 时更新
    if isOnline is not None:
        device.isOnline = isOnline

    # 状态：若传入空字符串则置为 None，否则更新
    if status is not None:
        device.status = status if status != "" else None

    db.commit()
    db.refresh(device)
    return device


def DeleteDevice(db: Session, id: int) -> Optional[int]:
    """
    根据 id 删除设备。
    成功删除返回 1，设备不存在返回 None。
    """
    device = db.query(M_Devices).filter(M_Devices.id == id).first()
    if not device:
        return None
    db.delete(device)
    db.commit()
    return 1
