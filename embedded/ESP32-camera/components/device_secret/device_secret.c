#include <string.h>
#include "nvs.h"
#include "esp_log.h"
#include "device_secret.h"

#define SECRET_NAMESPACE "device"
#define SECRET_KEY       "secret"

// 默认密钥（首次烧录/未写入时使用）
char secret[SECRET_MAX_LEN] = "b1f9562544a348c98c57a66b32a92d32";

esp_err_t save_secret_to_nvs(const char *secret_val)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(SECRET_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    err = nvs_set_str(handle, SECRET_KEY, secret_val);
    if (err == ESP_OK) nvs_commit(handle);
    nvs_close(handle);
    return err;
}

esp_err_t load_secret_from_nvs(char *secret_buf, size_t buf_len)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(SECRET_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return err;
    size_t required_size = buf_len;
    err = nvs_get_str(handle, SECRET_KEY, secret_buf, &required_size);
    nvs_close(handle);
    return err;
}
