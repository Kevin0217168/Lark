from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker, Session, declarative_base

from datacontrol.UserDb import *
from datacontrol.DeviceDb import *
from datacontrol.SensorDb import *

from Logset import async_log, logger

SQLALCHEMY_DATABASE_URL = "sqlite:///../database/lark.db"

engine = create_engine(
  SQLALCHEMY_DATABASE_URL, connect_args={"check_same_thread":False}
)

SessionLocal = sessionmaker(
  autocommit=False, autoflush=False, bind=engine
)

UserBase.metadata.create_all(bind=engine)
DeviceBase.metadata.create_all(bind=engine)
SensorDataBase.metadata.create_all(bind=engine)

class OpenDb:
  def __init__(self, logtag):
    self.tag = logtag

  def __enter__(self):
    logger.info(f"Db: [{self.tag}]: 打开数据库")
    self.db = SessionLocal()
    return self.db
  
  def __exit__(self, exc_type, exc, tb):
    self.db.close()
    logger.info(f"Db: [{self.tag}]: 数据库已关闭")


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
