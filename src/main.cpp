#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "config.h"
#include "sensor.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "lora_manager.h"
#include "relay_control.h"
#include "condition_control.h"
#include "web_server.h"
#include "system_monitor.h"

// ==================== 全局常量定义 ====================
const char* MQTT_SERVER = "iot.kebaidata.com";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "kbuser";
const char* MQTT_PASSWORD = "kbuserspasswd";
const char* MQTT_WILL_MSG = "offline";
const int MQTT_WILL_QOS = 1;
const bool MQTT_WILL_RETAIN = true;
const char* AP_SSID = "Scott_Device";
const char* AP_PASSWORD = "88888888";

// ==================== 全局状态变量 ====================
DeviceStatus deviceStatus = STATUS_OFFLINE;
bool hasNetworkConnection = false;
bool needReport = false;
String connectedLoRaNodeId = "";

unsigned long lastAcquisitionTime = 0;
unsigned long lastMqttReportTime = 0;
unsigned long lastWifiCheckTime = 0;
unsigned long lastNodeDiscoveryTime = 0;
unsigned long lastHeartbeatTime = 0;
unsigned long lastMqttConnectAttempt = 0;

// MQTT 重连机制配置
#define MQTT_RECONNECT_INTERVAL 5000  // 5秒重连一次
#define MQTT_RECONNECT_MAX_INTERVAL 60000  // 最多等60秒

// ==================== MQTT 消息回调 ====================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (length > 500) {
    Serial.println("❌ 指令长度超过限制");
    return;
  }
  
  Serial.print("📥 收到下行指令：");
  
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  Serial.println(message);
  
  // 解析 MQTT 主题提取目标设备 ID
  String topicStr = String(topic);
  int firstSlash = topicStr.indexOf('/');
  int secondSlash = topicStr.indexOf('/', firstSlash + 1);
  String targetDeviceId = topicStr.substring(firstSlash + 1, secondSlash);
  
  // 检查指令是否发给本设备
  if (targetDeviceId != String(mqttManager.getDeviceId())) {
    Serial.printf("🔄 指令目标设备：%s，当前设备：%s，转发为 LoRa 命令...\n",
                  targetDeviceId.c_str(), mqttManager.getDeviceId());
    
    String loraCmd = String(LORA_MSG_TYPE_CMD) + "," + targetDeviceId + "," + String(message);
    loraManager.sendMessage(loraCmd, LORA_PRIORITY_HIGH);
    return;
  }
  
  // 解析 JSON 指令
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.print("❌ JSON 解析失败：");
    Serial.println(error.c_str());
    return;
  }
  
  // 处理采集间隔调整
  if (doc.containsKey("acquisition_interval")) {
    int interval = doc["acquisition_interval"];
    if (interval > 0) {
      Serial.printf("⏱️ 采集间隔设置为：%d 秒\n", interval);
      mqttManager.publish(mqttManager.getRespTopic(), "{\"status\":\"ok\"}");
    }
  }
  
  // 处理继电器控制
  if (doc.containsKey("relay")) {
    int relayCmd = doc["relay"];
    if (relayCmd == 1) {
      relayControl.turnOn();
      Serial.println("🔌 继电器控制：开启");
      mqttManager.publish(mqttManager.getRespTopic(), "{\"status\":\"ok\",\"relay\":1}");
    } else if (relayCmd == 0) {
      relayControl.turnOff();
      Serial.println("🔌 继电器控制：关闭");
      mqttManager.publish(mqttManager.getRespTopic(), "{\"status\":\"ok\",\"relay\":0}");
    }
  }
  
  // 处理主动查询
  if (doc.containsKey("query") && doc["query"] == true) {
    needReport = true;
    Serial.println("📢 收到主动查询指令");
    mqttManager.publish(mqttManager.getRespTopic(), "{\"status\":\"ok\",\"message\":\"Query received\"}");
  }
  
  // 处理条件控制配置
  if (doc.containsKey("condition")) {
    JsonObject conditionObj = doc["condition"].as<JsonObject>();
    
    if (conditionObj.containsKey("enabled")) {
      conditionControl.setEnabled(conditionObj["enabled"]);
      Serial.printf("🔄 条件控制%s\n", conditionControl.isEnabled() ? "已启用" : "已禁用");
    }
    
    mqttManager.publish(mqttManager.getRespTopic(), conditionControl.toJSON().c_str());
  }
  
  // 处理 WiFi 重置
  if (doc.containsKey("reset_wifi") && doc["reset_wifi"] == true) {
    Serial.println("🔄 收到重置 WiFi 指令");
    wifiManager.resetConfig();
    mqttManager.publish(mqttManager.getRespTopic(), "{\"status\":\"ok\",\"message\":\"WiFi reset\"}");
    delay(2000);
    ESP.restart();
  }
}

// ==================== 检查网络连接 ====================
void checkNetworkConnection() {
  bool wifiConnected = (WiFi.status() == WL_CONNECTED);
  bool mqttConnected = mqttManager.isConnected();
  
  hasNetworkConnection = wifiConnected && mqttConnected;
  
  if (hasNetworkConnection) {
    deviceStatus = STATUS_ONLINE_WITH_NETWORK;
  } else if (wifiConnected) {
    deviceStatus = STATUS_ONLINE;
  } else {
    deviceStatus = STATUS_OFFLINE;
  }
}

// ==================== 采集传感器数据 ====================
void acquireSensorData() {
  float temperature, humidity;
  
  if (sensorManager.readBoth(temperature, humidity)) {
    Serial.print("🔍 本地采集 - 温度：");
    Serial.print(temperature);
    Serial.print(" °C\t湿度：");
    Serial.print(humidity);
    Serial.println(" %RH");
    
    // 执行条件控制
    if (conditionControl.checkConditions(temperature, humidity)) {
      relayControl.turnOn();
    } else {
      relayControl.turnOff();
    }
  }
}

// ==================== 上报传感器数据到 MQTT ====================
void reportSensorDataToMqtt() {
  float temperature, humidity;
  
  if (sensorManager.readBoth(temperature, humidity)) {
    Serial.print("📤 MQTT 上报 - 温度：");
    Serial.print(temperature);
    Serial.print(" °C\t湿度：");
    Serial.print(humidity);
    Serial.println(" %RH");
    
    char tempStr[10];
    char humStr[10];
    dtostrf(temperature, 4, 2, tempStr);
    dtostrf(humidity, 4, 2, humStr);
    
    // 检查发送结果
    bool tempPublished = mqttManager.publish(mqttManager.getTempTopic(), tempStr);
    bool humPublished = mqttManager.publish(mqttManager.getHumTopic(), humStr);
    
    if (!tempPublished || !humPublished) {
      Serial.println("❌ MQTT 发送失败！连接可能已断开，即将重连...");
    }
  }
}

// ==================== Setup 函数 ====================
void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║   ESP32-C3 条件控制器启动              ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("");
  
  // 初始化各模块
  Serial.println("🔄 初始化模块...");
  
  // 1. 传感器
  sensorManager.begin();
  
  // 2. WiFi
  wifiManager.begin();
  
  // 3. MQTT
  mqttManager.begin();
  mqttManager.setCallback(mqttCallback);
  
  // 4. LoRa
  loraManager.begin();
  
  // 5. 继电器
  relayControl.begin();
  
  // 6. 条件控制
  conditionControl.begin();
  
  // 7. 系统监控
  Serial.println("✅ 所有模块初始化完成！");
  Serial.println("════════════════════════════════════════");
}

// ==================== Loop 函数 ====================
void loop() {
  unsigned long currentTime = millis();
  
  // 检查网络连接
  checkNetworkConnection();
  
  // 定期检查 WiFi
  if (currentTime - lastWifiCheckTime >= WIFI_CHECK_INTERVAL) {
    lastWifiCheckTime = currentTime;
    wifiManager.connect();
  }
  
  // 连接 MQTT（带重连机制）
  if (!mqttManager.isConnected() && WiFi.status() == WL_CONNECTED) {
    if (currentTime - lastMqttConnectAttempt >= MQTT_RECONNECT_INTERVAL) {
      lastMqttConnectAttempt = currentTime;
      Serial.println("🔄 尝试重新连接 MQTT 服务器...");
      mqttManager.connect();
    }
  } else if (mqttManager.isConnected()) {
    // 连接成功，重置重连时间（这样断开后立即尝试重连）
    lastMqttConnectAttempt = 0;
  }
  
  // MQTT 循环
  mqttManager.loop();
  
  // 处理 HTTP 请求
  webServerManager.handleClient();
  
  // 处理 LoRa 消息
  String loraMessage = loraManager.receiveMessage();
  if (loraMessage.length() > 0) {
    loraManager.parseMessage(loraMessage);
  }
  
  // 处理 LoRa 队列
  loraManager.processQueue();
  
  // 定期采集传感器数据
  if (currentTime - lastAcquisitionTime >= DEFAULT_ACQUISITION_INTERVAL) {
    lastAcquisitionTime = currentTime;
    acquireSensorData();
  }
  
  // 定期上报数据到 MQTT
  if (currentTime - lastMqttReportTime >= DEFAULT_MQTT_REPORT_INTERVAL || needReport) {
    lastMqttReportTime = currentTime;
    
    if (hasNetworkConnection) {
      reportSensorDataToMqtt();
    } else {
      Serial.println("⚠️  网络连接不可用，跳过本次上报");
    }
    
    needReport = false;
  }
  
  // 定期输出系统状态和内存监控
  if (currentTime - lastNodeDiscoveryTime >= SYSTEM_STATS_INTERVAL) {
    lastNodeDiscoveryTime = currentTime;
    systemMonitor.printStats();
    systemMonitor.checkMemory();
    
    // 输出内存状态
    unsigned long freeHeap = ESP.getFreeHeap();
    unsigned long totalHeap = ESP.getHeapSize();
    float usagePercent = 100.0 * (totalHeap - freeHeap) / totalHeap;
    Serial.printf("💾 内存状态 - 可用：%lu 字节，使用率：%.1f%%，警告线：%d 字节\n", 
                  freeHeap, usagePercent, MEM_THRESHOLD);
  }
  
  // LoRa 节点发现
  if (currentTime - lastHeartbeatTime >= LORA_NODE_DISCOVERY_INTERVAL) {
    lastHeartbeatTime = currentTime;
    loraManager.sendDiscovery(String(mqttManager.getDeviceId()), hasNetworkConnection);
  }
  
  // 小延迟避免看门狗重启
  delay(10);
}