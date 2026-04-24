#ifndef __DEVICE_SECRET_H__
#define __DEVICE_SECRET_H__

#include <stddef.h>
#include "esp_err.h"

#define SECRET_MAX_LEN 64

/** 全局设备密钥（优先从 NVS 读取，否则使用编译时默认值） */
extern char secret[SECRET_MAX_LEN];

/** 写入 secret 到 NVS */
esp_err_t save_secret_to_nvs(const char *secret_val);

/** 从 NVS 读取 secret */
esp_err_t load_secret_from_nvs(char *secret_buf, size_t buf_len);

#endif /* __DEVICE_SECRET_H__ */
