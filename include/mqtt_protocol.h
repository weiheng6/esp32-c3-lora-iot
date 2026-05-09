#ifndef MQTT_PROTOCOL_H
#define MQTT_PROTOCOL_H

#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "protobuf_codec.h"

enum class TransmissionMode {
  JSON_MODE,
  PROTOBUF_MODE
};

class MQTTProtocol {
private:
  WiFiClient wifiClient;
  PubSubClient mqttClient;
  char clientId[30];
  char deviceId[13];
  
  char topicJsonData[50];
  char topicJsonCmd[50];
  char topicJsonResp[50];
  char topicProtoData[50];
  char topicProtoCmd[50];
  char topicProtoResp[50];
  char topicModeSwitch[50];
  char topicWill[50];
  
  TransmissionMode currentMode;
  bool firstConnection;
  
  void generateTopics();
  
public:
  MQTTProtocol();
  void begin();
  void connect();
  void loop();
  bool isConnected();
  
  void setMode(TransmissionMode mode);
  TransmissionMode getMode() const { return currentMode; }
  bool isJsonMode() const { return currentMode == TransmissionMode::JSON_MODE; }
  bool isProtobufMode() const { return currentMode == TransmissionMode::PROTOBUF_MODE; }
  
  bool publishJson(const char* topic, const char* payload, bool retain = false);
  bool publishProtobuf(const char* topic, const uint8_t* data, size_t length, bool retain = false);
  
  void setCallback(std::function<void(char*, byte*, unsigned int)> callback);
  void subscribeHandler(char* topic, byte* payload, unsigned int length);
  
  const char* getJsonDataTopic() const { return topicJsonData; }
  const char* getProtoDataTopic() const { return topicProtoData; }
  const char* getModeSwitchTopic() const { return topicModeSwitch; }
  const char* getDeviceId() const { return deviceId; }
  
  void publishSensorDataJson(float temperature, float humidity, int32_t batteryLevel, int32_t signalStrength);
  void publishSensorDataProtobuf(float temperature, float humidity, int32_t batteryLevel, int32_t signalStrength);
};

extern MQTTProtocol mqttProtocol;

#endif
