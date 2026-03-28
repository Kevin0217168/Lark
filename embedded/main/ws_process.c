#include "Wifista.h"
#include "Camera.h"
#include "main.h"
#include "esp_system.h"
#include "esp_app_format.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tasks.h"

static const char *TAG = "ws_cmd";

extern Device_t device;

/* ──────────────── 通用工具函数 ──────────────── */

/** 统一构造 JSON 响应并通过 WebSocket 发送 */
static void ws_reply(int ok, const char *msg, const char *key, const char *values)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"code\":%d,\"msg\":\"%s\",\"key\":\"%s\",\"values\":\"%s\"}",
             ok ? 1 : 0, msg, key ? key : "", values ? values : "");
    WebsocketSendText(buf, strlen(buf));
}

/** 从 cJSON values 项提取字符串（数字会转为字符串），返回 NULL 表示无效 */
static const char *extract_value_str(cJSON *values_item, char *buf, size_t buf_len)
{
    if (cJSON_IsString(values_item))
        return values_item->valuestring;
    if (cJSON_IsNumber(values_item)) {
        snprintf(buf, buf_len, "%d", values_item->valueint);
        return buf;
    }
    return NULL;
}

/* ──────────────── 帧大小 名称↔枚举 映射表 ──────────────── */

typedef struct {
    const char    *name;
    framesize_t    fs;
} FrameSizeEntry_t;

static const FrameSizeEntry_t fs_table[] = {
    { "FRAMESIZE_128X128", FRAMESIZE_128X128 },
    { "FRAMESIZE_240X240", FRAMESIZE_240X240 },
    { "FRAMESIZE_320X320", FRAMESIZE_320X320 },
    { "FRAMESIZE_VGA",     FRAMESIZE_VGA     },
    { "FRAMESIZE_SVGA",    FRAMESIZE_SVGA    },
    { "FRAMESIZE_HD",      FRAMESIZE_HD      },
    { "FRAMESIZE_FHD",     FRAMESIZE_FHD     },
};
#define FS_TABLE_SIZE (sizeof(fs_table) / sizeof(fs_table[0]))

static const char *framesize_to_str(framesize_t fs)
{
    for (size_t i = 0; i < FS_TABLE_SIZE; i++)
        if (fs_table[i].fs == fs) return fs_table[i].name;
    return "unknown";
}

static framesize_t str_to_framesize(const char *str)
{
    if (!str) return FRAMESIZE_INVALID;
    for (size_t i = 0; i < FS_TABLE_SIZE; i++)
        if (strcasecmp(str, fs_table[i].name) == 0) return fs_table[i].fs;
    return FRAMESIZE_INVALID;
}

/* ──────────────── 查询处理器 (code == 0) ──────────────── */

static void query_status(const char *key, cJSON *values_item)
{
    if (strcasecmp(key, "status") == 0) {
        const char *s = (device.status == DEVICE_ON_STREAM) ? "stream"
                      : (device.status == DEVICE_STANDBY)   ? "standby"
                      : "unknown";
        ws_reply(1, "OK.", key, s);
    } else {
        ws_reply(0, "未知的状态键", key, "");
    }
}

static void query_camera(const char *key, cJSON *values_item)
{
    sensor_t *s = esp_camera_sensor_get();
    if (!s) { ws_reply(0, "相机未初始化", key, ""); return; }

    char vbuf[32];
    if (strcasecmp(key, "frame_size") == 0) {
        ws_reply(1, "OK.", key, framesize_to_str(s->status.framesize));
    } else if (strcasecmp(key, "jpeg_quality") == 0) {
        snprintf(vbuf, sizeof(vbuf), "%d", s->status.quality);
        ws_reply(1, "OK.", key, vbuf);
    } else if (strcasecmp(key, "set_hmirror") == 0) {
        snprintf(vbuf, sizeof(vbuf), "%d", s->status.hmirror);
        ws_reply(1, "OK.", key, vbuf);
    } else if (strcasecmp(key, "set_vflip") == 0) {
        snprintf(vbuf, sizeof(vbuf), "%d", s->status.vflip);
        ws_reply(1, "OK.", key, vbuf);
    } else {
        ws_reply(0, "未知的camera键", key, "");
    }
}

static void query_device(const char *key, cJSON *values_item)
{
    if (strcasecmp(key, "version") == 0) {
        ws_reply(1, "OK.", key, esp_app_get_description()->version);
    } else {
        ws_reply(0, "未知的device键", key, "");
    }
}

/* ──────────────── 设定处理器 (code == 1) ──────────────── */

static void set_status(const char *key, cJSON *values_item)
{
    if (strcasecmp(key, "status") != 0) {
        ws_reply(0, "未知的status键.", key, "");
        return;
    }

    char vbuf[32];
    const char *val = extract_value_str(values_item, vbuf, sizeof(vbuf));

    if (val && strcasecmp(val, "stream") == 0) {
        ESP_LOGI(TAG, "进入推流模式");
        device.status = DEVICE_ON_STREAM;
        xSemaphoreGive(camera_stream_sem);   // 唤醒 camera_transmit_task
        ws_reply(1, "进入推流模式.", key, "stream");
    } else if (val && strcasecmp(val, "standby") == 0) {
        ESP_LOGI(TAG, "进入待机模式");
        device.status = DEVICE_STANDBY;
        ws_reply(1, "进入待机模式.", key, "standby");
    } else {
        ws_reply(0, "无效的状态值.", key, val ? val : "unknown");
    }
}

static void set_camera(const char *key, cJSON *values_item)
{
    sensor_t *s = esp_camera_sensor_get();
    if (!s) {
        ESP_LOGE(TAG, "相机未初始化");
        ws_reply(0, "相机未初始化", "", "");
        return;
    }

    char vbuf[32];

    if (strcasecmp(key, "frame_size") == 0) {
        const char *val = extract_value_str(values_item, vbuf, sizeof(vbuf));
        framesize_t fs = str_to_framesize(val);
        if (fs != FRAMESIZE_INVALID) {
            s->set_framesize(s, fs);
            ws_reply(1, "帧大小设置成功", key, val);
        } else {
            ws_reply(0, "无效的帧大小值", key, "");
        }
    } else if (strcasecmp(key, "jpeg_quality") == 0) {
        int quality = -1;
        if (cJSON_IsNumber(values_item))       quality = values_item->valueint;
        else if (cJSON_IsString(values_item))  quality = atoi(values_item->valuestring);

        if (quality >= 0 && quality <= 63) {
            s->set_quality(s, quality);
            snprintf(vbuf, sizeof(vbuf), "%d", quality);
            ws_reply(1, "JPEG质量设置成功", key, vbuf);
        } else {
            ws_reply(0, "JPEG质量值无效，需在0-63之间", key, "");
        }
    } else if (strcasecmp(key, "set_hmirror") == 0) {
        s->set_hmirror(s, 1);
        ws_reply(1, "水平镜像已翻转", key, "1");
    } else if (strcasecmp(key, "set_vflip") == 0) {
        s->set_vflip(s, 1);
        ws_reply(1, "垂直镜像已翻转", key, "1");
    } else {
        ws_reply(0, "未知的camera键", key, "");
    }
}

/** @return true 表示函数内部已处理 json 释放，调用方需直接 return */
static bool set_device(const char *key, cJSON *values_item, cJSON *json)
{
    if (strcasecmp(key, "restart") == 0) {
        ESP_LOGW(TAG, "收到重启指令，设备即将重启...");
        ws_reply(1, "设备即将重启.", key, "ok");
        cJSON_Delete(json);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        esp_restart();
        return true;  /* 不会到达，但保持语义完整 */
    }

    if (strcasecmp(key, "ota") == 0) {
        ESP_LOGI(TAG, "收到OTA指令，启动OTA任务...");
        TaskHandle_t h = NULL;
        if (xTaskCreate(ota_task, "ota_task_ws", 8192, NULL, 1, &h) == pdPASS) {
            ws_reply(1, "OTA任务已启动.", key, "ok");
        } else {
            ws_reply(0, "OTA任务启动失败.", key, "");
        }
        return false;
    }

    ws_reply(0, "未知的device键.", key, "");
    return false;
}

/* ──────────────── 分发表 ──────────────── */

typedef void (*query_handler_t)(const char *key, cJSON *values_item);

typedef struct {
    const char      *item;
    query_handler_t  query;   /* code == 0 */
    query_handler_t  set;     /* code == 1（device 特殊处理） */
} ItemDispatch_t;

static const ItemDispatch_t dispatch_table[] = {
    { "status", query_status, (query_handler_t)set_status },
    { "camera", query_camera, (query_handler_t)set_camera },
    { "device", query_device, NULL /* device 设定需要额外参数，单独处理 */ },
};
#define DISPATCH_TABLE_SIZE (sizeof(dispatch_table) / sizeof(dispatch_table[0]))

/* ──────────────── 入口 ──────────────── */

void ws_text_handler(void *handler_args, int len, const char *data_ptr)
{
    cJSON *json = cJSON_ParseWithLength(data_ptr, len);
    if (!json) {
        ESP_LOGW(TAG, "JSON 解析失败");
        ws_reply(0, "JSON 数据解析失败", "", "");
        return;
    }

    char *dbg = cJSON_Print(json);
    ESP_LOGD(TAG, "收到: %s", dbg);
    cJSON_free(dbg);

    cJSON *code_item   = cJSON_GetObjectItemCaseSensitive(json, "code");
    cJSON *item_item   = cJSON_GetObjectItemCaseSensitive(json, "item");
    cJSON *key_item    = cJSON_GetObjectItemCaseSensitive(json, "key");
    cJSON *values_item = cJSON_GetObjectItemCaseSensitive(json, "values");

    /* 字段校验 */
    if (!cJSON_IsNumber(code_item) || !cJSON_IsString(item_item) ||
        !cJSON_IsString(key_item)  || (!cJSON_IsString(values_item) && !cJSON_IsNumber(values_item)))
    {
        ESP_LOGE(TAG, "JSON 字段缺失或类型错误");
        ws_reply(0, "JSON 字段缺失或类型错误", "", "");
        cJSON_Delete(json);
        return;
    }

    int code        = code_item->valueint;
    const char *item = item_item->valuestring;
    const char *key  = key_item->valuestring;

    /* 通过分发表查找 item */
    const ItemDispatch_t *entry = NULL;
    for (size_t i = 0; i < DISPATCH_TABLE_SIZE; i++) {
        if (strcasecmp(item, dispatch_table[i].item) == 0) {
            entry = &dispatch_table[i];
            break;
        }
    }

    if (!entry) {
        ws_reply(0, (code == 0) ? "不支持的item" : "不支持的item或操作类型", key, "");
        if (code == 1) ESP_LOGW(TAG, "未处理的请求: item=%s, code=%d", item, code);
        cJSON_Delete(json);
        return;
    }

    if (code == 0) {
        /* 查询 */
        entry->query(key, values_item);
    } else if (code == 1) {
        /* 设定 —— device 模块需要传入 json 指针以支持重启 */
        if (strcasecmp(item, "device") == 0) {
            if (set_device(key, values_item, json))
                return;  /* json 已在内部释放（重启场景） */
        } else if (entry->set) {
            entry->set(key, values_item);
        }
    } else {
        ws_reply(0, "无效的code", key, "");
    }

    cJSON_Delete(json);
}