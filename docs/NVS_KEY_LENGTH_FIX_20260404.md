# NVS 密钥长度超限错误修复说明

## 问题描述

在设置条件控制开启/关闭时，每次都会看到大量的 NVS（Non-Volatile Storage）存储错误：

```
[124118][E][Preferences.cpp:185] putInt(): nvs_set_i32 fail: on_group_enabled KEY_TOO_LONG
[124126][E][Preferences.cpp:185] putInt(): nvs_set_i32 fail: on_cond_0_enabled KEY_TOO_LONG
[124135][E][Preferences.cpp:185] putInt(): nvs_set_i32 fail: on_cond_0_sensor KEY_TOO_LONG
...
```

虽然条件控制的功能实际上工作正常，但这些错误信息充斥着日志，严重影响可读性。

## 问题根因

ESP32 的 Preferences 库（基于 NVS）有一个**密钥长度的限制：最多 15 字节**。

在 `condition_control.cpp` 的 `saveConfig()` 和 `loadConfig()` 函数中使用的密钥名称过长：

| 密钥名称 | 长度 | 状态 |
|---------|------|------|
| `on_group_enabled` | 16 字节 | ❌ 超过限制 |
| `on_group_logic` | 14 字节 | ✅ 合法 |
| `on_cond_count` | 13 字节 | ✅ 合法 |
| `on_cond_0_enabled` | 17 字节 | ❌ 超过限制 |
| `on_cond_0_sensor` | 16 字节 | ❌ 超过限制 |
| `on_cond_0_compare` | 17 字节 | ❌ 超过限制 |
| `on_cond_0_threshold` | 18 字节 | ❌ 超过限制 |
| `off_group_enabled` | 17 字节 | ❌ 超过限制 |
| ... | ... | ... |

## 修复方案

### 密钥名称映射

将所有过长的密钥缩短，确保都不超过 15 字节：

```cpp
// 【旧的密钥】-> 【新的短密钥】
on_group_enabled    -> og_en       (5 字节)
on_group_logic      -> og_logic    (8 字节)
on_cond_count       -> og_cnt      (6 字节)

on_cond_0_enabled   -> oc0_en      (6 字节)
on_cond_0_sensor    -> oc0_st      (6 字节)
on_cond_0_compare   -> oc0_cmp     (7 字节)
on_cond_0_threshold -> oc0_th      (6 字节)

off_group_enabled   -> ofg_en      (6 字节)
off_group_logic     -> ofg_logic   (9 字节)
off_cond_count      -> ofg_cnt     (7 字节)

off_cond_0_enabled  -> ofc0_en     (7 字节)
off_cond_0_sensor   -> ofc0_st     (7 字节)
off_cond_0_compare  -> ofc0_cmp    (8 字节)
off_cond_0_threshold-> ofc0_th     (7 字节)
```

### 实现细节

**在 `loadConfig()` 函数中：**
```cpp
// 加载 ON 条件组
onConditionGroup.enabled = preferences.getInt("og_en", 0) == 1;
onConditionGroup.logicMode = preferences.getInt("og_logic", LOGIC_AND);
onConditionGroup.conditionCount = preferences.getInt("og_cnt", 0);

for (int i = 0; i < 4 && i < onConditionGroup.conditionCount; i++) {
  String prefix = "oc" + String(i) + "_";  // oc0_, oc1_, oc2_, oc3_
  onConditionGroup.conditions[i].enabled = preferences.getInt((prefix + "en").c_str(), 0) == 1;
  onConditionGroup.conditions[i].sensorType = preferences.getInt((prefix + "st").c_str(), SENSOR_TEMP);
  onConditionGroup.conditions[i].compareOp = preferences.getInt((prefix + "cmp").c_str(), COMPARE_GREATER_THAN);
  onConditionGroup.conditions[i].threshold = preferences.getFloat((prefix + "th").c_str(), 25.0);
}
```

**在 `saveConfig()` 函数中：**
```cpp
// 保存 ON 条件组
preferences.putInt("og_en", onConditionGroup.enabled ? 1 : 0);
preferences.putInt("og_logic", onConditionGroup.logicMode);
preferences.putInt("og_cnt", onConditionGroup.conditionCount);

for (int i = 0; i < 4; i++) {
  String prefix = "oc" + String(i) + "_";  // oc0_, oc1_, oc2_, oc3_
  preferences.putInt((prefix + "en").c_str(), onConditionGroup.conditions[i].enabled ? 1 : 0);
  preferences.putInt((prefix + "st").c_str(), onConditionGroup.conditions[i].sensorType);
  preferences.putInt((prefix + "cmp").c_str(), onConditionGroup.conditions[i].compareOp);
  preferences.putFloat((prefix + "th").c_str(), onConditionGroup.conditions[i].threshold);
}
```

## 修复结果

✅ **修复前的日志：**
```
[124118][E][Preferences.cpp:185] putInt(): nvs_set_i32 fail: on_group_enabled KEY_TOO_LONG
[124126][E][Preferences.cpp:185] putInt(): nvs_set_i32 fail: on_cond_0_enabled KEY_TOO_LONG
[124135][E][Preferences.cpp:185] putInt(): nvs_set_i32 fail: on_cond_0_sensor KEY_TOO_LONG
✅ 条件配置已保存
```

✅ **修复后的日志：**
```
✅ 条件配置已保存
🔄 条件控制已启用
✅ 条件配置已保存
🌡️ 温度滞回：34.0°C(高) - 29.0°C(低)
```

**所有 KEY_TOO_LONG 错误已消失！**

## 技术要点

1. **NVS 限制**：ESP32 Preferences 库的密钥长度不能超过 15 字节
2. **向后兼容性**：新密钥采用完全不同的命名，旧数据会被新密钥覆盖（因为旧密钥无法保存）
3. **功能保留**：条件控制的所有功能完全保留，没有任何修改
4. **日志改善**：错误日志中不再出现 KEY_TOO_LONG 错误，日志更清晰

## 文件修改清单

- `src/condition_control.cpp`：修改 `loadConfig()` 和 `saveConfig()` 函数中的密钥名称

## 验证

已在实际设备上验证，设置条件控制时：
- ✅ 不再出现 KEY_TOO_LONG 错误
- ✅ 条件控制功能正常工作
- ✅ 温度滞回配置正确保存
- ✅ 设备日志清晰易读

## 部署指南

1. 更新代码版本
2. 重新编译：`pio run -e adafruit_qtpy_esp32c3`
3. 上传到设备：`pio run -e adafruit_qtpy_esp32c3 -t upload`
4. 注意：首次使用新固件时，旧的条件控制配置将不会被加载（因为密钥已改变），需要重新配置条件

## 相关资源

- [ESP32 Preferences 库文档](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html)
- [NVS 密钥长度限制](https://github.com/espressif/arduino-esp32/issues/...)
