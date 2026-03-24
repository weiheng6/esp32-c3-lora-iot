#include "ota_manager.h"
#include <ArduinoOTA.h>

// 全局实例
OTAManager otaManager;

OTAManager::OTAManager() {}

void OTAManager::begin(const char* deviceName) {
  hostname = String(deviceName);
  
  // 配置 ArduinoOTA
  ArduinoOTA.setHostname(hostname.c_str());
  
  ArduinoOTA.onStart([]() {
    Serial.println("\n🔄 [OTA] 开始固件升级...");
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\n✅ [OTA] 固件升级完成！");
    Serial.println("🔄 [OTA] 设备重启中...");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static unsigned long lastPrintTime = 0;
    if (millis() - lastPrintTime > 500) {  // 每 500ms 打印一次进度
      lastPrintTime = millis();
      int percentage = (progress * 100) / total;
      Serial.printf("🔄 [OTA] 升级进度: %u/%u (%d%%)\n", progress, total, percentage);
    }
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("❌ [OTA] 错误代码: %u\n", error);
    
    if (error == OTA_AUTH_ERROR) {
      Serial.println("   → 认证失败");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("   → 开始失败");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("   → 连接失败");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("   → 接收失败");
    } else if (error == OTA_END_ERROR) {
      Serial.println("   → 结束失败");
    }
  });
  
  try {
    ArduinoOTA.begin();
    isEnabled = true;
    Serial.printf("✅ OTA 已启动 - 主机名: %s (可通过 Arduino IDE 上传固件)\n", hostname.c_str());
  } catch (const std::exception& e) {
    Serial.printf("❌ OTA 启动失败: %s\n", e.what());
    isEnabled = false;
  }
}

void OTAManager::handle() {
  if (isEnabled) {
    ArduinoOTA.handle();
  }
}
