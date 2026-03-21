#include "relay_control.h"
#include <Preferences.h>

RelayControl relayControl;
extern Preferences preferences;

RelayControl::RelayControl() : state(RELAY_OFF) {}

void RelayControl::begin() {
  pinMode(RELAY_PIN, OUTPUT);
  loadState();
  digitalWrite(RELAY_PIN, state);
  Serial.println("✅ 继电器初始化成功！");
}

void RelayControl::turnOn() {
  setState(RELAY_ON);
}

void RelayControl::turnOff() {
  setState(RELAY_OFF);
}

void RelayControl::setState(bool newState) {
  if (newState != state) {
    state = newState;
    digitalWrite(RELAY_PIN, state);
    Serial.printf("🔌 继电器已%s\n", state ? "开启" : "关闭");
    saveState();
  }
}

void RelayControl::toggle() {
  setState(!state);
}

void RelayControl::saveState() {
  preferences.begin("sensor_config", false);
  preferences.putBool("relay_state", state);
  preferences.end();
  Serial.println("✅ 继电器状态已保存");
}

void RelayControl::loadState() {
  preferences.begin("sensor_config", true);
  state = preferences.getBool("relay_state", RELAY_OFF);
  preferences.end();
  Serial.printf("✅ 已加载继电器状态：%s\n", state ? "开启" : "关闭");
}
