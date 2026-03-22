#include "sensor.h"
#include <Wire.h>

SensorManager sensorManager;

// 缓存上次成功读取的数据
static float lastTemp = 20.0f;
static float lastHumidity = 50.0f;
static unsigned long lastSuccessfulRead = 0;
static uint32_t i2cReadAttempts = 0;
static uint32_t i2cReadSuccesses = 0;

SensorManager::SensorManager() : initialized(false) {}

bool SensorManager::begin() {
  Wire.begin(SHT31_I2C_SDA, SHT31_I2C_SCL);
  Wire.setClock(400000);  // 设置 I2C 时钟频率为 400kHz（标准快速模式）
  
  // 尝试初始化传感器
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
  
  unsigned long readStart = millis();
  temperature = sht31.readTemperature();
  unsigned long readDuration = millis() - readStart;
  
  // 如果读取超过 5ms，记录警告（可能是 I2C 问题）
  if (readDuration > 5) {
    Serial.printf("⚠️  温度读取耗时 %lu ms\n", readDuration);
  }
  
  return !isnan(temperature);
}

bool SensorManager::readHumidity(float &humidity) {
  if (!initialized) return false;
  
  unsigned long readStart = millis();
  humidity = sht31.readHumidity();
  unsigned long readDuration = millis() - readStart;
  
  // 如果读取超过 5ms，记录警告（可能是 I2C 问题）
  if (readDuration > 5) {
    Serial.printf("⚠️  湿度读取耗时 %lu ms\n", readDuration);
  }
  
  return !isnan(humidity);
}

bool SensorManager::readBoth(float &temperature, float &humidity) {
  if (!initialized) {
    // 使用缓存值
    temperature = lastTemp;
    humidity = lastHumidity;
    return false;
  }
  
  i2cReadAttempts++;
  
  // ⚠️ 超时保护：最多等待 50ms（SHT31 实际响应时间约 40ms）
  unsigned long readStart = millis();
  unsigned long timeoutMs = 50;  // 50ms 超时
  
  // 尝试读取
  temperature = sht31.readTemperature();
  unsigned long tempReadTime = millis() - readStart;
  
  // 检查是否超时
  if (tempReadTime > timeoutMs) {
    Serial.printf("❌ I2C 读取超时（温度耗时 %lu ms > %lu ms），使用缓存值\n", 
                  tempReadTime, timeoutMs);
    temperature = lastTemp;
    humidity = lastHumidity;
    return false;
  }
  
  humidity = sht31.readHumidity();
  unsigned long readDuration = millis() - readStart;
  
  // ⚠️ 如果总耗时超过 50ms，立即告警（可能有 I2C 总线问题）
  if (readDuration > 50) {
    Serial.printf("⚠️⚠️ I2C 总耗时 %lu ms（超过 50ms 阈值 - 可能有问题）\n", readDuration);
  } else if (readDuration > 45) {
    Serial.printf("⚠️  I2C 读取耗时较长：%lu ms\n", readDuration);
  }
  
  // 检查数据有效性
  bool success = !isnan(temperature) && !isnan(humidity);
  if (!success) {
    Serial.println("❌ SHT31 读取失败（NaN），使用缓存值");
    temperature = lastTemp;
    humidity = lastHumidity;
    return false;
  }
  
  // 成功，更新缓存和统计
  lastTemp = temperature;
  lastHumidity = humidity;
  lastSuccessfulRead = millis();
  i2cReadSuccesses++;
  
  return true;
}
