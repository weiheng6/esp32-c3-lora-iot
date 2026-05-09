#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "config.h"
#include "log_manager.h"
#include "sensor.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "lora_manager.h"
#include "relay_control.h"
#include "condition_control.h"
#include "web_server.h"
#include "system_monitor.h"
#include "web_ui.h"
#include "ota_manager.h"
#include "error_recovery.h"
#include "ntp_client.h"

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

// 动态时间间隔配置
unsigned long mqttReportInterval = DEFAULT_MQTT_REPORT_INTERVAL;  // MQTT 上报间隔（可动态调整）

// 【关键】手动控制模式标志（全局变量，非 static）
bool manualRelayMode = false;  // true = 用户手动控制，false = 自动控制
unsigned long manualModeEndTime = 0;  // 手动模式什么时候结束（如果为0则一直手动）

// 引用在 wifi_manager.cpp 中定义的 Preferences 对象
extern Preferences preferences;

// 诊断计数器
static uint32_t i2cFailureCount = 0;
static uint32_t mqttFailureCount = 0;
static uint32_t loopSlowCount = 0;  // 100ms+ 的 loop 计数
static unsigned long lastLoopDuration = 0;
static uint32_t i2cResetCount = 0;  // I2C 总线复位计数

// MQTT 重连机制配置
#define MQTT_RECONNECT_INTERVAL 5000  // 5秒重连一次
#define MQTT_RECONNECT_MAX_INTERVAL 60000  // 最多等60秒

// ==================== 状态查询处理函数 ====================
void handleStatusQuery(const String& queryType, JsonDocument& responseDoc) {
  Serial.printf("🔍 处理状态查询: %s\n", queryType.c_str());
  
  // 设备开关状态
  if (queryType == "relay" || queryType == "all") {
    JsonObject relayObj = responseDoc.createNestedObject("relay");
    relayObj["state"] = relayControl.getState() ? 1 : 0;
    relayObj["manual_mode"] = manualRelayMode;
  }
  
  // 条件控制详情
  if (queryType == "condition" || queryType == "all") {
    // 解析条件控制的JSON响应并合并到responseDoc
    String conditionJson = conditionControl.toJSON();
    StaticJsonDocument<600> conditionDoc;
    deserializeJson(conditionDoc, conditionJson);
    responseDoc["condition"] = conditionDoc.as<JsonObject>();
  }
  
  // 定时控制详情
  if (queryType == "timer" || queryType == "all") {
    // 解析定时控制的JSON响应并合并到responseDoc
    String timerJson = conditionControl.getTimerJSON();
    StaticJsonDocument<500> timerDoc;
    deserializeJson(timerDoc, timerJson);
    responseDoc["timer"] = timerDoc.as<JsonObject>();
  }
  
  // 网络状态
  if (queryType == "network" || queryType == "all") {
    JsonObject networkObj = responseDoc.createNestedObject("network");
    networkObj["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
    networkObj["mqtt_connected"] = mqttManager.isConnected();
    networkObj["rssi"] = WiFi.RSSI();
    if (WiFi.status() == WL_CONNECTED) {
      networkObj["ip"] = WiFi.localIP().toString();
    }
  }
  
  // 设备信息
  if (queryType == "device" || queryType == "all") {
    JsonObject deviceObj = responseDoc.createNestedObject("device");
    deviceObj["id"] = mqttManager.getDeviceId();
    deviceObj["status"] = deviceStatus;
    deviceObj["free_heap"] = ESP.getFreeHeap();
    deviceObj["heap_size"] = ESP.getHeapSize();
  }
  
  // 传感器数据
  if (queryType == "sensor" || queryType == "all") {
    float temperature, humidity;
    if (sensorManager.readBoth(temperature, humidity)) {
      JsonObject sensorObj = responseDoc.createNestedObject("sensor");
      sensorObj["temperature"] = temperature;
      sensorObj["humidity"] = humidity;
    }
  }
}

// ==================== MQTT 消息回调 ====================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  #if DEBUG_MODE
  unsigned long callbackStart = millis();
  #endif
  
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
  
  // 处理 MQTT 上报间隔调整
  if (doc.containsKey("mqtt_report_interval")) {
    int interval = doc["mqtt_report_interval"];
    if (interval > 0) {
      mqttReportInterval = interval * 1000;  // 转换为毫秒
      lastMqttReportTime = millis();  // 重置时间戳，让新间隔立即生效
      Serial.printf("⏱️ MQTT上报间隔设置为：%d 秒 (%lu ms)\n", interval, mqttReportInterval);
      
      // 保存到 Preferences
      preferences.begin("mqtt_config", false);  // false 表示读写模式
      preferences.putULong("report_interval", mqttReportInterval);
      preferences.end();
      Serial.println("✅ MQTT上报间隔已保存到持久存储");
      
      char response[100];
      snprintf(response, sizeof(response), "{\"status\":\"ok\",\"mqtt_report_interval\":%d}", interval);
      mqttManager.publish(mqttManager.getRespTopic(), response);
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
  
  // 处理主动查询（支持多种查询类型）
  if (doc.containsKey("query")) {
    JsonVariant queryVariant = doc["query"];
    
    // 兼容旧格式: {"query": true}
    if (queryVariant.is<bool>() && queryVariant.as<bool>()) {
      needReport = true;
      Serial.println("📢 收到主动查询指令");
      mqttManager.publish(mqttManager.getRespTopic(), "{\"status\":\"ok\",\"message\":\"Query received\"}");
    }
    // 新格式: {"query": {"type": "xxx"}} 或 {"query": {"types": ["xxx", "yyy"]}}
    else if (queryVariant.is<JsonObject>()) {
      JsonObject queryObj = queryVariant.as<JsonObject>();
      
      // 构建响应文档
      StaticJsonDocument<800> responseDoc;
      responseDoc["status"] = "ok";
      
      // 处理单个类型查询
      if (queryObj.containsKey("type")) {
        String queryType = queryObj["type"].as<String>();
        handleStatusQuery(queryType, responseDoc);
      }
      // 处理多个类型查询
      else if (queryObj.containsKey("types")) {
        JsonArray typesArray = queryObj["types"].as<JsonArray>();
        for (JsonVariant type : typesArray) {
          String queryType = type.as<String>();
          handleStatusQuery(queryType, responseDoc);
        }
      }
      // 默认查询所有状态
      else {
        handleStatusQuery("all", responseDoc);
      }
      
      // 发布响应
      String response;
      serializeJson(responseDoc, response);
      mqttManager.publish(mqttManager.getRespTopic(), response.c_str());
      Serial.printf("📤 已发布状态查询响应: %s\n", response.c_str());
    }
  }
  
  // 处理条件控制配置
  if (doc.containsKey("condition")) {
    JsonObject conditionObj = doc["condition"].as<JsonObject>();
    
    if (conditionObj.containsKey("enabled")) {
      bool conditionEnabled = conditionObj["enabled"].as<bool>();
      bool previousEnabled = conditionControl.isEnabled();
      conditionControl.setEnabled(conditionEnabled);
      LOG_CMDF("� 条件控制%s", conditionControl.isEnabled() ? "已启用" : "已禁用");
      
      // 【关键修复】如果启用条件控制，必须禁用定时控制（互斥）
      if (conditionEnabled && conditionControl.isTimerEnabled()) {
        conditionControl.setTimerEnabled(false);
        LOG_CMD("⏰ 定时控制已自动禁用");
      }
      
      // 【新增】条件控制启用/禁用状态变化时，立即发布MQTT响应
      if (previousEnabled != conditionEnabled) {
        char response[60];
        snprintf(response, sizeof(response), 
                 "{\"status\":\"ok\",\"condition_enabled\":%s}", 
                 conditionEnabled ? "true" : "false");
        mqttManager.publish(mqttManager.getRespTopic(), response);
        Serial.printf("📤 条件控制%s - 已发布响应\n", conditionEnabled ? "已启用" : "已禁用");
      }
      
      // 【关键修复】如果禁用条件控制，则不处理其他条件参数，直接返回
      if (!conditionEnabled) {
        return;
      }
    }
    
    // 处理普通条件控制配置
    if (!conditionObj.containsKey("use_hysteresis") || !conditionObj["use_hysteresis"].as<bool>()) {
      // 单阈值模式
      bool hasTempCondition = false, hasHumiCondition = false;
      
      if (conditionObj.containsKey("temp") && conditionObj["temp"].is<JsonObject>()) {
        JsonObject tempObj = conditionObj["temp"].as<JsonObject>();
        if (tempObj.containsKey("enabled")) {
          bool enabled = tempObj["enabled"];
          float threshold = tempObj.containsKey("threshold") ? tempObj["threshold"].as<float>() : 25.0;
          int compareOp = tempObj.containsKey("compare") ? tempObj["compare"].as<int>() : COMPARE_GREATER_THAN;
          
          // 同时设置旧的条件变量（兼容性）和新的条件组
          conditionControl.setTempCondition(enabled, threshold, compareOp);
          conditionControl.setCondition(true, 0, enabled, SENSOR_TEMP, compareOp, threshold);
          
          hasTempCondition = enabled;
          LOG_CMDF("🌡️  温度条件：%s (阈值:%.1f°C)", enabled ? "启用" : "禁用", threshold);
        }
      }
      
      if (conditionObj.containsKey("humi") && conditionObj["humi"].is<JsonObject>()) {
        JsonObject humiObj = conditionObj["humi"].as<JsonObject>();
        if (humiObj.containsKey("enabled")) {
          bool enabled = humiObj["enabled"];
          float threshold = humiObj.containsKey("threshold") ? humiObj["threshold"].as<float>() : 60.0;
          int compareOp = humiObj.containsKey("compare") ? humiObj["compare"].as<int>() : COMPARE_GREATER_THAN;
          
          // 同时设置旧的条件变量（兼容性）和新的条件组
          conditionControl.setHumiCondition(enabled, threshold, compareOp);
          conditionControl.setCondition(true, 1, enabled, SENSOR_HUMI, compareOp, threshold);
          
          hasHumiCondition = enabled;
          LOG_CMDF("💧 湿度条件：%s (阈值:%.1f%%)", enabled ? "启用" : "禁用", threshold);
        }
      }
      
      if (conditionObj.containsKey("logic")) {
        bool andMode = (conditionObj["logic"].as<String>() == "and");
        conditionControl.setLogicMode(andMode);
        conditionControl.setConditionGroupLogic(true, andMode ? LOGIC_AND : LOGIC_OR);
        LOG_CMDF("🔗 逻辑模式：%s", andMode ? "AND" : "OR");
      }
      
      // 启用 ON 条件组（如果有任何条件启用）
      if (hasTempCondition || hasHumiCondition) {
        conditionControl.setConditionGroupEnabled(true, true);
      }
    }
    
    // 处理滞回控制配置
    if (conditionObj.containsKey("use_hysteresis") && conditionObj["use_hysteresis"].as<bool>()) {
      float tempHigh = 28.0, tempLow = 26.0, humiHigh = 70.0, humiLow = 60.0;
      bool tempEnabled = false, humiEnabled = false;
      
      if (conditionObj.containsKey("temp") && conditionObj["temp"].is<JsonObject>()) {
        JsonObject tempObj = conditionObj["temp"].as<JsonObject>();
        tempEnabled = tempObj.containsKey("enabled") ? tempObj["enabled"].as<bool>() : false;
        tempHigh = tempObj.containsKey("high_threshold") ? tempObj["high_threshold"].as<float>() : 28.0;
        tempLow = tempObj.containsKey("low_threshold") ? tempObj["low_threshold"].as<float>() : 26.0;
      }
      
      if (conditionObj.containsKey("humi") && conditionObj["humi"].is<JsonObject>()) {
        JsonObject humiObj = conditionObj["humi"].as<JsonObject>();
        humiEnabled = humiObj.containsKey("enabled") ? humiObj["enabled"].as<bool>() : false;
        humiHigh = humiObj.containsKey("high_threshold") ? humiObj["high_threshold"].as<float>() : 70.0;
        humiLow = humiObj.containsKey("low_threshold") ? humiObj["low_threshold"].as<float>() : 60.0;
      }
      
      // 启用滞回控制
      conditionControl.setHysteresis(true, tempHigh, tempLow, humiHigh, humiLow);
      
      // 设置温度和湿度条件（旧方式，兼容性）
      conditionControl.setTempCondition(tempEnabled, tempHigh, COMPARE_GREATER_THAN);
      conditionControl.setHumiCondition(humiEnabled, humiHigh, COMPARE_GREATER_THAN);
      
      // 【关键修复】同时设置新的条件组，确保条件检查能生效
      conditionControl.setCondition(true, 0, tempEnabled, SENSOR_TEMP, COMPARE_GREATER_THAN, tempHigh);
      conditionControl.setCondition(true, 1, humiEnabled, SENSOR_HUMI, COMPARE_GREATER_THAN, humiHigh);
      conditionControl.setConditionGroupLogic(true, LOGIC_OR);  // 滞回模式使用 OR 逻辑
      
      // 如果有任何条件启用，启用 ON 条件组
      if (tempEnabled || humiEnabled) {
        conditionControl.setConditionGroupEnabled(true, true);
      }
      
      // 确保条件控制已启用
      if (!conditionControl.isEnabled()) {
        conditionControl.setEnabled(true);
      }
      
      if (tempEnabled) LOG_CMDF("🌡️  温度滞回：%.1f°C(高) - %.1f°C(低)", tempHigh, tempLow);
      if (humiEnabled) LOG_CMDF("💧 湿度滞回：%.1f%%(高) - %.1f%%(低)", humiHigh, humiLow);
    }
    
    // 发送响应
    mqttManager.publish(mqttManager.getRespTopic(), conditionControl.toJSON().c_str());
  }
  
  // 处理定时控制配置
  if (doc.containsKey("timer")) {
    JsonObject timerObj = doc["timer"].as<JsonObject>();
    
    if (timerObj.containsKey("enabled")) {
      bool timerEnabled = timerObj["enabled"].as<bool>();
      conditionControl.setTimerEnabled(timerEnabled);
      LOG_CMDF("⏰ 定时控制%s", timerEnabled ? "已启用" : "已禁用");
      
      // 【关键修复】如果启用定时控制，必须禁用条件控制（定时优先级更高）
      if (timerEnabled && conditionControl.isEnabled()) {
        conditionControl.setEnabled(false);
        LOG_CMD("🔄 条件控制已自动禁用");
      }
    }
    
    // 处理时间段配置
    if (timerObj.containsKey("clear") && timerObj["clear"].as<bool>()) {
      conditionControl.clearTimeSlots();
      LOG_CMD("⏰ 已清除所有时间段");
    }
    
    if (timerObj.containsKey("slots") && timerObj["slots"].is<JsonArray>()) {
      JsonArray slotsArray = timerObj["slots"].as<JsonArray>();
      for (JsonObject slot : slotsArray) {
        uint8_t index = slot.containsKey("index") ? slot["index"].as<uint8_t>() : 0;
        bool enabled = slot.containsKey("enabled") ? slot["enabled"].as<bool>() : false;
        
        uint8_t startH = 0, startM = 0, endH = 0, endM = 0;
        bool state = false;
        
        if (slot.containsKey("start_time")) {
          String startTime = slot["start_time"].as<String>();
          sscanf(startTime.c_str(), "%hhu:%hhu", &startH, &startM);
        }
        if (slot.containsKey("end_time")) {
          String endTime = slot["end_time"].as<String>();
          sscanf(endTime.c_str(), "%hhu:%hhu", &endH, &endM);
        }
        state = slot.containsKey("state") ? slot["state"].as<bool>() : false;
        
        if (index < 8) {
          conditionControl.setTimeSlot(index, enabled, startH, startM, endH, endM, state);
          LOG_CMDF("⏰ 时间段 %d：%02d:%02d-%02d:%02d (状态:%s)", 
                  index, startH, startM, endH, endM, state ? "开启" : "关闭");
        }
      }
    }
    
    mqttManager.publish(mqttManager.getRespTopic(), conditionControl.getTimerJSON().c_str());
  }
  
  // 处理 WiFi 重置
  if (doc.containsKey("reset_wifi") && doc["reset_wifi"] == true) {
    Serial.println("🔄 收到重置 WiFi 指令");
    wifiManager.resetConfig();
    mqttManager.publish(mqttManager.getRespTopic(), "{\"status\":\"ok\",\"message\":\"WiFi reset\"}");
    delay(2000);
    ESP.restart();
  }
  
  // 处理 MQTT OTA 升级
  if (doc.containsKey("ota")) {
    JsonObject otaObj = doc["ota"].as<JsonObject>();
    
    if (otaObj.containsKey("url")) {
      String otaUrl = otaObj["url"].as<String>();
      Serial.printf("\n🔄 [OTA] 收到MQTT升级请求: %s\n", otaUrl.c_str());
      
      if (otaUrl.length() > 0) {
        // 上报开始升级
        mqttManager.publish(mqttManager.getRespTopic(), "{\"status\":\"ok\",\"message\":\"OTA update started\"}");
        
        // 执行升级
        bool success = otaManager.updateFromHTTP(otaUrl);
        
        if (success) {
          Serial.println("✅ [OTA] MQTT升级成功，设备将重启");
          mqttManager.publish(mqttManager.getRespTopic(), "{\"status\":\"ok\",\"message\":\"OTA update successful, restarting...\"}");
        } else {
          Serial.println("❌ [OTA] MQTT升级失败");
          mqttManager.publish(mqttManager.getRespTopic(), "{\"status\":\"error\",\"message\":\"OTA update failed\"}");
        }
      } else {
        Serial.println("❌ [OTA] OTA URL为空");
        mqttManager.publish(mqttManager.getRespTopic(), "{\"status\":\"error\",\"message\":\"OTA URL is empty\"}");
      }
    }
    
    if (otaObj.containsKey("status")) {
      // 查询OTA状态
      int status = otaManager.getStatus();
      int progress = otaManager.getProgress();
      char response[100];
      snprintf(response, sizeof(response), "{\"status\":\"ok\",\"ota_status\":%d,\"progress\":%d}", status, progress);
      mqttManager.publish(mqttManager.getRespTopic(), response);
    }
  }
  
  #if DEBUG_MODE
  unsigned long callbackDuration = millis() - callbackStart;
  if (callbackDuration > 50) {
    Serial.printf("⚠️  MQTT callback 耗时过长：%lu ms\n", callbackDuration);
  }
  #endif
}

// ==================== 检查网络连接 ====================
void checkNetworkConnection() {
  bool wifiConnected = (WiFi.status() == WL_CONNECTED);
  bool mqttConnected = mqttManager.isConnected();
  bool oldNetworkConnection = hasNetworkConnection;
  
  hasNetworkConnection = wifiConnected && mqttConnected;
  
  // 定期 I2C 总线复位检查（每 5 分钟或者发生多次慢循环时）
  static unsigned long lastI2CHealthCheck = 0;
  if (millis() - lastI2CHealthCheck > 300000) {  // 5 分钟一次
    lastI2CHealthCheck = millis();
    
    // 如果检测到大量慢循环（可能 I2C 卡住），执行复位
    if (loopSlowCount > 100) {
      Serial.println("⚠️⚠️ 检测到大量慢循环，执行 I2C 总线复位...");
      // I2C 总线复位
      Wire.end();
      delay(10);
      Wire.begin(SHT31_I2C_SDA, SHT31_I2C_SCL);
      Wire.setClock(400000);
      delay(10);
      i2cResetCount++;
      loopSlowCount = 0;  // 重置计数器
      Serial.printf("✅ I2C 总线已复位（第 %lu 次）\n", i2cResetCount);
    }
  }
  
  // 详细的网络状态诊断
  static unsigned long lastNetStatusLog = 0;
  if (millis() - lastNetStatusLog > 30000) {  // 每 30 秒输出一次
    lastNetStatusLog = millis();
    int wifiRssi = WiFi.RSSI();
    Serial.printf("🌐 网络状态诊断:\n");
    Serial.printf("   WiFi: %s (RSSI: %d dBm, 状态码: %d)\n", 
                  wifiConnected ? "✅ 已连接" : "❌ 未连接", wifiRssi, WiFi.status());
    Serial.printf("   MQTT: %s\n", mqttConnected ? "✅ 已连接" : "❌ 未连接");
    Serial.printf("   综合: %s\n", hasNetworkConnection ? "✅ 网络可用" : "⚠️  网络不可用");
    Serial.printf("   I2C复位次数: %lu\n", i2cResetCount);
  }
  
  // 状态变化时立即告警
  if (oldNetworkConnection != hasNetworkConnection) {
    if (hasNetworkConnection) {
      Serial.println("✅ 网络已恢复连接！");
    } else {
      Serial.printf("❌ 网络连接断开 (WiFi: %s, MQTT: %s)\n", 
                    wifiConnected ? "✅" : "❌", mqttConnected ? "✅" : "❌");
    }
  }
  
  if (hasNetworkConnection) {
    deviceStatus = STATUS_ONLINE_WITH_NETWORK;
  } else if (wifiConnected) {
    deviceStatus = STATUS_ONLINE;
  } else {
    deviceStatus = STATUS_OFFLINE;
  }
}

// 全局变量：记录上次继电器状态，用于检测变化
static bool lastRelayState = false;

// ==================== 采集传感器数据 ====================
void acquireSensorData() {
  float temperature, humidity;
  
  unsigned long sensorReadStart = millis();
  if (sensorManager.readBoth(temperature, humidity)) {
    unsigned long sensorReadDuration = millis() - sensorReadStart;
    
    // 监测 I2C 读取时间
    if (sensorReadDuration > 10) {
      Serial.printf("⚠️  I2C 读取耗时过长：%lu ms\n", sensorReadDuration);
    }
    
    Serial.print("🔍 本地采集 - 温度：");
    Serial.print(temperature);
    Serial.print(" °C\t湿度：");
    Serial.print(humidity);
    Serial.println(" %RH");
    
    // 记录继电器状态变化前的值
    bool previousRelayState = relayControl.getState();
    
    // 【关键修复】只在非手动模式下才执行自动控制
    if (!manualRelayMode) {
      // 执行条件控制或定时控制
      if (conditionControl.checkAllConditions(temperature, humidity)) {
        relayControl.turnOn();
      } else {
        relayControl.turnOff();
      }
    }
    
    // 检查继电器状态是否发生变化
    bool currentRelayState = relayControl.getState();
    if (previousRelayState != currentRelayState) {
      Serial.printf("🔌 继电器状态变化: %s -> %s\n", 
                    previousRelayState ? "开启" : "关闭", 
                    currentRelayState ? "开启" : "关闭");
      
      // 【新增】继电器状态变化时发布MQTT响应
      if (hasNetworkConnection && mqttManager.isConnected()) {
        char response[50];
        snprintf(response, sizeof(response), 
                 "{\"status\":\"ok\",\"relay\":%d,\"reason\":\"auto\"}", 
                 currentRelayState ? 1 : 0);
        mqttManager.publish(mqttManager.getRespTopic(), response);
        Serial.printf("📤 已发布继电器状态变化: %s\n", response);
      }
      
      // 更新上次状态记录
      lastRelayState = currentRelayState;
    }
  } else {
    i2cFailureCount++;
    if (i2cFailureCount % 10 == 0) {
      Serial.printf("❌ I2C 读取失败次数：%lu\n", i2cFailureCount);
    }
  }
}

// ==================== 上报传感器数据到 MQTT ====================
void reportSensorDataToMqtt() {
  float temperature, humidity;
  
  // 1. 首先检查网络连接状态
  if (!hasNetworkConnection) {
    // 详细诊断为什么网络不可用
    bool wifiConnected = (WiFi.status() == WL_CONNECTED);
    bool mqttConnected = mqttManager.isConnected();
    
    static unsigned long lastNetworkFailLog = 0;
    if (millis() - lastNetworkFailLog > 60000) {  // 60 秒内最多输出一次
      lastNetworkFailLog = millis();
      Serial.printf("⚠️  网络连接不可用（WiFi:%s, MQTT:%s）\n", 
                    wifiConnected ? "✅" : "❌", mqttConnected ? "✅" : "❌");
      if (!wifiConnected) {
        Serial.printf("   → WiFi 状态码：%d (期望: %d)\n", WiFi.status(), WL_CONNECTED);
      }
      if (!mqttConnected) {
        Serial.println("   → MQTT 未连接，需要重新连接");
      }
    }
    return;
  }
  
  // 2. 检查 WiFi 信号强度（< -70dBm 时跳过上报）
  int rssi = WiFi.RSSI();
  if (rssi < -70) {
    Serial.printf("⚠️  WiFi 信号弱（%d dBm），跳过 MQTT 上报\n", rssi);
    return;
  }
  
  // 3. 读取传感器数据
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
    
    // 4. 尝试发送数据
    unsigned long mqttSendStart = millis();
    bool tempPublished = mqttManager.publish(mqttManager.getTempTopic(), tempStr);
    bool humPublished = mqttManager.publish(mqttManager.getHumTopic(), humStr);
    unsigned long mqttSendDuration = millis() - mqttSendStart;
    
    // 5. 监测 MQTT 发送时间
    if (mqttSendDuration > 10) {
      Serial.printf("⚠️  MQTT 发送耗时：%lu ms\n", mqttSendDuration);
    }
    
    // 6. 记录发送失败
    if (!tempPublished || !humPublished) {
      mqttFailureCount++;
      if (mqttFailureCount % 10 == 0) {
        Serial.printf("❌ MQTT 发送失败次数：%lu (temp:%s, hum:%s)\n", 
                      mqttFailureCount, tempPublished ? "✅" : "❌", humPublished ? "✅" : "❌");
      }
    }
  }
}

// ==================== Setup 函数 ====================
void setup() {
  Serial.begin(115200);
  delay(500);
  
  // 初始化日志管理器
  // 调整日志级别：LOG_DEBUG - 打印所有日志
  //              LOG_INFO  - 仅打印信息和错误
  //              LOG_ERROR - 仅打印错误
  LogManager::init(LOG_DEBUG);  // 设置为调试模式，打印所有日志
  
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║   ESP32-C3 条件控制器启动              ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("");
  
  // 加载保存的 MQTT 上报间隔配置
  preferences.begin("mqtt_config", true);  // true 表示只读模式
  unsigned long savedInterval = preferences.getULong("report_interval", DEFAULT_MQTT_REPORT_INTERVAL);
  preferences.end();
  if (savedInterval > 0 && savedInterval != DEFAULT_MQTT_REPORT_INTERVAL) {
    mqttReportInterval = savedInterval;
    Serial.printf("✅ 已加载保存的MQTT上报间隔：%lu ms (%lu 秒)\n", mqttReportInterval, mqttReportInterval / 1000);
  } else {
    Serial.printf("✅ 使用默认MQTT上报间隔：%lu ms (1 秒)\n", DEFAULT_MQTT_REPORT_INTERVAL);
  }
  
  // 初始化各模块
  Serial.println("🔄 初始化模块...");
  
  // 1. 传感器
  sensorManager.begin();
  
  // 2. WiFi
  wifiManager.begin();
  
  // 3. NTP客户端（使用阿里云NTP服务器）
  ntpClient.begin("ntp.aliyun.com", 123, 8 * 3600);
  
  // 4. MQTT
  mqttManager.begin();
  mqttManager.setCallback(mqttCallback);
  
  // 5. LoRa
  loraManager.begin();
  
  // 6. 继电器
  relayControl.begin();
  
  // 7. 条件控制
  conditionControl.begin();
  
  // 8. 系统监控
  Serial.println("✅ 所有模块初始化完成！");
  Serial.println("════════════════════════════════════════");
  
  // 9. 错误恢复（故障诊断和恢复）
  errorRecovery.begin();
  
  // 10. OTA 固件升级（基于 WiFi）
  if (WiFi.status() == WL_CONNECTED) {
    String deviceName = String("esp32-c3-") + String(mqttManager.getDeviceId()).substring(0, 6);
    otaManager.begin(deviceName.c_str());
  }
  
  // 10. 增强的 Web UI（在 WiFi 连接后才启动，用于内网管理）
  // 注意：webServerManager 在 WiFi 未配置时由 wifiManager 启动，用于配置 WiFi
  // 这里的 webUIManager 在 WiFi 连接后启动，提供高级功能
  if (WiFi.status() == WL_CONNECTED) {
    webUIManager.begin();
    Serial.printf("✅ Web UI 已启动 - 访问地址：http://%s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("📌 WiFi 未连接，Web UI 将在连接后启动");
  }
  
  Serial.println("🎉 固件初始化完成！");
}

// ==================== Loop 函数 ====================
void loop() {
  unsigned long loopStart = millis();
  unsigned long currentTime = loopStart;
  bool wifiConnected = (WiFi.status() == WL_CONNECTED);
  
  // 检查网络连接
  checkNetworkConnection();
  
  // 定期检查 WiFi
  if (currentTime - lastWifiCheckTime >= WIFI_CHECK_INTERVAL) {
    lastWifiCheckTime = currentTime;
    wifiManager.connect();
    
    // 如果 WiFi 刚连接，启动 Web UI
    static bool webUIStarted = false;
    static bool mgmtInfoPublished = false;
    if (wifiConnected && !webUIStarted) {
      webUIStarted = true;
      if (!webUIManager.isRunning()) {
        webUIManager.begin();
        Serial.printf("✅ Web UI 已启动 - http://%s\n", WiFi.localIP().toString().c_str());
      }
    }
    
    // 如果 WiFi 已连接且 MQTT 已连接，推送管理信息一次
    if (wifiConnected && mqttManager.isConnected() && !mgmtInfoPublished) {
      mgmtInfoPublished = true;
      mqttManager.publishManagementInfo(WiFi.localIP().toString().c_str());
    }
  }
  
  // 连接 MQTT（带重连机制）
  if (!mqttManager.isConnected() && wifiConnected) {
    if (currentTime - lastMqttConnectAttempt >= MQTT_RECONNECT_INTERVAL) {
      lastMqttConnectAttempt = currentTime;
      Serial.println("🔄 尝试重新连接 MQTT...");
      mqttManager.connect();
    }
  } else if (mqttManager.isConnected()) {
    lastMqttConnectAttempt = 0;
  }
  
  // MQTT 循环
  mqttManager.loop();
  
  // 处理 HTTP 请求
  webServerManager.handleClient();
  webUIManager.handleClient();
  
  // 处理 LoRa 消息和队列
  String loraMessage = loraManager.receiveMessage();
  if (loraMessage.length() > 0) {
    loraManager.parseMessage(loraMessage);
  }
  loraManager.processQueue();
  
  // 定期采集传感器数据
  if (currentTime - lastAcquisitionTime >= DEFAULT_ACQUISITION_INTERVAL) {
    lastAcquisitionTime = currentTime;
    acquireSensorData();
  }
  
  // 定期上报数据到 MQTT
  if (currentTime - lastMqttReportTime >= mqttReportInterval || needReport) {
    lastMqttReportTime = currentTime;
    needReport = false;
    
    if (hasNetworkConnection) {
      reportSensorDataToMqtt();
    } else {
      Serial.println("⚠️  网络连接不可用，跳过本次上报");
    }
  }
  
  // 定期输出系统状态和内存监控
  if (currentTime - lastNodeDiscoveryTime >= SYSTEM_STATS_INTERVAL) {
    lastNodeDiscoveryTime = currentTime;
    systemMonitor.printStats();
    systemMonitor.checkMemory();
    
    unsigned long freeHeap = ESP.getFreeHeap();
    unsigned long totalHeap = ESP.getHeapSize();
    float usagePercent = 100.0 * (totalHeap - freeHeap) / totalHeap;
    Serial.printf("💾 内存: %lu/%lu (%.1f%%)\n", freeHeap, totalHeap, usagePercent);
  }
  
  // LoRa 节点发现
  if (currentTime - lastHeartbeatTime >= LORA_NODE_DISCOVERY_INTERVAL) {
    lastHeartbeatTime = currentTime;
    loraManager.sendDiscovery(String(mqttManager.getDeviceId()), hasNetworkConnection);
  }
  
  // 处理 OTA 固件升级和故障恢复
  otaManager.handle();
  errorRecovery.checkAndRecover();
  
  // 小延迟避免看门狗重启，并给系统任务（WiFi、蓝牙等）足够的 CPU 时间
  delay(20);  // 20ms 延迟
  
  // 诊断：精准失败计数和异常检测（取消冗长树形输出）
  lastLoopDuration = millis() - loopStart;
  
  if (lastLoopDuration > 100) {
    loopSlowCount++;
    Serial.printf("⚠️  慢循环 #%lu: %lu ms （I2C失败x%lu, MQTT失败x%lu）\n", 
                  loopSlowCount, lastLoopDuration, i2cFailureCount, mqttFailureCount);
  }
  
  // 每 30 秒输出一次汇总
  static unsigned long lastSummaryTime = 0;
  if (millis() - lastSummaryTime > 30000) {
    lastSummaryTime = millis();
    Serial.printf("📊 30秒汇总: 平均循环时间=%lu ms, 慢循环=%lu, I2C失败=%lu, MQTT失败=%lu\n", 
                  lastLoopDuration, loopSlowCount, i2cFailureCount, mqttFailureCount);
  }
}