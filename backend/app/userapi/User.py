from fastapi import APIRouter
from typing import Annotated, List
from fastapi import status, Path, Query, Depends, Body
from fastapi.responses import JSONResponse

from schema import *

import Db
import Security

router = APIRouter(prefix="/users", tags=["Users"])


@router.get("/me", response_model=CommonOut[Db.UserOut])
async def read_users_me(
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
):
    return CommonOut(data=current_user)


@router.get(
    "",
    description="查询符合条件的用户",
    response_model=CommonOut[List[Db.UserOut]],
    responses=R404_USER_NOT_FOUND,
)
async def get_users(
    filter_query: Annotated[UsersFilter, Query()],
    db: Db.Session = Depends(Db.GetDb("GetUsers")),
):
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
    description="获取指定id的用户",
    response_model=CommonOut[List[Db.UserOut]],
    responses=R404_USER_NOT_FOUND,
)
async def get_users(
    id: Annotated[int, Path(title="用户id", description="数据库用户唯一主键id")],
    db: Db.Session = Depends(Db.GetDb("GetUsers")),
):
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
    description="注册用户",
    response_model=CommonOut[Db.UserOut],
    responses=R400_USER_ALREADY_EXIST,
)
async def register_user(
    body: Annotated[UserItem, Body()],
    db: Db.Session = Depends(Db.GetDb("RegisterUser")),
):
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
    description="更新指定id的用户",
    response_model=CommonOut[Db.UserOut],
    responses=R404_USER_NOT_FOUND,
)
async def update_user(
    id: Annotated[int, Path(title="用户id", description="数据库用户唯一主键id")],
    body: Annotated[UserUpdateItem, Body()],
    db: Db.Session = Depends(Db.GetDb("UpdateUser")),
):
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


@router.delete(
    "/{id}",
    description="删除指定id的用户",
    response_model=CommonOut[Db.UserOut],
    responses={**R404_USER_NOT_FOUND, **R403_FORBIDDEN},
)
async def delete_user(
    id: Annotated[int, Path(title="用户id", description="数据库用户唯一主键id")],
    op: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("DeleteUser")),
):
    if op.role != "root":
        return JSONResponse(
            status_code=status.HTTP_403_FORBIDDEN,
            content=CommonOut(
                code=status.HTTP_403_FORBIDDEN, msg="Permission denied.", data=None
            ).model_dump(),
        )
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
