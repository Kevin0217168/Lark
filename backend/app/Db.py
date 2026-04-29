from sqlalchemy import create_engine, event
from sqlalchemy.orm import sessionmaker, Session, declarative_base

from datacontrol.UserDb import *
from datacontrol.DeviceDb import *
from datacontrol.SensorDb import *
from datacontrol.FirmwareDb import *
from datacontrol.DeviceLogDb import *
from datacontrol.BirdDb import *
from datacontrol.SensorUploadDb import *
from datacontrol.FeedingDb import *

from Logset import async_log, logger

SQLALCHEMY_DATABASE_URL = "sqlite:///../database/lark.db"

engine = create_engine(
  SQLALCHEMY_DATABASE_URL,
  connect_args={"check_same_thread": False},
  pool_size=15,
  max_overflow=0,
  pool_timeout=3,
)

# SQLite 性能优化: WAL 模式 + synchronous=NORMAL
# WAL: 读写互不阻塞, commit 延迟从 ~70ms 降到 ~1-5ms
# NORMAL: WAL 下仍安全, 仅 OS 级崩溃可能丢最后一个事务
# 这里 pool_size=3 且 max_overflow=0，保持连接数与并发限制一致，避免高并发时阻塞
@event.listens_for(engine, "connect")
def _set_sqlite_pragma(dbapi_conn, connection_record):
    cursor = dbapi_conn.cursor()
    cursor.execute("PRAGMA journal_mode=WAL")
    cursor.execute("PRAGMA synchronous=NORMAL")
    cursor.close()

SessionLocal = sessionmaker(
  autocommit=False, autoflush=False, bind=engine
)

UserBase.metadata.create_all(bind=engine)
DeviceBase.metadata.create_all(bind=engine)
SensorDataBase.metadata.create_all(bind=engine)
FirmwareBase.metadata.create_all(bind=engine)
DeviceLogBase.metadata.create_all(bind=engine)
BirdBase.metadata.create_all(bind=engine)
SensorUploadBase.metadata.create_all(bind=engine)
FeedingBase.metadata.create_all(bind=engine)

class OpenDb:
  def __init__(self, logtag):
    self.tag = logtag

  def __enter__(self):
    logger.debug(f"Db: [{self.tag}]: 打开数据库")
    self.db = SessionLocal()
    return self.db
  
  def __exit__(self, exc_type, exc, tb):
    self.db.close()
    logger.debug(f"Db: [{self.tag}]: 数据库已关闭")


def GetDb(tag: str):
  def dependency():
    with OpenDb(tag) as db:
      yield db
  return dependency


if __name__ == "__main__":
  with OpenDb("test1") as db:
    c = GetUsers(db=db, id=None)
    logger.info(c)
    
  for i in GetDb("test2")():
    c = GetUsers(db=db, id=None)
    logger.info(c)
