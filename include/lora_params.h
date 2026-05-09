#ifndef LORA_PARAMS_H
#define LORA_PARAMS_H

#include <Arduino.h>
#include <LoRa.h>
#include "config.h"

enum LoRaPreset {
  ULTRA_LONG_RANGE,
  LONG_RANGE,
  BALANCED,
  FAST_TRANSFER,
  LOW_POWER,
  CUSTOM
};

struct LoRaParams {
  uint32_t frequency;
  uint32_t bandwidth;
  uint8_t spreadingFactor;
  uint8_t codingRate;
  int8_t txPower;
  uint16_t preambleLength;
  
  LoRaParams() : frequency(LORA_FREQUENCY),
                 bandwidth(125E3),
                 spreadingFactor(7),
                 codingRate(5),
                 txPower(17),
                 preambleLength(8) {}
  
  LoRaParams(uint32_t freq, uint32_t bw, uint8_t sf, uint8_t cr, int8_t pwr, uint16_t preamble)
    : frequency(freq),
      bandwidth(bw),
      spreadingFactor(sf),
      codingRate(cr),
      txPower(pwr),
      preambleLength(preamble) {}
  
  bool operator==(const LoRaParams &other) const {
    return frequency == other.frequency &&
           bandwidth == other.bandwidth &&
           spreadingFactor == other.spreadingFactor &&
           codingRate == other.codingRate &&
           txPower == other.txPower &&
           preambleLength == other.preambleLength;
  }
  
  bool operator!=(const LoRaParams &other) const {
    return !(*this == other);
  }
};

struct LoRaAdaptiveConfig {
  bool enabled;
  int8_t minSensitivity;
  uint32_t maxAdjustInterval;
  uint32_t frequencyHoppingThreshold;
  
  LoRaAdaptiveConfig() : enabled(false),
                         minSensitivity(-120),
                         maxAdjustInterval(30000),
                         frequencyHoppingThreshold(60000) {}
};

enum TransmissionQuality {
  QUALITY_EXCELLENT,
  QUALITY_GOOD,
  QUALITY_FAIR,
  QUALITY_POOR,
  QUALITY_CRITICAL
};

class LoRaParamsManager {
private:
  LoRaParams currentParams;
  LoRaPreset currentPreset;
  LoRaAdaptiveConfig adaptiveConfig;
  bool configValid;
  
  static const LoRaParams presetTable[5];
  
  bool validateParams(const LoRaParams &params);
  void logParamChange(const LoRaParams &oldParams, const LoRaParams &newParams);
  
public:
  LoRaParamsManager();
  
  bool loadConfig();
  bool saveConfig();
  
  bool applyPreset(LoRaPreset preset);
  bool applyParams(const LoRaParams &params);
  bool applyParams(uint32_t frequency, uint32_t bandwidth, 
                   uint8_t spreadingFactor, uint8_t codingRate,
                   int8_t txPower, uint16_t preambleLength);
  
  LoRaParams getCurrentParams() const { return currentParams; }
  LoRaPreset getCurrentPreset() const { return currentPreset; }
  
  void setAdaptiveConfig(const LoRaAdaptiveConfig &config);
  LoRaAdaptiveConfig getAdaptiveConfig() const { return adaptiveConfig; }
  
  const char* getPresetName(LoRaPreset preset) const;
  
  bool isConfigValid() const { return configValid; }
  
  void resetToDefaults();
  void printCurrentParams() const;
};

extern LoRaParamsManager loraParamsManager;

#endif
