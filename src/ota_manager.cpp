#include "ota_manager.h"
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <Update.h>

// 全局实例
OTAManager otaManager;

OTAManager::OTAManager() : enabled(false), otaStatus(OTA_STATUS_IDLE), otaProgress(0) {}

void OTAManager::begin(const char* deviceName) {
  hostname = String(deviceName);
  
  // 配置 ArduinoOTA
  ArduinoOTA.setHostname(hostname.c_str());
  
  // 添加密码保护，防止未授权升级
  ArduinoOTA.setPassword("OTA@2024");  // 安全增强
  
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
    enabled = true;
    Serial.printf("✅ OTA 已启动 - 主机名: %s (可通过 Arduino IDE 上传固件)\n", hostname.c_str());
  } catch (const std::exception& e) {
    Serial.printf("❌ OTA 启动失败: %s\n", e.what());
    enabled = false;
  }
}

void OTAManager::handle() {
  if (enabled) {
    ArduinoOTA.handle();
  }
}

// 通过HTTP URL升级固件（支持从阿里云OSS下载）
bool OTAManager::updateFromHTTP(const String& url) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ [OTA] WiFi未连接，无法进行HTTP升级");
    return false;
  }

  HTTPClient http;
  
  Serial.printf("🔄 [OTA] 开始从URL升级: %s\n", url.c_str());
  
  // 更新状态为升级中
  otaStatus = OTA_STATUS_UPDATING;
  otaProgress = 0;
  
  if (!http.begin(url)) {
    Serial.println("❌ [OTA] HTTP连接失败");
    otaStatus = OTA_STATUS_FAILED;
    return false;
  }
  
  int httpCode = http.GET();
  
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("❌ [OTA] HTTP请求失败，错误码: %d\n", httpCode);
    http.end();
    otaStatus = OTA_STATUS_FAILED;
    return false;
  }
  
  // 获取固件大小
  int totalSize = http.getSize();
  Serial.printf("📦 [OTA] 固件大小: %d bytes\n", totalSize);
  
  // 检查是否有足够的空间
  if (!Update.begin(totalSize)) {
    Serial.println("❌ [OTA] Update.begin()失败，可能空间不足");
    http.end();
    otaStatus = OTA_STATUS_FAILED;
    return false;
  }
  
  // 设置回调函数
  int lastReportedProgress = -1;
  Update.onProgress([this, &lastReportedProgress](unsigned int progress, unsigned int total) {
    this->otaProgress = (progress * 100) / total;
    
    // 串口日志（每秒一次）
    static unsigned long lastPrintTime = 0;
    if (millis() - lastPrintTime > 1000) {
      lastPrintTime = millis();
      Serial.printf("🔄 [OTA] 下载进度: %d%% (%u/%u)\n", this->otaProgress, progress, total);
    }
  });
  
  // 写入固件数据
  WiFiClient* stream = http.getStreamPtr();
  size_t written = Update.writeStream(*stream);
  
  if (written != totalSize) {
    Serial.printf("❌ [OTA] 写入失败，已写入: %u / 期望: %d\n", written, totalSize);
    Update.end();
    http.end();
    otaStatus = OTA_STATUS_FAILED;
    return false;
  }
  
  // 结束更新并验证
  if (!Update.end(true)) {
    Serial.printf("❌ [OTA] 更新结束失败，错误: %d\n", Update.getError());
    http.end();
    otaStatus = OTA_STATUS_FAILED;
    return false;
  }
  
  http.end();
  
  Serial.println("\n✅ [OTA] HTTP升级完成！设备将重启...");
  delay(1000);
  ESP.restart();
  
  return true;
}

// 获取当前OTA状态
OTAManagerStatus OTAManager::getStatus() {
  return otaStatus;
}

// 获取当前升级进度
int OTAManager::getProgress() {
  return otaProgress;
}
