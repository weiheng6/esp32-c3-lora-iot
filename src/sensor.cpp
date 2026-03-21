#include "sensor.h"
#include <Wire.h>

SensorManager sensorManager;

SensorManager::SensorManager() : initialized(false) {}

bool SensorManager::begin() {
  Wire.begin(SHT31_I2C_SDA, SHT31_I2C_SCL);
  initialized = sht31.begin(SHT31_I2C_ADDRESS);
  
  if (initialized) {
    Serial.println("✅ SHT31 传感器初始化成功！");
  } else {
    Serial.println("⚠️  未检测到 SHT31 传感器，继续运行...");
  }
  
  return initialized;
}

bool SensorManager::readTemperature(float &temperature) {
  if (!initialized) return false;
  
  temperature = sht31.readTemperature();
  return !isnan(temperature);
}

bool SensorManager::readHumidity(float &humidity) {
  if (!initialized) return false;
  
  humidity = sht31.readHumidity();
  return !isnan(humidity);
}

bool SensorManager::readBoth(float &temperature, float &humidity) {
  if (!initialized) return false;
  
  temperature = sht31.readTemperature();
  humidity = sht31.readHumidity();
  return !isnan(temperature) && !isnan(humidity);
}
