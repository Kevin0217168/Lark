from fastapi import APIRouter
from typing import Annotated, List
from fastapi import status, Path, Query, Depends, Body
from fastapi.responses import JSONResponse

from schema import *

import Db
import Security

from Logset import async_log, logger

router = APIRouter(prefix="/users", tags=["Users"])


@router.get("/me", response_model=CommonOut[Db.UserOut], summary="获取当前登录用户信息")
async def read_users_me(
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
):
    """
    # 根据请求头获取当前登录的用户信息
    """

    return CommonOut(data=current_user)


@router.get(
    "",
    response_model=CommonOut[List[Db.UserOut]],
    responses=R404_USER_NOT_FOUND,
    summary="查询用户列表",
)
async def get_users(
    filter_query: Annotated[UsersFilter, Query()],
    db: Db.Session = Depends(Db.GetDb("GetUsers")),
):

    """
    # 查询符合条件的用户列表
    
    结果为空返回404
    """
    data = Db.GetUsers(db, **filter_query.model_dump())
    if len(data) == 0:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="User not found", data=None).model_dump(),
        )
    else:
        return CommonOut(data=data)


@router.get(
    "/{id}",
    response_model=CommonOut[List[Db.UserOut]],
    responses=R404_USER_NOT_FOUND,
    summary="根据ID查询单个用户",
)
async def get_users(
    id: Annotated[int, Path(title="用户id", description="数据库用户唯一主键id")],
    db: Db.Session = Depends(Db.GetDb("GetUsers")),
):
    """
    # 根据ID获取单个用户
    
    结果为空返回404
    """
    data = Db.GetUsers(db, id=id)
    if len(data) == 0:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="User not found", data=None).model_dump(),
        )
    else:
        return CommonOut(data=data)


@router.post(
    "",
    response_model=CommonOut[Db.UserOut],
    responses=R400_USER_ALREADY_EXIST,
    summary="注册新用户",
)
async def register_user(
    body: Annotated[UserItem, Body()],
    db: Db.Session = Depends(Db.GetDb("RegisterUser")),
):
    """
    # 注册新用户 (用户名唯一)
    规则不符返回422, 用户重复返回400, 注册成功返回当前用户信息
    
    ## 后端验证规则
    - 用户名 长度3-20 只能包含字母、数字、下划线，且不能以下划线开头或结尾
    - 密码 长度8-32 必须包含至少一个数字或字母
    - 用户昵称 长度1-50
    - 用户权限 必须是["root", "user", "readonly"]其中的一个
    - 图片地址必须以 http:// 或 https:// 开头
    
    """
    if len(Db.GetUsers(db, username=body.username)):
        # 用户已存在
        return JSONResponse(
            status_code=status.HTTP_400_BAD_REQUEST,
            content=CommonOut(
                code=status.HTTP_400_BAD_REQUEST, msg="User already exist.", data=None
            ).model_dump(),
        )

    # 直接注册
    return CommonOut(data=Db.RegisterUser(db, **body.model_dump(exclude_unset=True)))


@router.put(
    "/{id}",
    response_model=CommonOut[Db.UserOut],
    responses=R404_USER_NOT_FOUND,
    summary="更新用户信息",
)
async def update_user(
    id: Annotated[int, Path(title="用户id", description="数据库用户唯一主键id")],
    body: Annotated[UserUpdateItem, Body()],
    db: Db.Session = Depends(Db.GetDb("UpdateUser")),
):
    """
    # 使用唯一id更新用户信息
    用户不存在返回404, 规则不符返回422, 更改成功返回修改后的用户信息
    
    ## 后端验证规则
    - 用户名 长度3-20 只能包含字母、数字、下划线，且不能以下划线开头或结尾
    - 密码 长度8-32 必须包含至少一个数字或字母
    - 用户昵称 长度1-50
    - 用户权限 必须是["root", "user", "readonly"]其中的一个
    - 图片地址必须以 http:// 或 https:// 开头
    
    ## 更新规则 (优先满足验证规则)
    - 用户名, 密码, 昵称不允许改为空字符串
    - 若传入 NULL 则不更新；若传入空字符串则置为 NULL; 否则更新为新值。
    """
    data = Db.UpdateUser(db, id=id, **body.model_dump(exclude_unset=True))
    if data:
        # 用户唯一
        return CommonOut(data=data)
    else:
        # 用户不存在
        return JSONResponse(
            status_code=status.HTTP_404_NOT_FOUND,
            content=CommonOut(
                code=status.HTTP_404_NOT_FOUND, msg="User not found.", data=None
            ).model_dump(),
        )

from Logset import async_log, logger
@router.delete(
    "/{id}",
    response_model=CommonOut[Db.UserOut],
    responses={**R404_USER_NOT_FOUND, **R403_FORBIDDEN},
    summary="删除用户",
)
async def delete_user(
    id: Annotated[int, Path(title="用户id", description="数据库用户唯一主键id")],
    op: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("DeleteUser")),
):
    """
    # 使用唯一id删除用户
    
    
    ## 后端规则
    - user只能删除自己(用于注销)
    - root可以删除所有
    """

    if op.id != id and op.role != "root":
        return JSONResponse(
            status_code=status.HTTP_403_FORBIDDEN,
            content=CommonOut(
                code=status.HTTP_403_FORBIDDEN, msg="Permission denied.", data=None
            ).model_dump(),)
    data = Db.DeleteUser(db, id=id)
    if data:
        # 用户唯一
        return CommonOut(data=None)
    else:
        # 用户不存在
        return JSONResponse(
            status_code=status.HTTP_404_NOT_FOUND,
            content=CommonOut(
                code=status.HTTP_404_NOT_FOUND, msg="User not found.", data=None
            ).model_dump(),
        )
