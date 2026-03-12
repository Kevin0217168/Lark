from fastapi import FastAPI
import uvicorn

from stream import viewer, stream, device

app = FastAPI()

app.include_router(device.router)
app.include_router(viewer.router)
app.include_router(stream.router)

if __name__ == "__main__":
    uvicorn.run(app="main:app", host="0.0.0.0", port=8080, reload=True)

