#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>
#include <vector>

/**
 * ESP32-C3 引脚功能枚举
 * 用于定义GPIO引脚可以配置的功能
 */
enum PinMode {
  PIN_MODE_INPUT = 0,           /**< 输入模式 */
  PIN_MODE_OUTPUT = 1,          /**< 输出模式 */
  PIN_MODE_INPUT_PULLUP = 2,    /**< 输入模式 + 上拉电阻 */
  PIN_MODE_INPUT_PULLDOWN = 3,  /**< 输入模式 + 下拉电阻 */
  PIN_MODE_PWM = 4,             /**< PWM输出模式 */
  PIN_MODE_ADC = 5,             /**< ADC模拟输入模式 */
  PIN_MODE_I2C_SDA = 6,        /**< I2C数据线 */
  PIN_MODE_I2C_SCL = 7,         /**< I2C时钟线 */
  PIN_MODE_SPI_MOSI = 8,        /**< SPI数据输出 */
  PIN_MODE_SPI_MISO = 9,        /**< SPI数据输入 */
  PIN_MODE_SPI_SCK = 10,       /**< SPI时钟线 */
  PIN_MODE_SPI_CS = 11,        /**< SPI片选 */
  PIN_MODE_UART_TX = 12,        /**< UART发送 */
  PIN_MODE_UART_RX = 13         /**< UART接收 */
};

/**
 * 引脚功能类型枚举
 * 定义引脚可以承担的具体功能角色
 */
enum PinFunction {
  PIN_FUNC_UNASSIGNED = 0,      /**< 未分配 */
  PIN_FUNC_RELAY,               /**< 继电器控制 */
  PIN_FUNC_SENSOR_SDA,          /**< 传感器I2C数据线 */
  PIN_FUNC_SENSOR_SCL,          /**< 传感器I2C时钟线 */
  PIN_FUNC_LORA_SCK,            /**< LoRa SPI时钟 */
  PIN_FUNC_LORA_MISO,           /**< LoRa SPI数据输入 */
  PIN_FUNC_LORA_MOSI,           /**< LoRa SPI数据输出 */
  PIN_FUNC_LORA_NSS,            /**< LoRa SPI片选 */
  PIN_FUNC_LORA_RESET,          /**< LoRa复位 */
  PIN_FUNC_LORA_DIO0,           /**< LoRa中断引脚 */
  PIN_FUNC_USER_DEFINED         /**< 用户自定义功能 */
};

/**
 * 冲突解决方案枚举
 * 当引脚功能冲突时可以选择的解决方案
 */
enum PinConflictResolution {
  RESOLUTION_NONE = 0,          /**< 无冲突 */
  RESOLUTION_FORCE_NEW,         /**< 强制使用新配置 */
  RESOLUTION_SKIP_NEW,          /**< 保留原有配置 */
  RESOLUTION_ERROR              /**< 返回错误 */
};

/**
 * ESP32-C3 引脚兼容性信息结构
 * 描述每个GPIO引脚的硬件特性和限制
 */
struct PinCapability {
  uint8_t gpio;                 /**< GPIO编号 */
  bool hasADC;                  /**< 是否支持ADC */
  bool hasPWM;                  /**< 是否支持PWM */
  bool hasI2C;                  /**< 是否支持I2C */
  bool hasSPI;                  /**< 是否支持SPI */
  bool hasUART;                 /**< 是否支持UART */
  bool isStrappingPin;          /**< 是否为启动引脚(启动时有特殊电平要求) */
  bool isUSBEnabled;            /**< USB引脚(仅GPIO19/20) */
  const char* recommendedUse;  /**< 推荐用途 */
  const char* warning;          /**< 警告信息 */
};

/**
 * ESP32-C3 引脚兼容性表
 * 定义所有可用GPIO引脚的硬件能力
 */
static const PinCapability ESP32C3_PIN_TABLE[] = {
  {0,  true,  true,  true,  true,  true,  true,  false, "GPIO0 - 按钮/LED",       "⚠️ 启动引脚，高电平启动"},
  {1,  false, true,  true,  true,  true,  true,  false, "GPIO1 - 通用GPIO",        ""},
  {2,  true,  true,  true,  true,  true,  true,  false, "GPIO2 - 通用GPIO",        ""},
  {3,  false, true,  true,  true,  true,  true,  false, "GPIO3 - 通用GPIO",        ""},
  {4,  true,  true,  true,  true,  true,  true,  false, "GPIO4 - 通用GPIO",        ""},
  {5,  false, true,  true,  true,  true,  true,  false, "GPIO5 - 通用GPIO",        ""},
  {6,  false, true,  true,  true,  true,  true,  false, "GPIO6 - SPI Flash",       "⚠️ 用于SPI Flash,建议勿用"},
  {7,  false, true,  true,  true,  true,  true,  false, "GPIO7 - SPI Flash",       "⚠️ 用于SPI Flash,建议勿用"},
  {8,  false, true,  true,  true,  true,  true,  false, "GPIO8 - SPI Flash",       "⚠️ 用于SPI Flash,建议勿用"},
  {9,  false, true,  true,  true,  true,  true,  false, "GPIO9 - SPI Flash",       "⚠️ 用于SPI Flash,建议勿用"},
  {10, false, true,  true,  true,  true,  true,  false, "GPIO10 - SPI Flash",     "⚠️ 用于SPI Flash,建议勿用"},
  {11, false, true,  true,  true,  true,  true,  false, "GPIO11 - SPI Flash",     "⚠️ 用于SPI Flash,建议勿用"},
  {12, false, true,  true,  true,  true,  true,  false, "GPIO12 - RGB LED",        ""},
  {13, false, true,  true,  true,  true,  true,  false, "GPIO13 - Neopixel",       ""},
  {14, false, true,  true,  true,  true,  true,  false, "GPIO14 - 通用GPIO",        ""},
  {15, false, true,  true,  true,  true,  true,  false, "GPIO15 - 通用GPIO",        ""},
  {16, false, true,  true,  true,  true,  true,  false, "GPIO16 - 通用GPIO",        ""},
  {17, false, true,  true,  true,  true,  true,  false, "GPIO17 - 通用GPIO",        ""},
  {18, false, true,  true,  true,  true,  true,  false, "GPIO18 - 通用GPIO",        ""},
  {19, false, true,  true,  true,  true,  true,  false, "GPIO19 - USB D-",         "⚠️ USB引脚,仅用于USB功能"},
  {20, false, true,  true,  true,  true,  true,  false, "GPIO20 - USB D+",         "⚠️ USB引脚,仅用于USB功能"},
  {21, false, true,  true,  true,  true,  true,  false, "GPIO21 - 通用GPIO",        ""}
};

/**
 * 功能与引脚模式映射表
 * 定义每个功能需要的引脚模式
 */
static const struct {
  PinFunction func;
  PinMode requiredMode;
  bool requiresADC;
  bool requiresI2C;
  bool requiresSPI;
  const char* description;
} PIN_FUNCTION_MODES[] = {
  {PIN_FUNC_RELAY,          PIN_MODE_OUTPUT,      false, false, false, "继电器控制 - 需要GPIO输出"},
  {PIN_FUNC_SENSOR_SDA,     PIN_MODE_I2C_SDA,     false, true,  false, "传感器I2C数据线"},
  {PIN_FUNC_SENSOR_SCL,     PIN_MODE_I2C_SCL,     false, true,  false, "传感器I2C时钟线"},
  {PIN_FUNC_LORA_SCK,       PIN_MODE_SPI_SCK,      false, false, true,  "LoRa SPI时钟线"},
  {PIN_FUNC_LORA_MISO,      PIN_MODE_SPI_MISO,     false, false, true,  "LoRa SPI数据输入"},
  {PIN_FUNC_LORA_MOSI,      PIN_MODE_SPI_MOSI,     false, false, true,  "LoRa SPI数据输出"},
  {PIN_FUNC_LORA_NSS,       PIN_MODE_OUTPUT,       false, false, false, "LoRa SPI片选(需要GPIO输出)"},
  {PIN_FUNC_LORA_RESET,     PIN_MODE_OUTPUT,       false, false, false, "LoRa复位引脚"},
  {PIN_FUNC_LORA_DIO0,      PIN_MODE_INPUT_PULLUP, false, false, false, "LoRa中断引脚(需要上拉)"}
};

struct PinConfig {
  uint8_t pinNumber;
  PinMode mode;
  PinFunction function;
  String functionName;
  int initialValue;
  bool isInitialized;
  bool isReserved;
  
  PinConfig() 
    : pinNumber(0), 
      mode(PIN_MODE_INPUT), 
      function(PIN_FUNC_UNASSIGNED),
      functionName(""),
      initialValue(0),
      isInitialized(false),
      isReserved(false) {}
      
  PinConfig(uint8_t pin, PinMode mod, PinFunction func, const String& name, int initVal = 0)
    : pinNumber(pin),
      mode(mod),
      function(func),
      functionName(name),
      initialValue(initVal),
      isInitialized(false),
      isReserved(false) {}
};

struct PinConflictInfo {
  uint8_t pinNumber;
  PinFunction existingFunction;
  PinFunction requestedFunction;
  String existingFunctionName;
  String requestedFunctionName;
};

class PinConfigManager {
private:
  std::vector<PinConfig> pinConfigs;
  Preferences preferences;
  bool initialized;
  
  bool isPinOccupied(uint8_t pin, PinFunction func);
  PinConflictInfo detectConflict(uint8_t pin, PinFunction func);
  void configurePinMode(const PinConfig& config);
  
public:
  PinConfigManager();
  ~PinConfigManager();
  
  bool begin();
  void end();
  
  bool registerPin(const PinConfig& config);
  bool registerPin(uint8_t pin, PinMode mode, PinFunction func, const String& name, int initVal = 0);
  bool unregisterPin(uint8_t pin);
  
  bool reconfigurePin(uint8_t pin, PinMode newMode, int newInitialValue = -1);
  bool setPinFunction(uint8_t pin, PinFunction newFunction, const String& newName = "");
  
  bool initializePin(uint8_t pin);
  bool initializeAllPins();
  
  PinConfig* getPinConfig(uint8_t pin);
  PinConfig* getPinConfigByFunction(PinFunction func);
  std::vector<PinConfig*> getPinsByMode(PinMode mode);
  std::vector<PinConfig*> getPinsByFunction(PinFunction func);
  std::vector<PinConfig*> getAllPinConfigs();
  
  PinConflictResolution checkAndResolveConflict(uint8_t pin, PinFunction func);
  bool hasConflict(uint8_t pin, PinFunction func);
  
  bool saveConfiguration();
  bool loadConfiguration();
  bool resetToDefault();
  
  void printPinStatus();
  void listAllPins();
  
  int getRegisteredPinCount() const { return pinConfigs.size(); }
  bool isInitializedStatus() const { return initialized; }
};

extern PinConfigManager pinConfigManager;

#endif
