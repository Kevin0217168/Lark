#ifndef __REMOTE_LOG_H__
#define __REMOTE_LOG_H__

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief 远程日志模块（两阶段初始化，HTTP POST 上传）
 *
 * 使用流程:
 *   1. NVS 初始化后立即调用 remote_log_early_init()
 *      → 创建 16KB 环形缓冲区 + 安装 vprintf 钩子
 *      → WiFi/SNTP/OTA/WS 握手期间的日志全部缓存
 *   2. 网络就绪后调用 remote_log_start()
 *      → 启动 flush 任务，通过独立 HTTP POST 批量上传
 *      → 积攒的日志一次性刷出
 *
 * 认证: secret 放在 Authorization header
 */

/**
 * @brief 阶段一：早期初始化（WiFi 之前调用）
 *
 * 创建环形缓冲区并安装 vprintf 钩子。
 * 调用后 ESP_LOGx 输出会同时写入缓冲区，但尚不上传。
 *
 * @return ESP_OK 成功
 */
esp_err_t remote_log_early_init(void);

/**
 * @brief 阶段二：启动 HTTP POST 上传（网络就绪后调用）
 *
 * 启动 flush 任务，使用独立的 esp_http_client 发送（线程安全）。
 *
 * @param base_url  基础 URL，例如 "https://lark.mintlab.top"
 * @param path      API 路径，例如 "/api/logs"
 * @param port      端口号
 * @param secret    设备密钥，放在 Authorization header
 * @return ESP_OK 成功
 */
esp_err_t remote_log_start(const char *base_url, const char *path,
                           uint16_t port, const char *secret);

/**
 * @brief 检查日志上传是否已启动
 */
bool remote_log_is_connected(void);

/**
 * @brief 暂停远程日志上传（释放 TLS 连接，释放内部 SRAM 给 OTA 使用）
 *
 * flush 任务继续运行但跳过 HTTP 发送，日志暂存在环形缓冲区。
 * 持久 HTTP client 会被销毁以释放 TLS 会话占用的内部 SRAM。
 */
void remote_log_pause(void);

/**
 * @brief 恢复远程日志上传
 */
void remote_log_resume(void);

/**
 * @brief 重启前同步刷新：立即将缓冲区中的日志通过 HTTP POST 发出
 *
 * 在调用 esp_restart() 前调用此函数，确保缓冲区中的日志不会丢失。
 * 同步执行，不超过几百毫秒。
 */
void remote_log_flush_sync(void);

/**
 * @brief 停止远程日志（恢复默认 vprintf）
 */
void remote_log_deinit(void);

#endif /* __REMOTE_LOG_H__ */
