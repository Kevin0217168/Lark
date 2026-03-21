# ESP32-CAM WebSocket 控制接口文档

## 1. 连接信息

设备启动后，会自动建立 WebSocket 连接, 前端连接服务端ws并完成订阅后, 发送的信息会自动转发到对应的设备ws端, 设备的响应也会被原样转发返回

## 2. 消息格式

所有通信均为 JSON 格式，UTF-8 编码。

### 2.1 请求格式

```json
{
    "code": 0,          // 0: 查询, 1: 设定
    "item": "status",   // 操作模块（status / camera）
    "key": "status",    // 具体属性名
    "values": ""        // 查询时可为空，设定时传入值（可为字符串或数字）
}
```

字段说明：

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `code` | int | 是 | 0 表示查询，1 表示设定 |
| `item` | string | 是 | 操作模块，目前支持 `status` 和 `camera` |
| `key` | string | 是 | 模块下的具体属性，见下表 |
| `values` | string / int | 否 | 设定时必填，查询时可为空字符串或任意值 |

### 2.2 响应格式

```json
{
    "code": 1,          // 1: 成功, 0: 失败
    "msg": "OK.",       // 返回信息
    "key": "status",    // 对应的 key
    "values": "stream"  // 返回值（成功时有效）
}
```

## 3. 支持的操作

### 3.1 status 模块（设备状态）

| key | 操作 | values 取值 | 说明 |
|-----|------|-------------|------|
| `status` | 查询 | - | 返回当前设备模式：`stream`（推流）或 `standby`（待机） |
| `status` | 设定 | `"stream"` / `"standby"` | 切换设备工作模式 |

#### 示例

**查询状态**  
```json
{"code":0,"item":"status","key":"status","values":""}
```
响应：
```json
{"code":1,"msg":"OK.","key":"status","values":"stream"}
```

**设定为推流模式**  
```json
{"code":1,"item":"status","key":"status","values":"stream"}
```
响应：
```json
{"code":1,"msg":"进入推流模式.","key":"status","values":"stream"}
```

**设定为待机模式**  
```json
{"code":1,"item":"status","key":"status","values":"standby"}
```
响应：
```json
{"code":1,"msg":"进入待机模式.","key":"status","values":"standby"}
```

### 3.2 camera 模块（相机参数）

| key | 操作 | values 取值 | 说明 |
|-----|------|-------------|------|
| `frame_size` | 查询 | - | 返回当前帧大小（字符串） |
| `frame_size` | 设定 | 帧大小枚举字符串 | 支持：`FRAMESIZE_128X128`, `FRAMESIZE_240X240`, `FRAMESIZE_320X320`, `FRAMESIZE_VGA`, `FRAMESIZE_SVGA`, `FRAMESIZE_HD`, `FRAMESIZE_FHD` |
| `jpeg_quality` | 查询 | - | 返回当前 JPEG 压缩质量（0-63） |
| `jpeg_quality` | 设定 | 数字 0~63（可字符串） | 设置 JPEG 压缩质量，值越小质量越高 |
| `set_hmirror` | 查询 | - | 返回当前水平镜像状态（0/1） |
| `set_hmirror` | 设定 | 任意值（建议 1） | 执行一次水平镜像翻转（每次调用翻转一次） |
| `set_vflip` | 查询 | - | 返回当前垂直镜像状态（0/1） |
| `set_vflip` | 设定 | 任意值（建议 1） | 执行一次垂直镜像翻转（每次调用翻转一次） |

#### 示例

**查询当前帧大小**  
```json
{"code":0,"item":"camera","key":"frame_size","values":""}
```
响应：
```json
{"code":1,"msg":"OK.","key":"frame_size","values":"FRAMESIZE_SVGA"}
```

**设置帧大小为 VGA**  
```json
{"code":1,"item":"camera","key":"frame_size","values":"FRAMESIZE_VGA"}
```
响应：
```json
{"code":1,"msg":"帧大小设置成功","key":"frame_size","values":"FRAMESIZE_VGA"}
```

**查询 JPEG 质量**  
```json
{"code":0,"item":"camera","key":"jpeg_quality","values":""}
```
响应：
```json
{"code":1,"msg":"OK.","key":"jpeg_quality","values":"10"}
```

**设置 JPEG 质量为 20（数字）**  
```json
{"code":1,"item":"camera","key":"jpeg_quality","values":20}
```
响应：
```json
{"code":1,"msg":"JPEG质量设置成功","key":"jpeg_quality","values":"20"}
```

**查询水平镜像状态**  
```json
{"code":0,"item":"camera","key":"set_hmirror","values":""}
```
响应：
```json
{"code":1,"msg":"OK.","key":"set_hmirror","values":"0"}
```

**翻转水平镜像**  
```json
{"code":1,"item":"camera","key":"set_hmirror","values":1}
```
响应：
```json
{"code":1,"msg":"水平镜像已翻转","key":"set_hmirror","values":"1"}
```

## 4. 错误处理

当请求格式错误、参数无效或操作失败时，响应中的 `code` 为 `0`，`msg` 包含具体错误信息，`values` 为空或无效值。

常见错误示例：

| 错误场景 | 响应 |
|----------|------|
| JSON 解析失败 | `{"code":0,"msg":"JSON 数据解析失败","key":"","values":""}` |
| 缺少必要字段 | `{"code":0,"msg":"返回的 JSON 缺少必要字段或类型错误","key":"","values":""}` |
| 无效的 item | `{"code":0,"msg":"不支持的item","key":"status","values":""}` |
| 无效的 status 值 | `{"code":0,"msg":"无效的状态值.","key":"status","values":"unknown_mode"}` |
| 无效的 frame_size | `{"code":0,"msg":"无效的帧大小值","key":"frame_size","values":""}` |
| JPEG 质量超出范围 | `{"code":0,"msg":"JPEG质量值无效，需在0-63之间","key":"jpeg_quality","values":""}` |
| 相机未初始化 | `{"code":0,"msg":"相机未初始化","key":"","values":""}` |

## 5. 注意事项

1. **连接时序**：设备启动后会自动连接 WiFi 并建立 WebSocket 连接，前端应在连接建立后发送命令。可通过 WebSocket 的 `onopen` 事件判断。
2. **推流模式**：当设备处于 `stream` 模式时，会持续通过 WebSocket 发送摄像头帧数据（二进制），前端需正确处理二进制消息。
3. **参数生效**：相机参数（帧大小、质量、镜像）设置后会立即生效，但可能影响当前推流（如帧大小改变后图像尺寸变化）。
4. **镜像翻转**：`set_hmirror` 和 `set_vflip` 每次调用都会翻转一次，无法直接设置绝对状态，但可通过查询当前状态后计算需要翻转的次数来控制。
5. **兼容性**：`values` 字段在设定时支持数字和字符串，但建议按文档使用约定类型以避免歧义。

## 6. 快速测试

可使用 WebSocket 客户端（如 `wscat`）或前端代码进行测试：

```javascript
const ws = new WebSocket('wss://lark.mintlab.top/api/stream/device/ws?secret=b1f9562544a348c98c57a66b32a92d32');

ws.onopen = () => {
    // 查询设备状态
    ws.send(JSON.stringify({code:0, item:"status", key:"status", values:""}));
};

ws.onmessage = (event) => {
    // 处理文本消息（JSON）
    if (typeof event.data === 'string') {
        console.log('收到:', JSON.parse(event.data));
    }
    // 二进制数据为图像帧，需单独处理
};
```

---

文档已涵盖所有已实现功能，前端可据此进行集成。如有疑问，请联系设备维护方。