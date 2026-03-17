import re
from typing import Annotated, List, Optional, Literal, Generic, TypeVar
from pydantic import BaseModel, Field, field_validator

T = TypeVar('T')

class CommonOut(BaseModel, Generic[T]):
    code: int = Field(default=200)
    msg: str = Field(default="successful!")
    data: Optional[T] = None


class UsersFilter(BaseModel):
  model_config = {"extra": "forbid"}
  
  id:Optional[int] = Field(title="用户id", description="数据库用户唯一主键id", default=None)
  username:Optional[str] = Field(title="用户名", description="数据库唯一用户名", default=None)
  nickname:Optional[str] = Field(title="用户昵称", description="数据库用户昵称", default=None)
  role:Optional[Literal["root", "user", "readonly"]] = Field(title="用户权限", description="数据库用户权限", default=None)
  email:Optional[str] = Field(title="用户邮箱", description="数据库用户邮箱", default=None)
  

class UserItem(BaseModel):
  username: str = Field(
      title="用户名",
      description="数据库唯一用户名",
      examples=["mint"],
      min_length=3,
      max_length=20,
      pattern=r"^[a-zA-Z0-9_]+$"  # 只允许字母、数字、下划线
  )
  password: str = Field(
      title="用户密码",
      description="用户密码",
      examples=["123456789"],
      min_length=8,
      max_length=32
  )
  nickname: str = Field(
      title="用户昵称",
      description="数据库用户昵称(不唯一)",
      examples=["猫猫"],
      min_length=1,
      max_length=50
  )
  role: Literal["root", "user", "readonly"] = Field(
      title="用户权限",
      description="数据库用户权限",
      examples=["root"]
  )
  avatar: Optional[str] = Field(
      title="用户头像",
      description="用户头像图片访问地址",
      examples=[""],
      default=None
  )
  
  @field_validator("password")
  @classmethod
  def validate_password_strength(cls, v: Optional[str]) -> Optional[str]:
      if v is None:
          return v
      # 密码必须包含至少一个数字和一个字母
      if not re.search(r"\d", v):
          raise ValueError("密码必须包含至少一个数字")
      if not re.search(r"[a-zA-Z]", v):
          raise ValueError("密码必须包含至少一个字母")
      return v

  @field_validator("username")
  @classmethod
  def validate_username_format(cls, v: Optional[str]) -> Optional[str]:
      if v is None:
          return v
      # 用户名只能包含字母、数字、下划线，且不能以下划线开头或结尾
      if not re.match(r"^[a-zA-Z0-9][a-zA-Z0-9_]*[a-zA-Z0-9]$", v):
          raise ValueError("用户名只能包含字母、数字、下划线，且不能以下划线开头或结尾")
      return v

  @field_validator("avatar")
  @classmethod
  def validate_image_url(cls, v: Optional[str]) -> Optional[str]:
      if v == None or v == "":
        return v
      # 简单的 URL 格式校验（可扩展为更严格的检查）
      if not v.startswith(("http://", "https://")):
        raise ValueError("图片地址必须以 http:// 或 https:// 开头")
      # 可继续校验文件扩展名等
      return v


class UserUpdateItem(BaseModel):
  username:str = Field(title="用户名", description="数据库唯一用户名", examples=["mint"], default=None)
  password:str = Field(title="用户密码", description="用户密码", examples=["123456789"], default=None)
  nickname:str = Field(title="用户昵称", description="数据库用户昵称(不唯一)", examples=["猫猫"], default=None)
  role:Literal["root", "user", "readonly"] = Field(title="用户权限", description="数据库用户权限", examples=["root"], default=None)
  avatar:str = Field(title="用户头像", description="用户头像图片访问地址", examples=[""], default=None)
  banner:str = Field(title="用户自定义主图", description="用户自定义主图图片访问地址", examples=[""], default=None)
  email:str = Field(title="用户邮箱", description="数据库用户邮箱", examples=["BI6PPD@163.com"], default=None)
  
  @field_validator("password")
  @classmethod
  def validate_password_strength(cls, v: Optional[str]) -> Optional[str]:
      if v is None:
          return v
      # 密码必须包含至少一个数字和一个字母
      if not re.search(r"\d", v):
          raise ValueError("密码必须包含至少一个数字")
      if not re.search(r"[a-zA-Z]", v):
          raise ValueError("密码必须包含至少一个字母")
      return v

  @field_validator("username")
  @classmethod
  def validate_username_format(cls, v: Optional[str]) -> Optional[str]:
      if v is None:
          return v
      # 用户名只能包含字母、数字、下划线，且不能以下划线开头或结尾
      if not re.match(r"^[a-zA-Z0-9][a-zA-Z0-9_]*[a-zA-Z0-9]$", v):
          raise ValueError("用户名只能包含字母、数字、下划线，且不能以下划线开头或结尾")
      return v

  @field_validator("avatar", "banner")
  @classmethod
  def validate_image_url(cls, v: Optional[str]) -> Optional[str]:
      if v is None:
          return v
      # 简单的 URL 格式校验（可扩展为更严格的检查）
      if not v.startswith(("http://", "https://")):
          raise ValueError("图片地址必须以 http:// 或 https:// 开头")
      # 可继续校验文件扩展名等
      return v
  

class LoginItem(BaseModel):
  username:str = Field(title="用户名", description="数据库唯一用户名", examples=["mint"])
  password:str = Field(title="用户密码", description="用户密码", examples=["123456789"])

R404_USER_NOT_FOUND = {404: {
              "model": CommonOut[None],
              "description": "User not found",
              "content": {
                "application/json": {
                  "example": {"code": 404, "msg": "User not found"}
                }
              }
            }
          }

R400_USER_ALREADY_EXIST = {400: {
              "model": CommonOut[None],
              "description": "User already exist.",
              "content": {
                "application/json": {
                  "example": {"code": 400, "msg": "User already exist."}
                }
              }
            }
          }

R400_USER_IS_NOT_UNIQUE = {400: {
              "model": CommonOut[None],
              "description": "User is not unique.",
              "content": {
                "application/json": {
                  "example": {"code": 400, "msg": "User is not unique."}
                }
              }
            }
          }

R400_LOGIN_INCORRECT = {400: {
              "model": CommonOut[None],
              "description": "Incorrect username or password.",
              "content": {
                "application/json": {
                  "example": {"code": 400, "msg": "Incorrect username or password"}
                }
              }
            }
          }

R200_LOGIN_SUCCESS = {200: {
              "description": "Login is successful.",
              "content": {
                "application/json": {
                  "example": {"access_token": "token", "token_type": "bearer"}
                }
              }
            }
          }

R403_FORBIDDEN = {403: {
              "description": "Permission denied.",
              "content": {
                "application/json": {
                  "example": {"code": 403, "msg": "Permission denied."}
                }
              }
            }
          }