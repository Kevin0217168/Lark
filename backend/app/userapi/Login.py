from fastapi import APIRouter, HTTPException, Response
from fastapi import status, Path, Query, Depends, Body, Cookie
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm
from fastapi.responses import JSONResponse
from datetime import datetime, timedelta, timezone
from schema import *

import Db
import Security

from Logset import async_log, logger

router = APIRouter(tags=["Login"])


@router.post(
    "/login",
    summary="用户登录",
    description="用户登录, 记录cookie, 并发放短token",
    responses={**R200_LOGIN_SUCCESS, **R400_LOGIN_INCORRECT},
)
def login(
    form_data: Annotated[OAuth2PasswordRequestForm, Depends()],
    response: Response,
    db: Db.Session = Depends(Db.GetDb("login")),
):
    data = Db.GetUsers(db, username=form_data.username)
    if len(data) == 1:
        data = data.pop()
        if Db.VerifyPassword(form_data.password, data.password):
            token = Security.CreateAccessToken(
                {"username": form_data.username, "sub": "token"}
            )
            long_token = Security.CreateAccessToken(
                {"username": form_data.username, "sub": "cookie"}, timedelta(days=1)
            )
            response.set_cookie(
                key="long_token",
                value=long_token,
                max_age=86400,  # 有效期1天（秒）
                secure=True,  # 仅HTTPS传输
                httponly=True,  # 禁止JS访问
            )
            return {"access_token": token, "token_type": "bearer"}
        return JSONResponse(
            status_code=400,
            content=CommonOut(
                code=400, msg="Incorrect username or password", data=None
            ).model_dump(),
        )
    else:
        return JSONResponse(
            status_code=400,
            content=CommonOut(
                code=400, msg="Incorrect username or password", data=None
            ).model_dump(),
        )


@router.post(
    "/refresh", summary="刷新Token", description="通过cookie验证登录状态, 刷新token", responses=R200_LOGIN_SUCCESS
)
async def RefreshToken(
    op: Annotated[Db.M_Users, Depends(Security.GetCurrentUserByCookie)],
    long_token: Annotated[str, Cookie()],
):
    token = Security.CreateAccessToken({"username": op.username, "sub": "token"})
    return {"access_token": token, "token_type": "bearer"}

@router.post(
    "/logout",
    response_model=CommonOut[Db.UserOut],
    summary="用户退出登录",
)
async def logout_user( 
    response: Response,
    op: Annotated[Db.M_Users, Depends(Security.GetCurrentUserByCookie)],
):
    """
    # 用户单点退出登录(使本机cookie失效)
    
    退出登录成功返回退出的用户信息
    """
    
    response.delete_cookie(key="long_token",
                secure=True,  # 仅HTTPS传输
                httponly=True,  # 禁止JS访问
                )
    return CommonOut(code=200, msg="Logout.", data=op)
