#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <Arduino.h>

// 日志级别定义
enum LogLevel {
  LOG_NONE = 0,    // 不打印日志
  LOG_ERROR = 1,   // 仅打印错误
  LOG_INFO = 2,    // 打印信息和错误
  LOG_DEBUG = 3    // 打印所有日志（调试模式）
};

class LogManager {
public:
  // 初始化日志管理器
  static void init(LogLevel level = LOG_DEBUG);
  
  // 设置日志级别
  static void setLogLevel(LogLevel level);
  
  // 获取当前日志级别
  static LogLevel getLogLevel();
  
  // 打印错误日志 - 总是打印
  static void error(const char* message);
  static void errorf(const char* format, ...);
  
  // 打印信息日志 - 在 INFO 及以上级别打印
  static void info(const char* message);
  static void infof(const char* format, ...);
  
  // 打印调试日志 - 仅在 DEBUG 级别打印
  static void debug(const char* message);
  static void debugf(const char* format, ...);
  
  // 【关键】打印命令配置日志 - 用户通过 MQTT 发送的设置命令
  // 这个日志级别在任何模式下都会打印，帮助用户理解发送了什么配置
  static void command(const char* message);
  static void commandf(const char* format, ...);
  
  // 【关键】打印条件触发日志 - 条件满足或定时触发事件
  // 这个日志级别在任何模式下都会打印，帮助用户理解自动控制的原因
  static void trigger(const char* message);
  static void triggerf(const char* format, ...);

private:
  static LogLevel currentLevel;
  static const char* levelPrefix[4];
};

// 为了方便使用，定义宏快捷方式
#define LOG_ERROR(msg) LogManager::error(msg)
#define LOG_ERRORF(...) LogManager::errorf(__VA_ARGS__)

#define LOG_INFO(msg) LogManager::info(msg)
#define LOG_INFOF(...) LogManager::infof(__VA_ARGS__)

#define LOG_DEBUG(msg) LogManager::debug(msg)
#define LOG_DEBUGF(...) LogManager::debugf(__VA_ARGS__)

#define LOG_CMD(msg) LogManager::command(msg)
#define LOG_CMDF(...) LogManager::commandf(__VA_ARGS__)

#define LOG_TRIGGER(msg) LogManager::trigger(msg)
#define LOG_TRIGGERF(...) LogManager::triggerf(__VA_ARGS__)

#endif // LOG_MANAGER_H
