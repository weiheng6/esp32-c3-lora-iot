# 日志输出优化 - 快速使用指南

## 📋 目标

在调试条件控制和定时控制功能时，只看关键日志（设置命令 + 触发事件），隐藏其他无关日志。

## 🎯 实现方案

通过新增的日志管理系统 `LogManager`，将所有日志分为 4 个级别：

```
LOG_DEBUG   🔧  (最详细，包含所有日志)
    ↓
LOG_INFO    ℹ️  (包含信息和错误)
    ↓
LOG_ERROR   ❌  (仅错误)
    ↓
LOG_NONE         (无日志)
```

## 📝 日志类型说明

### 总是打印的日志（无论级别如何）

**📝 设置命令**
```cpp
LOG_CMD("📋 条件控制已启用");
LOG_CMDF("🌡️ 温度条件：%s", enabled ? "启用" : "禁用");
```
❌ **不会被过滤的日志**，用户需要看到他们发送了什么命令

**⚡ 触发事件**
```cpp
LOG_TRIGGER("📵 OFF条件满足 - 继电器将关闭");
LOG_TRIGGERF("🔌 继电器已%s", state ? "开启" : "关闭");
```
❌ **不会被过滤的日志**，用户需要看到条件何时被触发

### 可被过滤的日志（根据级别）

**ℹ️  信息日志**（INFO 级别及以上可见）
```cpp
LOG_INFO("系统启动完成");
```

**🔧 调试日志**（DEBUG 级别可见）
```cpp
LOG_DEBUG("调试信息");
LOG_DEBUGF("变量值：%d", value);
```

**❌ 错误日志**（任何级别都可见）
```cpp
LOG_ERROR("传感器读取失败");
LOG_ERRORF("错误代码：%d", error_code);
```

## 🚀 快速开始

### 1️⃣ 在 `main.cpp` setup() 中初始化

```cpp
void setup() {
  Serial.begin(115200);
  delay(500);
  
  // 初始化日志系统 - 选择一个日志级别
  LogManager::init(LOG_DEBUG);   // 打印所有日志（开发调试用）
  // LogManager::init(LOG_INFO); // 仅打印信息和错误
  // LogManager::init(LOG_ERROR); // 仅打印错误
  // LogManager::init(LOG_NONE);  // 关闭所有日志
  
  // ... 其他初始化代码
}
```

### 2️⃣ 在代码中使用日志宏

```cpp
// 打印设置命令（总是打印）
LOG_CMD("条件配置已保存");
LOG_CMDF("温度阈值：%.1f°C", threshold);

// 打印触发事件（总是打印）
LOG_TRIGGER("条件满足 - 继电器开启");
LOG_TRIGGERF("继电器已%s", state ? "开启" : "关闭");

// 打印信息（如果日志级别 >= INFO）
LOG_INFO("系统就绪");
LOG_INFOF("WiFi 信号强度：%d dBm", rssi);

// 打印调试信息（仅 DEBUG 级别）
LOG_DEBUG("调试信息");
LOG_DEBUGF("循环时间：%lu ms", duration);

// 打印错误（任何级别）
LOG_ERROR("错误发生");
LOG_ERRORF("错误代码：%d", errno);
```

## 📊 日志级别对比

### 场景 1：调试条件控制

**设置**：`LogManager::init(LOG_INFO);`

**日志输出**：
```
📝 📋 条件控制已启用
📝 🌡️ 温度条件：启用 (阈值:25.0°C)
📝 💧 湿度条件：禁用
📝 🔗 逻辑模式：AND
📝 条件配置已保存
⚡ 📵 OFF条件满足 - 继电器将关闭
⚡ 🔌 继电器已关闭
```

**特点**：
- ✅ 所有设置命令可见
- ✅ 所有触发事件可见
- ❌ 无调试细节信息
- ✅ 日志量少，容易阅读

### 场景 2：完整调试

**设置**：`LogManager::init(LOG_DEBUG);`

**日志输出**：
```
🔧 条件控制管理器初始化成功
🔧 已加载条件控制配置
ℹ️ 已加载继电器状态
📝 📋 条件控制已启用
📝 🌡️ 温度条件：启用
📝 条件配置已保存
⚡ 📵 OFF条件满足 - 继电器将关闭
⚡ 🔌 继电器已关闭
🔧 调试信息：...
```

**特点**：
- ✅ 所有信息可见
- ✅ 包含调试细节
- ⚠️  日志量较多

### 场景 3：生产运行

**设置**：`LogManager::init(LOG_ERROR);`

**日志输出**：
```
❌ [仅在出错时打印错误信息]
```

**特点**：
- ❌ 无调试信息
- ✅ 只有错误日志
- ✅ 最小输出，性能最优

## 🔄 动态调整日志级别

如果需要在运行时改变日志级别，可以调用：

```cpp
// 在任何地方调用以改变日志级别
LogManager::setLogLevel(LOG_DEBUG);   // 切换为调试模式
LogManager::setLogLevel(LOG_INFO);    // 切换为信息模式
LogManager::setLogLevel(LOG_ERROR);   // 切换为错误模式
```

## 📁 相关文件

| 文件 | 说明 |
|------|------|
| `include/log_manager.h` | 日志管理器头文件，定义 LogLevel 和 LogManager 类 |
| `src/log_manager.cpp` | 日志管理器实现 |
| `src/main.cpp` | 包含 LogManager 初始化 |
| `src/condition_control.cpp` | 使用 LOG_CMD/LOG_TRIGGER |
| `src/relay_control.cpp` | 使用 LOG_CMD/LOG_TRIGGER |

## 💡 最佳实践

### ✅ 建议

```cpp
// 使用 LOG_CMD 记录用户设置命令
LOG_CMDF("条件控制%s", enabled ? "已启用" : "已禁用");

// 使用 LOG_TRIGGER 记录条件触发
LOG_TRIGGER("ON条件满足");

// 使用 LOG_DEBUG 记录中间状态
LOG_DEBUG("开始处理任务");

// 使用 LOG_ERROR 记录错误情况
LOG_ERROR("I2C 读取失败");
```

### ❌ 避免

```cpp
// 不要为初始化信息过度报告
// LOG_INFO("初始化 WiFi..."); // ❌ 太啰嗦

// 不要记录频繁的循环状态（会淹没日志）
// LOG_DEBUG("循环次数：123"); // ❌ 太频繁

// 不要混合使用 Serial.println 和 LOG_*
// Serial.println("..."); // ❌ 不一致
// LOG_INFO("..."); // ✅ 统一使用 LOG_*
```

## 🧪 测试验证

### 验证 DEBUG 级别
```cpp
LogManager::init(LOG_DEBUG);
// 应该看到所有日志
```

### 验证 INFO 级别
```cpp
LogManager::init(LOG_INFO);
// 应该看到命令、触发、信息、错误日志
// 不应该看到调试日志
```

### 验证 ERROR 级别
```cpp
LogManager::init(LOG_ERROR);
// 应该只看到错误日志和命令/触发日志
```

## 🎓 示例代码

### 完整示例

```cpp
#include "log_manager.h"

void setup() {
  Serial.begin(115200);
  
  // 初始化日志系统
  LogManager::init(LOG_DEBUG);
  
  LOG_INFO("系统启动中...");
}

void loop() {
  // 接收到设置命令
  if (receivedCommand()) {
    LOG_CMDF("收到设置命令：%s", command.c_str());
    applyCommand();
    LOG_CMD("命令已应用");
  }
  
  // 条件检查
  if (conditionMet()) {
    LOG_TRIGGER("⚡ 条件已满足，执行动作");
    executeAction();
  }
  
  // 调试信息
  LOG_DEBUGF("当前状态：%d", status);
  
  // 错误处理
  if (error) {
    LOG_ERROR("发生错误！");
  }
}
```

## 📞 常见问题

**Q: 命令日志为什么总是打印？**  
A: 因为用户需要确认他们的设置命令是否被设备接收到了。

**Q: 如何在代码中临时查看所有日志？**  
A: 调用 `LogManager::setLogLevel(LOG_DEBUG);`

**Q: 可以同时输出到文件吗？**  
A: 当前不支持，但可以扩展 LogManager 来添加此功能。

**Q: 修改日志级别后需要重新编译吗？**  
A: 不需要，调用 `setLogLevel()` 即可动态改变。

## 📚 更多信息

详见：`docs/LOG_OPTIMIZATION_SUMMARY_20260404.md`
