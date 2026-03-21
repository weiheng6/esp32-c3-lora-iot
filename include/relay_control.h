#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

class RelayControl {
private:
  bool state;
  
public:
  RelayControl();
  void begin();
  void turnOn();
  void turnOff();
  void setState(bool newState);
  bool getState() const { return state; }
  void saveState();
  void loadState();
  void toggle();
};

extern RelayControl relayControl;

#endif // RELAY_CONTROL_H
