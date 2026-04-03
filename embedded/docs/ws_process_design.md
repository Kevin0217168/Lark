# 嵌入式 WebSocket 命令处理的扁平化设计：映射表 + 分发表实战

> 在嵌入式物联网项目中，设备端常常需要通过 WebSocket 接收来自服务端的 JSON 指令，完成查询、设置等操作。随着功能膨胀，传统的 `if-else` / `switch-case` 写法会迅速变得臃肿且难以维护。本文以一个 ESP32 项目的真实代码为例，介绍如何用 **映射表 + 分发表** 将命令处理逻辑彻底扁平化。

---

## 1. 问题：嵌套地狱

一个典型的 WebSocket 文本帧协议长这样：

```json
{
  "code": 0,
  "item": "camera",
  "key": "frame_size",
  "values": "FRAMESIZE_VGA"
}
```

- `code`：`0` 查询，`1` 设定
- `item`：模块名（`status` / `camera` / `device`）
- `key`：参数名
- `values`：参数值

最朴素的实现方式是：

```c
if (code == 0) {
    if (strcmp(item, "status") == 0) {
        if (strcmp(key, "status") == 0) { /* ... */ }
    } else if (strcmp(item, "camera") == 0) {
        if (strcmp(key, "frame_size") == 0) { /* ... */ }
        else if (strcmp(key, "jpeg_quality") == 0) { /* ... */ }
        // ...
    }
} else if (code == 1) {
    // 再来一遍……
}
```

**三层嵌套，每增加一个 item 或 key 就要在两处（查询/设定）各加一段。** 模块一多，这个函数轻松突破 300 行，可读性和可维护性直线下降。

---

## 2. 解法概览：两级扁平化

整体思路是将「嵌套分支」拆成两级独立的 **表驱动（Table-Driven）** 结构：

| 层级 | 职责 | 数据结构 |
|------|------|----------|
| **映射表** | 枚举值 ↔ 字符串的双向转换 | `FrameSizeEntry_t[]` |
| **分发表** | `item` → 对应的查询/设定处理函数 | `ItemDispatch_t[]` |

入口函数 `ws_text_handler` 只做三件事：**解析 JSON → 查分发表 → 调用处理函数**，不含任何业务逻辑。

```
┌─────────────────────┐
│  ws_text_handler()  │  入口：JSON 解析 + 字段校验
└────────┬────────────┘
         │ 遍历 dispatch_table[]
         ▼
┌────────────────────────────────────────┐
│  dispatch_table[i].query / .set        │  按 code 选择查询或设定
│  ┌──────────┬──────────┬────────────┐  │
│  │ status   │ camera   │ device     │  │  每个 item 一行
│  └──────────┴──────────┴────────────┘  │
└────────┬───────────────────────────────┘
         │ 在具体 handler 中按 key 处理
         ▼
   query_camera() / set_camera() / ...
```

---

## 3. 映射表：枚举 ↔ 字符串的零成本转换

嵌入式开发中经常需要在「人类可读的字符串」和「C 枚举值」之间转换。传统做法是写一大堆 `switch-case`，每增加一个枚举值就要改两处（`to_str` 和 `from_str`）。

### 定义

```c
typedef struct {
    const char    *name;
    framesize_t    fs;
} FrameSizeEntry_t;

static const FrameSizeEntry_t fs_table[] = {
    { "FRAMESIZE_128X128", FRAMESIZE_128X128 },
    { "FRAMESIZE_240X240", FRAMESIZE_240X240 },
    { "FRAMESIZE_VGA",     FRAMESIZE_VGA     },
    { "FRAMESIZE_SVGA",    FRAMESIZE_SVGA    },
    { "FRAMESIZE_HD",      FRAMESIZE_HD      },
    { "FRAMESIZE_FHD",     FRAMESIZE_FHD     },
};
```

### 正反查找

```c
// 枚举 → 字符串
static const char *framesize_to_str(framesize_t fs) {
    for (size_t i = 0; i < FS_TABLE_SIZE; i++)
        if (fs_table[i].fs == fs) return fs_table[i].name;
    return "unknown";
}

// 字符串 → 枚举
static framesize_t str_to_framesize(const char *str) {
    for (size_t i = 0; i < FS_TABLE_SIZE; i++)
        if (strcasecmp(str, fs_table[i].name) == 0) return fs_table[i].fs;
    return FRAMESIZE_INVALID;
}
```

**优势：**

- **单一数据源**：新增分辨率只需在 `fs_table[]` 加一行，正反查找自动生效。
- **编译期可见**：表是 `const` 数组，存储在 `.rodata`，不占 RAM。
- **大小写容错**：查找使用 `strcasecmp`，协议鲁棒性更好。

---

## 4. 分发表：item × code 的扁平路由

这是整个设计的核心。我们定义一个统一的处理函数签名，然后用结构体数组把 `item` 名称和对应的查询、设定函数关联起来。

### 处理函数签名

```c
typedef void (*query_handler_t)(const char *key, cJSON *values_item);
```

所有查询和设定 handler 都遵守同一签名（入参为 `key` 和 `values`），确保可以用函数指针统一调用。

### 分发表定义

```c
typedef struct {
    const char      *item;     // 模块名称
    query_handler_t  query;    // code == 0 时调用
    query_handler_t  set;      // code == 1 时调用
} ItemDispatch_t;

static const ItemDispatch_t dispatch_table[] = {
    { "status", query_status, (query_handler_t)set_status },
    { "camera", query_camera, (query_handler_t)set_camera },
    { "device", query_device, NULL },  // device 的 set 需要特殊处理
};
```

### 入口函数中的调度逻辑

```c
// 1. 遍历分发表，O(n) 查找 item
const ItemDispatch_t *entry = NULL;
for (size_t i = 0; i < DISPATCH_TABLE_SIZE; i++) {
    if (strcasecmp(item, dispatch_table[i].item) == 0) {
        entry = &dispatch_table[i];
        break;
    }
}

// 2. 未找到 → 统一错误响应
if (!entry) {
    ws_reply(0, "不支持的item", key, "");
    cJSON_Delete(json);
    return;
}

// 3. 按 code 分发
if (code == 0) {
    entry->query(key, values_item);
} else if (code == 1) {
    // device 模块的 set 需要额外传入 json（支持重启前释放资源）
    if (strcasecmp(item, "device") == 0) {
        if (set_device(key, values_item, json)) return;
    } else if (entry->set) {
        entry->set(key, values_item);
    }
}
```

**入口函数体只有约 50 行，且不含任何具体业务分支。** 所有业务逻辑都下沉到各个 handler 中。

---

## 5. 业务 Handler 内部：key 级别的平铺

每个 handler 内部再按 `key` 做一层 `if-else`，但此时每个函数只关心自己模块的字段，职责单一：

```c
static void query_camera(const char *key, cJSON *values_item) {
    sensor_t *s = esp_camera_sensor_get();
    if (!s) { ws_reply(0, "相机未初始化", key, ""); return; }

    char vbuf[32];
    if (strcasecmp(key, "frame_size") == 0) {
        ws_reply(1, "OK.", key, framesize_to_str(s->status.framesize));
    } else if (strcasecmp(key, "jpeg_quality") == 0) {
        snprintf(vbuf, sizeof(vbuf), "%d", s->status.quality);
        ws_reply(1, "OK.", key, vbuf);
    } else {
        ws_reply(0, "未知的camera键", key, "");
    }
}
```

- 每个 handler **不超过 30 行**，context 清晰
- 错误处理统一走 `ws_reply` 返回标准格式

---

## 6. 统一响应：ws_reply

所有对外输出都通过一个函数完成，保证协议格式一致：

```c
static void ws_reply(int ok, const char *msg, const char *key, const char *values) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"code\":%d,\"msg\":\"%s\",\"key\":\"%s\",\"values\":\"%s\"}",
             ok ? 1 : 0, msg, key ? key : "", values ? values : "");
    WebsocketSendText(buf, strlen(buf));
}
```

这带来的好处：
- 修改响应格式只改一处
- 不会出现某个分支忘了返回 `code` 字段的问题
- 调用处代码极其简洁

---

## 7. 特殊分支的优雅处理

并非所有模块都能完美套入统一签名。`device` 模块的 `restart` 指令需要在重启前手动释放 JSON 对象，函数签名多了一个 `cJSON *json` 参数。

处理方式是在分发表中将 `device.set` 设为 `NULL`，在入口函数中做一次显式判断：

```c
if (strcasecmp(item, "device") == 0) {
    if (set_device(key, values_item, json))
        return;  // json 已在 set_device 内部释放（重启场景）
}
```

`set_device` 返回 `bool` 来告知调用方是否已接管 `json` 的生命周期——这是一个**最小特例化**的策略：不为了追求完美统一而引入复杂的抽象，而是在必要时用最小的代码量处理例外。

---

## 8. 扩展性分析

### 新增一个 item 模块（如 `sensor`）

只需 3 步：

1. 编写 `query_sensor()` 和 `set_sensor()` 两个函数
2. 在分发表加一行：
   ```c
   { "sensor", query_sensor, (query_handler_t)set_sensor },
   ```
3. 完成。入口函数 **零改动**。

### 新增一个 key（如给 camera 加 `brightness`）

只需在 `query_camera` / `set_camera` 中各加一个 `else if` 分支。其他模块和入口函数完全不受影响。

### 新增一个枚举映射（如 `FRAMESIZE_QVGA`）

在 `fs_table[]` 加一行即可，`to_str` 和 `from_str` 自动适配。

---

## 9. 设计要点总结

| 设计原则 | 具体体现 |
|----------|----------|
| **表驱动替代分支** | 映射表消除 switch-case，分发表消除 if-else 嵌套 |
| **单一职责** | 入口函数只做路由，业务逻辑下沉到各 handler |
| **统一接口** | 所有 handler 遵循 `(key, values_item)` 签名 |
| **统一出口** | `ws_reply()` 保证响应格式一致 |
| **最小特例化** | 仅 `device.set` 做特殊处理，不过度抽象 |
| **编译期安全** | 映射表和分发表均为 `const`，存 `.rodata` 段 |
| **零外部依赖** | 仅依赖 cJSON + ESP-IDF 标准 API |

---

## 10. 适用场景

这套模式适用于任何「**协议字段 → 处理函数**」的映射场景：

- MQTT 主题路由
- HTTP REST 端点分发
- 串口 AT 指令解析
- BLE 特征值读写回调

核心思想不变：**用数据（表）代替控制流（分支），用函数指针代替内联逻辑。** 代码量可能差不多，但可读性和可维护性天差地别。

---

*基于 ESP-IDF v5.x + cJSON，运行于 ESP32 平台。*
