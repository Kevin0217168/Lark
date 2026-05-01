from pydantic import BaseModel
from sqlalchemy import Column, Integer, String, DateTime, Index
from sqlalchemy.orm import sessionmaker, Session, declarative_base
from typing import Optional, List
from datetime import datetime, timezone, timedelta

FeedingBase = declarative_base()


def get_local_time():
    return datetime.now(timezone.utc) + timedelta(hours=8)


def get_today_start():
    now = get_local_time()
    return now.replace(hour=0, minute=0, second=0, microsecond=0)


def get_today_end():
    now = get_local_time()
    return now.replace(hour=23, minute=59, second=59, microsecond=999999)


class M_FeedingRecords(FeedingBase):
    __tablename__ = "feeding_records"

    id = Column(Integer, primary_key=True, index=True, comment="记录ID，主键自增")
    bird_id = Column(Integer, nullable=False, index=True, comment="雏鸟ID")
    user_id = Column(Integer, nullable=False, index=True, comment="操作用户ID")
    action_type = Column(String(20), nullable=False, index=True, comment="操作类型: feeding")
    timestamp = Column(DateTime, default=get_local_time, nullable=False, comment="操作时间")

    __table_args__ = (
        Index('idx_feeding_bird_date', 'bird_id', 'action_type', 'timestamp'),
        Index('idx_feeding_user_date', 'user_id', 'action_type', 'timestamp'),
    )


class FeedingRecordOut(BaseModel):
    id: int
    bird_id: int
    user_id: int
    action_type: str
    timestamp: str

    class Config:
        from_attributes = True

    @classmethod
    def from_orm(cls, obj):
        data = {
            "id": obj.id,
            "bird_id": obj.bird_id,
            "user_id": obj.user_id,
            "action_type": obj.action_type,
            "timestamp": obj.timestamp.isoformat() if obj.timestamp else None,
        }
        return cls(**data)


class FeedingStatusOut(BaseModel):
    action_type: str
    today_count: int
    daily_limit: int
    remaining: int
    completed: bool
    last_action_time: Optional[str] = None


def CreateFeedingRecord(db: Session, bird_id: int, user_id: int, action_type: str) -> M_FeedingRecords:
    record = M_FeedingRecords(
        bird_id=bird_id,
        user_id=user_id,
        action_type=action_type,
    )
    db.add(record)
    db.commit()
    db.refresh(record)
    return record


def GetTodayRecords(db: Session, bird_id: int, action_type: str) -> List[M_FeedingRecords]:
    today_start = get_today_start()
    today_end = get_today_end()
    return db.query(M_FeedingRecords).filter(
        M_FeedingRecords.bird_id == bird_id,
        M_FeedingRecords.action_type == action_type,
        M_FeedingRecords.timestamp >= today_start,
        M_FeedingRecords.timestamp <= today_end,
    ).order_by(M_FeedingRecords.timestamp.desc()).all()


def GetTodayRecordCount(db: Session, bird_id: int, action_type: str) -> int:
    today_start = get_today_start()
    today_end = get_today_end()
    return db.query(M_FeedingRecords).filter(
        M_FeedingRecords.bird_id == bird_id,
        M_FeedingRecords.action_type == action_type,
        M_FeedingRecords.timestamp >= today_start,
        M_FeedingRecords.timestamp <= today_end,
    ).count()


def GetFeedingHistory(db: Session, bird_id: int, limit: int = 50) -> List[M_FeedingRecords]:
    return db.query(M_FeedingRecords).filter(
        M_FeedingRecords.bird_id == bird_id,
        M_FeedingRecords.action_type == "feeding",
    ).order_by(M_FeedingRecords.timestamp.desc()).limit(limit).all()



