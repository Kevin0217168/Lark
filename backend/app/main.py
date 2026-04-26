import uvicorn
from fastapi import FastAPI, Request, HTTPException, Response
from fastapi.responses import JSONResponse
from datetime import datetime
import http
import time
import traceback
import json
import asyncio
from stream import Stream
from userapi import User, Login
from userapi import Bird as BirdApi
from deviceapi import Device
from sensorapi import SensorData
from sensorapi import SensorUpload
from invitationapi import router as invitation_router
from firmwareapi import Firmware
from logapi import DeviceLog

import os
from Logset import async_log, logger, request_logger, log_executor
from contextlib import asynccontextmanager
import datacontrol.DeviceLogDb as DeviceLogDb
import datacontrol.SensorUploadDb as SensorUploadDb
import datacontrol.SensorDb as SensorDb


_cleanup_task = None


async def cleanup_old_logs_job():
    while True:
        try:
            await asyncio.sleep(86400)  # 每24小时执行一次
            try:
                from Db import OpenDb
                with OpenDb("cleanup_old_data") as db:
                    log_count = DeviceLogDb.CleanupOldDeviceLogs(db, days=7)
                    upload_count = SensorUploadDb.CleanupOldSensorUploads(db, days=7)
                    sensor_count = SensorDb.CleanupOldSensorData(db, days=7)
                    await async_log(logger, "info", f"定期清理过期数据完成: 设备日志 {log_count} 条, 传感器上传 {upload_count} 条, 传感器数据 {sensor_count} 条")
            except Exception as e:
                await async_log(logger, "error", f"定期清理过期数据数据库操作出错: {str(e)}")
        except Exception as e:
            await async_log(logger, "error", f"定期清理过期数据任务出错: {str(e)}")
import asyncio


@asynccontextmanager
async def lifespan(app: FastAPI):
    global _cleanup_task
    print("应用启动，资源初始化...")
    
    _cleanup_task = asyncio.create_task(cleanup_old_logs_job())
    
    try:
        yield
    finally:
        if _cleanup_task is not None:
            _cleanup_task.cancel()
            try:
                await _cleanup_task
            except asyncio.CancelledError:
                pass
        log_executor.shutdown(wait=True)
        print("应用关闭，资源已清理...")


isDeploy = os.environ.get("FASTAPI_DEPLOY", None)
prepath = "" if isDeploy is None else "/"
prefix = "/api" if isDeploy is None else ""

app = FastAPI(
    lifespan=lifespan,
    title="云雀 Lark",
    summary="物联网系统后端API开放接口文档",
    version="0.7.1",
    root_path="/api",
    docs_url=None,
    redoc_url=None,
    contact={
        "name": "Lark--云雀 物联网系统前端",
        "url": "https://lark.mintlab.top/",
    },
    openapi_external_docs={
        "description": "云雀 Lark Github项目",
        "url": "https://github.com/Kevin0217168/Lark",
    },
)


app.include_router(Stream.router)
app.include_router(User.router)
app.include_router(Login.router)
app.include_router(BirdApi.router)
app.include_router(Device.router)
app.include_router(SensorData.router)
app.include_router(SensorUpload.router)
app.include_router(invitation_router)
app.include_router(Firmware.router)
app.include_router(DeviceLog.router)

# 挂载静态文件
from fastapi.staticfiles import StaticFiles

app.mount(path=f"{prefix}/static", app=StaticFiles(directory="static"), name="static")

from fastapi.openapi.docs import (
    get_redoc_html,
    get_swagger_ui_html,
    get_swagger_ui_oauth2_redirect_html,
)


@app.get("/docs", include_in_schema=False)
async def custom_swagger_ui_html():
    await async_log(logger, "info", "docs")
    return get_swagger_ui_html(
        title=app.title + " - Swagger UI",
        oauth2_redirect_url=app.swagger_ui_oauth2_redirect_url,
        openapi_url="openapi.json",
        swagger_js_url=f"{prepath}api/static/swagger-ui-bundle.js",
        swagger_css_url=f"{prepath}api/static/swagger-ui.css",
    )


@app.get(app.swagger_ui_oauth2_redirect_url, include_in_schema=False)
async def swagger_ui_redirect():
    return get_swagger_ui_oauth2_redirect_html()


@app.get("/redoc", include_in_schema=False)
async def redoc_html():
    await async_log(logger, "info", "redoc")
    return get_redoc_html(
        title=app.title + " - ReDoc",
        openapi_url="openapi.json",
        redoc_js_url=f"{prepath}api/static/redoc.standalone.js",
    )


@app.get("/")
async def hello(name: str):
    await async_log(logger, "info", "根目录")
    return f"Hello {name}!"


# 全局异常处理
@app.exception_handler(Exception)
async def global_exception_handler(request: Request, exc: Exception):
    # 记录异常详细信息
    error_id = f"{datetime.now().strftime('%Y%m%d%H%M%S')}_{id(request)}"

    await async_log(
        logger,
        "error",
        f"未处理的异常 [ID: {error_id}]: {str(exc)}\n"
        f"请求路径: {request.url.path}\n"
        f"请求方法: {request.method}\n"
        f"客户端IP: {request.client.host}\n"
        f"堆栈跟踪:\n{traceback.format_exc()}",
    )

    # 返回友好的错误响应
    return JSONResponse(
        status_code=500, content={"message": "内部服务器错误", "error_id": error_id}
    )

class RequestLimiter:
    def __init__(self, max_concurrent: int):
        self._max = max_concurrent
        self._current = 0
        self._lock = asyncio.Lock()

    async def acquire(self) -> bool:
        async with self._lock:
            if self._current >= self._max:
                return False
            self._current += 1
            return True

    async def release(self) -> None:
        async with self._lock:
            if self._current > 0:
                self._current -= 1

request_limiter = RequestLimiter(5)

# 请求日志中间件
@app.middleware("http")
async def log_requests(request: Request, call_next):
    if not await request_limiter.acquire():
        return JSONResponse({"detail": "server busy"}, status_code=503)

    try:
        # 记录请求开始时间
        start_time = time.time()

        # 获取请求信息
        request_info = {
            "method": request.method,
            "url": str(request.url),
            "path": request.url.path,
            "query_params": dict(request.query_params),
            "client": {"host": request.client.host, "port": request.client.port},
            "headers": dict(request.headers),
        }

        # 记录请求体（对于非表单数据）
        try:
            if request.method in ["POST", "PUT", "PATCH"]:
                body = await request.body()
                if body:
                    try:
                        request_info["body"] = json.loads(body)
                    except json.JSONDecodeError:
                        request_info["body"] = body.decode("utf-8")
        except Exception:
            request_info["body"] = "无法解析请求体"

        # 处理请求
        response = await call_next(request)

        # 计算处理时间
        process_time = (time.time() - start_time) * 1000

        # 安全获取原因短语
        status_code = response.status_code
        try:
            reason = http.HTTPStatus(status_code).phrase
        except ValueError:
            reason = "Unknown"  # 处理非标准状态码

        # 记录响应信息
        await async_log(
            logger,
            "info",
            f"{request.method} {request.url.path} - "
            f"{response.status_code} {reason}",
        )

        await async_log(
            request_logger,
            "info",
            f"{request.method} {request.url.path} - "
            f"{response.status_code} {reason} - "
            f"处理时间: {process_time:.2f}ms\n"
            f"请求详情: {json.dumps(request_info, ensure_ascii=False, default=str)}\n",
        )

        return response
    finally:
        await request_limiter.release()


if __name__ == "__main__":
    if not isDeploy:
        uvicorn.run(app="main:app", host="0.0.0.0", port=8080, reload=True)
