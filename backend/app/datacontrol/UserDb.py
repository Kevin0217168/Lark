from pydantic import BaseModel
from sqlalchemy import Column, Integer, String, DateTime, ForeignKey, Index, Boolean
from sqlalchemy.dialects.sqlite import JSON
from sqlalchemy.orm import relationship
from typing import Optional, Literal, List, Dict, Any
from sqlalchemy.orm import sessionmaker, Session, declarative_base
from pwdlib import PasswordHash
from datetime import datetime, timezone, timedelta

password_hash = PasswordHash.recommended()

UserBase = declarative_base()

# 获取本地时间（UTC+8），使用时区感知时间
# 兼容后端Token时间校验与未来 Pydantic/SQLAlchemy 时区行为
def get_local_time():
    return datetime.now(timezone.utc) + timedelta(hours=8)


def VerifyPassword(plain_password, hashed_password):
    return password_hash.verify(plain_password, hashed_password)

def ConvertHash(password):
    return password_hash.hash(password)

class M_Users(UserBase):
  __tablename__ = "users"
  
  id = Column(Integer, primary_key=True, index=True) # index = True 创建索引, 方便查询
  username = Column(String, unique=True, index=True)
  password = Column(String)
  nickname = Column(String)
  role = Column(String)
  avatar = Column(String)
  banner = Column(String)
  email = Column(String)
  invitation_code = Column(String, index=True)  # 注册时使用的邀请码
  extra = Column(JSON, nullable=True, default=None)  # 额外信息，字典类型，默认为null
  
  # 关联邀请码
  invitation_codes = relationship("M_InvitationCodes", back_populates="user")

class M_InvitationCodes(UserBase):
  __tablename__ = "invitation_codes"

  id = Column(Integer, primary_key=True, index=True)  # 自增主键
  user_id = Column(Integer, ForeignKey("users.id"), nullable=False, index=True)  # 外键关联用户表
  created_at = Column(DateTime, nullable=False, default=get_local_time)  # 生成时间，默认当前本地时间
  code = Column(String, nullable=False, unique=True, index=True)  # 邀请码，唯一
  expire_at = Column(DateTime, nullable=False, index=True)  # 到期时间
  remaining_uses = Column(Integer, nullable=False, default=1, index=True)  # 可用剩余次数，默认1
  is_used = Column(Boolean, nullable=False, default=False, index=True)  # 是否已使用，默认False
  user_type = Column(String, nullable=False, default="clouduser", index=True)  # 邀请码对应的用户类型：user 或 clouduser

  # 关联用户
  user = relationship("M_Users", back_populates="invitation_codes")

  # 复合索引，提高查询性能
  __table_args__ = (
      Index('idx_expire_uses', 'expire_at', 'remaining_uses'),
      Index('idx_used_expire', 'is_used', 'expire_at'),  # 用于过期检查
  )

class UserOut(BaseModel):
  id: int
  username: str
  nickname: str
  role: Literal["root", "user", "clouduser"]
  email: str | None = None
  banner: str | None = None
  avatar: str | None = None
  invitation_code: str | None = None
  extra: Dict[str, Any] | None = None

  class Config:
    from_attributes = True
    
class InvitationCodeOut(BaseModel):
  id: int
  user_id: int
  created_at: datetime
  code: str
  expire_at: datetime
  remaining_uses: int
  is_used: bool
  user_type: str  # 邀请码对应的用户类型：user 或 clouduser

  class Config:
    from_attributes = True
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

def RegisterUser(db: Session, username:str, password:str, nickname:str, role:str, email:str=None, avatar:str=None, invitation_code:str=None, extra:Dict[str, Any]=None):
  # 计算密码哈希
  password = ConvertHash(password)
  
  new_user = M_Users(username = username, password=password, nickname=nickname, role=role, email=email, avatar=avatar, invitation_code=invitation_code, extra=extra)
  db.add(new_user)
  db.commit()
  db.refresh(new_user)
  return new_user

def UpdateUser(db: Session, id:int, username:str=None, password:str=None, nickname:str=None, role:str=None, email:str=None, avatar:str=None, banner:str=None, extra:Dict[str, Any]=None):
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
  
  # 更新额外信息字段
  if extra is not None:
      user.extra = extra

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


def CreateInvitationCode(db: Session, user_id: int, code: str, expire_at: datetime, remaining_uses: int = 1, user_type: str = "clouduser"):
  """
  创建邀请码

  参数:
    db: 数据库会话
    user_id: 生成邀请码的用户ID
    code: 邀请码字符串
    expire_at: 到期时间
    remaining_uses: 可用次数，默认1
    user_type: 用户类型，user 或 clouduser

  返回:
    M_InvitationCodes: 新创建的邀请码对象
  """
  new_code = M_InvitationCodes(
      user_id=user_id,
      code=code,
      expire_at=expire_at,
      remaining_uses=remaining_uses,
      user_type=user_type
  )
  db.add(new_code)
  db.commit()
  db.refresh(new_code)
  return new_code


def GetInvitationCode(db: Session, code: str):
  """
  根据邀请码获取邀请码对象
  
  参数:
    db: 数据库会话
    code: 邀请码字符串
  
  返回:
    M_InvitationCodes: 邀请码对象，不存在返回None
  """
  return db.query(M_InvitationCodes).filter(M_InvitationCodes.code == code).first()


def GetInvitationCodesByUser(db: Session, user_id: int):
  """
  获取用户生成的所有邀请码
  
  参数:
    db: 数据库会话
    user_id: 用户ID
  
  返回:
    List[M_InvitationCodes]: 邀请码列表
  """
  return db.query(M_InvitationCodes).filter(M_InvitationCodes.user_id == user_id).all()


def UseInvitationCode(db: Session, code: str):
  """
  使用邀请码（减少可用次数）
  
  参数:
    db: 数据库会话
    code: 邀请码字符串
  
  返回:
    bool: 使用成功返回True，失败返回False
  """
  invitation_code = db.query(M_InvitationCodes).filter(
      M_InvitationCodes.code == code,
      M_InvitationCodes.remaining_uses > 0,
      M_InvitationCodes.expire_at > get_local_time()
  ).first()
  
  if not invitation_code:
    return False
  
  invitation_code.remaining_uses -= 1
  # 只要使用过一次就标记为已使用
  invitation_code.is_used = True
  db.commit()
  return True


def CleanupExpiredInvitationCodes(db: Session):
  """
  清理过期的邀请码
  
  仅删除：未被使用（is_used=False）且已过期的邀请码
  保留：已被使用（is_used=True）即使已过期的邀请码
  
  参数:
    db: 数据库会话
  
  返回:
    int: 删除的邀请码数量
  """
  try:
    # 查找未被使用且已过期的邀请码
    expired_codes = db.query(M_InvitationCodes).filter(
        M_InvitationCodes.is_used == False,
        M_InvitationCodes.expire_at < get_local_time()
    ).all()
    
    deleted_count = 0
    for code in expired_codes:
      db.delete(code)
      deleted_count += 1
    
    db.commit()
    return deleted_count
    
  except Exception as e:
    db.rollback()
    raise


def GetInvitationCodeStats(db: Session):
  """
  获取邀请码统计信息
  
  参数:
    db: 数据库会话
  
  返回:
    dict: 统计信息
  """
  total = db.query(M_InvitationCodes).count()
  used = db.query(M_InvitationCodes).filter(M_InvitationCodes.is_used == True).count()
  unused = total - used
  
  expired_unused = db.query(M_InvitationCodes).filter(
      M_InvitationCodes.is_used == False,
      M_InvitationCodes.expire_at < get_local_time()
  ).count()
  
  return {
      "total": total,
      "used": used,
      "unused": unused,
      "expired_unused": expired_unused
  }


def DeleteInvitationCode(db: Session, code: str):
  """
  删除邀请码
  
  参数:
    db: 数据库会话
    code: 邀请码字符串
  
  返回:
    bool: 删除成功返回True，失败返回False
  """
  invitation_code = db.query(M_InvitationCodes).filter(M_InvitationCodes.code == code).first()
  if not invitation_code:
    return False
  
  db.delete(invitation_code)
  db.commit()
  return True


def AdoptBird(db: Session, user_id: int, bird_id: int) -> dict:
  """
  用户认领雏鸟
  
  每个用户只能认领一只雏鸟，认领后会更新用户的extra字段，添加已认领雏鸟的信息
  
  参数:
    db: 数据库会话
    user_id: 用户ID
    bird_id: 雏鸟ID
  
  返回:
    dict: 包含操作结果和相关信息的字典
  """
  from .BirdDb import GetBirdById, UpdateBirdStatus
  
  # 1. 检查用户是否存在
  user = db.query(M_Users).filter(M_Users.id == user_id).first()
  if not user:
    return {"success": False, "message": "用户不存在"}
  
  # 2. 检查用户是否已经认领了雏鸟
  if user.extra and user.extra.get("adopted_bird"):
    return {"success": False, "message": "您已经认领了一只雏鸟，不能再认领"}
  
  # 3. 检查雏鸟是否存在
  bird = GetBirdById(db, bird_id)
  if not bird:
    return {"success": False, "message": "雏鸟不存在"}
  
  # 4. 检查雏鸟是否可认领
  if bird.status != "available":
    return {"success": False, "message": "该雏鸟不可认领"}
  
  # 5. 更新用户的extra字段，添加已认领雏鸟信息
  # 使用 dict() 创建副本，确保 SQLAlchemy 能可靠追踪变更
  extra = dict(user.extra) if user.extra else {}
  extra["adopted_bird"] = {
    "bird_id": bird.id,
    "bird_name": bird.name,
    "bird_species": bird.species,
    "birth_date": bird.birth_date.isoformat() if bird.birth_date else None,
    "description": bird.description,
    "adopted_at": get_local_time().isoformat()
  }

  user.extra = dict(extra)

  # 6. 更新雏鸟状态为已认领
  updated_bird = UpdateBirdStatus(db, bird_id, "adopted")
  if not updated_bird:
    db.rollback()
    return {"success": False, "message": "更新雏鸟状态失败"}
  
  # 7. 提交事务
  db.commit()
  
  return {
    "success": True,
    "message": "认领雏鸟成功",
    "adopted_bird": extra["adopted_bird"]
  }


def GetUserAdoptedBird(db: Session, user_id: int) -> dict:
  """
  获取用户已认领的雏鸟信息
  
  参数:
    db: 数据库会话
    user_id: 用户ID
  
  返回:
    dict: 已认领雏鸟的信息，如果没有则返回空字典
  """
  user = db.query(M_Users).filter(M_Users.id == user_id).first()
  if not user or not user.extra:
    return {}
  
  return user.extra.get("adopted_bird", {})


def GetUserByAdoptedBird(db: Session, bird_id: int) -> Optional[M_Users]:
  """
  根据雏鸟ID查找认领该雏鸟的用户
  
  参数:
    db: 数据库会话
    bird_id: 雏鸟ID
  
  返回:
    M_Users: 认领该雏鸟的用户对象，未找到返回None
  """
  # 查找extra字段中adopted_bird包含指定bird_id的用户
  # 注意：这是一个简单的实现，实际应用中可能需要更复杂的查询
  users = db.query(M_Users).all()
  for user in users:
    if user.extra and user.extra.get("adopted_bird"):
      adopted_bird = user.extra["adopted_bird"]
      if adopted_bird.get("bird_id") == bird_id:
        return user
  return None


def ReleaseAdoptedBird(db: Session, user_id: int) -> dict:
  """
  用户释放已认领的雏鸟
  
  参数:
    db: 数据库会话
    user_id: 用户ID
  
  返回:
    dict: 包含操作结果和相关信息的字典
  """
  # 1. 检查用户是否存在
  user = db.query(M_Users).filter(M_Users.id == user_id).first()
  if not user:
    return {"success": False, "message": "用户不存在"}
  
  # 2. 检查用户是否有已认领的雏鸟
  if not user.extra or not user.extra.get("adopted_bird"):
    return {"success": False, "message": "您没有已认领的雏鸟"}
  
  # 3. 获取已认领雏鸟的ID
  adopted_bird = user.extra["adopted_bird"]
  bird_id = adopted_bird.get("bird_id")
  
  if not bird_id:
    return {"success": False, "message": "已认领雏鸟信息不完整"}
  
  # 4. 更新雏鸟状态为可认领
  from .BirdDb import M_Birds
  bird = db.query(M_Birds).filter(M_Birds.id == bird_id).first()
  if not bird:
    db.rollback()
    return {"success": False, "message": "雏鸟不存在"}
  
  bird.status = "available"
  db.commit()
  db.refresh(bird)
  
  # 5. 从用户的extra字段中移除已认领雏鸟信息
  # 使用 dict() 创建副本，确保 SQLAlchemy 能可靠追踪变更
  extra = dict(user.extra) if user.extra else {}
  if "adopted_bird" in extra:
    del extra["adopted_bird"]
  user.extra = dict(extra) if extra else None
  
  # 6. 提交事务
  db.commit()
  
  return {
    "success": True,
    "message": "释放雏鸟成功",
    "released_bird": adopted_bird
  }
