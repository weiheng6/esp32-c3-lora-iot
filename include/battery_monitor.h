#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <Arduino.h>

class BatteryMonitor {
private:
    uint8_t adcPin;
    uint16_t fullVoltage;
    uint16_t emptyVoltage;
    uint32_t capacityMah;
    uint32_t currentConsumptionUa;
    bool initialized;
    float lastVoltage;
    uint8_t lastPercentage;
    unsigned long lastReadTime;

    uint8_t voltageToPercentage(float voltage);
    float calculateEstimatedRuntime(uint8_t percentage);

public:
    BatteryMonitor();
    bool begin(uint8_t adcPin = 0);
    float readVoltage();
    uint8_t readPercentage();
    bool isLowBattery(uint8_t threshold = 20);
    bool isCharging();
    float getEstimatedRuntime();
    void calibrate(uint16_t fullV, uint16_t emptyV, uint32_t capacityMah);
    void setCurrentConsumption(uint32_t consumptionUa);
    bool isInitialized() const { return initialized; }
};

#endif
