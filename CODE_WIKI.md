# ESP32-C3 智能温湿度监控系统 - Code Wiki

## 目录

1. [项目概述](#1-项目概述)
2. [项目架构](#2-项目架构)
3. [模块详解](#3-模块详解)
4. [关键类与函数](#4-关键类与函数)
5. [依赖关系](#5-依赖关系)
6. [配置说明](#6-配置说明)
7. [运行方式](#7-运行方式)
8. [MQTT通信协议](#8-mqtt通信协议)
9. [Web接口说明](#9-web接口说明)

---

## 1. 项目概述

### 1.1 项目简介

ESP32-C3智能温湿度监控系统是一个基于ESP32-C3微控制器的物联网应用项目，支持温湿度采集、MQTT云通信、LoRa远距离通信、条件控制、定时控制和远程固件升级功能。

### 1.2 硬件平台

| 组件 | 型号/规格 |
|------|----------|
| 主控芯片 | ESP32-C3 (Adafruit QT Py ESP32-C3) |
| 温湿度传感器 | SHT31 (I2C接口) |
| 继电器模块 | 单路继电器 |
| LoRa模块 | SX1276 (可选) |

### 1.3 软件平台

| 项目 | 版本/说明 |
|------|----------|
| 开发框架 | Arduino ESP32 |
| 构建工具 | PlatformIO |
| MQTT库 | PubSubClient 2.8 |
| JSON库 | ArduinoJson 6.21.2 |

---

## 2. 项目架构

### 2.1 系统架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                        ESP32-C3 系统                            │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │
│  │  传感器层   │  │  通信层     │  │  控制层     │             │
│  ├─────────────┤  ├─────────────┤  ├─────────────┤             │
│  │ SensorManager│  │WiFiManager │  │RelayControl │             │
│  │ SHT31 I2C   │  │MQTTManager │  │ConditionCtrl│             │
│  │             │  │LoRaManager │  │TimerControl │             │
│  └─────────────┘  └─────────────┘  └─────────────┘             │
│         │                │                │                     │
│         └────────────────┼────────────────┘                     │
│                          │                                      │
│                   ┌──────┴──────┐                               │
│                   │   main.cpp  │                               │
│                   │  (主循环)   │                               │
│                   └─────────────┘                               │
│                          │                                       │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │                      服务层                                 │ │
│  │  WebServer │ WebUI │ OTAManager │ SystemMonitor │ NTPClient │ │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 目录结构

```
esp32-c3/
├── include/                      # 头文件目录
│   ├── config.h                 # 全局配置参数
│   ├── sensor.h                 # 传感器管理接口
│   ├── wifi_manager.h           # WiFi管理接口
│   ├── mqtt_manager.h           # MQTT管理接口
│   ├── lora_manager.h           # LoRa管理接口
│   ├── relay_control.h          # 继电器控制接口
│   ├── condition_control.h      # 条件控制接口
│   ├── web_server.h             # Web服务器接口
│   ├── web_ui.h                 # Web用户界面接口
│   ├── ota_manager.h            # OTA升级接口
│   ├── system_monitor.h         # 系统监控接口
│   ├── error_recovery.h         # 故障恢复接口
│   ├── log_manager.h            # 日志管理接口
│   ├── ntp_client.h             # NTP时间同步接口
│   └── Adafruit_SHT31.h         # SHT31传感器驱动
├── src/                          # 源代码目录
│   ├── main.cpp                 # 主程序入口
│   ├── sensor.cpp               # 传感器实现
│   ├── wifi_manager.cpp         # WiFi管理实现
│   ├── mqtt_manager.cpp         # MQTT管理实现
│   ├── lora_manager.cpp         # LoRa管理实现
│   ├── relay_control.cpp        # 继电器控制实现
│   ├── condition_control.cpp    # 条件控制实现
│   ├── web_server.cpp           # Web服务器实现
│   ├── web_ui.cpp              # Web用户界面实现
│   ├── ota_manager.cpp         # OTA升级实现
│   ├── system_monitor.cpp       # 系统监控实现
│   ├── error_recovery.cpp       # 故障恢复实现
│   ├── log_manager.cpp          # 日志管理实现
│   ├── ntp_client.cpp           # NTP时间同步实现
│   └── Adafruit_SHT31.cpp       # SHT31传感器驱动
├── docs/                         # 文档目录
├── test/                         # 测试目录
└── platformio.ini              # PlatformIO配置文件
```

---

## 3. 模块详解

### 3.1 传感器模块 (SensorManager)

**职责**：管理SHT31温湿度传感器，负责数据的采集和读取。

**主要功能**：
- 初始化I2C通信
- 读取温度值
- 读取湿度值
- 同时读取温湿度数据

**引脚配置**：
| 功能 | GPIO |
|------|------|
| SDA | GPIO8 |
| SCL | GPIO9 |
| I2C地址 | 0x44 |

### 3.2 WiFi管理模块 (WiFiManager)

**职责**：管理WiFi连接，支持STA和AP两种模式。

**主要功能**：
- 从持久存储加载WiFi配置
- 连接指定的WiFi网络
- 启动AP热点模式（用于配置）
- NTP时间同步

**配置存储**：
- 使用Preferences库存储SSID和密码
- 支持WiFi配置重置

### 3.3 MQTT通信模块 (MQTTManager)

**职责**：管理与MQTT服务器的通信，处理消息的发布和订阅。

**主要功能**：
- MQTT连接管理
- 消息发布（温度、湿度、状态）
- 消息订阅（接收控制指令）
- 遗嘱消息（设备离线通知）

**MQTT主题结构**：
| 主题 | 方向 | 用途 |
|------|------|------|
| esp32/{device_id}/data/temp | 上行 | 温度数据 |
| esp32/{device_id}/data/hum | 上行 | 湿度数据 |
| esp32/{device_id}/command | 下行 | 控制指令 |
| esp32/{device_id}/response | 上行 | 指令响应 |

### 3.4 LoRa通信模块 (LoRaManager)

**职责**：管理LoRa无线通信，支持远距离节点通信。

**主要功能**：
- LoRa模块初始化
- 消息发送和接收
- 消息队列管理
- 节点发现和心跳
- 多节点管理

**引脚配置**：
| 功能 | GPIO |
|------|------|
| SCK | GPIO4 |
| MISO | GPIO5 |
| MOSI | GPIO6 |
| NSS | GPIO7 |
| RESET | GPIO10 |
| 频率 | 433MHz |

### 3.5 继电器控制模块 (RelayControl)

**职责**：控制继电器的开关状态。

**主要功能**：
- 继电器开关控制
- 状态持久化存储
- 状态查询

**引脚配置**：
| 功能 | GPIO |
|------|------|
| 继电器控制 | GPIO20 |

### 3.6 条件控制模块 (ConditionControl)

**职责**：实现基于温湿度的自动控制和定时控制功能。

**主要功能**：
- 温度条件控制（单阈值/滞回模式）
- 湿度条件控制（单阈值/滞回模式）
- 条件逻辑组合（AND/OR）
- 定时控制（最多8个时间段）
- 条件组管理

**支持的比较操作**：
| 操作符 | 说明 |
|--------|------|
| > | 大于 |
| < | 小于 |
| = | 等于 |
| >= | 大于等于 |
| <= | 小于等于 |
| != | 不等于 |

### 3.7 Web服务器模块 (WebServerManager)

**职责**：提供内嵌的Web配置界面，用于WiFi配置。

**主要功能**：
- WiFi网络扫描
- WiFi凭据配置
- 网络状态显示

**访问方式**：
- 连接设备热点后访问 http://192.168.4.1

### 3.8 Web用户界面模块 (WebUIManager)

**职责**：提供完整的Web管理界面。

**主要功能**：
- 状态监控面板
- 继电器控制
- 条件控制配置
- 定时控制配置
- OTA固件升级
- 配置导入导出

### 3.9 OTA升级模块 (OTAManager)

**职责**：管理固件远程升级功能。

**升级方式**：
| 方式 | 说明 |
|------|------|
| Web URL升级 | 从HTTP服务器下载固件 |
| Web文件上传 | 本地上传固件文件 |
| MQTT远程升级 | 通过MQTT下发升级指令 |

### 3.10 系统监控模块 (SystemMonitor)

**职责**：监控系统运行状态和资源使用。

**监控内容**：
- 内存使用情况
- 系统运行统计
- 异常检测

### 3.11 故障恢复模块 (ErrorRecoveryManager)

**职责**：检测和恢复系统故障，保障稳定性。

**主要功能**：
- 重启次数统计
- I2C故障计数
- MQTT故障计数
- 自动恢复机制

### 3.12 日志管理模块 (LogManager)

**职责**：统一管理系统日志输出。

**日志级别**：
| 级别 | 说明 |
|------|------|
| LOG_NONE | 不打印日志 |
| LOG_ERROR | 仅打印错误 |
| LOG_INFO | 打印信息和错误 |
| LOG_DEBUG | 打印所有日志 |

### 3.13 NTP客户端模块 (NTPClient)

**职责**：从NTP服务器同步时间。

**默认配置**：
| 参数 | 值 |
|------|-----|
| 服务器 | ntp.aliyun.com |
| 端口 | 123 |
| 时区偏移 | UTC+8 |

---

## 4. 关键类与函数

### 4.1 SensorManager 类

```cpp
class SensorManager {
public:
    SensorManager();                                    // 构造函数
    bool begin();                                       // 初始化传感器
    bool readTemperature(float &temperature);          // 读取温度
    bool readHumidity(float &humidity);                // 读取湿度
    bool readBoth(float &temperature, float &humidity); // 同时读取温湿度
    bool isInitialized() const;                        // 检查初始化状态
};
```

### 4.2 WiFiManager 类

```cpp
class WiFiManager {
public:
    WiFiManager();
    void begin();                                      // 初始化WiFi
    void connect();                                    // 连接WiFi
    void startAPMode();                                // 启动AP模式
    bool isConnected() const;                          // 检查连接状态
    bool isConfigured() const;                         // 检查是否已配置
    void setCredentials(const char* ssid, const char* password); // 设置凭据
    void resetConfig();                                // 重置配置
};
```

### 4.3 MQTTManager 类

```cpp
class MQTTManager {
public:
    MQTTManager();
    void begin();                                      // 初始化MQTT
    void connect();                                    // 连接服务器
    void loop();                                       // 处理消息循环
    bool isConnected() const;                          // 检查连接状态
    bool publish(const char* topic, const char* payload, bool retain = false); // 发布消息
    void setCallback(std::function<void(char*, byte*, unsigned int)> callback); // 设置回调
    
    // 主题获取
    const char* getTempTopic() const;
    const char* getHumTopic() const;
    const char* getCmdTopic() const;
    const char* getRespTopic() const;
    const char* getDeviceId() const;
};
```

### 4.4 LoRaManager 类

```cpp
class LoRaManager {
public:
    LoRaManager();
    bool begin();                                      // 初始化LoRa
    void sendMessage(const String &message, int priority = MEDIUM); // 发送消息
    void addToQueue(const String &message, int priority); // 添加到队列
    void processQueue();                               // 处理队列
    String receiveMessage();                          // 接收消息
    void parseMessage(const String &message);         // 解析消息
    void sendDiscovery(const String &deviceId, bool hasNetwork); // 发送发现
    void sendHeartbeat(const String &deviceId, bool hasNetwork); // 发送心跳
};
```

### 4.5 RelayControl 类

```cpp
class RelayControl {
public:
    RelayControl();
    void begin();                                      // 初始化继电器
    void turnOn();                                     // 开启继电器
    void turnOff();                                    // 关闭继电器
    void setState(bool newState);                      // 设置状态
    bool getState() const;                            // 获取状态
    void saveState();                                 // 保存状态
    void loadState();                                 // 加载状态
    void toggle();                                     // 切换状态
};
```

### 4.6 ConditionControl 类

```cpp
class ConditionControl {
public:
    ConditionControl();
    void begin();
    
    // 条件控制
    void setEnabled(bool value);
    void setCondition(bool isOnGroup, uint8_t condIndex, bool enabled, 
                      uint8_t sensorType, uint8_t compareOp, float threshold);
    void setConditionGroupLogic(bool isOnGroup, uint8_t logicMode);
    void setTempCondition(bool enabled, float threshold, int compareOp);
    void setHumiCondition(bool enabled, float threshold, int compareOp);
    void setHysteresis(bool enabled, float tempHigh, float tempLow, 
                       float humiHigh, float humiLow);
    
    // 定时控制
    void setTimerEnabled(bool value);
    void setTimeSlot(uint8_t index, bool enabled, uint8_t startH, 
                     uint8_t startM, uint8_t endH, uint8_t endM, bool state);
    void clearTimeSlots();
    
    // 条件检查
    bool checkConditions(float temperature, float humidity);
    bool checkTimer();
    bool checkAllConditions(float temperature, float humidity);
    
    // JSON序列化
    String toJSON() const;
    String getTimerJSON() const;
};
```

### 4.7 WebUIManager 类

```cpp
class WebUIManager {
public:
    WebUIManager(uint16_t port = 80);
    void begin();
    void handleClient();
    bool isRunning() const;
};
```

### 4.8 OTAManager 类

```cpp
class OTAManager {
public:
    OTAManager();
    void begin(const char* deviceName = "esp32-c3");
    void handle();
    bool updateFromHTTP(const String& url);           // HTTP OTA升级
    int getStatus() const;                            // 获取升级状态
    int getProgress() const;                          // 获取升级进度
    bool isRunning() const;
};
```

### 4.9 SystemMonitor 类

```cpp
class SystemMonitor {
public:
    SystemMonitor();
    void printStats();                                // 打印系统统计
    void reportMemory();                              // 上报内存状态
    void checkMemory();                               // 检查内存
    bool isMemoryLow();                               // 检查内存是否过低
};
```

### 4.10 ErrorRecoveryManager 类

```cpp
class ErrorRecoveryManager {
public:
    void begin();
    void recordReboot();                              // 记录重启
    void recordI2CFailure();                         // 记录I2C故障
    void recordMQTTFailure();                         // 记录MQTT故障
    void checkAndRecover();                           // 检查并恢复
    void printStats();                                // 打印统计
    uint32_t getRebootCount();
};
```

---

## 5. 依赖关系

### 5.1 外部依赖

| 库名 | 版本 | 用途 |
|------|------|------|
| PubSubClient | 2.8 | MQTT通信 |
| ArduinoJson | 6.21.2 | JSON解析 |
| LoRa | - | LoRa无线通信 |
| ArduinoOTA | - | OTA远程升级 |

### 5.2 模块依赖关系图

```
main.cpp
├── config.h
├── sensor.h ──────────────► Adafruit_SHT31.h
├── wifi_manager.h ────────► Preferences.h, WiFi.h
├── mqtt_manager.h ────────► PubSubClient.h, WiFiClient.h
├── lora_manager.h ────────► LoRa.h, SPI.h
├── relay_control.h ───────► Preferences.h
├── condition_control.h ───► Preferences.h
├── web_server.h ──────────► WebServer.h, WiFi.h
├── web_ui.h ──────────────► WebServer.h
├── ota_manager.h
├── system_monitor.h
├── error_recovery.h ──────► Preferences.h
├── log_manager.h
└── ntp_client.h ──────────► WiFiUdp.h
```

### 5.3 数据流向

```
传感器(SHT31)
    │
    ▼
SensorManager::readBoth()
    │
    ├──► 本地条件检查 ──► RelayControl ──► 继电器输出
    │
    ├──► MQTTManager::publish() ──► MQTT服务器 ──► 云端
    │
    └──► LoRaManager::sendMessage() ──► LoRa无线 ──► 远端节点

外部指令(MQTT/Web)
    │
    ▼
mqttCallback / WebUI handlers
    │
    ├──► RelayControl ──► 继电器输出
    │
    ├──► ConditionControl ──► 自动控制逻辑
    │
    └──► OTAManager ──► 固件升级
```

---

## 6. 配置说明

### 6.1 config.h 主要配置项

```cpp
// 调试配置
#define DEBUG_MODE 0

// 硬件引脚
#define RELAY_PIN 20
#define SHT31_I2C_SDA 8
#define SHT31_I2C_SCL 9
#define SHT31_I2C_ADDRESS 0x44

// LoRa引脚
#define LORA_SCK 4
#define LORA_MISO 5
#define LORA_MOSI 6
#define LORA_NSS 7
#define LORA_RESET 10
#define LORA_FREQUENCY 433E6

// 时间间隔(ms)
#define DEFAULT_ACQUISITION_INTERVAL 1000
#define DEFAULT_MQTT_REPORT_INTERVAL 1000
#define WIFI_CHECK_INTERVAL 30000
#define SYSTEM_STATS_INTERVAL 10000
#define LORA_NODE_DISCOVERY_INTERVAL 30000

// 内存配置
#define MEM_THRESHOLD (15 * 1024)
#define LORA_QUEUE_SIZE 10
#define MAX_LORA_NODES 10
```

### 6.2 main.cpp 网络配置

```cpp
// MQTT服务器配置
const char* MQTT_SERVER = "iot.kebaidata.com";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "kbuser";
const char* MQTT_PASSWORD = "kbuserspasswd";

// AP热点配置
const char* AP_SSID = "Scott_Device";
const char* AP_PASSWORD = "88888888";
```

### 6.3 platformio.ini 构建配置

```ini
[env:adafruit_qtpy_esp32c3]
platform = espressif32
board = adafruit_qtpy_esp32c3
framework = arduino

lib_deps =
    PubSubClient@2.8
    ArduinoJson@6.21.2
    LoRa
    ArduinoOTA

build_flags =
    -Wall
    -Wextra
    -O2

monitor_speed = 115200
```

---

## 7. 运行方式

### 7.1 开发环境搭建

1. 安装VS Code
2. 安装PlatformIO插件
3. 克隆项目代码
4. 使用PlatformIO打开项目

### 7.2 编译固件

```bash
# 编译固件
platformio run

# 上传固件
platformio run --target upload

# 监控串口输出
platformio device monitor --port COMx --baud 115200
```

### 7.3 启动流程

```
系统启动
    │
    ▼
1. 初始化日志系统 (LogManager::init)
    │
    ▼
2. 加载持久配置 (Preferences)
    │
    ▼
3. 初始化传感器 (sensorManager.begin)
    │
    ▼
4. 初始化WiFi (wifiManager.begin)
    │
    ├──► WiFi已配置 ──► 连接WiFi ──► 启动Web服务器
    │
    └──► WiFi未配置 ──► 启动AP模式 ──► 等待配置
    │
    ▼
5. 初始化NTP客户端 (ntpClient.begin)
    │
    ▼
6. 初始化MQTT (mqttManager.begin)
    │
    ▼
7. 初始化LoRa (loraManager.begin)
    │
    ▼
8. 初始化继电器 (relayControl.begin)
    │
    ▼
9. 初始化条件控制 (conditionControl.begin)
    │
    ▼
10. 初始化系统监控 (systemMonitor)
    │
    ▼
11. 初始化故障恢复 (errorRecovery.begin)
    │
    ▼
12. 初始化OTA (otaManager.begin)
    │
    ▼
13. 启动Web UI (webUIManager.begin)
    │
    ▼
进入主循环 (loop)
```

### 7.4 主循环执行流程

```cpp
void loop() {
    // 1. 检查网络连接状态
    checkNetworkConnection();
    
    // 2. 定期检查WiFi连接
    if (需要检查WiFi) {
        wifiManager.connect();
        // WiFi连接后启动Web UI
    }
    
    // 3. MQTT连接管理
    if (!mqttManager.isConnected() && WiFi已连接) {
        mqttManager.connect();
    }
    mqttManager.loop();
    
    // 4. 处理Web请求
    webServerManager.handleClient();
    webUIManager.handleClient();
    
    // 5. 处理LoRa消息
    String msg = loraManager.receiveMessage();
    if (msg.length() > 0) {
        loraManager.parseMessage(msg);
    }
    loraManager.processQueue();
    
    // 6. 定期采集传感器数据
    if (到达采集间隔) {
        acquireSensorData();
    }
    
    // 7. 定期上报MQTT数据
    if (到达上报间隔) {
        reportSensorDataToMqtt();
    }
    
    // 8. 定期输出系统状态
    if (到达统计间隔) {
        systemMonitor.printStats();
        systemMonitor.checkMemory();
    }
    
    // 9. LoRa节点发现
    if (到达心跳间隔) {
        loraManager.sendDiscovery(...);
    }
    
    // 10. OTA和故障恢复处理
    otaManager.handle();
    errorRecovery.checkAndRecover();
    
    // 11. 短暂延迟
    delay(20);
}
```

---

## 8. MQTT通信协议

### 8.1 主题定义

| 主题格式 | 方向 | 说明 |
|----------|------|------|
| esp32/{device_id}/data/temp | 上行 | 温度数据 |
| esp32/{device_id}/data/hum | 上行 | 湿度数据 |
| esp32/{device_id}/command | 下行 | 控制指令 |
| esp32/{device_id}/response | 上行 | 指令响应 |

### 8.2 上行数据格式

**温度数据**：
```
25.50
```

**湿度数据**：
```
60.20
```

### 8.3 下行指令格式

**继电器控制**：
```json
{"relay": 1}   // 开启
{"relay": 0}   // 关闭
```

**条件控制**：
```json
{
  "condition": {
    "enabled": true,
    "use_hysteresis": true,
    "temp": {
      "enabled": true,
      "high_threshold": 30.0,
      "low_threshold": 28.0
    },
    "humi": {
      "enabled": false
    }
  }
}
```

**定时控制**：
```json
{
  "timer": {
    "enabled": true,
    "slots": [
      {
        "index": 0,
        "enabled": true,
        "start_time": "08:00",
        "end_time": "18:00",
        "state": 1
      }
    ]
  }
}
```

**OTA升级**：
```json
{"ota": {"url": "https://example.com/firmware.bin"}}

{"ota": {"status": true}}
```

**状态查询**：
```json
{"query": {"type": "relay"}}
{"query": {"types": ["relay", "condition", "timer"]}}
{"query": {}}
```

### 8.4 上行响应格式

**继电器控制响应**：
```json
{"status":"ok","relay":1}
```

**条件控制响应**：
```json
{"status":"ok","condition_enabled":true}
```

**状态查询响应**：
```json
{
  "status": "ok",
  "relay": {"state": 1, "manual_mode": false},
  "condition": {"enabled": true, ...},
  "timer": {"enabled": false, ...},
  "network": {"wifi_connected": true, "mqtt_connected": true, ...},
  "device": {"id": "esp32-c3-abc123", "status": 2, ...},
  "sensor": {"temperature": 25.5, "humidity": 60.0}
}
```

---

## 9. Web接口说明

### 9.1 Web UI端点

| 端点 | 方法 | 说明 |
|------|------|------|
| / | GET | 主页面 |
| /api/config | GET/POST | 获取/设置配置 |
| /api/status | GET | 获取设备状态 |
| /api/time | GET | 获取当前时间 |
| /api/relay | POST | 继电器控制 |
| /api/condition | GET/POST | 条件控制 |
| /api/timer | GET/POST | 定时控制 |
| /api/restart | POST | 重启设备 |
| /api/ota/url | POST | OTA URL升级 |
| /api/ota/upload | POST | OTA文件上传 |
| /api/ota/status | GET | OTA升级状态 |

### 9.2 API响应格式

```json
{
  "status": "ok",
  "data": {...}
}
```

或

```json
{
  "status": "error",
  "message": "错误描述"
}
```

---

## 附录

### A. 引脚映射表

| 功能 | GPIO | 说明 |
|------|------|------|
| 继电器 | 20 | 继电器控制 |
| SDA | 8 | I2C数据线 |
| SCL | 9 | I2C时钟线 |
| LoRa SCK | 4 | SPI时钟 |
| LoRa MISO | 5 | SPI输入 |
| LoRa MOSI | 6 | SPI输出 |
| LoRa NSS | 7 | SPI片选 |
| LoRa RESET | 10 | 复位引脚 |

### B. 常量定义

| 常量 | 值 | 说明 |
|------|------|------|
| DEVICE_STATUS_OFFLINE | 0 | 设备离线 |
| DEVICE_STATUS_ONLINE | 1 | 设备在线 |
| DEVICE_STATUS_ONLINE_WITH_NETWORK | 2 | 在线且有网络 |

### C. 固件版本历史

| 版本 | 日期 | 更新内容 |
|------|------|----------|
| v1.0.0 | 2026-05 | 初始版本 |
| v1.1.0 | 2026-05 | 添加条件控制和定时控制 |
| v1.2.0 | 2026-05 | 添加OTA升级功能 |
| v1.3.0 | 2026-05 | 添加故障恢复机制 |
| v1.4.0 | 2026-05 | 添加MQTT状态查询功能 |
