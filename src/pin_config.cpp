#include "pin_config.h"
#include "config.h"
#include "log_manager.h"

const char* getPinModeName(PinMode mode);
const char* getFunctionName(PinFunction func);

PinConfigManager pinConfigManager;

PinConfigManager::PinConfigManager() : initialized(false) {}

PinConfigManager::~PinConfigManager() {
  if (initialized) {
    end();
  }
}

bool PinConfigManager::begin() {
  if (initialized) {
    LOG_DEBUG("PinConfigManager already initialized");
    return true;
  }
  
  pinConfigs.clear();
  
  PinConfig defaultPins[] = {
    PinConfig(RELAY_PIN, PIN_MODE_OUTPUT, PIN_FUNC_RELAY, "RELAY", RELAY_OFF),
    PinConfig(SHT31_I2C_SDA, PIN_MODE_I2C_SDA, PIN_FUNC_SENSOR_SDA, "SHT31_SDA", 0),
    PinConfig(SHT31_I2C_SCL, PIN_MODE_I2C_SCL, PIN_FUNC_SENSOR_SCL, "SHT31_SCL", 0),
    PinConfig(LORA_SCK, PIN_MODE_SPI_SCK, PIN_FUNC_LORA_SCK, "LoRa_SCK", 0),
    PinConfig(LORA_MISO, PIN_MODE_SPI_MISO, PIN_FUNC_LORA_MISO, "LoRa_MISO", 0),
    PinConfig(LORA_MOSI, PIN_MODE_SPI_MOSI, PIN_FUNC_LORA_MOSI, "LoRa_MOSI", 0),
    PinConfig(LORA_NSS, PIN_MODE_OUTPUT, PIN_FUNC_LORA_NSS, "LoRa_NSS", HIGH),
    PinConfig(LORA_RESET, PIN_MODE_OUTPUT, PIN_FUNC_LORA_RESET, "LoRa_RESET", HIGH)
  };
  
  for (const auto& pin : defaultPins) {
    pinConfigs.push_back(pin);
  }
  
  initialized = true;
  LOG_DEBUG("PinConfigManager initialized");
  
  return initializeAllPins();
}

void PinConfigManager::end() {
  if (!initialized) return;
  
  for (auto& config : pinConfigs) {
    if (config.isInitialized) {
      pinMode(config.pinNumber, INPUT);
      config.isInitialized = false;
    }
  }
  
  pinConfigs.clear();
  initialized = false;
  LOG_DEBUG("PinConfigManager stopped");
}

bool PinConfigManager::registerPin(const PinConfig& config) {
  if (!initialized) {
    LOG_DEBUG("PinConfigManager not initialized");
    return false;
  }
  
  auto it = std::find_if(pinConfigs.begin(), pinConfigs.end(),
    [config](const PinConfig& pc) { return pc.pinNumber == config.pinNumber; });
  
  if (it != pinConfigs.end()) {
    LOG_DEBUGF("Pin %d already registered", config.pinNumber);
    return false;
  }
  
  if (isPinOccupied(config.pinNumber, config.function)) {
    LOG_DEBUGF("Pin %d occupied by %s", config.pinNumber, config.functionName.c_str());
    return false;
  }
  
  pinConfigs.push_back(config);
  LOG_DEBUGF("Pin %d registered: %s (%s)", config.pinNumber, config.functionName.c_str(), 
              getPinModeName(config.mode));
  
  return initializePin(config.pinNumber);
}

bool PinConfigManager::registerPin(uint8_t pin, PinMode mode, PinFunction func, 
                                    const String& name, int initVal) {
  PinConfig config(pin, mode, func, name, initVal);
  return registerPin(config);
}

bool PinConfigManager::unregisterPin(uint8_t pin) {
  if (!initialized) return false;
  
  auto it = std::find_if(pinConfigs.begin(), pinConfigs.end(),
    [pin](const PinConfig& pc) { return pc.pinNumber == pin; });
  
  if (it == pinConfigs.end()) {
    LOG_DEBUGF("Pin %d not found", pin);
    return false;
  }
  
  if (it->isInitialized) {
    pinMode(pin, INPUT);
  }
  
  pinConfigs.erase(it);
  LOG_DEBUGF("Pin %d unregistered", pin);
  
  return true;
}

bool PinConfigManager::reconfigurePin(uint8_t pin, PinMode newMode, int newInitialValue) {
  if (!initialized) return false;
  
  auto it = std::find_if(pinConfigs.begin(), pinConfigs.end(),
    [pin](const PinConfig& pc) { return pc.pinNumber == pin; });
  
  if (it == pinConfigs.end()) {
    LOG_DEBUGF("Pin %d not registered", pin);
    return false;
  }
  
  it->mode = newMode;
  if (newInitialValue >= 0) {
    it->initialValue = newInitialValue;
  }
  
  if (it->isInitialized) {
    configurePinMode(*it);
  }
  
  LOG_DEBUGF("Pin %d reconfigured to %s", pin, getPinModeName(newMode));
  
  return true;
}

bool PinConfigManager::setPinFunction(uint8_t pin, PinFunction newFunction, const String& newName) {
  if (!initialized) return false;
  
  auto it = std::find_if(pinConfigs.begin(), pinConfigs.end(),
    [pin](const PinConfig& pc) { return pc.pinNumber == pin; });
  
  if (it == pinConfigs.end()) {
    LOG_DEBUGF("Pin %d not registered", pin);
    return false;
  }
  
  it->function = newFunction;
  if (newName.length() > 0) {
    it->functionName = newName;
  }
  
  LOG_DEBUGF("Pin %d function set to %s", pin, newName.c_str());
  
  return true;
}

bool PinConfigManager::initializePin(uint8_t pin) {
  auto it = std::find_if(pinConfigs.begin(), pinConfigs.end(),
    [pin](const PinConfig& pc) { return pc.pinNumber == pin; });
  
  if (it == pinConfigs.end()) {
    return false;
  }
  
  configurePinMode(*it);
  it->isInitialized = true;
  
  return true;
}

bool PinConfigManager::initializeAllPins() {
  if (!initialized) return false;
  
  bool allSuccess = true;
  for (auto& config : pinConfigs) {
    if (!config.isInitialized) {
      configurePinMode(config);
      config.isInitialized = true;
    }
  }
  
  LOG_DEBUG("All pins initialized");
  return allSuccess;
}

void PinConfigManager::configurePinMode(const PinConfig& config) {
  switch (config.mode) {
    case PIN_MODE_INPUT:
      pinMode(config.pinNumber, INPUT);
      break;
    case PIN_MODE_OUTPUT:
      pinMode(config.pinNumber, OUTPUT);
      digitalWrite(config.pinNumber, config.initialValue);
      break;
    case PIN_MODE_INPUT_PULLUP:
      pinMode(config.pinNumber, INPUT_PULLUP);
      break;
    case PIN_MODE_INPUT_PULLDOWN:
      pinMode(config.pinNumber, INPUT_PULLDOWN);
      break;
    case PIN_MODE_PWM:
      pinMode(config.pinNumber, OUTPUT);
      digitalWrite(config.pinNumber, config.initialValue);
      break;
    case PIN_MODE_ADC:
      pinMode(config.pinNumber, INPUT);
      break;
    case PIN_MODE_I2C_SDA:
    case PIN_MODE_I2C_SCL:
    case PIN_MODE_SPI_MOSI:
    case PIN_MODE_SPI_MISO:
    case PIN_MODE_SPI_SCK:
    case PIN_MODE_SPI_CS:
    case PIN_MODE_UART_TX:
    case PIN_MODE_UART_RX:
      break;
    default:
      pinMode(config.pinNumber, INPUT);
      break;
  }
}

PinConfig* PinConfigManager::getPinConfig(uint8_t pin) {
  auto it = std::find_if(pinConfigs.begin(), pinConfigs.end(),
    [pin](const PinConfig& pc) { return pc.pinNumber == pin; });
  
  return (it != pinConfigs.end()) ? &(*it) : nullptr;
}

PinConfig* PinConfigManager::getPinConfigByFunction(PinFunction func) {
  auto it = std::find_if(pinConfigs.begin(), pinConfigs.end(),
    [func](const PinConfig& pc) { return pc.function == func; });
  
  return (it != pinConfigs.end()) ? &(*it) : nullptr;
}

std::vector<PinConfig*> PinConfigManager::getPinsByMode(PinMode mode) {
  std::vector<PinConfig*> result;
  for (auto& config : pinConfigs) {
    if (config.mode == mode) {
      result.push_back(&config);
    }
  }
  return result;
}

std::vector<PinConfig*> PinConfigManager::getPinsByFunction(PinFunction func) {
  std::vector<PinConfig*> result;
  for (auto& config : pinConfigs) {
    if (config.function == func) {
      result.push_back(&config);
    }
  }
  return result;
}

std::vector<PinConfig*> PinConfigManager::getAllPinConfigs() {
  std::vector<PinConfig*> result;
  for (auto& config : pinConfigs) {
    result.push_back(&config);
  }
  return result;
}

bool PinConfigManager::isPinOccupied(uint8_t pin, PinFunction func) {
  auto it = std::find_if(pinConfigs.begin(), pinConfigs.end(),
    [pin, func](const PinConfig& pc) { 
      return pc.pinNumber == pin && pc.function != PIN_FUNC_UNASSIGNED && pc.function != func;
    });
  
  return it != pinConfigs.end();
}

PinConflictInfo PinConfigManager::detectConflict(uint8_t pin, PinFunction func) {
  PinConflictInfo conflict;
  conflict.pinNumber = pin;
  conflict.requestedFunction = func;
  
  auto it = std::find_if(pinConfigs.begin(), pinConfigs.end(),
    [pin](const PinConfig& pc) { return pc.pinNumber == pin; });
  
  if (it != pinConfigs.end()) {
    conflict.existingFunction = it->function;
    conflict.existingFunctionName = it->functionName;
  } else {
    conflict.existingFunction = PIN_FUNC_UNASSIGNED;
    conflict.existingFunctionName = "Unassigned";
  }
  
  conflict.requestedFunctionName = getFunctionName(func);
  
  return conflict;
}

bool PinConfigManager::hasConflict(uint8_t pin, PinFunction func) {
  return isPinOccupied(pin, func);
}

PinConflictResolution PinConfigManager::checkAndResolveConflict(uint8_t pin, PinFunction func) {
  if (!hasConflict(pin, func)) {
    return RESOLUTION_NONE;
  }
  
  auto it = std::find_if(pinConfigs.begin(), pinConfigs.end(),
    [pin](const PinConfig& pc) { return pc.pinNumber == pin; });
  
  if (it != pinConfigs.end() && it->isReserved) {
    return RESOLUTION_SKIP_NEW;
  }
  
  return RESOLUTION_FORCE_NEW;
}

bool PinConfigManager::saveConfiguration() {
  bool success = preferences.begin("pin_config", false);
  if (!success) {
    LOG_DEBUG("Cannot open Preferences for save");
    return false;
  }
  
  preferences.putInt("pin_count", pinConfigs.size());
  
  char key[16];
  int index = 0;
  for (const auto& config : pinConfigs) {
    snprintf(key, sizeof(key), "pin_%d", index);
    uint32_t packed = (config.pinNumber & 0xFF) | 
                      ((config.mode & 0xFF) << 8) | 
                      ((config.function & 0xFF) << 16);
    preferences.putUInt(key, packed);
    
    snprintf(key, sizeof(key), "name_%d", index);
    preferences.putString(key, config.functionName);
    
    snprintf(key, sizeof(key), "init_%d", index);
    preferences.putInt(key, config.initialValue);
    
    index++;
  }
  
  preferences.end();
  LOG_DEBUG("Pin config saved");
  
  return true;
}

bool PinConfigManager::loadConfiguration() {
  bool success = preferences.begin("pin_config", true);
  if (!success) {
    LOG_DEBUG("Cannot open Preferences for load");
    return false;
  }
  
  int pinCount = preferences.getInt("pin_count", -1);
  if (pinCount < 0) {
    preferences.end();
    LOG_DEBUG("No saved pin config found");
    return false;
  }
  
  pinConfigs.clear();
  
  char key[16];
  for (int i = 0; i < pinCount; i++) {
    snprintf(key, sizeof(key), "pin_%d", i);
    uint32_t packed = preferences.getUInt(key, 0);
    
    PinConfig config;
    config.pinNumber = packed & 0xFF;
    config.mode = static_cast<PinMode>((packed >> 8) & 0xFF);
    config.function = static_cast<PinFunction>((packed >> 16) & 0xFF);
    
    snprintf(key, sizeof(key), "name_%d", i);
    config.functionName = preferences.getString(key, "");
    
    snprintf(key, sizeof(key), "init_%d", i);
    config.initialValue = preferences.getInt(key, 0);
    
    pinConfigs.push_back(config);
  }
  
  preferences.end();
  LOG_DEBUGF("Loaded %d pin configs", pinCount);
  
  return initializeAllPins();
}

bool PinConfigManager::resetToDefault() {
  pinConfigs.clear();
  preferences.begin("pin_config", false);
  preferences.clear();
  preferences.end();
  
  return begin();
}

void PinConfigManager::printPinStatus() {
  Serial.println("\n========== Pin Status ==========");
  Serial.printf("Total pins: %d\n", getRegisteredPinCount());
  Serial.println("-----------------------------");
  
  for (const auto& config : pinConfigs) {
    Serial.printf("Pin %2d | Mode: %-15s | Func: %-12s | %s\n",
      config.pinNumber,
      getPinModeName(config.mode),
      config.functionName.c_str(),
      config.isInitialized ? "[Init]" : "[Not Init]");
  }
  
  Serial.println("==============================\n");
}

void PinConfigManager::listAllPins() {
  Serial.println("\n========== Pin List ==========");
  
  for (const auto& config : pinConfigs) {
    Serial.printf("GPIO%-2d -> %s (%s)\n",
      config.pinNumber,
      config.functionName.c_str(),
      getPinModeName(config.mode));
  }
  
  Serial.println("===============================\n");
}

const char* getPinModeName(PinMode mode) {
  switch (mode) {
    case PIN_MODE_INPUT: return "INPUT";
    case PIN_MODE_OUTPUT: return "OUTPUT";
    case PIN_MODE_INPUT_PULLUP: return "INPUT_PULLUP";
    case PIN_MODE_INPUT_PULLDOWN: return "INPUT_PULLDOWN";
    case PIN_MODE_PWM: return "PWM";
    case PIN_MODE_ADC: return "ADC";
    case PIN_MODE_I2C_SDA: return "I2C_SDA";
    case PIN_MODE_I2C_SCL: return "I2C_SCL";
    case PIN_MODE_SPI_MOSI: return "SPI_MOSI";
    case PIN_MODE_SPI_MISO: return "SPI_MISO";
    case PIN_MODE_SPI_SCK: return "SPI_SCK";
    case PIN_MODE_SPI_CS: return "SPI_CS";
    case PIN_MODE_UART_TX: return "UART_TX";
    case PIN_MODE_UART_RX: return "UART_RX";
    default: return "UNKNOWN";
  }
}

const char* getFunctionName(PinFunction func) {
  switch (func) {
    case PIN_FUNC_UNASSIGNED: return "Unassigned";
    case PIN_FUNC_RELAY: return "Relay";
    case PIN_FUNC_SENSOR_SDA: return "Sensor_SDA";
    case PIN_FUNC_SENSOR_SCL: return "Sensor_SCL";
    case PIN_FUNC_LORA_SCK: return "LoRa_SCK";
    case PIN_FUNC_LORA_MISO: return "LoRa_MISO";
    case PIN_FUNC_LORA_MOSI: return "LoRa_MOSI";
    case PIN_FUNC_LORA_NSS: return "LoRa_NSS";
    case PIN_FUNC_LORA_RESET: return "LoRa_RESET";
    case PIN_FUNC_LORA_DIO0: return "LoRa_DIO0";
    case PIN_FUNC_USER_DEFINED: return "User Defined";
    default: return "Unknown";
  }
}
