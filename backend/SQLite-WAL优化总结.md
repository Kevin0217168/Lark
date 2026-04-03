# SQLite 性能优化实战：从 70ms 到 1ms 的日志写入之旅

> 在 IoT 项目 Lark 中，ESP32 设备每秒向后端上报日志，FastAPI + SQLite 的写入延迟高达 70ms，优化后降至 1ms 以内。本文详解 SQLite 的 journal_mode、synchronous 机制原理，以及 ORM 层的批量写入优化。

## 背景

Lark 是一个 ESP32 物联网监控平台，嵌入式端通过 HTTP POST 每秒批量上报 `ESP_LOG` 日志到 FastAPI 后端，存入 SQLite。

在优化前，日志上报接口的处理时间波动很大：

```
POST /api/logs - 200 OK - 处理时间: 74.46ms
POST /api/logs - 200 OK - 处理时间: 28.54ms
POST /api/logs - 200 OK - 处理时间: 17.43ms
POST /api/logs - 200 OK - 处理时间: 71.78ms
POST /api/logs - 200 OK - 处理时间: 78.86ms
```

平均约 40ms，峰值接近 80ms。对于一个简单的日志写入操作来说，这个速度显然不正常。

## 一、SQLite Journal Mode 详解

SQLite 通过 **日志(Journal)** 机制保证事务的原子性——要么完全提交，要么完全回滚。`journal_mode` 控制的就是这个日志的工作方式。

### 1.1 DELETE 模式（默认）

```
PRAGMA journal_mode = DELETE;
```

这是 SQLite 的默认模式，工作流程：

```
写入事务开始
  ├─ 1. 创建 rollback journal 文件（.db-journal）
  ├─ 2. 将要修改的原始页拷贝到 journal 文件
  ├─ 3. fsync journal 文件 → 确保落盘
  ├─ 4. 修改数据库文件中的页
  ├─ 5. fsync 数据库文件 → 确保落盘
  └─ 6. 删除 journal 文件  → 事务完成标志
```

**关键痛点**：每次 commit 需要 **2 次 fsync**（步骤 3 和 5）。`fsync` 是告诉操作系统"把内存缓冲区的数据真正写入物理磁盘"，这是整个流程中最慢的操作。在 HDD 上一次 fsync 约 10-30ms，SSD 上约 1-10ms。

**读写互斥**：写事务持有排他锁期间，所有读操作被阻塞。

### 1.2 WAL 模式（Write-Ahead Logging）

```
PRAGMA journal_mode = WAL;
```

WAL 彻底反转了写入逻辑：

```
                      ┌──────────────┐
                      │  数据库文件    │  ← 只读（checkpoint 时才写入）
                      │  .db         │
                      └──────────────┘
                             ▲
                             │ checkpoint（批量回写）
                             │
┌──────────────┐      ┌──────────────┐
│  共享内存索引  │ ←──→ │  WAL 日志文件  │ ← 所有写入先到这里
│  .db-shm     │      │  .db-wal     │
└──────────────┘      └──────────────┘
```

**WAL 的写入流程**：

```
写入事务开始
  ├─ 1. 将修改后的页 追加(append) 到 WAL 文件末尾
  ├─ 2. fsync WAL 文件（取决于 synchronous 设置）
  └─ 3. 完成 ← 没有第二次 fsync，没有删除操作
```

**核心优势**：

| 特性 | DELETE | WAL |
|------|--------|-----|
| 写入操作 | 拷贝原始页 → 修改 → 删除 journal | 追加新页到 WAL 尾部 |
| fsync 次数 | 2 次 | 0~1 次 |
| 读写并发 | 写阻塞读 | 读写互不阻塞 |
| 文件操作 | 每次创建/删除 journal | 追加写入（顺序 I/O） |

**为什么 WAL 更快？**

1. **顺序写入** vs 随机写入：WAL 只做 append，磁盘顺序 I/O 比随机 I/O 快 10-100 倍
2. **单次 fsync** vs 两次：减少一半的磁盘同步等待
3. **无文件创建/删除**：省去了 journal 文件的 create/unlink 系统调用

**WAL 的 Checkpoint 机制**：

WAL 文件不会无限增长。当 WAL 达到 `wal_autocheckpoint`（默认 1000 页 ≈ 4MB）时，SQLite 自动将 WAL 中的修改回写到数据库主文件。这个过程叫 **checkpoint**，通常在读操作时顺带完成，不影响写入性能。

### 1.3 其他模式（简要）

| 模式 | 特点 |
|------|------|
| **TRUNCATE** | 类似 DELETE，但截断 journal 而非删除（省一次 unlink 系统调用） |
| **PERSIST** | journal 文件创建后不删除，header 置零表示无效（省 create + unlink） |
| **MEMORY** | journal 存在内存中，断电即丢（不安全，仅测试用） |
| **OFF** | 不创建 journal（无事务保护，极端场景才用） |

对于服务端应用，**WAL 是绝大多数场景的最优选择**。

## 二、Synchronous 级别详解

`synchronous` 控制 SQLite 调用 `fsync` 的频率，直接影响"数据安全性 vs 写入速度"的权衡。

```
PRAGMA synchronous = FULL | NORMAL | OFF;
```

### 各级别对比

#### FULL（默认值 = 2）

```
每次 commit:
  → fsync journal/WAL 文件  ✓
  → fsync 数据库文件         ✓
```

**最安全**：即使操作系统崩溃或突然断电，数据库也不会损坏，不会丢失已提交的事务。代价是每次写入都要等磁盘完成物理写入。

#### NORMAL（值 = 1）

```
每次 commit:
  → fsync WAL 文件          ✓（关键时刻才做）
  → fsync 数据库文件         ✗（交给 OS 决定何时刷盘）
```

**在 WAL 模式下的安全性分析**：

- **进程崩溃（segfault / kill）**：不丢数据 ✓ —— OS 缓冲区会自动刷盘
- **操作系统崩溃（内核 panic）**：可能丢失最后一个事务 —— OS 缓冲区中未刷盘的数据丢失
- **断电**：可能丢失最后一个事务 —— 同上
- **数据库损坏**：不会 ✓ —— WAL 的校验和机制保护完整性

> 💡 **关键理解**：NORMAL 在 WAL 模式下，丢的只是"最后一个未刷盘的事务"，数据库完整性始终有保障。对于 IoT 日志场景，丢失一条日志完全可接受。

#### OFF（值 = 0）

```
每次 commit:
  → fsync journal/WAL 文件  ✗
  → fsync 数据库文件         ✗
```

**最快但最危险**：完全不调用 fsync，任何非正常关机都可能导致**数据库损坏**。生产环境不推荐。

### 直觉理解

```
       安全性                     速度
 ◄─────────────────────────────────────────►
 OFF          NORMAL              FULL
 │             │                    │
 不 fsync      关键时刻 fsync       每次都 fsync
 断电可损坏    断电丢最后1个事务     绝对不丢
 ~0.1ms       ~1ms                 ~10-70ms
```

## 三、ORM 层优化：bulk_insert_mappings

SQLAlchemy 的标准 `add_all()` 会为每个对象维护 identity map（身份映射），跟踪对象状态变化。这对日志这种"写入后不再修改"的场景是纯浪费。

```python
# 优化前：add_all — 创建 ORM 对象 + identity map 跟踪
entries = []
for item in parsed:
    entries.append(M_DeviceLog(
        device_id=device_id,
        timestamp=timestamp,
        level=item["level"],
        ...
    ))
db.add_all(entries)  # 每个对象进入 Session 跟踪
db.commit()

# 优化后：bulk_insert_mappings — 直接插入字典，跳过 ORM 跟踪
mappings = [
    {
        "device_id": device_id,
        "timestamp": timestamp,
        "level": item["level"],
        ...
    }
    for item in parsed
]
db.bulk_insert_mappings(M_DeviceLog, mappings)  # 直接生成 INSERT 语句
db.commit()
```

`bulk_insert_mappings` 跳过了：
- 对象实例化（不创建 Python ORM 对象）
- Identity map 注册（不跟踪对象状态）  
- 属性事件触发（不触发 attribute event）

对于批量插入场景，性能提升约 2-5 倍。

## 四、实际优化效果

### 项目配置

```python
# Db.py — SQLAlchemy 引擎配置
from sqlalchemy import create_engine, event

engine = create_engine(
    "sqlite:///../database/lark.db",
    connect_args={"check_same_thread": False}
)

@event.listens_for(engine, "connect")
def _set_sqlite_pragma(dbapi_conn, connection_record):
    cursor = dbapi_conn.cursor()
    cursor.execute("PRAGMA journal_mode=WAL")
    cursor.execute("PRAGMA synchronous=NORMAL")
    cursor.close()
```

### 基准测试结果

测试环境：Linux, SQLite 3.x, SSD

**单条日志写入（50 次取样）**：

| 指标 | 优化前 (DELETE + FULL) | 优化后 (WAL + NORMAL) |
|------|----------------------|----------------------|
| 最小值 | ~17ms | **0.15ms** |
| 平均值 | ~40ms | **0.95ms** |
| 最大值 | ~79ms | **18.93ms**（首次冷启动） |

**6 条日志批量写入（50 次取样）**：

| 指标 | 优化后 (WAL + NORMAL + bulk_insert) |
|------|-------------------------------------|
| 最小值 | **0.21ms** |
| 平均值 | **0.27ms** |
| 最大值 | **0.52ms** |

> 平均写入延迟从 **40ms 降至不到 1ms**，提升约 **40 倍**。

### 数据库文件变化

```
优化前:
  lark.db           360K    ← 数据库主文件
  lark.db-journal   (临时)  ← 每次写入创建/删除

优化后:
  lark.db           360K    ← 数据库主文件（checkpoint 时更新）
  lark.db-shm        32K    ← 共享内存索引（WAL 读取加速）
  lark.db-wal       4.0M    ← WAL 日志（自动 checkpoint 回收）
```

## 五、适用场景总结

| 场景 | 推荐配置 | 理由 |
|------|---------|------|
| IoT 日志 / 传感器数据 | WAL + NORMAL | 高频写入，偶尔丢一条可接受 |
| 用户账号 / 配置数据 | WAL + FULL | 低频写入，数据不能丢 |
| 金融交易 / 订单 | WAL + FULL（或用 PostgreSQL） | 必须绝对安全 |
| 临时缓存 / 测试 | WAL + OFF 或 MEMORY | 速度优先，数据不重要 |

## 六、注意事项

1. **WAL 模式是持久的**：设置一次后写入数据库文件头，下次打开自动生效。但 `synchronous` 需要每次连接时设置。
2. **网络文件系统不兼容**：WAL 依赖共享内存（mmap），NFS/SMB 上不能用 WAL 模式。
3. **WAL 文件增长**：高频写入时 `.db-wal` 可能增长，但 `wal_autocheckpoint`（默认 1000 页 ≈ 4MB）会自动回收。
4. **备份注意**：备份 SQLite 数据库时，必须同时复制 `.db`、`.db-shm`、`.db-wal` 三个文件，或使用 `VACUUM INTO` 命令。
5. **只读场景无收益**：WAL 主要优化写入性能，纯读场景两种模式差异不大。

---

*Lark 项目的完整优化还包括日志解析改造：将 ESP32 批量文本拆分为单条存储，自动解析日志等级(E/W/I/D/V)、组件标签(tag)、设备 tick，支持按等级/标签/时间范围检索。这属于业务层改造，不在本文讨论范围。*
