#include "Adafruit_SHT31.h"

Adafruit_SHT31::Adafruit_SHT31() : _addr(0x44), _lastTemp(0), _lastHumi(0) {}

bool Adafruit_SHT31::begin(uint8_t addr) {
  _addr = addr;
  _wire = &Wire;
  
  // 尝试读取一次来检查设备是否存在
  _wire->beginTransmission(_addr);
  return (_wire->endTransmission() == 0);
}

bool Adafruit_SHT31::readData() {
  // 命令：0x2C06 （High precision, clock stretching enabled）
  _wire->beginTransmission(_addr);
  _wire->write(0x2C);
  _wire->write(0x06);
  if (_wire->endTransmission() != 0) {
    return false;
  }
  
  delay(20); // 等待测量完成
  
  // 读取 6 个字节：2 字节温度 + 1 字节 CRC + 2 字节湿度 + 1 字节 CRC
  _wire->requestFrom(_addr, (uint8_t)6);
  if (_wire->available() != 6) {
    return false;
  }
  
  uint16_t rawTemp = _wire->read() << 8;
  rawTemp |= _wire->read();
  uint8_t crcTemp = _wire->read();
  
  uint16_t rawHumi = _wire->read() << 8;
  rawHumi |= _wire->read();
  uint8_t crcHumi = _wire->read();
  
  // 计算实际温度和湿度
  _lastTemp = -45.0 + (175.0 * rawTemp / 65535.0);
  _lastHumi = 100.0 * rawHumi / 65535.0;
  
  return true;
}

float Adafruit_SHT31::readTemperature() {
  if (readData()) {
    return _lastTemp;
  }
  return NAN;
}

float Adafruit_SHT31::readHumidity() {
  if (readData()) {
    return _lastHumi;
  }
  return NAN;
}
