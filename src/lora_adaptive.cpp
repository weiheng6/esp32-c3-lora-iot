#include "lora_adaptive.h"

LoRaAdaptiveController loraAdaptiveController;

LoRaAdaptiveController::LoRaAdaptiveController() :
    enabled(false),
    lastAdjustTime(0),
    adjustCooldown(5000),
    currentRSSI(-120),
    currentSNR(-10.0f),
    packetLossRate(0.0f),
    batteryLevel(100),
    reportInterval(1000),
    historyIndex(0),
    historyCount(0),
    currentQuality(QUALITY_GOOD),
    transitionFactor(1.0f),
    totalPackets(0),
    lostPackets(0),
    successfulPackets(0) {
    
    for (int i = 0; i < 10; i++) {
        rssiHistory[i] = -120;
        snrHistory[i] = -10.0f;
        lossHistory[i] = 0.0f;
    }
    
    targetParams = loraParamsManager.getCurrentParams();
    appliedParams = targetParams;
}

void LoRaAdaptiveController::begin() {
    enabled = true;
    lastAdjustTime = millis();
    targetParams = loraParamsManager.getCurrentParams();
    appliedParams = targetParams;
    Serial.println("[LoRaAdaptive] Started");
}

float LoRaAdaptiveController::calculateAverageRSSI() {
    if (historyCount == 0) return -120;
    
    float sum = 0;
    int count = min(historyCount, 10);
    for (int i = 0; i < count; i++) {
        sum += rssiHistory[i];
    }
    return sum / count;
}

float LoRaAdaptiveController::calculateAverageSNR() {
    if (historyCount == 0) return -10.0f;
    
    float sum = 0;
    int count = min(historyCount, 10);
    for (int i = 0; i < count; i++) {
        sum += snrHistory[i];
    }
    return sum / count;
}

float LoRaAdaptiveController::calculateAverageLossRate() {
    if (historyCount == 0) return 0.0f;
    
    float sum = 0;
    int count = min(historyCount, 10);
    for (int i = 0; i < count; i++) {
        sum += lossHistory[i];
    }
    return sum / count;
}

int16_t LoRaAdaptiveController::getAverageRSSI() const {
    if (historyCount == 0) return -120;
    
    float sum = 0;
    int count = min(historyCount, 10);
    for (int i = 0; i < count; i++) {
        sum += rssiHistory[i];
    }
    return (int16_t)(sum / count);
}

float LoRaAdaptiveController::getAverageSNR() const {
    if (historyCount == 0) return -10.0f;
    
    float sum = 0;
    int count = min(historyCount, 10);
    for (int i = 0; i < count; i++) {
        sum += snrHistory[i];
    }
    return sum / count;
}

void LoRaAdaptiveController::updateHistory(int16_t rssi, float snr, float loss) {
    rssiHistory[historyIndex] = rssi;
    snrHistory[historyIndex] = snr;
    lossHistory[historyIndex] = loss;
    
    historyIndex = (historyIndex + 1) % 10;
    if (historyCount < 10) {
        historyCount++;
    }
}

bool LoRaAdaptiveController::shouldAdjust() {
    if (!enabled) return false;
    
    if (millis() - lastAdjustTime < adjustCooldown) {
        return false;
    }
    
    if (historyCount < 3) {
        return false;
    }
    
    return true;
}

LoRaParams LoRaAdaptiveController::calculateOptimalParams() {
    LoRaParams optimal = loraParamsManager.getCurrentParams();
    
    float avgRSSI = calculateAverageRSSI();
    float avgSNR = calculateAverageSNR();
    float avgLoss = calculateAverageLossRate();
    
    if (avgRSSI < -110 || avgSNR < -5.0f || avgLoss > 0.3f) {
        if (optimal.spreadingFactor < 12) {
            optimal.spreadingFactor += 1;
        }
        if (optimal.bandwidth > 62500) {
            optimal.bandwidth = optimal.bandwidth / 2;
        }
        if (optimal.codingRate < 8) {
            optimal.codingRate += 1;
        }
        if (avgLoss > 0.2f && optimal.txPower < 20) {
            optimal.txPower += 2;
        }
    }
    else if (avgRSSI > -80 && avgSNR > 10.0f && avgLoss < 0.05f) {
        if (optimal.spreadingFactor > 7) {
            optimal.spreadingFactor -= 1;
        }
        if (optimal.bandwidth < 250000) {
            optimal.bandwidth = optimal.bandwidth * 2;
        }
        if (optimal.txPower > 2) {
            optimal.txPower -= 2;
        }
    }
    
    if (batteryLevel < 20) {
        optimal.txPower = min(optimal.txPower, (int8_t)10);
        if (optimal.spreadingFactor > 7) {
            optimal.spreadingFactor -= 1;
        }
    }
    
    if (reportInterval < 500) {
        if (optimal.spreadingFactor > 8) {
            optimal.spreadingFactor -= 1;
        }
        if (optimal.bandwidth < 250000) {
            optimal.bandwidth = optimal.bandwidth * 2;
        }
    }
    
    return optimal;
}

void LoRaAdaptiveController::applySmoothTransition() {
    if (transitionFactor >= 1.0f) {
        appliedParams = targetParams;
        return;
    }
    
    transitionFactor = min(transitionFactor + 0.2f, 1.0f);
    
    appliedParams.frequency = appliedParams.frequency + 
        (targetParams.frequency - appliedParams.frequency) * transitionFactor;
    appliedParams.bandwidth = appliedParams.bandwidth + 
        (targetParams.bandwidth - appliedParams.bandwidth) * transitionFactor;
    appliedParams.spreadingFactor = appliedParams.spreadingFactor + 
        (targetParams.spreadingFactor - appliedParams.spreadingFactor) * transitionFactor;
    appliedParams.codingRate = appliedParams.codingRate + 
        (targetParams.codingRate - appliedParams.codingRate) * transitionFactor;
    appliedParams.txPower = appliedParams.txPower + 
        (targetParams.txPower - appliedParams.txPower) * transitionFactor;
    appliedParams.preambleLength = appliedParams.preambleLength + 
        (targetParams.preambleLength - appliedParams.preambleLength) * transitionFactor;
}

void LoRaAdaptiveController::incrementAdjustCounter() {
    lastAdjustTime = millis();
    transitionFactor = 0.0f;
}

void LoRaAdaptiveController::performAdaptiveAdjustment() {
    if (!shouldAdjust()) return;
    
    LoRaParams newTarget = calculateOptimalParams();
    
    if (newTarget != targetParams) {
        Serial.println("[LoRaAdaptive] Adjusting parameters");
        targetParams = newTarget;
        
        applySmoothTransition();
        
        loraParamsManager.applyParams(appliedParams);
        
        incrementAdjustCounter();
        
        Serial.printf("[LoRaAdaptive] Done: SF%d, BW=%.1fkHz, CR=4/%d, PWR=%ddBm\n",
                     appliedParams.spreadingFactor,
                     appliedParams.bandwidth / 1E3,
                     appliedParams.codingRate,
                     appliedParams.txPower);
    }
}

void LoRaAdaptiveController::adjustByRSSI(int16_t rssi) {
    currentRSSI = rssi;
    updateHistory(rssi, currentSNR, packetLossRate);
    
    if (rssi < -115) {
        currentQuality = QUALITY_CRITICAL;
    } else if (rssi < -100) {
        currentQuality = QUALITY_POOR;
    } else if (rssi < -85) {
        currentQuality = QUALITY_FAIR;
    } else if (rssi < -70) {
        currentQuality = QUALITY_GOOD;
    } else {
        currentQuality = QUALITY_EXCELLENT;
    }
    
    performAdaptiveAdjustment();
}

void LoRaAdaptiveController::adjustBySNR(float snr) {
    currentSNR = snr;
    updateHistory(currentRSSI, snr, packetLossRate);
    
    if (snr < -5.0f) {
        currentQuality = QUALITY_CRITICAL;
    } else if (snr < 2.5f) {
        currentQuality = QUALITY_POOR;
    } else if (snr < 7.5f) {
        currentQuality = QUALITY_FAIR;
    } else if (snr < 12.5f) {
        currentQuality = QUALITY_GOOD;
    } else {
        currentQuality = QUALITY_EXCELLENT;
    }
    
    performAdaptiveAdjustment();
}

void LoRaAdaptiveController::adjustByPacketLoss(float lossRate) {
    packetLossRate = lossRate;
    updateHistory(currentRSSI, currentSNR, lossRate);
    
    if (lossRate > 0.3f) {
        currentQuality = QUALITY_CRITICAL;
    } else if (lossRate > 0.15f) {
        currentQuality = QUALITY_POOR;
    } else if (lossRate > 0.05f) {
        currentQuality = QUALITY_FAIR;
    } else if (lossRate > 0.01f) {
        currentQuality = QUALITY_GOOD;
    } else {
        currentQuality = QUALITY_EXCELLENT;
    }
    
    performAdaptiveAdjustment();
}

void LoRaAdaptiveController::adjustByBattery(uint8_t level) {
    batteryLevel = level;
    
    if (level < 10) {
        Serial.printf("[LoRaAdaptive] Low battery (%d%%), reducing power\n", level);
        LoRaParams lowPower = loraParamsManager.getCurrentParams();
        lowPower.txPower = 2;
        lowPower.spreadingFactor = 7;
        loraParamsManager.applyParams(lowPower);
    }
}

void LoRaAdaptiveController::adjustByFrequency(uint32_t interval) {
    reportInterval = interval;
    
    if (interval < 500) {
        Serial.printf("[LoRaAdaptive] High frequency mode (%.1fs interval)\n", interval / 1000.0f);
        LoRaParams fastParams = loraParamsManager.getCurrentParams();
        if (fastParams.spreadingFactor > 8) {
            fastParams.spreadingFactor = 8;
        }
        fastParams.bandwidth = 250000;
        loraParamsManager.applyParams(fastParams);
    }
}

TransmissionQuality LoRaAdaptiveController::getCurrentQuality() const {
    return currentQuality;
}

const char* LoRaAdaptiveController::getQualityString() const {
    switch (currentQuality) {
        case QUALITY_EXCELLENT: return "Excellent";
        case QUALITY_GOOD: return "Good";
        case QUALITY_FAIR: return "Fair";
        case QUALITY_POOR: return "Poor";
        case QUALITY_CRITICAL: return "Critical";
        default: return "Unknown";
    }
}

void LoRaAdaptiveController::enable() {
    enabled = true;
    lastAdjustTime = millis();
    Serial.println("[LoRaAdaptive] Enabled");
}

void LoRaAdaptiveController::disable() {
    enabled = false;
    Serial.println("[LoRaAdaptive] Disabled");
}

void LoRaAdaptiveController::recordPacketSent() {
    totalPackets++;
}

void LoRaAdaptiveController::recordPacketReceived() {
    successfulPackets++;
    if (totalPackets > 0) {
        packetLossRate = 1.0f - (float)successfulPackets / totalPackets;
    }
}

void LoRaAdaptiveController::recordPacketLost() {
    lostPackets++;
    totalPackets++;
    if (totalPackets > 0) {
        packetLossRate = (float)lostPackets / totalPackets;
    }
}

void LoRaAdaptiveController::resetStatistics() {
    totalPackets = 0;
    lostPackets = 0;
    successfulPackets = 0;
    packetLossRate = 0.0f;
    historyCount = 0;
    historyIndex = 0;
    Serial.println("[LoRaAdaptive] Statistics reset");
}

void LoRaAdaptiveController::printStatistics() const {
    Serial.println("[LoRaAdaptive] Statistics:");
    Serial.printf("  Total: %lu, Success: %lu, Lost: %lu\n", totalPackets, successfulPackets, lostPackets);
    Serial.printf("  Loss rate: %.2f%%\n", packetLossRate * 100);
    Serial.printf("  Avg RSSI: %ddBm, Avg SNR: %.2fdB\n", getAverageRSSI(), getAverageSNR());
    Serial.printf("  Quality: %s\n", getQualityString());
}

void LoRaAdaptiveController::printCurrentStatus() const {
    Serial.println("[LoRaAdaptive] Status:");
    Serial.printf("  Enabled: %s\n", enabled ? "Yes" : "No");
    Serial.printf("  RSSI: %ddBm, SNR: %.2fdB\n", currentRSSI, currentSNR);
    Serial.printf("  Loss rate: %.2f%%\n", packetLossRate * 100);
    Serial.printf("  Battery: %d%%\n", batteryLevel);
    Serial.printf("  Report interval: %lums\n", reportInterval);
    Serial.printf("  Quality: %s\n", getQualityString());
}
