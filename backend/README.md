# Lark 后端开发文档

## 项目简介

Lark 后端是智能鸟厂监控系统的服务端，基于 FastAPI 框架构建，提供设备管理、实时视频流、传感器数据处理、雏鸟管理、喂食控制等核心功能。采用 SQLite 数据库存储，支持 JWT 认证和 WebSocket 实时通信。


## 参考文档
- [Stream 模块文档](app/stream/README.md)
- [Firmware 固件管理模块文档](app/firmwareapi/README.md)
- [异步获取设备版本号 — 实现解析](app/deviceapi/README.md)

## 学习笔记
- [基于 asyncio.Future 实现 WebSocket 请求-响应模式：异步获取 IoT 设备版本号](app/docs/async-device-version.md)
- [SQLite 性能优化实战：从 70ms 到 1ms 的日志写入之旅](./SQLite-WAL优化总结.md)

## 技术栈

### 核心框架
- **FastAPI 0.135** - 现代高性能 Python Web 框架，自动生成交互式 API 文档
- **Uvicorn 0.41** - ASGI 服务器，支持异步处理
- **SQLAlchemy 2.0** - ORM 框架，数据库操作抽象层
- **Pydantic 2.12** - 数据验证和序列化

### 安全与认证
- **PyJWT 2.12** - JWT Token 生成与验证
- **pwdlib + Argon2** - 密码哈希（Argon2 算法）
- **itsdangerous** - 安全签名和令牌

### 通信
- **WebSockets** - 实时视频流传输和设备控制
- **HTTP/RESTful API** - 标准接口规范

### 数据库
- **SQLite** - 轻量级关系型数据库
- WAL 模式 + synchronous=NORMAL 优化（写入延迟从 ~70ms 降至 ~1-5ms）

## 文件树结构

```
├── README.md                           后端文档(本文件)
├── requirements.txt                    Python 依赖列表
├── app/                                应用主目录
│   ├── main.py                         FastAPI 入口文件
│   ├── Db.py                           数据库配置和连接管理
│   ├── Security.py                     JWT 认证和安全工具
│   ├── schema.py                       Pydantic 数据模型定义
│   ├── Logset.py                       日志配置（异步日志、分级存储）
│   ├── generate_uuid.py                UUID 生成工具
│   ├── datacontrol/                    数据库模型和操作层
│   │   ├── UserDb.py                   用户表模型和 CRUD
│   │   ├── DeviceDb.py                 设备表模型和 CRUD
│   │   ├── BirdDb.py                   雏鸟表模型和 CRUD
│   │   ├── SensorDb.py                 传感器数据表（温湿度）
│   │   ├── SensorUploadDb.py           传感器上传记录表（原始数据）
│   │   ├── DeviceLogDb.py              设备日志表
│   │   ├── FeedingDb.py                喂食记录表
│   │   └── FirmwareDb.py               固件信息表
│   ├── userapi/                        用户相关 API
│   │   ├── Login.py                    登录/注册/Token 刷新
│   │   ├── User.py                     用户管理（CRUD、筛选）
│   │   └── Bird.py                     雏鸟管理 API
│   ├── deviceapi/                      设备管理 API
│   │   └── Device.py                   设备 CRUD、鸟笼分组
│   ├── sensorapi/                      传感器数据 API
│   │   ├── SensorData.py               分组统计、历史查询
│   │   └── SensorUpload.py             传感器原始数据上传/查询
│   ├── stream/                         实时视频流模块
│   │   ├── Stream.py                   WebSocket 设备连接和消息转发
│   │   └── Viewer.py                   观看者 WebSocket 连接管理
│   ├── feedingapi/                     喂食系统 API
│   │   └── Feeding.py                  触发喂食、权限验证、状态查询
│   ├── logapi/                         设备日志 API
│   │   └── DeviceLog.py                日志查询、清理
│   ├── invitationapi/                  邀请码系统
│   │   └── InvitationCode.py           邀请码生成和验证
│   ├── firmwareapi/                    固件管理
│   │   └── Firmware.py                 固件版本控制
│   └── tests/                          单元测试
│       ├── test_login.py               登录相关测试
│       ├── test_users.py               用户管理测试
│       ├── test_birds.py               雏鸟管理测试
│       └── test_db.py                  数据库操作测试
├── database/                           SQLite 数据库文件目录
└── log/                                日志文件目录
    ├── app.log                         INFO/WARNING 级别日志
    ├── error.log                       ERROR 及以上级别日志
    └── requests.log                    HTTP 请求日志
```

## 数据库模型

### 用户表 (users)
| 字段 | 类型 | 说明 |
|------|------|------|
| id | Integer | 主键自增 |
| username | String | 唯一用户名 |
| password_hash | String | Argon2 哈希密码 |
| nickname | String | 用户昵称 |
| role | String | 角色：root / user / clouduser |
| email | String | 邮箱 |
| avatar | String | 头像 URL |
| extra | JSON | 扩展信息 |

### 设备表 (devices)
| 字段 | 类型 | 说明 |
|------|------|------|
| id | Integer | 主键自增 |
| secret | String | 设备密钥（唯一） |
| name | String | 设备名称 |
| device_type | String | 类型：ESP32-CAM / ESP32-C3 |
| area | String | 所在区域 |
| number | Integer | 区域编号 |
| isOnline | Boolean | 是否在线 |
| status | String | 状态：stream / standby / warning / error / none |

### 雏鸟表 (birds)
| 字段 | 类型 | 说明 |
|------|------|------|
| id | Integer | 主键自增 |
| name | String | 雏鸟名称 |
| species | String | 品种 |
| birth_date | Date | 出生日期 |
| area | String | 绑定鸟笼区域 |
| number | Integer | 绑定鸟笼编号 |
| status | String | 状态：available / adopted / grown |
| description | String | 描述 |
| avatar_url | String | 照片 URL |

### 传感器数据表 (sensor_data)
| 字段 | 类型 | 说明 |
|------|------|------|
| id | Integer | 主键自增 |
| device_id | Integer | 所属设备 |
| timestamp | DateTime | 记录时间 |
| temperature | Float | 温度 |
| humidity | Float | 湿度 |

### 传感器上传表 (sensor_upload)
| 字段 | 类型 | 说明 |
|------|------|------|
| id | Integer | 主键自增 |
| device_id | String | 设备标识 |
| sensor_type | String | 传感器类型：pms9103m / sound_meter / veml7700 / uv_meter |
| data_type | String | 数据类型 |
| data | String | JSON 格式原始数据 |
| timestamp | DateTime | 上传时间 |

### 喂食记录表 (feeding_records)
| 字段 | 类型 | 说明 |
|------|------|------|
| id | Integer | 主键自增 |
| bird_id | Integer | 雏鸟 ID |
| user_id | Integer | 操作用户 |
| action_type | String | 动作类型：feeding |
| created_at | DateTime | 操作时间 |

## API 路由结构

| 前缀 | 模块 | 说明 |
|------|------|------|
| `/api/login` | Login | 登录、注册、Token 刷新 |
| `/api/users` | User | 用户管理 |
| `/api/birds` | Bird | 雏鸟管理 |
| `/api/devices` | Device | 设备管理 |
| `/api/sensors` | SensorData | 传感器分组统计 |
| `/api/sensor-upload` | SensorUpload | 传感器原始数据 |
| `/api/stream` | Stream | WebSocket 视频流 |
| `/api/feeding` | Feeding | 喂食控制 |
| `/api/logs` | DeviceLog | 设备日志 |
| `/api/invitation` | Invitation | 邀请码 |
| `/api/firmware` | Firmware | 固件管理 |

## 认证系统

### JWT Token 机制
- **Access Token**：有效期 30 分钟，用于 API 请求认证
- **Refresh Token**：存储在 Cookie 中，有效期 1 天，用于无感刷新 Access Token
- **Token 刷新**：前端在 401 时自动调用 `/api/refresh`，在 Cookie 有效期内可持续获取新 Token

### 用户角色
- **root**：管理员，拥有所有权限
- **user**：普通用户，可查看数据和设备
- **clouduser**：云养用户，只能访问云养相关接口

### 认证方式
```python
# Header 认证（标准 API 请求）
Authorization: Bearer <access_token>

# Cookie 认证（长期保持登录）
Cookie: long_token=<refresh_token>
```

## 核心功能模块

### 1. 设备管理 (deviceapi/Device.py)
- 设备注册、查询、更新、删除
- 鸟笼分组查询（按 area + number 分组）
- 在线状态管理
- 设备密钥验证

### 2. 实时视频流 (stream/)
#### 设备连接 (Stream.py)
- ESP32-CAM 设备通过 WebSocket 连接到 `/api/stream/ws`
- 设备发送二进制图片帧和 JSON 状态消息
- 服务器自动转发给所有订阅者

#### 观看者连接 (Viewer.py)
- 用户通过 WebSocket 连接到 `/api/stream/viewer/ws/{device_id}?token=<JWT>`
- 自动订阅指定设备
- 支持多设备并行观看（每个连接一个设备）
- 设备离线时自动通知并关闭连接

### 3. 传感器数据处理
#### 分组统计 (sensorapi/SensorData.py)
- `/api/sensors/grouped` - 按时间分组统计温湿度
- 支持自定义时间范围和分组数
- 返回每组的平均温度和湿度

#### 原始数据上传 (sensorapi/SensorUpload.py)
- `/api/sensor-upload` - 设备上传传感器原始数据
- 支持多种传感器类型：PM2.5、声音、光照、UV
- 数据以 JSON 字符串存储，灵活扩展

### 4. 喂食系统 (feedingapi/Feeding.py)
- `/api/feeding/trigger` - 触发喂食操作
- 权限验证：root 用户可直接喂食，clouduser 需绑定对应雏鸟
- 自动查找鸟笼内的 CAM 设备发送指令
- 每日限制 1 次喂食

### 5. 雏鸟管理 (userapi/Bird.py)
- 雏鸟 CRUD 操作
- 按状态筛选（available / adopted / grown）
- 绑定鸟笼（area + number）
- 领养记录管理

## 日志系统

### 分级日志存储
- **app.log** - INFO/WARNING 级别，按天轮转，保留 7 天
- **error.log** - ERROR 及以上级别，按 2MB 大小轮转，保留 10 个备份
- **requests.log** - HTTP 请求详情，按天轮转，保留 7 天

### 异步日志
```python
from Logset import async_log, logger

# 异步记录日志（不阻塞主线程）
await async_log(logger, "info", "操作成功")
await async_log(logger, "error", f"发生错误: {e}")
```

## 数据库连接管理

### 上下文管理器
```python
import Db

# 自动打开和关闭数据库连接
with Db.OpenDb("操作标签") as db:
    devices = Db.GetDevices(db)
    # 连接在 with 块结束时自动关闭
```

### FastAPI 依赖注入
```python
from fastapi import Depends

@app.get("/api/devices")
async def get_devices(db: Db.Session = Depends(Db.GetDb("get_devices"))):
    return Db.GetDevices(db)
```

## 开发命令

```bash
# 安装依赖
pip install -r requirements.txt

# 启动开发服务器（热重载）
python app/main.py

# 启动生产服务器
FASTAPI_DEPLOY=1 uvicorn app.main:app --host 0.0.0.0 --port 8080

# 运行测试
pytest app/tests/ -v

# 生成邀请码
python app/generate_uuid.py
```

## 环境变量

| 变量名 | 说明 | 默认值 |
|--------|------|--------|
| `FASTAPI_DEPLOY` | 部署模式（设置后启用生产配置） | 未设置 |

## 性能优化

### SQLite WAL 模式
```python
# Db.py 中配置
@event.listens_for(engine, "connect")
def _set_sqlite_pragma(dbapi_conn, connection_record):
    cursor = dbapi_conn.cursor()
    cursor.execute("PRAGMA journal_mode=WAL")
    cursor.execute("PRAGMA synchronous=NORMAL")
    cursor.close()
```

### 连接池
- pool_size=15
- max_overflow=0
- pool_timeout=3

### 定期清理任务
- 每 24 小时自动清理 7 天前的过期数据
- 清理范围：设备日志、传感器上传、传感器数据

## 测试

项目包含完整的单元测试：
- `test_login.py` - 登录、注册、Token 验证
- `test_users.py` - 用户管理 CRUD
- `test_birds.py` - 雏鸟管理 CRUD
- `test_db.py` - 数据库连接和操作

运行测试：
```bash
cd backend/app
pytest tests/ -v
```

## 项目特点

1. **异步架构**：基于 FastAPI + Uvicorn，全异步处理
2. **自动 API 文档**：访问 `/docs` 查看 Swagger UI，`/redoc` 查看 ReDoc
3. **双 Token 认证**：Access Token + Refresh Cookie，安全且用户体验好
4. **WebSocket 视频流**：低延迟实时视频传输，支持多设备并行
5. **鸟笼为中心**：设备按鸟笼分组管理，支持 CAM + C3 双设备
6. **六维传感器**：温度、湿度、PM2.5、声音、光照、紫外线
7. **权限分级**：root / user / clouduser 三级权限控制
8. **SQLite 优化**：WAL 模式实现高性能写入

## 如何登录?
1. 用户进入首页时,或者任何请求出现401状态码时, 首先调用`POST | /api/refresh`尝试自动登录, 判断其返回状态码
    - 返回状态码200
        1. 自动登录成功, 返回值中有
        ```json
        {
        "access_token": "(你获得的token)",
        "token_type": "bearer"
        }
        ```

        2. 设置请求头 `Authorization: bearer (你获得的token)` 接下来的所有请求都带上, 保持登录
    - 返回状态码401
        1. 登录失效, 跳转到登录页面使用户手动登录
        2. 获取表单, 发送`POST | /api/login` 判断其返回状态码
            - 状态码200 参考1,设置请求头token, 登录成功

> 只要手动登录过, cookie有效期一天, 有效期使用`POST | /api/refresh`自动登录获取token, token有效期15分钟, 过期则会造成401响应, 此时再次调用自动登录获取新token, 在cookie有效期内都可以随时获取新token, 超过有效期返回401, 再次手动登录

## 如何连接设备?
### 新模式（推荐）— 一个 WebSocket 对应一个设备
**一个用户可以同时开启多个 WebSocket 连接，每个连接订阅一个不同的设备。**

1. 直接开启ws连接, 传入设备id和token验证, 注意设备id是路径参数!
```
wss:/lark.mintlab.top/api/stream/viewer/ws/{device_id}?token=<JWT>
```
| 参数 | 类型 | 说明 |
|------|------|------|
| `device_id` | int (路径参数) | 要订阅的设备 ID |
| `token` | string (查询参数) | 用户 JWT 认证令牌 |

#### 错误情况

| 错误码 | 说明 |
|--------|------|
| WebSocket 1008 | `Device not found` — 设备 ID 不存在 |
| WebSocket 1008 | `Device not connected` — 设备不在线 |
| `{"code": 400, ...}` | 订阅失败（设备 WebSocket 不可达），连接随后关闭 |

#### 关键特性

- **自动订阅**：WebSocket 连接建立后自动订阅目标设备，无需额外 HTTP 调用
- **自动取消订阅**：WebSocket 断开时自动清理订阅关系
- **设备断开通知**：设备离线时，服务器向观看者发送 `"设备已断开"` 文本消息，随后关闭观看者 WebSocket
- **多设备并行观看**：同一用户可同时打开多个 WebSocket，每个连接观看不同设备
- **连接去重**：同一 (user, device) 对若已有连接，旧连接会被自动关闭


### 旧模式（已弃用）— 一个 WebSocket 对应多个设备

1. 进入实时页面, 前端调用`wss://lark.mintlab.top/api/stream/viewer/ws?token=(登录获取的token)`
与服务器建立ws连接, 注意token要传在参数里
2. 调用`GET /devices`获取全部设备, 检查状态, 设备状态由后端自动设置
    - **isOnline** 代表是否在线
    - **status** 代表设备状态
        - `standby`待机, 没有任何观看者
        - `stream`推流, 自动采集图片并传输
3. 按照返回设备数据中的`isOnline`字段显示可选列表, `为false`的显示灰色
4. 用户选择在线设备, 前端发送请求`POST /stream/viewer/following/{device_id}`开启对应设备推流
5. 从ws连接中接收设备json格式响应, code=1代表操作成功
    -   ```json
        {
        "code": 1,
        "msg": "进入推流模式.",
        "key": "status",
        "values": "stream"
        }
        ```
6. 从ws连接中接收二进制数据, 直接显示图片
    - 图片处理方法参考git `ws-test` 分支里的测试内容
7. 调用`DELETE /stream/viewer/following/{device_id}` 取消服务器推流(也可以直接断开ws连接)
8. 客户端ws断开服务器会自动处理内存, 当设备的观看者为0时, 自动关闭设备推流

## 嵌入式ws通信规定(待完善)

### 向设备发送格式
```json
{
    "code": 0,         // 0查询, 1设定
    "item": "status",  // 要操作的块
    "key":  "status",
    "values": "stream"
}
```

### 从设备接收格式
```json
{
    "code": 0,         // 0失败, 1成功
    "msg": "OK.",      // 返回信息
    "key":  "status",  // 返回键
    "values": "stream"  // 返回值
}
```

### 字段
所有前端请求只要满足json格式, 会直接被转发到设备端处理, 具体请求和响应格式请参考[嵌入式技术文档](../embedded/README.md)

- `code`
    - 1 设置
    - 0 查询
- `item`
    - status 状态部分
        - `key`
            - status
                - `values`
                    - stream 进入推流模式
                    - standby 进入待机模式
    - camera 摄像部分
        - `key`
            - frame_size
                - `values`
                    - FRAMESIZE_128X128
                    - FRAMESIZE_240X240
                    - FRAMESIZE_320X320
                    - FRAMESIZE_VGA
                    - FRAMESIZE_SVGA
                    - FRAMESIZE_HD
                    - FRAMESIZE_FHD
                - example
                    ```json
                    {
                        "code": 1,                 // 1设定
                        "item": "camera",          // 设置camera模块
                        "key":  "frame_size",      // 设置帧大小
                        "values": "FRAMESIZE_SVGA" // 帧大小
                    }
                    ```
            - jpeg_quality
                - `values`
                    - 数字0-63之间
            - set_hmirror 设置水平反转 重复设置会再次翻转
                - `values`
                    - 数字1确认修改
            - set_vflip 设置垂直反转 重复设置会再次翻转
                - `values`
                    - 数字1确认修改
                
