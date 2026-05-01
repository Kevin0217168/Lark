# ESP32 WebSocket secret 参数变成乱码：从 C++ name mangling 到 Archive 链接陷阱

笔者花了整整一个下午，从一个简单的 `"secret=�f"` 乱码出发，顺藤摸瓜挖出了 C++ 调用 C
库、name mangling、强弱符号、Archive 链接顺序、CMake 增量编译陷阱等多个问题。这篇
文章还原完整的排查过程与方法论。

## 现象

ESP32-S3 上运行的项目，`main.cpp` 中拼接 WebSocket URL：

```cpp
// main.cpp
#include "device_secret.h"

char ws_path[256];
snprintf(ws_path, sizeof(ws_path),
         "/api/stream/device/ws?secret=%s", secret);
WebsocketStart("wss://lark.mintlab.top", ws_path, 443);
```

`secret` 定义在独立的 `device_secret` 组件中：

```c
// components/device_secret/device_secret.c
char secret[64] = "b1f9562544a348c98c57a66b32a92d32";
```

预期发出的 URL 是 `/api/stream/device/ws?secret=b1f9562544a348c98c57a66b32a92d32`，
但运行时日志打出：

```
I (5145) websocket: 启动 WebSocket 连接:
    wss://lark.mintlab.top:443/api/stream/device/ws?secret=�f
                                                          ^^^ 乱码
```

服务器返回 `HTTP 400`，浏览器直接访问同一 URL 正常。`secret` 显然没有读到正确的值。

---

## 排查第一层：C++ name mangling

### 什么是 name mangling？

C 语言的符号名和源码名称一一对应（`secret` → `secret`）。C++ 支持函数重载、
namespace，编译器必须把参数类型编码到符号名里——比如 `void foo(int)` 可能被编码
为 `_Z3fooi`。

如果 `secret` 在 C 文件中以 C 链接编译，而 C++ 文件以 C++ 链接去引用它，链接器
会找 mangled 后的名字，两者对不上。

### 检查

`device_secret.h` 原始版本：

```c
// 修复前
#ifndef __DEVICE_SECRET_H__
#define __DEVICE_SECRET_H__

#define SECRET_MAX_LEN 64
extern char secret[SECRET_MAX_LEN];

#endif
```

**确实缺了 `extern "C"`。** 第一反应就是加守卫：

```c
#ifdef __cplusplus
extern "C" {
#endif

extern char secret[SECRET_MAX_LEN];

#ifdef __cplusplus
}
#endif
```

编译刷写——**没有任何变化。**

### 但是：`extern "C"` 确实是必须的

这里先插一句：即使这次修复没有立刻解决表面问题，`extern "C"` 守卫本身是正确且必
需的。C++ 调用 C 库的头文件，要么在头文件中加守卫，要么调用方写：

```cpp
extern "C" {
#include "c_library.h"
}
```

只是这次的根因不在这一层。

---

## 排查第二层：CMake 为什么没重编译？

加了 `extern "C"` 后重新 `idf.py build`，只编译了 4 步：

```
[2/4] Performing build step for 'bootloader'
[3/4] No install step for 'bootloader'
[4/4] Completed 'bootloader'
```

`main.cpp` 没有被重编译。为什么？

`device_secret` 组件通过 `idf_component.yml` 引用外部路径：

```yaml
# main/idf_component.yml
device_secret:
  path: ../../ESP32-camera/components/device_secret
```

CMake 的增量编译追踪依赖于 `IMPLICIT_DEPENDS`，对项目树外的文件可能不追踪。修改
外部组件的头文件后，依赖它的 `.cpp` 不会自动重新编译。

**解决方法**：`idf.py fullclean build`，强制全量重编。

`fullclean` 后重新编译了 1102 步，`main.cpp` 确实重新编译了。刷写——**问题依旧**。

这说明 `extern "C"` 不是根因。问题在更深层。

---

## 排查第三层：引入 ELF 符号分析

既然代码逻辑没问题，就要直接查最终二进制中 `secret` 符号的实际情况。

### 工具 1：`nm` — 查看符号类型

```bash
xtensa-esp32s3-elf-nm build/ina231_example.elf | grep -w secret
```

输出：

```
3fc9beb0 V secret
```

`V` 表示 weak（弱符号）。而我们期望的是 `D`（.data section 中已初始化的强
符号）。`nm` 不显示大小，需要更详细的信息。

### 工具 2：`objdump -t` — 查看符号大小和 section

```bash
xtensa-esp32s3-elf-objdump -t build/ina231_example.elf | grep -w secret
```

输出：

```
3fc9beb0  w    O .dram0.data    00000004 secret
          ^                      ^^^^^^^^
         weak                    只有 4 字节！
```

**`secret` 只有 4 字节。** 而 `device_secret.c` 中定义的是 `char secret[64]`。
64 字节的数组绝不可能被链接成 4 字节——这必定是**另一个**不同来源的定义。

### 工具 3：Linker Map — 追踪符号来源

ESP-IDF 生成 `build/ina231_example.map`，记录每个符号来自哪个目标文件。

```bash
grep "secret" build/ina231_example.map | grep "\.data\."
```

输出：

```
.data.secret   0x3fc9beb0   0x4  esp-idf/main/libmain.a(tasks.cpp.obj)
```

**`secret` 来自 `tasks.cpp.obj`，而不是 `device_secret.c.obj`。**

`grep` 搜索整个项目的 `secret` 定义：

```cpp
// ESP32-C3/main/tasks.cpp:65（修复前）
__attribute__((weak)) const char *secret = "default-secret";
```

找到了。

---

## 排查第四层：为什么强符号没覆盖弱符号？

先厘清三个关键概念。

### 强弱符号

| 类型 | 定义 | 示例 |
|------|------|------|
| 强符号 | 普通全局定义，无 `weak` 标记 | `int x = 1;` |
| 弱符号 | 带 `__attribute__((weak))` | `__attribute__((weak)) int x = 0;` |
| 未定义引用 | `extern` 声明，需要链接时解析 | `extern int x;` |

常识性规则：**强符号覆盖弱符号。** 链接器看到一个强 `secret` 和一个弱 `secret`，
应该选强的。

### Archive 链接的特殊性

这句话在 `.o` 文件之间成立，但在 **Archive (`.a`)** 中不成立。

Archive 是若干 `.o` 打包在一起。链接器处理 Archive 时**不是**把所有 `.o` 都加
进来，而是按需提取——只有能解决当前未解析引用的 `.o` 才会被选中。

ESP-IDF 用 CMake 把每个组件编译成独立的 `.a`，链接顺序由组件依赖关系决定。在
这个项目中，`main` 组件排在 `device_secret` 前面。

### 实际发生的过程

```
链接器状态表：

1. main.cpp.obj 引用了 secret（未定义）
   → 链接器: "我需要一个叫 secret 的符号"

2. 扫描 libmain.a
   → tasks.cpp.obj 中有 __attribute__((weak)) const char *secret
   → 拉入 tasks.cpp.obj ✅
   → secret 已解析！链接器不再需要这个符号

3. 扫描 libdevice_secret.a
   → 没有任何未解析符号需要 secret
   → device_secret.c.obj 从不被加载
   → 里面的 char secret[64] = "b1f95..." 从未进入链接
```

**强符号所在的 `.o` 文件从未被链接器加载**，自然轮不到"强覆盖弱"这条规则出场。

验证——linker map 中 `libdevice_secret.a` 的 LOAD 块里没有 `device_secret.c.obj`：

```
LOAD esp-idf/device_secret/libdevice_secret.a    ← 库本身被加载了
# 但 .data.secret 的来源是:
.data.secret   0x4  esp-idf/main/libmain.a(tasks.cpp.obj)    ← 只有这一个
```

### 还有第二个问题：类型不兼容

```cpp
// tasks.cpp — 弱符号
__attribute__((weak)) const char *secret = "default-secret";
//                       ^^^^^^^^^^^^^^   指针，4 字节

// device_secret.c — 强符号
char secret[64] = "b1f956...";
//   ^^^^^^^^^  数组，64 字节
```

即使两个符号同时被链接器看到，它们也是**完全不同的对象**——一个是指针（`.data`
中 4 字节），一个是数组（`.data` 中 64 字节）。链接器不会因为它们同名就把它们
合并，而是报错 "multiple definition"。

### `__attribute__((weak))` 的设计初衷

弱符号的本意是提供 fallback："有真正定义就用真正的，没有就返回默认值"。函数级别
很常见：

```c
__attribute__((weak)) void setup_hardware(void) {
    // 默认空实现，平台代码可覆盖
}
```

但在跨 Archive 的场景下，如果弱符号的 `.o` 先被拉入，它就"阻断"了后面强符号的
加载。`weak` 不是"优先级低"，而是"可被替代的备用选项，但链接器不知道后面还有
更好的"。

---

## 修复

两步：

**1. `tasks.cpp`：删除弱符号，改为 include 真正的头文件**

```cpp
// 删除：
// __attribute__((weak)) const char *secret = "default-secret";

// 新增：
#include "device_secret.h"
```

**2. `device_secret.h`：加上 `extern "C"`**

```c
#ifdef __cplusplus
extern "C" {
#endif
extern char secret[SECRET_MAX_LEN];
#ifdef __cplusplus
}
#endif
```

`fullclean build` 后验证：

```bash
$ xtensa-esp32s3-elf-objdump -t build/ina231_example.elf | grep -w secret
3fc9bec0 g     O .dram0.data    00000040 secret
         ^                       ^^^^^^^^
        强符号(global)           64 字节 ✅
```

运行时日志：

```
I (3147) websocket: 启动 WebSocket 连接:
    wss://lark.mintlab.top:443/api/stream/device/ws?secret=
    b1f9562544a348c98c57a66b32a92d32
                                                          ✅
```

---

## 排查方法总结

| 层 | 工具 | 关键命令 | 发现 |
|----|------|----------|------|
| 直觉 | 经验 | 看代码 | `extern "C"` 缺失 |
| 构建 | CMake | `idf.py build` | 只有 4 步，没重编 |
| 符号类型 | `nm` | `nm elf \| grep symbol` | `V` = weak |
| 符号大小 | `objdump -t` | `objdump -t elf \| grep symbol` | 4 字节，不是 64 |
| 符号来源 | Linker Map | `grep symbol build/*.map` | 来自 `tasks.cpp` |
| 归档检查 | `nm` on `.o` | `nm component.obj \| grep symbol` | 确认单个文件的符号 |

推荐的排查顺序：

```
现象（日志乱码）
  ↓ 直觉
name mangling？→ 加 extern "C"
  ↓ 无效
编译是否生效？→ 看重新编译了多少步 → 太少 → fullclean
  ↓ 还不行
符号对了吗？→ nm → 发现是 weak 且只有 4B
  ↓
从哪来的？→ linker map → tasks.cpp.obj
  ↓
根因 → weak 符号阻断 Archive 中的强符号
```

---

## 知识点

**C++ 调用 C 库的完整清单**：
1. 头文件加 `#ifdef __cplusplus extern "C" { ... } #endif`
2. `.h` 改完后记得 `fullclean`——CMake 不一定追踪外部组件的头文件变更
3. 不要在当前编译单元定义同名弱符号——会阻断 Archive 中的强符号加载

**`__attribute__((weak))` 的陷阱**：
- `.o` 之间：强覆盖弱 ✅
- Archive 之间：弱符号的 `.o` 先被拉入 → 后续强符号的 `.o` 不加载 ❌
- 类型不匹配：`const char *` ≠ `char[64]`，链接器不合并

**Archive 链接规则**：
- `.a` 由多个 `.o` 打包而成
- 链接器按需提取 `.o`，不是全部加载
- 只有能解决"当前未解析符号"的 `.o` 才会被选中
- 被弱符号提前满足的符号，后续强符号永远不会被看到

---

*ESP-IDF v5.5.3 · ESP32-S3 · 2026-05-01*
