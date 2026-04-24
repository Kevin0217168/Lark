import re
from pydantic import BaseModel
from sqlalchemy import Column, Integer, String, DateTime, Text, Index
from datetime import datetime
from typing import Optional, List, Literal
from sqlalchemy.orm import Session, declarative_base

DeviceLogBase = declarative_base()

# ESP-IDF 日志等级映射
LOG_LEVEL_MAP = {
    "E": "ERROR",
    "W": "WARN",
    "I": "INFO",
    "D": "DEBUG",
    "V": "VERBOSE",
}
VALID_LOG_LEVELS = list(LOG_LEVEL_MAP.values())

# ESP-IDF 日志格式: "E (12345) tag: message"
_ESP_LOG_RE = re.compile(r"^([EWIDV]) \((\d+)\) ([^:]+):\s*(.*)$")


def parse_esp_log_lines(raw_text: str) -> List[dict]:
    """
    解析 ESP-IDF 批量日志文本为单条日志列表。

    ESP_LOGx 格式: "E (12345) tag: message"
    - 等级字母: E/W/I/D/V
    - (tick): 设备启动以来的 tick 计数 (毫秒)
    - tag: 组件标签
    - message: 日志内容

    多行日志（续行不以等级字母开头）会合并到上一条。

    Returns:
        [{"level": "ERROR", "tick": 12345, "tag": "wifi", "content": "..."}, ...]
    """
    results: List[dict] = []
    current: Optional[dict] = None

    for line in raw_text.splitlines():
        m = _ESP_LOG_RE.match(line)
        if m:
            # 保存前一条
            if current is not None:
                results.append(current)
            current = {
                "level": LOG_LEVEL_MAP[m.group(1)],
                "tick": int(m.group(2)),
                "tag": m.group(3).strip(),
                "content": m.group(4),
            }
        else:
            # 续行：追加到当前日志内容
            if current is not None:
                current["content"] += "\n" + line
            # 如果没有当前条目（文本不以标准日志行开头），作为 INFO 存储
            elif line.strip():
                current = {
                    "level": "INFO",
                    "tick": 0,
                    "tag": "unknown",
                    "content": line,
                }

    # 最后一条
    if current is not None:
        results.append(current)

    return results


class M_DeviceLog(DeviceLogBase):
    """设备日志表 —— 单条存储 ESP32 日志，含等级和标签"""
    __tablename__ = "device_logs"

    id = Column(Integer, primary_key=True, index=True)
    device_id = Column(Integer, nullable=False, index=True)
    timestamp = Column(DateTime, nullable=False, index=True, default=datetime.now)
    level = Column(String(8), nullable=False, default="INFO", index=True)  # ERROR/WARN/INFO/DEBUG/VERBOSE
    tag = Column(String(64), nullable=False, default="")                    # ESP 组件标签
    tick = Column(Integer, nullable=False, default=0)                       # 设备启动 tick (ms)
    content = Column(Text, nullable=False)                                  # 单条日志内容

    __table_args__ = (
        Index("ix_devicelog_device_timestamp", "device_id", "timestamp"),
        Index("ix_devicelog_device_level", "device_id", "level"),
    )


class DeviceLogItem(BaseModel):
    id: int
    device_id: int
    timestamp: datetime
    level: str
    tag: str
    tick: int
    content: str

    class Config:
        from_attributes = True


class DeviceLogOut(BaseModel):
    device_id: int
    logs: List[DeviceLogItem]


def AddDeviceLog(
    db: Session,
    device_id: int,
    content: str,
    level: str = "INFO",
    tag: str = "",
    tick: int = 0,
    timestamp: datetime = None,
) -> M_DeviceLog:
    """插入一条设备日志"""
    if timestamp is None:
        timestamp = datetime.now()
    log_entry = M_DeviceLog(
        device_id=device_id,
        timestamp=timestamp,
        level=level,
        tag=tag,
        tick=tick,
        content=content,
    )
    db.add(log_entry)
    db.commit()
    db.refresh(log_entry)
    return log_entry


def AddDeviceLogsBatch(
    db: Session,
    device_id: int,
    raw_text: str,
    timestamp: datetime = None,
) -> int:
    """
    解析 ESP-IDF 批量日志文本并逐条写入数据库。
    使用 bulk_insert_mappings 跳过 ORM identity map 跟踪，减少写入开销。

    Returns: 写入的日志条数
    """
    if timestamp is None:
        timestamp = datetime.now()

    parsed = parse_esp_log_lines(raw_text)
    if not parsed:
        return 0

    mappings = [
        {
            "device_id": device_id,
            "timestamp": timestamp,
            "level": item["level"],
            "tag": item["tag"],
            "tick": item["tick"],
            "content": item["content"],
        }
        for item in parsed
    ]

    db.bulk_insert_mappings(M_DeviceLog, mappings)
    db.commit()
    return len(mappings)


def GetDeviceLogs(
    db: Session,
    device_id: Optional[int] = None,
    start_time: Optional[datetime] = None,
    end_time: Optional[datetime] = None,
    level: Optional[str] = None,
    tag: Optional[str] = None,
    skip: int = 0,
    limit: int = 100,
) -> List[M_DeviceLog]:
    """查询设备日志，支持按设备、时间范围、等级、标签过滤和分页"""
    query = db.query(M_DeviceLog)

    if device_id is not None:
        query = query.filter(M_DeviceLog.device_id == device_id)
    if start_time is not None:
        query = query.filter(M_DeviceLog.timestamp >= start_time)
    if end_time is not None:
        query = query.filter(M_DeviceLog.timestamp <= end_time)
    if level is not None:
        query = query.filter(M_DeviceLog.level == level.upper())
    if tag is not None:
        query = query.filter(M_DeviceLog.tag == tag)

    query = query.order_by(M_DeviceLog.timestamp.desc(), M_DeviceLog.tick.desc())
    return query.offset(skip).limit(limit).all()


def DeleteDeviceLogs(
    db: Session,
    device_id: int,
    start_time: Optional[datetime] = None,
    end_time: Optional[datetime] = None,
    level: Optional[str] = None,
) -> int:
    """删除设备日志，返回删除条数"""
    query = db.query(M_DeviceLog).filter(M_DeviceLog.device_id == device_id)
    if start_time is not None:
        query = query.filter(M_DeviceLog.timestamp >= start_time)
    if end_time is not None:
        query = query.filter(M_DeviceLog.timestamp <= end_time)
    if level is not None:
        query = query.filter(M_DeviceLog.level == level.upper())

    count = query.delete(synchronize_session=False)
    db.commit()
    return count


def CleanupOldDeviceLogs(db: Session, days: int = 7) -> int:
    """
    删除超过指定天数的设备日志。
    
    Args:
        db: 数据库会话
        days: 保留天数，默认7天
        
    Returns:
        删除的日志条数
    """
    from datetime import timedelta
    cutoff_time = datetime.now() - timedelta(days=days)
    
    count = db.query(M_DeviceLog).filter(M_DeviceLog.timestamp < cutoff_time).delete(synchronize_session=False)
    db.commit()
    return count
