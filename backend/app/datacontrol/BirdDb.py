"""
BirdDb.py - 雏鸟信息数据库操作模块

本模块定义了雏鸟信息表(birds)的结构和数据库操作接口，
用于管理云养鸟系统中的所有雏鸟基本信息。

数据表设计遵循 `/docs/云养鸟系统框架设计文档.md` 规范。
"""

from pydantic import BaseModel
from sqlalchemy import Column, Integer, String, Date, DateTime, ForeignKey, Index
from sqlalchemy.orm import sessionmaker, Session, declarative_base
from typing import Optional, Literal, List
from datetime import datetime, timezone, timedelta

# 创建雏鸟模块的 Base
BirdBase = declarative_base()


def get_local_time():
    """
    获取本地时间（UTC+8）
    兼容后端Token时间校验与未来 Pydantic/SQLAlchemy 时区行为
    """
    return datetime.now(timezone.utc) + timedelta(hours=8)


class M_Birds(BirdBase):
    """
    雏鸟信息表模型
    
    存储所有雏鸟的基本信息，包括名称、品种、出生日期、关联设备等。
    每只雏鸟可以关联到一个设备（摄像头/喂食器），用于实时监控和远程投喂。
    """
    __tablename__ = "birds"
    
    # 主键，自增ID，唯一标识每只雏鸟
    id = Column(Integer, primary_key=True, index=True, comment="雏鸟ID，主键自增")
    
    # 雏鸟名称，用于展示给用户，如"小云"、"豆豆"
    name = Column(String(50), nullable=False, comment="雏鸟名称，如'小云'、'豆豆'")
    
    # 品种，如"鹦鹉"、"鸽子"、"文鸟"等
    species = Column(String(50), nullable=False, comment="品种，如'鹦鹉'、'鸽子'、'文鸟'")
    
    # 出生日期，用于计算日龄和成长阶段
    birth_date = Column(Date, nullable=False, comment="出生日期，用于计算日龄")
    
    # 关联的设备ID，关联到 devices 表
    # 通过该设备可以获取实时画面和控制喂食器
    device_id = Column(
        Integer, 
        nullable=True, 
        comment="关联的设备ID（摄像头/喂食器）"
    )
    
    # 雏鸟状态：
    # - available: 可认领，用户可以进行认领
    # - adopted: 已认领，已被某个用户认领
    # - grown: 已长成，雏鸟已长大，不再参与云养
    status = Column(
        String(20), 
        default="available", 
        nullable=False, 
        comment="状态：available(可认领)/adopted(已认领)/grown(已长成)"
    )
    
    # 雏鸟描述，展示给用户看的详细介绍
    description = Column(String(500), nullable=True, comment="雏鸟描述，展示给用户看")
    
    # 雏鸟照片URL，用于在列表和详情页展示
    avatar_url = Column(String(255), nullable=True, comment="雏鸟照片URL，用于列表展示")
    
    # 记录创建时间，默认当前时间
    created_at = Column(
        DateTime, 
        default=get_local_time, 
        nullable=False, 
        comment="创建时间，默认当前时间"
    )
    
    # 记录更新时间，默认当前时间，更新时自动修改
    updated_at = Column(
        DateTime, 
        default=get_local_time, 
        onupdate=get_local_time,
        nullable=False, 
        comment="更新时间，自动更新"
    )
    
    # 复合索引：按状态查询（用于筛选可认领的雏鸟）
    # 注意：SQLite 不支持索引的 comment 参数
    __table_args__ = (
        Index('idx_birds_status', 'status'),  # 状态索引，用于快速筛选可认领雏鸟
        Index('idx_birds_device', 'device_id'),  # 设备ID索引，用于关联查询
        Index('idx_birds_species', 'species'),  # 品种索引，用于按品种筛选
    )


class BirdOut(BaseModel):
    """
    雏鸟信息输出模型
    
    用于API响应，定义了返回给前端的雏鸟数据结构。
    """
    id: int
    name: str
    species: str
    birth_date: str  # ISO格式日期字符串
    device_id: Optional[int] = None
    status: Literal["available", "adopted", "grown"]
    description: Optional[str] = None
    avatar_url: Optional[str] = None
    created_at: str
    updated_at: str
    
    class Config:
        from_attributes = True
    
    @classmethod
    def from_orm(cls, obj):
        """从ORM对象转换，处理日期格式"""
        data = {
            "id": obj.id,
            "name": obj.name,
            "species": obj.species,
            "birth_date": obj.birth_date.isoformat() if obj.birth_date else None,
            "device_id": obj.device_id,
            "status": obj.status,
            "description": obj.description,
            "avatar_url": obj.avatar_url,
            "created_at": obj.created_at.isoformat() if obj.created_at else None,
            "updated_at": obj.updated_at.isoformat() if obj.updated_at else None,
        }
        return cls(**data)


class BirdCreate(BaseModel):
    """
    创建雏鸟请求模型
    
    定义了创建新雏鸟时需要的字段。
    """
    name: str
    species: str
    birth_date: str  # ISO格式日期字符串，如"2024-01-15"
    device_id: Optional[int] = None
    description: Optional[str] = None
    avatar_url: Optional[str] = None


class BirdUpdate(BaseModel):
    """
    更新雏鸟请求模型
    
    定义了更新雏鸟信息时可更新的字段，所有字段均为可选。
    """
    name: Optional[str] = None
    species: Optional[str] = None
    birth_date: Optional[str] = None
    device_id: Optional[int] = None
    status: Optional[Literal["available", "adopted", "grown"]] = None
    description: Optional[str] = None
    avatar_url: Optional[str] = None


def GetBirds(
    db: Session,
    id: Optional[int] = None,
    name: Optional[str] = None,
    species: Optional[str] = None,
    device_id: Optional[int] = None,
    status: Optional[str] = None,
) -> List[M_Birds]:
    """
    查询雏鸟列表
    
    根据传入的条件查询雏鸟，返回符合条件的雏鸟列表。
    所有参数均为可选，仅当传入非 None 时才加入过滤条件。
    
    Args:
        db: 数据库会话对象
        id: 雏鸟ID，精确匹配
        name: 雏鸟名称，模糊匹配
        species: 品种，精确匹配
        device_id: 关联设备ID，精确匹配
        status: 状态，精确匹配（available/adopted/grown）
    
    Returns:
        List[M_Birds]: 符合条件的雏鸟对象列表
    """
    conditions = []
    if id is not None:
        conditions.append(M_Birds.id == id)
    if name is not None:
        conditions.append(M_Birds.name.contains(name))
    if species is not None:
        conditions.append(M_Birds.species == species)
    if device_id is not None:
        conditions.append(M_Birds.device_id == device_id)
    if status is not None:
        conditions.append(M_Birds.status == status)
    
    birds = db.query(M_Birds).filter(*conditions).all()
    return birds


def GetBirdById(db: Session, bird_id: int) -> Optional[M_Birds]:
    """
    根据ID获取单只雏鸟信息
    
    Args:
        db: 数据库会话对象
        bird_id: 雏鸟ID
    
    Returns:
        Optional[M_Birds]: 雏鸟对象，不存在则返回None
    """
    return db.query(M_Birds).filter(M_Birds.id == bird_id).first()


def CreateBird(
    db: Session,
    name: str,
    species: str,
    birth_date: str,
    device_id: Optional[int] = None,
    description: Optional[str] = None,
    avatar_url: Optional[str] = None,
    status: str = "available",
) -> M_Birds:
    """
    创建新雏鸟记录
    
    Args:
        db: 数据库会话对象
        name: 雏鸟名称
        species: 品种
        birth_date: 出生日期，ISO格式字符串（如"2024-01-15"）
        device_id: 关联设备ID，可选
        description: 雏鸟描述，可选
        avatar_url: 照片URL，可选
        status: 状态，默认为"available"
    
    Returns:
        M_Birds: 新创建的雏鸟对象
    
    Raises:
        ValueError: 当设备ID不存在时
    """
    # 验证设备ID是否存在
    if device_id is not None:
        from .DeviceDb import GetDevices
        devices = GetDevices(db, id=device_id)
        if not devices:
            raise ValueError(f"设备ID {device_id} 不存在")
    
    # 将ISO格式日期字符串转换为date对象
    birth_date_obj = datetime.strptime(birth_date, "%Y-%m-%d").date()
    
    new_bird = M_Birds(
        name=name,
        species=species,
        birth_date=birth_date_obj,
        device_id=device_id,
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
    device_id: Optional[int] = None,
    status: Optional[str] = None,
    description: Optional[str] = None,
    avatar_url: Optional[str] = None,
) -> Optional[M_Birds]:
    """
    更新雏鸟信息
    
    根据雏鸟ID更新信息，仅更新传入的非None字段。
    字符串字段传入空字符串时会清空该字段。
    
    Args:
        db: 数据库会话对象
        bird_id: 雏鸟ID
        name: 名称，可选
        species: 品种，可选
        birth_date: 出生日期，ISO格式字符串，可选
        device_id: 设备ID，可选
        status: 状态，可选
        description: 描述，可选
        avatar_url: 照片URL，可选
    
    Returns:
        Optional[M_Birds]: 更新后的雏鸟对象，不存在则返回None
    
    Raises:
        ValueError: 当设备ID不存在时
    """
    bird = db.query(M_Birds).filter(M_Birds.id == bird_id).first()
    if not bird:
        return None
    
    # 名称：非None时更新，空字符串也允许（清空名称）
    if name is not None:
        bird.name = name
    
    # 品种：非None时更新
    if species is not None:
        bird.species = species
    
    # 出生日期：非None时更新
    if birth_date is not None:
        bird.birth_date = datetime.strptime(birth_date, "%Y-%m-%d").date()
    
    # 设备ID：非None时更新，需要验证设备是否存在
    if device_id is not None:
        from .DeviceDb import GetDevices
        devices = GetDevices(db, id=device_id)
        if not devices:
            raise ValueError(f"设备ID {device_id} 不存在")
        bird.device_id = device_id
    
    # 状态：非None时更新
    if status is not None:
        # 检查状态是否从adopted变为其他状态
        if bird.status == "adopted" and status != "adopted":
            # 查找认领该雏鸟的用户
            from .UserDb import GetUserByAdoptedBird, ReleaseAdoptedBird
            user = GetUserByAdoptedBird(db, bird.id)
            if user:
                # 释放用户的雏鸟
                ReleaseAdoptedBird(db, user.id)
        bird.status = status
    
    # 描述：非None时更新，空字符串清空描述
    if description is not None:
        bird.description = description if description != "" else None
    
    # 照片URL：非None时更新，空字符串清空URL
    if avatar_url is not None:
        bird.avatar_url = avatar_url if avatar_url != "" else None
    
    db.commit()
    db.refresh(bird)
    return bird


def DeleteBird(db: Session, bird_id: int) -> Optional[int]:
    """
    删除雏鸟记录
    
    Args:
        db: 数据库会话对象
        bird_id: 雏鸟ID
    
    Returns:
        Optional[int]: 成功删除返回1，雏鸟不存在返回None
    """
    bird = db.query(M_Birds).filter(M_Birds.id == bird_id).first()
    if not bird:
        return None
    
    db.delete(bird)
    db.commit()
    return 1


def UpdateBirdStatus(db: Session, bird_id: int, status: str) -> Optional[M_Birds]:
    """
    更新雏鸟状态
    
    快速更新雏鸟状态，用于认领、释放等操作。
    如果状态从"adopted"变为其他状态，会自动清空认领该雏鸟的用户信息中的认领信息。
    
    Args:
        db: 数据库会话对象
        bird_id: 雏鸟ID
        status: 新状态（available/adopted/grown）
    
    Returns:
        Optional[M_Birds]: 更新后的雏鸟对象，不存在则返回None
    """
    bird = db.query(M_Birds).filter(M_Birds.id == bird_id).first()
    if not bird:
        return None
    
    # 检查状态是否从adopted变为其他状态
    if bird.status == "adopted" and status != "adopted":
        # 查找认领该雏鸟的用户
        from .UserDb import GetUserByAdoptedBird, ReleaseAdoptedBird
        user = GetUserByAdoptedBird(db, bird_id)
        if user:
            # 释放用户的雏鸟
            ReleaseAdoptedBird(db, user.id)
    
    bird.status = status
    db.commit()
    db.refresh(bird)
    return bird


def GetAvailableBirds(db: Session) -> List[M_Birds]:
    """
    获取所有可认领的雏鸟列表
    
    Args:
        db: 数据库会话对象
    
    Returns:
        List[M_Birds]: 状态为available的雏鸟列表
    """
    return db.query(M_Birds).filter(M_Birds.status == "available").all()


def GetBirdsBySpecies(db: Session, species: str) -> List[M_Birds]:
    """
    按品种获取雏鸟列表
    
    Args:
        db: 数据库会话对象
        species: 品种名称
    
    Returns:
        List[M_Birds]: 指定品种的雏鸟列表
    """
    return db.query(M_Birds).filter(M_Birds.species == species).all()
