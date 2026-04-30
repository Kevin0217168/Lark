# ESP32-C3 传感器集线器 WebSocket 控制接口文档

## 1. 连接信息

设备启动后，会自动建立 WebSocket 连接到服务端。前端连接服务端 WS 并完成订阅后，发送的信息会自动转发到对应设备的 WS 端，设备的响应也会被原样转发返回。

## 2. 消息格式

所有通信均为 JSON 格式，UTF-8 编码。

### 2.1 请求格式

```json
{
    "code": 0,          // 0: 查询, 1: 设定
    "item": "device",   // 操作模块（device / light / sensor）
    "key": "version",   // 具体属性名
    "values": ""        // 查询时可为空，设定时传入值（可为字符串或数字）
}
```

字段说明：

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `code` | int | 是 | 0 表示查询，1 表示设定 |
| `item` | string | 是 | 操作模块，支持 `device`、`light`、`sensor` |
| `key` | string | 是 | 模块下的具体属性，见下表 |
| `values` | string / int | 否 | 设定时必填，查询时可为空字符串或任意值 |

### 2.2 响应格式

```json
{
    "code": 1,          // 1: 成功, 0: 失败
    "msg": "OK.",       // 返回信息
    "key": "version",   // 对应的 key
    "values": "1.0.0"   // 返回值（成功时有效，sensor 查询时为 JSON 字符串）
}
```

## 3. 支持的操作

### 3.1 device 模块（设备控制）

| key | 操作 | values 取值 | 说明 |
|-----|------|-------------|------|
| `version` | 查询 | - | 返回当前固件版本号 |
| `status` | 查询 | - | 返回设备在线状态 |
| `restart` | 设定 | 任意值 | 设备重启（发送响应后约 500ms 重启） |
| `ota` | 设定 | 任意值 | 启动 OTA 固件更新任务 |
| `auto_feed` | 设定 | 任意值 | 启动自动上粮任务（预留接口） |

#### 示例

**查询固件版本**
```json
{"code":0,"item":"device","key":"version","values":""}
```
响应：
```json
{"code":1,"msg":"OK.","key":"version","values":"1.0.0"}
```

**查询设备状态**
```json
{"code":0,"item":"device","key":"status","values":""}
```
响应：
```json
{"code":1,"msg":"OK.","key":"status","values":"online"}
```

**重启设备**
```json
{"code":1,"item":"device","key":"restart","values":""}
```
响应：
```json
{"code":1,"msg":"设备即将重启.","key":"restart","values":"ok"}
```
> 注意：设备会在发送响应后约 500ms 执行重启，WebSocket 连接将断开。

**启动 OTA 更新**
```json
{"code":1,"item":"device","key":"ota","values":""}
```
响应（成功启动）：
```json
{"code":1,"msg":"OTA任务已启动.","key":"ota","values":"ok"}
```
响应（启动失败）：
```json
{"code":0,"msg":"OTA任务启动失败.","key":"ota","values":""}
```

**启动自动上粮**
```json
{"code":1,"item":"device","key":"auto_feed","values":""}
```
响应（成功启动）：
```json
{"code":1,"msg":"自动上粮任务已启动.","key":"auto_feed","values":"ok"}
```
响应（启动失败）：
```json
{"code":0,"msg":"自动上粮任务启动失败.","key":"auto_feed","values":""}
```
> 注意：`auto_feed` 为预留接口，默认实现为弱符号空函数。需在其他文件中定义同名强符号函数来覆盖以实现实际功能。

### 3.2 light 模块（灯光控制）

| key | 操作 | values 取值 | 说明 |
|-----|------|-------------|------|
| `brightness` | 查询 | - | 返回当前灯光亮度（0~100） |
| `brightness` | 设定 | 数字 0~100（可字符串） | 设置灯光亮度 |

#### 示例

**查询当前亮度**
```json
{"code":0,"item":"light","key":"brightness","values":""}
```
响应：
```json
{"code":1,"msg":"OK.","key":"brightness","values":"100"}
```

**设置亮度为 50**
```json
{"code":1,"item":"light","key":"brightness","values":50}
```
响应：
```json
{"code":1,"msg":"亮度设置成功.","key":"brightness","values":"50"}
```

**设置亮度为 80（字符串）**
```json
{"code":1,"item":"light","key":"brightness","values":"80"}
```
响应：
```json
{"code":1,"msg":"亮度设置成功.","key":"brightness","values":"80"}
```
> 注意：`brightness` 为预留接口，默认 `light_set_brightness()` 为空实现。需在其他文件中定义强符号函数来覆盖以实现实际灯光控制。

### 3.3 sensor 模块（传感器数据查询）

sensor 模块仅支持查询（code=0）。`key` 为传感器名称，`values` 返回该传感器的原始 JSON 数据。

| key | 说明 | 支持的传感器 |
|-----|------|-------------|
| `ina231` | 电压/电流/功率传感器 (I²C) | INA231 |
| `sgp30` | 空气质量传感器 (I²C) | SGP30 |
| `veml7700` | 环境光传感器 (I²C) | VEML7700 |
| `pms9103m` | 颗粒物传感器 (UART) | PMS9103M |
| `sound_meter` | 噪声传感器 (ADC) | 模拟麦克风模块 |
| `uv_meter` | 紫外线传感器 (ADC) | 模拟 UV 模块 |

#### 3.3.1 INA231 — 电压/电流/功率

```json
{"code":0,"item":"sensor","key":"ina231","values":""}
```
响应：
```json
{"code":1,"msg":"OK.","key":"ina231","values":"{\"bus_v\":3.3000,\"bus_v_cal\":3.3012,\"shunt_v\":0.000125,\"current_ma\":1.250,\"power_mw\":4.127}"}
```

返回值字段：

| 字段 | 类型 | 单位 | 说明 |
|------|------|------|------|
| `bus_v` | float | V | 总线电压原始值 |
| `bus_v_cal` | float | V | 总线电压校准值（多点校准） |
| `shunt_v` | float | V | 分流电阻两端电压 |
| `current_ma` | float | mA | 电流 |
| `power_mw` | float | mW | 实际功率（校准后电压 × 电流） |

#### 3.3.2 SGP30 — 空气质量

```json
{"code":0,"item":"sensor","key":"sgp30","values":""}
```
响应：
```json
{"code":1,"msg":"OK.","key":"sgp30","values":"{\"co2_ppm\":450,\"tvoc_ppb\":12}"}
```

返回值字段：

| 字段 | 类型 | 单位 | 说明 |
|------|------|------|------|
| `co2_ppm` | uint | ppm | 等效二氧化碳浓度 |
| `tvoc_ppb` | uint | ppb | 总挥发性有机物浓度 |

#### 3.3.3 VEML7700 — 环境光

```json
{"code":0,"item":"sensor","key":"veml7700","values":""}
```
响应：
```json
{"code":1,"msg":"OK.","key":"veml7700","values":"{\"lux\":1250.50,\"gain\":2,\"integration_time\":100}"}
```

返回值字段：

| 字段 | 类型 | 单位 | 说明 |
|------|------|------|------|
| `lux` | float | lx | 光照度 |
| `gain` | uint | - | 当前增益档位 |
| `integration_time` | uint | ms | 积分时间 |

#### 3.3.4 PMS9103M — 颗粒物

```json
{"code":0,"item":"sensor","key":"pms9103m","values":""}
```
响应：
```json
{"code":1,"msg":"OK.","key":"pms9103m","values":"{\"pm1_0_cf1\":12,\"pm2_5_cf1\":18,\"pm10_cf1\":22,\"pm1_0_atm\":10,\"pm2_5_atm\":15,\"pm10_atm\":20,\"cnt_0_3um\":1500,\"cnt_0_5um\":420,\"cnt_1_0um\":88,\"cnt_2_5um\":15,\"cnt_5_0um\":3,\"cnt_10um\":1}"}
```

返回值字段：

| 字段 | 类型 | 单位 | 说明 |
|------|------|------|------|
| `pm1_0_cf1` | uint | μg/m³ | PM1.0 浓度（CF=1，标准粒子） |
| `pm2_5_cf1` | uint | μg/m³ | PM2.5 浓度（CF=1） |
| `pm10_cf1` | uint | μg/m³ | PM10 浓度（CF=1） |
| `pm1_0_atm` | uint | μg/m³ | PM1.0 浓度（大气环境） |
| `pm2_5_atm` | uint | μg/m³ | PM2.5 浓度（大气环境） |
| `pm10_atm` | uint | μg/m³ | PM10 浓度（大气环境） |
| `cnt_0_3um` | uint | 个/0.1L | ≥0.3μm 颗粒数 |
| `cnt_0_5um` | uint | 个/0.1L | ≥0.5μm 颗粒数 |
| `cnt_1_0um` | uint | 个/0.1L | ≥1.0μm 颗粒数 |
| `cnt_2_5um` | uint | 个/0.1L | ≥2.5μm 颗粒数 |
| `cnt_5_0um` | uint | 个/0.1L | ≥5.0μm 颗粒数 |
| `cnt_10um` | uint | 个/0.1L | ≥10μm 颗粒数 |

#### 3.3.5 Sound Meter — 噪声

```json
{"code":0,"item":"sensor","key":"sound_meter","values":""}
```
响应：
```json
{"code":1,"msg":"OK.","key":"sound_meter","values":"{\"raw\":2048,\"voltage_v\":1.650,\"db\":55.2}"}
```

返回值字段：

| 字段 | 类型 | 单位 | 说明 |
|------|------|------|------|
| `raw` | uint | - | ADC 原始采样值 |
| `voltage_v` | float | V | 麦克风输出电压 |
| `db` | float | dB | 校准后的分贝值 |

#### 3.3.6 UV Meter — 紫外线

```json
{"code":0,"item":"sensor","key":"uv_meter","values":""}
```
响应：
```json
{"code":1,"msg":"OK.","key":"uv_meter","values":"{\"raw\":512,\"voltage_v\":0.825,\"uv_index\":3.50,\"level\":\"Moderate\"}"}
```

返回值字段：

| 字段 | 类型 | 单位 | 说明 |
|------|------|------|------|
| `raw` | uint | - | ADC 原始采样值 |
| `voltage_v` | float | V | UV 传感器输出电压 |
| `uv_index` | float | - | 紫外线指数 (UVI) |
| `level` | string | - | 紫外线等级文字描述 |

> 注意：各传感器在启动时独立初始化。若某传感器未就绪，查询将返回失败响应。设备会在后续采集周期中自动重试初始化。

## 4. 错误处理

当请求格式错误、参数无效或操作失败时，响应中的 `code` 为 `0`，`msg` 包含具体错误信息。

常见错误示例：

| 错误场景 | 响应 |
|----------|------|
| JSON 解析失败 | `{"code":0,"msg":"JSON 数据解析失败","key":"","values":""}` |
| 缺少必要字段或类型错误 | `{"code":0,"msg":"JSON 字段缺失或类型错误","key":"","values":""}` |
| 无效的 code | `{"code":0,"msg":"无效的code","key":"xxx","values":""}` |
| 不支持的 item | `{"code":0,"msg":"不支持的item","key":"xxx","values":""}` |
| 不支持的设定操作 | `{"code":0,"msg":"该item不支持设定操作","key":"xxx","values":""}` |
| 未知的 device 键 | `{"code":0,"msg":"未知的device键.","key":"xxx","values":""}` |
| 未知的 light 键 | `{"code":0,"msg":"未知的light键.","key":"xxx","values":""}` |
| 亮度值超出范围 | `{"code":0,"msg":"亮度值无效，需在0~100之间.","key":"brightness","values":""}` |
| 传感器未就绪或名称未知 | `{"code":0,"msg":"传感器未就绪或名称未知","key":"xxx","values":""}` |

## 5. 注意事项

1. **连接时序**：设备启动后会自动连接 WiFi 并建立 WebSocket 连接，建议在连接建立后（`onopen`）发送命令。
2. **传感器就绪**：传感器初始化可能因硬件连接问题失败，查询前可先通过 `device/status` 确认设备在线。传感器失败不影响设备整体运行。
3. **sensor 查询触发实时读取**：每次 sensor 查询都会立即调用对应传感器的采集函数，获取当前最新值，不依赖后台定时采集周期。
4. **后台采集**：设备每分钟自动采集全部 6 个传感器数据并聚合上报到服务端。`sensor` 查询接口为独立的按需读取通道，两者互不影响。
5. **预留接口**：`light`（亮度控制）和 `device/auto_feed`（自动上粮）为预留弱符号接口，默认无实际操作。需在项目其他文件中定义同名强符号函数来实现具体功能。
6. **兼容性**：`values` 字段在设定时支持数字和字符串类型，建议按文档使用约定类型以避免歧义。

## 6. 快速测试

可使用 WebSocket 客户端进行测试：

```javascript
const ws = new WebSocket('wss://lark.mintlab.top/api/stream/device/ws?secret=YOUR_SECRET');

ws.onopen = () => {
    // 查询固件版本
    ws.send(JSON.stringify({code:0, item:"device", key:"version", values:""}));

    // 查询设备状态
    ws.send(JSON.stringify({code:0, item:"device", key:"status", values:""}));

    // 查询环境光
    ws.send(JSON.stringify({code:0, item:"sensor", key:"veml7700", values:""}));

    // 查询空气质量
    ws.send(JSON.stringify({code:0, item:"sensor", key:"sgp30", values:""}));

    // 设置灯光亮度
    ws.send(JSON.stringify({code:1, item:"light", key:"brightness", values:50}));
};

ws.onmessage = (event) => {
    if (typeof event.data === 'string') {
        console.log('收到:', JSON.parse(event.data));
    }
};
```

---

文档已涵盖所有已实现功能，前端可据此进行集成。如有疑问，请联系设备维护方。
