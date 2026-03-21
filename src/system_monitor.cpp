#include "system_monitor.h"
#include "mqtt_manager.h"
#include <Preferences.h>

SystemMonitor systemMonitor;
extern Preferences preferences;

SystemMonitor::SystemMonitor() : lastStatsTime(0), lastMemoryReportTime(0) {}

void SystemMonitor::printStats() {
  #if DEBUG_MODE
  unsigned long uptimeSeconds = millis() / 1000;
  unsigned long hours = uptimeSeconds / 3600;
  unsigned long minutes = (uptimeSeconds % 3600) / 60;
  unsigned long seconds = uptimeSeconds % 60;
  
  unsigned long freeHeap = ESP.getFreeHeap();
  unsigned long totalHeap = ESP.getHeapSize();
  unsigned long maxAllocHeap = ESP.getMaxAllocHeap();
  
  int rssi = WiFi.RSSI();
  
  Serial.println("📊 系统状态信息：");
  Serial.printf("   运行时间：%02lu:%02lu:%02lu\n", hours, minutes, seconds);
  Serial.printf("   内存使用：可用 %lu 字节，总计 %lu 字节，最大分配 %lu 字节\n", 
               freeHeap, totalHeap, maxAllocHeap);
  Serial.printf("   WiFi 信号：%d dBm\n", rssi);
  Serial.println("--------------------------");
  #endif
}

void SystemMonitor::reportMemory() {
  if (!mqttManager.isConnected()) {
    return;
  }
  
  unsigned long freeHeap = ESP.getFreeHeap();
  unsigned long totalHeap = ESP.getHeapSize();
  unsigned long usedHeap = totalHeap - freeHeap;
  unsigned long minFreeHeap = ESP.getMinFreeHeap();
  
  char jsonBuffer[256];
  snprintf(jsonBuffer, sizeof(jsonBuffer),
           "{\"free_heap\":%lu,\"used_heap\":%lu,\"total_heap\":%lu,\"min_free_heap\":%lu}",
           freeHeap, usedHeap, totalHeap, minFreeHeap);
  
  if (mqttManager.publish(mqttManager.getMemoryTopic(), jsonBuffer)) {
    Serial.printf("📤 内存数据已发布到主题：%s\n", mqttManager.getMemoryTopic());
  }
}

void SystemMonitor::checkMemory() {
  unsigned long freeHeap = ESP.getFreeHeap();
  
  if (freeHeap < MEM_THRESHOLD) {
    Serial.printf("⚠️  内存不足，当前可用：%lu 字节，阈值：%d 字节\n", freeHeap, MEM_THRESHOLD);
    Serial.println("🔄 系统将在 5 秒后自动重启...");
    
    preferences.begin("sensor_config", false);
    preferences.putULong("acquisition_interval", 1000);
    preferences.end();
    
    if (mqttManager.isConnected()) {
      mqttManager.publish(mqttManager.getWillTopic(), "offline", MQTT_WILL_RETAIN);
    }
    
    delay(5000);
    ESP.restart();
  }
}

bool SystemMonitor::isMemoryLow() {
  return ESP.getFreeHeap() < MEM_THRESHOLD;
}
