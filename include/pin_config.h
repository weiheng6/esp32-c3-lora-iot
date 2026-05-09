#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>
#include <vector>

enum PinMode {
  PIN_MODE_INPUT = 0,
  PIN_MODE_OUTPUT = 1,
  PIN_MODE_INPUT_PULLUP = 2,
  PIN_MODE_INPUT_PULLDOWN = 3,
  PIN_MODE_PWM = 4,
  PIN_MODE_ADC = 5,
  PIN_MODE_I2C_SDA = 6,
  PIN_MODE_I2C_SCL = 7,
  PIN_MODE_SPI_MOSI = 8,
  PIN_MODE_SPI_MISO = 9,
  PIN_MODE_SPI_SCK = 10,
  PIN_MODE_SPI_CS = 11,
  PIN_MODE_UART_TX = 12,
  PIN_MODE_UART_RX = 13
};

enum PinFunction {
  PIN_FUNC_UNASSIGNED = 0,
  PIN_FUNC_RELAY,
  PIN_FUNC_SENSOR_SDA,
  PIN_FUNC_SENSOR_SCL,
  PIN_FUNC_LORA_SCK,
  PIN_FUNC_LORA_MISO,
  PIN_FUNC_LORA_MOSI,
  PIN_FUNC_LORA_NSS,
  PIN_FUNC_LORA_RESET,
  PIN_FUNC_LORA_DIO0,
  PIN_FUNC_USER_DEFINED
};

enum PinConflictResolution {
  RESOLUTION_NONE = 0,
  RESOLUTION_FORCE_NEW,
  RESOLUTION_SKIP_NEW,
  RESOLUTION_ERROR
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
