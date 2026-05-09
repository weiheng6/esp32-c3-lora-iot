#ifndef DRIVER_MANAGER_H
#define DRIVER_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "pin_config.h"

class DriverManager {
private:
    static DriverManager* instance;
    bool initialized;

public:
    DriverManager();
    ~DriverManager();

    static DriverManager* getInstance();
    bool begin();
    void end();

    bool isInitialized() const { return initialized; }
};

extern DriverManager driverManager;

#endif
