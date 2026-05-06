# ESP32-C3 智能温湿度监控系统固件

基于 ESP32-C3 的智能温湿度监控系统，支持 MQTT 云通信、LoRa 远距离通信、条件控制、定时控制和远程固件升级。

---

## 📋 目录

- [功能特点](#功能特点)
- [硬件要求](#硬件要求)
- [软件要求](#软件要求)
- [快速开始](#快速开始)
- [固件升级](#固件升级)
- [MQTT 协议说明](#mqtt-协议说明)
- [Web 界面](#web-界面)
- [条件控制](#条件控制)
- [定时控制](#定时控制)
- [项目结构](#项目结构)
- [配置说明](#配置说明)
- [故障排查](#故障排查)

---

## ✨ 功能特点

| 功能模块 | 说明 |
|----------|------|
| 🌡️ **温湿度采集** | 基于 SHT31 传感器，支持定时采集 |
| 🌐 **WiFi 连接** | 支持 STA 模式连接路由器，AP 模式配置 |
| 📡 **MQTT 通信** | 支持远程数据上报和指令控制 |
| 📡 **LoRa 通信** | 支持远距离节点通信和指令转发 |
| 🔌 **继电器控制** | 支持手动控制和自动控制 |
| 🎛️ **条件控制** | 基于温湿度条件自动控制继电器 |
| ⏰ **定时控制** | 支持8组时间段定时控制 |
| 🔄 **OTA 升级** | 支持 Web 界面 URL升级、文件上传、MQTT远程升级 |
| 📊 **系统监控** | 内存监控、网络状态诊断 |
| 🚨 **故障恢复** | 自动检测重启循环，保障系统稳定 |

---

## 🛠️ 硬件要求

- **主控芯片**: ESP32-C3 (Adafruit QT Py ESP32-C3)
- **温湿度传感器**: SHT31 (I2C)
- **继电器模块**: 单路继电器
- **LoRa 模块**: SX1276 (可选)
- **电源**: 5V/1A 适配器

### 引脚配置

| 功能 | 引脚 | 说明 |
|------|------|------|
| 继电器控制 | GPIO20 | 继电器模块控制引脚 |
| SHT31 SDA | GPIO8 | I2C 数据线 |
| SHT31 SCL | GPIO9 | I2C 时钟线 |
| LoRa SCK | GPIO4 | SPI 时钟 |
| LoRa MISO | GPIO5 | SPI 输入 |
| LoRa MOSI | GPIO6 | SPI 输出 |
| LoRa NSS | GPIO7 | SPI 片选 |
| LoRa RST | GPIO10 | 复位引脚 |

---

## 📦 软件要求

- **开发环境**: PlatformIO
- **框架**: Arduino ESP32
- **依赖库**:
  - PubSubClient (MQTT)
  - ArduinoJson
  - LoRa
  - ArduinoOTA

---

## 🚀 快速开始

### 1. 克隆项目

```bash
git clone <repository-url>
cd esp32-c3
```

### 2. 配置参数

编辑 `src/main.cpp` 中的配置：

```cpp
// MQTT 配置
const char* MQTT_SERVER = "iot.kebaidata.com";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "kbuser";
const char* MQTT_PASSWORD = "kbuserspasswd";

// AP 热点配置
const char* AP_SSID = "Scott_Device";
const char* AP_PASSWORD = "88888888";
```

### 3. 编译固件

```bash
platformio run
```

生成的固件位于：`.pio/build/adafruit_qtpy_esp32c3/firmware.bin`

### 4. 上传固件

```bash
platformio run --target upload
```

### 5. 查看串口日志

```bash
platformio device monitor --port COMx --baud 115200
```

---

## 🔄 固件升级

支持三种固件升级方式，满足不同场景需求：

### 1. Web 界面 URL 升级

适合远程升级场景，固件存放在云端存储（如阿里云OSS）：

1. 连接设备热点（默认：`Scott_Device`，密码：`88888888`）或局域网
2. 访问 `http://192.168.4.1`
3. 进入"OTA升级"页面
4. 选择"URL升级"选项卡
5. 输入固件URL（如：`https://kb-rom-update.oss-cn-beijing.aliyuncs.com/firmware.bin`）
6. 点击"开始升级"
7. 等待升级完成（进度条实时显示）

**特点**:
- ✅ 支持从阿里云OSS等HTTP服务器下载
- ✅ 实时进度显示
- ✅ 升级完成自动重启

### 2. Web 界面文件上传

适合本地开发和调试场景：

1. 连接设备热点或局域网
2. 进入"OTA升级"页面
3. 选择"文件上传"选项卡
4. 点击"选择文件"，选择本地编译好的 `.bin` 文件
5. 点击"上传并升级"
6. 等待升级完成

**特点**:
- ✅ 无需网络即可升级
- ✅ 支持大文件上传
- ✅ 自动验证固件完整性

### 3. MQTT 远程升级

适合批量远程升级场景，无需物理接触设备：

**主题**: `esp32/{device_id}/command`

**升级指令**:
```json
{
  "ota": {
    "url": "https://kb-rom-update.oss-cn-beijing.aliyuncs.com/firmware.bin"
  }
}
```

**查询状态**:
```json
{
  "ota": {
    "status": true
  }
}
```

**响应消息**:

| 情况 | 响应内容 |
|------|----------|
| 升级开始 | `{"status":"ok","message":"OTA update started"}` |
| 升级成功 | `{"status":"ok","message":"OTA update successful, restarting..."}` |
| 升级失败 | `{"status":"error","message":"OTA update failed"}` |
| 查询状态 | `{"status":"ok","ota_status":1,"progress":50}` |

**OTA状态码说明**:
| 状态码 | 含义 |
|--------|------|
| 0 | 空闲 |
| 1 | 升级中 |
| 2 | 完成 |
| 3 | 失败 |

**特点**:
- ✅ 支持远程批量升级
- ✅ 支持升级状态查询
- ✅ 自动验证固件完整性
- ✅ 升级失败自动回滚

### 生成固件

使用 PlatformIO 编译生成固件：

```bash
platformio run
```

生成的固件位于：`.pio/build/adafruit_qtpy_esp32c3/firmware.bin`

### 固件上传到阿里云OSS

1. 登录阿里云OSS控制台
2. 创建或进入Bucket
3. 上传编译好的 `firmware.bin` 文件
4. 设置文件权限为"公共读"
5. 复制文件URL用于OTA升级

---

## 📡 MQTT 协议说明

### 主题结构

| 主题 | 方向 | 说明 |
|------|------|------|
| `esp32/{device_id}/data` | 上行 | 传感器数据上报 |
| `esp32/{device_id}/command` | 下行 | 控制指令 |
| `esp32/{device_id}/response` | 上行 | 指令响应 |

### 数据上报格式

```json
{
  "device_id": "8856A66E4924",
  "temperature": 25.5,
  "humidity": 60.2,
  "relay_state": 1,
  "timestamp": 1715000000
}
```

### 控制指令

#### 继电器控制

```json
{
  "relay": 1
}
```

#### 条件控制

```json
{
  "condition": {
    "enabled": true,
    "use_hysteresis": true,
    "temp": {
      "enabled": true,
      "high_threshold": 30.0,
      "low_threshold": 28.0
    },
    "humi": {
      "enabled": false
    }
  }
}
```

#### 定时控制

```json
{
  "timer": {
    "enabled": true,
    "slots": [
      {
        "index": 0,
        "enabled": true,
        "start_time": "08:00",
        "end_time": "18:00",
        "state": 1
      }
    ]
  }
}
```

#### WiFi 重置

```json
{
  "reset_wifi": true
}
```

---

## 🌐 Web 界面

设备启动后会创建 WiFi 热点，默认配置：

- **SSID**: `Scott_Device`
- **密码**: `88888888`

### 访问方式

1. 连接设备热点
2. 打开浏览器访问 `http://192.168.4.1`

### 功能页面

| 页面 | 功能 |
|------|------|
| 状态监控 | 实时显示温湿度和继电器状态 |
| WiFi 配置 | 配置 WiFi 连接参数 |
| 条件控制 | 设置自动控制条件 |
| 定时控制 | 设置定时开关 |
| OTA 升级 | 固件升级 |
| 系统信息 | 查看设备信息 |

---

## 🎛️ 条件控制

### 滞回控制模式

支持基于温湿度的滞回控制：

```json
{
  "condition": {
    "enabled": true,
    "use_hysteresis": true,
    "temp": {
      "enabled": true,
      "high_threshold": 30.0,
      "low_threshold": 28.0
    },
    "humi": {
      "enabled": true,
      "high_threshold": 70.0,
      "low_threshold": 60.0
    }
  }
}
```

**工作原理**:
- 温度 > 30°C → 继电器开启
- 温度 < 28°C → 继电器关闭
- 湿度 > 70% → 继电器开启
- 湿度 < 60% → 继电器关闭

---

## ⏰ 定时控制

支持最多 8 组时间段定时控制：

```json
{
  "timer": {
    "enabled": true,
    "slots": [
      {
        "index": 0,
        "enabled": true,
        "start_time": "08:00",
        "end_time": "12:00",
        "state": 1
      },
      {
        "index": 1,
        "enabled": true,
        "start_time": "14:00",
        "end_time": "18:00",
        "state": 1
      }
    ]
  }
}
```

**参数说明**:
- `index`: 时间段索引 (0-7)
- `enabled`: 是否启用
- `start_time`: 开始时间 (HH:MM)
- `end_time`: 结束时间 (HH:MM)
- `state`: 目标状态 (0=关闭, 1=开启)

---

## 📁 项目结构

```
esp32-c3/
├── include/              # 头文件目录
│   ├── config.h          # 配置参数定义
│   ├── sensor.h          # 传感器接口
│   ├── wifi_manager.h    # WiFi管理接口
│   ├── mqtt_manager.h    # MQTT管理接口
│   ├── lora_manager.h    # LoRa管理接口
│   ├── relay_control.h   # 继电器控制接口
│   ├── condition_control.h # 条件控制接口
│   ├── web_server.h      # Web服务器接口
│   ├── web_ui.h          # Web界面接口
│   ├── ota_manager.h     # OTA升级接口
│   ├── system_monitor.h  # 系统监控接口
│   └── error_recovery.h  # 故障恢复接口
├── src/                  # 源代码目录
│   ├── main.cpp          # 主程序入口
│   ├── sensor.cpp        # 传感器实现
│   ├── wifi_manager.cpp  # WiFi管理实现
│   ├── mqtt_manager.cpp  # MQTT管理实现
│   ├── lora_manager.cpp  # LoRa管理实现
│   ├── relay_control.cpp # 继电器控制实现
│   ├── condition_control.cpp # 条件控制实现
│   ├── web_server.cpp    # Web服务器实现
│   ├── web_ui.cpp        # Web界面实现
│   ├── ota_manager.cpp   # OTA升级实现
│   ├── system_monitor.cpp # 系统监控实现
│   └── error_recovery.cpp # 故障恢复实现
├── docs/                 # 文档目录
├── test/                 # 测试目录
└── platformio.ini        # PlatformIO配置
```

---

## ⚙️ 配置说明

### 主要配置项

| 配置项 | 说明 | 默认值 |
|--------|------|--------|
| `DEBUG_MODE` | 调试模式开关 | 0 (关闭) |
| `RELAY_PIN` | 继电器控制引脚 | 20 |
| `SHT31_I2C_SDA` | SHT31 SDA引脚 | 8 |
| `SHT31_I2C_SCL` | SHT31 SCL引脚 | 9 |
| `LORA_FREQUENCY` | LoRa频率 | 433E6 |
| `DEFAULT_ACQUISITION_INTERVAL` | 采集间隔(ms) | 1000 |
| `DEFAULT_MQTT_REPORT_INTERVAL` | MQTT上报间隔(ms) | 1000 |
| `MEM_THRESHOLD` | 内存重启阈值(字节) | 15360 |

### 修改配置

编辑 `include/config.h` 文件修改硬件配置，编辑 `src/main.cpp` 修改网络配置。

---

## 🔍 故障排查

### 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 串口无输出 | 串口波特率错误 | 确认波特率为115200 |
| WiFi连接失败 | 密码错误或信号弱 | 检查密码和信号强度 |
| MQTT连接失败 | 服务器地址或端口错误 | 检查MQTT配置 |
| OTA升级失败 | 固件文件损坏 | 重新编译固件 |
| 传感器无数据 | I2C接线错误 | 检查SDA/SCL接线 |

### 日志解读

串口日志包含丰富的诊断信息：

```
🌐 WiFi: ✅ 已连接 (RSSI: -57 dBm)
📡 MQTT: ✅ 已连接
🔍 本地采集 - 温度：25.50 °C，湿度：60.20 %RH
📤 MQTT 上报 - 温度：25.50 °C，湿度：60.20 %RH
```

---

## 📝 版本历史

| 版本 | 日期 | 更新内容 |
|------|------|----------|
| v1.0.0 | 2026-05 | 初始版本，支持基本功能 |
| v1.1.0 | 2026-05 | 添加MQTT远程升级 |
| v1.2.0 | 2026-05 | 添加条件控制和定时控制 |

---

## 📄 许可证

MIT License

---

## 📞 联系信息

如有问题或建议，请联系开发团队。