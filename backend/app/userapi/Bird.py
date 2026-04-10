"""
Bird.py - 雏鸟管理 API 模块

提供雏鸟信息的 RESTful API 接口，包括：
- 创建雏鸟（仅 root 用户）
- 查询雏鸟列表
- 获取单只雏鸟详情
- 更新雏鸟信息（仅 root 用户）
- 删除雏鸟（仅 root 用户）

所有修改类操作仅限 root 用户执行。
"""

from fastapi import APIRouter, Depends, HTTPException, status, Body, Path, Query
from typing import Annotated, List, Optional
from fastapi.responses import JSONResponse
from datetime import datetime

from schema import CommonOut
import Db
import Security
from Logset import async_log, logger

router = APIRouter(prefix="/birds", tags=["Birds"])


# 响应定义
R404_BIRD_NOT_FOUND = {
    status.HTTP_404_NOT_FOUND: {
        "model": CommonOut,
        "description": "雏鸟不存在",
    }
}

R403_FORBIDDEN = {
    status.HTTP_403_FORBIDDEN: {
        "model": CommonOut,
        "description": "权限不足，仅限 root 用户",
    }
}

R400_BAD_REQUEST = {
    status.HTTP_400_BAD_REQUEST: {
        "model": CommonOut,
        "description": "请求参数错误",
    }
}


# 依赖函数：验证当前用户是否为 root
def require_root_user(
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
) -> Db.M_Users:
    """
    验证当前用户是否为 root 管理员
    
    Args:
        current_user: 当前登录用户
    
    Returns:
        Db.M_Users: 验证通过的用户对象
    
    Raises:
        HTTPException: 403 权限不足
    """
    if current_user.role != "root":
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="权限不足，仅限 root 用户执行此操作",
        )
    return current_user


@router.post(
    "",
    response_model=CommonOut[Db.BirdOut],
    responses={**R400_BAD_REQUEST, **R403_FORBIDDEN},
    summary="创建新雏鸟",
)
async def create_bird(
    body: Annotated[Db.BirdCreate, Body(
        examples=[
            {
                "name": "白面鸮",
                "species": "白脸角鸮",
                "birth_date": "2025-10-06",
                "device_id": 1,
                "description": "干员白面鸮，前莱茵生命公司，数据维护专员。在医疗类源石技艺领域取得不菲成就，于医疗数据维护，常规医疗方案应用，多项目医疗行为等相关领域，拥有丰富经验。",
                "avatar_url": "https://storage.moegirl.org.cn/moegirl/commons/a/a9/%E6%98%8E%E6%97%A5%E6%96%B9%E8%88%9F%E7%AB%8B%E7%BB%98_%E7%99%BD%E9%9D%A2%E9%B8%AE_1.png"
            }
        ]
    )],
    current_user: Annotated[Db.M_Users, Depends(require_root_user)],
    db: Db.Session = Depends(Db.GetDb("CreateBird")),
):
    """
    # 创建新雏鸟记录
    
    仅限 root 用户调用此接口创建新的雏鸟信息。
    
    ## 请求参数
    - **name**: 雏鸟名称，长度 1-50 字符
    - **species**: 品种，如"鹦鹉"、"鸽子"、"文鸟"
    - **birth_date**: 出生日期，ISO 格式（YYYY-MM-DD）
    - **device_id**: 关联设备 ID，可选
    - **description**: 雏鸟描述，可选
    - **avatar_url**: 照片 URL，可选
    
    ## 请求示例
    ```json
    {
        "name": "白面鸮",
        "species": "白脸角鸮",
        "birth_date": "2025-10-06",
        "device_id": 1,
        "description": "干员白面鸮，前莱茵生命公司，数据维护专员。在医疗类源石技艺领域取得不菲成就，于医疗数据维护，常规医疗方案应用，多项目医疗行为等相关领域，拥有丰富经验。",
        "avatar_url": "https://storage.moegirl.org.cn/moegirl/commons/a/a9/%E6%98%8E%E6%97%A5%E6%96%B9%E8%88%9F%E7%AB%8B%E7%BB%98_%E7%99%BD%E9%9D%A2%E9%B8%AE_1.png"
    }
    ```
    
    ## 响应
    - 成功：返回 200 和创建的雏鸟信息
    - 失败：返回 400 参数错误或 403 权限不足
    """
    try:
        # 创建雏鸟记录
        new_bird = Db.CreateBird(
            db=db,
            name=body.name,
            species=body.species,
            birth_date=body.birth_date,
            device_id=body.device_id,
            description=body.description,
            avatar_url=body.avatar_url,
            status="available",  # 默认状态为可认领
        )
        
        await async_log(
            logger, 
            "info", 
            f"创建雏鸟成功: {new_bird.name} (ID: {new_bird.id}) - 操作者: {current_user.username}"
        )
        
        # 转换为输出模型
        bird_out = Db.BirdOut.from_orm(new_bird)
        return CommonOut(data=bird_out)
        
    except ValueError as e:
        # 日期格式错误等
        await async_log(logger, "warning", f"创建雏鸟失败: 参数错误 - {str(e)}")
        return JSONResponse(
            status_code=status.HTTP_400_BAD_REQUEST,
            content=CommonOut(
                code=status.HTTP_400_BAD_REQUEST,
                msg=f"参数错误: {str(e)}",
                data=None,
            ).model_dump(),
        )
    except Exception as e:
        await async_log(logger, "error", f"创建雏鸟失败: {str(e)}")
        return JSONResponse(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            content=CommonOut(
                code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                msg="服务器内部错误",
                data=None,
            ).model_dump(),
        )


@router.get(
    "",
    response_model=CommonOut[List[Db.BirdOut]],
    summary="查询雏鸟列表",
)
async def get_birds(
    name: Optional[str] = Query(None, description="按名称模糊查询"),
    species: Optional[str] = Query(None, description="按品种查询"),
    status: Optional[str] = Query(None, description="按状态查询：available/adopted/grown"),
    device_id: Optional[int] = Query(None, description="按设备ID查询"),
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)] = None,
    db: Db.Session = Depends(Db.GetDb("GetBirds")),
):
    """
    # 查询雏鸟列表
    
    支持多条件组合查询，所有参数均为可选。
    
    ## 查询参数
    - **name**: 按名称模糊匹配
    - **species**: 按品种精确匹配
    - **status**: 按状态精确匹配（available/adopted/grown）
    - **device_id**: 按关联设备ID精确匹配
    
    ## 响应
    - 成功：返回 200 和雏鸟列表（可能为空列表）
    """
    try:
        birds = Db.GetBirds(
            db=db,
            name=name,
            species=species,
            status=status,
            device_id=device_id,
        )
        
        # 转换为输出模型列表
        birds_out = [Db.BirdOut.from_orm(bird) for bird in birds]
        return CommonOut(data=birds_out)
        
    except Exception as e:
        await async_log(logger, "error", f"查询雏鸟列表失败: {str(e)}")
        return JSONResponse(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            content=CommonOut(
                code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                msg="服务器内部错误",
                data=None,
            ).model_dump(),
        )


@router.get(
    "/{bird_id}",
    response_model=CommonOut[Db.BirdOut],
    responses=R404_BIRD_NOT_FOUND,
    summary="获取单只雏鸟详情",
)
async def get_bird_by_id(
    bird_id: Annotated[int, Path(title="雏鸟ID", description="雏鸟唯一标识")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)] = None,
    db: Db.Session = Depends(Db.GetDb("GetBirdById")),
):
    """
    # 根据ID获取单只雏鸟详情
    
    ## 路径参数
    - **bird_id**: 雏鸟唯一ID
    
    ## 响应
    - 成功：返回 200 和雏鸟详情
    - 失败：返回 404 雏鸟不存在
    """
    bird = Db.GetBirdById(db, bird_id)
    
    if not bird:
        return JSONResponse(
            status_code=status.HTTP_404_NOT_FOUND,
            content=CommonOut(
                code=status.HTTP_404_NOT_FOUND,
                msg="雏鸟不存在",
                data=None,
            ).model_dump(),
        )
    
    bird_out = Db.BirdOut.from_orm(bird)
    return CommonOut(data=bird_out)


@router.put(
    "/{bird_id}",
    response_model=CommonOut[Db.BirdOut],
    responses={**R404_BIRD_NOT_FOUND, **R403_FORBIDDEN},
    summary="更新雏鸟信息",
)
async def update_bird(
    bird_id: Annotated[int, Path(title="雏鸟ID", description="雏鸟唯一标识")],
    body: Annotated[Db.BirdUpdate, Body()],
    current_user: Annotated[Db.M_Users, Depends(require_root_user)],
    db: Db.Session = Depends(Db.GetDb("UpdateBird")),
):
    """
    # 更新雏鸟信息
    
    仅限 root 用户调用此接口更新雏鸟信息。
    仅更新传入的非 None 字段。
    
    ## 路径参数
    - **bird_id**: 雏鸟唯一ID
    
    ## 请求参数（可选）
    - **name**: 雏鸟名称
    - **species**: 品种
    - **birth_date**: 出生日期（YYYY-MM-DD）
    - **device_id**: 关联设备ID
    - **status**: 状态（available/adopted/grown）
    - **description**: 描述
    - **avatar_url**: 照片URL
    
    ## 响应
    - 成功：返回 200 和更新后的雏鸟信息
    - 失败：返回 404 雏鸟不存在 或 403 权限不足
    """
    try:
        # 检查雏鸟是否存在
        existing_bird = Db.GetBirdById(db, bird_id)
        if not existing_bird:
            return JSONResponse(
                status_code=status.HTTP_404_NOT_FOUND,
                content=CommonOut(
                    code=status.HTTP_404_NOT_FOUND,
                    msg="雏鸟不存在",
                    data=None,
                ).model_dump(),
            )
        
        # 更新雏鸟信息
        updated_bird = Db.UpdateBird(
            db=db,
            bird_id=bird_id,
            name=body.name,
            species=body.species,
            birth_date=body.birth_date,
            device_id=body.device_id,
            status=body.status,
            description=body.description,
            avatar_url=body.avatar_url,
        )
        
        await async_log(
            logger,
            "info",
            f"更新雏鸟成功: ID={bird_id} - 操作者: {current_user.username}"
        )
        
        bird_out = Db.BirdOut.from_orm(updated_bird)
        return CommonOut(data=bird_out)
        
    except ValueError as e:
        await async_log(logger, "warning", f"更新雏鸟失败: 参数错误 - {str(e)}")
        return JSONResponse(
            status_code=status.HTTP_400_BAD_REQUEST,
            content=CommonOut(
                code=status.HTTP_400_BAD_REQUEST,
                msg=f"参数错误: {str(e)}",
                data=None,
            ).model_dump(),
        )
    except Exception as e:
        await async_log(logger, "error", f"更新雏鸟失败: {str(e)}")
        return JSONResponse(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            content=CommonOut(
                code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                msg="服务器内部错误",
                data=None,
            ).model_dump(),
        )


@router.delete(
    "/{bird_id}",
    response_model=CommonOut[None],
    responses={**R404_BIRD_NOT_FOUND, **R403_FORBIDDEN},
    summary="删除雏鸟",
)
async def delete_bird(
    bird_id: Annotated[int, Path(title="雏鸟ID", description="雏鸟唯一标识")],
    current_user: Annotated[Db.M_Users, Depends(require_root_user)],
    db: Db.Session = Depends(Db.GetDb("DeleteBird")),
):
    """
    # 删除雏鸟记录
    
    仅限 root 用户调用此接口删除雏鸟。
    删除操作不可恢复，请谨慎使用。
    
    ## 路径参数
    - **bird_id**: 雏鸟唯一ID
    
    ## 响应
    - 成功：返回 200
    - 失败：返回 404 雏鸟不存在 或 403 权限不足
    """
    try:
        # 检查雏鸟是否存在
        existing_bird = Db.GetBirdById(db, bird_id)
        if not existing_bird:
            return JSONResponse(
                status_code=status.HTTP_404_NOT_FOUND,
                content=CommonOut(
                    code=status.HTTP_404_NOT_FOUND,
                    msg="雏鸟不存在",
                    data=None,
                ).model_dump(),
            )
        
        # 删除雏鸟
        result = Db.DeleteBird(db, bird_id)
        
        if result:
            await async_log(
                logger,
                "info",
                f"删除雏鸟成功: ID={bird_id}, 名称={existing_bird.name} - 操作者: {current_user.username}"
            )
            return CommonOut(data=None)
        else:
            return JSONResponse(
                status_code=status.HTTP_404_NOT_FOUND,
                content=CommonOut(
                    code=status.HTTP_404_NOT_FOUND,
                    msg="雏鸟不存在",
                    data=None,
                ).model_dump(),
            )
            
    except Exception as e:
        await async_log(logger, "error", f"删除雏鸟失败: {str(e)}")
        return JSONResponse(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            content=CommonOut(
                code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                msg="服务器内部错误",
                data=None,
            ).model_dump(),
        )


@router.patch(
    "/{bird_id}/status",
    response_model=CommonOut[Db.BirdOut],
    responses={**R404_BIRD_NOT_FOUND, **R403_FORBIDDEN, **R400_BAD_REQUEST},
    summary="更新雏鸟状态",
)
async def update_bird_status(
    bird_id: Annotated[int, Path(title="雏鸟ID", description="雏鸟唯一标识")],
    status: Annotated[str, Query(description="新状态：available/adopted/grown")],
    current_user: Annotated[Db.M_Users, Depends(require_root_user)],
    db: Db.Session = Depends(Db.GetDb("UpdateBirdStatus")),
):
    """
    # 快速更新雏鸟状态
    
    仅限 root 用户调用此接口快速更新雏鸟状态。
    用于认领、释放等操作。
    
    ## 路径参数
    - **bird_id**: 雏鸟唯一ID
    
    ## 查询参数
    - **status**: 新状态（available/adopted/grown）
    
    ## 响应
    - 成功：返回 200 和更新后的雏鸟信息
    - 失败：返回 404 雏鸟不存在 或 400 状态无效 或 403 权限不足
    """
    # 验证状态值
    valid_statuses = ["available", "adopted", "grown"]
    if status not in valid_statuses:
        return JSONResponse(
            status_code=status.HTTP_400_BAD_REQUEST,
            content=CommonOut(
                code=status.HTTP_400_BAD_REQUEST,
                msg=f"无效的状态值，必须是: {', '.join(valid_statuses)}",
                data=None,
            ).model_dump(),
        )
    
    try:
        updated_bird = Db.UpdateBirdStatus(db, bird_id, status)
        
        if not updated_bird:
            return JSONResponse(
                status_code=status.HTTP_404_NOT_FOUND,
                content=CommonOut(
                    code=status.HTTP_404_NOT_FOUND,
                    msg="雏鸟不存在",
                    data=None,
                ).model_dump(),
            )
        
        await async_log(
            logger,
            "info",
            f"更新雏鸟状态成功: ID={bird_id}, 新状态={status} - 操作者: {current_user.username}"
        )
        
        bird_out = Db.BirdOut.from_orm(updated_bird)
        return CommonOut(data=bird_out)
        
    except Exception as e:
        await async_log(logger, "error", f"更新雏鸟状态失败: {str(e)}")
        return JSONResponse(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            content=CommonOut(
                code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                msg="服务器内部错误",
                data=None,
            ).model_dump(),
        )


@router.post(
    "/{bird_id}/adopt",
    response_model=CommonOut[dict],
    responses={**R404_BIRD_NOT_FOUND, **R400_BAD_REQUEST},
    summary="认领雏鸟",
)
async def adopt_bird(
    bird_id: Annotated[int, Path(title="雏鸟ID", description="雏鸟唯一标识")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("AdoptBird")),
):
    """
    # 认领雏鸟
    
    用户认领雏鸟，每个用户只能认领一只雏鸟。
    认领成功后，雏鸟状态会更新为已认领，用户的 extra 字段会添加已认领雏鸟的信息。
    
    ## 路径参数
    - **bird_id**: 雏鸟唯一ID
    
    ## 响应
    - 成功：返回 200 和已认领雏鸟信息
    - 失败：返回 404 雏鸟不存在 或 400 无法认领（如已认领过或雏鸟不可认领）
    """
    try:
        # 调用认领雏鸟函数
        result = Db.AdoptBird(db, current_user.id, bird_id)
        
        if not result["success"]:
            return JSONResponse(
                status_code=status.HTTP_400_BAD_REQUEST,
                content=CommonOut(
                    code=status.HTTP_400_BAD_REQUEST,
                    msg=result["message"],
                    data=None,
                ).model_dump(),
            )
        
        await async_log(
            logger,
            "info",
            f"用户 {current_user.username} 成功认领雏鸟 ID={bird_id}"
        )
        
        return CommonOut(
            data={
                "adopted_bird": result["adopted_bird"],
                "message": result["message"]
            }
        )
        
    except Exception as e:
        await async_log(logger, "error", f"认领雏鸟失败: {str(e)}")
        return JSONResponse(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            content=CommonOut(
                code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                msg="服务器内部错误",
                data=None,
            ).model_dump(),
        )


@router.delete(
    "/adopted/me",
    response_model=CommonOut[dict],
    responses={**R400_BAD_REQUEST},
    summary="释放已认领的雏鸟",
)
async def release_bird(
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("ReleaseAdoptedBird")),
):
    """
    # 释放已认领的雏鸟
    
    用户释放已认领的雏鸟，释放后雏鸟状态会更新为可认领，用户的 extra 字段会移除已认领雏鸟的信息。
    
    ## 响应
    - 成功：返回 200 和已释放雏鸟信息
    - 失败：返回 400 无法释放（如没有已认领的雏鸟）
    """
    try:
        # 调用释放雏鸟函数
        result = Db.ReleaseAdoptedBird(db, current_user.id)
        
        if not result["success"]:
            return JSONResponse(
                status_code=status.HTTP_400_BAD_REQUEST,
                content=CommonOut(
                    code=status.HTTP_400_BAD_REQUEST,
                    msg=result["message"],
                    data=None,
                ).model_dump(),
            )
        
        await async_log(
            logger,
            "info",
            f"用户 {current_user.username} 成功释放雏鸟 ID={result['released_bird'].get('bird_id')}"
        )
        
        return CommonOut(
            data={
                "released_bird": result["released_bird"],
                "message": result["message"]
            }
        )
        
    except Exception as e:
        await async_log(logger, "error", f"释放雏鸟失败: {str(e)}")
        return JSONResponse(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            content=CommonOut(
                code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                msg="服务器内部错误",
                data=None,
            ).model_dump(),
        )


@router.get(
    "/adopted/me",
    response_model=CommonOut[dict],
    summary="获取当前用户已认领的雏鸟",
)
async def get_my_adopted_bird(
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("GetUserAdoptedBird")),
):
    """
    # 获取当前用户已认领的雏鸟信息
    
    返回当前登录用户已认领的雏鸟详细信息。
    
    ## 响应
    - 成功：返回 200 和已认领雏鸟信息（如果有）
    - 无已认领雏鸟：返回 200 和空对象
    """
    try:
        # 获取用户已认领的雏鸟信息
        adopted_bird = Db.GetUserAdoptedBird(db, current_user.id)
        
        return CommonOut(
            data={
                "adopted_bird": adopted_bird
            }
        )
        
    except Exception as e:
        await async_log(logger, "error", f"获取已认领雏鸟信息失败: {str(e)}")
        return JSONResponse(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            content=CommonOut(
                code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                msg="服务器内部错误",
                data=None,
            ).model_dump(),
        )
