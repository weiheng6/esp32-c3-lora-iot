#ifndef CONDITION_CONTROL_H
#define CONDITION_CONTROL_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

class ConditionControl {
private:
  // 条件控制开关
  bool enabled;
  
  // 单阈值模式
  float tempThreshold;
  float humiThreshold;
  int tempCompareOp;
  int humiCompareOp;
  bool tempConditionEnabled;
  bool humiConditionEnabled;
  bool conditionLogicAnd;
  
  // 滞回控制模式
  bool useHysteresis;
  float tempHighThreshold;
  float tempLowThreshold;
  float humiHighThreshold;
  float humiLowThreshold;

public:
  ConditionControl();
  void begin();
  void loadConfig();
  void saveConfig();
  
  // 配置方法
  void setEnabled(bool value);
  void setTempCondition(bool enabled, float threshold, int compareOp);
  void setHumiCondition(bool enabled, float threshold, int compareOp);
  void setHysteresis(bool enabled, float tempHigh, float tempLow, float humiHigh, float humiLow);
  void setLogicMode(bool andMode);
  
  // 条件检查
  bool checkConditions(float temperature, float humidity);
  
  // 获取配置
  bool isEnabled() const { return enabled; }
  bool getHysteresisMode() const { return useHysteresis; }
  bool getTempConditionEnabled() const { return tempConditionEnabled; }
  bool getHumiConditionEnabled() const { return humiConditionEnabled; }
  float getTempThreshold() const { return tempThreshold; }
  float getHumiThreshold() const { return humiThreshold; }
  
  // JSON 序列化
  String toJSON() const;
};

extern ConditionControl conditionControl;

#endif // CONDITION_CONTROL_H
