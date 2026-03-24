#include "error_recovery.h"
#include <Wire.h>

// 全局实例
ErrorRecoveryManager errorRecovery;

void ErrorRecoveryManager::begin() {
  errorLog.begin("error_log", false);  // 读写模式
  
  // 读取启动次数
  rebootCount = errorLog.getULong("reboot_count", 0);
  rebootCount++;
  errorLog.putULong("reboot_count", rebootCount);
  
  // 读取故障计数（仅用于诊断，每次启动重置）
  i2cFailureCount = 0;
  mqttFailureCount = 0;
  
  Serial.printf("📊 [ErrorRecovery] 设备启动次数: %lu\n", rebootCount);
  
  // 如果启动次数异常多，可能陷入重启循环
  if (rebootCount > 10) {
    Serial.println("⚠️⚠️ [ErrorRecovery] 检测到可能的重启循环！");
    Serial.println("      原因可能：");
    Serial.println("      1. 固件 bug");
    Serial.println("      2. 传感器硬件故障");
    Serial.println("      3. 内存不足");
    Serial.println("      建议：检查串口日志，考虑重新编译固件");
  }
  
  errorLog.end();
}

void ErrorRecoveryManager::recordReboot() {
  errorLog.begin("error_log", false);
  errorLog.putULong("last_reboot_time", millis());
  errorLog.end();
}

void ErrorRecoveryManager::recordI2CFailure() {
  i2cFailureCount++;
  if (i2cFailureCount % 10 == 0) {  // 每 10 次失败记录一次
    Serial.printf("⚠️ [ErrorRecovery] I2C 故障次数: %lu\n", i2cFailureCount);
  }
}

void ErrorRecoveryManager::recordMQTTFailure() {
  mqttFailureCount++;
  if (mqttFailureCount % 10 == 0) {
    Serial.printf("⚠️ [ErrorRecovery] MQTT 故障次数: %lu\n", mqttFailureCount);
  }
}

void ErrorRecoveryManager::checkAndRecover() {
  unsigned long currentTime = millis();
  
  // 检查故障恢复触发条件
  static unsigned long lastRecoveryCheck = 0;
  if (currentTime - lastRecoveryCheck < 30000) {  // 每 30 秒检查一次
    return;
  }
  lastRecoveryCheck = currentTime;
  
  // 条件 1: 大量 I2C 故障，重启 I2C 总线
  if (i2cFailureCount > 50) {
    Serial.println("🔄 [ErrorRecovery] I2C 故障过多，执行总线复位...");
    Wire.end();
    delay(10);
    Wire.begin(8, 9);  // SDA=GPIO8, SCL=GPIO9 (Adafruit QtPy ESP32-C3)
    Wire.setClock(400000);
    delay(10);
    i2cFailureCount = 0;  // 重置计数器
    Serial.println("✅ [ErrorRecovery] I2C 总线已复位");
  }
  
  // 条件 2: 内存不足，记录警告
  uint32_t freeMemory = ESP.getFreeHeap();
  const uint32_t CRITICAL_MEMORY_THRESHOLD = 20000;  // 20KB
  
  if (freeMemory < CRITICAL_MEMORY_THRESHOLD) {
    Serial.printf("❌ [ErrorRecovery] 内存严重不足: %lu 字节 (阈值: %lu)\n", 
                  freeMemory, CRITICAL_MEMORY_THRESHOLD);
    
    if (currentTime - lastCriticalErrorTime > 60000) {  // 60 秒只重启一次
      lastCriticalErrorTime = currentTime;
      Serial.println("🔄 [ErrorRecovery] 执行内存恢复重启...");
      delay(1000);
      ESP.restart();
    }
  }
  
  // 条件 3: MQTT 连接失败过多，考虑重新初始化
  if (mqttFailureCount > 100) {
    Serial.println("🔄 [ErrorRecovery] MQTT 故障过多，考虑重新连接...");
    // 这里可以添加 MQTT 重新连接的逻辑
    mqttFailureCount = 0;
  }
}

void ErrorRecoveryManager::printStats() {
  Serial.printf("📊 [ErrorRecovery] 诊断统计:\n");
  Serial.printf("   启动次数: %lu\n", rebootCount);
  Serial.printf("   当前会话 I2C 故障: %lu\n", i2cFailureCount);
  Serial.printf("   当前会话 MQTT 故障: %lu\n", mqttFailureCount);
  Serial.printf("   剩余内存: %lu 字节 (%u%% 使用率)\n", 
                ESP.getFreeHeap(), 
                (100 * (ESP.getHeapSize() - ESP.getFreeHeap())) / ESP.getHeapSize());
}
