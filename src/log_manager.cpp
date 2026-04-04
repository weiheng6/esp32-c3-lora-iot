#include "log_manager.h"
#include <cstdarg>

// 初始化静态成员
LogLevel LogManager::currentLevel = LOG_DEBUG;
const char* LogManager::levelPrefix[4] = {
  "",           // LOG_NONE
  "❌",         // LOG_ERROR
  "ℹ️ ",        // LOG_INFO
  "🔧"          // LOG_DEBUG
};

void LogManager::init(LogLevel level) {
  currentLevel = level;
}

void LogManager::setLogLevel(LogLevel level) {
  currentLevel = level;
}

LogLevel LogManager::getLogLevel() {
  return currentLevel;
}

void LogManager::error(const char* message) {
  // 错误日志总是打印
  Serial.print("❌ ");
  Serial.println(message);
}

void LogManager::errorf(const char* format, ...) {
  // 错误日志总是打印
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  Serial.print("❌ ");
  Serial.println(buffer);
}

void LogManager::info(const char* message) {
  if (currentLevel >= LOG_INFO) {
    Serial.print("ℹ️  ");
    Serial.println(message);
  }
}

void LogManager::infof(const char* format, ...) {
  if (currentLevel >= LOG_INFO) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Serial.print("ℹ️  ");
    Serial.println(buffer);
  }
}

void LogManager::debug(const char* message) {
  if (currentLevel >= LOG_DEBUG) {
    Serial.print("🔧 ");
    Serial.println(message);
  }
}

void LogManager::debugf(const char* format, ...) {
  if (currentLevel >= LOG_DEBUG) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Serial.print("🔧 ");
    Serial.println(buffer);
  }
}

void LogManager::command(const char* message) {
  // 命令日志总是打印（用户需要看到他们发送的命令被接收了）
  Serial.print("📝 ");
  Serial.println(message);
}

void LogManager::commandf(const char* format, ...) {
  // 命令日志总是打印
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  Serial.print("📝 ");
  Serial.println(buffer);
}

void LogManager::trigger(const char* message) {
  // 触发日志总是打印（用户需要看到自动控制何时被触发）
  Serial.print("⚡ ");
  Serial.println(message);
}

void LogManager::triggerf(const char* format, ...) {
  // 触发日志总是打印
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  Serial.print("⚡ ");
  Serial.println(buffer);
}
