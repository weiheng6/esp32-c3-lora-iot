#include "mqtt_manager.h"
#include "config.h"
#include <functional>
#include <ArduinoJson.h>

MQTTManager mqttManager;

MQTTManager::MQTTManager() : mqttClient(wifiClient), firstConnection(true) {
  memset(clientId, 0, sizeof(clientId));
  memset(deviceId, 0, sizeof(deviceId));
  memset(topicTemp, 0, sizeof(topicTemp));
  memset(topicHum, 0, sizeof(topicHum));
  memset(topicCmd, 0, sizeof(topicCmd));
  memset(topicResp, 0, sizeof(topicResp));
  memset(topicWill, 0, sizeof(topicWill));
  memset(topicMemory, 0, sizeof(topicMemory));
  memset(topicRestart, 0, sizeof(topicRestart));
  memset(topicMgmtInfo, 0, sizeof(topicMgmtInfo));
}

void MQTTManager::begin() {
  // 获取 MAC 地址生成设备 ID
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(deviceId, sizeof(deviceId), "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  Serial.printf("✅ 已生成设备 ID：%s\n", deviceId);
  
  // 生成客户端 ID
  snprintf(clientId, sizeof(clientId), "ESP32_%s", deviceId);
  Serial.printf("✅ 已生成客户端 ID：%s\n", clientId);
  
  // 生成 MQTT 主题
  generateTopics();
  
  // 配置 MQTT 服务器
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  
  Serial.println("✅ MQTT 管理器初始化成功");
}

void MQTTManager::generateTopics() {
  snprintf(topicTemp, sizeof(topicTemp), "esp32/%s/temperature", deviceId);
  snprintf(topicHum, sizeof(topicHum), "esp32/%s/humidity", deviceId);
  snprintf(topicCmd, sizeof(topicCmd), "esp32/%s/command", deviceId);
  snprintf(topicResp, sizeof(topicResp), "esp32/%s/response", deviceId);
  snprintf(topicWill, sizeof(topicWill), "esp32/%s/status", deviceId);
  snprintf(topicMemory, sizeof(topicMemory), "esp32/%s/system/memory", deviceId);
  snprintf(topicRestart, sizeof(topicRestart), "esp32/%s/system/restart", deviceId);
  snprintf(topicMgmtInfo, sizeof(topicMgmtInfo), "esp32/%s/system/management_info", deviceId);
  
  Serial.println("✅ MQTT 主题已生成：");
  Serial.printf("   温度：%s\n", topicTemp);
  Serial.printf("   湿度：%s\n", topicHum);
  Serial.printf("   命令：%s\n", topicCmd);
  Serial.printf("   响应：%s\n", topicResp);
  Serial.printf("   管理信息：%s\n", topicMgmtInfo);
}

void MQTTManager::connect() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  if (!mqttClient.connected()) {
    Serial.print("🔄 正在连接 MQTT 服务器...");
    
    if (mqttClient.connect(clientId, MQTT_USER, MQTT_PASSWORD,
                           topicWill, MQTT_WILL_QOS, MQTT_WILL_RETAIN,
                           MQTT_WILL_MSG, true)) {
      Serial.println("✅");
      Serial.println("📡 MQTT 服务器连接成功！");
      
      publish(topicWill, "online", MQTT_WILL_RETAIN);
      
      if (firstConnection) {
        publish(topicRestart, "1", true);
        firstConnection = false;
      }
      
      mqttClient.subscribe(topicCmd);
      mqttClient.subscribe("esp32/+/command");
    } else {
      Serial.print("❌ 连接失败，错误代码：");
      Serial.println(mqttClient.state());
    }
  }
}

void MQTTManager::loop() {
  if (mqttClient.connected()) {
    unsigned long mqttLoopStart = millis();
    mqttClient.loop();
    unsigned long mqttLoopDuration = millis() - mqttLoopStart;
    
    // 监测 MQTT loop 耗时
    if (mqttLoopDuration > 100) {
      Serial.printf("⚠️  MQTT loop 耗时过长：%lu ms\n", mqttLoopDuration);
    }
  }
}

bool MQTTManager::isConnected() const {
  return (const_cast<PubSubClient&>(mqttClient)).connected();
}

bool MQTTManager::publish(const char* topic, const char* payload, bool retain) {
  // 1. 检查 MQTT 连接
  if (!mqttClient.connected()) {
    return false;
  }
  
  // 2. 检查 WiFi 信号强度（-70 dBm 以下不发送）
  int rssi = WiFi.RSSI();
  if (rssi < -70) {
    Serial.printf("⚠️  WiFi 信号弱（%d dBm），延迟 MQTT 发布\n", rssi);
    return false;  // 返回 false，上层会在下个周期重试
  }
  
  // 3. 检查 MQTT 缓冲区大小（PubSubClient 默认缓冲 256 字节）
  // 如果 payload 接近缓冲区大小，延迟发送
  size_t payloadLen = strlen(payload);
  if (payloadLen > 200) {
    Serial.printf("⚠️  MQTT payload 过大（%lu 字节），可能导致缓冲溢出\n", payloadLen);
    return false;
  }
  
  // 4. 尝试发布
  unsigned long publishStart = millis();
  bool result = mqttClient.publish(topic, payload, retain);
  unsigned long publishDuration = millis() - publishStart;
  
  // 5. 监测发布时间
  if (publishDuration > 5) {
    Serial.printf("⚠️  MQTT 发布耗时 %lu ms\n", publishDuration);
  }
  
  return result;
}

void MQTTManager::setCallback(std::function<void(char*, byte*, unsigned int)> callback) {
  mqttClient.setCallback(callback);
}

void MQTTManager::publishManagementInfo(const char* ipAddress) {
  if (!mqttClient.connected()) {
    Serial.printf("⚠️  MQTT 未连接，无法推送管理信息\n");
    return;
  }

  // 创建 JSON 文档
  StaticJsonDocument<200> doc;
  doc["ip_address"] = ipAddress;
  doc["management_url"] = String("http://") + String(ipAddress);
  doc["device_id"] = deviceId;
  doc["timestamp"] = millis();
  
  // 如果能获取到 WiFi 信息，添加到 JSON
  if (WiFi.isConnected()) {
    doc["ssid"] = WiFi.SSID();
    doc["signal_strength"] = WiFi.RSSI();
  }
  
  // 序列化 JSON 并发布
  String payload;
  serializeJson(doc, payload);
  
  bool result = publish(topicMgmtInfo, payload.c_str(), true);  // 使用 retain 标志
  if (result) {
    Serial.printf("✅ 已推送管理信息到 MQTT: %s\n", topicMgmtInfo);
    Serial.printf("   数据: %s\n", payload.c_str());
  } else {
    Serial.printf("❌ 推送管理信息失败\n");
  }
}
