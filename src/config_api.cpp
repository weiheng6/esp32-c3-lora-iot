#include "config_api.h"
#include "config.h"
#include "lora_params.h"
#include "power_manager.h"
#include "pin_config.h"
#include "mqtt_manager.h"
#include "wifi_manager.h"

// 声明外部的 WiFiManager 实例
extern WiFiManager wifiManager;

extern PinConfigManager pinConfigManager;

ConfigAPI* ConfigAPI::instance = nullptr;

ConfigAPI::ConfigAPI() {
    instance = this;
}

bool ConfigAPI::begin() {
    if (!preferences.begin("config_api", false)) {
        Serial.println("[ConfigAPI] Failed to begin preferences");
        return false;
    }

    loadDeviceConfig();
    loadNetworkConfig();
    loadControlConfig();
    loadSystemConfig();
    loadLoraConfig();
    loadPowerConfig();
    loadPinConfig();
    
    // 启动时同步 WiFi 配置：如果 ConfigAPI 有配置而 WiFiManager 没有，同步过去
    if (strlen(networkConfig.wifiSsid) > 0 && !wifiManager.isConfigured()) {
        Serial.printf("[ConfigAPI] 同步 WiFi 配置到 WiFiManager: %s\n", networkConfig.wifiSsid);
        wifiManager.setCredentials(networkConfig.wifiSsid, networkConfig.wifiPassword);
    }
    // 反之，如果 WiFiManager 有配置而 ConfigAPI 没有，同步到 ConfigAPI
    else if (strlen(wifiManager.getSSID()) > 0 && strlen(networkConfig.wifiSsid) == 0) {
        Serial.printf("[ConfigAPI] 从 WiFiManager 同步 WiFi 配置: %s\n", wifiManager.getSSID());
        strncpy(networkConfig.wifiSsid, wifiManager.getSSID(), sizeof(networkConfig.wifiSsid) - 1);
        strncpy(networkConfig.wifiPassword, wifiManager.getPassword(), sizeof(networkConfig.wifiPassword) - 1);
        saveNetworkConfig();
    }

    Serial.println("[ConfigAPI] Configuration API initialized");
    return true;
}

void ConfigAPI::end() {
    preferences.end();
}

bool ConfigAPI::loadDeviceConfig() {
    preferences.getString("dev_name", deviceConfig.deviceName, sizeof(deviceConfig.deviceName));
    preferences.getString("dev_id", deviceConfig.deviceId, sizeof(deviceConfig.deviceId));
    deviceConfig.hardwareVersion = preferences.getUChar("hw_ver", 1);
    deviceConfig.firmwareVersion = preferences.getUInt("fw_ver", 100);
    deviceConfig.sensorType = preferences.getUChar("sensor_type", 0);
    deviceConfig.acquisitionInterval = preferences.getUShort("acq_int", 1000);
    deviceConfig.reportInterval = preferences.getUShort("rpt_int", 5000);
    deviceConfig.autoReportEnabled = preferences.getBool("auto_rpt", true);
    deviceConfig.lowPowerMode = preferences.getBool("low_pwr", false);
    return true;
}

bool ConfigAPI::saveDeviceConfig() {
    preferences.putString("dev_name", deviceConfig.deviceName);
    preferences.putString("dev_id", deviceConfig.deviceId);
    preferences.putUChar("hw_ver", deviceConfig.hardwareVersion);
    preferences.putUInt("fw_ver", deviceConfig.firmwareVersion);
    preferences.putUChar("sensor_type", deviceConfig.sensorType);
    preferences.putUShort("acq_int", deviceConfig.acquisitionInterval);
    preferences.putUShort("rpt_int", deviceConfig.reportInterval);
    preferences.putBool("auto_rpt", deviceConfig.autoReportEnabled);
    preferences.putBool("low_pwr", deviceConfig.lowPowerMode);
    return true;
}

bool ConfigAPI::loadNetworkConfig() {
    preferences.getString("wifi_ssid", networkConfig.wifiSsid, sizeof(networkConfig.wifiSsid));
    preferences.getString("wifi_pass", networkConfig.wifiPassword, sizeof(networkConfig.wifiPassword));
    preferences.getString("mqtt_srv", networkConfig.mqttServer, sizeof(networkConfig.mqttServer));
    networkConfig.mqttPort = preferences.getUShort("mqtt_port", 1883);
    preferences.getString("mqtt_user", networkConfig.mqttUsername, sizeof(networkConfig.mqttUsername));
    preferences.getString("mqtt_pass", networkConfig.mqttPassword, sizeof(networkConfig.mqttPassword));
    preferences.getString("mqtt_topic", networkConfig.mqttTopic, sizeof(networkConfig.mqttTopic));
    networkConfig.transmissionMode = preferences.getUChar("tx_mode", 0);
    networkConfig.mqttEnabled = preferences.getBool("mqtt_en", true);
    networkConfig.loraEnabled = preferences.getBool("lora_en", true);
    return true;
}

bool ConfigAPI::saveNetworkConfig() {
    preferences.putString("wifi_ssid", networkConfig.wifiSsid);
    preferences.putString("wifi_pass", networkConfig.wifiPassword);
    preferences.putString("mqtt_srv", networkConfig.mqttServer);
    preferences.putUShort("mqtt_port", networkConfig.mqttPort);
    preferences.putString("mqtt_user", networkConfig.mqttUsername);
    preferences.putString("mqtt_pass", networkConfig.mqttPassword);
    preferences.putString("mqtt_topic", networkConfig.mqttTopic);
    preferences.putUChar("tx_mode", networkConfig.transmissionMode);
    preferences.putBool("mqtt_en", networkConfig.mqttEnabled);
    preferences.putBool("lora_en", networkConfig.loraEnabled);
    return true;
}

bool ConfigAPI::loadControlConfig() {
    controlConfig.conditionEnabled = preferences.getBool("cond_en", false);
    controlConfig.conditionLogic = preferences.getUChar("cond_logic", 0);
    controlConfig.timerEnabled = preferences.getBool("timer_en", false);
    controlConfig.timeSlotCount = preferences.getUChar("timer_count", 0);
    controlConfig.manualModeEnabled = preferences.getBool("manual_en", false);
    controlConfig.relayState = preferences.getUChar("relay_state", 0);
    return true;
}

bool ConfigAPI::saveControlConfig() {
    preferences.putBool("cond_en", controlConfig.conditionEnabled);
    preferences.putUChar("cond_logic", controlConfig.conditionLogic);
    preferences.putBool("timer_en", controlConfig.timerEnabled);
    preferences.putUChar("timer_count", controlConfig.timeSlotCount);
    preferences.putBool("manual_en", controlConfig.manualModeEnabled);
    preferences.putUChar("relay_state", controlConfig.relayState);
    return true;
}

bool ConfigAPI::loadSystemConfig() {
    systemConfig.logLevel = preferences.getUChar("log_level", 1);
    systemConfig.otaAutoCheck = preferences.getBool("ota_auto", false);
    preferences.getString("ota_url", systemConfig.otaUrl, sizeof(systemConfig.otaUrl));
    systemConfig.securityEnabled = preferences.getBool("sec_en", false);
    preferences.getString("admin_pwd", systemConfig.adminPassword, sizeof(systemConfig.adminPassword));
    return true;
}

bool ConfigAPI::saveSystemConfig() {
    preferences.putUChar("log_level", systemConfig.logLevel);
    preferences.putBool("ota_auto", systemConfig.otaAutoCheck);
    preferences.putString("ota_url", systemConfig.otaUrl);
    preferences.putBool("sec_en", systemConfig.securityEnabled);
    preferences.putString("admin_pwd", systemConfig.adminPassword);
    return true;
}

bool ConfigAPI::loadLoraConfig() {
    preferences.getBytes("lora_params", &loraParams, sizeof(LoRaParams));
    preferences.getBytes("lora_adapt", &loraAdaptiveConfig, sizeof(LoRaAdaptiveConfig));
    return true;
}

bool ConfigAPI::saveLoraConfig() {
    preferences.putBytes("lora_params", &loraParams, sizeof(LoRaParams));
    preferences.putBytes("lora_adapt", &loraAdaptiveConfig, sizeof(LoRaAdaptiveConfig));
    return true;
}

bool ConfigAPI::loadPowerConfig() {
    powerConfig.sleepDuration = preferences.getUInt("sleep_dur", 60000000);
    powerConfig.wakeInterval = preferences.getUInt("wake_int", 300000);
    powerConfig.wifiEnabled = preferences.getBool("wifi_en", true);
    powerConfig.loraEnabled = preferences.getBool("lora_pwr_en", true);
    powerConfig.sensorEnabled = preferences.getBool("sensor_en", true);
    powerConfig.cpuFreq = preferences.getUChar("cpu_freq", 80);
    powerConfig.txPower = preferences.getChar("tx_pwr", 17);
    return true;
}

bool ConfigAPI::savePowerConfig() {
    preferences.putUInt("sleep_dur", powerConfig.sleepDuration);
    preferences.putUInt("wake_int", powerConfig.wakeInterval);
    preferences.putBool("wifi_en", powerConfig.wifiEnabled);
    preferences.putBool("lora_pwr_en", powerConfig.loraEnabled);
    preferences.putBool("sensor_en", powerConfig.sensorEnabled);
    preferences.putUChar("cpu_freq", powerConfig.cpuFreq);
    preferences.putChar("tx_pwr", powerConfig.txPower);
    return true;
}

bool ConfigAPI::loadPinConfig() {
    for (int i = 0; i < 20; i++) {
        char key[16];
        snprintf(key, sizeof(key), "pin_%d", i);
        preferences.getBytes(key, &pinConfigs[i], sizeof(PinConfig));
    }
    return true;
}

bool ConfigAPI::savePinConfig() {
    for (int i = 0; i < 20; i++) {
        char key[16];
        snprintf(key, sizeof(key), "pin_%d", i);
        preferences.putBytes(key, &pinConfigs[i], sizeof(PinConfig));
    }
    return true;
}

String ConfigAPI::deviceConfigToJson() const {
    StaticJsonDocument<512> doc;
    doc["deviceName"] = deviceConfig.deviceName;
    doc["deviceId"] = deviceConfig.deviceId;
    doc["hardwareVersion"] = deviceConfig.hardwareVersion;
    doc["firmwareVersion"] = deviceConfig.firmwareVersion;
    doc["sensorType"] = deviceConfig.sensorType;
    doc["acquisitionInterval"] = deviceConfig.acquisitionInterval;
    doc["reportInterval"] = deviceConfig.reportInterval;
    doc["autoReportEnabled"] = deviceConfig.autoReportEnabled;
    doc["lowPowerMode"] = deviceConfig.lowPowerMode;
    
    String output;
    serializeJson(doc, output);
    return output;
}

String ConfigAPI::networkConfigToJson() const {
    StaticJsonDocument<512> doc;
    doc["wifiSsid"] = networkConfig.wifiSsid;
    doc["wifiPassword"] = networkConfig.wifiPassword;
    doc["mqttServer"] = networkConfig.mqttServer;
    doc["mqttPort"] = networkConfig.mqttPort;
    doc["mqttUsername"] = networkConfig.mqttUsername;
    doc["mqttPassword"] = networkConfig.mqttPassword;
    doc["mqttTopic"] = networkConfig.mqttTopic;
    doc["transmissionMode"] = networkConfig.transmissionMode;
    doc["mqttEnabled"] = networkConfig.mqttEnabled;
    doc["loraEnabled"] = networkConfig.loraEnabled;
    doc["wifiConnected"] = (WiFi.status() == WL_CONNECTED);
    doc["mqttConnected"] = mqttManager.isConnected();
    
    // 添加AP模式和IP地址信息
    bool isAPMode = WiFi.getMode() & WIFI_AP;
    doc["isAPMode"] = isAPMode;
    if (isAPMode) {
        doc["apIP"] = WiFi.softAPIP().toString();
        doc["apSSID"] = AP_SSID;
        doc["stationIP"] = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";
    } else {
        doc["stationIP"] = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

String ConfigAPI::loraConfigToJson() const {
    StaticJsonDocument<512> doc;
    doc["frequency"] = loraParams.frequency;
    doc["bandwidth"] = loraParams.bandwidth;
    doc["spreadingFactor"] = loraParams.spreadingFactor;
    doc["codingRate"] = loraParams.codingRate;
    doc["txPower"] = loraParams.txPower;
    doc["preambleLength"] = loraParams.preambleLength;
    doc["adaptiveEnabled"] = loraAdaptiveConfig.enabled;
    doc["minSensitivity"] = loraAdaptiveConfig.minSensitivity;
    doc["maxAdjustInterval"] = loraAdaptiveConfig.maxAdjustInterval;
    doc["frequencyHoppingThreshold"] = loraAdaptiveConfig.frequencyHoppingThreshold;
    doc["currentPreset"] = loraParamsManager.getCurrentPreset();
    
    String output;
    serializeJson(doc, output);
    return output;
}

String ConfigAPI::powerConfigToJson() const {
    StaticJsonDocument<512> doc;
    doc["sleepDuration"] = powerConfig.sleepDuration;
    doc["wakeInterval"] = powerConfig.wakeInterval;
    doc["wifiEnabled"] = powerConfig.wifiEnabled;
    doc["loraEnabled"] = powerConfig.loraEnabled;
    doc["sensorEnabled"] = powerConfig.sensorEnabled;
    doc["cpuFreq"] = powerConfig.cpuFreq;
    doc["txPower"] = powerConfig.txPower;
    doc["currentMode"] = powerManager.getCurrentMode();
    doc["batteryVoltage"] = powerManager.getBatteryInfo().voltage;
    doc["batteryPercentage"] = powerManager.getBatteryInfo().percentage;
    doc["isCharging"] = powerManager.getBatteryInfo().isCharging;
    
    String output;
    serializeJson(doc, output);
    return output;
}

String ConfigAPI::pinConfigToJson() const {
    StaticJsonDocument<1024> doc;
    JsonArray pins = doc.to<JsonArray>();
    
    std::vector<PinConfig*> allConfigs = pinConfigManager.getAllPinConfigs();
    for (const auto& cfg : allConfigs) {
        JsonObject pin = pins.createNestedObject();
        pin["pin"] = cfg->pinNumber;
        pin["mode"] = cfg->mode;
        pin["function"] = cfg->function;
        pin["functionName"] = cfg->functionName;
        pin["initialValue"] = cfg->initialValue;
        pin["isReserved"] = cfg->isReserved;
        pin["isInitialized"] = cfg->isInitialized;
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

String ConfigAPI::controlConfigToJson() const {
    StaticJsonDocument<512> doc;
    doc["conditionEnabled"] = controlConfig.conditionEnabled;
    doc["conditionLogic"] = controlConfig.conditionLogic;
    doc["timerEnabled"] = controlConfig.timerEnabled;
    doc["timeSlotCount"] = controlConfig.timeSlotCount;
    doc["manualModeEnabled"] = controlConfig.manualModeEnabled;
    doc["relayState"] = controlConfig.relayState;
    
    String output;
    serializeJson(doc, output);
    return output;
}

String ConfigAPI::systemConfigToJson() const {
    StaticJsonDocument<512> doc;
    doc["logLevel"] = systemConfig.logLevel;
    doc["otaAutoCheck"] = systemConfig.otaAutoCheck;
    doc["otaUrl"] = systemConfig.otaUrl;
    doc["securityEnabled"] = systemConfig.securityEnabled;
    doc["adminPassword"] = systemConfig.adminPassword;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["heapSize"] = ESP.getHeapSize();
    doc["uptime"] = millis() / 1000;
    
    String output;
    serializeJson(doc, output);
    return output;
}

String ConfigAPI::getDeviceConfig() const {
    return deviceConfigToJson();
}

bool ConfigAPI::setDeviceConfig(const String& json) {
    if (!parseDeviceConfig(json)) {
        return false;
    }
    return saveDeviceConfig();
}

bool ConfigAPI::parseDeviceConfig(const String& json) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.println("[ConfigAPI] Failed to parse device config JSON");
        return false;
    }
    
    if (doc.containsKey("deviceName")) {
        strncpy(deviceConfig.deviceName, doc["deviceName"].as<const char*>(), sizeof(deviceConfig.deviceName) - 1);
    }
    if (doc.containsKey("deviceId")) {
        strncpy(deviceConfig.deviceId, doc["deviceId"].as<const char*>(), sizeof(deviceConfig.deviceId) - 1);
    }
    if (doc.containsKey("hardwareVersion")) {
        deviceConfig.hardwareVersion = doc["hardwareVersion"].as<uint8_t>();
    }
    if (doc.containsKey("firmwareVersion")) {
        deviceConfig.firmwareVersion = doc["firmwareVersion"].as<uint32_t>();
    }
    if (doc.containsKey("sensorType")) {
        deviceConfig.sensorType = doc["sensorType"].as<uint8_t>();
    }
    if (doc.containsKey("acquisitionInterval")) {
        deviceConfig.acquisitionInterval = doc["acquisitionInterval"].as<uint16_t>();
    }
    if (doc.containsKey("reportInterval")) {
        deviceConfig.reportInterval = doc["reportInterval"].as<uint16_t>();
    }
    if (doc.containsKey("autoReportEnabled")) {
        deviceConfig.autoReportEnabled = doc["autoReportEnabled"].as<bool>();
    }
    if (doc.containsKey("lowPowerMode")) {
        deviceConfig.lowPowerMode = doc["lowPowerMode"].as<bool>();
    }
    
    return true;
}

String ConfigAPI::getNetworkConfig() const {
    return networkConfigToJson();
}

bool ConfigAPI::setNetworkConfig(const String& json) {
    // 先记录旧的 WiFi 配置，用于判断是否需要重新连接
    String oldWifiSsid = networkConfig.wifiSsid;
    String oldWifiPassword = networkConfig.wifiPassword;
    
    if (!parseNetworkConfig(json)) {
        return false;
    }
    
    // 保存到 ConfigAPI 的存储
    if (!saveNetworkConfig()) {
        return false;
    }
    
    // 检查 WiFi 配置是否有变化
    bool wifiConfigChanged = 
        (String(networkConfig.wifiSsid) != oldWifiSsid) || 
        (String(networkConfig.wifiPassword) != oldWifiPassword);
    
    // 如果 WiFi 配置有变化，同步到 WiFiManager 并尝试连接
    if (wifiConfigChanged && strlen(networkConfig.wifiSsid) > 0) {
        Serial.printf("🔄 WiFi配置已更新 - 新SSID: %s\n", networkConfig.wifiSsid);
        wifiManager.setCredentials(networkConfig.wifiSsid, networkConfig.wifiPassword);
        
        // 断开当前连接（如果有的话）
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("🔄 断开当前WiFi连接...");
            WiFi.disconnect(true);
            delay(100);
        }
        
        // 尝试连接新WiFi
        Serial.println("🔄 尝试连接新的WiFi...");
        // 注意：WiFi 连接会在主循环的 checkNetworkConnection 中处理
    }
    
    return true;
}

bool ConfigAPI::parseNetworkConfig(const String& json) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.println("[ConfigAPI] Failed to parse network config JSON");
        return false;
    }
    
    if (doc.containsKey("wifiSsid")) {
        strncpy(networkConfig.wifiSsid, doc["wifiSsid"].as<const char*>(), sizeof(networkConfig.wifiSsid) - 1);
    }
    if (doc.containsKey("wifiPassword")) {
        strncpy(networkConfig.wifiPassword, doc["wifiPassword"].as<const char*>(), sizeof(networkConfig.wifiPassword) - 1);
    }
    if (doc.containsKey("mqttServer")) {
        strncpy(networkConfig.mqttServer, doc["mqttServer"].as<const char*>(), sizeof(networkConfig.mqttServer) - 1);
    }
    if (doc.containsKey("mqttPort")) {
        networkConfig.mqttPort = doc["mqttPort"].as<uint16_t>();
    }
    if (doc.containsKey("mqttUsername")) {
        strncpy(networkConfig.mqttUsername, doc["mqttUsername"].as<const char*>(), sizeof(networkConfig.mqttUsername) - 1);
    }
    if (doc.containsKey("mqttPassword")) {
        strncpy(networkConfig.mqttPassword, doc["mqttPassword"].as<const char*>(), sizeof(networkConfig.mqttPassword) - 1);
    }
    if (doc.containsKey("mqttTopic")) {
        strncpy(networkConfig.mqttTopic, doc["mqttTopic"].as<const char*>(), sizeof(networkConfig.mqttTopic) - 1);
    }
    if (doc.containsKey("transmissionMode")) {
        networkConfig.transmissionMode = doc["transmissionMode"].as<uint8_t>();
    }
    if (doc.containsKey("mqttEnabled")) {
        networkConfig.mqttEnabled = doc["mqttEnabled"].as<bool>();
    }
    if (doc.containsKey("loraEnabled")) {
        networkConfig.loraEnabled = doc["loraEnabled"].as<bool>();
    }
    
    return true;
}

String ConfigAPI::getLoraParams() const {
    return loraConfigToJson();
}

bool ConfigAPI::setLoraParams(const String& json) {
    if (!parseLoraConfig(json)) {
        return false;
    }
    return saveLoraConfig();
}

bool ConfigAPI::parseLoraConfig(const String& json) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.println("[ConfigAPI] Failed to parse LoRa config JSON");
        return false;
    }
    
    if (doc.containsKey("frequency")) {
        loraParams.frequency = doc["frequency"].as<uint32_t>();
    }
    if (doc.containsKey("bandwidth")) {
        loraParams.bandwidth = doc["bandwidth"].as<uint32_t>();
    }
    if (doc.containsKey("spreadingFactor")) {
        loraParams.spreadingFactor = doc["spreadingFactor"].as<uint8_t>();
    }
    if (doc.containsKey("codingRate")) {
        loraParams.codingRate = doc["codingRate"].as<uint8_t>();
    }
    if (doc.containsKey("txPower")) {
        loraParams.txPower = doc["txPower"].as<int8_t>();
    }
    if (doc.containsKey("preambleLength")) {
        loraParams.preambleLength = doc["preambleLength"].as<uint16_t>();
    }
    if (doc.containsKey("adaptiveEnabled")) {
        loraAdaptiveConfig.enabled = doc["adaptiveEnabled"].as<bool>();
    }
    if (doc.containsKey("minSensitivity")) {
        loraAdaptiveConfig.minSensitivity = doc["minSensitivity"].as<int8_t>();
    }
    if (doc.containsKey("maxAdjustInterval")) {
        loraAdaptiveConfig.maxAdjustInterval = doc["maxAdjustInterval"].as<uint32_t>();
    }
    if (doc.containsKey("frequencyHoppingThreshold")) {
        loraAdaptiveConfig.frequencyHoppingThreshold = doc["frequencyHoppingThreshold"].as<uint32_t>();
    }
    
    return true;
}

String ConfigAPI::getPowerConfig() const {
    return powerConfigToJson();
}

bool ConfigAPI::setPowerConfig(const String& json) {
    if (!parsePowerConfig(json)) {
        return false;
    }
    return savePowerConfig();
}

bool ConfigAPI::parsePowerConfig(const String& json) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.println("[ConfigAPI] Failed to parse power config JSON");
        return false;
    }
    
    if (doc.containsKey("sleepDuration")) {
        powerConfig.sleepDuration = doc["sleepDuration"].as<uint32_t>();
    }
    if (doc.containsKey("wakeInterval")) {
        powerConfig.wakeInterval = doc["wakeInterval"].as<uint32_t>();
    }
    if (doc.containsKey("wifiEnabled")) {
        powerConfig.wifiEnabled = doc["wifiEnabled"].as<bool>();
    }
    if (doc.containsKey("loraEnabled")) {
        powerConfig.loraEnabled = doc["loraEnabled"].as<bool>();
    }
    if (doc.containsKey("sensorEnabled")) {
        powerConfig.sensorEnabled = doc["sensorEnabled"].as<bool>();
    }
    if (doc.containsKey("cpuFreq")) {
        powerConfig.cpuFreq = doc["cpuFreq"].as<uint8_t>();
    }
    if (doc.containsKey("txPower")) {
        powerConfig.txPower = doc["txPower"].as<int8_t>();
    }
    
    return true;
}

String ConfigAPI::getPinConfig() const {
    return pinConfigToJson();
}

bool ConfigAPI::setPinConfig(const String& json) {
    if (!parsePinConfig(json)) {
        return false;
    }
    return savePinConfig();
}

bool ConfigAPI::parsePinConfig(const String& json) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.println("[ConfigAPI] Failed to parse pin config JSON");
        return false;
    }
    
    if (doc.containsKey("pin") && doc.containsKey("mode")) {
        uint8_t pinNum = doc["pin"].as<uint8_t>();
        if (pinNum < 20) {
            pinConfigs[pinNum].mode = (PinMode)doc["mode"].as<uint8_t>();
            pinConfigs[pinNum].initialValue = doc.containsKey("initialValue") ? doc["initialValue"].as<int>() : 0;
        }
    }
    
    return true;
}

String ConfigAPI::getControlStrategy() const {
    return controlConfigToJson();
}

bool ConfigAPI::setControlStrategy(const String& json) {
    if (!parseControlConfig(json)) {
        return false;
    }
    return saveControlConfig();
}

bool ConfigAPI::parseControlConfig(const String& json) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.println("[ConfigAPI] Failed to parse control config JSON");
        return false;
    }
    
    if (doc.containsKey("conditionEnabled")) {
        controlConfig.conditionEnabled = doc["conditionEnabled"].as<bool>();
    }
    if (doc.containsKey("conditionLogic")) {
        controlConfig.conditionLogic = doc["conditionLogic"].as<uint8_t>();
    }
    if (doc.containsKey("timerEnabled")) {
        controlConfig.timerEnabled = doc["timerEnabled"].as<bool>();
    }
    if (doc.containsKey("timeSlotCount")) {
        controlConfig.timeSlotCount = doc["timeSlotCount"].as<uint8_t>();
    }
    if (doc.containsKey("manualModeEnabled")) {
        controlConfig.manualModeEnabled = doc["manualModeEnabled"].as<bool>();
    }
    if (doc.containsKey("relayState")) {
        controlConfig.relayState = doc["relayState"].as<uint8_t>();
    }
    
    return true;
}

String ConfigAPI::getSystemConfig() const {
    return systemConfigToJson();
}

bool ConfigAPI::setSystemConfig(const String& json) {
    if (!parseSystemConfig(json)) {
        return false;
    }
    return saveSystemConfig();
}

bool ConfigAPI::parseSystemConfig(const String& json) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.println("[ConfigAPI] Failed to parse system config JSON");
        return false;
    }
    
    if (doc.containsKey("logLevel")) {
        systemConfig.logLevel = doc["logLevel"].as<uint8_t>();
    }
    if (doc.containsKey("otaAutoCheck")) {
        systemConfig.otaAutoCheck = doc["otaAutoCheck"].as<bool>();
    }
    if (doc.containsKey("otaUrl")) {
        strncpy(systemConfig.otaUrl, doc["otaUrl"].as<const char*>(), sizeof(systemConfig.otaUrl) - 1);
    }
    if (doc.containsKey("securityEnabled")) {
        systemConfig.securityEnabled = doc["securityEnabled"].as<bool>();
    }
    if (doc.containsKey("adminPassword")) {
        strncpy(systemConfig.adminPassword, doc["adminPassword"].as<const char*>(), sizeof(systemConfig.adminPassword) - 1);
    }
    
    return true;
}

String ConfigAPI::getTransmissionMode() const {
    StaticJsonDocument<128> doc;
    doc["mode"] = networkConfig.transmissionMode;
    doc["mqttEnabled"] = networkConfig.mqttEnabled;
    doc["loraEnabled"] = networkConfig.loraEnabled;
    
    String output;
    serializeJson(doc, output);
    return output;
}

bool ConfigAPI::setTransmissionMode(const String& json) {
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        return false;
    }
    
    if (doc.containsKey("mode")) {
        networkConfig.transmissionMode = doc["mode"].as<uint8_t>();
    }
    if (doc.containsKey("mqttEnabled")) {
        networkConfig.mqttEnabled = doc["mqttEnabled"].as<bool>();
    }
    if (doc.containsKey("loraEnabled")) {
        networkConfig.loraEnabled = doc["loraEnabled"].as<bool>();
    }
    
    return saveNetworkConfig();
}

String ConfigAPI::exportAllConfig() const {
    StaticJsonDocument<2048> doc;
    
    JsonObject deviceObj = doc.createNestedObject("device");
    StaticJsonDocument<512> tempDoc;
    deserializeJson(tempDoc, deviceConfigToJson());
    for (auto kv : tempDoc.as<JsonObject>()) {
        deviceObj[kv.key()] = kv.value();
    }
    
    JsonObject networkObj = doc.createNestedObject("network");
    deserializeJson(tempDoc, networkConfigToJson());
    for (auto kv : tempDoc.as<JsonObject>()) {
        networkObj[kv.key()] = kv.value();
    }
    
    JsonObject loraObj = doc.createNestedObject("lora");
    deserializeJson(tempDoc, loraConfigToJson());
    for (auto kv : tempDoc.as<JsonObject>()) {
        loraObj[kv.key()] = kv.value();
    }
    
    JsonObject powerObj = doc.createNestedObject("power");
    deserializeJson(tempDoc, powerConfigToJson());
    for (auto kv : tempDoc.as<JsonObject>()) {
        powerObj[kv.key()] = kv.value();
    }
    
    JsonObject controlObj = doc.createNestedObject("control");
    deserializeJson(tempDoc, controlConfigToJson());
    for (auto kv : tempDoc.as<JsonObject>()) {
        controlObj[kv.key()] = kv.value();
    }
    
    JsonObject systemObj = doc.createNestedObject("system");
    deserializeJson(tempDoc, systemConfigToJson());
    for (auto kv : tempDoc.as<JsonObject>()) {
        systemObj[kv.key()] = kv.value();
    }
    
    doc["timestamp"] = millis();
    doc["version"] = "1.0";
    
    String output;
    serializeJson(doc, output);
    return output;
}

bool ConfigAPI::importAllConfig(const String& json) {
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.println("[ConfigAPI] Failed to parse import JSON");
        return false;
    }
    
    bool success = true;
    
    if (doc.containsKey("device")) {
        String deviceJson;
        serializeJson(doc["device"], deviceJson);
        success &= setDeviceConfig(deviceJson);
    }
    if (doc.containsKey("network")) {
        String networkJson;
        serializeJson(doc["network"], networkJson);
        success &= setNetworkConfig(networkJson);
    }
    if (doc.containsKey("lora")) {
        String loraJson;
        serializeJson(doc["lora"], loraJson);
        success &= setLoraParams(loraJson);
    }
    if (doc.containsKey("power")) {
        String powerJson;
        serializeJson(doc["power"], powerJson);
        success &= setPowerConfig(powerJson);
    }
    if (doc.containsKey("control")) {
        String controlJson;
        serializeJson(doc["control"], controlJson);
        success &= setControlStrategy(controlJson);
    }
    if (doc.containsKey("system")) {
        String systemJson;
        serializeJson(doc["system"], systemJson);
        success &= setSystemConfig(systemJson);
    }
    
    return success;
}

bool ConfigAPI::resetToDefaults() {
    preferences.clear();
    
    deviceConfig = DeviceConfig();
    networkConfig = NetworkConfig();
    controlConfig = ControlStrategyConfig();
    systemConfig = SystemConfig();
    loraParams = LoRaParams();
    loraAdaptiveConfig = LoRaAdaptiveConfig();
    powerConfig = PowerConfig();
    
    saveDeviceConfig();
    saveNetworkConfig();
    saveControlConfig();
    saveSystemConfig();
    saveLoraConfig();
    savePowerConfig();
    
    Serial.println("[ConfigAPI] Configuration reset to defaults");
    return true;
}

ConfigAPI configAPI;
