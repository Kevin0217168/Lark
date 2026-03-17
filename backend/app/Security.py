from fastapi import HTTPException
from fastapi import status, Path, Query, Depends, Body, Cookie
from pydantic import BaseModel
from typing import Annotated, List
import jwt
from jwt.exceptions import InvalidTokenError
from datetime import datetime, timedelta, timezone
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm

import Db

oauth2_scheme = OAuth2PasswordBearer(tokenUrl="api/login")

# to get a string like this run:
# openssl rand -hex 32
SECRET_KEY = "289313931e02a200f1b5288395d36515d8daa90b279202f879a6da680b974975"
ALGORITHM = "HS256"
ACCESS_TOKEN_EXPIRE_MINUTES = 30


def VerifyToken(db:Db.Session, token:str, credentials_exception:HTTPException) -> (Db.M_Users):
  try:
      payload = jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
      username = payload.get("username")
      sub = payload.get("sub")
      print(f"解码token: username:{username} sub:{sub}")
      if username is None:
        raise credentials_exception
      if credentials_exception.detail == "Could not validate credentials" and sub == "cookie":
        raise credentials_exception
  except InvalidTokenError:
    print("token解码失败")
    raise credentials_exception
  users = Db.GetUsers(db, username=username)
  if users is None:
      raise credentials_exception
  return users.pop()

async def GetCurrentUser(token: Annotated[str, Depends(oauth2_scheme)],
                         db:Db.Session = Depends(Db.GetDb("GetCurrentUser"))):
  credentials_exception = HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Could not validate credentials",
        headers={"WWW-Authenticate": "Bearer"},
    )
  return VerifyToken(db, token, credentials_exception)
  

async def GetCurrentUserByCookie(long_token:Annotated[str | None, Cookie()] = None,
                                db:Db.Session = Depends(Db.GetDb("GetCurrentUserByCookie"))):
  credentials_exception = HTTPException(
      status_code=status.HTTP_401_UNAUTHORIZED,
      detail="Cookies do not match.",
  )
  return VerifyToken(db, long_token, credentials_exception)
  

def CreateAccessToken(data: dict, expires_delta: timedelta | None = None):
  to_encode = data.copy()
  if expires_delta:
      expire = datetime.now(timezone.utc) + expires_delta
  else:
      expire = datetime.now(timezone.utc) + timedelta(minutes=15)
  to_encode.update({"exp": expire})
  encoded_jwt = jwt.encode(to_encode, SECRET_KEY, algorithm=ALGORITHM)
  return encoded_jwt