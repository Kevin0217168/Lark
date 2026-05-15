from fastapi import APIRouter, Depends, status
from typing import Annotated, List, Optional
from fastapi.responses import JSONResponse
from fastapi import Path, Query

from schema import CommonOut
import Db
import Security
from Logset import async_log, logger
from datacontrol.FeedingDb import (
    CreateFeedingRecord,
    GetTodayRecordCount,
    GetFeedingHistory,
    FeedingRecordOut,
    FeedingStatusOut,
)

from datacontrol.DeviceDb import GetDevicesByAreaNumber
from deviceapi.Device import CMD_FEED_START, CMD_FEED_STOP, esp32IdDict, _is_device_ws_alive

router = APIRouter(prefix="/feeding", tags=["Feeding"])

DAILY_FEEDING_LIMIT = 1

def _check_feeding_permission(db, current_user, bird_id: int) -> Optional[JSONResponse]:
    if current_user.role == "root":
        return None
    adopted = Db.GetUserAdoptedBird(db, current_user.id)
    if not adopted or adopted.get("bird_id") != bird_id:
        return JSONResponse(
            status_code=403,
            content=CommonOut(code=403, msg="您未认领该雏鸟，无法执行此操作", data=None).model_dump(),
        )
    return None

def _find_c3_device_id(db, bird_id: int, device_id: Optional[int]) -> Optional[int]:
    if device_id is not None:
        return device_id
    from datacontrol.BirdDb import GetBirdById
    bird = GetBirdById(db, bird_id)
    if not bird or not bird.area:
        return None
    devices = GetDevicesByAreaNumber(db, bird.area, bird.number)
    c3 = next((d for d in devices if d.device_type == "ESP32-C3"), None)
    return c3.id if c3 else None


async def _send_action_command(device_id: int, command: dict, action_name: str, bird_id: int):
    device = esp32IdDict.get(device_id)
    if device is None or not _is_device_ws_alive(device):
        return JSONResponse(
            status_code=503,
            content=CommonOut(code=503, msg="设备不在线，无法执行操作", data=None).model_dump(),
        )

    try:
        await device.websocket.send_json(command)
    except Exception as e:
        await async_log(logger, "error", f"发送{action_name}指令到设备{device_id}失败: {e}")
        return JSONResponse(
            status_code=502,
            content=CommonOut(code=502, msg=f"指令发送失败: {e}", data=None).model_dump(),
        )

    return None


@router.post(
    "/trigger",
    response_model=CommonOut[dict],
    summary="触发喂食操作",
)
async def trigger_feeding(
    bird_id: Annotated[int, Query(description="雏鸟ID")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    device_id: Annotated[Optional[int], Query(description="C3设备ID（可选，不传则自动查找鸟笼中的C3设备）")] = None,
    db: Db.Session = Depends(Db.GetDb("TriggerFeeding")),
):
    bird = Db.GetBirdById(db, bird_id)
    if not bird:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="雏鸟不存在", data=None).model_dump(),
        )

    perm_err = _check_feeding_permission(db, current_user, bird_id)
    if perm_err:
        return perm_err

    c3_dev_id = _find_c3_device_id(db, bird_id, device_id)
    if not c3_dev_id:
        return JSONResponse(
            status_code=400,
            content=CommonOut(code=400, msg="未找到鸟笼中的C3设备，无法执行喂食", data=None).model_dump(),
        )

    today_count = GetTodayRecordCount(db, bird_id, "feeding")
    if today_count >= DAILY_FEEDING_LIMIT:
        return JSONResponse(
            status_code=400,
            content=CommonOut(code=400, msg=f"今日喂食次数已达上限（{DAILY_FEEDING_LIMIT}次/天）", data={
                "today_count": today_count,
                "daily_limit": DAILY_FEEDING_LIMIT,
            }).model_dump(),
        )

    err = await _send_action_command(c3_dev_id, CMD_FEED_START, "喂食", bird_id)
    if err:
        return err

    record = CreateFeedingRecord(db, bird_id, current_user.id, "feeding")

    await async_log(logger, "info", f"用户 {current_user.username} 触发雏鸟 {bird_id} 喂食操作，设备 {c3_dev_id}")

    return CommonOut(
        msg="喂食已开始，2 分钟后自动停止.",
        data={
            "key": "feed",
            "values": "started",
            "record": FeedingRecordOut.from_orm(record).model_dump(),
            "remaining": DAILY_FEEDING_LIMIT - (today_count + 1),
        }
    )


@router.get(
    "/status",
    response_model=CommonOut[dict],
    summary="查询喂食状态",
)
async def get_feeding_status(
    bird_id: Annotated[int, Query(description="雏鸟ID")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    db: Db.Session = Depends(Db.GetDb("GetFeedingStatus")),
):
    bird = Db.GetBirdById(db, bird_id)
    if not bird:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="雏鸟不存在", data=None).model_dump(),
        )

    perm_err = _check_feeding_permission(db, current_user, bird_id)
    if perm_err:
        return perm_err

    feeding_count = GetTodayRecordCount(db, bird_id, "feeding")
    feeding_records = GetFeedingHistory(db, bird_id, limit=1)

    feeding_status = FeedingStatusOut(
        action_type="feeding",
        today_count=feeding_count,
        daily_limit=DAILY_FEEDING_LIMIT,
        remaining=DAILY_FEEDING_LIMIT - feeding_count,
        completed=feeding_count >= DAILY_FEEDING_LIMIT,
        last_action_time=feeding_records[0].timestamp.isoformat() if feeding_records else None,
    )

    return CommonOut(data={
        "feeding": feeding_status.model_dump(),
        "bird_id": bird_id,
    })


@router.post(
    "/stop",
    response_model=CommonOut[dict],
    summary="停止喂食操作",
)
async def stop_feeding(
    bird_id: Annotated[int, Query(description="雏鸟ID")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)],
    device_id: Annotated[Optional[int], Query(description="C3设备ID（可选，不传则自动查找鸟笼中的C3设备）")] = None,
    db: Db.Session = Depends(Db.GetDb("StopFeeding")),
):
    bird = Db.GetBirdById(db, bird_id)
    if not bird:
        return JSONResponse(
            status_code=404,
            content=CommonOut(code=404, msg="雏鸟不存在", data=None).model_dump(),
        )

    perm_err = _check_feeding_permission(db, current_user, bird_id)
    if perm_err:
        return perm_err

    c3_dev_id = _find_c3_device_id(db, bird_id, device_id)
    if not c3_dev_id:
        return JSONResponse(
            status_code=400,
            content=CommonOut(code=400, msg="未找到鸟笼中的C3设备，无法执行停止喂食", data=None).model_dump(),
        )

    err = await _send_action_command(c3_dev_id, CMD_FEED_STOP, "停止喂食", bird_id)
    if err:
        return err

    record = CreateFeedingRecord(db, bird_id, current_user.id, "stop_feeding")

    await async_log(logger, "info", f"用户 {current_user.username} 停止雏鸟 {bird_id} 喂食操作，设备 {c3_dev_id}")

    return CommonOut(
        msg="喂食已停止.",
        data={
            "key": "feed",
            "values": "stopped",
            "record": FeedingRecordOut.from_orm(record).model_dump(),
        }
    )


@router.get(
    "/history",
    response_model=CommonOut[List[FeedingRecordOut]],
    summary="查询喂食历史记录",
)
async def get_feeding_history(
    bird_id: Annotated[int, Query(description="雏鸟ID")],
    current_user: Annotated[Db.M_Users, Depends(Security.GetCurrentUser)] = None,
    limit: int = Query(default=50, ge=1, le=200, description="返回记录数"),
    db: Db.Session = Depends(Db.GetDb("GetFeedingHistory")),
):
    if current_user:
        perm_err = _check_feeding_permission(db, current_user, bird_id)
        if perm_err:
            return perm_err
    records = GetFeedingHistory(db, bird_id, limit=limit)
    return CommonOut(data=[FeedingRecordOut.from_orm(r) for r in records])
