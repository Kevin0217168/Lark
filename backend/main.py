from fastapi import FastAPI
import uvicorn

from stream import Device, Stream, Viewer
from userapi import User, Login

app = FastAPI(root_path="/api",
              docs_url=None, 
              redoc_url=None)

app.include_router(Device.router)
app.include_router(Viewer.router)
app.include_router(Stream.router)
app.include_router(User.router)
app.include_router(Login.router)
app.include_router(Login.refresh_router)

# 挂载静态文件
from fastapi.staticfiles import StaticFiles
app.mount("/static", StaticFiles(directory="static"), name="static")

from fastapi.openapi.docs import (
    get_redoc_html,
    get_swagger_ui_html,
    get_swagger_ui_oauth2_redirect_html,
)

@app.get("/docs", include_in_schema=False)
async def custom_swagger_ui_html():
    return get_swagger_ui_html(
        openapi_url=app.openapi_url,
        title=app.title + " - Swagger UI",
        oauth2_redirect_url=app.swagger_ui_oauth2_redirect_url,
        swagger_js_url="/api/static/swagger-ui-bundle.js",
        swagger_css_url="/api/static/swagger-ui.css",
    )


@app.get(app.swagger_ui_oauth2_redirect_url, include_in_schema=False)
async def swagger_ui_redirect():
    return get_swagger_ui_oauth2_redirect_html()


@app.get("/redoc", include_in_schema=False)
async def redoc_html():
    return get_redoc_html(
        openapi_url=app.openapi_url,
        title=app.title + " - ReDoc",
        redoc_js_url="/api/static/redoc.standalone.js",
    )

@app.get("/")
async def hello(name: str):
    return f"Hello {name}!"

if __name__ == "__main__":
    uvicorn.run(app="main:app", host="localhost", port=8080, reload=True)

