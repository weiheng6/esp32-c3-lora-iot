#include "condition_control.h"
#include "relay_control.h"
#include <ArduinoJson.h>
#include <Preferences.h>

ConditionControl conditionControl;
extern Preferences preferences;
extern RelayControl relayControl;

ConditionControl::ConditionControl()
    : enabled(false), tempThreshold(25.0), humiThreshold(60.0),
      tempCompareOp(COMPARE_GREATER_THAN), humiCompareOp(COMPARE_GREATER_THAN),
      tempConditionEnabled(false), humiConditionEnabled(false), conditionLogicAnd(true),
      useHysteresis(false), tempHighThreshold(28.0), tempLowThreshold(26.0),
      humiHighThreshold(70.0), humiLowThreshold(60.0) {}

void ConditionControl::begin() {
  loadConfig();
  Serial.println("✅ 条件控制管理器初始化成功");
}

void ConditionControl::loadConfig() {
  preferences.begin("cond_ctrl", true);
  enabled = preferences.getInt("enabled", 0) == 1;
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
  preferences.end();
  
  Serial.printf("✅ 已加载条件控制配置：%s\n", enabled ? "已启用" : "已禁用");
}

void ConditionControl::saveConfig() {
  preferences.begin("cond_ctrl", false);
  preferences.putInt("enabled", enabled ? 1 : 0);
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
  preferences.end();
  
  Serial.println("✅ 条件配置已保存");
}

void ConditionControl::setEnabled(bool value) {
  enabled = value;
  saveConfig();
}

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
    return false;
  }
  
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
        tempConditionMet = relayControl.getState();
      }
    }
    
    if (humiConditionEnabled) {
      if (humidity > humiHighThreshold) {
        humiConditionMet = true;
      } else if (humidity < humiLowThreshold) {
        humiConditionMet = false;
      } else {
        humiConditionMet = relayControl.getState();
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
  
  Serial.printf("📋 条件判断结果：%s（%s逻辑）\n", finalCondition ? "满足" : "不满足",
                conditionLogicAnd ? "AND" : "OR");
  
  return finalCondition;
}

String ConditionControl::toJSON() const {
  StaticJsonDocument<300> doc;
  doc["enabled"] = enabled;
  doc["use_hysteresis"] = useHysteresis;
  
  if (useHysteresis) {
    doc["temp"]["enabled"] = tempConditionEnabled;
    doc["temp"]["high_threshold"] = tempHighThreshold;
    doc["temp"]["low_threshold"] = tempLowThreshold;
    
    doc["humi"]["enabled"] = humiConditionEnabled;
    doc["humi"]["high_threshold"] = humiHighThreshold;
    doc["humi"]["low_threshold"] = humiLowThreshold;
  } else {
    doc["temp"]["enabled"] = tempConditionEnabled;
    doc["temp"]["threshold"] = tempThreshold;
    doc["temp"]["compare"] = tempCompareOp;
    
    doc["humi"]["enabled"] = humiConditionEnabled;
    doc["humi"]["threshold"] = humiThreshold;
    doc["humi"]["compare"] = humiCompareOp;
    
    doc["logic"] = conditionLogicAnd ? "and" : "or";
  }
  
  String result;
  serializeJson(doc, result);
  return result;
}
