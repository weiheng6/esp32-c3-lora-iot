#ifndef LORA_MANAGER_H
#define LORA_MANAGER_H

#include <LoRa.h>
#include <SPI.h>
#include "config.h"

typedef struct {
  String message;
  int priority;
} LoRaMessage;

typedef struct {
  String nodeId;
  bool hasNetwork;
  unsigned long lastSeen;
  int rssi;
} LoRaNode;

class LoRaManager {
private:
  LoRaMessage queue[LORA_QUEUE_SIZE];
  int queueSize;
  LoRaNode nodes[MAX_LORA_NODES];
  int nodeCount;
  String lastSentMessage;
  unsigned long lastSendTime;

public:
  LoRaManager();
  bool begin();
  void sendMessage(const String &message, int priority = LORA_PRIORITY_MEDIUM);
  void addToQueue(const String &message, int priority);
  void processQueue();
  String receiveMessage();
  void parseMessage(const String &message);
  
  // 节点管理
  void updateNode(const String &nodeId, bool hasNetwork, int rssi);
  String findBestNode();
  void sendDiscovery(const String &deviceId, bool hasNetwork);
  void sendHeartbeat(const String &deviceId, bool hasNetwork);
  
  // 获取队列状态
  int getQueueSize() const { return queueSize; }
  int getNodeCount() const { return nodeCount; }
};

extern LoRaManager loraManager;

#endif // LORA_MANAGER_H
