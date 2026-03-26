from pydantic import BaseModel
from sqlalchemy import Column, Integer, String
from typing import Optional, List
from sqlalchemy.orm import sessionmaker, Session, declarative_base

FirmwareBase = declarative_base()


class M_Firmwares(FirmwareBase):
    __tablename__ = "firmwares"

    id = Column(
        Integer, primary_key=True, index=True
    )  # index = True 创建索引, 方便查询
    device_type = Column(String, index=True)  # 设备类型
    name = Column(String)  # 固件名称
    version = Column(String)  # 固件版本
    filepath = Column(String)  # 固件文件路径


def GetFirmwares(
    db: Session,
    id: Optional[int] = None,
    device_type: Optional[str] = None,
    name: Optional[str] = None,
    version: Optional[str] = None,
) -> List[M_Firmwares]:
    """
    根据传入的条件查询固件，返回符合条件的固件列表。
    所有参数均为可选，仅当传入非 None 时才加入过滤条件。
    """
    conditions = []
    if id is not None:
        conditions.append(M_Firmwares.id == id)
    if device_type is not None:
        conditions.append(M_Firmwares.device_type == device_type)
    if name is not None:
        conditions.append(M_Firmwares.name == name)
    if version is not None:
        conditions.append(M_Firmwares.version == version)

    firmwares = db.query(M_Firmwares).filter(*conditions).all()
    return firmwares


def GetLatestFirmware(
    db: Session,
    device_type: str,
) -> Optional[M_Firmwares]:
    """
    根据设备类型获取最新固件（按id降序取第一条）。
    """
    firmware = (
        db.query(M_Firmwares)
        .filter(M_Firmwares.device_type == device_type)
        .order_by(M_Firmwares.id.desc())
        .first()
    )
    return firmware


def CreateFirmware(
    db: Session,
    device_type: str,
    name: str,
    version: str,
    filepath: str,
) -> M_Firmwares:
    """
    创建新固件记录。
    返回新创建的固件对象。
    """
    new_firmware = M_Firmwares(
        device_type=device_type,
        name=name,
        version=version,
        filepath=filepath,
    )
    db.add(new_firmware)
    db.commit()
    db.refresh(new_firmware)
    return new_firmware


def DeleteFirmware(db: Session, id: int) -> Optional[int]:
    """
    根据 id 删除固件。
    成功删除返回 1，固件不存在返回 None。
    """
    firmware = db.query(M_Firmwares).filter(M_Firmwares.id == id).first()
    if not firmware:
        return None
    db.delete(firmware)
    db.commit()
    return 1
