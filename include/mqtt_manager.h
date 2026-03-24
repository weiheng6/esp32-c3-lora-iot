#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFi.h>
#include "config.h"

class MQTTManager {
private:
  WiFiClient wifiClient;
  PubSubClient mqttClient;
  char clientId[30];
  char deviceId[13];
  char topicTemp[50];
  char topicHum[50];
  char topicCmd[50];
  char topicResp[50];
  char topicWill[50];
  char topicMemory[50];
  char topicRestart[50];
  char topicMgmtInfo[50];  // 新增：管理界面信息主题
  bool firstConnection;

  void generateTopics();

public:
  MQTTManager();
  void begin();
  void connect();
  void loop();
  bool isConnected() const;
  bool publish(const char* topic, const char* payload, bool retain = false);
  void setCallback(std::function<void(char*, byte*, unsigned int)> callback);
  
  // 发送管理界面地址到 MQTT
  void publishManagementInfo(const char* ipAddress);
  
  // 获取主题
  const char* getTempTopic() const { return topicTemp; }
  const char* getHumTopic() const { return topicHum; }
  const char* getCmdTopic() const { return topicCmd; }
  const char* getRespTopic() const { return topicResp; }
  const char* getWillTopic() const { return topicWill; }
  const char* getMemoryTopic() const { return topicMemory; }
  const char* getRestartTopic() const { return topicRestart; }
  const char* getMgmtInfoTopic() const { return topicMgmtInfo; }
  const char* getDeviceId() const { return deviceId; }
};

extern MQTTManager mqttManager;

#endif // MQTT_MANAGER_H
