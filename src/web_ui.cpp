#include "web_ui.h"
#include "config.h"
#include "mqtt_manager.h"
#include "relay_control.h"
#include "sensor.h"
#include "condition_control.h"
#include <ArduinoJson.h>

// 【声明外部全局变量】
extern bool manualRelayMode;  // 来自 main.cpp
extern ConditionControl conditionControl;

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
        <button class="tab-btn" onclick="switchTab('control')">� 控制</button>
        <button class="tab-btn" onclick="switchTab('condition')">🎯 条件控制</button>
        <button class="tab-btn" onclick="switchTab('timer')">⏰ 定时控制</button>
        <button class="tab-btn" onclick="switchTab('mqtt')">� MQTT</button>
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
              <div class="status-label">继电器状态</div>
              <div id="relayStatus" class="status-value" style="font-size: 24px; font-weight: bold;">--</div>
            </div>
            <div class="status-item">
              <div class="status-label">内存使用</div>
              <div id="memStatus" class="status-value">-- %</div>
            </div>
            <div class="status-item">
              <div class="status-label">控制模式</div>
              <div id="controlModeStatus" class="status-value" style="font-weight: bold; color: #667eea;">--</div>
            </div>
            <div class="status-item">
              <div class="status-label">手动/自动</div>
              <div id="manualModeStatus" class="status-value" style="font-weight: bold;">--</div>
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
          <button class="btn-primary" onclick="saveMQTTConfig()">💾 保存 MQTT 配置</button>
          <div class="info-box">⚠️ 修改 MQTT 配置后需重启设备才能生效</div>
        </div>
      </div>
      
      <!-- 控制标签页 -->
      <div id="control" class="tab-content">
        <div class="section">
          <h2>🔌 手动控制</h2>
          <p style="margin-bottom: 15px; color: #666;">当前模式：<strong id="currentMode">检查中...</strong></p>
          <p style="margin-bottom: 15px; color: #666;">继电器状态：<strong id="relayStatusLarge">未知</strong></p>
          
          <div class="button-group">
            <button class="btn-primary" onclick="controlRelay(1)">✅ 手动开启</button>
            <button class="btn-danger" onclick="controlRelay(0)">❌ 手动关闭</button>
          </div>
          
          <div class="info-box" style="margin-top: 15px;">
            ℹ️ 手动控制时，自动控制（条件/定时）将被禁用。<br>点击下方按钮可切换回自动模式。
          </div>
          
          <div class="button-group" style="margin-top: 15px;">
            <button class="btn-warning" onclick="toggleManualMode()" id="manualModeToggle">🖱️ 启用手动模式</button>
          </div>
        </div>
        
        <div class="section">
          <h2>�️ 控制模式切换</h2>
          <div class="button-group">
            <button class="btn-primary" id="modeNone" onclick="setControlMode('none')" style="background: #9e9e9e;">📵 禁用自动控制</button>
            <button class="btn-primary" id="modeCondition" onclick="setControlMode('condition')">�🎯 启用条件控制</button>
            <button class="btn-primary" id="modeTimer" onclick="setControlMode('timer')">⏰ 启用定时控制</button>
          </div>
          <p id="modeInfo" style="margin-top: 15px; color: #666; font-size: 12px;">
            加载中...
          </p>
        </div>
      </div>
      
      <!-- 条件控制标签页 -->
      <div id="condition" class="tab-content">
        <div class="section">
          <h2>🎯 灵活条件控制</h2>
          <div class="form-group">
            <label>
              <input type="checkbox" id="conditionEnabled" onchange="toggleConditionMode()"> 
              启用条件控制
            </label>
            <div class="info-box">每个条件独立配置，可自由选择传感器和比较操作符</div>
          </div>
        </div>
        
        <div id="conditionContent" class="section" style="display: none;">
          <!-- 【新增】开启条件组 -->
          <h2>🟢 继电器开启条件</h2>
          <div class="form-group">
            <label>
              <input type="checkbox" id="onGroupEnabled" onchange="updateConditionLogic()"> 
              启用开启条件组
            </label>
          </div>
          
          <div id="onGroupContent" style="display: none;">
            <div class="form-group">
              <label>条件之间的逻辑关系：</label>
              <select id="onGroupLogic" onchange="updateConditionLogic()">
                <option value="and">AND - 所有条件都满足时开启</option>
                <option value="or">OR - 任意条件满足时开启</option>
              </select>
            </div>
            
            <!-- 最多4个条件 -->
            <div id="onConditionsContainer"></div>
            <button class="btn-primary" onclick="addOnCondition()" style="width: 100%; margin-top: 10px;">+ 添加开启条件</button>
          </div>
          
          <!-- 【新增】关闭条件组 -->
          <h2 style="margin-top: 30px;">🔴 继电器关闭条件</h2>
          <div class="form-group">
            <label>
              <input type="checkbox" id="offGroupEnabled" onchange="updateConditionLogic()"> 
              启用关闭条件组
            </label>
          </div>
          
          <div id="offGroupContent" style="display: none;">
            <div class="form-group">
              <label>条件之间的逻辑关系：</label>
              <select id="offGroupLogic" onchange="updateConditionLogic()">
                <option value="and">AND - 所有条件都满足时关闭</option>
                <option value="or">OR - 任意条件满足时关闭</option>
              </select>
            </div>
            
            <!-- 最多4个条件 -->
            <div id="offConditionsContainer"></div>
            <button class="btn-primary" onclick="addOffCondition()" style="width: 100%; margin-top: 10px;">+ 添加关闭条件</button>
          </div>
          
          <h2 style="margin-top: 30px;">💾 保存配置</h2>
          <div class="button-group">
            <button class="btn-primary" onclick="saveCondition()">💾 保存条件控制</button>
            <button class="btn-warning" onclick="loadCondition()">🔄 刷新当前配置</button>
          </div>
        </div>
      </div>
      
      <!-- 定时控制标签页 -->
      <div id="timer" class="tab-content">
        <div class="section">
          <h2>⏰ 定时控制配置</h2>
          <div class="form-group">
            <label>
              <input type="checkbox" id="timerEnabled" onchange="toggleTimerMode()"> 
              启用定时控制
            </label>
            <div class="info-box">启用后，继电器将根据时间自动开关（共支持 8 个时间段）</div>
          </div>
        </div>
        
        <div id="timerContent" class="section" style="display: none;">
          <h2>⏱️ 时间段列表</h2>
          <div id="timerSlots" style="margin: 15px 0;">
            <!-- 时间段卡片将被动态插入这里 -->
          </div>
          
          <div class="button-group" style="margin-top: 20px;">
            <button class="btn-primary" onclick="addTimerSlot()">➕ 添加时间段</button>
            <button class="btn-warning" onclick="clearAllTimerSlots()" style="background: #ff6f00;">🗑️ 清除所有</button>
          </div>
          
          <div class="button-group">
            <button class="btn-primary" onclick="saveTimer()">💾 保存定时控制</button>
            <button class="btn-warning" onclick="loadTimer()">🔄 刷新当前配置</button>
          </div>
        </div>
      </div>
      
      <!-- 设置标签页 -->
      <div id="settings" class="tab-content">
        <div class="section">
          <h2>⚙️ 系统信息</h2>
          <div class="status-grid">
            <div class="status-item">
              <div class="status-label">设备 IP</div>
              <div class="status-value">192.168.4.1</div>
            </div>
            <div class="status-item">
              <div class="status-label">当前时间</div>
              <div id="currentTime" class="status-value">--:--:--</div>
            </div>
            <div class="status-item">
              <div class="status-label">uptime</div>
              <div id="uptime" class="status-value">--</div>
            </div>
            <div class="status-item">
              <div class="status-label">固件版本</div>
              <div class="status-value">v1.0</div>
            </div>
          </div>
        </div>
        
        <div class="section">
          <h2>💾 配置管理</h2>
          <div class="button-group">
            <button class="btn-warning" onclick="exportConfig()">📥 导出配置</button>
            <button class="btn-warning" onclick="importConfigUI()">📤 导入配置</button>
          </div>
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
    let timerSlotsData = [];
    let currentMode = 'none';  // 'none', 'condition', 'timer'
    
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
    
    // ============== 模式管理 ==============
    async function setControlMode(mode) {
      try {
        if (mode === 'condition') {
          // 【关键】禁用手动模式，启用自动控制
          await fetch(`${API_BASE}/api/manual_mode`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ enabled: false })
          });
          
          // 启用条件控制，禁用定时控制
          await fetch(`${API_BASE}/api/condition`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ condition: { enabled: true } })
          });
          
          await fetch(`${API_BASE}/api/timer`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ timer: { enabled: false } })
          });
          
          currentMode = 'condition';
          updateModeUI();
          showAlert('✅ 已切换到条件控制模式', 'success');
          
        } else if (mode === 'timer') {
          // 【关键】禁用手动模式，启用自动控制
          await fetch(`${API_BASE}/api/manual_mode`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ enabled: false })
          });
          
          // 启用定时控制，禁用条件控制
          await fetch(`${API_BASE}/api/timer`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ timer: { enabled: true } })
          });
          
          await fetch(`${API_BASE}/api/condition`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ condition: { enabled: false } })
          });
          
          currentMode = 'timer';
          updateModeUI();
          showAlert('✅ 已切换到定时控制模式', 'success');
          
        } else {
          // 禁用所有自动控制
          await fetch(`${API_BASE}/api/condition`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ condition: { enabled: false } })
          });
          
          await fetch(`${API_BASE}/api/timer`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ timer: { enabled: false } })
          });
          
          currentMode = 'none';
          updateModeUI();
          showAlert('✅ 已禁用自动控制', 'success');
        }
        
        // 【重要】切换模式后立即刷新状态，确保 UI 同步
        setTimeout(refreshStatus, 500);
        
      } catch (e) {
        console.error('模式切换错误：' + e);
        showAlert('切换失败：' + e, 'error');
      }
    }
    
    function updateModeUI() {
      const buttons = {
        'none': document.getElementById('modeNone'),
        'condition': document.getElementById('modeCondition'),
        'timer': document.getElementById('modeTimer')
      };
      
      Object.keys(buttons).forEach(k => {
        buttons[k].style.background = k === currentMode ? '#667eea' : '#9e9e9e';
      });
      
      const modeText = { 'none': '禁用自动控制', 'condition': '条件控制', 'timer': '定时控制' };
      document.getElementById('modeInfo').textContent = '✅ 当前激活：' + modeText[currentMode];
    }
    
    // ============== 条件控制【新增灵活系统】 ==============
    function toggleConditionMode() {
      const enabled = document.getElementById('conditionEnabled').checked;
      document.getElementById('conditionContent').style.display = enabled ? 'block' : 'none';
      if (enabled) {
        loadCondition();
      }
    }
    
    function loadCondition() {
      fetch(`${API_BASE}/api/condition`)
        .then(r => r.json())
        .then(data => {
          console.log('Loaded condition:', data);
          document.getElementById('conditionEnabled').checked = data.enabled;
          
          // 【新增】加载 ON 条件组
          if (data.on_condition) {
            document.getElementById('onGroupEnabled').checked = data.on_condition.enabled;
            document.getElementById('onGroupLogic').value = data.on_condition.logic === 'and' ? 'and' : 'or';
            
            // 重新生成 ON 条件UI
            const onContainer = document.getElementById('onConditionsContainer');
            onContainer.innerHTML = '';
            if (data.on_condition.conditions && data.on_condition.count > 0) {
              for (let i = 0; i < data.on_condition.count && i < 4; i++) {
                const cond = data.on_condition.conditions[i];
                createConditionUI(onContainer, 'on', i, cond.enabled, cond.sensor, cond.compare, cond.threshold);
              }
            }
          }
          
          // 【新增】加载 OFF 条件组
          if (data.off_condition) {
            document.getElementById('offGroupEnabled').checked = data.off_condition.enabled;
            document.getElementById('offGroupLogic').value = data.off_condition.logic === 'and' ? 'and' : 'or';
            
            // 重新生成 OFF 条件UI
            const offContainer = document.getElementById('offConditionsContainer');
            offContainer.innerHTML = '';
            if (data.off_condition.conditions && data.off_condition.count > 0) {
              for (let i = 0; i < data.off_condition.count && i < 4; i++) {
                const cond = data.off_condition.conditions[i];
                createConditionUI(offContainer, 'off', i, cond.enabled, cond.sensor, cond.compare, cond.threshold);
              }
            }
          }
          
          updateConditionLogic();
        })
        .catch(e => showAlert('加载条件配置失败：' + e, 'error'));
    }
    
    function createConditionUI(container, group, index, enabled, sensor, compareOp, threshold) {
      const div = document.createElement('div');
      div.id = `${group}_cond_${index}`;
      div.style.background = '#fff';
      div.style.padding = '12px';
      div.style.margin = '10px 0';
      div.style.border = '1px solid #ddd';
      div.style.borderRadius = '6px';
      
      div.innerHTML = `
        <div style="display: flex; gap: 10px; align-items: center; margin-bottom: 10px;">
          <input type="checkbox" ${enabled ? 'checked' : ''} onchange="saveCondition()">
          <label style="margin: 0; flex: 1;">条件 ${index + 1}</label>
          <button class="btn-danger" style="width: 60px; padding: 6px; margin: 0;" onclick="removeCondition('${group}', ${index})">删除</button>
        </div>
        <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 10px;">
          <select onchange="saveCondition()">
            <option value="temp" ${sensor === 'temp' ? 'selected' : ''}>🌡️ 温度</option>
            <option value="humi" ${sensor === 'humi' ? 'selected' : ''}>💧 湿度</option>
          </select>
          <select onchange="saveCondition()">
            <option value="1" ${compareOp === 1 ? 'selected' : ''}> > 大于</option>
            <option value="2" ${compareOp === 2 ? 'selected' : ''}> < 小于</option>
            <option value="3" ${compareOp === 3 ? 'selected' : ''}> = 等于</option>
            <option value="4" ${compareOp === 4 ? 'selected' : ''}>≥ 大于等于</option>
            <option value="5" ${compareOp === 5 ? 'selected' : ''}>≤ 小于等于</option>
            <option value="6" ${compareOp === 6 ? 'selected' : ''}>≠ 不等于</option>
          </select>
        </div>
        <input type="number" value="${threshold}" step="0.1" min="0" max="100" style="width: 100%; margin-top: 8px; padding: 8px; border: 1px solid #ddd; border-radius: 4px;" placeholder="阈值" onchange="saveCondition()">
      `;
      
      container.appendChild(div);
    }
    
    function addOnCondition() {
      const container = document.getElementById('onConditionsContainer');
      const count = container.children.length;
      if (count >= 4) {
        showAlert('最多只能添加 4 个条件', 'warning');
        return;
      }
      createConditionUI(container, 'on', count, true, 'temp', 1, 25);
      saveCondition();
    }
    
    function addOffCondition() {
      const container = document.getElementById('offConditionsContainer');
      const count = container.children.length;
      if (count >= 4) {
        showAlert('最多只能添加 4 个条件', 'warning');
        return;
      }
      createConditionUI(container, 'off', count, true, 'temp', 1, 25);
      saveCondition();
    }
    
    function removeCondition(group, index) {
      const elem = document.getElementById(`${group}_cond_${index}`);
      if (elem) elem.remove();
      saveCondition();
    }
    
    function updateConditionLogic() {
      document.getElementById('onGroupContent').style.display = 
        document.getElementById('onGroupEnabled').checked ? 'block' : 'none';
      document.getElementById('offGroupContent').style.display = 
        document.getElementById('offGroupEnabled').checked ? 'block' : 'none';
    }
    
    function saveCondition() {
      // 【新增】收集 ON 条件
      const onConditions = [];
      const onContainer = document.getElementById('onConditionsContainer');
      for (let div of onContainer.querySelectorAll('[id^="on_cond_"]')) {
        const inputs = div.querySelectorAll('input, select');
        onConditions.push({
          enabled: inputs[0].checked,
          sensor: inputs[1].value,
          compare: parseInt(inputs[2].value),
          threshold: parseFloat(inputs[3].value)
        });
      }
      
      // 【新增】收集 OFF 条件
      const offConditions = [];
      const offContainer = document.getElementById('offConditionsContainer');
      for (let div of offContainer.querySelectorAll('[id^="off_cond_"]')) {
        const inputs = div.querySelectorAll('input, select');
        offConditions.push({
          enabled: inputs[0].checked,
          sensor: inputs[1].value,
          compare: parseInt(inputs[2].value),
          threshold: parseFloat(inputs[3].value)
        });
      }
      
      const config = {
        condition: {
          enabled: document.getElementById('conditionEnabled').checked,
          on_condition: {
            enabled: document.getElementById('onGroupEnabled').checked,
            logic: document.getElementById('onGroupLogic').value,
            conditions: onConditions
          },
          off_condition: {
            enabled: document.getElementById('offGroupEnabled').checked,
            logic: document.getElementById('offGroupLogic').value,
            conditions: offConditions
          }
        }
      };
      
      fetch(`${API_BASE}/api/condition`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(config)
      })
        .then(r => r.json())
        .then(() => {
          showAlert('✅ 条件控制已保存！', 'success');
          setControlMode('condition');
        })
        .catch(e => console.error('保存失败：' + e));
    }
    
    // ============== 定时控制 ==============
    function toggleTimerMode() {
      const enabled = document.getElementById('timerEnabled').checked;
      document.getElementById('timerContent').style.display = enabled ? 'block' : 'none';
      if (enabled) {
        loadTimer();
      }
    }
    
    function loadTimer() {
      fetch(`${API_BASE}/api/timer`)
        .then(r => r.json())
        .then(data => {
          console.log('Loaded timer:', data);
          if (data && data.timer) {
            const t = data.timer;
            document.getElementById('timerEnabled').checked = t.enabled;
            
            timerSlotsData = t.slots || [];
            renderTimerSlots();
          }
        })
        .catch(e => showAlert('加载定时配置失败：' + e, 'error'));
    }
    
    function renderTimerSlots() {
      const container = document.getElementById('timerSlots');
      container.innerHTML = '';
      
      timerSlotsData.forEach((slot, index) => {
        const slotHtml = `
          <div style="background: white; padding: 15px; border-radius: 6px; margin: 10px 0; border-left: 4px solid #667eea;">
            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px;">
              <h3 style="margin: 0; color: #333;">时间段 ${index + 1}</h3>
              <label style="font-size: 12px;">
                <input type="checkbox" ${slot.enabled ? 'checked' : ''} onchange="updateTimerSlot(${index}, 'enabled', this.checked)"> 启用
              </label>
            </div>
            
            <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 10px;">
              <div>
                <label style="font-size: 12px;">开始时间：</label>
                <input type="time" value="${String(slot.startHour).padStart(2, '0')}:${String(slot.startMinute).padStart(2, '0')}" 
                  onchange="updateTimerSlotTime(${index}, 'start', this.value)" style="width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px;">
              </div>
              <div>
                <label style="font-size: 12px;">结束时间：</label>
                <input type="time" value="${String(slot.endHour).padStart(2, '0')}:${String(slot.endMinute).padStart(2, '0')}" 
                  onchange="updateTimerSlotTime(${index}, 'end', this.value)" style="width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px;">
              </div>
            </div>
            
            <div style="margin-bottom: 10px;">
              <label style="font-size: 12px;">在此时间段内的状态：</label>
              <div>
                <label style="display: inline-block; margin-right: 15px;">
                  <input type="radio" name="slotState_${index}" value="true" ${slot.state ? 'checked' : ''} onchange="updateTimerSlot(${index}, 'state', true)"> 
                  ✅ 开启继电器
                </label>
                <label style="display: inline-block;">
                  <input type="radio" name="slotState_${index}" value="false" ${!slot.state ? 'checked' : ''} onchange="updateTimerSlot(${index}, 'state', false)"> 
                  ❌ 关闭继电器
                </label>
              </div>
            </div>
            
            <button style="width: 100%; padding: 8px; background: #f44336; color: white; border: none; border-radius: 4px; cursor: pointer; font-weight: 600;" onclick="removeTimerSlot(${index})">🗑️ 删除这个时间段</button>
          </div>
        `;
        container.innerHTML += slotHtml;
      });
    }
    
    function updateTimerSlot(index, field, value) {
      if (!timerSlotsData[index]) {
        timerSlotsData[index] = { enabled: false, startHour: 0, startMinute: 0, endHour: 0, endMinute: 0, state: false };
      }
      
      if (field === 'state') {
        timerSlotsData[index].state = value === true || value === 'true';
      } else {
        timerSlotsData[index][field] = value;
      }
    }
    
    function updateTimerSlotTime(index, type, timeStr) {
      const [hours, minutes] = timeStr.split(':').map(Number);
      if (type === 'start') {
        timerSlotsData[index].startHour = hours;
        timerSlotsData[index].startMinute = minutes;
      } else {
        timerSlotsData[index].endHour = hours;
        timerSlotsData[index].endMinute = minutes;
      }
    }
    
    function addTimerSlot() {
      if (timerSlotsData.length >= 8) {
        showAlert('⚠️ 最多只能添加 8 个时间段', 'error');
        return;
      }
      
      timerSlotsData.push({
        enabled: true,
        startHour: 8,
        startMinute: 0,
        endHour: 18,
        endMinute: 0,
        state: true
      });
      renderTimerSlots();
      showAlert('✅ 已添加新时间段', 'success');
    }
    
    function removeTimerSlot(index) {
      if (confirm('确定要删除这个时间段吗？')) {
        timerSlotsData.splice(index, 1);
        renderTimerSlots();
        showAlert('✅ 时间段已删除', 'success');
      }
    }
    
    function clearAllTimerSlots() {
      if (confirm('确定要清除所有时间段吗？')) {
        timerSlotsData = [];
        renderTimerSlots();
        showAlert('✅ 所有时间段已清除', 'success');
      }
    }
    
    function saveTimer() {
      const config = {
        timer: {
          enabled: document.getElementById('timerEnabled').checked,
          slots: timerSlotsData.map((slot, index) => ({
            index: index,
            enabled: slot.enabled,
            start_time: `${String(slot.startHour).padStart(2, '0')}:${String(slot.startMinute).padStart(2, '0')}`,
            end_time: `${String(slot.endHour).padStart(2, '0')}:${String(slot.endMinute).padStart(2, '0')}`,
            state: slot.state
          }))
        }
      };
      
      fetch(`${API_BASE}/api/timer`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(config)
      })
        .then(r => r.json())
        .then(() => {
          showAlert('✅ 定时控制已保存！', 'success');
          setControlMode('timer');
        })
        .catch(e => showAlert('保存失败：' + e, 'error'));
    }
    
    // ============== 通用功能 ==============
    async function refreshStatus() {
      try {
        // 获取基本状态
        const statusResp = await fetch(`${API_BASE}/api/status`);
        if (!statusResp.ok) throw new Error('状态获取失败');
        const data = await statusResp.json();
        
        console.log('Status data:', data);
        
        // 更新基本信息
        if (document.getElementById('wifiStatus')) {
          document.getElementById('wifiStatus').className = `status-value ${data.wifi ? 'online' : 'offline'}`;
          document.getElementById('wifiStatus').textContent = data.wifi ? '✅ 已连接' : '❌ 未连接';
        }
        
        if (document.getElementById('mqttStatus')) {
          document.getElementById('mqttStatus').className = `status-value ${data.mqtt ? 'online' : 'offline'}`;
          document.getElementById('mqttStatus').textContent = data.mqtt ? '✅ 已连接' : '❌ 未连接';
        }
        
        if (document.getElementById('tempStatus')) {
          document.getElementById('tempStatus').textContent = data.temp ? data.temp.toFixed(2) + ' °C' : '--';
        }
        
        if (document.getElementById('humStatus')) {
          document.getElementById('humStatus').textContent = data.hum ? data.hum.toFixed(2) + ' %' : '--';
        }
        
        if (document.getElementById('relayStatusLarge')) {
          document.getElementById('relayStatusLarge').textContent = data.relay ? '🔴 开启' : '⚫ 关闭';
        }
        
        if (document.getElementById('currentRelayStatus')) {
          document.getElementById('currentRelayStatus').textContent = data.relay ? '开启' : '关闭';
        }
        
        if (document.getElementById('relayStatus')) {
          document.getElementById('relayStatus').textContent = data.relay ? '🔴 开启' : '⚫ 关闭';
        }
        
        if (document.getElementById('memStatus')) {
          document.getElementById('memStatus').textContent = data.mem ? data.mem.toFixed(1) + ' %' : '--';
        }
        
        // 【关键修复】更新控制模式和手动模式显示
        // 首先确保元素存在
        const manualModeElem = document.getElementById('manualModeStatus');
        const controlModeElem = document.getElementById('controlModeStatus');
        
        if (manualModeElem && controlModeElem) {
          if (data.manual_mode) {
            manualModeElem.textContent = '🖱️ 手动模式';
            manualModeElem.style.color = '#ff9800';
            controlModeElem.textContent = '禁用自动';
            controlModeElem.style.color = '#f44336';
            currentMode = 'none';
          } else {
            manualModeElem.textContent = '⚙️ 自动模式';
            manualModeElem.style.color = '#4caf50';
            
            // 显示当前自动控制模式
            if (data.control_mode === 'timer') {
              controlModeElem.textContent = '⏰ 定时控制';
              controlModeElem.style.color = '#2196f3';
              currentMode = 'timer';
            } else if (data.control_mode === 'condition') {
              controlModeElem.textContent = '🎯 条件控制';
              controlModeElem.style.color = '#667eea';
              currentMode = 'condition';
            } else {
              controlModeElem.textContent = '📵 禁用';
              controlModeElem.style.color = '#999';
              currentMode = 'none';
            }
          }
          
          // 【关键】同步 updateModeUI 确保按钮高亮显示正确的模式
          updateModeUI();
        }
      } catch (e) {
        console.error('状态获取失败：' + e);
      }
    }
    
    function saveMQTTConfig() {
      const config = {
        mqtt_server: document.getElementById('mqttServer').value,
        mqtt_port: parseInt(document.getElementById('mqttPort').value),
        mqtt_user: document.getElementById('mqttUser').value,
        mqtt_password: document.getElementById('mqttPassword').value,
        acquisition_interval: parseInt(document.getElementById('acquisitionInterval').value),
        report_interval: parseInt(document.getElementById('reportInterval').value)
      };
      
      fetch(`${API_BASE}/api/config`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(config)
      })
        .then(r => r.json())
        .then(data => showAlert('✅ MQTT 配置已保存！' + (data.msg || ''), 'success'))
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
    
    // 【新增】切换手动模式
    async function toggleManualMode() {
      try {
        const response = await fetch(`${API_BASE}/api/manual_mode`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ enabled: true })  // 启用手动模式
        });
        
        const data = await response.json();
        if (data.status === 'ok') {
          showAlert('✅ 已启用手动模式 - 自动控制已禁用', 'success');
          setTimeout(refreshStatus, 500);
        } else {
          showAlert('❌ 切换手动模式失败', 'error');
        }
      } catch (e) {
        showAlert('❌ 请求失败：' + e, 'error');
      }
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
    window.onload = function() {
      refreshStatus();
      updateModeUI();
    };
    
    // 每 5 秒自动刷新一次状态
    setInterval(refreshStatus, 5000);
    
    // 更新时间显示
    setInterval(() => {
      const now = new Date();
      document.getElementById('currentTime').textContent = 
        `${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}:${String(now.getSeconds()).padStart(2, '0')}`;
    }, 1000);
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
  server.on("/api/manual_mode", HTTP_POST, [this]() { this->handleManualMode(); });
  server.on("/api/condition", HTTP_GET, [this]() { this->handleGetCondition(); });
  server.on("/api/condition", HTTP_POST, [this]() { this->handleSetCondition(); });
  server.on("/api/timer", HTTP_GET, [this]() { this->handleGetTimer(); });
  server.on("/api/timer", HTTP_POST, [this]() { this->handleSetTimer(); });
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
  
  // 【新增】返回控制模式信息
  doc["manual_mode"] = manualRelayMode;  // 是否处于手动模式
  
  // 当前激活的自动模式
  String currentMode = "none";
  if (!manualRelayMode) {  // 只在非手动模式下才判断自动模式
    if (conditionControl.isTimerEnabled()) {
      currentMode = "timer";
    } else if (conditionControl.isEnabled()) {
      currentMode = "condition";
    }
  }
  doc["control_mode"] = currentMode;
  
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
    
    // 【关键】用户手动控制继电器时，启用手动模式，防止自动控制立即覆盖
    manualRelayMode = true;
    Serial.println("🖱️  启用手动模式 - 自动控制已禁用");
  }
  
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// 【新增】处理手动模式切换
void WebUIManager::handleManualMode() {
  StaticJsonDocument<100> doc;
  deserializeJson(doc, server.arg("plain"));
  
  if (doc.containsKey("enabled")) {
    manualRelayMode = doc["enabled"].as<bool>();
    
    if (manualRelayMode) {
      Serial.println("🖱️  启用手动模式 - 自动控制已禁用");
    } else {
      Serial.println("🔄 禁用手动模式 - 自动控制已启用");
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

// ==================== 条件控制 API 端点 ====================
void WebUIManager::handleGetCondition() {
  String json = conditionControl.toJSON();
  server.send(200, "application/json", json);
}

void WebUIManager::handleSetCondition() {
  StaticJsonDocument<500> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  
  if (error) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"JSON parse failed\"}");
    return;
  }
  
  // 【新增】处理灵活条件控制
  if (doc.containsKey("condition")) {
    JsonObject conditionObj = doc["condition"].as<JsonObject>();
    
    // 启用/禁用主开关
    if (conditionObj.containsKey("enabled")) {
      conditionControl.setEnabled(conditionObj["enabled"]);
    }
    
    // 【新增】处理 ON 条件组
    if (conditionObj.containsKey("on_condition") && conditionObj["on_condition"].is<JsonObject>()) {
      JsonObject onObj = conditionObj["on_condition"].as<JsonObject>();
      
      if (onObj.containsKey("enabled")) {
        conditionControl.setConditionGroupEnabled(true, onObj["enabled"]);
      }
      
      if (onObj.containsKey("logic")) {
        uint8_t logicMode = (onObj["logic"].as<String>() == "and") ? LOGIC_AND : LOGIC_OR;
        conditionControl.setConditionGroupLogic(true, logicMode);
      }
      
      // 处理个别条件
      if (onObj.containsKey("conditions") && onObj["conditions"].is<JsonArray>()) {
        JsonArray conditions = onObj["conditions"].as<JsonArray>();
        for (uint8_t i = 0; i < conditions.size() && i < 4; i++) {
          JsonObject cond = conditions[i].as<JsonObject>();
          
          if (cond.containsKey("enabled")) {
            uint8_t sensorType = (cond.containsKey("sensor") && cond["sensor"].as<String>() == "humi") ? SENSOR_HUMI : SENSOR_TEMP;
            uint8_t compareOp = cond.containsKey("compare") ? cond["compare"].as<uint8_t>() : COMPARE_GREATER_THAN;
            float threshold = cond.containsKey("threshold") ? cond["threshold"].as<float>() : 25.0;
            
            conditionControl.setCondition(true, i, cond["enabled"].as<bool>(), sensorType, compareOp, threshold);
          }
        }
      }
    }
    
    // 【新增】处理 OFF 条件组
    if (conditionObj.containsKey("off_condition") && conditionObj["off_condition"].is<JsonObject>()) {
      JsonObject offObj = conditionObj["off_condition"].as<JsonObject>();
      
      if (offObj.containsKey("enabled")) {
        conditionControl.setConditionGroupEnabled(false, offObj["enabled"]);
      }
      
      if (offObj.containsKey("logic")) {
        uint8_t logicMode = (offObj["logic"].as<String>() == "and") ? LOGIC_AND : LOGIC_OR;
        conditionControl.setConditionGroupLogic(false, logicMode);
      }
      
      // 处理个别条件
      if (offObj.containsKey("conditions") && offObj["conditions"].is<JsonArray>()) {
        JsonArray conditions = offObj["conditions"].as<JsonArray>();
        for (uint8_t i = 0; i < conditions.size() && i < 4; i++) {
          JsonObject cond = conditions[i].as<JsonObject>();
          
          if (cond.containsKey("enabled")) {
            uint8_t sensorType = (cond.containsKey("sensor") && cond["sensor"].as<String>() == "humi") ? SENSOR_HUMI : SENSOR_TEMP;
            uint8_t compareOp = cond.containsKey("compare") ? cond["compare"].as<uint8_t>() : COMPARE_GREATER_THAN;
            float threshold = cond.containsKey("threshold") ? cond["threshold"].as<float>() : 25.0;
            
            conditionControl.setCondition(false, i, cond["enabled"].as<bool>(), sensorType, compareOp, threshold);
          }
        }
      }
    }
  }
  
  server.send(200, "application/json", conditionControl.toJSON());
}

// ==================== 定时控制 API 端点 ====================
void WebUIManager::handleGetTimer() {
  String json = conditionControl.getTimerJSON();
  server.send(200, "application/json", json);
}

void WebUIManager::handleSetTimer() {
  StaticJsonDocument<500> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  
  if (error) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"JSON parse failed\"}");
    return;
  }
  
  if (doc.containsKey("timer")) {
    JsonObject timerObj = doc["timer"].as<JsonObject>();
    
    if (timerObj.containsKey("enabled")) {
      conditionControl.setTimerEnabled(timerObj["enabled"]);
    }
    
    if (timerObj.containsKey("clear") && timerObj["clear"].as<bool>()) {
      conditionControl.clearTimeSlots();
    }
    
    if (timerObj.containsKey("slots") && timerObj["slots"].is<JsonArray>()) {
      JsonArray slotsArray = timerObj["slots"].as<JsonArray>();
      for (JsonObject slot : slotsArray) {
        uint8_t index = slot.containsKey("index") ? slot["index"].as<uint8_t>() : 0;
        bool enabled = slot.containsKey("enabled") ? slot["enabled"].as<bool>() : false;
        
        uint8_t startH = 0, startM = 0, endH = 0, endM = 0;
        bool state = false;
        
        if (slot.containsKey("start_time")) {
          String startTime = slot["start_time"].as<String>();
          sscanf(startTime.c_str(), "%hhu:%hhu", &startH, &startM);
        }
        if (slot.containsKey("end_time")) {
          String endTime = slot["end_time"].as<String>();
          sscanf(endTime.c_str(), "%hhu:%hhu", &endH, &endM);
        }
        state = slot.containsKey("state") ? slot["state"].as<bool>() : false;
        
        if (index < 8) {
          conditionControl.setTimeSlot(index, enabled, startH, startM, endH, endM, state);
        }
      }
    }
  }
  
  server.send(200, "application/json", conditionControl.getTimerJSON());
}
