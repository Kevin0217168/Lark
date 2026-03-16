from fastapi import APIRouter, HTTPException, Response
from fastapi import status, Path, Query, Depends, Body, Cookie
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm
from datetime import datetime, timedelta, timezone
from .schema import *

import Db
import Security

router = APIRouter(prefix="/login", tags=["Login"])

@router.post("", description="用户登录",
            responses={**R200_LOGIN_SUCCESS, **R400_LOGIN_INCORRECT})
def login(form_data: Annotated[OAuth2PasswordRequestForm, Depends()],
          response:Response,
          db:Db.Session = Depends(Db.GetDb("login")),):
  data = Db.GetUsers(db, username = form_data.username)
  if len(data) == 1:
    data = data.pop()
    if Db.VerifyPassword(form_data.password, data.password):
      token = Security.CreateAccessToken({"username": form_data.username,"sub": "token"})
      long_token = Security.CreateAccessToken({"username": form_data.username,"sub": "cookie"}, timedelta(days=1))
      response.set_cookie(
        key="long_token",
        value=long_token,
        max_age=86400,       # 有效期1天（秒）
        secure=True,         # 仅HTTPS传输
        httponly=True,       # 禁止JS访问
      )
      return {"access_token": token, "token_type": "bearer"}
    return CommonOut(code=400, msg="Incorrect username or password", data=None)
  else:
    return CommonOut(code=400, msg="Incorrect username or password", data=None)

@router.get("")
async def read_users_me(
  op:Annotated[Db.M_Users, Depends(Security.GetCurrentUserByCookie)],
  long_token: Annotated[str, Cookie()]
):
  token = Security.CreateAccessToken({"username": op.username,"sub": "token"})
  return {"access_token": token, "token_type": "bearer"}