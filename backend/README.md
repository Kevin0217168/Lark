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
 

## 嵌入式ws通信规定

### 发送格式
```json
{
    "code": 0,         // 0查询, 1设定
    "item": "status",  // 要操作的块
    "key":  "status",
    "value": "stream"
}
```
### 接收格式
```json
{
    "code": 0,         // 0失败, 1成功
    "msg": "OK.",      // 返回信息
    "key":  "status",  // 返回键
    "value": "stream"  // 返回值
}
```