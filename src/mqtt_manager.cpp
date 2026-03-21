#include "mqtt_manager.h"
#include "config.h"
#include <functional>

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
  
  Serial.println("✅ MQTT 主题已生成：");
  Serial.printf("   温度：%s\n", topicTemp);
  Serial.printf("   湿度：%s\n", topicHum);
  Serial.printf("   命令：%s\n", topicCmd);
  Serial.printf("   响应：%s\n", topicResp);
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
    mqttClient.loop();
  }
}

bool MQTTManager::isConnected() const {
  return (const_cast<PubSubClient&>(mqttClient)).connected();
}

bool MQTTManager::publish(const char* topic, const char* payload, bool retain) {
  if (!mqttClient.connected()) {
    return false;
  }
  return mqttClient.publish(topic, payload, retain);
}

void MQTTManager::setCallback(std::function<void(char*, byte*, unsigned int)> callback) {
  mqttClient.setCallback(callback);
}
