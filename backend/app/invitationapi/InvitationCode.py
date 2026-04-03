from fastapi import APIRouter, Depends, HTTPException, Body, status, Request
from fastapi.responses import JSONResponse
from typing import Annotated, Optional, Dict
from pydantic import BaseModel, Field
from schema import CommonOut
import Db
import Security
from Logset import async_log, logger
from datetime import datetime, timedelta, timezone
import secrets
import string

router = APIRouter(prefix="/invitation-codes", tags=["Invitation Codes"])

# 请求频率限制（内存-based，生产环境建议使用Redis）
rate_limit_store: Dict[str, Dict[str, any]] = {}
RATE_LIMIT_PER_MINUTE = 5  # 每分钟最多5次请求

# 获取本地时间（UTC+8）
def get_local_time():
    return datetime.now(timezone(timedelta(hours=8)))


def generate_invitation_code(length: int = 10) -> str:
    """
    生成随机邀请码
    
    参数:
        length: 邀请码长度
    
    返回:
        str: 邀请码
    """
    alphabet = string.ascii_letters + string.digits
    return ''.join(secrets.choice(alphabet) for _ in range(length))


def check_rate_limit(ip: str) -> bool:
    """
    检查请求频率限制
    
    参数:
        ip: 客户端IP地址
    
    返回:
        bool: True表示允许请求，False表示超过限制
    """
    current_time = datetime.now()
    
    if ip not in rate_limit_store:
        rate_limit_store[ip] = {
            "count": 1,
            "last_reset": current_time
        }
        return True
    
    entry = rate_limit_store[ip]
    time_since_reset = (current_time - entry["last_reset"]).total_seconds()
    
    if time_since_reset > 60:  # 1分钟重置
        entry["count"] = 1
        entry["last_reset"] = current_time
        return True
    
    if entry["count"] >= RATE_LIMIT_PER_MINUTE:
        return False
    
    entry["count"] += 1
    return True


class InvitationCodeCreate(BaseModel):
    """邀请码创建请求模型"""
    expiresIn: Optional[int] = Field(
        default=24, 
        ge=1, 
        le=48, 
        description="有效期（小时），1-48小时"
    )
    maxUses: Optional[int] = Field(
        default=3, 
        ge=1, 
        le=5, 
        description="最大使用次数，1-5次"
    )


class InvitationCodeResponse(BaseModel):
    """邀请码创建响应模型"""
    code: str
    expiresAt: datetime
    maxUses: int
    createdAt: datetime
    createdByUserId: int


@router.post(
    "",
    response_model=CommonOut[InvitationCodeResponse],
    status_code=status.HTTP_201_CREATED,
    responses={
        401: {"description": "未授权"},
        403: {"description": "权限不足"},
        400: {"description": "参数错误"},
        429: {"description": "请求频率过高"},
    }
)
async def create_invitation_code(
    request: Request,
    body: Annotated[InvitationCodeCreate, Body()],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("CreateInvitationCode")),
):
    """
    # 创建邀请码
    
    仅允许具有root角色的用户创建邀请码
    
    ## 请求参数
    - `expiresIn`: 有效期（小时），1-48小时，默认24小时
    - `maxUses`: 最大使用次数，1-5次，默认3次
    
    ## 响应
    - `code`: 生成的邀请码
    - `expiresAt`: 过期时间
    - `maxUses`: 最大使用次数
    - `createdAt`: 创建时间
    - `createdByUserId`: 创建者用户ID
    """
    # 检查请求频率限制
    client_ip = request.client.host
    if not check_rate_limit(client_ip):
        await async_log(
            logger,
            "warning",
            f"请求频率过高: IP={client_ip}"
        )
        raise HTTPException(
            status_code=status.HTTP_429_TOO_MANY_REQUESTS,
            detail="请求频率过高，请1分钟后再试"
        )
    
    # 检查用户角色
    if current_user.role != "root":
        await async_log(
            logger,
            "warning",
            f"权限不足: 用户 {current_user.username} (ID: {current_user.id}, 角色: {current_user.role}) 尝试创建邀请码"
        )
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="权限不足，仅管理员(root)可创建邀请码"
        )
    
    try:
        # 生成邀请码
        invitation_code = generate_invitation_code()
        
        # 计算过期时间
        expire_at = get_local_time() + timedelta(hours=body.expiresIn)
        
        # 创建邀请码
        new_code = Db.CreateInvitationCode(
            db=db,
            user_id=current_user.id,
            code=invitation_code,
            expire_at=expire_at,
            remaining_uses=body.maxUses
        )
        
        # 记录操作日志
        await async_log(
            logger,
            "info",
            f"邀请码创建成功: 代码={invitation_code}, 创建者={current_user.username}, 创建者ID={current_user.id}, 有效期={body.expiresIn}小时, 最大使用次数={body.maxUses}"
        )
        
        # 构建响应
        response_data = InvitationCodeResponse(
            code=new_code.code,
            expiresAt=new_code.expire_at,
            maxUses=body.maxUses,
            createdAt=new_code.created_at,
            createdByUserId=current_user.id
        )
        
        return CommonOut(
            code=status.HTTP_201_CREATED,
            msg="邀请码创建成功",
            data=response_data
        )
        
    except Exception as e:
        await async_log(
            logger,
            "error",
            f"创建邀请码失败: {str(e)}"
        )
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="服务器内部错误"
        )