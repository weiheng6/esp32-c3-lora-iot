#include "mqtt_protocol.h"
#include <WiFi.h>
#include <functional>
#include <Preferences.h>

ProtobufCodec protoCodec(256);
MQTTProtocol mqttProtocol;

MQTTProtocol::MQTTProtocol() 
    : mqttClient(wifiClient), currentMode(TransmissionMode::JSON_MODE), firstConnection(true) {
    memset(clientId, 0, sizeof(clientId));
    memset(deviceId, 0, sizeof(deviceId));
    memset(topicJsonData, 0, sizeof(topicJsonData));
    memset(topicJsonCmd, 0, sizeof(topicJsonCmd));
    memset(topicJsonResp, 0, sizeof(topicJsonResp));
    memset(topicProtoData, 0, sizeof(topicProtoData));
    memset(topicProtoCmd, 0, sizeof(topicProtoCmd));
    memset(topicProtoResp, 0, sizeof(topicProtoResp));
    memset(topicModeSwitch, 0, sizeof(topicModeSwitch));
    memset(topicWill, 0, sizeof(topicWill));
}

void MQTTProtocol::begin() {
    Preferences prefs;
    prefs.begin("mqtt_proto", true);
    uint8_t savedMode = prefs.getUChar("mode", 0);
    prefs.end();
    
    if (savedMode == 1) {
        currentMode = TransmissionMode::PROTOBUF_MODE;
        Serial.println("[MQTTProtocol] Loaded saved PROTOBUF mode");
    } else {
        currentMode = TransmissionMode::JSON_MODE;
        Serial.println("[MQTTProtocol] Using default JSON mode");
    }
    
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(deviceId, sizeof(deviceId), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    Serial.printf("[MQTTProtocol] Device ID: %s\n", deviceId);
    
    snprintf(clientId, sizeof(clientId), "ESP32_%s", deviceId);
    
    generateTopics();
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    
    Serial.println("[MQTTProtocol] Initialized");
}

void MQTTProtocol::generateTopics() {
    snprintf(topicJsonData, sizeof(topicJsonData), "esp32/%s/json/data", deviceId);
    snprintf(topicJsonCmd, sizeof(topicJsonCmd), "esp32/%s/json/command", deviceId);
    snprintf(topicJsonResp, sizeof(topicJsonResp), "esp32/%s/json/response", deviceId);
    
    snprintf(topicProtoData, sizeof(topicProtoData), "esp32/%s/proto/data", deviceId);
    snprintf(topicProtoCmd, sizeof(topicProtoCmd), "esp32/%s/proto/command", deviceId);
    snprintf(topicProtoResp, sizeof(topicProtoResp), "esp32/%s/proto/response", deviceId);
    
    snprintf(topicModeSwitch, sizeof(topicModeSwitch), "esp32/%s/mode", deviceId);
    snprintf(topicWill, sizeof(topicWill), "esp32/%s/status", deviceId);
}

void MQTTProtocol::connect() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    
    if (!mqttClient.connected()) {
        Serial.print("[MQTTProtocol] Connecting...");
        
        if (mqttClient.connect(clientId, MQTT_USER, MQTT_PASSWORD,
                               topicWill, MQTT_WILL_QOS, MQTT_WILL_RETAIN,
                               MQTT_WILL_MSG, true)) {
            Serial.println("OK");
            
            mqttClient.publish(topicWill, "online", MQTT_WILL_RETAIN);
            
            if (firstConnection) {
                char restartPayload[64];
                snprintf(restartPayload, sizeof(restartPayload), 
                         "{\"device_id\":\"%s\",\"restart\":true}", deviceId);
                mqttClient.publish(topicJsonResp, restartPayload, true);
                firstConnection = false;
            }
            
            mqttClient.subscribe(topicJsonCmd);
            mqttClient.subscribe(topicProtoCmd);
            mqttClient.subscribe(topicModeSwitch);
            
            Serial.printf("[MQTTProtocol] Mode: %s\n", 
                         isJsonMode() ? "JSON" : "Protobuf");
        } else {
            Serial.printf("FAILED (code: %d)\n", mqttClient.state());
        }
    }
}

void MQTTProtocol::loop() {
    if (mqttClient.connected()) {
        mqttClient.loop();
    }
}

bool MQTTProtocol::isConnected() {
    return mqttClient.connected();
}

void MQTTProtocol::setMode(TransmissionMode mode) {
    if (currentMode == mode) {
        return;
    }
    
    TransmissionMode oldMode = currentMode;
    currentMode = mode;
    
    Preferences prefs;
    prefs.begin("mqtt_proto", false);
    prefs.putUChar("mode", mode == TransmissionMode::PROTOBUF_MODE ? 1 : 0);
    prefs.end();
    
    Serial.printf("[MQTTProtocol] Mode switch: %s -> %s (saved to flash)\n",
                 oldMode == TransmissionMode::JSON_MODE ? "JSON" : "Protobuf",
                 mode == TransmissionMode::JSON_MODE ? "JSON" : "Protobuf");
    
    StaticJsonDocument<100> doc;
    doc["device_id"] = deviceId;
    doc["old_mode"] = (oldMode == TransmissionMode::JSON_MODE) ? "json" : "protobuf";
    doc["new_mode"] = (mode == TransmissionMode::JSON_MODE) ? "json" : "protobuf";
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(topicModeSwitch, payload.c_str(), true);
}

bool MQTTProtocol::publishJson(const char* topic, const char* payload, bool retain) {
    if (!mqttClient.connected()) {
        return false;
    }
    
    return mqttClient.publish(topic, payload, retain);
}

bool MQTTProtocol::publishProtobuf(const char* topic, const uint8_t* data, size_t length, bool retain) {
    if (!mqttClient.connected() || !data || length == 0) {
        return false;
    }
    
    return mqttClient.publish(topic, data, length, retain);
}

void MQTTProtocol::setCallback(std::function<void(char*, byte*, unsigned int)> callback) {
    mqttClient.setCallback(callback);
}

void MQTTProtocol::subscribeHandler(char* topic, byte* payload, unsigned int length) {
    if (!topic || !payload || length == 0) {
        return;
    }
    
    char topicStr[64];
    strncpy(topicStr, topic, sizeof(topicStr) - 1);
    topicStr[sizeof(topicStr) - 1] = '\0';
    
    if (strstr(topicStr, "/mode")) {
        StaticJsonDocument<100> doc;
        DeserializationError error = deserializeJson(doc, payload, length);
        if (!error && doc.containsKey("mode")) {
            const char* mode = doc["mode"];
            if (strcmp(mode, "json") == 0) {
                setMode(TransmissionMode::JSON_MODE);
            } else if (strcmp(mode, "protobuf") == 0) {
                setMode(TransmissionMode::PROTOBUF_MODE);
            }
        }
        return;
    }
    
    if (strstr(topicStr, "/proto/command")) {
        char cmdType[32] = {0};
        char cmdParams[128] = {0};
        if (protoCodec.decodeCommand(cmdType, sizeof(cmdType), 
                                     cmdParams, sizeof(cmdParams),
                                     payload, length)) {
            Serial.printf("[MQTTProtocol] Protobuf cmd: type=%s\n", cmdType);
        }
        return;
    }
    
    if (strstr(topicStr, "/json/command")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, payload, length);
        if (!error) {
            Serial.printf("[MQTTProtocol] JSON cmd received\n");
        }
        return;
    }
}

void MQTTProtocol::publishSensorDataJson(float temperature, float humidity, 
                                        int32_t batteryLevel, int32_t signalStrength) {
    StaticJsonDocument<256> doc;
    doc["device_id"] = deviceId;
    doc["timestamp"] = millis();
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["battery"] = batteryLevel;
    doc["rssi"] = signalStrength;
    
    String payload;
    serializeJson(doc, payload);
    
    if (publishJson(topicJsonData, payload.c_str())) {
        Serial.println("[MQTTProtocol] JSON data published");
    }
}

void MQTTProtocol::publishSensorDataProtobuf(float temperature, float humidity,
                                            int32_t batteryLevel, int32_t signalStrength) {
    protoCodec.reset();
    
    if (protoCodec.encodeSensorData(deviceId, millis(), 
                                    temperature, humidity,
                                    batteryLevel, signalStrength)) {
        if (publishProtobuf(topicProtoData, 
                            protoCodec.getBuffer(), 
                            protoCodec.getEncodedSize())) {
            Serial.printf("[MQTTProtocol] Protobuf data published (%d bytes)\n", 
                         protoCodec.getEncodedSize());
        }
    }
}
