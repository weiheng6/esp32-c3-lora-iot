#ifndef CONDITION_CONTROL_H
#define CONDITION_CONTROL_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

// 比较操作符定义
#define COMPARE_GREATER_THAN 1      // >
#define COMPARE_LESS_THAN 2         // <
#define COMPARE_EQUAL 3             // =
#define COMPARE_GREATER_EQUAL 4     // >=
#define COMPARE_LESS_EQUAL 5        // <=
#define COMPARE_NOT_EQUAL 6         // !=

// 传感器类型定义
#define SENSOR_TEMP 0
#define SENSOR_HUMI 1

// 逻辑关系定义
#define LOGIC_AND 0
#define LOGIC_OR 1

// 定时控制结构体
struct TimeSlot {
  bool enabled;
  uint8_t startHour;
  uint8_t startMinute;
  uint8_t endHour;
  uint8_t endMinute;
  bool state;  // true: 开启，false: 关闭
};

// 【新增】单条件结构体
struct Condition {
  bool enabled;
  uint8_t sensorType;      // 0: 温度, 1: 湿度
  uint8_t compareOp;       // 比较操作符
  float threshold;         // 阈值
  
  Condition() : enabled(false), sensorType(SENSOR_TEMP), 
                compareOp(COMPARE_GREATER_THAN), threshold(25.0) {}
  
  bool evaluate(float temperature, float humidity) const {
    if (!enabled) return false;
    
    float value = (sensorType == SENSOR_TEMP) ? temperature : humidity;
    
    switch (compareOp) {
      case COMPARE_GREATER_THAN:      return value > threshold;
      case COMPARE_LESS_THAN:         return value < threshold;
      case COMPARE_EQUAL:             return abs(value - threshold) < 0.1;
      case COMPARE_GREATER_EQUAL:     return value >= (threshold - 0.05);
      case COMPARE_LESS_EQUAL:        return value <= (threshold + 0.05);
      case COMPARE_NOT_EQUAL:         return abs(value - threshold) >= 0.1;
      default:                        return false;
    }
  }
};

// 【新增】条件组结构体 - 表示一组条件和它们的逻辑关系
struct ConditionGroup {
  bool enabled;
  Condition conditions[4];           // 最多 4 个条件
  uint8_t logicMode;                 // 0: AND, 1: OR
  uint8_t conditionCount;            // 实际条件数量
  
  ConditionGroup() : enabled(false), logicMode(LOGIC_AND), conditionCount(0) {}
  
  bool evaluate(float temperature, float humidity) const {
    if (!enabled || conditionCount == 0) return false;
    
    if (logicMode == LOGIC_AND) {
      // 所有条件都必须满足
      for (uint8_t i = 0; i < conditionCount; i++) {
        if (!conditions[i].evaluate(temperature, humidity)) {
          return false;
        }
      }
      return true;
    } else {
      // 至少一个条件满足
      for (uint8_t i = 0; i < conditionCount; i++) {
        if (conditions[i].evaluate(temperature, humidity)) {
          return true;
        }
      }
      return false;
    }
  }
};

class ConditionControl {
private:
  // 【新增】灵活条件控制
  bool enabled;
  ConditionGroup onConditionGroup;   // 继电器开启条件组
  ConditionGroup offConditionGroup;  // 继电器关闭条件组
  
  // 定时控制
  bool timerEnabled;
  TimeSlot timeSlots[8];  // 支持最多 8 个时间段
  
  // 【保留】旧的条件数据（为了兼容性，不删除）
  float tempThreshold;
  float humiThreshold;
  int tempCompareOp;
  int humiCompareOp;
  bool tempConditionEnabled;
  bool humiConditionEnabled;
  bool conditionLogicAnd;
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
  
  // 条件控制配置方法
  void setEnabled(bool value);
  
  // 【新增】灵活条件配置方法
  void setCondition(bool isOnGroup, uint8_t condIndex, bool enabled, uint8_t sensorType, uint8_t compareOp, float threshold);
  void setConditionGroupLogic(bool isOnGroup, uint8_t logicMode);
  void setConditionGroupEnabled(bool isOnGroup, bool enabled);
  void getCondition(bool isOnGroup, uint8_t condIndex, Condition& out) const;
  
  // 【保留】旧的条件方法（兼容性）
  void setTempCondition(bool enabled, float threshold, int compareOp);
  void setHumiCondition(bool enabled, float threshold, int compareOp);
  void setHysteresis(bool enabled, float tempHigh, float tempLow, float humiHigh, float humiLow);
  void setLogicMode(bool andMode);
  
  // 定时控制配置方法
  void setTimerEnabled(bool value);
  void setTimeSlot(uint8_t index, bool enabled, uint8_t startH, uint8_t startM, uint8_t endH, uint8_t endM, bool state);
  void clearTimeSlots();
  TimeSlot getTimeSlot(uint8_t index) const;
  
  // 条件检查
  bool checkConditions(float temperature, float humidity);
  bool checkTimer();
  bool checkAllConditions(float temperature, float humidity);
  
  // 获取配置
  bool isEnabled() const { return enabled; }
  bool isTimerEnabled() const { return timerEnabled; }
  const ConditionGroup& getOnConditionGroup() const { return onConditionGroup; }
  const ConditionGroup& getOffConditionGroup() const { return offConditionGroup; }
  
  // JSON 序列化
  String toJSON() const;
  String getTimerJSON() const;
};

extern ConditionControl conditionControl;

#endif // CONDITION_CONTROL_H
