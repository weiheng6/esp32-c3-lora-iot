#include "relay_control.h"
#include "log_manager.h"
#include <Preferences.h>

RelayControl relayControl;
extern Preferences preferences;

RelayControl::RelayControl() : state(RELAY_OFF) {}

void RelayControl::begin() {
  pinMode(RELAY_PIN, OUTPUT);
  loadState();
  digitalWrite(RELAY_PIN, state);
  LOG_DEBUG("继电器初始化成功");
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
    // 状态变化时使用触发日志
    LOG_TRIGGERF("🔌 继电器已%s", state ? "开启" : "关闭");
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
  LOG_DEBUG("继电器状态已保存");
}

void RelayControl::loadState() {
  preferences.begin("sensor_config", true);
  state = preferences.getBool("relay_state", RELAY_OFF);
  preferences.end();
  LOG_DEBUG("已加载继电器状态");
}
