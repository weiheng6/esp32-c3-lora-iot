#include "power_manager.h"
#include "config.h"
#include "wifi_manager.h"
#include "lora_manager.h"
#include "sensor.h"
#include <Preferences.h>

uint32_t RTC_RODATA_ATTR rtcWakeCount = 0;
uint32_t RTC_RODATA_ATTR rtcSleepAccumulated = 0;
uint8_t RTC_RODATA_ATTR rtcPowerMode = POWER_MODE_ACTIVE;

static Preferences powerPreferences;

PowerManager::PowerManager() 
    : currentMode(POWER_MODE_ACTIVE)
    , config()
    , batteryInfo()
    , batteryMonitor()
    , initialized(false)
    , sleepStartTime(0)
    , wakeCount(0)
    , sleepAccumulated(0)
    , wakeupPin(0)
    , externalWakeupEnabled(false) {
    config.sleepDuration = 60000000;
    config.wakeInterval = 300000;
    config.wifiEnabled = true;
    config.loraEnabled = true;
    config.sensorEnabled = true;
    config.cpuFreq = 80;
    config.txPower = 0;
    
    batteryInfo.voltage = 0.0f;
    batteryInfo.percentage = 100;
    batteryInfo.isCharging = false;
    batteryInfo.remainingCapacity = 0;
    batteryInfo.estimatedHours = 0.0f;
}

bool PowerManager::begin() {
    if (initialized) {
        return true;
    }
    
    if (!powerPreferences.begin("power_mgr", false)) {
        Serial.println("[PowerManager] Failed to open preferences");
        return false;
    }
    
    if (!batteryMonitor.begin()) {
        Serial.println("[PowerManager] Battery monitor init failed");
    }
    
    config.sleepDuration = powerPreferences.getUInt("sleep_dur", 60000000);
    config.wakeInterval = powerPreferences.getUInt("wake_int", 300000);
    config.wifiEnabled = powerPreferences.getBool("wifi_en", true);
    config.loraEnabled = powerPreferences.getBool("lora_en", true);
    config.sensorEnabled = powerPreferences.getBool("sensor_en", true);
    config.cpuFreq = powerPreferences.getUChar("cpu_freq", 80);
    config.txPower = powerPreferences.getChar("tx_power", 0);
    
    wakeCount = rtcWakeCount;
    sleepAccumulated = rtcSleepAccumulated;
    currentMode = static_cast<PowerMode>(rtcPowerMode);
    
    esp_sleep_wakeup_cause_t wakeupCause = esp_sleep_get_wakeup_cause();
    if (wakeupCause != ESP_SLEEP_WAKEUP_UNDEFINED) {
        wakeCount++;
        uint64_t wakeTime = esp_timer_get_time();
        if (sleepStartTime > 0) {
            sleepAccumulated += (wakeTime - sleepStartTime) / 1000;
        }
        Serial.printf("[PowerManager] Wakeup #%u, accumulated sleep: %ums\n", 
                      wakeCount, sleepAccumulated);
    }
    
    updateBatteryInfo();
    applyCpuFrequency();
    applyTxPower();
    
    initialized = true;
    Serial.printf("[PowerManager] Initialized in mode: %d\n", currentMode);
    return true;
}

void PowerManager::setMode(PowerMode mode) {
    if (currentMode == mode) {
        return;
    }
    
    Serial.printf("[PowerManager] Switching from mode %d to %d\n", currentMode, mode);
    
    if (mode == POWER_MODE_LIGHT_SLEEP || mode == POWER_MODE_DEEP_SLEEP) {
        saveState();
    }
    
    currentMode = mode;
    rtcPowerMode = static_cast<uint8_t>(mode);
    
    switch (mode) {
        case POWER_MODE_ACTIVE:
            config.cpuFreq = 80;
            config.txPower = 20;
            config.wifiEnabled = true;
            config.loraEnabled = true;
            config.sensorEnabled = true;
            setCpuFrequencyMhz(80);
            break;
            
        case POWER_MODE_LISTENING:
            config.cpuFreq = 40;
            config.txPower = 10;
            config.wifiEnabled = true;
            config.loraEnabled = true;
            config.sensorEnabled = false;
            setCpuFrequencyMhz(40);
            break;
            
        case POWER_MODE_LIGHT_SLEEP:
            config.cpuFreq = 10;
            config.txPower = 0;
            config.wifiEnabled = false;
            config.loraEnabled = true;
            config.sensorEnabled = false;
            setCpuFrequencyMhz(10);
            break;
            
        case POWER_MODE_DEEP_SLEEP:
            config.cpuFreq = 0;
            config.txPower = 0;
            config.wifiEnabled = false;
            config.loraEnabled = false;
            config.sensorEnabled = false;
            break;
    }
    
    applyCpuFrequency();
    applyTxPower();
    
    powerPreferences.putUChar("power_mode", rtcPowerMode);
}

void PowerManager::applyCpuFrequency() {
    if (config.cpuFreq > 0) {
        setCpuFrequencyMhz(config.cpuFreq);
        Serial.printf("[PowerManager] CPU frequency set to %u MHz\n", config.cpuFreq);
    }
}

void PowerManager::applyTxPower() {
    if (config.txPower > 0 && config.wifiEnabled) {
        WiFi.setTxPower(static_cast<wifi_power_t>(config.txPower));
        Serial.printf("[PowerManager] TX Power set to %d\n", config.txPower);
    }
}

void PowerManager::saveState() {
    rtcWakeCount = wakeCount;
    rtcSleepAccumulated = sleepAccumulated;
    rtcPowerMode = static_cast<uint8_t>(currentMode);
    
    powerPreferences.putUInt("wake_count", wakeCount);
    powerPreferences.putUInt("sleep_acc", sleepAccumulated);
    powerPreferences.putUChar("power_mode", rtcPowerMode);
    
    Serial.println("[PowerManager] State saved to RTC memory");
}

void PowerManager::restoreState() {
    wakeCount = rtcWakeCount;
    sleepAccumulated = rtcSleepAccumulated;
    currentMode = static_cast<PowerMode>(rtcPowerMode);
    
    Serial.printf("[PowerManager] State restored: wake #%u, sleep %ums\n", 
                  wakeCount, sleepAccumulated);
}

void PowerManager::enterSleep() {
    if (currentMode == POWER_MODE_ACTIVE) {
        Serial.println("[PowerManager] Cannot sleep in ACTIVE mode");
        return;
    }
    
    Serial.printf("[PowerManager] Entering %s...\n", 
                  currentMode == POWER_MODE_DEEP_SLEEP ? "DEEP_SLEEP" : "LIGHT_SLEEP");
    
    if (config.wifiEnabled == false) {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    }
    
    sleepStartTime = esp_timer_get_time();
    
    saveState();
    
    if (currentMode == POWER_MODE_DEEP_SLEEP) {
        esp_sleep_enable_timer_wakeup(config.sleepDuration);
        if (externalWakeupEnabled) {
            esp_deep_sleep_enable_gpio_wakeup(1ULL << wakeupPin, ESP_GPIO_WAKEUP_GPIO_LOW);
        }
        esp_deep_sleep_start();
    } else {
        esp_sleep_enable_timer_wakeup(config.sleepDuration);
        esp_light_sleep_start();
    }
}

void PowerManager::wakeUp() {
    Serial.println("[PowerManager] Waking up...");
    
    esp_sleep_wakeup_cause_t wakeupCause = esp_sleep_get_wakeup_cause();
    
    switch (wakeupCause) {
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("[PowerManager] Wakeup reason: Timer");
            break;
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("[PowerManager] Wakeup reason: EXT0");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            Serial.println("[PowerManager] Wakeup reason: EXT1");
            break;
        case ESP_SLEEP_WAKEUP_GPIO:
            Serial.println("[PowerManager] Wakeup reason: GPIO");
            break;
        default:
            Serial.printf("[PowerManager] Wakeup reason: %d\n", wakeupCause);
            break;
    }
    
    restoreState();
    wakeCount++;
    rtcWakeCount = wakeCount;
    
    if (config.wifiEnabled) {
        wifiManager.connect();
    }
    
    if (config.loraEnabled) {
        loraManager.begin();
    }
    
    updateBatteryInfo();
    
    Serial.printf("[PowerManager] Wakeup #%u complete\n", wakeCount);
}

BatteryInfo PowerManager::getBatteryInfo() {
    updateBatteryInfo();
    return batteryInfo;
}

void PowerManager::updateBatteryInfo() {
    batteryInfo.voltage = batteryMonitor.readVoltage();
    batteryInfo.percentage = batteryMonitor.readPercentage();
    batteryInfo.isCharging = batteryMonitor.isCharging();
    batteryInfo.estimatedHours = batteryMonitor.getEstimatedRuntime();
    batteryInfo.remainingCapacity = (batteryInfo.percentage * 1000) / 100;
}

float PowerManager::calculatePowerBudget() {
    float baseCurrent = 0.0f;
    
    switch (currentMode) {
        case POWER_MODE_ACTIVE:
            baseCurrent = 28000.0f;
            if (config.wifiEnabled) baseCurrent += 15000.0f;
            if (config.loraEnabled) baseCurrent += 5000.0f;
            if (config.sensorEnabled) baseCurrent += 500.0f;
            break;
            
        case POWER_MODE_LISTENING:
            baseCurrent = 15000.0f;
            if (config.loraEnabled) baseCurrent += 3000.0f;
            break;
            
        case POWER_MODE_LIGHT_SLEEP:
            baseCurrent = 1500.0f;
            if (config.loraEnabled) baseCurrent += 500.0f;
            break;
            
        case POWER_MODE_DEEP_SLEEP:
            baseCurrent = 10.0f;
            break;
    }
    
    return baseCurrent;
}

float PowerManager::getPowerBudget() {
    return calculatePowerBudget();
}

void PowerManager::setWakeupPin(uint8_t pin) {
    wakeupPin = pin;
    Serial.printf("[PowerManager] Wakeup pin set to GPIO %d\n", pin);
}

void PowerManager::enableExternalWakeup(uint8_t pin) {
    wakeupPin = pin;
    externalWakeupEnabled = true;
    pinMode(pin, INPUT_PULLUP);
    
    esp_deep_sleep_enable_gpio_wakeup(1ULL << pin, ESP_GPIO_WAKEUP_GPIO_LOW);
    Serial.printf("[PowerManager] External wakeup enabled on GPIO %d\n", pin);
}

void PowerManager::disableExternalWakeup() {
    externalWakeupEnabled = false;
    Serial.println("[PowerManager] External wakeup disabled");
}

void PowerManager::configureTimerWakeup(uint64_t durationUs) {
    config.sleepDuration = durationUs;
    powerPreferences.putUInt("sleep_dur", durationUs);
    
    esp_err_t err = esp_sleep_enable_timer_wakeup(durationUs);
    if (err == ESP_OK) {
        Serial.printf("[PowerManager] Timer wakeup configured: %llu us\n", durationUs);
    } else {
        Serial.printf("[PowerManager] Timer wakeup config failed: %d\n", err);
    }
}

PowerManager powerManager;
