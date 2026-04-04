#include "condition_control.h"
#include "relay_control.h"
#include "log_manager.h"
#include <ArduinoJson.h>
#include <Preferences.h>

ConditionControl conditionControl;
extern Preferences preferences;
extern RelayControl relayControl;

ConditionControl::ConditionControl()
    : enabled(false), timerEnabled(false),
      tempThreshold(25.0), humiThreshold(60.0),
      tempCompareOp(COMPARE_GREATER_THAN), humiCompareOp(COMPARE_GREATER_THAN),
      tempConditionEnabled(false), humiConditionEnabled(false), conditionLogicAnd(true),
      useHysteresis(false), tempHighThreshold(28.0), tempLowThreshold(26.0),
      humiHighThreshold(70.0), humiLowThreshold(60.0) {
  // 初始化时间段
  for (int i = 0; i < 8; i++) {
    timeSlots[i] = {false, 0, 0, 0, 0, false};
  }
}

void ConditionControl::begin() {
  loadConfig();
  LOG_DEBUG("条件控制管理器初始化成功");
}

void ConditionControl::loadConfig() {
  preferences.begin("cond_ctrl", true);
  
  // 【新增】加载灵活条件控制配置
  enabled = preferences.getInt("enabled", 0) == 1;
  
  // 【新增】加载 ON 条件组（使用短密钥以符合 NVS 15 字节限制）
  onConditionGroup.enabled = preferences.getInt("og_en", 0) == 1;
  onConditionGroup.logicMode = preferences.getInt("og_logic", LOGIC_AND);
  onConditionGroup.conditionCount = preferences.getInt("og_cnt", 0);
  
  for (int i = 0; i < 4 && i < onConditionGroup.conditionCount; i++) {
    String prefix = "oc" + String(i) + "_";
    onConditionGroup.conditions[i].enabled = preferences.getInt((prefix + "en").c_str(), 0) == 1;
    onConditionGroup.conditions[i].sensorType = preferences.getInt((prefix + "st").c_str(), SENSOR_TEMP);
    onConditionGroup.conditions[i].compareOp = preferences.getInt((prefix + "cmp").c_str(), COMPARE_GREATER_THAN);
    onConditionGroup.conditions[i].threshold = preferences.getFloat((prefix + "th").c_str(), 25.0);
  }
  
  // 【新增】加载 OFF 条件组（使用短密钥以符合 NVS 15 字节限制）
  offConditionGroup.enabled = preferences.getInt("ofg_en", 0) == 1;
  offConditionGroup.logicMode = preferences.getInt("ofg_logic", LOGIC_AND);
  offConditionGroup.conditionCount = preferences.getInt("ofg_cnt", 0);
  
  for (int i = 0; i < 4 && i < offConditionGroup.conditionCount; i++) {
    String prefix = "ofc" + String(i) + "_";
    offConditionGroup.conditions[i].enabled = preferences.getInt((prefix + "en").c_str(), 0) == 1;
    offConditionGroup.conditions[i].sensorType = preferences.getInt((prefix + "st").c_str(), SENSOR_TEMP);
    offConditionGroup.conditions[i].compareOp = preferences.getInt((prefix + "cmp").c_str(), COMPARE_GREATER_THAN);
    offConditionGroup.conditions[i].threshold = preferences.getFloat((prefix + "th").c_str(), 25.0);
  }
  
  // 【保留】加载旧的条件配置（兼容性）
  tempThreshold = preferences.getFloat("temp_threshold", 25.0);
  humiThreshold = preferences.getFloat("humi_threshold", 60.0);
  tempCompareOp = preferences.getInt("temp_compare_op", COMPARE_GREATER_THAN);
  humiCompareOp = preferences.getInt("humi_compare_op", COMPARE_GREATER_THAN);
  tempConditionEnabled = preferences.getInt("temp_enabled", 0) == 1;
  humiConditionEnabled = preferences.getInt("humi_enabled", 0) == 1;
  conditionLogicAnd = preferences.getInt("logic_and", 1) == 1;
  useHysteresis = preferences.getInt("hysteresis", 0) == 1;
  tempHighThreshold = preferences.getFloat("temp_high", 28.0);
  tempLowThreshold = preferences.getFloat("temp_low", 26.0);
  humiHighThreshold = preferences.getFloat("humi_high", 70.0);
  humiLowThreshold = preferences.getFloat("humi_low", 60.0);
  timerEnabled = preferences.getInt("timer_enabled", 0) == 1;
  
  // 加载时间段配置
  for (int i = 0; i < 8; i++) {
    String prefix = "timer_" + String(i) + "_";
    timeSlots[i].enabled = preferences.getInt((prefix + "enabled").c_str(), 0) == 1;
    timeSlots[i].startHour = preferences.getInt((prefix + "sh").c_str(), 0);
    timeSlots[i].startMinute = preferences.getInt((prefix + "sm").c_str(), 0);
    timeSlots[i].endHour = preferences.getInt((prefix + "eh").c_str(), 0);
    timeSlots[i].endMinute = preferences.getInt((prefix + "em").c_str(), 0);
    timeSlots[i].state = preferences.getInt((prefix + "state").c_str(), 0) == 1;
  }
  
  preferences.end();
  
  LOG_DEBUG("已加载条件控制配置");
  LOG_DEBUG("已加载定时控制配置");
}

void ConditionControl::saveConfig() {
  preferences.begin("cond_ctrl", false);
  
  // 【新增】保存灵活条件控制配置
  preferences.putInt("enabled", enabled ? 1 : 0);
  
  // 【新增】保存 ON 条件组（使用短密钥以符合 NVS 15 字节限制）
  preferences.putInt("og_en", onConditionGroup.enabled ? 1 : 0);
  preferences.putInt("og_logic", onConditionGroup.logicMode);
  preferences.putInt("og_cnt", onConditionGroup.conditionCount);
  
  for (int i = 0; i < 4; i++) {
    String prefix = "oc" + String(i) + "_";
    preferences.putInt((prefix + "en").c_str(), onConditionGroup.conditions[i].enabled ? 1 : 0);
    preferences.putInt((prefix + "st").c_str(), onConditionGroup.conditions[i].sensorType);
    preferences.putInt((prefix + "cmp").c_str(), onConditionGroup.conditions[i].compareOp);
    preferences.putFloat((prefix + "th").c_str(), onConditionGroup.conditions[i].threshold);
  }
  
  // 【新增】保存 OFF 条件组（使用短密钥以符合 NVS 15 字节限制）
  preferences.putInt("ofg_en", offConditionGroup.enabled ? 1 : 0);
  preferences.putInt("ofg_logic", offConditionGroup.logicMode);
  preferences.putInt("ofg_cnt", offConditionGroup.conditionCount);
  
  for (int i = 0; i < 4; i++) {
    String prefix = "ofc" + String(i) + "_";
    preferences.putInt((prefix + "en").c_str(), offConditionGroup.conditions[i].enabled ? 1 : 0);
    preferences.putInt((prefix + "st").c_str(), offConditionGroup.conditions[i].sensorType);
    preferences.putInt((prefix + "cmp").c_str(), offConditionGroup.conditions[i].compareOp);
    preferences.putFloat((prefix + "th").c_str(), offConditionGroup.conditions[i].threshold);
  }
  
  // 【保留】保存旧的条件配置（兼容性）
  preferences.putFloat("temp_threshold", tempThreshold);
  preferences.putFloat("humi_threshold", humiThreshold);
  preferences.putInt("temp_compare_op", tempCompareOp);
  preferences.putInt("humi_compare_op", humiCompareOp);
  preferences.putInt("temp_enabled", tempConditionEnabled ? 1 : 0);
  preferences.putInt("humi_enabled", humiConditionEnabled ? 1 : 0);
  preferences.putInt("logic_and", conditionLogicAnd ? 1 : 0);
  preferences.putInt("hysteresis", useHysteresis ? 1 : 0);
  preferences.putFloat("temp_high", tempHighThreshold);
  preferences.putFloat("temp_low", tempLowThreshold);
  preferences.putFloat("humi_high", humiHighThreshold);
  preferences.putFloat("humi_low", humiLowThreshold);
  preferences.putInt("timer_enabled", timerEnabled ? 1 : 0);
  
  // 保存时间段配置
  for (int i = 0; i < 8; i++) {
    String prefix = "timer_" + String(i) + "_";
    preferences.putInt((prefix + "enabled").c_str(), timeSlots[i].enabled ? 1 : 0);
    preferences.putInt((prefix + "sh").c_str(), timeSlots[i].startHour);
    preferences.putInt((prefix + "sm").c_str(), timeSlots[i].startMinute);
    preferences.putInt((prefix + "eh").c_str(), timeSlots[i].endHour);
    preferences.putInt((prefix + "em").c_str(), timeSlots[i].endMinute);
    preferences.putInt((prefix + "state").c_str(), timeSlots[i].state ? 1 : 0);
  }
  
  preferences.end();
  
  LOG_CMD("条件配置已保存");
}

void ConditionControl::setEnabled(bool value) {
  enabled = value;
  saveConfig();
}

// 【新增】设置灵活条件
void ConditionControl::setCondition(bool isOnGroup, uint8_t condIndex, bool cEnabled, uint8_t sensorType, uint8_t compareOp, float threshold) {
  if (condIndex >= 4) return;
  
  ConditionGroup* group = isOnGroup ? &onConditionGroup : &offConditionGroup;
  group->conditions[condIndex].enabled = cEnabled;
  group->conditions[condIndex].sensorType = sensorType;
  group->conditions[condIndex].compareOp = compareOp;
  group->conditions[condIndex].threshold = threshold;
  
  if (cEnabled && condIndex >= group->conditionCount) {
    group->conditionCount = condIndex + 1;
  }
  
  saveConfig();
}

// 【新增】设置条件组逻辑
void ConditionControl::setConditionGroupLogic(bool isOnGroup, uint8_t logicMode) {
  ConditionGroup* group = isOnGroup ? &onConditionGroup : &offConditionGroup;
  group->logicMode = logicMode;
  saveConfig();
}

// 【新增】启用/禁用条件组
void ConditionControl::setConditionGroupEnabled(bool isOnGroup, bool cEnabled) {
  ConditionGroup* group = isOnGroup ? &onConditionGroup : &offConditionGroup;
  group->enabled = cEnabled;
  saveConfig();
}

// 【新增】获取条件
void ConditionControl::getCondition(bool isOnGroup, uint8_t condIndex, Condition& out) const {
  if (condIndex >= 4) return;
  const ConditionGroup* group = isOnGroup ? &onConditionGroup : &offConditionGroup;
  out = group->conditions[condIndex];
}

// 【保留】旧方法 - 兼容性
void ConditionControl::setTempCondition(bool cEnabled, float threshold, int compareOp) {
  tempConditionEnabled = cEnabled;
  tempThreshold = threshold;
  tempCompareOp = compareOp;
  saveConfig();
}

void ConditionControl::setHumiCondition(bool cEnabled, float threshold, int compareOp) {
  humiConditionEnabled = cEnabled;
  humiThreshold = threshold;
  humiCompareOp = compareOp;
  saveConfig();
}

void ConditionControl::setHysteresis(bool cEnabled, float tempHigh, float tempLow, float humiHigh, float humiLow) {
  useHysteresis = cEnabled;
  tempHighThreshold = tempHigh;
  tempLowThreshold = tempLow;
  humiHighThreshold = humiHigh;
  humiLowThreshold = humiLow;
  saveConfig();
}

void ConditionControl::setLogicMode(bool andMode) {
  conditionLogicAnd = andMode;
  saveConfig();
}


bool ConditionControl::checkConditions(float temperature, float humidity) {
  if (!enabled) {
    // 【关键修复】当条件控制被禁用时，返回当前继电器状态，不进行任何判断
    return relayControl.getState();
  }
  
  bool currentRelayState = relayControl.getState();
  
  // 【新增】首先检查 OFF 条件组 - 如果满足则关闭
  if (offConditionGroup.enabled && offConditionGroup.evaluate(temperature, humidity)) {
    LOG_TRIGGER("📵 OFF条件满足 - 继电器将关闭");
    return false;
  }
  
  // 【新增】检查 ON 条件组 - 如果满足则打开
  if (onConditionGroup.enabled && onConditionGroup.evaluate(temperature, humidity)) {
    LOG_TRIGGER("✅ ON条件满足 - 继电器将开启");
    return true;
  }
  
  // 如果 ON 条件组未启用，使用旧的条件逻辑（向后兼容）
  if (!onConditionGroup.enabled && !offConditionGroup.enabled) {
    bool tempConditionMet = false;
    bool humiConditionMet = false;
    
    if (useHysteresis) {
      // 滞回控制逻辑
      if (tempConditionEnabled) {
        if (temperature > tempHighThreshold) {
          tempConditionMet = true;
        } else if (temperature < tempLowThreshold) {
          tempConditionMet = false;
        } else {
          tempConditionMet = currentRelayState;
        }
      }
      
      if (humiConditionEnabled) {
        if (humidity > humiHighThreshold) {
          humiConditionMet = true;
        } else if (humidity < humiLowThreshold) {
          humiConditionMet = false;
        } else {
          humiConditionMet = currentRelayState;
        }
      }
    } else {
      // 单阈值控制逻辑
      if (tempConditionEnabled) {
        switch (tempCompareOp) {
          case COMPARE_GREATER_THAN:
            tempConditionMet = (temperature > tempThreshold);
            break;
          case COMPARE_LESS_THAN:
            tempConditionMet = (temperature < tempThreshold);
            break;
          case COMPARE_EQUAL:
            tempConditionMet = (abs(temperature - tempThreshold) < 0.1);
            break;
          case COMPARE_GREATER_EQUAL:
            tempConditionMet = (temperature >= (tempThreshold - 0.05));
            break;
          case COMPARE_LESS_EQUAL:
            tempConditionMet = (temperature <= (tempThreshold + 0.05));
            break;
          case COMPARE_NOT_EQUAL:
            tempConditionMet = (abs(temperature - tempThreshold) >= 0.1);
            break;
        }
      }
      
      if (humiConditionEnabled) {
        switch (humiCompareOp) {
          case COMPARE_GREATER_THAN:
            humiConditionMet = (humidity > humiThreshold);
            break;
          case COMPARE_LESS_THAN:
            humiConditionMet = (humidity < humiThreshold);
            break;
          case COMPARE_EQUAL:
            humiConditionMet = (abs(humidity - humiThreshold) < 0.5);
            break;
          case COMPARE_GREATER_EQUAL:
            humiConditionMet = (humidity >= (humiThreshold - 0.05));
            break;
          case COMPARE_LESS_EQUAL:
            humiConditionMet = (humidity <= (humiThreshold + 0.05));
            break;
          case COMPARE_NOT_EQUAL:
            humiConditionMet = (abs(humidity - humiThreshold) >= 0.5);
            break;
        }
      }
    }
    
    if (!tempConditionEnabled && !humiConditionEnabled) {
      return false;
    }
    
    bool finalCondition = false;
    if (conditionLogicAnd) {
      finalCondition = true;
      if (tempConditionEnabled) finalCondition = finalCondition && tempConditionMet;
      if (humiConditionEnabled) finalCondition = finalCondition && humiConditionMet;
    } else {
      finalCondition = false;
      if (tempConditionEnabled) finalCondition = finalCondition || tempConditionMet;
      if (humiConditionEnabled) finalCondition = finalCondition || humiConditionMet;
    }
    
    if (finalCondition) {
      LOG_TRIGGER("⚡ 旧条件逻辑满足 - 继电器将开启");
    }
    
    return finalCondition;
  }
  
  // 默认保持当前状态
  return currentRelayState;
}

String ConditionControl::toJSON() const {
  StaticJsonDocument<600> doc;
  doc["enabled"] = enabled;
  
  // 【新增】返回灵活条件配置
  doc["on_condition"]["enabled"] = onConditionGroup.enabled;
  doc["on_condition"]["logic"] = onConditionGroup.logicMode == LOGIC_AND ? "and" : "or";
  doc["on_condition"]["count"] = onConditionGroup.conditionCount;
  
  JsonArray onConditions = doc["on_condition"].createNestedArray("conditions");
  for (int i = 0; i < onConditionGroup.conditionCount && i < 4; i++) {
    JsonObject cond = onConditions.createNestedObject();
    cond["enabled"] = onConditionGroup.conditions[i].enabled;
    cond["sensor"] = onConditionGroup.conditions[i].sensorType == SENSOR_TEMP ? "temp" : "humi";
    cond["compare"] = onConditionGroup.conditions[i].compareOp;
    cond["threshold"] = onConditionGroup.conditions[i].threshold;
  }
  
  doc["off_condition"]["enabled"] = offConditionGroup.enabled;
  doc["off_condition"]["logic"] = offConditionGroup.logicMode == LOGIC_AND ? "and" : "or";
  doc["off_condition"]["count"] = offConditionGroup.conditionCount;
  
  JsonArray offConditions = doc["off_condition"].createNestedArray("conditions");
  for (int i = 0; i < offConditionGroup.conditionCount && i < 4; i++) {
    JsonObject cond = offConditions.createNestedObject();
    cond["enabled"] = offConditionGroup.conditions[i].enabled;
    cond["sensor"] = offConditionGroup.conditions[i].sensorType == SENSOR_TEMP ? "temp" : "humi";
    cond["compare"] = offConditionGroup.conditions[i].compareOp;
    cond["threshold"] = offConditionGroup.conditions[i].threshold;
  }
  
  String result;
  serializeJson(doc, result);
  return result;
}

void ConditionControl::setTimerEnabled(bool value) {
  timerEnabled = value;
  saveConfig();
}

void ConditionControl::setTimeSlot(uint8_t index, bool enabled, uint8_t startH, uint8_t startM, uint8_t endH, uint8_t endM, bool state) {
  if (index < 8) {
    timeSlots[index].enabled = enabled;
    timeSlots[index].startHour = startH;
    timeSlots[index].startMinute = startM;
    timeSlots[index].endHour = endH;
    timeSlots[index].endMinute = endM;
    timeSlots[index].state = state;
    saveConfig();
  }
}

void ConditionControl::clearTimeSlots() {
  for (int i = 0; i < 8; i++) {
    timeSlots[i] = {false, 0, 0, 0, 0, false};
  }
  saveConfig();
}

TimeSlot ConditionControl::getTimeSlot(uint8_t index) const {
  if (index < 8) {
    return timeSlots[index];
  }
  return {false, 0, 0, 0, 0, false};
}

bool ConditionControl::checkTimer() {
  if (!timerEnabled) {
    return false;
  }
  
  // 获取当前时间
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  uint8_t currentHour = timeinfo->tm_hour;
  uint8_t currentMinute = timeinfo->tm_min;
  uint8_t currentSecond = timeinfo->tm_sec;
  
  // 转换为分钟便于比较
  uint16_t currentTime = currentHour * 60 + currentMinute;
  
  // 【新增】打印当前时间以便调试
  LOG_DEBUGF("⏰ [当前时间: %02d:%02d:%02d] 开始检查定时器时间段", 
             currentHour, currentMinute, currentSecond);
  
  // 检查所有启用的时间段
  for (int i = 0; i < 8; i++) {
    if (timeSlots[i].enabled) {
      uint16_t startTime = timeSlots[i].startHour * 60 + timeSlots[i].startMinute;
      uint16_t endTime = timeSlots[i].endHour * 60 + timeSlots[i].endMinute;
      
      // 【新增】打印时间段信息用于调试
      LOG_DEBUGF("  🕐 检查时间段[%d]: %02d:%02d ~ %02d:%02d (目标状态:%s)", 
                 i, timeSlots[i].startHour, timeSlots[i].startMinute, 
                 timeSlots[i].endHour, timeSlots[i].endMinute,
                 timeSlots[i].state ? "开启" : "关闭");
      
      // 处理跨越午夜的情况
      if (startTime <= endTime) {
        if (currentTime >= startTime && currentTime < endTime) {
          LOG_TRIGGERF("✅ 当前时间在时间段[%d]内 - 继电器将%s", 
                     i, timeSlots[i].state ? "开启" : "关闭");
          return timeSlots[i].state;
        }
      } else {
        // 跨越午夜
        if (currentTime >= startTime || currentTime < endTime) {
          LOG_TRIGGERF("✅ 当前时间在跨越午夜的时间段[%d]内 - 继电器将%s", 
                     i, timeSlots[i].state ? "开启" : "关闭");
          return timeSlots[i].state;
        }
      }
    }
  }
  
  LOG_DEBUG("⏰ 当前时间不在任何启用的时间段内 - 继电器将关闭");
  return false;
}

bool ConditionControl::checkAllConditions(float temperature, float humidity) {
  // 【关键修复】如果条件控制和定时控制都禁用，则不进行任何自动控制
  if (!enabled && !timerEnabled) {
    // 【新增】打印当前时间以便调试
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    LOG_DEBUGF("⏰ [%02d:%02d:%02d] 条件控制和定时控制都已禁用，保持当前继电器状态", 
               timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    return relayControl.getState();
  }
  
  // 如果定时控制启用，直接返回定时结果
  if (timerEnabled) {
    return checkTimer();
  }
  
  // 否则检查条件控制
  return checkConditions(temperature, humidity);
}

String ConditionControl::getTimerJSON() const {
  StaticJsonDocument<500> doc;
  doc["enabled"] = timerEnabled;
  
  JsonArray slots = doc.createNestedArray("slots");
  for (int i = 0; i < 8; i++) {
    if (timeSlots[i].enabled) {
      JsonObject slot = slots.createNestedObject();
      slot["index"] = i;
      slot["enabled"] = timeSlots[i].enabled;
      
      // 格式化时间为 HH:MM
      char startTime[6], endTime[6];
      snprintf(startTime, sizeof(startTime), "%02d:%02d", timeSlots[i].startHour, timeSlots[i].startMinute);
      snprintf(endTime, sizeof(endTime), "%02d:%02d", timeSlots[i].endHour, timeSlots[i].endMinute);
      
      slot["start_time"] = startTime;
      slot["end_time"] = endTime;
      slot["state"] = timeSlots[i].state;
    }
  }
  
  String result;
  serializeJson(doc, result);
  return result;
}
