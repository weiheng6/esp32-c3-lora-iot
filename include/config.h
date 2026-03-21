#ifndef CONFIG_H
#define CONFIG_H

// ==================== 调试配置 ====================
#define DEBUG_MODE 0  // 0: 关闭调试, 1: 开启调试

// ==================== 硬件引脚配置 ====================

// 继电器配置
#define RELAY_PIN 20
#define RELAY_ON HIGH
#define RELAY_OFF LOW

// SHT31 传感器（I2C）
#define SHT31_I2C_SDA 8
#define SHT31_I2C_SCL 9
#define SHT31_I2C_ADDRESS 0x44

// LoRa 模块（SPI）
#define LORA_SCK 4
#define LORA_MISO 5
#define LORA_MOSI 6
#define LORA_NSS 7
#define LORA_RESET 10
#define LORA_FREQUENCY 433E6

// ==================== 时间间隔配置 ====================
#define DEFAULT_ACQUISITION_INTERVAL 1000        // 传感器采集间隔（ms）
#define DEFAULT_MQTT_REPORT_INTERVAL 1000        // MQTT 上报间隔（ms）
#define WIFI_CHECK_INTERVAL 30000                // WiFi 检查间隔（ms）
#define LORA_SEND_INTERVAL 100                   // LoRa 发送间隔（ms）
#define SYSTEM_STATS_INTERVAL 10000              // 系统状态输出间隔（ms）
#define MEMORY_REPORT_INTERVAL 10000             // 内存上报间隔（ms）
#define LORA_NODE_DISCOVERY_INTERVAL 30000       // LoRa 节点发现间隔（ms）
#define LORA_HEARTBEAT_INTERVAL 60000            // LoRa 心跳间隔（ms）

// ==================== 内存与队列配置 ====================
#define MEM_THRESHOLD (15 * 1024)               // 内存重启阈值（字节）
#define LORA_QUEUE_SIZE 10                      // LoRa 消息队列大小
#define MAX_LORA_NODES 10                       // 最大 LoRa 节点数

// ==================== 条件控制比较操作符 ====================
#define COMPARE_GREATER_THAN 1
#define COMPARE_LESS_THAN 2
#define COMPARE_EQUAL 3

// ==================== MQTT 配置 ====================
extern const char* MQTT_SERVER;
extern const int MQTT_PORT;
extern const char* MQTT_USER;
extern const char* MQTT_PASSWORD;
extern const char* MQTT_WILL_MSG;
extern const int MQTT_WILL_QOS;
extern const bool MQTT_WILL_RETAIN;

// ==================== WiFi AP 模式配置 ====================
extern const char* AP_SSID;
extern const char* AP_PASSWORD;

// ==================== LoRa 消息优先级 ====================
#define LORA_PRIORITY_HIGHEST 4
#define LORA_PRIORITY_HIGH 3
#define LORA_PRIORITY_MEDIUM 2
#define LORA_PRIORITY_LOW 1
#define LORA_PRIORITY_LOWEST 0

// ==================== LoRa 消息类型 ====================
#define LORA_MSG_TYPE_DISCOVERY 1
#define LORA_MSG_TYPE_HEARTBEAT 2
#define LORA_MSG_TYPE_DATA 3
#define LORA_MSG_TYPE_CMD 4
#define LORA_MSG_TYPE_RESPONSE 5

// ==================== 设备状态枚举 ====================
enum DeviceStatus {
  STATUS_OFFLINE = 0,
  STATUS_ONLINE = 1,
  STATUS_ONLINE_WITH_NETWORK = 2
};

#endif // CONFIG_H
