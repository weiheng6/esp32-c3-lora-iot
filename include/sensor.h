#ifndef SENSOR_H
#define SENSOR_H

#include <Adafruit_SHT31.h>
#include "config.h"

class SensorManager {
private:
  Adafruit_SHT31 sht31;
  bool initialized;

public:
  SensorManager();
  bool begin();
  bool readTemperature(float &temperature);
  bool readHumidity(float &humidity);
  bool readBoth(float &temperature, float &humidity);
  bool isInitialized() const { return initialized; }
};

extern SensorManager sensorManager;

#endif // SENSOR_H
