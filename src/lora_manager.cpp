#include "lora_manager.h"
#include "pin_config.h"

LoRaManager loraManager;

LoRaManager::LoRaManager() : queueSize(0), nodeCount(0), lastSendTime(0) {}

bool LoRaManager::begin() {
  Serial.println("🔄 正在初始化 LoRa 模块...");
  
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  LoRa.setPins(LORA_NSS, LORA_RESET, -1);
  
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("❌ LoRa 模块初始化失败！");
    return false;
  }
  
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setPreambleLength(8);
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();
  
  // 更新LoRa相关引脚的初始化状态
  PinConfig* sckConfig = pinConfigManager.getPinConfig(LORA_SCK);
  if (sckConfig) sckConfig->isInitialized = true;
  PinConfig* misoConfig = pinConfigManager.getPinConfig(LORA_MISO);
  if (misoConfig) misoConfig->isInitialized = true;
  PinConfig* mosiConfig = pinConfigManager.getPinConfig(LORA_MOSI);
  if (mosiConfig) mosiConfig->isInitialized = true;
  PinConfig* nssConfig = pinConfigManager.getPinConfig(LORA_NSS);
  if (nssConfig) nssConfig->isInitialized = true;
  PinConfig* resetConfig = pinConfigManager.getPinConfig(LORA_RESET);
  if (resetConfig) resetConfig->isInitialized = true;
  
  Serial.println("✅ LoRa 模块初始化成功！");
  return true;
}

void LoRaManager::addToQueue(const String &message, int priority) {
  if (queueSize >= LORA_QUEUE_SIZE) {
    if (queue[0].priority < priority) {
      for (int i = 0; i < queueSize - 1; i++) {
        queue[i] = queue[i + 1];
      }
      queueSize--;
    } else {
      Serial.printf("📤 LoRa 队列已满，丢弃低优先级消息\n");
      return;
    }
  }
  
  int insertIndex = queueSize;
  for (int i = 0; i < queueSize; i++) {
    if (queue[i].priority < priority) {
      insertIndex = i;
      break;
    }
  }
  
  for (int i = queueSize; i > insertIndex; i--) {
    queue[i] = queue[i - 1];
  }
  
  queue[insertIndex].message = message;
  queue[insertIndex].priority = priority;
  queueSize++;
  
  Serial.printf("📤 已添加 LoRa 消息到队列，优先级：%d\n", priority);
}

void LoRaManager::processQueue() {
  if (queueSize == 0) {
    return;
  }
  
  if (millis() - lastSendTime < LORA_SEND_INTERVAL) {
    return;
  }
  
  unsigned long loraStart = millis();
  
  String message = queue[0].message;
  lastSentMessage = message;
  
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();
  
  unsigned long loraDuration = millis() - loraStart;
  Serial.printf("📤 已发送 LoRa 消息：%s (耗时：%lu ms)\n", message.c_str(), loraDuration);
  
  if (loraDuration > 50) {
    Serial.printf("⚠️  LoRa 发送耗时过长：%lu ms\n", loraDuration);
  }
  
  lastSendTime = millis();
  
  for (int i = 0; i < queueSize - 1; i++) {
    queue[i] = queue[i + 1];
  }
  queueSize--;
}

String LoRaManager::receiveMessage() {
  unsigned long loraRxStart = millis();
  String message = "";
  int packetSize = LoRa.parsePacket();
  unsigned long loraRxDuration = millis() - loraRxStart;
  
  // 监测 LoRa parsePacket 耗时
  if (loraRxDuration > 20) {
    Serial.printf("⚠️  LoRa parsePacket 耗时过长：%lu ms\n", loraRxDuration);
  }
  
  if (packetSize) {
    while (LoRa.available()) {
      message += (char)LoRa.read();
    }
    
    if (message != lastSentMessage) {
      Serial.printf("📥 已接收 LoRa 消息：%s (耗时：%lu ms)\n", message.c_str(), loraRxDuration);
      return message;
    } else {
      Serial.println("📭 忽略自己发送的 LoRa 消息");
      return "";
    }
  }
  
  return message;
}

void LoRaManager::sendMessage(const String &message, int priority) {
  addToQueue(message, priority);
}

void LoRaManager::updateNode(const String &nodeId, bool hasNetwork, int rssi) {
  for (int i = 0; i < nodeCount; i++) {
    if (nodes[i].nodeId == nodeId) {
      nodes[i].hasNetwork = hasNetwork;
      nodes[i].lastSeen = millis();
      nodes[i].rssi = rssi;
      return;
    }
  }
  
  if (nodeCount < MAX_LORA_NODES) {
    nodes[nodeCount].nodeId = nodeId;
    nodes[nodeCount].hasNetwork = hasNetwork;
    nodes[nodeCount].lastSeen = millis();
    nodes[nodeCount].rssi = rssi;
    nodeCount++;
    Serial.printf("✅ 已添加 LoRa 节点：%s\n", nodeId.c_str());
  }
}

String LoRaManager::findBestNode() {
  String bestNodeId = "";
  int bestRssi = -128;
  bool foundNetworkNode = false;
  
  // 清理过期节点
  for (int i = nodeCount - 1; i >= 0; i--) {
    if (millis() - nodes[i].lastSeen > 180000) {
      for (int j = i; j < nodeCount - 1; j++) {
        nodes[j] = nodes[j + 1];
      }
      nodeCount--;
    }
  }
  
  for (int i = 0; i < nodeCount; i++) {
    if (nodes[i].hasNetwork) {
      if (!foundNetworkNode || nodes[i].rssi > bestRssi) {
        bestNodeId = nodes[i].nodeId;
        bestRssi = nodes[i].rssi;
        foundNetworkNode = true;
      }
    } else if (!foundNetworkNode && nodes[i].rssi > bestRssi) {
      bestNodeId = nodes[i].nodeId;
      bestRssi = nodes[i].rssi;
    }
  }
  
  return bestNodeId;
}

void LoRaManager::sendDiscovery(const String &deviceId, bool hasNetwork) {
  String discoveryMsg = String(LORA_MSG_TYPE_DISCOVERY) + "," + deviceId + "," + String(hasNetwork ? 1 : 0);
  sendMessage(discoveryMsg, LORA_PRIORITY_LOW);
  Serial.println("📤 已发送 LoRa 节点发现消息");
}

void LoRaManager::sendHeartbeat(const String &deviceId, bool hasNetwork) {
  String heartbeatMsg = String(LORA_MSG_TYPE_HEARTBEAT) + "," + deviceId + "," + String(hasNetwork ? 1 : 0);
  sendMessage(heartbeatMsg, LORA_PRIORITY_LOW);
  Serial.println("📤 已发送 LoRa 心跳包");
}

void LoRaManager::parseMessage(const String &message) {
  // 占位符，实现委托给主程序
  Serial.printf("📥 LoRa 消息解析（需在主程序实现）：%s\n", message.c_str());
}
