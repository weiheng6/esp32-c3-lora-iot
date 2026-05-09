#include "lora_params.h"
#include <Preferences.h>

LoRaParamsManager loraParamsManager;

const LoRaParams LoRaParamsManager::presetTable[5] = {
    LoRaParams(433000000, 62500, 12, 8, 20, 16),
    LoRaParams(433000000, 125000, 10, 8, 17, 12),
    LoRaParams(433000000, 125000, 9, 5, 17, 8),
    LoRaParams(433000000, 250000, 7, 5, 14, 6),
    LoRaParams(433000000, 125000, 8, 5, 10, 8)
};

LoRaParamsManager::LoRaParamsManager() : 
    currentParams(),
    currentPreset(BALANCED),
    adaptiveConfig(),
    configValid(false) {
}

bool LoRaParamsManager::loadConfig() {
    Preferences prefs;
    if (!prefs.begin("lora_params", true)) {
        Serial.println("[LoRaParams] Failed to open preferences");
        return false;
    }
    
    currentParams.frequency = prefs.getUInt("freq", LORA_FREQUENCY);
    currentParams.bandwidth = prefs.getUInt("bw", 125000);
    currentParams.spreadingFactor = prefs.getUChar("sf", 9);
    currentParams.codingRate = prefs.getUChar("cr", 5);
    currentParams.txPower = prefs.getChar("txpwr", 17);
    currentParams.preambleLength = prefs.getUShort("preamble", 8);
    
    currentPreset = (LoRaPreset)prefs.getUChar("preset", BALANCED);
    
    adaptiveConfig.enabled = prefs.getBool("adapt_en", false);
    adaptiveConfig.minSensitivity = prefs.getChar("min_sens", -120);
    adaptiveConfig.maxAdjustInterval = prefs.getUInt("adj_int", 30000);
    adaptiveConfig.frequencyHoppingThreshold = prefs.getUInt("hop_thr", -100);
    
    prefs.end();
    
    configValid = validateParams(currentParams);
    Serial.println("[LoRaParams] Configuration loaded");
    return true;
}

bool LoRaParamsManager::saveConfig() {
    Preferences prefs;
    if (!prefs.begin("lora_params", false)) {
        Serial.println("[LoRaParams] Failed to open preferences");
        return false;
    }
    
    prefs.putUInt("freq", currentParams.frequency);
    prefs.putUInt("bw", currentParams.bandwidth);
    prefs.putUChar("sf", currentParams.spreadingFactor);
    prefs.putUChar("cr", currentParams.codingRate);
    prefs.putChar("txpwr", currentParams.txPower);
    prefs.putUShort("preamble", currentParams.preambleLength);
    prefs.putUChar("preset", (uint8_t)currentPreset);
    
    prefs.putBool("adapt_en", adaptiveConfig.enabled);
    prefs.putChar("min_sens", adaptiveConfig.minSensitivity);
    prefs.putUInt("adj_int", adaptiveConfig.maxAdjustInterval);
    prefs.putUInt("hop_thr", adaptiveConfig.frequencyHoppingThreshold);
    
    prefs.end();
    
    Serial.println("[LoRaParams] Configuration saved");
    return true;
}

bool LoRaParamsManager::validateParams(const LoRaParams &params) {
    if (params.frequency < 150000000 || params.frequency > 960000000) {
        Serial.println("[LoRaParams] Invalid frequency");
        return false;
    }
    if (params.bandwidth < 7800 || params.bandwidth > 500000) {
        Serial.println("[LoRaParams] Invalid bandwidth");
        return false;
    }
    if (params.spreadingFactor < 7 || params.spreadingFactor > 12) {
        Serial.println("[LoRaParams] Invalid spreading factor");
        return false;
    }
    if (params.codingRate < 5 || params.codingRate > 8) {
        Serial.println("[LoRaParams] Invalid coding rate");
        return false;
    }
    if (params.txPower < 2 || params.txPower > 20) {
        Serial.println("[LoRaParams] Invalid TX power");
        return false;
    }
    if (params.preambleLength < 6 || params.preambleLength > 255) {
        Serial.println("[LoRaParams] Invalid preamble length");
        return false;
    }
    
    return true;
}

void LoRaParamsManager::logParamChange(const LoRaParams &oldParams, const LoRaParams &newParams) {
    Serial.println("[LoRaParams] Parameter change:");
    if (oldParams.frequency != newParams.frequency) {
        Serial.printf("  Frequency: %lu -> %lu Hz\n", oldParams.frequency, newParams.frequency);
    }
    if (oldParams.bandwidth != newParams.bandwidth) {
        Serial.printf("  Bandwidth: %lu -> %lu Hz\n", oldParams.bandwidth, newParams.bandwidth);
    }
    if (oldParams.spreadingFactor != newParams.spreadingFactor) {
        Serial.printf("  Spreading Factor: %d -> %d\n", oldParams.spreadingFactor, newParams.spreadingFactor);
    }
    if (oldParams.codingRate != newParams.codingRate) {
        Serial.printf("  Coding Rate: 4/%d -> 4/%d\n", oldParams.codingRate, newParams.codingRate);
    }
    if (oldParams.txPower != newParams.txPower) {
        Serial.printf("  TX Power: %d -> %d dBm\n", oldParams.txPower, newParams.txPower);
    }
    if (oldParams.preambleLength != newParams.preambleLength) {
        Serial.printf("  Preamble: %d -> %d\n", oldParams.preambleLength, newParams.preambleLength);
    }
}

bool LoRaParamsManager::applyPreset(LoRaPreset preset) {
    if (preset < 0 || preset >= 5) {
        Serial.println("[LoRaParams] Invalid preset index");
        return false;
    }
    
    LoRaParams oldParams = currentParams;
    currentParams = presetTable[preset];
    currentPreset = preset;
    configValid = true;
    
    logParamChange(oldParams, currentParams);
    
    LoRa.setFrequency(currentParams.frequency);
    LoRa.setSignalBandwidth(currentParams.bandwidth);
    LoRa.setSpreadingFactor(currentParams.spreadingFactor);
    LoRa.setCodingRate4(currentParams.codingRate);
    LoRa.setTxPower(currentParams.txPower);
    LoRa.setPreambleLength(currentParams.preambleLength);
    
    saveConfig();
    Serial.printf("[LoRaParams] Applied preset: %s\n", getPresetName(preset));
    return true;
}

bool LoRaParamsManager::applyParams(const LoRaParams &params) {
    if (!validateParams(params)) {
        Serial.println("[LoRaParams] Invalid parameters");
        return false;
    }
    
    LoRaParams oldParams = currentParams;
    currentParams = params;
    currentPreset = CUSTOM;
    configValid = true;
    
    logParamChange(oldParams, currentParams);
    
    LoRa.setFrequency(currentParams.frequency);
    LoRa.setSignalBandwidth(currentParams.bandwidth);
    LoRa.setSpreadingFactor(currentParams.spreadingFactor);
    LoRa.setCodingRate4(currentParams.codingRate);
    LoRa.setTxPower(currentParams.txPower);
    LoRa.setPreambleLength(currentParams.preambleLength);
    
    saveConfig();
    Serial.println("[LoRaParams] Parameters applied");
    return true;
}

bool LoRaParamsManager::applyParams(uint32_t frequency, uint32_t bandwidth, 
                                     uint8_t spreadingFactor, uint8_t codingRate,
                                     int8_t txPower, uint16_t preambleLength) {
    LoRaParams params(frequency, bandwidth, spreadingFactor, codingRate, txPower, preambleLength);
    
    return applyParams(params);
}

void LoRaParamsManager::setAdaptiveConfig(const LoRaAdaptiveConfig &config) {
    adaptiveConfig = config;
    saveConfig();
    Serial.printf("[LoRaParams] Adaptive config updated: enabled=%d\n", config.enabled);
}

const char* LoRaParamsManager::getPresetName(LoRaPreset preset) const {
    switch (preset) {
        case ULTRA_LONG_RANGE: return "Ultra Long Range";
        case LONG_RANGE: return "Long Range";
        case BALANCED: return "Balanced";
        case FAST_TRANSFER: return "Fast Transfer";
        case LOW_POWER: return "Low Power";
        case CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

void LoRaParamsManager::resetToDefaults() {
    applyPreset(BALANCED);
    adaptiveConfig = LoRaAdaptiveConfig();
    saveConfig();
    Serial.println("[LoRaParams] Reset to defaults");
}

void LoRaParamsManager::printCurrentParams() const {
    Serial.println("[LoRaParams] Current settings:");
    Serial.printf("  Frequency: %lu Hz\n", currentParams.frequency);
    Serial.printf("  Bandwidth: %lu Hz\n", currentParams.bandwidth);
    Serial.printf("  Spreading Factor: %d\n", currentParams.spreadingFactor);
    Serial.printf("  Coding Rate: 4/%d\n", currentParams.codingRate);
    Serial.printf("  TX Power: %d dBm\n", currentParams.txPower);
    Serial.printf("  Preamble Length: %d\n", currentParams.preambleLength);
    Serial.printf("  Preset: %s\n", getPresetName(currentPreset));
    Serial.printf("  Adaptive: %s\n", adaptiveConfig.enabled ? "Enabled" : "Disabled");
}
