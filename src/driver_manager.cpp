#include "driver_manager.h"

DriverManager* DriverManager::instance = nullptr;

DriverManager::DriverManager() : initialized(false) {
}

DriverManager::~DriverManager() {
}

DriverManager* DriverManager::getInstance() {
    if (instance == nullptr) {
        instance = new DriverManager();
    }
    return instance;
}

bool DriverManager::begin() {
    if (initialized) {
        return true;
    }
    initialized = true;
    Serial.println("[DriverManager] Initialized");
    return true;
}

void DriverManager::end() {
    if (!initialized) {
        return;
    }
    initialized = false;
    Serial.println("[DriverManager] Stopped");
}

DriverManager driverManager;
