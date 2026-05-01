#include "Adafruit_VEML7700_espidf.h"
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

static const char *TAG = "Adafruit_VEML7700";

Adafruit_VEML7700::Adafruit_VEML7700(i2c_port_t i2c_num, gpio_num_t sda_pin, gpio_num_t scl_pin, uint8_t addr, SemaphoreHandle_t i2c_mutex)
    : _i2c_num(i2c_num), _sda_pin(sda_pin), _scl_pin(scl_pin), _addr(addr), _freq_hz(100000), _lastRead(0), _initialized(false), _i2c_mutex(i2c_mutex) {}

bool Adafruit_VEML7700::begin(int freq_hz) {
  _freq_hz = freq_hz;

  ESP_LOGI(TAG, "尝试初始化 VEML7700，地址=0x%02x, I2C端口=%d", _addr, _i2c_num);

  uint16_t id = 0;
  esp_err_t err = readRegister(VEML7700_ALS_CONFIG, id);
  ESP_LOGI(TAG, "读取 ALS_CONFIG (0x00) 返回: err=%s, value=0x%04x", 
           esp_err_to_name(err), id);
  
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "VEML7700 not found at address 0x%02x (err: %s)", _addr, esp_err_to_name(err));
    return false;
  }

  enable(false);
  interruptEnable(false);
  setPersistence(VEML7700_PERS_1);
  setGain(VEML7700_GAIN_1_8);
  setIntegrationTime(VEML7700_IT_100MS);
  powerSaveEnable(false);
  enable(true);
  _lastRead = millis();
  _initialized = true;
  return true;
}

esp_err_t Adafruit_VEML7700::writeRegister(uint8_t reg, uint16_t value) {
  uint8_t data[3] = {reg, (uint8_t)(value & 0xFF), (uint8_t)((value >> 8) & 0xFF)};
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (_addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write(cmd, data, sizeof(data), true);
  i2c_master_stop(cmd);
  
  esp_err_t err = ESP_ERR_INVALID_STATE;
  if (_i2c_mutex) {
    if (xSemaphoreTake(_i2c_mutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) == pdTRUE) {
      err = i2c_master_cmd_begin(_i2c_num, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
      xSemaphoreGive(_i2c_mutex);
    }
  } else {
    err = i2c_master_cmd_begin(_i2c_num, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
  }
  
  i2c_cmd_link_delete(cmd);
  return err;
}

esp_err_t Adafruit_VEML7700::readRegister(uint8_t reg, uint16_t &value) {
  uint8_t data[2] = {0};
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (_addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, reg, true);
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (_addr << 1) | I2C_MASTER_READ, true);
  i2c_master_read(cmd, data, sizeof(data), I2C_MASTER_LAST_NACK);
  i2c_master_stop(cmd);
  
  esp_err_t err = ESP_ERR_INVALID_STATE;
  if (_i2c_mutex) {
    if (xSemaphoreTake(_i2c_mutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) == pdTRUE) {
      err = i2c_master_cmd_begin(_i2c_num, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
      xSemaphoreGive(_i2c_mutex);
    }
  } else {
    err = i2c_master_cmd_begin(_i2c_num, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
  }
  
  i2c_cmd_link_delete(cmd);
  if (err == ESP_OK) {
    value = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
  }
  return err;
}

esp_err_t Adafruit_VEML7700::updateRegisterBits(uint8_t reg, uint16_t mask, uint8_t shift, uint16_t value) {
  uint16_t current = 0;
  esp_err_t err = readRegister(reg, current);
  if (err != ESP_OK) {
    return err;
  }
  uint16_t newValue = (current & ~mask) | ((value << shift) & mask);
  return writeRegister(reg, newValue);
}

uint16_t Adafruit_VEML7700::readRegisterBits(uint8_t reg, uint16_t mask, uint8_t shift) {
  uint16_t current = 0;
  if (readRegister(reg, current) != ESP_OK) {
    return 0;
  }
  return (current & mask) >> shift;
}

float Adafruit_VEML7700::readLux(luxMethod method) {
  bool wait = true;
  switch (method) {
  case VEML_LUX_NORMAL_NOWAIT:
    wait = false;
  case VEML_LUX_NORMAL:
    return computeLux(readALS(wait));
  case VEML_LUX_CORRECTED_NOWAIT:
    wait = false;
  case VEML_LUX_CORRECTED:
    return computeLux(readALS(wait), true);
  case VEML_LUX_AUTO:
    return autoLux();
  default:
    return -1.0f;
  }
}

uint16_t Adafruit_VEML7700::readALS(bool wait) {
  if (wait)
    readWait();
  _lastRead = millis();
  uint16_t value = 0;
  readRegister(VEML7700_ALS_DATA, value);
  return value;
}

uint16_t Adafruit_VEML7700::readWhite(bool wait) {
  if (wait)
    readWait();
  _lastRead = millis();
  uint16_t value = 0;
  readRegister(VEML7700_WHITE_DATA, value);
  return value;
}

void Adafruit_VEML7700::enable(bool enable) {
  updateRegisterBits(VEML7700_ALS_CONFIG, 0x0001, 0, !enable);
  if (enable) {
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

bool Adafruit_VEML7700::enabled(void) {
  return readRegisterBits(VEML7700_ALS_CONFIG, 0x0001, 0) == 0;
}

void Adafruit_VEML7700::interruptEnable(bool enable) {
  updateRegisterBits(VEML7700_ALS_CONFIG, 0x0002, 1, enable ? 1 : 0);
}

bool Adafruit_VEML7700::interruptEnabled(void) {
  return readRegisterBits(VEML7700_ALS_CONFIG, 0x0002, 1) == 1;
}

void Adafruit_VEML7700::setPersistence(uint8_t pers) {
  updateRegisterBits(VEML7700_ALS_CONFIG, 0x0030, 4, pers);
}

uint8_t Adafruit_VEML7700::getPersistence(void) {
  return (uint8_t)readRegisterBits(VEML7700_ALS_CONFIG, 0x0030, 4);
}

void Adafruit_VEML7700::setIntegrationTime(uint8_t it, bool wait) {
  int flushDelay = wait ? getIntegrationTimeValue() : 0;
  updateRegisterBits(VEML7700_ALS_CONFIG, 0x03C0, 6, it);
  if (flushDelay > 0) {
    vTaskDelay(pdMS_TO_TICKS(flushDelay));
  }
  _lastRead = millis();
}

uint8_t Adafruit_VEML7700::getIntegrationTime(void) {
  return (uint8_t)readRegisterBits(VEML7700_ALS_CONFIG, 0x03C0, 6);
}

int Adafruit_VEML7700::getIntegrationTimeValue(void) {
  switch (getIntegrationTime()) {
  case VEML7700_IT_25MS:
    return 25;
  case VEML7700_IT_50MS:
    return 50;
  case VEML7700_IT_100MS:
    return 100;
  case VEML7700_IT_200MS:
    return 200;
  case VEML7700_IT_400MS:
    return 400;
  case VEML7700_IT_800MS:
    return 800;
  default:
    return -1;
  }
}

void Adafruit_VEML7700::setGain(uint8_t gain) {
  updateRegisterBits(VEML7700_ALS_CONFIG, 0x1800, 11, gain);
  _lastRead = millis();
}

uint8_t Adafruit_VEML7700::getGain(void) {
  return (uint8_t)readRegisterBits(VEML7700_ALS_CONFIG, 0x1800, 11);
}

float Adafruit_VEML7700::getGainValue(void) {
  switch (getGain()) {
  case VEML7700_GAIN_1_8:
    return 0.125f;
  case VEML7700_GAIN_1_4:
    return 0.25f;
  case VEML7700_GAIN_1:
    return 1.0f;
  case VEML7700_GAIN_2:
    return 2.0f;
  default:
    return -1.0f;
  }
}

void Adafruit_VEML7700::powerSaveEnable(bool enable) {
  updateRegisterBits(VEML7700_ALS_POWER_SAVE, 0x0001, 0, enable ? 1 : 0);
}

bool Adafruit_VEML7700::powerSaveEnabled(void) {
  return readRegisterBits(VEML7700_ALS_POWER_SAVE, 0x0001, 0) == 1;
}

void Adafruit_VEML7700::setPowerSaveMode(uint8_t mode) {
  updateRegisterBits(VEML7700_ALS_POWER_SAVE, 0x0006, 1, mode);
}

uint8_t Adafruit_VEML7700::getPowerSaveMode(void) {
  return (uint8_t)readRegisterBits(VEML7700_ALS_POWER_SAVE, 0x0006, 1);
}

void Adafruit_VEML7700::setLowThreshold(uint16_t value) {
  writeRegister(VEML7700_ALS_THREHOLD_LOW, value);
}

uint16_t Adafruit_VEML7700::getLowThreshold(void) {
  uint16_t value = 0;
  readRegister(VEML7700_ALS_THREHOLD_LOW, value);
  return value;
}

void Adafruit_VEML7700::setHighThreshold(uint16_t value) {
  writeRegister(VEML7700_ALS_THREHOLD_HIGH, value);
}

uint16_t Adafruit_VEML7700::getHighThreshold(void) {
  uint16_t value = 0;
  readRegister(VEML7700_ALS_THREHOLD_HIGH, value);
  return value;
}

uint16_t Adafruit_VEML7700::interruptStatus(void) {
  uint16_t value = 0;
  readRegister(VEML7700_INTERRUPTSTATUS, value);
  return value;
}

float Adafruit_VEML7700::getResolution(void) {
  return MAX_RES * (IT_MAX / getIntegrationTimeValue()) * (GAIN_MAX / getGainValue());
}

float Adafruit_VEML7700::computeLux(uint16_t rawALS, bool corrected) {
  float lux = getResolution() * rawALS;
  if (corrected)
    lux = (((6.0135e-13f * lux - 9.3924e-9f) * lux + 8.1488e-5f) * lux + 1.0023f) * lux;
  return lux;
}

void Adafruit_VEML7700::readWait(void) {
  unsigned long timeToWait = 2UL * getIntegrationTimeValue();
  unsigned long timeWaited = millis() - _lastRead;
  if (timeWaited < timeToWait) {
    vTaskDelay(pdMS_TO_TICKS(timeToWait - timeWaited));
  }
}

float Adafruit_VEML7700::autoLux(void) {
  const uint8_t gains[] = {VEML7700_GAIN_1_8, VEML7700_GAIN_1_4, VEML7700_GAIN_1, VEML7700_GAIN_2};
  const uint8_t intTimes[] = {VEML7700_IT_25MS, VEML7700_IT_50MS, VEML7700_IT_100MS, VEML7700_IT_200MS, VEML7700_IT_400MS, VEML7700_IT_800MS};

  uint8_t gainIndex = 0;
  uint8_t itIndex = 2;
  bool useCorrection = false;

  setGain(gains[gainIndex]);
  setIntegrationTime(intTimes[itIndex]);

  uint16_t ALS = readALS(true);
  if (ALS <= 100) {
    while ((ALS <= 100) && !((gainIndex == 3) && (itIndex == 5))) {
      if (gainIndex < 3) {
        setGain(gains[++gainIndex]);
      } else if (itIndex < 5) {
        setIntegrationTime(intTimes[++itIndex]);
      }
      ALS = readALS(true);
    }
  } else {
    useCorrection = true;
    while ((ALS > 10000) && (itIndex > 0)) {
      setIntegrationTime(intTimes[--itIndex]);
      ALS = readALS(true);
    }
  }
  return computeLux(ALS, useCorrection);
}
