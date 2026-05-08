/*!
 *  @file Adafruit_VEML7700_espidf.h
 *
 *  ESP-IDF port of the Adafruit VEML7700 Lux sensor driver.
 *
 *  This port uses the ESP-IDF I2C master APIs directly.
 */

#ifndef _ADAFRUIT_VEML7700_ESP32_H
#define _ADAFRUIT_VEML7700_ESP32_H

#include <stdint.h>
#include <driver/i2c.h>
#include <esp_err.h>
#include <esp_timer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define VEML7700_I2CADDR_DEFAULT 0x10 ///< I2C address

#define VEML7700_ALS_CONFIG 0x00        ///< Light configuration register
#define VEML7700_ALS_THREHOLD_HIGH 0x01 ///< Light high threshold for irq
#define VEML7700_ALS_THREHOLD_LOW 0x02  ///< Light low threshold for irq
#define VEML7700_ALS_POWER_SAVE 0x03    ///< Power save register
#define VEML7700_ALS_DATA 0x04          ///< The light data output
#define VEML7700_WHITE_DATA 0x05        ///< The white light data output
#define VEML7700_INTERRUPTSTATUS 0x06   ///< What IRQ (if any)

#define VEML7700_INTERRUPT_HIGH 0x4000 ///< Interrupt status for high threshold
#define VEML7700_INTERRUPT_LOW 0x8000  ///< Interrupt status for low threshold

#define VEML7700_GAIN_1 0x00   ///< ALS gain 1x
#define VEML7700_GAIN_2 0x01   ///< ALS gain 2x
#define VEML7700_GAIN_1_8 0x02 ///< ALS gain 1/8x
#define VEML7700_GAIN_1_4 0x03 ///< ALS gain 1/4x

#define VEML7700_IT_100MS 0x00 ///< ALS integration time 100ms
#define VEML7700_IT_200MS 0x01 ///< ALS integration time 200ms
#define VEML7700_IT_400MS 0x02 ///< ALS integration time 400ms
#define VEML7700_IT_800MS 0x03 ///< ALS integration time 800ms
#define VEML7700_IT_50MS 0x08  ///< ALS integration time 50ms
#define VEML7700_IT_25MS 0x0C  ///< ALS integration time 25ms

#define VEML7700_PERS_1 0x00 ///< ALS irq persistence 1 sample
#define VEML7700_PERS_2 0x01 ///< ALS irq persistence 2 samples
#define VEML7700_PERS_4 0x02 ///< ALS irq persistence 4 samples
#define VEML7700_PERS_8 0x03 ///< ALS irq persistence 8 samples

#define VEML7700_POWERSAVE_MODE1 0x00 ///< Power saving mode 1
#define VEML7700_POWERSAVE_MODE2 0x01 ///< Power saving mode 2
#define VEML7700_POWERSAVE_MODE3 0x02 ///< Power saving mode 3
#define VEML7700_POWERSAVE_MODE4 0x03 ///< Power saving mode 4

/** Options for lux reading method */
typedef enum {
  VEML_LUX_NORMAL,
  VEML_LUX_CORRECTED,
  VEML_LUX_AUTO,
  VEML_LUX_NORMAL_NOWAIT,
  VEML_LUX_CORRECTED_NOWAIT
} luxMethod;

class Adafruit_VEML7700 {
public:
  Adafruit_VEML7700(i2c_port_t i2c_num = I2C_NUM_0,
                    gpio_num_t sda_pin = GPIO_NUM_21,
                    gpio_num_t scl_pin = GPIO_NUM_20,
                    uint8_t addr = VEML7700_I2CADDR_DEFAULT,
                    SemaphoreHandle_t i2c_mutex = NULL);

  bool begin(int freq_hz = 100000);
  void enable(bool enable);
  bool enabled(void);
  void interruptEnable(bool enable);
  bool interruptEnabled(void);
  void setPersistence(uint8_t pers);
  uint8_t getPersistence(void);
  void setIntegrationTime(uint8_t it, bool wait = true);
  uint8_t getIntegrationTime(void);
  int getIntegrationTimeValue(void);
  void setGain(uint8_t gain);
  uint8_t getGain(void);
  float getGainValue(void);
  void powerSaveEnable(bool enable);
  bool powerSaveEnabled(void);
  void setPowerSaveMode(uint8_t mode);
  uint8_t getPowerSaveMode(void);
  void setLowThreshold(uint16_t value);
  uint16_t getLowThreshold(void);
  void setHighThreshold(uint16_t value);
  uint16_t getHighThreshold(void);
  uint16_t interruptStatus(void);
  uint16_t readALS(bool wait = false);
  uint16_t readWhite(bool wait = false);
  float readLux(luxMethod method = VEML_LUX_NORMAL);

private:
  static constexpr float MAX_RES = 0.0036f;
  static constexpr float GAIN_MAX = 2.0f;
  static constexpr float IT_MAX = 800.0f;
  static constexpr int I2C_TIMEOUT_MS = 1000;

  float getResolution(void);
  float computeLux(uint16_t rawALS, bool corrected = false);
  float autoLux(void);
  void readWait(void);
  uint32_t millis(void) { return (uint32_t)(esp_timer_get_time() / 1000ULL); }

  esp_err_t writeRegister(uint8_t reg, uint16_t value);
  esp_err_t readRegister(uint8_t reg, uint16_t &value);
  esp_err_t updateRegisterBits(uint8_t reg, uint16_t mask, uint8_t shift, uint16_t value);
  uint16_t readRegisterBits(uint8_t reg, uint16_t mask, uint8_t shift);

  i2c_port_t _i2c_num;
  gpio_num_t _sda_pin;
  gpio_num_t _scl_pin;
  uint8_t _addr;
  int _freq_hz;
  unsigned long _lastRead;
  bool _initialized;
  SemaphoreHandle_t _i2c_mutex;
};

#endif // _ADAFRUIT_VEML7700_ESP32_H
