import uvicorn
from fastapi import FastAPI, Request, HTTPException, Response
from fastapi.responses import JSONResponse
from datetime import datetime
import http
import time
import traceback
import json
from stream import Stream
from userapi import User, Login
from deviceapi import Device

import os
from Logset import async_log, logger, request_logger, log_executor
from contextlib import asynccontextmanager


@asynccontextmanager
async def lifespan(app: FastAPI):
    # 启动逻辑（原 on_event("startup") 可以写在这里）
    print("应用启动，资源初始化...")
    yield
    # 关闭逻辑（原 on_event("shutdown") 写在这里）
    log_executor.shutdown(wait=True)
    print("应用关闭，资源已清理...")


isDeploy = os.environ.get("FASTAPI_DEPLOY", None)
prepath = "" if isDeploy is None else "/"
prefix = "/api" if isDeploy is None else ""

app = FastAPI(
    lifespan=lifespan,
    title="云雀 Lark",
    summary="物联网系统后端API开放接口文档",
    version="0.4.1",
    root_path="/api",
    docs_url=None,
    redoc_url=None,
    contact={
        "name": "Mint",
        "url": "https://www.mintlab.top/",
    },
    openapi_external_docs={
        "description": "云雀 Lark Github项目",
        "url": "https://github.com/Kevin0217168/Lark",
    },
)


app.include_router(Stream.router)
app.include_router(User.router)
app.include_router(Login.router)
app.include_router(Device.router)

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


# 请求日志中间件
@app.middleware("http")
async def log_requests(request: Request, call_next):
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


if __name__ == "__main__":
    if not isDeploy:
        uvicorn.run(app="main:app", host="0.0.0.0", port=8080, reload=True)
