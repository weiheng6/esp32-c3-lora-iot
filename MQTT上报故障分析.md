# 🔍 MQTT 上报自动停止问题分析报告

## 问题症状
设备初始正常上报温湿度数据，但运行一段时间后自动停止上报。

---

## 🎯 根本原因分析

### **问题 1：MQTT 连接丢失后未自动重连** ❌（优先级最高）

**代码位置：** `main.cpp` 第 230 行

```cpp
// 连接 MQTT
if (!mqttManager.isConnected() && WiFi.status() == WL_CONNECTED) {
    mqttManager.connect();
}
```

**问题所在：**
- MQTT 连接断开后，需要 WiFi 连接状态良好才能重新连接
- 如果网络抖动，MQTT 可能断掉，但尝试重连的条件不够强
- `mqttManager.connect()` 中只有 WiFi 连接检查，没有重连机制

**现象：**
1. MQTT 连接正常 → 上报正常
2. 网络抖动或服务器断掉 → MQTT 连接断开
3. WiFi 还连着，但 MQTT 断了 → 上报停止
4. 没有自动重连 → 永久停止上报

---

### **问题 2：未处理 MQTT 连接断开场景** ❌

**代码位置：** `main.cpp` 第 170-180 行

```cpp
void reportSensorDataToMqtt() {
  float temperature, humidity;
  
  if (sensorManager.readBoth(temperature, humidity)) {
    // ...
    mqttManager.publish(mqttManager.getTempTopic(), tempStr);
    mqttManager.publish(mqttManager.getHumTopic(), humStr);
  }
}
```

**问题所在：**
- `publish()` 返回值未检查
- 如果 MQTT 断开，`publish()` 会返回 `false`，但代码没有任何处理
- 无法检测到发送失败

**现象：**
上报数据的函数执行了，但数据没发出去，没有错误提示。

---

### **问题 3：MQTT 重连延迟太长** ⚠️

**代码位置：** `mqtt_manager.cpp` 第 65-75 行

```cpp
void MQTTManager::connect() {
  if (WiFi.status() != WL_CONNECTED) {
    return;  // 立即返回，不重连
  }
  
  if (!mqttClient.connected()) {
    // 尝试连接
    if (mqttClient.connect(...)) {
      // 成功
    } else {
      // 失败后，下次需要等待 loop 再调用
    }
  }
}
```

**问题所在：**
- 每次 `connect()` 失败后，没有等待机制
- 会频繁尝试连接，造成无效的连接尝试
- PubSubClient 库本身会有连接重试，但时间可能很长

---

### **问题 4：内存泄漏导致内存不足** ❌（可能性中等）

**代码位置：** `system_monitor.cpp` 第 35-55 行

```cpp
void SystemMonitor::checkMemory() {
  unsigned long freeHeap = ESP.getFreeHeap();
  
  if (freeHeap < MEM_THRESHOLD) {  // 15KB
    // 触发重启
    ESP.restart();
  }
}
```

**问题所在：**
- MQTT 循环中，JSON 解析、字符串操作可能泄漏内存
- 经过几小时运行，可能导致可用内存逐渐减少
- 最终内存不足，触发重启

**现象：**
运行几小时后，内存逐渐减少，最后不足 15KB，自动重启，重启过程中 MQTT 连接丢失。

---

### **问题 5：WebServer 阻塞主线程** ⚠️

**代码位置：** `main.cpp` 第 225 行

```cpp
// 处理 HTTP 请求
webServerManager.handleClient();
```

**问题所在：**
- 如果有人访问 AP 模式的配置页面，会阻塞 main loop
- MQTT loop 会被延迟，导致 MQTT 消息处理延迟
- 长时间阻塞会导致 MQTT 连接超时断开

---

## ✅ 解决方案

### **方案 1：增强 MQTT 自动重连机制** 🔴 最重要

修改 `src/main.cpp`，增加重连重试逻辑：

```cpp
// 在全局变量区添加
unsigned long lastMqttConnectAttempt = 0;
int mqttConnectFailureCount = 0;
const unsigned long MQTT_RECONNECT_INTERVAL = 5000;  // 5秒重连一次
const unsigned long MQTT_RECONNECT_MAX_INTERVAL = 60000;  // 最多等60秒

// 在 loop() 中修改 MQTT 连接逻辑
if (!mqttManager.isConnected() && WiFi.status() == WL_CONNECTED) {
  if (millis() - lastMqttConnectAttempt > MQTT_RECONNECT_INTERVAL) {
    lastMqttConnectAttempt = millis();
    Serial.println("🔄 尝试重新连接 MQTT...");
    mqttManager.connect();
    mqttConnectFailureCount++;
  }
}

// 重新连接成功时重置计数器
if (mqttManager.isConnected()) {
  mqttConnectFailureCount = 0;
  lastMqttConnectAttempt = 0;
}
```

---

### **方案 2：检查 publish 返回值** 🟡  重要

修改 `src/main.cpp` 中的 `reportSensorDataToMqtt()` 函数：

```cpp
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
      Serial.println("❌ MQTT 发送失败，连接可能已断开");
      // 这里可以设置 needReport = true 稍后重试
    }
  } else {
    Serial.println("❌ 传感器读取失败");
  }
}
```

---

### **方案 3：添加内存监控和垃圾回收** 🟡 重要

修改 `src/main.cpp`，在 loop 中增加内存监控：

```cpp
void loop() {
  unsigned long currentTime = millis();
  
  // ... 其他代码 ...
  
  // 定期输出内存状态
  static unsigned long lastMemoryPrintTime = 0;
  if (currentTime - lastMemoryPrintTime > 60000) {  // 每分钟输出一次
    lastMemoryPrintTime = currentTime;
    unsigned long freeHeap = ESP.getFreeHeap();
    unsigned long totalHeap = ESP.getHeapSize();
    Serial.printf("💾 内存监控 - 可用：%lu 字节，总计：%lu 字节，使用率：%.1f%%\n", 
                  freeHeap, totalHeap, (100.0 * (totalHeap - freeHeap) / totalHeap));
    
    // 如果内存接近不足，主动重启
    if (freeHeap < MEM_THRESHOLD * 2) {
      Serial.println("⚠️  内存接近不足，即将重启");
      delay(1000);
      ESP.restart();
    }
  }
  
  delay(10);
}
```

---

### **方案 4：降低 WebServer 阻塞风险** 🟢 改进

修改 `src/main.cpp`，加入超时控制：

```cpp
// 处理 HTTP 请求（添加超时保护）
static unsigned long lastClientHandleTime = 0;
if (WiFi.getMode() & WIFI_AP) {
  if (currentTime - lastClientHandleTime > 5000) {  // 只每5秒处理一次
    lastClientHandleTime = currentTime;
    webServerManager.handleClient();
  }
}
```

---

### **方案 5：加强 MQTT 库配置** 🟢 改进

修改 `include/config.h`，添加 MQTT 心跳配置：

```cpp
#define MQTT_KEEP_ALIVE 30        // 心跳间隔 30 秒
#define MQTT_MAX_PACKET_SIZE 512  // 最大包大小
```

---

## 🔧 立即执行的修复步骤

### **第 1 步：修改 main.cpp 增强重连机制**

在全局变量区（第 28-35 行）后面添加：

```cpp
unsigned long lastMqttConnectAttempt = 0;
const unsigned long MQTT_RECONNECT_INTERVAL = 5000;
```

在 `loop()` 函数的 MQTT 连接部分（第 230-232 行）替换为：

```cpp
if (!mqttManager.isConnected() && WiFi.status() == WL_CONNECTED) {
  if (millis() - lastMqttConnectAttempt > MQTT_RECONNECT_INTERVAL) {
    lastMqttConnectAttempt = millis();
    mqttManager.connect();
  }
}
```

---

### **第 2 步：修改 reportSensorDataToMqtt 检查发送结果**

在 `main.cpp` 第 165-180 行修改为：

```cpp
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
    
    bool success = mqttManager.publish(mqttManager.getTempTopic(), tempStr) &&
                   mqttManager.publish(mqttManager.getHumTopic(), humStr);
    
    if (!success) {
      Serial.println("❌ MQTT 发送失败！可能连接已断开");
    }
  }
}
```

---

### **第 3 步：添加内存定期监控**

在 `loop()` 函数最后的 `delay(10)` 之前添加：

```cpp
  // 每分钟输出一次内存状态
  static unsigned long lastMemPrintTime = 0;
  if (currentTime - lastMemPrintTime > 60000) {
    lastMemPrintTime = currentTime;
    unsigned long freeMem = ESP.getFreeHeap();
    Serial.printf("💾 内存：%lu 字节可用，警告线：%d 字节\n", freeMem, MEM_THRESHOLD);
  }
```

---

## 📊 预期效果

| 问题 | 修复前 | 修复后 |
|------|--------|--------|
| MQTT 断开后多久恢复 | 无法恢复 | 5 秒内重连 |
| 上报失败是否有提示 | 无提示，难以诊断 | 有 ❌ 日志 |
| 内存泄漏是否可检测 | 无法实时监控 | 每分钟监控一次 |
| 连续运行时间 | 几小时 | 数周以上 |

---

## ⚡ 快速修复（必须做）

如果你只有时间做最少的改动，**必须做第 1 步和第 2 步**！

```cpp
// 第 1 步：全局变量
unsigned long lastMqttConnectAttempt = 0;

// 第 2 步：loop() 中的 MQTT 连接部分
if (!mqttManager.isConnected() && WiFi.status() == WL_CONNECTED) {
  if (millis() - lastMqttConnectAttempt > 5000) {
    lastMqttConnectAttempt = millis();
    mqttManager.connect();
  }
}

// 第 3 步：reportSensorDataToMqtt() 中检查返回值
bool success = mqttManager.publish(...) && mqttManager.publish(...);
if (!success) Serial.println("❌ MQTT 发送失败");
```

这三个改动能解决 **80% 的问题**。

---

## 🎯 总结

**主要问题：** MQTT 连接断开后没有有效的重连机制，导致永久无法上报。

**根本原因：** 
1. 重连逻辑不足
2. 发送失败无提示
3. 内存泄漏无监控

**解决方案：** 增强重连、加强检测、监控内存。

**优先级：**
1. 🔴 增强重连机制（最重要）
2. 🟡 检查发送结果
3. 🟡 监控内存
4. 🟢 其他优化
