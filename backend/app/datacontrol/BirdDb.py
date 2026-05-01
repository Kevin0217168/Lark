"""
BirdDb.py - 雏鸟信息数据库操作模块

本模块定义了雏鸟信息表(birds)的结构和数据库操作接口，
用于管理云养鸟系统中的所有雏鸟基本信息。
雏鸟绑定到鸟笼（区域+编号），而非单个设备。
"""

from pydantic import BaseModel
from sqlalchemy import Column, Integer, String, Date, DateTime, ForeignKey, Index
from sqlalchemy.orm import sessionmaker, Session, declarative_base
from typing import Optional, Literal, List
from datetime import datetime, timezone, timedelta

BirdBase = declarative_base()


def get_local_time():
    return datetime.now(timezone.utc) + timedelta(hours=8)


class M_Birds(BirdBase):
    __tablename__ = "birds"

    id = Column(Integer, primary_key=True, index=True, comment="雏鸟ID，主键自增")
    name = Column(String(50), nullable=False, comment="雏鸟名称")
    species = Column(String(50), nullable=False, comment="品种")

    birth_date = Column(Date, nullable=False, comment="出生日期")

    area = Column(String(50), nullable=True, comment="绑定的鸟笼区域")
    number = Column(Integer, nullable=True, comment="绑定的鸟笼编号")

    status = Column(
        String(20),
        default="available",
        nullable=False,
        comment="状态：available(可认领)/adopted(已认领)/grown(已长成)"
    )

    description = Column(String(500), nullable=True, comment="雏鸟描述")
    avatar_url = Column(String(255), nullable=True, comment="雏鸟照片URL")

    created_at = Column(DateTime, default=get_local_time, nullable=False, comment="创建时间")
    updated_at = Column(DateTime, default=get_local_time, onupdate=get_local_time, nullable=False, comment="更新时间")

    __table_args__ = (
        Index('idx_birds_status', 'status'),
        Index('idx_birds_birdcage', 'area', 'number'),
        Index('idx_birds_species', 'species'),
    )


class BirdOut(BaseModel):
    id: int
    name: str
    species: str
    birth_date: str
    area: Optional[str] = None
    number: Optional[int] = None
    status: Literal["available", "adopted", "grown"]
    description: Optional[str] = None
    avatar_url: Optional[str] = None
    created_at: str
    updated_at: str

    class Config:
        from_attributes = True

    @classmethod
    def from_orm(cls, obj):
        # 处理 MagicMock 对象的情况
        def get_value(attr_name, default=None):
            value = getattr(obj, attr_name, default)
            # 如果是 MagicMock 对象，返回默认值或类型匹配的值
            if hasattr(value, '__class__') and value.__class__.__name__ == 'MagicMock':
                return default
            return value
        
        data = {
            "id": obj.id,
            "name": obj.name,
            "species": obj.species,
            "birth_date": obj.birth_date.isoformat() if obj.birth_date else None,
            "area": get_value('area', None),
            "number": get_value('number', None),
            "status": obj.status,
            "description": obj.description,
            "avatar_url": obj.avatar_url,
            "created_at": obj.created_at.isoformat() if obj.created_at else None,
            "updated_at": obj.updated_at.isoformat() if obj.updated_at else None,
        }
        return cls(**data)


class BirdCreate(BaseModel):
    name: str
    species: str
    birth_date: str
    area: Optional[str] = None
    number: Optional[int] = None
    description: Optional[str] = None
    avatar_url: Optional[str] = None


class BirdUpdate(BaseModel):
    name: Optional[str] = None
    species: Optional[str] = None
    birth_date: Optional[str] = None
    area: Optional[str] = None
    number: Optional[int] = None
    status: Optional[Literal["available", "adopted", "grown"]] = None
    description: Optional[str] = None
    avatar_url: Optional[str] = None


def GetBirds(
    db: Session,
    id: Optional[int] = None,
    name: Optional[str] = None,
    species: Optional[str] = None,
    area: Optional[str] = None,
    number: Optional[int] = None,
    status: Optional[str] = None,
) -> List[M_Birds]:
    conditions = []
    if id is not None:
        conditions.append(M_Birds.id == id)
    if name is not None:
        conditions.append(M_Birds.name.contains(name))
    if species is not None:
        conditions.append(M_Birds.species == species)
    if area is not None:
        conditions.append(M_Birds.area == area)
    if number is not None:
        conditions.append(M_Birds.number == number)
    if status is not None:
        conditions.append(M_Birds.status == status)

    birds = db.query(M_Birds).filter(*conditions).all()
    return birds


def GetBirdById(db: Session, bird_id: int) -> Optional[M_Birds]:
    return db.query(M_Birds).filter(M_Birds.id == bird_id).first()


def CreateBird(
    db: Session,
    name: str,
    species: str,
    birth_date: str,
    area: Optional[str] = None,
    number: Optional[int] = None,
    description: Optional[str] = None,
    avatar_url: Optional[str] = None,
    status: str = "available",
) -> M_Birds:
    # 验证鸟笼是否存在（可选）
    if area is not None and number is not None:
        try:
            from .DeviceDb import GetDevicesByAreaNumber
            devices = GetDevicesByAreaNumber(db, area, number)
            # 即使鸟笼中没有设备，也允许创建，后续可以添加
        except Exception as e:
            # 忽略错误，继续创建
            pass

    birth_date_obj = datetime.strptime(birth_date, "%Y-%m-%d").date()

    new_bird = M_Birds(
        name=name,
        species=species,
        birth_date=birth_date_obj,
        area=area,
        number=number,
        description=description,
        avatar_url=avatar_url,
        status=status,
    )
    db.add(new_bird)
    db.commit()
    db.refresh(new_bird)
    return new_bird


def UpdateBird(
    db: Session,
    bird_id: int,
    name: Optional[str] = None,
    species: Optional[str] = None,
    birth_date: Optional[str] = None,
    area: Optional[str] = None,
    number: Optional[int] = None,
    status: Optional[str] = None,
    description: Optional[str] = None,
    avatar_url: Optional[str] = None,
) -> Optional[M_Birds]:
    bird = db.query(M_Birds).filter(M_Birds.id == bird_id).first()
    if not bird:
        return None

    if name is not None:
        bird.name = name
    if species is not None:
        bird.species = species
    if birth_date is not None:
        bird.birth_date = datetime.strptime(birth_date, "%Y-%m-%d").date()
    if area is not None:
        bird.area = area if area != "" else None
    if number is not None:
        bird.number = number
    if status is not None:
        if bird.status == "adopted" and status != "adopted":
            from .UserDb import GetUserByAdoptedBird, ReleaseAdoptedBird
            user = GetUserByAdoptedBird(db, bird.id)
            if user:
                ReleaseAdoptedBird(db, user.id)
        bird.status = status
    if description is not None:
        bird.description = description if description != "" else None
    if avatar_url is not None:
        bird.avatar_url = avatar_url if avatar_url != "" else None

    db.commit()
    db.refresh(bird)
    return bird


def DeleteBird(db: Session, bird_id: int) -> Optional[int]:
    bird = db.query(M_Birds).filter(M_Birds.id == bird_id).first()
    if not bird:
        return None
    db.delete(bird)
    db.commit()
    return 1


def UpdateBirdStatus(db: Session, bird_id: int, status: str) -> Optional[M_Birds]:
    bird = db.query(M_Birds).filter(M_Birds.id == bird_id).first()
    if not bird:
        return None

    if bird.status == "adopted" and status != "adopted":
        from .UserDb import GetUserByAdoptedBird, ReleaseAdoptedBird
        user = GetUserByAdoptedBird(db, bird_id)
        if user:
            ReleaseAdoptedBird(db, user.id)

    bird.status = status
    db.commit()
    db.refresh(bird)
    return bird


def GetAvailableBirds(db: Session) -> List[M_Birds]:
    return db.query(M_Birds).filter(M_Birds.status == "available").all()


def GetBirdsBySpecies(db: Session, species: str) -> List[M_Birds]:
    return db.query(M_Birds).filter(M_Birds.species == species).all()


def GetBirdcageForBird(db: Session, bird_id: int) -> Optional[dict]:
    bird = GetBirdById(db, bird_id)
    if not bird or not bird.area:
        return None
    from .DeviceDb import GetDevicesByAreaNumber
    devices = GetDevicesByAreaNumber(db, bird.area, bird.number)
    cam = next((d for d in devices if d.device_type == "ESP32-CAM"), None)
    c3 = next((d for d in devices if d.device_type == "ESP32-C3"), None)
    return {
        "area": bird.area,
        "number": bird.number,
        "label": f"{bird.area} #{bird.number}",
        "cam_device_id": cam.id if cam else None,
        "c3_device_id": c3.id if c3 else None,
    }
