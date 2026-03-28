from fastapi import APIRouter, Request, Depends, Query, Path, Header
from fastapi.responses import JSONResponse, PlainTextResponse
from typing import Annotated, List, Optional
from datetime import datetime
from Logset import logger

import Db
import Security
from schema import CommonOut, R404_DEVICE_NOT_FOUND_BY_SECRET, R403_FORBIDDEN

router = APIRouter(prefix="/logs", tags=["DevLogs"])

# 日志等级可选值
_LEVEL_ENUM = ["ERROR", "WARN", "INFO", "DEBUG", "VERBOSE"]


@router.post(
    "",
    response_model=CommonOut[int],
    responses=R404_DEVICE_NOT_FOUND_BY_SECRET,
    summary="设备上报日志（ESP32 调用）",
)
async def upload_device_log(
    request: Request,
    authorization: Annotated[str, Header(description="设备密钥 (secret)")],
    db: Db.Session = Depends(Db.GetDb("upload_device_log")),
):
    """
    # 设备上报日志

    - 认证方式: Authorization header 携带设备 secret
    - Body: 纯文本日志内容 (text/plain)，ESP-IDF 格式
    - 自动解析每行日志的等级(E/W/I/D/V)、标签、tick，逐条存入数据库
    - 返回写入的日志条数
    """
    # 1. 通过 secret 查找设备
    devices = Db.GetDevices(db, secret=authorization)
    if not devices:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="Device not found by secret", data=None).model_dump(),
        )
    device = devices[0]

    # 2. 读取纯文本 body
    body = await request.body()
    content = body.decode("utf-8", errors="replace")

    if not content.strip():
        return CommonOut(msg="Empty log, skipped", data=0)

    # 3. 解析并逐条存入数据库
    count = Db.AddDeviceLogsBatch(
        db,
        device_id=device.id,
        raw_text=content,
    )

    return CommonOut(msg=f"{count} log(s) uploaded", data=count)


@router.get(
    "/{device_id}",
    response_model=CommonOut[List[Db.DeviceLogItem]],
    summary="查询设备日志",
)
async def get_device_logs(
    device_id: Annotated[int, Path(title="设备ID")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    start_time: Annotated[Optional[datetime], Query()] = None,
    end_time: Annotated[Optional[datetime], Query()] = None,
    level: Annotated[Optional[str], Query(description="日志等级过滤: ERROR/WARN/INFO/DEBUG/VERBOSE")] = None,
    tag: Annotated[Optional[str], Query(description="ESP 组件标签过滤")] = None,
    skip: Annotated[int, Query(ge=0)] = 0,
    limit: Annotated[int, Query(ge=1, le=500)] = 50,
    db: Db.Session = Depends(Db.GetDb("get_device_logs")),
):
    """
    # 查询设备日志（需要用户登录）

    - 支持时间范围、日志等级、组件标签过滤和分页
    - 返回按时间倒序排列的日志列表（空列表不返回 404）
    """
    logs = Db.GetDeviceLogs(
        db,
        device_id=device_id,
        start_time=start_time,
        end_time=end_time,
        level=level,
        tag=tag,
        skip=skip,
        limit=limit,
    )
    items = [Db.DeviceLogItem.model_validate(log) for log in logs]
    return CommonOut(data=items)


@router.get(
    "/{device_id}/text",
    response_class=PlainTextResponse,
    summary="查询设备日志（纯文本）",
)
async def get_device_logs_text(
    device_id: Annotated[int, Path(title="设备ID")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    start_time: Annotated[Optional[datetime], Query()] = None,
    end_time: Annotated[Optional[datetime], Query()] = None,
    level: Annotated[Optional[str], Query(description="日志等级过滤: ERROR/WARN/INFO/DEBUG/VERBOSE")] = None,
    tag: Annotated[Optional[str], Query(description="ESP 组件标签过滤")] = None,
    skip: Annotated[int, Query(ge=0)] = 0,
    limit: Annotated[int, Query(ge=1, le=500)] = 50,
    db: Db.Session = Depends(Db.GetDb("get_device_logs_text")),
):
    """
    # 查询设备日志（纯文本格式，方便终端查看）

    还原 ESP-IDF 日志格式输出，每条之间用分隔线分隔
    """
    logs = Db.GetDeviceLogs(
        db,
        device_id=device_id,
        start_time=start_time,
        end_time=end_time,
        level=level,
        tag=tag,
        skip=skip,
        limit=limit,
    )
    if not logs:
        return PlainTextResponse("No logs found", status_code=404)

    # 倒序查询的结果反转为正序输出
    logs.reverse()

    # 等级首字母反查
    _level_char = {"ERROR": "E", "WARN": "W", "INFO": "I", "DEBUG": "D", "VERBOSE": "V"}

    text_parts = []
    for log in logs:
        char = _level_char.get(log.level, "I")
        header = f"──── {log.timestamp.strftime('%Y-%m-%d %H:%M:%S')} [{log.level}] ────"
        body = f"{char} ({log.tick}) {log.tag}: {log.content}"
        text_parts.append(f"{header}\n{body}")
    return PlainTextResponse("\n".join(text_parts))


@router.delete(
    "/{device_id}",
    response_model=CommonOut[int],
    responses=R403_FORBIDDEN,
    summary="删除设备日志（仅 root）",
)
async def delete_device_logs(
    device_id: Annotated[int, Path(title="设备ID")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    start_time: Annotated[Optional[datetime], Query()] = None,
    end_time: Annotated[Optional[datetime], Query()] = None,
    level: Annotated[Optional[str], Query(description="日志等级过滤: ERROR/WARN/INFO/DEBUG/VERBOSE")] = None,
    db: Db.Session = Depends(Db.GetDb("delete_device_logs")),
):
    """
    # 删除设备日志（仅 root 可操作）

    - 支持按时间范围和日志等级过滤删除
    - 不提供过滤条件则删除该设备所有日志
    - 返回删除的记录数
    """
    if current_user.role != "root":
        return JSONResponse(
            status_code=403,
            content=CommonOut(code=403, msg="Permission denied.", data=None).model_dump(),
        )

    deleted_count = Db.DeleteDeviceLogs(
        db,
        device_id=device_id,
        start_time=start_time,
        end_time=end_time,
        level=level,
    )
    if deleted_count == 0:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="No logs found", data=None).model_dump(),
        )
    return CommonOut(data=deleted_count)
