#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "battery_monitor.h"

enum PowerMode {
    POWER_MODE_ACTIVE = 0,
    POWER_MODE_LISTENING = 1,
    POWER_MODE_LIGHT_SLEEP = 2,
    POWER_MODE_DEEP_SLEEP = 3
};

struct PowerConfig {
    uint32_t sleepDuration;
    uint32_t wakeInterval;
    bool wifiEnabled;
    bool loraEnabled;
    bool sensorEnabled;
    uint8_t cpuFreq;
    int8_t txPower;
};

struct BatteryInfo {
    float voltage;
    uint8_t percentage;
    bool isCharging;
    uint32_t remainingCapacity;
    float estimatedHours;
};

class PowerManager {
private:
    PowerMode currentMode;
    PowerConfig config;
    BatteryInfo batteryInfo;
    BatteryMonitor batteryMonitor;
    bool initialized;
    uint64_t sleepStartTime;
    uint32_t wakeCount;
    uint32_t sleepAccumulated;
    uint8_t wakeupPin;
    bool externalWakeupEnabled;

    void applyCpuFrequency();
    void applyTxPower();
    void saveState();
    void restoreState();
    float calculatePowerBudget();
    void updateBatteryInfo();

public:
    PowerManager();
    bool begin();
    void setMode(PowerMode mode);
    void enterSleep();
    void wakeUp();
    BatteryInfo getBatteryInfo();
    PowerMode getCurrentMode() const { return currentMode; }
    float getPowerBudget();
    void setWakeupPin(uint8_t pin);
    void enableExternalWakeup(uint8_t pin);
    void disableExternalWakeup();
    void configureTimerWakeup(uint64_t durationUs);
    uint32_t getSleepDuration() const { return sleepAccumulated; }
    uint32_t getWakeCount() const { return wakeCount; }
    bool isInitialized() const { return initialized; }
};

extern PowerManager powerManager;

extern "C" uint32_t RTC_RODATA_ATTR rtcWakeCount;
extern "C" uint32_t RTC_RODATA_ATTR rtcSleepAccumulated;
extern "C" uint8_t RTC_RODATA_ATTR rtcPowerMode;

#endif
