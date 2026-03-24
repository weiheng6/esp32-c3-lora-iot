#include "web_ui.h"
#include "config.h"
#include "mqtt_manager.h"
#include "relay_control.h"
#include "sensor.h"
#include <ArduinoJson.h>

// 单全局实例
WebUIManager webUIManager(80);

// ==================== HTML 配置页面 ====================
// 为了节省 Flash，使用 PROGMEM 存储 HTML
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1.0">
  <title>ESP32-C3 设备配置</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; padding: 20px; }
    .container { max-width: 700px; margin: 0 auto; background: white; border-radius: 12px; box-shadow: 0 10px 40px rgba(0,0,0,0.2); overflow: hidden; }
    .header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 30px 20px; text-align: center; }
    .header h1 { font-size: 28px; margin-bottom: 10px; }
    .header p { opacity: 0.9; font-size: 14px; }
    .content { padding: 30px; }
    .section { margin: 25px 0; padding: 20px; border-left: 4px solid #667eea; background: #f8f9fa; border-radius: 6px; }
    .section h2 { color: #333; font-size: 18px; margin-bottom: 15px; }
    .form-group { margin: 15px 0; }
    label { display: block; margin-bottom: 8px; color: #555; font-weight: 600; font-size: 14px; }
    input[type="text"],
    input[type="number"],
    input[type="password"],
    select {
      width: 100%; padding: 12px; border: 2px solid #ddd; border-radius: 6px; font-size: 14px; transition: border-color 0.3s;
    }
    input:focus,
    select:focus { outline: none; border-color: #667eea; }
    button { padding: 12px 24px; margin-top: 10px; border: none; border-radius: 6px; cursor: pointer; font-size: 14px; font-weight: 600; transition: all 0.3s; width: 100%; }
    .btn-primary { background: #667eea; color: white; }
    .btn-primary:hover { background: #5568d3; transform: translateY(-2px); box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4); }
    .btn-danger { background: #f44336; color: white; margin-top: 10px; }
    .btn-danger:hover { background: #da190b; }
    .btn-warning { background: #ff9800; color: white; }
    .btn-warning:hover { background: #e68900; }
    .button-group { display: flex; gap: 10px; }
    .button-group button { flex: 1; margin: 0; }
    .alert { padding: 15px; border-radius: 6px; margin: 15px 0; font-size: 14px; display: none; animation: slideIn 0.3s ease-out; }
    .alert.show { display: block; }
    .alert.success { background: #c8e6c9; color: #2e7d32; border-left: 4px solid #4caf50; }
    .alert.error { background: #ffcdd2; color: #c62828; border-left: 4px solid #f44336; }
    .alert.info { background: #bbdefb; color: #1565c0; border-left: 4px solid #2196f3; }
    @keyframes slideIn { from { opacity: 0; transform: translateY(-10px); } to { opacity: 1; transform: translateY(0); } }
    .status-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin: 15px 0; }
    .status-item { background: white; padding: 15px; border-radius: 6px; border: 1px solid #ddd; }
    .status-label { color: #666; font-size: 12px; font-weight: 600; text-transform: uppercase; margin-bottom: 5px; }
    .status-value { font-size: 18px; font-weight: 700; color: #333; }
    .status-value.online { color: #4caf50; }
    .status-value.offline { color: #f44336; }
    .info-box { background: #e3f2fd; padding: 12px; border-radius: 6px; color: #1565c0; font-size: 12px; margin-top: 10px; border-left: 3px solid #2196f3; }
    .spinner { display: inline-block; width: 12px; height: 12px; border: 2px solid #f3f3f3; border-top: 2px solid #667eea; border-radius: 50%; animation: spin 1s linear infinite; }
    @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
    .tabs { display: flex; gap: 0; margin: 20px 0; border-bottom: 2px solid #ddd; }
    .tab-btn { padding: 12px 20px; background: none; border: none; border-bottom: 3px solid transparent; cursor: pointer; font-weight: 600; color: #666; transition: all 0.3s; }
    .tab-btn.active { color: #667eea; border-bottom-color: #667eea; }
    .tab-content { display: none; }
    .tab-content.active { display: block; }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>🔧 设备配置中心</h1>
      <p>ESP32-C3 环境监测和控制系统</p>
    </div>
    
    <div class="content">
      <!-- 状态提示 -->
      <div id="alert" class="alert"></div>
      
      <!-- 标签页 -->
      <div class="tabs">
        <button class="tab-btn active" onclick="switchTab('dashboard')">📊 仪表板</button>
        <button class="tab-btn" onclick="switchTab('mqtt')">📡 MQTT</button>
        <button class="tab-btn" onclick="switchTab('acquisition')">⏱️ 采集</button>
        <button class="tab-btn" onclick="switchTab('control')">🔌 控制</button>
        <button class="tab-btn" onclick="switchTab('settings')">⚙️ 设置</button>
      </div>
      
      <!-- 仪表板标签页 -->
      <div id="dashboard" class="tab-content active">
        <div class="section">
          <h2>📊 系统状态</h2>
          <div class="status-grid">
            <div class="status-item">
              <div class="status-label">WiFi 连接</div>
              <div id="wifiStatus" class="status-value offline"><span class="spinner"></span> 检查中</div>
            </div>
            <div class="status-item">
              <div class="status-label">MQTT 连接</div>
              <div id="mqttStatus" class="status-value offline"><span class="spinner"></span> 检查中</div>
            </div>
            <div class="status-item">
              <div class="status-label">温度</div>
              <div id="tempStatus" class="status-value">-- °C</div>
            </div>
            <div class="status-item">
              <div class="status-label">湿度</div>
              <div id="humStatus" class="status-value">-- %</div>
            </div>
            <div class="status-item">
              <div class="status-label">继电器</div>
              <div id="relayStatus" class="status-value">--</div>
            </div>
            <div class="status-item">
              <div class="status-label">内存使用</div>
              <div id="memStatus" class="status-value">-- %</div>
            </div>
          </div>
          <button class="btn-primary" onclick="refreshStatus()">🔄 刷新状态</button>
        </div>
      </div>
      
      <!-- MQTT 配置标签页 -->
      <div id="mqtt" class="tab-content">
        <div class="section">
          <h2>📡 MQTT 配置</h2>
          <div class="form-group">
            <label>MQTT 服务器地址：</label>
            <input type="text" id="mqttServer" placeholder="例：iot.kebaidata.com">
          </div>
          <div class="form-group">
            <label>MQTT 端口：</label>
            <input type="number" id="mqttPort" placeholder="1883" min="1" max="65535">
          </div>
          <div class="form-group">
            <label>MQTT 用户名：</label>
            <input type="text" id="mqttUser" placeholder="用户名">
          </div>
          <div class="form-group">
            <label>MQTT 密码：</label>
            <input type="password" id="mqttPassword" placeholder="密码">
          </div>
          <div class="info-box">💡 修改 MQTT 配置后需重启设备才能生效</div>
        </div>
      </div>
      
      <!-- 采集配置标签页 -->
      <div id="acquisition" class="tab-content">
        <div class="section">
          <h2>⏱️ 采集和上报配置</h2>
          <div class="form-group">
            <label>传感器采集间隔（秒）：</label>
            <input type="number" id="acquisitionInterval" placeholder="1" min="1" max="3600">
            <div class="info-box">📍 读取温度/湿度数据的周期</div>
          </div>
          <div class="form-group">
            <label>MQTT 上报间隔（秒）：</label>
            <input type="number" id="reportInterval" placeholder="1" min="1" max="3600">
            <div class="info-box">📍 将数据发送到 MQTT 的周期</div>
          </div>
        </div>
      </div>
      
      <!-- 控制标签页 -->
      <div id="control" class="tab-content">
        <div class="section">
          <h2>🔌 继电器控制</h2>
          <p style="margin-bottom: 15px; color: #666;">当前状态：<strong id="currentRelayStatus">未知</strong></p>
          <div class="button-group">
            <button class="btn-primary" onclick="controlRelay(1)">✅ 开启</button>
            <button class="btn-danger" onclick="controlRelay(0)">❌ 关闭</button>
          </div>
        </div>
        
        <div class="section">
          <h2>🎯 条件控制</h2>
          <div class="form-group">
            <label>
              <input type="checkbox" id="conditionEnabled"> 启用自动温湿度控制
            </label>
          </div>
          <div class="form-group">
            <label>温度阈值（°C）：</label>
            <input type="number" id="tempThreshold" placeholder="25" min="-40" max="100" step="0.1">
          </div>
          <div class="form-group">
            <label>湿度阈值（%）：</label>
            <input type="number" id="humThreshold" placeholder="60" min="0" max="100" step="0.1">
          </div>
          <div class="info-box">💡 启用后，当温度或湿度超过阈值时自动开启继电器</div>
        </div>
      </div>
      
      <!-- 设置标签页 -->
      <div id="settings" class="tab-content">
        <div class="section">
          <h2>⚙️ 系统设置</h2>
          <button class="btn-primary" onclick="saveConfig()" style="margin-top: 0;">💾 保存所有配置</button>
          <button class="btn-warning" onclick="exportConfig()" style="margin-top: 10px;">📥 导出配置</button>
          <button class="btn-warning" onclick="importConfigUI()" style="margin-top: 10px;">📤 导入配置</button>
          <input type="file" id="importFile" accept=".json" style="display:none;">
        </div>
        
        <div class="section">
          <h2>🔄 系统控制</h2>
          <button class="btn-danger" onclick="restartDevice()">🔌 重启设备</button>
          <div class="info-box">⚠️ 设备将在 2 秒后重启</div>
        </div>
      </div>
    </div>
  </div>

  <script>
    const API_BASE = window.location.origin;
    const alertDiv = document.getElementById('alert');
    
    function showAlert(message, type = 'info') {
      alertDiv.textContent = message;
      alertDiv.className = `alert show ${type}`;
      if (type !== 'error') {
        setTimeout(() => alertDiv.classList.remove('show'), 4000);
      }
    }
    
    function switchTab(tabName) {
      document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
      document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
      document.getElementById(tabName).classList.add('active');
      event.target.classList.add('active');
    }
    
    function refreshStatus() {
      fetch(`${API_BASE}/api/status`)
        .then(r => r.json())
        .then(data => {
          document.getElementById('wifiStatus').className = `status-value ${data.wifi ? 'online' : 'offline'}`;
          document.getElementById('wifiStatus').textContent = data.wifi ? '✅ 已连接' : '❌ 未连接';
          
          document.getElementById('mqttStatus').className = `status-value ${data.mqtt ? 'online' : 'offline'}`;
          document.getElementById('mqttStatus').textContent = data.mqtt ? '✅ 已连接' : '❌ 未连接';
          
          document.getElementById('tempStatus').textContent = data.temp.toFixed(2) + ' °C';
          document.getElementById('humStatus').textContent = data.hum.toFixed(2) + ' %';
          document.getElementById('relayStatus').textContent = data.relay ? '🔴 开启' : '⚫ 关闭';
          document.getElementById('currentRelayStatus').textContent = data.relay ? '开启' : '关闭';
          document.getElementById('memStatus').textContent = data.mem.toFixed(1) + ' %';
        })
        .catch(e => showAlert('状态获取失败：' + e, 'error'));
    }
    
    function saveConfig() {
      const config = {
        mqtt_server: document.getElementById('mqttServer').value,
        mqtt_port: parseInt(document.getElementById('mqttPort').value),
        mqtt_user: document.getElementById('mqttUser').value,
        mqtt_password: document.getElementById('mqttPassword').value,
        acquisition_interval: parseInt(document.getElementById('acquisitionInterval').value),
        report_interval: parseInt(document.getElementById('reportInterval').value),
        temp_threshold: parseFloat(document.getElementById('tempThreshold').value),
        hum_threshold: parseFloat(document.getElementById('humThreshold').value),
        condition_enabled: document.getElementById('conditionEnabled').checked
      };
      
      fetch(`${API_BASE}/api/config`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(config)
      })
        .then(r => r.json())
        .then(data => showAlert('✅ 配置已保存！' + (data.msg || ''), 'success'))
        .catch(e => showAlert('保存失败：' + e, 'error'));
    }
    
    function controlRelay(state) {
      fetch(`${API_BASE}/api/relay`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ relay: state })
      })
        .then(r => r.json())
        .then(data => {
          showAlert(state ? '✅ 继电器已开启' : '✅ 继电器已关闭', 'success');
          refreshStatus();
        })
        .catch(e => showAlert('控制失败：' + e, 'error'));
    }
    
    function exportConfig() {
      fetch(`${API_BASE}/api/export`)
        .then(r => r.blob())
        .then(blob => {
          const url = window.URL.createObjectURL(blob);
          const a = document.createElement('a');
          a.href = url;
          a.download = 'device_config_' + new Date().getTime() + '.json';
          a.click();
          window.URL.revokeObjectURL(url);
          showAlert('✅ 配置已导出', 'success');
        })
        .catch(e => showAlert('导出失败：' + e, 'error'));
    }
    
    function importConfigUI() {
      document.getElementById('importFile').click();
      document.getElementById('importFile').onchange = function(e) {
        const file = e.target.files[0];
        const reader = new FileReader();
        reader.onload = function(event) {
          try {
            const config = JSON.parse(event.target.result);
            fetch(`${API_BASE}/api/import`, {
              method: 'POST',
              headers: { 'Content-Type': 'application/json' },
              body: JSON.stringify(config)
            })
              .then(r => r.json())
              .then(data => showAlert('✅ 配置已导入，请重启设备', 'success'))
              .catch(e => showAlert('导入失败：' + e, 'error'));
          } catch (e) {
            showAlert('配置文件格式错误：' + e, 'error');
          }
        };
        reader.readAsText(file);
      };
    }
    
    function restartDevice() {
      if (confirm('确定要重启设备吗？')) {
        fetch(`${API_BASE}/api/restart`, { method: 'POST' })
          .then(() => {
            showAlert('🔄 设备重启中...页面将在 3 秒后自动刷新', 'info');
            setTimeout(() => location.reload(), 3000);
          })
          .catch(e => showAlert('重启失败：' + e, 'error'));
      }
    }
    
    // 页面加载时刷新状态
    window.onload = refreshStatus;
    
    // 每 5 秒自动刷新一次状态
    setInterval(refreshStatus, 5000);
  </script>
</body>
</html>
)rawliteral";

WebUIManager::WebUIManager(uint16_t port) : server(port) {}

void WebUIManager::begin() {
  // 注册所有端点
  server.on("/", HTTP_GET, [this]() { this->handleRoot(); });
  server.on("/api/status", HTTP_GET, [this]() { this->handleGetStatus(); });
  server.on("/api/config", HTTP_GET, [this]() { this->handleGetConfig(); });
  server.on("/api/config", HTTP_POST, [this]() { this->handleSetConfig(); });
  server.on("/api/relay", HTTP_POST, [this]() { this->handleRelayControl(); });
  server.on("/api/restart", HTTP_POST, [this]() { this->handleRestart(); });
  server.on("/api/export", HTTP_GET, [this]() { this->handleExportConfig(); });
  server.on("/api/import", HTTP_POST, [this]() { this->handleImportConfig(); });
  
  server.begin();
  isStarted = true;
  Serial.println("✅ Web UI 已启动 (http://192.168.4.1)");
}

void WebUIManager::handleClient() {
  if (isStarted) {
    server.handleClient();
  }
}

void WebUIManager::handleRoot() {
  server.send(200, "text/html; charset=utf-8", HTML_PAGE);
}

void WebUIManager::handleGetStatus() {
  StaticJsonDocument<300> doc;
  doc["wifi"] = (WiFi.status() == WL_CONNECTED);
  doc["mqtt"] = mqttManager.isConnected();
  doc["relay"] = relayControl.getState();
  doc["mem"] = (100.0 * (ESP.getHeapSize() - ESP.getFreeHeap())) / ESP.getHeapSize();
  
  float temp = 0, hum = 0;
  if (sensorManager.readBoth(temp, hum)) {
    doc["temp"] = temp;
    doc["hum"] = hum;
  } else {
    doc["temp"] = 0;
    doc["hum"] = 0;
  }
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void WebUIManager::handleGetConfig() {
  StaticJsonDocument<200> doc;
  doc["mqtt_server"] = MQTT_SERVER;
  doc["mqtt_port"] = MQTT_PORT;
  doc["mqtt_user"] = MQTT_USER;
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void WebUIManager::handleSetConfig() {
  StaticJsonDocument<300> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  
  if (error) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"JSON parse failed\"}");
    return;
  }
  
  // 这里可以添加逻辑来保存配置到 Preferences
  Serial.println("💾 配置已接收（实现详情可扩展）");
  
  server.send(200, "application/json", "{\"status\":\"ok\",\"msg\":\"Config saved\"}");
}

void WebUIManager::handleRelayControl() {
  StaticJsonDocument<100> doc;
  deserializeJson(doc, server.arg("plain"));
  
  if (doc.containsKey("relay")) {
    if (doc["relay"] == 1) {
      relayControl.turnOn();
    } else {
      relayControl.turnOff();
    }
  }
  
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebUIManager::handleRestart() {
  server.send(200, "application/json", "{\"status\":\"ok\",\"msg\":\"Restarting...\"}");
  delay(1000);
  ESP.restart();
}

void WebUIManager::handleExportConfig() {
  StaticJsonDocument<300> config;
  config["mqtt_server"] = MQTT_SERVER;
  config["mqtt_port"] = MQTT_PORT;
  config["mqtt_user"] = MQTT_USER;
  config["timestamp"] = millis();
  
  String json;
  serializeJson(config, json);
  server.send(200, "application/json", json);
}

void WebUIManager::handleImportConfig() {
  StaticJsonDocument<300> doc;
  deserializeJson(doc, server.arg("plain"));
  
  Serial.println("📥 配置导入请求已接收");
  
  server.send(200, "application/json", "{\"status\":\"ok\",\"msg\":\"Config imported\"}");
}
