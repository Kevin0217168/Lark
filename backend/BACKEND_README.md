# 后端接口文档
## 主要技术栈
FastAPI + SQLite
大部分接口遵循 RESTful 风格开发
## 项目文件树结构
```
.
├── BACKEND_README.md                     后端文档(本文件)
├── generate_uuid.py                      (生成唯一id的临时文件)
├── main.py                               FastAPI入口文件
└── stream                                直播组件
    ├── device.py                         设备路由
    ├── stream.py                         直播路由(ws)
    └── viewer.py                         观看者路由


```

## 接口文档
### 路由 /device
#### GET / 获取所有设备
#### GET /{id} 获取对应id设备
#### PUT /{id} 修改对应id设备,全部更新
#### POST / 以服务端产生的id号创建设备
#### POST /{id} 修改对应id设备,部分更新
#### DELETE /{id} 删除对应id设备

### 路由 /viewer
#### GET / 获取所有观看者
#### GET /{id} 获取对应id观看者
#### PUT /{id} 修改对应id观看者,全部更新
#### POST / 以服务端产生的id号创建观看者
#### POST /{id} 修改对应id观看者,部分更新
#### DELETE /{id} 删除对应id观看者