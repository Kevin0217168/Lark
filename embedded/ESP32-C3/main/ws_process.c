#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_app_format.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "cJSON.h"
#include "Wifista.h"
#include "tasks.h"

static const char *TAG = "ws_cmd";

/* ──────────────── 全局状态 ──────────────── */

/** 灯光亮度 0~100（预留，默认 100） */
static int g_light_brightness = 100;

/* ──────────────── 通用工具函数 ──────────────── */

/** 统一构造 JSON 响应并通过 WebSocket 发送 */
static void ws_reply(int ok, const char *msg, const char *key, const char *values)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"code\":%d,\"msg\":\"%s\",\"key\":\"%s\",\"values\":\"%s\"}",
             ok ? 1 : 0, msg, key ? key : "", values ? values : "");
    WebsocketSendText((uint8_t *)buf, strlen(buf));
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

/** 从 values 项提取 0~100 整数，失败返回 -1 */
static int extract_brightness(cJSON *values_item)
{
    if (cJSON_IsNumber(values_item))
        return values_item->valueint;
    if (cJSON_IsString(values_item))
        return atoi(values_item->valuestring);
    return -1;
}

/* ──────────────── 自动上粮任务（预留接口）─────────────── */

/**
 * @brief 自动上粮任务 - 预留接口
 * 用户可在其他文件中定义同名强符号函数来覆盖此弱符号实现。
 */
__attribute__((weak)) void auto_feed_task(void *pvParameter)
{
    ESP_LOGW(TAG, "自动上粮任务未实现（预留接口），请覆盖 auto_feed_task 函数");
    vTaskDelete(NULL);
}

/**
 * @brief 设置灯光亮度 - 预留接口
 * 用户可在其他文件中定义同名强符号函数来覆盖此弱符号实现。
 * @param brightness  0~100
 */
__attribute__((weak)) void light_set_brightness(int brightness)
{
    ESP_LOGW(TAG, "灯光控制未实现（预留接口），亮度=%d, 请覆盖 light_set_brightness 函数", brightness);
}

/* ──────────────── 查询处理器 (code == 0) ──────────────── */

static void query_device(const char *key, cJSON *values_item)
{
    if (strcasecmp(key, "version") == 0) {
        ws_reply(1, "OK.", key, esp_app_get_description()->version);
    } else if (strcasecmp(key, "status") == 0) {
        ws_reply(1, "OK.", key, "online");
    } else {
        ws_reply(0, "未知的device键", key, "");
    }
}

static void query_light(const char *key, cJSON *values_item)
{
    if (strcasecmp(key, "brightness") == 0) {
        char vbuf[8];
        snprintf(vbuf, sizeof(vbuf), "%d", g_light_brightness);
        ws_reply(1, "OK.", key, vbuf);
    } else {
        ws_reply(0, "未知的light键", key, "");
    }
}

static void query_sensor(const char *key, cJSON *values_item)
{
    /* key 即传感器名称: ina231 / sgp30 / veml7700 / pms9103m / sound_meter / uv_meter */
    char json_buf[256];
    if (sensor_read_one(key, json_buf, sizeof(json_buf))) {
        ws_reply(1, "OK.", key, json_buf);
    } else {
        ws_reply(0, "传感器未就绪或名称未知", key, "");
    }
}

/* ──────────────── 设定处理器 (code == 1) ──────────────── */

/** @return true 表示函数内部已处理 json 释放，调用方需直接 return */
static bool set_device(const char *key, cJSON *values_item, cJSON *json)
{
    if (strcasecmp(key, "restart") == 0) {
        ESP_LOGW(TAG, "收到重启指令，设备即将重启...");
        ws_reply(1, "设备即将重启.", key, "ok");
        cJSON_Delete(json);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        esp_restart();
        return true;
    }

    if (strcasecmp(key, "ota") == 0) {
        ESP_LOGI(TAG, "收到 OTA 指令，启动 OTA 任务...");
        static uint8_t ota_param;
        TaskHandle_t h = NULL;
        if (xTaskCreate(ota_task, "ota_task_ws", 8192, &ota_param, 1, &h) == pdPASS) {
            ws_reply(1, "OTA任务已启动.", key, "ok");
        } else {
            ws_reply(0, "OTA任务启动失败.", key, "");
        }
        return false;
    }

    if (strcasecmp(key, "auto_feed") == 0) {
        ESP_LOGI(TAG, "收到自动上粮指令，启动自动上粮任务...");
        static uint8_t feed_param;
        TaskHandle_t h = NULL;
        if (xTaskCreate(auto_feed_task, "auto_feed", 4096, &feed_param, 1, &h) == pdPASS) {
            ws_reply(1, "自动上粮任务已启动.", key, "ok");
        } else {
            ws_reply(0, "自动上粮任务启动失败.", key, "");
        }
        return false;
    }

    ws_reply(0, "未知的device键.", key, "");
    return false;
}

static void set_light(const char *key, cJSON *values_item)
{
    if (strcasecmp(key, "brightness") != 0) {
        ws_reply(0, "未知的light键.", key, "");
        return;
    }

    int b = extract_brightness(values_item);
    if (b < 0 || b > 100) {
        ws_reply(0, "亮度值无效，需在0~100之间.", key, "");
        return;
    }

    g_light_brightness = b;
    light_set_brightness(b);  /* 调用预留接口 */

    char vbuf[8];
    snprintf(vbuf, sizeof(vbuf), "%d", b);
    ws_reply(1, "亮度设置成功.", key, vbuf);
}

/* ──────────────── 分发表 ──────────────── */

typedef void (*query_handler_t)(const char *key, cJSON *values_item);

typedef struct {
    const char      *item;
    query_handler_t  query;   /* code == 0 */
    query_handler_t  set;     /* code == 1（device 特殊处理） */
} ItemDispatch_t;

static const ItemDispatch_t dispatch_table[] = {
    { "device", query_device, NULL /* device 设定需要 json 指针，单独处理 */ },
    { "light",  query_light,  (query_handler_t)set_light },
    { "sensor", query_sensor, NULL /* 传感器仅支持查询 */ },
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

    int         code = code_item->valueint;
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
        } else {
            ws_reply(0, "该item不支持设定操作", key, "");
        }
    } else {
        ws_reply(0, "无效的code", key, "");
    }

    cJSON_Delete(json);
}
