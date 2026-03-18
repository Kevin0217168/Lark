from fastapi import FastAPI
import uvicorn

from stream import Stream
from userapi import User, Login
from deviceapi import Device

import os

isDeploy = os.environ.get("FASTAPI_DEPLOY", None)
prepath = "" if isDeploy is None else "/"
prefix = "/api" if isDeploy is None else ""

app = FastAPI(
    title="云雀 Lark",
    summary="物联网系统后端API开放接口文档",
    version="0.3.2",
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

# app.include_router(Viewer.router)
app.include_router(Stream.router)
app.include_router(User.router)
app.include_router(Login.router)
app.include_router(Login.refresh_router)
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
    return get_redoc_html(
        title=app.title + " - ReDoc",
        openapi_url="openapi.json",
        redoc_js_url=f"{prepath}api/static/redoc.standalone.js",
    )


@app.get("/")
async def hello(name: str):
    return f"Hello {name}!"


if __name__ == "__main__":
    uvicorn.run(app="main:app", host="localhost", port=8080, reload=True)
