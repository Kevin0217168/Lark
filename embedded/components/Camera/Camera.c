#include "Camera.h"

#define CAMERA_MODEL_AI_THINKER // Has PSRAM

#include "camera_pins.h"
#include "esp_psram.h"
#include "esp_timer.h"
#include "esp_log.h"

// Enable LED FLASH setting
#define CONFIG_LED_ILLUMINATOR_ENABLED 0

// LED FLASH setup
#if CONFIG_LED_ILLUMINATOR_ENABLED

#define LED_LEDC_CHANNEL 2 // Using different ledc channel/timer than camera
#define CONFIG_LED_MAX_INTENSITY 255

int led_duty = 0;
bool isStreaming = false;

#endif

static const char *TAG = "Camera";

void CameraInit()
{
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_UXGA;
    config.pixel_format = PIXFORMAT_JPEG; // for streaming
    // config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
    //                      for larger pre-allocated frame buffer.
    if (config.pixel_format == PIXFORMAT_JPEG)
    {
        size_t psram_size = esp_psram_get_size();
        if (esp_psram_is_initialized() && psram_size > 0)
        {
            ESP_LOGI(TAG, "PSRAM 已初始化，大小为 %zu 字节\n", psram_size);
            config.jpeg_quality = 10;
            config.fb_count = 2;
            config.grab_mode = CAMERA_GRAB_LATEST;
        }
        else
        {
            ESP_LOGW(TAG, "PSRAM 不可用, 缓冲区将设置到DRAM");
            // Limit the frame size when PSRAM is not available
            config.frame_size = FRAMESIZE_SVGA;
            config.fb_location = CAMERA_FB_IN_DRAM;
        }
    }

    // 初始化摄像头
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return;
    }

    // 获取传感器指针并检查
    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL)
    {
        ESP_LOGE(TAG, "Failed to get sensor");
        return;
    }

    if (s->id.PID == OV3660_PID)
    {
        // s->set_hmirror(s, 1);
        // s->set_vflip(s, 1);
    }

    // // initial sensors are flipped vertically and colors are a bit saturated
    // if (s->id.PID == OV2640_PID)
    // {
    //     s->set_vflip(s, 1);       // flip it back
    //     s->set_brightness(s, 1);  // up the brightness just a bit
    //     s->set_saturation(s, -2); // lower the saturation
    // }
    // // drop down frame size for higher initial frame rate
    // if (config.pixel_format == PIXFORMAT_JPEG)
    // {
    //     s->set_framesize(s, FRAMESIZE_QVGA);
    // }

    // #if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
    //     s->set_vflip(s, 1);
    //     s->set_hmirror(s, 1);
    // #endif

    // #if defined(CAMERA_MODEL_ESP32S3_EYE)
    //     s->set_vflip(s, 1);
    // #endif

    // // Setup LED FLash if LED pin is defined in camera_pins.h
    // #if defined(LED_GPIO_NUM)
    //     setupLedFlash(LED_GPIO_NUM);
    // #endif
}

void CameraTakePhoto(void(PhotoHandler)(camera_fb_t *))
{
    int64_t start_us = esp_timer_get_time();

    camera_fb_t *fb = esp_camera_fb_get();

    if (fb == NULL)
    {
        ESP_LOGE(TAG, "Failed to get frame");
        return;
    }

    if (PhotoHandler != NULL)
    {
        PhotoHandler(fb);
    }

    esp_camera_fb_return(fb);

    int64_t end_us = esp_timer_get_time();
    float time = (end_us - start_us) / 1000;
    ESP_LOGI(TAG, "拍摄照片处理用时: %.2f ms (%.2f FPS)", time, 1000 / time);
}
