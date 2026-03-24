#ifndef ERROR_RECOVERY_H
#define ERROR_RECOVERY_H

#include <Arduino.h>
#include <Preferences.h>

// 故障恢复管理器类
class ErrorRecoveryManager {
private:
  Preferences errorLog;
  uint32_t rebootCount = 0;
  uint32_t i2cFailureCount = 0;
  uint32_t mqttFailureCount = 0;
  unsigned long lastCriticalErrorTime = 0;
  
public:
  void begin();
  void recordReboot();
  void recordI2CFailure();
  void recordMQTTFailure();
  void checkAndRecover();
  void printStats();
  uint32_t getRebootCount() { return rebootCount; }
};

extern ErrorRecoveryManager errorRecovery;

#endif
