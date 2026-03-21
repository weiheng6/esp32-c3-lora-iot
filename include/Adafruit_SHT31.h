#ifndef ADAFRUIT_SHT31_H
#define ADAFRUIT_SHT31_H

#include <Arduino.h>
#include <Wire.h>

// 简化版 SHT31 驱动（用于 PlatformIO 环境）
class Adafruit_SHT31 {
private:
  TwoWire* _wire;
  uint8_t _addr;
  float _lastTemp;
  float _lastHumi;

  bool readData();

public:
  Adafruit_SHT31();
  bool begin(uint8_t addr = 0x44);
  float readTemperature();
  float readHumidity();
};

#endif
