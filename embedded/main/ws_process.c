#include "Wifista.h"
#include "Camera.h"
#include "main.h"

static const char* TAG = "ws_text_process";

extern Device_t device;

void ws_text_handler(void *handler_args, int len, const char *data_ptr)
{
    cJSON *json = cJSON_ParseWithLength(data_ptr, len);
    if (json != NULL)
    {
        char *string = cJSON_Print(json);
        ESP_LOGI(TAG, "收到 JSON 数据, 解析成功: \n%s\n", string);
        cJSON_free(string);

        // 解析公共字段
        cJSON *code_item = cJSON_GetObjectItemCaseSensitive(json, "code");
        cJSON *item_item = cJSON_GetObjectItemCaseSensitive(json, "item");
        cJSON *key_item = cJSON_GetObjectItemCaseSensitive(json, "key");
        cJSON *values_item = cJSON_GetObjectItemCaseSensitive(json, "values");

        if (cJSON_IsNumber(code_item) && cJSON_IsString(item_item) && cJSON_IsString(key_item) && (cJSON_IsString(values_item) || cJSON_IsNumber(values_item)))
        {
            int code = code_item->valueint;
            const char *item = item_item->valuestring;
            const char *key = key_item->valuestring;

            // 处理 status 模块
            if (strcasecmp(item, "status") == 0 && code == 1)
            {
                if (strcasecmp(key, "status") == 0)
                {
                    const char *value_str = NULL;
                    char value_buf[32];
                    if (cJSON_IsString(values_item))
                    {
                        value_str = values_item->valuestring;
                    }
                    else if (cJSON_IsNumber(values_item))
                    {
                        snprintf(value_buf, sizeof(value_buf), "%d", values_item->valueint);
                        value_str = value_buf;
                    }

                    if (value_str && strcasecmp(value_str, "stream") == 0)
                    {
                        ESP_LOGI(TAG, "进入推流模式");
                        device.status = DEVICE_ON_STREAM;
                        char return_data[128] = "{\"code\":1,\"msg\":\"进入推流模式.\",\"key\":\"status\",\"values\":\"stream\"}";
                        WebsocketSendText(return_data, strlen(return_data));
                    }
                    else if (value_str && strcasecmp(value_str, "standby") == 0)
                    {
                        ESP_LOGI(TAG, "进入待机模式");
                        device.status = DEVICE_STANDBY;
                        char return_data[128] = "{\"code\":1,\"msg\":\"进入待机模式.\",\"key\":\"status\",\"values\":\"standby\"}";
                        WebsocketSendText(return_data, strlen(return_data));
                    }
                    else
                    {
                        char return_data[128];
                        snprintf(return_data, sizeof(return_data),
                                 "{\"code\":0,\"msg\":\"无效的状态值.\",\"key\":\"%s\",\"values\":\"%s\"}",
                                 key, value_str ? value_str : "unknown");
                        WebsocketSendText(return_data, strlen(return_data));
                    }
                }
                else
                {
                    char return_data[128];
                    snprintf(return_data, sizeof(return_data),
                             "{\"code\":0,\"msg\":\"未知的status键.\",\"key\":\"%s\",\"values\":\"\"}",
                             key);
                    WebsocketSendText(return_data, strlen(return_data));
                }
            }
            // 处理 camera 模块
            else if (strcasecmp(item, "camera") == 0 && code == 1)
            {
                sensor_t *s = esp_camera_sensor_get();
                if (s == NULL)
                {
                    ESP_LOGE(TAG, "相机未初始化");
                    WebsocketSendText("{\"code\":0,\"msg\":\"相机未初始化\",\"key\":\"\",\"values\":\"\"}", strlen("{\"code\":0,\"msg\":\"相机未初始化\",\"key\":\"\",\"values\":\"\"}"));
                    cJSON_Delete(json);
                    return;
                }

                int success = 0;
                char msg[64] = "";
                char values_buf[32] = "";   // 用于存储返回的values字符串

                // 根据key执行相应操作
                if (strcasecmp(key, "frame_size") == 0)
                {
                    // 获取帧大小字符串
                    const char *frame_str = NULL;
                    char temp_buf[32];
                    if (cJSON_IsString(values_item))
                    {
                        frame_str = values_item->valuestring;
                    }
                    else if (cJSON_IsNumber(values_item))
                    {
                        // 如果传入数字，转换为对应的字符串（可选）
                        snprintf(temp_buf, sizeof(temp_buf), "%d", values_item->valueint);
                        frame_str = temp_buf;
                    }

                    framesize_t fs = FRAMESIZE_INVALID;
                    if (strcasecmp(frame_str, "FRAMESIZE_128X128") == 0) fs = FRAMESIZE_128X128;
                    else if (strcasecmp(frame_str, "FRAMESIZE_240X240") == 0) fs = FRAMESIZE_240X240;
                    else if (strcasecmp(frame_str, "FRAMESIZE_320X320") == 0) fs = FRAMESIZE_320X320;
                    else if (strcasecmp(frame_str, "FRAMESIZE_VGA") == 0) fs = FRAMESIZE_VGA;
                    else if (strcasecmp(frame_str, "FRAMESIZE_SVGA") == 0) fs = FRAMESIZE_SVGA;
                    else if (strcasecmp(frame_str, "FRAMESIZE_HD") == 0) fs = FRAMESIZE_HD;
                    else if (strcasecmp(frame_str, "FRAMESIZE_FHD") == 0) fs = FRAMESIZE_FHD;

                    if (fs != FRAMESIZE_INVALID)
                    {
                        s->set_framesize(s, fs);
                        success = 1;
                        snprintf(msg, sizeof(msg), "帧大小设置成功");
                        // 返回设置后的帧大小（原样返回）
                        snprintf(values_buf, sizeof(values_buf), "%s", frame_str);
                    }
                    else
                    {
                        success = 0;
                        snprintf(msg, sizeof(msg), "无效的帧大小值");
                    }
                }
                else if (strcasecmp(key, "jpeg_quality") == 0)
                {
                    int quality = -1;
                    if (cJSON_IsNumber(values_item))
                    {
                        quality = values_item->valueint;
                    }
                    else if (cJSON_IsString(values_item))
                    {
                        quality = atoi(values_item->valuestring);
                    }

                    if (quality >= 0 && quality <= 63)
                    {
                        s->set_quality(s, quality);
                        success = 1;
                        snprintf(msg, sizeof(msg), "JPEG质量设置成功");
                        snprintf(values_buf, sizeof(values_buf), "%d", quality);
                    }
                    else
                    {
                        success = 0;
                        snprintf(msg, sizeof(msg), "JPEG质量值无效，需在0-63之间");
                    }
                }
                else if (strcasecmp(key, "set_hmirror") == 0)
                {
                    // 根据文档，重复设置会再次翻转，直接调用即可
                    s->set_hmirror(s, 1);   // 参数1表示翻转
                    success = 1;
                    snprintf(msg, sizeof(msg), "水平镜像已翻转");
                    snprintf(values_buf, sizeof(values_buf), "1");
                }
                else if (strcasecmp(key, "set_vflip") == 0)
                {
                    s->set_vflip(s, 1);
                    success = 1;
                    snprintf(msg, sizeof(msg), "垂直镜像已翻转");
                    snprintf(values_buf, sizeof(values_buf), "1");
                }
                else
                {
                    success = 0;
                    snprintf(msg, sizeof(msg), "未知的camera键");
                }

                // 构造返回JSON
                char return_data[256];
                snprintf(return_data, sizeof(return_data),
                         "{\"code\":%d,\"msg\":\"%s\",\"key\":\"%s\",\"values\":\"%s\"}",
                         success ? 1 : 0,
                         msg,
                         key,
                         success ? values_buf : "");
                WebsocketSendText(return_data, strlen(return_data));
            }
            else
            {
                // 其他未处理的情况
                char return_data[128];
                snprintf(return_data, sizeof(return_data),
                         "{\"code\":0,\"msg\":\"不支持的item或操作类型\",\"key\":\"%s\",\"values\":\"\"}",
                         key);
                WebsocketSendText(return_data, strlen(return_data));
                ESP_LOGW(TAG, "未处理的请求: item=%s, code=%d", item, code);
            }
        }
        else
        {
            ESP_LOGE(TAG, "返回的 JSON 缺少必要字段或类型错误");
            // 反馈状态
            char return_data[128] = "{\"code\":0,\"msg\":\"返回的 JSON 缺少必要字段或类型错误\",\"key\":\"\",\"values\":\"\"}";
            WebsocketSendText(return_data, strlen(return_data));
        }

        cJSON_Delete(json);
    }
    else
    {
        ESP_LOGI(TAG, "收到 JSON 数据, 解析失败");
        char return_data[128] = "{\"code\":0,\"msg\":\"JSON 数据解析失败\",\"key\":\"\",\"values\":\"\"}";
        WebsocketSendText(return_data, strlen(return_data));
    }
}