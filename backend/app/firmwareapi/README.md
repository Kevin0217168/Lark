# Firmware 固件管理模块文档

## 概述

Firmware 模块负责设备固件的**全生命周期管理**，包括：

- 固件文件上传与存储
- 固件元数据的增删查
- 设备通过密钥获取最新固件信息
- 设备通过密钥下载最新固件二进制文件（用于 OTA 升级）

---

## 架构

```
┌─────────────┐                          ┌──────────┐
│  管理员前端  │ ── POST /upload ───────► │          │ ──► static/firmwares/ (文件存储)
│  (root)     │ ── DELETE /{id} ───────► │  Server  │ ──► SQLite firmwares 表 (元数据)
└─────────────┘                          │(Firmware)│
                                         │          │
┌─────────────┐                          │          │
│  ESP32 设备  │ ── POST /latest ──────► │          │ ──► 返回固件信息 JSON
│             │ ── POST /download ─────► │          │ ──► 返回固件二进制文件
└─────────────┘                          └──────────┘
```

### 核心流程

```
管理员上传固件                               设备 OTA 更新
    │                                           │
    ├─1─► 验证 root 权限                        ├─1─► 发送设备密钥
    ├─2─► 检查 设备类型+版本 是否重复             ├─2─► 服务器查询设备 → 获取 device_type
    ├─3─► 保存文件到 static/firmwares/           ├─3─► 根据 device_type 查询最新固件
    ├─4─► 写入数据库记录                         ├─4─► 返回固件信息 / 固件文件
    └─5─► 返回固件元数据                         └─5─► 设备执行 OTA 升级
```

---

## 数据库模型

### `firmwares` 表

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | Integer, PK, 自增 | 固件唯一主键 |
| `device_type` | String, 索引 | 适用的设备类型（如 `ESP32-CAM`） |
| `name` | String | 固件名称（如 `lark_firmware`） |
| `version` | String | 固件版本号（如 `1.0.0`） |
| `filepath` | String | 固件文件相对路径（如 `static/firmwares/ESP32-CAM_1.0.0_fw.bin`） |

### `devices` 表新增字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `device_type` | String | 设备类型，用于关联对应固件 |

> 设备的 `device_type` 字段与固件的 `device_type` 字段进行匹配，实现"按设备类型分发固件"。

---

## 文件存储

固件文件统一存放在 `static/firmwares/` 目录下，文件命名规则：

```
{device_type}_{version}_{原始文件名}
```

示例：
```
static/firmwares/ESP32-CAM_1.0.0_firmware.bin
static/firmwares/ESP32-S3_2.1.0_lark_ota.bin
```

该目录在模块加载时自动创建（`os.makedirs(..., exist_ok=True)`）。

---

## API 接口

### 1. 查询固件列表

```
GET /firmwares?device_type={可选}
```

| 参数 | 位置 | 类型 | 必填 | 说明 |
|------|------|------|------|------|
| `device_type` | Query | string | 否 | 按设备类型筛选 |

**认证**：无需认证

**响应**：

| 状态码 | 说明 |
|--------|------|
| 200 | 成功返回固件列表 |
| 404 | 无匹配固件 |

**响应示例**：
```json
{
  "code": 200,
  "msg": "successful!",
  "data": [
    {
      "id": 1,
      "device_type": "ESP32-CAM",
      "name": "lark_firmware",
      "version": "1.0.0",
      "filepath": "static/firmwares/ESP32-CAM_1.0.0_firmware.bin"
    }
  ]
}
```

---

### 2. 上传固件

```
POST /firmwares/upload
Content-Type: multipart/form-data
Authorization: Bearer <token>
```

| 参数 | 位置 | 类型 | 必填 | 说明 |
|------|------|------|------|------|
| `device_type` | Form | string | 是 | 固件适用的设备类型 |
| `name` | Form | string | 是 | 固件名称 |
| `version` | Form | string | 是 | 固件版本号 |
| `file` | Form (File) | binary | 是 | 固件二进制文件 |

**认证**：需要 Bearer Token，且用户角色必须为 `root`

**处理流程**：

```
请求到达
  ├─► 验证用户权限（非 root → 403）
  ├─► 检查 device_type + version 是否已存在（重复 → 400）
  ├─► 生成文件名: {device_type}_{version}_{原始文件名}
  ├─► 将文件写入 static/firmwares/ 目录
  ├─► 在 firmwares 表中插入记录（存储相对路径）
  └─► 返回新固件元数据
```

**响应**：

| 状态码 | 说明 |
|--------|------|
| 200 | 上传成功，返回固件信息 |
| 400 | 相同设备类型+版本的固件已存在 |
| 403 | 权限不足（非 root 用户） |

**响应示例**：
```json
{
  "code": 200,
  "msg": "successful!",
  "data": {
    "id": 2,
    "device_type": "ESP32-CAM",
    "name": "lark_firmware",
    "version": "1.1.0",
    "filepath": "static/firmwares/ESP32-CAM_1.1.0_firmware.bin"
  }
}
```

---

### 3. 删除固件

```
DELETE /firmwares/{id}
Authorization: Bearer <token>
```

| 参数 | 位置 | 类型 | 必填 | 说明 |
|------|------|------|------|------|
| `id` | Path | int | 是 | 数据库固件唯一主键 id |

**认证**：需要 Bearer Token，且用户角色必须为 `root`

**处理流程**：

```
请求到达
  ├─► 验证用户权限（非 root → 403）
  ├─► 查询固件记录获取文件路径
  ├─► 删除服务器上的固件文件（若存在）
  ├─► 删除数据库中的固件记录
  └─► 返回成功
```

**响应**：

| 状态码 | 说明 |
|--------|------|
| 200 | 删除成功 |
| 403 | 权限不足（非 root 用户） |
| 404 | 固件不存在 |

---

### 4. 设备获取最新固件信息

```
POST /firmwares/latest
Content-Type: application/json
```

| 参数 | 位置 | 类型 | 必填 | 说明 |
|------|------|------|------|------|
| `secret` | Body | string | 是 | 设备密钥 |

**认证**：无需 Token，通过设备密钥认证

**处理流程**：

```
请求到达
  ├─► 根据 secret 查询设备
  │     └─ 设备不存在 → 404
  ├─► 检查设备 device_type 是否已设置
  │     └─ 未设置 → 404 "Device type not set"
  ├─► 根据 device_type 查询最新固件（按 id 降序取第一条）
  │     └─ 无固件 → 404
  └─► 返回固件元数据
```

**请求示例**：
```json
{
  "secret": "b1f9562544a348c98c57a66b32a92d32"
}
```

**响应示例**：
```json
{
  "code": 200,
  "msg": "successful!",
  "data": {
    "id": 2,
    "device_type": "ESP32-CAM",
    "name": "lark_firmware",
    "version": "1.1.0",
    "filepath": "static/firmwares/ESP32-CAM_1.1.0_firmware.bin"
  }
}
```

**错误响应**：

| 状态码 | msg | 说明 |
|--------|-----|------|
| 404 | `Device not found` | 密钥无效，设备不存在 |
| 404 | `Device type not set` | 设备未设置 device_type |
| 404 | `Firmware not found` | 该设备类型无可用固件 |

---

### 5. 设备下载最新固件文件

```
POST /firmwares/download
Content-Type: application/json
```

| 参数 | 位置 | 类型 | 必填 | 说明 |
|------|------|------|------|------|
| `secret` | Body | string | 是 | 设备密钥 |

**认证**：无需 Token，通过设备密钥认证

**处理流程**：

```
请求到达
  ├─► 根据 secret 查询设备
  │     └─ 设备不存在 → 404
  ├─► 检查设备 device_type 是否已设置
  │     └─ 未设置 → 404
  ├─► 根据 device_type 查询最新固件
  │     └─ 无固件 → 404
  ├─► 检查固件文件是否存在于服务器
  │     └─ 文件丢失 → 404 "Firmware file not found on server"
  └─► 返回固件二进制文件（FileResponse, application/octet-stream）
```

**请求示例**：
```json
{
  "secret": "b1f9562544a348c98c57a66b32a92d32"
}
```

**响应**：

| 状态码 | Content-Type | 说明 |
|--------|--------------|------|
| 200 | `application/octet-stream` | 固件二进制文件 |
| 404 | `application/json` | 设备/固件/文件不存在 |

> 设备端可直接将响应体写入 Flash 进行 OTA 升级。

---

## 数据控制层 (`FirmwareDb.py`)

| 函数 | 参数 | 返回 | 说明 |
|------|------|------|------|
| `GetFirmwares()` | `id`, `device_type`, `name`, `version`（均可选） | `List[M_Firmwares]` | 条件查询固件列表 |
| `GetLatestFirmware()` | `device_type` (必填) | `Optional[M_Firmwares]` | 按 id 降序获取最新一条固件 |
| `CreateFirmware()` | `device_type`, `name`, `version`, `filepath` | `M_Firmwares` | 创建固件记录 |
| `DeleteFirmware()` | `id` | `1` 或 `None` | 删除固件记录，成功返回 1 |

---

## 设备类型关联机制

固件分发依赖于设备表中的 `device_type` 字段：

```
┌──────────────────────┐         ┌──────────────────────┐
│      devices 表       │         │     firmwares 表      │
├──────────────────────┤         ├──────────────────────┤
│ id: 1                │         │ id: 1                │
│ secret: "abc123..."  │ ──────► │ device_type: "ESP32-CAM"
│ device_type: "ESP32-CAM"      │ name: "lark_fw"      │
│ name: "Bird-Cam-01"  │         │ version: "1.0.0"     │
│ area: "雏鸟区"       │         │ filepath: "static/..." │
└──────────────────────┘         └──────────────────────┘
                                 │ id: 2                │
        device_type 匹配 ──────► │ device_type: "ESP32-CAM"
                                 │ name: "lark_fw"      │
                                 │ version: "1.1.0"  ◄── 最新
                                 │ filepath: "static/..." │
                                 └──────────────────────┘
```

查询最新固件时，按 `id DESC` 取第一条记录。这意味着**后上传的固件被视为最新版本**。

---

## 与设备 OTA 的配合

设备端 OTA 升级的典型流程：

```
ESP32 设备                                     服务器
  │                                              │
  │  ① POST /firmwares/latest                    │
  │     {"secret": "abc123..."}                  │
  ├─────────────────────────────────────────────►│
  │                                              ├─ 查设备 → 取 device_type
  │                                              ├─ 查最新固件
  │◄─────────────────────────────────────────────┤
  │  {"data": {"version": "1.1.0", ...}}         │
  │                                              │
  │  ② 比较本地版本与远端版本                      │
  │     if 远端更新 → 下载                         │
  │                                              │
  │  ③ POST /firmwares/download                  │
  │     {"secret": "abc123..."}                  │
  ├─────────────────────────────────────────────►│
  │                                              ├─ 查设备 → 查固件 → 读文件
  │◄─────────────────────────────────────────────┤
  │  [固件二进制数据]                              │
  │                                              │
  │  ④ 写入 Flash → 重启                          │
```

---

## 文件结构

```
firmwareapi/
├── __init__.py       # 空文件（Python 包标记）
├── Firmware.py       # API 路由 & 业务逻辑
└── README.md         # 本文档

datacontrol/
├── FirmwareDb.py     # 固件数据库模型 & CRUD 函数

static/
└── firmwares/        # 固件文件存储目录（自动创建）

schema.py             # 新增: FirmwareOut, FirmwareItem, FirmwareSecret 模型
```

### Firmware.py 内部结构

| 区域 | 内容 |
|------|------|
| 常量 | `FIRMWARE_DIR` — 固件存储绝对路径 |
| 查询接口 | `get_firmwares()` — 按设备类型筛选固件列表 |
| 上传接口 | `upload_firmware()` — root 上传固件文件（multipart/form-data）|
| 删除接口 | `delete_firmware()` — root 删除固件（同时清理文件）|
| 设备查询接口 | `get_latest_firmware()` — 设备密钥 → 设备类型 → 最新固件信息 |
| 设备下载接口 | `download_latest_firmware()` — 设备密钥 → 最新固件二进制文件 |

### FirmwareDb.py 内部结构

| 区域 | 内容 |
|------|------|
| ORM 模型 | `M_Firmwares` — firmwares 表映射 |
| 查询函数 | `GetFirmwares()` — 多条件组合查询 |
| 最新查询 | `GetLatestFirmware()` — 按设备类型获取最新固件 |
| 创建函数 | `CreateFirmware()` — 插入新固件记录 |
| 删除函数 | `DeleteFirmware()` — 按 id 删除固件记录 |
