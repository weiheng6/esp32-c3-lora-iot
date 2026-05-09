#ifndef CONFIG_API_H
#define CONFIG_API_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "lora_params.h"
#include "power_manager.h"
#include "pin_config.h"

struct DeviceConfig {
    char deviceName[32];
    char deviceId[32];
    uint8_t hardwareVersion;
    uint32_t firmwareVersion;
    uint32_t sensorType;
    uint16_t acquisitionInterval;
    uint16_t reportInterval;
    bool autoReportEnabled;
    bool lowPowerMode;

    DeviceConfig() {
        memset(deviceName, 0, sizeof(deviceName));
        memset(deviceId, 0, sizeof(deviceId));
        hardwareVersion = 1;
        firmwareVersion = 100;
        sensorType = 0;
        acquisitionInterval = 1000;
        reportInterval = 5000;
        autoReportEnabled = true;
        lowPowerMode = false;
    }
};

struct NetworkConfig {
    char wifiSsid[32];
    char wifiPassword[64];
    char mqttServer[64];
    uint16_t mqttPort;
    char mqttUsername[32];
    char mqttPassword[64];
    char mqttTopic[64];
    uint8_t transmissionMode;
    bool mqttEnabled;
    bool loraEnabled;

    NetworkConfig() {
        memset(wifiSsid, 0, sizeof(wifiSsid));
        memset(wifiPassword, 0, sizeof(wifiPassword));
        memset(mqttServer, 0, sizeof(mqttServer));
        mqttPort = 1883;
        memset(mqttUsername, 0, sizeof(mqttUsername));
        memset(mqttPassword, 0, sizeof(mqttPassword));
        memset(mqttTopic, 0, sizeof(mqttTopic));
        transmissionMode = 0;
        mqttEnabled = true;
        loraEnabled = true;
    }
};

struct ControlStrategyConfig {
    bool conditionEnabled;
    uint8_t conditionLogic;
    bool timerEnabled;
    uint8_t timeSlotCount;
    bool manualModeEnabled;
    uint8_t relayState;

    ControlStrategyConfig() {
        conditionEnabled = false;
        conditionLogic = 0;
        timerEnabled = false;
        timeSlotCount = 0;
        manualModeEnabled = false;
        relayState = 0;
    }
};

struct SystemConfig {
    uint8_t logLevel;
    bool otaAutoCheck;
    char otaUrl[128];
    bool securityEnabled;
    char adminPassword[32];

    SystemConfig() {
        logLevel = 1;
        otaAutoCheck = false;
        memset(otaUrl, 0, sizeof(otaUrl));
        securityEnabled = false;
        memset(adminPassword, 0, sizeof(adminPassword));
    }
};

class ConfigAPI {
private:
    static ConfigAPI* instance;
    Preferences preferences;

    DeviceConfig deviceConfig;
    NetworkConfig networkConfig;
    ControlStrategyConfig controlConfig;
    SystemConfig systemConfig;
    LoRaParams loraParams;
    LoRaAdaptiveConfig loraAdaptiveConfig;
    PowerConfig powerConfig;
    PinConfig pinConfigs[20];

    bool loadDeviceConfig();
    bool saveDeviceConfig();
    bool loadNetworkConfig();
    bool saveNetworkConfig();
    bool loadControlConfig();
    bool saveControlConfig();
    bool loadSystemConfig();
    bool saveSystemConfig();
    bool loadLoraConfig();
    bool saveLoraConfig();
    bool loadPowerConfig();
    bool savePowerConfig();
    bool loadPinConfig();
    bool savePinConfig();

    String deviceConfigToJson() const;
    String networkConfigToJson() const;
    String loraConfigToJson() const;
    String powerConfigToJson() const;
    String pinConfigToJson() const;
    String controlConfigToJson() const;
    String systemConfigToJson() const;

    bool parseDeviceConfig(const String& json);
    bool parseNetworkConfig(const String& json);
    bool parseLoraConfig(const String& json);
    bool parsePowerConfig(const String& json);
    bool parsePinConfig(const String& json);
    bool parseControlConfig(const String& json);
    bool parseSystemConfig(const String& json);

public:
    ConfigAPI();

    bool begin();
    void end();

    String getDeviceConfig() const;
    bool setDeviceConfig(const String& json);

    String getNetworkConfig() const;
    bool setNetworkConfig(const String& json);

    String getLoraParams() const;
    bool setLoraParams(const String& json);

    String getPowerConfig() const;
    bool setPowerConfig(const String& json);

    String getPinConfig() const;
    bool setPinConfig(const String& json);

    String getControlStrategy() const;
    bool setControlStrategy(const String& json);

    String getSystemConfig() const;
    bool setSystemConfig(const String& json);

    String getTransmissionMode() const;
    bool setTransmissionMode(const String& json);

    String exportAllConfig() const;
    bool importAllConfig(const String& json);

    bool resetToDefaults();

    DeviceConfig getDeviceConfigData() const { return deviceConfig; }
    NetworkConfig getNetworkConfigData() const { return networkConfig; }
    LoRaParams getLoraParamsData() const { return loraParams; }
    PowerConfig getPowerConfigData() const { return powerConfig; }
};

extern ConfigAPI configAPI;

#endif
