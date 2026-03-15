from fastapi import FastAPI
import uvicorn

from stream import Device, Stream, Viewer

app = FastAPI()

app.include_router(Device.router)
app.include_router(Viewer.router)
app.include_router(Stream.router)

@app.get("/")
async def hello(name: str):
    return f"Hello {name}!"

if __name__ == "__main__":
    uvicorn.run(app="main:app", host="0.0.0.0", port=8080, reload=True)

