#include "battery_monitor.h"

BatteryMonitor::BatteryMonitor()
    : adcPin(0)
    , fullVoltage(4200)
    , emptyVoltage(3000)
    , capacityMah(2000)
    , currentConsumptionUa(50000)
    , initialized(false)
    , lastVoltage(0.0f)
    , lastPercentage(100)
    , lastReadTime(0) {
}

bool BatteryMonitor::begin(uint8_t adcPin) {
    this->adcPin = adcPin;
    
    if (adcPin > 0) {
        pinMode(adcPin, INPUT);
    }
    
    initialized = true;
    Serial.printf("[BatteryMonitor] Initialized on ADC pin GPIO %d\n", adcPin);
    return true;
}

float BatteryMonitor::readVoltage() {
    if (!initialized) {
        return 0.0f;
    }
    
    if (adcPin == 0) {
        lastVoltage = 3.3f;
        return lastVoltage;
    }
    
    const int samples = 10;
    uint32_t total = 0;
    
    for (int i = 0; i < samples; i++) {
        total += analogRead(adcPin);
        delayMicroseconds(100);
    }
    
    uint32_t adcValue = total / samples;
    float voltage = (adcValue / 4095.0f) * 3.3f * 2.0f;
    
    lastVoltage = voltage;
    lastReadTime = millis();
    
    return lastVoltage;
}

uint8_t BatteryMonitor::readPercentage() {
    float voltage = readVoltage() * 1000.0f;
    lastPercentage = voltageToPercentage(voltage);
    return lastPercentage;
}

uint8_t BatteryMonitor::voltageToPercentage(float voltage) {
    if (voltage >= fullVoltage) {
        return 100;
    } else if (voltage <= emptyVoltage) {
        return 0;
    }
    
    float normalized = (voltage - emptyVoltage) / (fullVoltage - emptyVoltage);
    normalized = pow(normalized, 0.7f);
    
    uint8_t percentage = (uint8_t)(normalized * 100.0f);
    
    if (percentage > 100) percentage = 100;
    
    return percentage;
}

bool BatteryMonitor::isLowBattery(uint8_t threshold) {
    uint8_t currentPercentage = readPercentage();
    return currentPercentage <= threshold;
}

bool BatteryMonitor::isCharging() {
    return false;
}

float BatteryMonitor::getEstimatedRuntime() {
    uint8_t currentPercentage = readPercentage();
    return calculateEstimatedRuntime(currentPercentage);
}

float BatteryMonitor::calculateEstimatedRuntime(uint8_t percentage) {
    if (percentage == 0 || currentConsumptionUa == 0) {
        return 0.0f;
    }
    
    uint32_t remainingMah = (capacityMah * percentage) / 100;
    float hoursRemaining = (float)remainingMah / (currentConsumptionUa / 1000.0f);
    
    return hoursRemaining;
}

void BatteryMonitor::calibrate(uint16_t fullV, uint16_t emptyV, uint32_t capacityMah) {
    this->fullVoltage = fullV;
    this->emptyVoltage = emptyV;
    this->capacityMah = capacityMah;
    
    Serial.printf("[BatteryMonitor] Calibrated: full=%umV, empty=%umV, capacity=%umAh\n",
                  fullV, emptyV, capacityMah);
}

void BatteryMonitor::setCurrentConsumption(uint32_t consumptionUa) {
    this->currentConsumptionUa = consumptionUa;
    Serial.printf("[BatteryMonitor] Current consumption set to %u uA\n", consumptionUa);
}
