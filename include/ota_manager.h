#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>

// OTA 管理器类
class OTAManager {
private:
  bool isEnabled = false;
  String hostname;
  
public:
  OTAManager();
  void begin(const char* deviceName = "esp32-c3");
  void handle();
  bool isRunning() const { return isEnabled; }
};

extern OTAManager otaManager;

#endif
