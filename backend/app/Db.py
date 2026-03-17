from pydantic import BaseModel
from typing import Optional, Literal, List

from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker, Session, declarative_base
from sqlalchemy import Column, Integer, String
from pwdlib import PasswordHash

password_hash = PasswordHash.recommended()

def VerifyPassword(plain_password, hashed_password):
    return password_hash.verify(plain_password, hashed_password)

def ConvertHash(password):
    return password_hash.hash(password)


SQLALCHEMY_DATABASE_URL = "sqlite:///../database/lark.db"

engine = create_engine(
  SQLALCHEMY_DATABASE_URL, connect_args={"check_same_thread":False}
)

SessionLocal = sessionmaker(
  autocommit=False, autoflush=False, bind=engine
)

Base = declarative_base()

class M_Users(Base):
  __tablename__ = "users"
  
  id = Column(Integer, primary_key=True, index=True) # index = True 创建索引, 方便查询
  username = Column(String, unique=True, index=True)
  password = Column(String)
  nickname = Column(String)
  role = Column(String)
  avatar = Column(String)
  banner = Column(String)
  email = Column(String)
  
class UserOut(BaseModel):
  id: int
  username: str
  nickname: str
  role: Literal["root", "user", "readonly"]
  email: str | None = None
  banner: str | None = None
  avatar: str | None = None

  class Config:
    from_attributes = True

Base.metadata.create_all(bind=engine)

class OpenDb:
  def __init__(self, logtag):
    self.tag = logtag

  def __enter__(self):
    print(f"Db: [{self.tag}]: 打开数据库")
    self.db = SessionLocal()
    return self.db
  
  def __exit__(self, exc_type, exc, tb):
    self.db.close()
    print(f"Db: [{self.tag}]: 数据库已关闭")


def GetDb(tag: str):
  def dependency():
    with OpenDb(tag) as db:
      yield db
  return dependency
    

def GetUsers(db: Session, id=None, username=None, nickname=None, role=None, email=None) -> (List[M_Users]): 
  conditions = []
  if id is not None:
    conditions.append(M_Users.id == id)
  if username is not None:
    conditions.append(M_Users.username == username)
  if nickname is not None:
    conditions.append(M_Users.nickname == nickname)
  if role is not None:
    conditions.append(M_Users.role == role)
  if email is not None:
    conditions.append(M_Users.email == email)
      
  users = db.query(M_Users).filter(*conditions).all()
  return users

def RegisterUser(db: Session, username:str, password:str, nickname:str, role:str, email:str=None, avatar:str=None):
  # 计算密码哈希
  password = ConvertHash(password)
  
  new_user = M_Users(username = username, password=password, nickname=nickname, role=role, email=email, avatar=avatar)
  db.add(new_user)
  db.commit()
  db.refresh(new_user)
  return new_user

def UpdateUser(db: Session, id:int, username:str=None, password:str=None, nickname:str=None, role:str=None, email:str=None, avatar:str=None, banner:str=None):
  # 查找用户
  user = db.query(M_Users).filter(M_Users.id == id).first()
  if not user:
    return None

  # 密码：非空字符串才更新
  if password is not None and password != "":
      user.password = ConvertHash(password)

  # 用户名：非空字符串才更新
  if username is not None and username != "":
      user.username = username

  # 昵称：非空字符串才更新
  if nickname is not None and nickname != "":
      user.nickname = nickname

  # 角色：非空字符串才更新
  if role is not None and role != "":
      user.role = role

  # 对于允许清空的字段：如果传入空字符串，则设置为 None；否则如果是其他非 None 值，则更新为该值；如果为 None，不更新
  if email is not None:
      user.email = email if email != "" else None
  if avatar is not None:
      user.avatar = avatar if avatar != "" else None
  if banner is not None:
      user.banner = banner if banner != "" else None

  db.commit()
  db.refresh(user)  # 刷新以获取数据库中的最新数据（如触发器生成的值）
  return user

def DeleteUser(db: Session, id:int):
  # 查找用户
  user = db.query(M_Users).filter(M_Users.id == id).first()
  if not user:
    return None
  
  db.delete(user)
  db.commit()
  return 1


if __name__ == "__main__":
  with OpenDb("test1") as db:
    c = GetUsers(db=db, id=None)
    print(c)
    
  for i in GetDb("test2")():
    c = GetUsers(db=db, id=None)
    print(c)
