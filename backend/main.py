from fastapi import FastAPI
import uvicorn

from stream import Device, Stream, Viewer
from userapi import User, Login

app = FastAPI()

app.include_router(Device.router, prefix="/api")
app.include_router(Viewer.router, prefix="/api")
app.include_router(Stream.router, prefix="/api")
app.include_router(User.router, prefix="/api")

@app.get("/")
async def hello(name: str):
    return f"Hello {name}!"

if __name__ == "__main__":
    uvicorn.run(app="main:app", host="localhost", port=8080, reload=True)

