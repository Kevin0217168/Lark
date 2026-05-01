#include "Adafruit_VEML7700_espidf.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "VEML7700";

static const char *gainToString(uint8_t gain) {
  switch (gain) {
  case VEML7700_GAIN_1_8:
    return "1/8x";
  case VEML7700_GAIN_1_4:
    return "1/4x";
  case VEML7700_GAIN_1:
    return "1x";
  case VEML7700_GAIN_2:
    return "2x";
  default:
    return "unknown";
  }
}

static const char *integrationTimeToString(uint8_t it) {
  switch (it) {
  case VEML7700_IT_25MS:
    return "25 ms";
  case VEML7700_IT_50MS:
    return "50 ms";
  case VEML7700_IT_100MS:
    return "100 ms";
  case VEML7700_IT_200MS:
    return "200 ms";
  case VEML7700_IT_400MS:
    return "400 ms";
  case VEML7700_IT_800MS:
    return "800 ms";
  default:
    return "unknown";
  }
}

extern "C" void app_main(void) {
  Adafruit_VEML7700 sensor(I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_20);

  if (!sensor.begin()) {
    ESP_LOGE(TAG, "VEML7700 init failed");
    while (true) {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  ESP_LOGI(TAG, "VEML7700 initialized");

  while (true) {
    float lux = sensor.readLux(VEML_LUX_AUTO);
    uint8_t gainCode = sensor.getGain();
    uint8_t itCode = sensor.getIntegrationTime();
    ESP_LOGI(TAG, "Lux = %.2f", lux);
    ESP_LOGI(TAG, "Gain = %u (%s), Integration Time = %s", gainCode,
             gainToString(gainCode), integrationTimeToString(itCode));
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
