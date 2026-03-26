# 后端接口文档
## 主要技术栈
FastAPI + SQLite
大部分接口遵循 RESTful 风格开发
## 项目文件树结构
```
.
├── README.md                             后端文档(本文件)
├── generate_uuid.py                      (生成唯一id的临时文件)
├── Db.py                                 SQLite数据库配置文件
├── main.py                               FastAPI入口文件
├── stream                                直播组件
│    ├── device.py                        设备路由
│    ├── stream.py                        直播路由(ws)
│    └── viewer.py                        观看者路由
└── userapi
    ├── Login.py                          登录路由
    ├── schema.py                         数据格式定义文件
    └── User.py                           用户路由

```

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
                
                