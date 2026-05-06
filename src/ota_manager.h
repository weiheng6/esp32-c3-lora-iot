#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>

// OTA状态枚举（使用不同的名称避免与ArduinoOTA冲突）
typedef enum {
  OTA_STATUS_IDLE,
  OTA_STATUS_UPDATING,
  OTA_STATUS_COMPLETED,
  OTA_STATUS_FAILED
} OTAManagerStatus;

class OTAManager {
public:
  OTAManager();
  void begin(const char* deviceName);
  void handle();
  bool updateFromHTTP(const String& url);
  OTAManagerStatus getStatus();
  int getProgress();
  bool isEnabled() { return enabled; }
  
  // 添加公共setter方法
  void setStatus(OTAManagerStatus status) { otaStatus = status; }
  void setProgress(int progress) { otaProgress = progress; }

private:
  String hostname;
  bool enabled;
  OTAManagerStatus otaStatus;
  int otaProgress;
};

extern OTAManager otaManager;

#endif // OTA_MANAGER_H