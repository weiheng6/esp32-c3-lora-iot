#include "web_ui.h"
#include "config.h"
#include "config_api.h"
#include "mqtt_manager.h"
#include "relay_control.h"
#include "sensor.h"
#include "condition_control.h"
#include "ota_manager.h"
#include "ntp_client.h"
#include "lora_manager.h"
#include "power_manager.h"
#include <ArduinoJson.h>
#include <Update.h>

extern bool manualRelayMode;
extern ConditionControl conditionControl;

WebUIManager webUIManager(80);

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>ESP32-C3 配置中心</title>
  <style>
    :root {
      --primary: #4F46E5;
      --primary-dark: #4338CA;
      --success: #10B981;
      --danger: #EF4444;
      --warning: #F59E0B;
      --info: #3B82F6;
      --gray-50: #F9FAFB;
      --gray-100: #F3F4F6;
      --gray-200: #E5E7EB;
      --gray-300: #D1D5DB;
      --gray-400: #9CA3AF;
      --gray-500: #6B7280;
      --gray-600: #4B5563;
      --gray-700: #374151;
      --gray-800: #1F2937;
      --gray-900: #111827;
    }
    
    * { margin: 0; padding: 0; box-sizing: border-box; }
    
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
      background: linear-gradient(135deg, var(--gray-100) 0%, var(--gray-200) 100%);
      min-height: 100vh;
      color: var(--gray-800);
    }
    
    .app-container {
      display: flex;
      min-height: 100vh;
    }
    
    .sidebar {
      width: 260px;
      background: var(--gray-900);
      color: white;
      padding: 20px 0;
      position: fixed;
      height: 100vh;
      overflow-y: auto;
      transition: transform 0.3s ease;
      z-index: 1000;
    }
    
    .sidebar-header {
      padding: 0 20px 20px;
      border-bottom: 1px solid var(--gray-700);
      margin-bottom: 20px;
    }
    
    .sidebar-header h1 {
      font-size: 18px;
      font-weight: 600;
      margin-bottom: 4px;
    }
    
    .sidebar-header p {
      font-size: 12px;
      color: var(--gray-400);
    }
    
    .nav-item {
      display: flex;
      align-items: center;
      padding: 12px 20px;
      color: var(--gray-300);
      text-decoration: none;
      transition: all 0.2s;
      cursor: pointer;
      border-left: 3px solid transparent;
    }
    
    .nav-item:hover {
      background: var(--gray-800);
      color: white;
    }
    
    .nav-item.active {
      background: var(--gray-800);
      color: white;
      border-left-color: var(--primary);
    }
    
    .nav-item .icon {
      margin-right: 12px;
      font-size: 18px;
      width: 24px;
      text-align: center;
    }
    
    .main-content {
      flex: 1;
      margin-left: 260px;
      padding: 24px;
      transition: margin-left 0.3s ease;
    }
    
    .mobile-header {
      display: none;
      background: var(--gray-900);
      color: white;
      padding: 16px;
      position: sticky;
      top: 0;
      z-index: 999;
    }
    
    .menu-toggle {
      background: none;
      border: none;
      color: white;
      font-size: 24px;
      cursor: pointer;
    }
    
    .page-header {
      margin-bottom: 24px;
    }
    
    .page-header h2 {
      font-size: 24px;
      font-weight: 600;
      color: var(--gray-900);
    }
    
    .page-header p {
      color: var(--gray-500);
      margin-top: 4px;
    }
    
    .card {
      background: white;
      border-radius: 12px;
      box-shadow: 0 1px 3px rgba(0,0,0,0.1);
      padding: 24px;
      margin-bottom: 20px;
    }
    
    .card-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 20px;
      padding-bottom: 16px;
      border-bottom: 1px solid var(--gray-200);
    }
    
    .card-title {
      font-size: 16px;
      font-weight: 600;
      color: var(--gray-800);
    }
    
    .status-badge {
      display: inline-flex;
      align-items: center;
      padding: 4px 12px;
      border-radius: 20px;
      font-size: 12px;
      font-weight: 500;
    }
    
    .status-badge.online {
      background: #D1FAE5;
      color: #065F46;
    }
    
    .status-badge.offline {
      background: #FEE2E2;
      color: #991B1B;
    }
    
    .status-badge.warning {
      background: #FEF3C7;
      color: #92400E;
    }
    
    .form-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
      gap: 20px;
    }
    
    .form-group {
      margin-bottom: 16px;
    }
    
    .form-group label {
      display: block;
      font-size: 14px;
      font-weight: 500;
      color: var(--gray-700);
      margin-bottom: 8px;
    }
    
    .form-control {
      width: 100%;
      padding: 10px 14px;
      border: 1px solid var(--gray-300);
      border-radius: 8px;
      font-size: 14px;
      transition: border-color 0.2s, box-shadow 0.2s;
    }
    
    .form-control:focus {
      outline: none;
      border-color: var(--primary);
      box-shadow: 0 0 0 3px rgba(79, 70, 229, 0.1);
    }
    
    .btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      padding: 10px 20px;
      border-radius: 8px;
      font-size: 14px;
      font-weight: 500;
      cursor: pointer;
      transition: all 0.2s;
      border: none;
      gap: 8px;
    }
    
    .btn-primary {
      background: var(--primary);
      color: white;
    }
    
    .btn-primary:hover {
      background: var(--primary-dark);
    }
    
    .btn-success {
      background: var(--success);
      color: white;
    }
    
    .btn-danger {
      background: var(--danger);
      color: white;
    }
    
    .btn-warning {
      background: var(--warning);
      color: white;
    }
    
    .btn-outline {
      background: transparent;
      border: 1px solid var(--gray-300);
      color: var(--gray-700);
    }
    
    .btn-outline:hover {
      background: var(--gray-100);
    }
    
    .btn-group {
      display: flex;
      gap: 12px;
      flex-wrap: wrap;
    }
    
    .stats-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      gap: 16px;
      margin-bottom: 24px;
    }
    
    .stat-card {
      background: white;
      border-radius: 12px;
      padding: 20px;
      box-shadow: 0 1px 3px rgba(0,0,0,0.1);
    }
    
    .stat-label {
      font-size: 12px;
      color: var(--gray-500);
      text-transform: uppercase;
      letter-spacing: 0.5px;
      margin-bottom: 8px;
    }
    
    .stat-value {
      font-size: 28px;
      font-weight: 700;
      color: var(--gray-900);
    }
    
    .stat-value.success { color: var(--success); }
    .stat-value.danger { color: var(--danger); }
    .stat-value.warning { color: var(--warning); }
    
    .stat-change {
      font-size: 12px;
      margin-top: 8px;
    }
    
    .toggle-switch {
      position: relative;
      display: inline-block;
      width: 48px;
      height: 24px;
    }
    
    .toggle-switch input {
      opacity: 0;
      width: 0;
      height: 0;
    }
    
    .toggle-slider {
      position: absolute;
      cursor: pointer;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background-color: var(--gray-300);
      transition: 0.3s;
      border-radius: 24px;
    }
    
    .toggle-slider:before {
      position: absolute;
      content: "";
      height: 18px;
      width: 18px;
      left: 3px;
      bottom: 3px;
      background-color: white;
      transition: 0.3s;
      border-radius: 50%;
    }
    
    .toggle-switch input:checked + .toggle-slider {
      background-color: var(--primary);
    }
    
    .toggle-switch input:checked + .toggle-slider:before {
      transform: translateX(24px);
    }
    
    .tab-nav {
      display: flex;
      border-bottom: 1px solid var(--gray-200);
      margin-bottom: 20px;
      overflow-x: auto;
    }
    
    .tab-btn {
      padding: 12px 20px;
      background: none;
      border: none;
      border-bottom: 2px solid transparent;
      cursor: pointer;
      font-size: 14px;
      font-weight: 500;
      color: var(--gray-500);
      white-space: nowrap;
      transition: all 0.2s;
    }
    
    .tab-btn:hover {
      color: var(--gray-700);
    }
    
    .tab-btn.active {
      color: var(--primary);
      border-bottom-color: var(--primary);
    }
    
    .tab-content {
      display: none;
    }
    
    .tab-content.active {
      display: block;
    }
    
    .alert {
      padding: 16px;
      border-radius: 8px;
      margin-bottom: 20px;
      display: none;
      animation: slideIn 0.3s ease-out;
    }
    
    .alert.show { display: block; }
    .alert.success { background: #D1FAE5; color: #065F46; border-left: 4px solid #10B981; }
    .alert.error { background: #FEE2E2; color: #991B1B; border-left: 4px solid #EF4444; }
    .alert.info { background: #DBEAFE; color: #1E40AF; border-left: 4px solid #3B82F6; }
    .alert.warning { background: #FEF3C7; color: #92400E; border-left: 4px solid #F59E0B; }
    
    @keyframes slideIn {
      from { opacity: 0; transform: translateY(-10px); }
      to { opacity: 1; transform: translateY(0); }
    }
    
    .table-container {
      overflow-x: auto;
    }
    
    table {
      width: 100%;
      border-collapse: collapse;
    }
    
    th, td {
      padding: 12px 16px;
      text-align: left;
      border-bottom: 1px solid var(--gray-200);
    }
    
    th {
      font-size: 12px;
      font-weight: 600;
      color: var(--gray-500);
      text-transform: uppercase;
      letter-spacing: 0.5px;
      background: var(--gray-50);
    }
    
    tr:hover {
      background: var(--gray-50);
    }
    
    .progress-bar {
      height: 8px;
      background: var(--gray-200);
      border-radius: 4px;
      overflow: hidden;
    }
    
    .progress-fill {
      height: 100%;
      background: var(--primary);
      transition: width 0.3s ease;
    }
    
    .progress-fill.success { background: var(--success); }
    .progress-fill.warning { background: var(--warning); }
    .progress-fill.danger { background: var(--danger); }
    
    .gauge-container {
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 20px;
    }
    
    .gauge {
      width: 150px;
      height: 150px;
      border-radius: 50%;
      background: conic-gradient(var(--primary) 0deg, var(--gray-200) 0deg);
      display: flex;
      justify-content: center;
      align-items: center;
      position: relative;
    }
    
    .gauge-inner {
      width: 120px;
      height: 120px;
      background: white;
      border-radius: 50%;
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
    }
    
    .gauge-value {
      font-size: 24px;
      font-weight: 700;
    }
    
    .gauge-label {
      font-size: 12px;
      color: var(--gray-500);
    }
    
    .input-group {
      display: flex;
      gap: 8px;
    }
    
    .input-group .form-control {
      flex: 1;
    }
    
    .info-box {
      background: var(--gray-50);
      border-left: 4px solid var(--info);
      padding: 12px 16px;
      border-radius: 4px;
      font-size: 13px;
      color: var(--gray-600);
      margin: 12px 0;
    }
    
    .datetime-display {
      background: var(--gray-900);
      color: white;
      padding: 12px 20px;
      border-radius: 8px;
      font-family: 'Courier New', monospace;
      font-size: 14px;
      margin-bottom: 20px;
    }
    
    @media (max-width: 768px) {
      .sidebar {
        transform: translateX(-100%);
      }
      
      .sidebar.open {
        transform: translateX(0);
      }
      
      .main-content {
        margin-left: 0;
      }
      
      .mobile-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
      }
      
      .form-grid {
        grid-template-columns: 1fr;
      }
      
      .stats-grid {
        grid-template-columns: repeat(2, 1fr);
      }
      
      .btn-group {
        flex-direction: column;
      }
      
      .btn-group .btn {
        width: 100%;
      }
    }
    
    .overlay {
      display: none;
      position: fixed;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background: rgba(0,0,0,0.5);
      z-index: 999;
    }
    
    .overlay.show {
      display: block;
    }
    
    .spinner {
      display: inline-block;
      width: 16px;
      height: 16px;
      border: 2px solid var(--gray-300);
      border-top-color: var(--primary);
      border-radius: 50%;
      animation: spin 0.8s linear infinite;
    }
    
    @keyframes spin {
      to { transform: rotate(360deg); }
    }
    
    .lora-preset-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
      gap: 12px;
      margin-bottom: 20px;
    }
    
    .preset-card {
      padding: 16px;
      border: 2px solid var(--gray-200);
      border-radius: 8px;
      text-align: center;
      cursor: pointer;
      transition: all 0.2s;
    }
    
    .preset-card:hover {
      border-color: var(--primary);
    }
    
    .preset-card.active {
      border-color: var(--primary);
      background: rgba(79, 70, 229, 0.05);
    }
    
    .preset-card .icon {
      font-size: 24px;
      margin-bottom: 8px;
    }
    
    .preset-card .name {
      font-size: 13px;
      font-weight: 500;
    }
    
    .power-mode-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
      gap: 16px;
    }
    
    .power-mode-card {
      padding: 20px;
      border: 2px solid var(--gray-200);
      border-radius: 12px;
      text-align: center;
      cursor: pointer;
      transition: all 0.2s;
    }
    
    .power-mode-card:hover {
      border-color: var(--primary);
      transform: translateY(-2px);
    }
    
    .power-mode-card.active {
      border-color: var(--success);
      background: rgba(16, 185, 129, 0.05);
    }
    
    .power-mode-card .icon {
      font-size: 32px;
      margin-bottom: 12px;
    }
    
    .power-mode-card .name {
      font-size: 14px;
      font-weight: 600;
      margin-bottom: 4px;
    }
    
    .power-mode-card .desc {
      font-size: 11px;
      color: var(--gray-500);
    }
  </style>
</head>
<body>
  <div class="overlay" id="overlay" onclick="closeSidebar()"></div>
  
  <div class="mobile-header">
    <button class="menu-toggle" onclick="toggleSidebar()">☰</button>
    <span>ESP32-C3 配置中心</span>
  </div>
  
  <div class="app-container">
    <nav class="sidebar" id="sidebar">
      <div class="sidebar-header">
        <h1>🔧 配置中心</h1>
        <p>ESP32-C3 智能设备</p>
      </div>
      
      <a class="nav-item active" onclick="switchPage('dashboard')">
        <span class="icon">📊</span> 仪表盘
      </a>
      <a class="nav-item" onclick="switchPage('device')">
        <span class="icon">📱</span> 设备配置
      </a>
      <a class="nav-item" onclick="switchPage('network')">
        <span class="icon">📡</span> 网络配置
      </a>
      <a class="nav-item" onclick="switchPage('control')">
        <span class="icon">🎮</span> 控制策略
      </a>
      <a class="nav-item" onclick="switchPage('lora')">
        <span class="icon">📻</span> LoRa设置
      </a>
      <a class="nav-item" onclick="switchPage('power')">
        <span class="icon">🔋</span> 功耗设置
      </a>
      <a class="nav-item" onclick="switchPage('system')">
        <span class="icon">⚙️</span> 系统设置
      </a>
      <a class="nav-item" onclick="switchPage('config')">
        <span class="icon">💾</span> 配置管理
      </a>
    </nav>
    
    <main class="main-content">
      <div class="datetime-display" id="datetime">📅 加载中...</div>
      <div class="alert" id="alert"></div>
      
      <!-- Dashboard Page -->
      <div id="page-dashboard" class="page">
        <div class="page-header">
          <h2>📊 系统仪表盘</h2>
          <p>实时监控设备状态和传感器数据</p>
        </div>
        
        <div class="stats-grid" id="statsGrid">
          <div class="stat-card">
            <div class="stat-label">温度</div>
            <div class="stat-value" id="statTemp">--°C</div>
          </div>
          <div class="stat-card">
            <div class="stat-label">湿度</div>
            <div class="stat-value" id="statHum">--%</div>
          </div>
          <div class="stat-card">
            <div class="stat-label">WiFi信号</div>
            <div class="stat-value" id="statRssi">-- dBm</div>
          </div>
          <div class="stat-card">
            <div class="stat-label">内存使用</div>
            <div class="stat-value" id="statMem">--%</div>
          </div>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">🔌 设备状态</span>
            <button class="btn btn-outline" onclick="refreshStatus()">🔄 刷新</button>
          </div>
          
          <div class="form-grid">
            <div>
              <div class="stat-label">WiFi 连接</div>
              <span class="status-badge" id="wifiStatus">检查中...</span>
            </div>
            <div>
              <div class="stat-label">MQTT 连接</div>
              <span class="status-badge" id="mqttStatus">检查中...</span>
            </div>
            <div>
              <div class="stat-label">LoRa 模块</div>
              <span class="status-badge" id="loraStatus">检查中...</span>
            </div>
            <div>
              <div class="stat-label">继电器状态</div>
              <span class="status-badge" id="relayStatusBadge">--</span>
            </div>
          </div>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">🎮 快速控制</span>
          </div>
          <div class="btn-group">
            <button class="btn btn-success" onclick="controlRelay(1)">✅ 开启继电器</button>
            <button class="btn btn-danger" onclick="controlRelay(0)">❌ 关闭继电器</button>
            <button class="btn btn-warning" onclick="toggleManualMode()">🔄 切换模式</button>
          </div>
          <div class="info-box">
            当前控制模式: <strong id="currentControlMode">--</strong>
          </div>
        </div>
      </div>
      
      <!-- Device Page -->
      <div id="page-device" class="page" style="display:none;">
        <div class="page-header">
          <h2>📱 设备配置</h2>
          <p>配置设备信息和引脚映射</p>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">🖥️ 设备信息</span>
          </div>
          <div class="form-grid" id="deviceForm">
            <div class="form-group">
              <label>设备名称</label>
              <input type="text" class="form-control" id="deviceName" placeholder="请输入设备名称">
            </div>
            <div class="form-group">
              <label>设备ID</label>
              <input type="text" class="form-control" id="deviceId" placeholder="请输入设备ID">
            </div>
            <div class="form-group">
              <label>采集间隔 (ms)</label>
              <input type="number" class="form-control" id="acqInterval" min="100" max="60000">
            </div>
            <div class="form-group">
              <label>上报间隔 (ms)</label>
              <input type="number" class="form-control" id="rptInterval" min="100" max="60000">
            </div>
            <div class="form-group">
              <label>自动上报</label>
              <div>
                <label class="toggle-switch">
                  <input type="checkbox" id="autoReport">
                  <span class="toggle-slider"></span>
                </label>
              </div>
            </div>
            <div class="form-group">
              <label>低功耗模式</label>
              <div>
                <label class="toggle-switch">
                  <input type="checkbox" id="lowPower">
                  <span class="toggle-slider"></span>
                </label>
              </div>
            </div>
          </div>
          <div class="btn-group" style="margin-top:20px;">
            <button class="btn btn-primary" onclick="saveDeviceConfig()">💾 保存配置</button>
            <button class="btn btn-outline" onclick="loadDeviceConfig()">🔄 刷新</button>
          </div>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">🔌 引脚映射</span>
          </div>
          <div class="table-container" id="pinTable">
            <table>
              <thead>
                <tr>
                  <th>引脚</th>
                  <th>功能</th>
                  <th>模式</th>
                  <th>状态</th>
                </tr>
              </thead>
              <tbody id="pinTableBody">
              </tbody>
            </table>
          </div>
        </div>
      </div>
      
      <!-- Network Page -->
      <div id="page-network" class="page" style="display:none;">
        <div class="page-header">
          <h2>📡 网络配置</h2>
          <p>配置WiFi、MQTT和传输模式</p>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">📶 WiFi配置</span>
          </div>
          <div class="form-grid">
            <div class="form-group">
              <label>WiFi SSID</label>
              <input type="text" class="form-control" id="wifiSsid" placeholder="请输入WiFi名称">
            </div>
            <div class="form-group">
              <label>WiFi 密码</label>
              <input type="password" class="form-control" id="wifiPass" placeholder="请输入WiFi密码">
            </div>
          </div>
          <div class="info-box">
            <strong>当前连接状态:</strong> <span id="wifiConnStatus">--</span>
          </div>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">📨 MQTT配置</span>
          </div>
          <div class="form-grid">
            <div class="form-group">
              <label>MQTT 服务器</label>
              <input type="text" class="form-control" id="mqttServer" placeholder="例: iot.example.com">
            </div>
            <div class="form-group">
              <label>MQTT 端口</label>
              <input type="number" class="form-control" id="mqttPort" min="1" max="65535" value="1883">
            </div>
            <div class="form-group">
              <label>MQTT 用户名</label>
              <input type="text" class="form-control" id="mqttUser" placeholder="请输入用户名">
            </div>
            <div class="form-group">
              <label>MQTT 密码</label>
              <input type="password" class="form-control" id="mqttPass" placeholder="请输入密码">
            </div>
            <div class="form-group">
              <label>MQTT Topic</label>
              <input type="text" class="form-control" id="mqttTopic" placeholder="请输入Topic">
            </div>
            <div class="form-group">
              <label>启用MQTT</label>
              <div>
                <label class="toggle-switch">
                  <input type="checkbox" id="mqttEnabled" checked>
                  <span class="toggle-slider"></span>
                </label>
              </div>
            </div>
          </div>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">🔄 传输模式</span>
          </div>
          <div class="form-grid">
            <div class="form-group">
              <label>传输模式</label>
              <select class="form-control" id="txMode">
                <option value="0">MQTT优先</option>
                <option value="1">LoRa优先</option>
                <option value="2">双模并发</option>
                <option value="3">仅MQTT</option>
                <option value="4">仅LoRa</option>
              </select>
            </div>
            <div class="form-group">
              <label>启用LoRa</label>
              <div>
                <label class="toggle-switch">
                  <input type="checkbox" id="loraEnabled" checked>
                  <span class="toggle-slider"></span>
                </label>
              </div>
            </div>
          </div>
          <div class="btn-group" style="margin-top:20px;">
            <button class="btn btn-primary" onclick="saveNetworkConfig()">💾 保存配置</button>
            <button class="btn btn-outline" onclick="loadNetworkConfig()">🔄 刷新</button>
          </div>
        </div>
      </div>
      
      <!-- Control Page -->
      <div id="page-control" class="page" style="display:none;">
        <div class="page-header">
          <h2>🎮 控制策略</h2>
          <p>配置条件控制和定时控制</p>
        </div>
        
        <div class="tab-nav">
          <button class="tab-btn active" onclick="switchTab('condition')">🎯 条件控制</button>
          <button class="tab-btn" onclick="switchTab('timer')">⏰ 定时控制</button>
          <button class="tab-btn" onclick="switchTab('manual')">🖱️ 手动控制</button>
        </div>
        
        <div id="tab-condition" class="tab-content active">
          <div class="card">
            <div class="card-header">
              <span class="card-title">🎯 条件控制配置</span>
              <label class="toggle-switch">
                <input type="checkbox" id="conditionEnabled" onchange="toggleCondition()">
                <span class="toggle-slider"></span>
              </label>
            </div>
            <div class="info-box">
              配置温度/湿度阈值，当条件满足时自动控制继电器开关
            </div>
            <div id="conditionContent">
              <div class="form-group">
                <label>开启条件 (温度大于)</label>
                <div class="input-group">
                  <input type="number" class="form-control" id="onThreshold" step="0.1" placeholder="30.0">
                  <button class="btn btn-outline" onclick="loadCondition()">刷新</button>
                </div>
              </div>
            </div>
            <div class="btn-group" style="margin-top:20px;">
              <button class="btn btn-primary" onclick="saveCondition()">💾 保存配置</button>
            </div>
          </div>
        </div>
        
        <div id="tab-timer" class="tab-content">
          <div class="card">
            <div class="card-header">
              <span class="card-title">⏰ 定时控制配置</span>
              <label class="toggle-switch">
                <input type="checkbox" id="timerEnabled" onchange="toggleTimer()">
                <span class="toggle-slider"></span>
              </label>
            </div>
            <div id="timerSlots">
              <div class="info-box">定时控制功能开发中...</div>
            </div>
          </div>
        </div>
        
        <div id="tab-manual" class="tab-content">
          <div class="card">
            <div class="card-header">
              <span class="card-title">🖱️ 手动控制</span>
            </div>
            <div class="form-grid">
              <div>
                <div class="stat-label">当前继电器状态</div>
                <div class="stat-value" id="manualRelayState">--</div>
              </div>
              <div>
                <div class="stat-label">控制模式</div>
                <div class="stat-value" id="manualModeState">--</div>
              </div>
            </div>
            <div class="btn-group" style="margin-top:20px;">
              <button class="btn btn-success" onclick="controlRelay(1)">✅ 开启</button>
              <button class="btn btn-danger" onclick="controlRelay(0)">❌ 关闭</button>
              <button class="btn btn-warning" onclick="toggleManualMode()">🔄 切换模式</button>
            </div>
          </div>
        </div>
      </div>
      
      <!-- LoRa Page -->
      <div id="page-lora" class="page" style="display:none;">
        <div class="page-header">
          <h2>📻 LoRa设置</h2>
          <p>配置LoRa无线通信参数</p>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">⚡ 预设配置</span>
          </div>
          <div class="lora-preset-grid">
            <div class="preset-card active" onclick="selectPreset(0)">
              <div class="icon">🚀</div>
              <div class="name">超远距离</div>
            </div>
            <div class="preset-card" onclick="selectPreset(1)">
              <div class="icon">📡</div>
              <div class="name">远距离</div>
            </div>
            <div class="preset-card" onclick="selectPreset(2)">
              <div class="icon">⚖️</div>
              <div class="name">平衡模式</div>
            </div>
            <div class="preset-card" onclick="selectPreset(3)">
              <div class="icon">⚡</div>
              <div class="name">快速传输</div>
            </div>
            <div class="preset-card" onclick="selectPreset(4)">
              <div class="icon">🔋</div>
              <div class="name">低功耗</div>
            </div>
          </div>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">📝 详细参数</span>
          </div>
          <div class="form-grid" id="loraForm">
            <div class="form-group">
              <label>频率 (Hz)</label>
              <input type="number" class="form-control" id="loraFreq" min="137000000" max="525000000" value="433000000">
            </div>
            <div class="form-group">
              <label>带宽 (Hz)</label>
              <select class="form-control" id="loraBw">
                <option value="7800">7.8 kHz</option>
                <option value="10400">10.4 kHz</option>
                <option value="15600">15.6 kHz</option>
                <option value="20800">20.8 kHz</option>
                <option value="31250">31.25 kHz</option>
                <option value="41700">41.7 kHz</option>
                <option value="62500">62.5 kHz</option>
                <option value="125000" selected>125 kHz</option>
                <option value="250000">250 kHz</option>
                <option value="500000">500 kHz</option>
              </select>
            </div>
            <div class="form-group">
              <label>扩频因子 (SF)</label>
              <select class="form-control" id="loraSf">
                <option value="6">SF6</option>
                <option value="7" selected>SF7</option>
                <option value="8">SF8</option>
                <option value="9">SF9</option>
                <option value="10">SF10</option>
                <option value="11">SF11</option>
                <option value="12">SF12</option>
              </select>
            </div>
            <div class="form-group">
              <label>编码率 (CR)</label>
              <select class="form-control" id="loraCr">
                <option value="5" selected>4/5</option>
                <option value="6">4/6</option>
                <option value="7">4/7</option>
                <option value="8">4/8</option>
              </select>
            </div>
            <div class="form-group">
              <label>发射功率 (dBm)</label>
              <input type="number" class="form-control" id="loraTxPower" min="0" max="20" value="17">
            </div>
            <div class="form-group">
              <label>前导码长度</label>
              <input type="number" class="form-control" id="loraPreamble" min="6" max="255" value="8">
            </div>
          </div>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">🔄 自适应配置</span>
            <label class="toggle-switch">
              <input type="checkbox" id="loraAdaptive">
              <span class="toggle-slider"></span>
            </label>
          </div>
          <div class="form-grid">
            <div class="form-group">
              <label>最小灵敏度 (dBm)</label>
              <input type="number" class="form-control" id="loraMinSens" value="-120">
            </div>
            <div class="form-group">
              <label>最大调整间隔 (ms)</label>
              <input type="number" class="form-control" id="loraMaxAdjust" value="30000">
            </div>
          </div>
          <div class="btn-group" style="margin-top:20px;">
            <button class="btn btn-primary" onclick="saveLoraConfig()">💾 保存配置</button>
            <button class="btn btn-outline" onclick="loadLoraConfig()">🔄 刷新</button>
          </div>
        </div>
      </div>
      
      <!-- Power Page -->
      <div id="page-power" class="page" style="display:none;">
        <div class="page-header">
          <h2>🔋 功耗设置</h2>
          <p>配置电源管理和电池监控</p>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">⚡ 功耗模式</span>
          </div>
          <div class="power-mode-grid">
            <div class="power-mode-card active" onclick="selectPowerMode(0)">
              <div class="icon">🏃</div>
              <div class="name">活跃模式</div>
              <div class="desc">全功能运行</div>
            </div>
            <div class="power-mode-card" onclick="selectPowerMode(1)">
              <div class="icon">👂</div>
              <div class="name">监听模式</div>
              <div class="desc">低功耗监听</div>
            </div>
            <div class="power-mode-card" onclick="selectPowerMode(2)">
              <div class="icon">💤</div>
              <div class="name">轻度睡眠</div>
              <div class="desc">快速唤醒</div>
            </div>
            <div class="power-mode-card" onclick="selectPowerMode(3)">
              <div class="icon">🌙</div>
              <div class="name">深度睡眠</div>
              <div class="desc">最低功耗</div>
            </div>
          </div>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">🔋 电池状态</span>
          </div>
          <div class="stats-grid">
            <div class="stat-card">
              <div class="stat-label">电压</div>
              <div class="stat-value" id="batteryVoltage">-- V</div>
            </div>
            <div class="stat-card">
              <div class="stat-label">电量</div>
              <div class="stat-value" id="batteryPercent">--%</div>
            </div>
            <div class="stat-card">
              <div class="stat-label">状态</div>
              <div class="stat-value" id="batteryStatus">--</div>
            </div>
          </div>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">📝 详细配置</span>
          </div>
          <div class="form-grid">
            <div class="form-group">
              <label>睡眠时长 (微秒)</label>
              <input type="number" class="form-control" id="sleepDuration" value="60000000">
            </div>
            <div class="form-group">
              <label>唤醒间隔 (微秒)</label>
              <input type="number" class="form-control" id="wakeInterval" value="300000000">
            </div>
            <div class="form-group">
              <label>CPU频率 (MHz)</label>
              <select class="form-control" id="cpuFreq">
                <option value="40">40 MHz</option>
                <option value="80" selected>80 MHz</option>
                <option value="120">120 MHz</option>
                <option value="160">160 MHz</option>
              </select>
            </div>
            <div class="form-group">
              <label>发射功率 (dBm)</label>
              <input type="number" class="form-control" id="txPower" min="0" max="20" value="17">
            </div>
          </div>
          <div class="btn-group" style="margin-top:20px;">
            <button class="btn btn-primary" onclick="savePowerConfig()">💾 保存配置</button>
            <button class="btn btn-outline" onclick="loadPowerConfig()">🔄 刷新</button>
          </div>
        </div>
      </div>
      
      <!-- System Page -->
      <div id="page-system" class="page" style="display:none;">
        <div class="page-header">
          <h2>⚙️ 系统设置</h2>
          <p>配置日志、OTA升级和安全选项</p>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">📋 系统信息</span>
          </div>
          <div class="form-grid">
            <div class="stat-card">
              <div class="stat-label">固件版本</div>
              <div class="stat-value" id="sysFwVer">v1.0.0</div>
            </div>
            <div class="stat-card">
              <div class="stat-label">可用内存</div>
              <div class="stat-value" id="sysFreeMem">-- KB</div>
            </div>
            <div class="stat-card">
              <div class="stat-label">运行时间</div>
              <div class="stat-value" id="sysUptime">--</div>
            </div>
          </div>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">📝 日志配置</span>
          </div>
          <div class="form-grid">
            <div class="form-group">
              <label>日志级别</label>
              <select class="form-control" id="logLevel">
                <option value="0">关闭</option>
                <option value="1" selected>错误</option>
                <option value="2">警告</option>
                <option value="3">信息</option>
                <option value="4">调试</option>
              </select>
            </div>
          </div>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">🔄 OTA升级</span>
          </div>
          <div class="form-grid">
            <div class="form-group">
              <label>自动检查更新</label>
              <div>
                <label class="toggle-switch">
                  <input type="checkbox" id="otaAutoCheck">
                  <span class="toggle-slider"></span>
                </label>
              </div>
            </div>
            <div class="form-group" style="grid-column: 1/-1;">
              <label>固件URL</label>
              <input type="text" class="form-control" id="otaUrl" placeholder="https://example.com/firmware.bin">
            </div>
          </div>
          <div class="btn-group" style="margin-top:20px;">
            <button class="btn btn-primary" onclick="checkOTA()">🔍 检查更新</button>
            <button class="btn btn-outline" onclick="saveSystemConfig()">💾 保存配置</button>
          </div>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">🔐 安全设置</span>
          </div>
          <div class="form-grid">
            <div class="form-group">
              <label>启用密码保护</label>
              <div>
                <label class="toggle-switch">
                  <input type="checkbox" id="securityEnabled">
                  <span class="toggle-slider"></span>
                </label>
              </div>
            </div>
            <div class="form-group">
              <label>管理密码</label>
              <input type="password" class="form-control" id="adminPassword" placeholder="请输入管理密码">
            </div>
          </div>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">🔄 系统控制</span>
          </div>
          <div class="btn-group">
            <button class="btn btn-danger" onclick="restartDevice()">🔌 重启设备</button>
            <button class="btn btn-warning" onclick="resetConfig()">⚠️ 恢复出厂</button>
          </div>
        </div>
      </div>
      
      <!-- Config Page -->
      <div id="page-config" class="page" style="display:none;">
        <div class="page-header">
          <h2>💾 配置管理</h2>
          <p>导入导出和备份恢复</p>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">📥 导出配置</span>
          </div>
          <p style="color:var(--gray-600);margin-bottom:16px;">将当前所有配置导出为JSON文件，便于备份和迁移</p>
          <button class="btn btn-primary" onclick="exportConfig()">📥 导出配置文件</button>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">📤 导入配置</span>
          </div>
          <p style="color:var(--gray-600);margin-bottom:16px;">从JSON文件导入配置，设备将应用新配置并重启</p>
          <input type="file" id="importFile" accept=".json" style="display:none" onchange="handleImport(event)">
          <button class="btn btn-outline" onclick="document.getElementById('importFile').click()">📤 选择文件导入</button>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">⚠️ 危险操作</span>
          </div>
          <div class="info-box" style="background:#FEE2E2;border-color:#EF4444;">
            恢复出厂设置将清除所有配置，包括WiFi、MQTT、LoRa等所有参数
          </div>
          <button class="btn btn-danger" onclick="resetAllConfig()">🗑️ 恢复出厂设置</button>
        </div>
      </div>
    </main>
  </div>

  <script>
    const API_BASE = window.location.origin;
    let currentPage = 'dashboard';
    let currentTab = 'condition';
    let currentPreset = 0;
    let currentPowerMode = 0;
    
    function toggleSidebar() {
      const sidebar = document.getElementById('sidebar');
      const overlay = document.getElementById('overlay');
      sidebar.classList.toggle('open');
      overlay.classList.toggle('show');
    }
    
    function closeSidebar() {
      const sidebar = document.getElementById('sidebar');
      const overlay = document.getElementById('overlay');
      sidebar.classList.remove('open');
      overlay.classList.remove('show');
    }
    
    function switchPage(page) {
      currentPage = page;
      document.querySelectorAll('.page').forEach(p => p.style.display = 'none');
      document.getElementById('page-' + page).style.display = 'block';
      document.querySelectorAll('.nav-item').forEach(n => n.classList.remove('active'));
      event.target.closest('.nav-item').classList.add('active');
      closeSidebar();
      
      switch(page) {
        case 'dashboard': refreshStatus(); break;
        case 'device': loadDeviceConfig(); break;
        case 'network': loadNetworkConfig(); break;
        case 'control': loadCondition(); break;
        case 'lora': loadLoraConfig(); break;
        case 'power': loadPowerConfig(); break;
        case 'system': loadSystemConfig(); break;
      }
    }
    
    function switchTab(tab) {
      currentTab = tab;
      document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
      document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
      document.getElementById('tab-' + tab).classList.add('active');
      event.target.classList.add('active');
    }
    
    function showAlert(message, type = 'info') {
      const alert = document.getElementById('alert');
      alert.textContent = message;
      alert.className = `alert show ${type}`;
      if (type !== 'error') {
        setTimeout(() => alert.classList.remove('show'), 4000);
      }
    }
    
    function updateDateTime() {
      fetch(`${API_BASE}/api/time`)
        .then(r => r.json())
        .then(data => {
          if (data.status === 'ok') {
            const dt = `${data.year}-${String(data.month).padStart(2,'0')}-${String(data.day).padStart(2,'0')} ${String(data.hour).padStart(2,'0')}:${String(data.minute).padStart(2,'0')}:${String(data.second).padStart(2,'0')}`;
            document.getElementById('datetime').textContent = '📅 ' + dt;
          }
        })
        .catch(() => {});
    }
    
    setInterval(updateDateTime, 1000);
    updateDateTime();
    
    async function refreshStatus() {
      try {
        const resp = await fetch(`${API_BASE}/api/status`);
        const data = await resp.json();
        
        document.getElementById('statTemp').textContent = data.temp ? data.temp.toFixed(1) + '°C' : '--°C';
        document.getElementById('statHum').textContent = data.hum ? data.hum.toFixed(1) + '%' : '--%';
        document.getElementById('statMem').textContent = data.mem ? data.mem.toFixed(0) + '%' : '--%';
        
        const wifiBadge = document.getElementById('wifiStatus');
        wifiBadge.className = `status-badge ${data.wifi ? 'online' : 'offline'}`;
        wifiBadge.textContent = data.wifi ? '✅ 已连接' : '❌ 断开';
        
        const mqttBadge = document.getElementById('mqttStatus');
        mqttBadge.className = `status-badge ${data.mqtt ? 'online' : 'offline'}`;
        mqttBadge.textContent = data.mqtt ? '✅ 已连接' : '❌ 断开';
        
        const loraBadge = document.getElementById('loraStatus');
        loraBadge.className = 'status-badge online';
        loraBadge.textContent = data.lora ? '✅ 正常' : '⚠️ 未连接';
        
        const relayBadge = document.getElementById('relayStatusBadge');
        relayBadge.className = `status-badge ${data.relay ? 'online' : 'offline'}`;
        relayBadge.textContent = data.relay ? '🔴 开启' : '⚫ 关闭';
        
        document.getElementById('statRssi').textContent = data.rssi ? data.rssi + ' dBm' : '-- dBm';
        
        let modeText = '未知';
        if (data.manual_mode) {
          modeText = '🖱️ 手动模式';
        } else if (data.control_mode === 'timer') {
          modeText = '⏰ 定时控制';
        } else if (data.control_mode === 'condition') {
          modeText = '🎯 条件控制';
        } else {
          modeText = '📵 已禁用';
        }
        document.getElementById('currentControlMode').textContent = modeText;
        
        document.getElementById('manualRelayState').textContent = data.relay ? '🔴 开启' : '⚫ 关闭';
        document.getElementById('manualModeState').textContent = data.manual_mode ? '🖱️ 手动' : '⚙️ 自动';
      } catch (e) {
        console.error('Status refresh error:', e);
      }
    }
    
    function controlRelay(state) {
      fetch(`${API_BASE}/api/relay`, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify({relay: state})
      })
        .then(r => r.json())
        .then(() => {
          showAlert(state ? '✅ 继电器已开启' : '✅ 继电器已关闭', 'success');
          setTimeout(refreshStatus, 500);
        })
        .catch(e => showAlert('控制失败: ' + e, 'error'));
    }
    
    function toggleManualMode() {
      fetch(`${API_BASE}/api/manual_mode`, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify({enabled: true})
      })
        .then(r => r.json())
        .then(() => {
          showAlert('🔄 模式已切换', 'success');
          setTimeout(refreshStatus, 500);
        })
        .catch(e => showAlert('切换失败: ' + e, 'error'));
    }
    
    async function loadDeviceConfig() {
      try {
        const resp = await fetch(`${API_BASE}/api/device/config`);
        const data = await resp.json();
        
        document.getElementById('deviceName').value = data.deviceName || '';
        document.getElementById('deviceId').value = data.deviceId || '';
        document.getElementById('acqInterval').value = data.采集间隔 || 1000;
        document.getElementById('rptInterval').value = data.上报间隔 || 5000;
        document.getElementById('autoReport').checked = data.自动上报开关 !== false;
        document.getElementById('lowPower').checked = data.低功耗模式 === true;
        
        loadPinConfig();
      } catch (e) {
        showAlert('加载设备配置失败', 'error');
      }
    }
    
    function saveDeviceConfig() {
      const config = {
        deviceName: document.getElementById('deviceName').value,
        deviceId: document.getElementById('deviceId').value,
        采集间隔: parseInt(document.getElementById('acqInterval').value),
        上报间隔: parseInt(document.getElementById('rptInterval').value),
        自动上报开关: document.getElementById('autoReport').checked,
        低功耗模式: document.getElementById('lowPower').checked
      };
      
      fetch(`${API_BASE}/api/device/config`, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify(config)
      })
        .then(r => r.json())
        .then(() => showAlert('✅ 设备配置已保存', 'success'))
        .catch(e => showAlert('保存失败: ' + e, 'error'));
    }
    
    async function loadPinConfig() {
      try {
        const resp = await fetch(`${API_BASE}/api/pin/config`);
        const data = await resp.json();
        
        const tbody = document.getElementById('pinTableBody');
        tbody.innerHTML = '';
        
        data.forEach(pin => {
          const tr = document.createElement('tr');
          tr.innerHTML = `
            <td>GPIO${pin.pin}</td>
            <td>${pin.functionName || '未分配'}</td>
            <td>${['输入','输出','上拉输入','下拉输入'][pin.mode] || '未知'}</td>
            <td>${pin.isReserved ? '🔒 保留' : '✅ 可用'}</td>
          `;
          tbody.appendChild(tr);
        });
      } catch (e) {
        console.error('Load pin config error:', e);
      }
    }
    
    async function loadNetworkConfig() {
      try {
        const resp = await fetch(`${API_BASE}/api/network/config`);
        const data = await resp.json();
        
        document.getElementById('wifiSsid').value = data.wifiSsid || '';
        document.getElementById('wifiPass').value = data.wifiPassword || '';
        document.getElementById('mqttServer').value = data.mqttServer || '';
        document.getElementById('mqttPort').value = data.mqttPort || 1883;
        document.getElementById('mqttUser').value = data.mqttUsername || '';
        document.getElementById('mqttPass').value = data.mqttPassword || '';
        document.getElementById('mqttTopic').value = data.mqttTopic || '';
        document.getElementById('mqttEnabled').checked = data.mqttEnabled !== false;
        document.getElementById('loraEnabled').checked = data.loraEnabled !== false;
        document.getElementById('txMode').value = data.transmissionMode || 0;
        
        const status = data.wifiConnected ? '✅ 已连接' : '❌ 未连接';
        document.getElementById('wifiConnStatus').innerHTML = `<span class="status-badge ${data.wifiConnected ? 'online' : 'offline'}">${status}</span>`;
      } catch (e) {
        showAlert('加载网络配置失败', 'error');
      }
    }
    
    function saveNetworkConfig() {
      const config = {
        wifiSsid: document.getElementById('wifiSsid').value,
        wifiPassword: document.getElementById('wifiPass').value,
        mqttServer: document.getElementById('mqttServer').value,
        mqttPort: parseInt(document.getElementById('mqttPort').value),
        mqttUsername: document.getElementById('mqttUser').value,
        mqttPassword: document.getElementById('mqttPass').value,
        mqttTopic: document.getElementById('mqttTopic').value,
        mqttEnabled: document.getElementById('mqttEnabled').checked,
        loraEnabled: document.getElementById('loraEnabled').checked,
        transmissionMode: parseInt(document.getElementById('txMode').value)
      };
      
      fetch(`${API_BASE}/api/network/config`, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify(config)
      })
        .then(r => r.json())
        .then(() => showAlert('✅ 网络配置已保存', 'success'))
        .catch(e => showAlert('保存失败: ' + e, 'error'));
    }
    
    function selectPreset(index) {
      currentPreset = index;
      document.querySelectorAll('.preset-card').forEach((c, i) => {
        c.classList.toggle('active', i === index);
      });
      
      const presets = [
        {freq:433000000,bw:125000,sf:12,cr:5,tx:20,pre:8},
        {freq:433000000,bw:125000,sf:9,cr:5,tx:17,pre:8},
        {freq:433000000,bw:125000,sf:7,cr:5,tx:17,pre:8},
        {freq:433000000,bw:250000,sf:7,cr:5,tx:17,pre:4},
        {freq:433000000,bw:125000,sf:10,cr:5,tx:2,pre:6}
      ];
      
      const p = presets[index];
      document.getElementById('loraFreq').value = p.freq;
      document.getElementById('loraBw').value = p.bw;
      document.getElementById('loraSf').value = p.sf;
      document.getElementById('loraCr').value = p.cr;
      document.getElementById('loraTxPower').value = p.tx;
      document.getElementById('loraPreamble').value = p.pre;
    }
    
    async function loadLoraConfig() {
      try {
        const resp = await fetch(`${API_BASE}/api/lora/config`);
        const data = await resp.json();
        
        document.getElementById('loraFreq').value = data.frequency || 433000000;
        document.getElementById('loraBw').value = data.bandwidth || 125000;
        document.getElementById('loraSf').value = data.spreadingFactor || 7;
        document.getElementById('loraCr').value = data.codingRate || 5;
        document.getElementById('loraTxPower').value = data.txPower || 17;
        document.getElementById('loraPreamble').value = data.preambleLength || 8;
        document.getElementById('loraAdaptive').checked = data.adaptiveEnabled === true;
        document.getElementById('loraMinSens').value = data.minSensitivity || -120;
        document.getElementById('loraMaxAdjust').value = data.maxAdjustInterval || 30000;
        
        selectPreset(data.currentPreset || 0);
      } catch (e) {
        showAlert('加载LoRa配置失败', 'error');
      }
    }
    
    function saveLoraConfig() {
      const config = {
        frequency: parseInt(document.getElementById('loraFreq').value),
        bandwidth: parseInt(document.getElementById('loraBw').value),
        spreadingFactor: parseInt(document.getElementById('loraSf').value),
        codingRate: parseInt(document.getElementById('loraCr').value),
        txPower: parseInt(document.getElementById('loraTxPower').value),
        preambleLength: parseInt(document.getElementById('loraPreamble').value),
        adaptiveEnabled: document.getElementById('loraAdaptive').checked,
        minSensitivity: parseInt(document.getElementById('loraMinSens').value),
        maxAdjustInterval: parseInt(document.getElementById('loraMaxAdjust').value)
      };
      
      fetch(`${API_BASE}/api/lora/config`, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify(config)
      })
        .then(r => r.json())
        .then(() => showAlert('✅ LoRa配置已保存', 'success'))
        .catch(e => showAlert('保存失败: ' + e, 'error'));
    }
    
    function selectPowerMode(index) {
      currentPowerMode = index;
      document.querySelectorAll('.power-mode-card').forEach((c, i) => {
        c.classList.toggle('active', i === index);
      });
    }
    
    async function loadPowerConfig() {
      try {
        const resp = await fetch(`${API_BASE}/api/power/config`);
        const data = await resp.json();
        
        document.getElementById('sleepDuration').value = data.sleepDuration || 60000000;
        document.getElementById('wakeInterval').value = data.wakeInterval || 300000000;
        document.getElementById('cpuFreq').value = data.cpuFreq || 80;
        document.getElementById('txPower').value = data.txPower || 17;
        
        document.getElementById('batteryVoltage').textContent = (data.batteryVoltage || 0).toFixed(2) + ' V';
        document.getElementById('batteryPercent').textContent = (data.batteryPercentage || 0) + '%';
        document.getElementById('batteryStatus').textContent = data.isCharging ? '⚡ 充电中' : '🔋 使用中';
        
        selectPowerMode(data.currentMode || 0);
      } catch (e) {
        showAlert('加载功耗配置失败', 'error');
      }
    }
    
    function savePowerConfig() {
      const config = {
        sleepDuration: parseInt(document.getElementById('sleepDuration').value),
        wakeInterval: parseInt(document.getElementById('wakeInterval').value),
        cpuFreq: parseInt(document.getElementById('cpuFreq').value),
        txPower: parseInt(document.getElementById('txPower').value)
      };
      
      fetch(`${API_BASE}/api/power/config`, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify(config)
      })
        .then(r => r.json())
        .then(() => showAlert('✅ 功耗配置已保存', 'success'))
        .catch(e => showAlert('保存失败: ' + e, 'error'));
    }
    
    async function loadCondition() {
      try {
        const resp = await fetch(`${API_BASE}/api/condition`);
        const data = await resp.json();
        
        document.getElementById('conditionEnabled').checked = data.enabled === true;
        if (data.on_condition && data.on_condition.conditions && data.on_condition.conditions[0]) {
          document.getElementById('onThreshold').value = data.on_condition.conditions[0].threshold || 30;
        }
      } catch (e) {
        console.error('Load condition error:', e);
      }
    }
    
    function toggleCondition() {
      const enabled = document.getElementById('conditionEnabled').checked;
      document.getElementById('conditionContent').style.display = enabled ? 'block' : 'none';
    }
    
    function saveCondition() {
      const config = {
        condition: {
          enabled: document.getElementById('conditionEnabled').checked,
          on_condition: {
            enabled: true,
            logic: 'and',
            conditions: [{
              enabled: true,
              sensor: 'temp',
              compare: 1,
              threshold: parseFloat(document.getElementById('onThreshold').value) || 30
            }]
          },
          off_condition: {
            enabled: true,
            logic: 'or',
            conditions: []
          }
        }
      };
      
      fetch(`${API_BASE}/api/condition`, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify(config)
      })
        .then(r => r.json())
        .then(() => showAlert('✅ 条件控制已保存', 'success'))
        .catch(e => showAlert('保存失败: ' + e, 'error'));
    }
    
    function toggleTimer() {
      const enabled = document.getElementById('timerEnabled').checked;
      document.getElementById('timerSlots').style.display = enabled ? 'block' : 'none';
    }
    
    async function loadSystemConfig() {
      try {
        const resp = await fetch(`${API_BASE}/api/system/config`);
        const data = await resp.json();
        
        document.getElementById('sysFwVer').textContent = 'v' + (data.firmwareVersion || '1.0.0');
        document.getElementById('sysFreeMem').textContent = Math.round((data.freeHeap || 0) / 1024) + ' KB';
        
        const uptime = data.uptime || 0;
        const hours = Math.floor(uptime / 3600);
        const mins = Math.floor((uptime % 3600) / 60);
        document.getElementById('sysUptime').textContent = hours + 'h ' + mins + 'm';
        
        document.getElementById('logLevel').value = data.日志级别 || 1;
        document.getElementById('otaAutoCheck').checked = data.OTA自动检查 === true;
        document.getElementById('otaUrl').value = data.OTAUrl || '';
        document.getElementById('securityEnabled').checked = data.安全开关 === true;
        document.getElementById('adminPassword').value = data.管理密码 || '';
      } catch (e) {
        console.error('Load system config error:', e);
      }
    }
    
    function saveSystemConfig() {
      const config = {
        日志级别: parseInt(document.getElementById('logLevel').value),
        OTA自动检查: document.getElementById('otaAutoCheck').checked,
        OTAUrl: document.getElementById('otaUrl').value,
        安全开关: document.getElementById('securityEnabled').checked,
        管理密码: document.getElementById('adminPassword').value
      };
      
      fetch(`${API_BASE}/api/system/config`, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify(config)
      })
        .then(r => r.json())
        .then(() => showAlert('✅ 系统配置已保存', 'success'))
        .catch(e => showAlert('保存失败: ' + e, 'error'));
    }
    
    function checkOTA() {
      showAlert('🔍 正在检查更新...', 'info');
    }
    
    function restartDevice() {
      if (confirm('确定要重启设备吗？设备将中断连接...')) {
        fetch(`${API_BASE}/api/restart`, {method:'POST'})
          .then(() => {
            showAlert('🔄 设备重启中...', 'info');
            setTimeout(() => location.reload(), 3000);
          })
          .catch(e => showAlert('重启失败: ' + e, 'error'));
      }
    }
    
    function resetConfig() {
      if (confirm('确定要恢复出厂设置吗？')) {
        showAlert('⚠️ 功能开发中...', 'warning');
      }
    }
    
    function exportConfig() {
      fetch(`${API_BASE}/api/export`)
        .then(r => r.blob())
        .then(blob => {
          const url = URL.createObjectURL(blob);
          const a = document.createElement('a');
          a.href = url;
          a.download = 'esp32_config_' + Date.now() + '.json';
          a.click();
          URL.revokeObjectURL(url);
          showAlert('✅ 配置已导出', 'success');
        })
        .catch(e => showAlert('导出失败: ' + e, 'error'));
    }
    
    function handleImport(event) {
      const file = event.target.files[0];
      if (!file) return;
      
      const reader = new FileReader();
      reader.onload = function(e) {
        try {
          const config = JSON.parse(e.target.result);
          fetch(`${API_BASE}/api/import`, {
            method: 'POST',
            headers: {'Content-Type':'application/json'},
            body: JSON.stringify(config)
          })
            .then(r => r.json())
            .then(() => {
              showAlert('✅ 配置已导入，设备将重启', 'success');
              setTimeout(() => location.reload(), 2000);
            })
            .catch(err => showAlert('导入失败: ' + err, 'error'));
        } catch (err) {
          showAlert('配置文件格式错误', 'error');
        }
      };
      reader.readAsText(file);
    }
    
    function resetAllConfig() {
      if (confirm('⚠️ 危险操作！确定要清除所有配置吗？这将删除所有设置！')) {
        if (confirm('再次确认：所有配置将被清除，设备将恢复出厂设置！')) {
          fetch(`${API_BASE}/api/reset`, {method:'POST'})
            .then(() => {
              showAlert('⚠️ 正在恢复出厂设置...', 'info');
              setTimeout(() => location.reload(), 3000);
            })
            .catch(e => showAlert('操作失败: ' + e, 'error'));
        }
      }
    }
    
    setInterval(refreshStatus, 5000);
    refreshStatus();
  </script>
</body>
</html>
)rawliteral";

WebUIManager::WebUIManager(uint16_t port) : server(port) {}

void WebUIManager::begin() {
  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/api/status", HTTP_GET, [this]() { handleGetStatus(); });
  server.on("/api/time", HTTP_GET, [this]() { handleGetTime(); });
  server.on("/api/config", HTTP_GET, [this]() { handleGetConfig(); });
  server.on("/api/config", HTTP_POST, [this]() { handleSetConfig(); });
  server.on("/api/relay", HTTP_POST, [this]() { handleRelayControl(); });
  server.on("/api/manual_mode", HTTP_POST, [this]() { handleManualMode(); });
  server.on("/api/condition", HTTP_GET, [this]() { handleGetCondition(); });
  server.on("/api/condition", HTTP_POST, [this]() { handleSetCondition(); });
  server.on("/api/timer", HTTP_GET, [this]() { handleGetTimer(); });
  server.on("/api/timer", HTTP_POST, [this]() { handleSetTimer(); });
  server.on("/api/restart", HTTP_POST, [this]() { handleRestart(); });
  server.on("/api/export", HTTP_GET, [this]() { handleExportConfig(); });
  server.on("/api/import", HTTP_POST, [this]() { handleImportConfig(); });
  server.on("/api/reset", HTTP_POST, [this]() { handleResetConfig(); });
  
  server.on("/api/ota/url", HTTP_POST, [this]() { handleOTAURL(); });
  server.on("/api/ota/upload", HTTP_POST, 
    [this]() { this->handleOTAUpload(); },
    [this]() { this->handleOTAUpload(); }
  );
  server.on("/api/ota/status", HTTP_GET, [this]() { handleOTAStatus(); });
  
  server.on("/api/device/config", HTTP_GET, [this]() { handleGetDeviceConfig(); });
  server.on("/api/device/config", HTTP_POST, [this]() { handleSetDeviceConfig(); });
  server.on("/api/pin/config", HTTP_GET, [this]() { handleGetPinConfig(); });
  server.on("/api/pin/config", HTTP_POST, [this]() { handleSetPinConfig(); });
  server.on("/api/network/config", HTTP_GET, [this]() { handleGetNetworkConfig(); });
  server.on("/api/network/config", HTTP_POST, [this]() { handleSetNetworkConfig(); });
  server.on("/api/lora/config", HTTP_GET, [this]() { handleGetLoraParams(); });
  server.on("/api/lora/config", HTTP_POST, [this]() { handleSetLoraParams(); });
  server.on("/api/power/config", HTTP_GET, [this]() { handleGetPowerConfig(); });
  server.on("/api/power/config", HTTP_POST, [this]() { handleSetPowerConfig(); });
  server.on("/api/control/strategy", HTTP_GET, [this]() { handleGetControlStrategy(); });
  server.on("/api/control/strategy", HTTP_POST, [this]() { handleSetControlStrategy(); });
  server.on("/api/system/config", HTTP_GET, [this]() { handleGetSystemConfig(); });
  server.on("/api/system/config", HTTP_POST, [this]() { handleSetSystemConfig(); });
  server.on("/api/transmission/mode", HTTP_GET, [this]() { handleGetTransmissionMode(); });
  server.on("/api/transmission/mode", HTTP_POST, [this]() { handleSetTransmissionMode(); });
  
  server.begin();
  isStarted = true;
  Serial.println("✅ Web UI Manager started");
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
  StaticJsonDocument<400> doc;
  doc["wifi"] = (WiFi.status() == WL_CONNECTED);
  doc["mqtt"] = mqttManager.isConnected();
  doc["relay"] = relayControl.getState();
  doc["lora"] = true;
  doc["mem"] = (100.0 * (ESP.getHeapSize() - ESP.getFreeHeap())) / ESP.getHeapSize();
  doc["rssi"] = WiFi.RSSI();
  doc["manual_mode"] = manualRelayMode;
  
  String currentMode = "none";
  if (!manualRelayMode) {
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
  }
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void WebUIManager::handleGetConfig() {
  StaticJsonDocument<200> doc;
  doc["mqtt_server"] = MQTT_SERVER;
  doc["mqtt_port"] = MQTT_PORT;
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void WebUIManager::handleSetConfig() {
  StaticJsonDocument<300> doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"status\":\"error\"}");
    return;
  }
  server.send(200, "application/json", "{\"status\":\"ok\"}");
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
    manualRelayMode = true;
  }
  
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebUIManager::handleManualMode() {
  StaticJsonDocument<100> doc;
  deserializeJson(doc, server.arg("plain"));
  
  if (doc.containsKey("enabled")) {
    manualRelayMode = doc["enabled"].as<bool>();
  }
  
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebUIManager::handleRestart() {
  server.send(200, "application/json", "{\"status\":\"ok\"}");
  delay(1000);
  ESP.restart();
}

void WebUIManager::handleResetConfig() {
  configAPI.resetToDefaults();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
  delay(1000);
  ESP.restart();
}

void WebUIManager::handleExportConfig() {
  String json = configAPI.exportAllConfig();
  server.send(200, "application/json", json);
}

void WebUIManager::handleImportConfig() {
  StaticJsonDocument<2048> doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"JSON parse failed\"}");
    return;
  }
  
  String jsonStr;
  serializeJson(doc, jsonStr);
  
  if (configAPI.importAllConfig(jsonStr)) {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\"}");
  }
}

void WebUIManager::handleGetCondition() {
  String json = conditionControl.toJSON();
  server.send(200, "application/json", json);
}

void WebUIManager::handleSetCondition() {
  StaticJsonDocument<500> doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"status\":\"error\"}");
    return;
  }
  
  if (doc.containsKey("condition")) {
    JsonObject conditionObj = doc["condition"].as<JsonObject>();
    
    if (conditionObj.containsKey("enabled")) {
      conditionControl.setEnabled(conditionObj["enabled"]);
    }
    
    if (conditionObj.containsKey("on_condition")) {
      JsonObject onObj = conditionObj["on_condition"].as<JsonObject>();
      if (onObj.containsKey("enabled")) {
        conditionControl.setConditionGroupEnabled(true, onObj["enabled"]);
      }
      if (onObj.containsKey("logic")) {
        uint8_t logicMode = (onObj["logic"].as<String>() == "and") ? 1 : 2;
        conditionControl.setConditionGroupLogic(true, logicMode);
      }
      if (onObj.containsKey("conditions")) {
        JsonArray conditions = onObj["conditions"].as<JsonArray>();
        for (uint8_t i = 0; i < conditions.size() && i < 4; i++) {
          JsonObject cond = conditions[i].as<JsonObject>();
          if (cond.containsKey("enabled")) {
            uint8_t sensorType = (cond.containsKey("sensor") && cond["sensor"].as<String>() == "humi") ? 1 : 0;
            uint8_t compareOp = cond.containsKey("compare") ? cond["compare"].as<uint8_t>() : 1;
            float threshold = cond.containsKey("threshold") ? cond["threshold"].as<float>() : 25.0;
            conditionControl.setCondition(true, i, cond["enabled"].as<bool>(), sensorType, compareOp, threshold);
          }
        }
      }
    }
    
    if (conditionObj.containsKey("off_condition")) {
      JsonObject offObj = conditionObj["off_condition"].as<JsonObject>();
      if (offObj.containsKey("enabled")) {
        conditionControl.setConditionGroupEnabled(false, offObj["enabled"]);
      }
      if (offObj.containsKey("logic")) {
        uint8_t logicMode = (offObj["logic"].as<String>() == "and") ? 1 : 2;
        conditionControl.setConditionGroupLogic(false, logicMode);
      }
    }
  }
  
  server.send(200, "application/json", conditionControl.toJSON());
}

void WebUIManager::handleGetTimer() {
  StaticJsonDocument<550> wrapperDoc;
  String timerJson = conditionControl.getTimerJSON();
  
  StaticJsonDocument<500> timerDoc;
  deserializeJson(timerDoc, timerJson);
  wrapperDoc["timer"] = timerDoc.as<JsonObject>();
  
  String json;
  serializeJson(wrapperDoc, json);
  server.send(200, "application/json", json);
}

void WebUIManager::handleSetTimer() {
  StaticJsonDocument<500> doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"status\":\"error\"}");
    return;
  }
  
  if (doc.containsKey("timer")) {
    JsonObject timerObj = doc["timer"].as<JsonObject>();
    
    if (timerObj.containsKey("enabled")) {
      conditionControl.setTimerEnabled(timerObj["enabled"]);
    }
    
    if (timerObj.containsKey("slots")) {
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

void WebUIManager::handleGetTime() {
  StaticJsonDocument<150> doc;
  struct tm timeinfo;
  
  if (ntpClient.getTime(&timeinfo, 2000)) {
    doc["status"] = "ok";
    doc["synced"] = true;
    doc["year"] = timeinfo.tm_year + 1900;
    doc["month"] = timeinfo.tm_mon + 1;
    doc["day"] = timeinfo.tm_mday;
    doc["hour"] = timeinfo.tm_hour;
    doc["minute"] = timeinfo.tm_min;
    doc["second"] = timeinfo.tm_sec;
  } else {
    doc["status"] = "error";
    doc["synced"] = false;
  }
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void WebUIManager::handleOTAURL() {
  StaticJsonDocument<200> doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"status\":\"error\"}");
    return;
  }
  
  if (!doc.containsKey("url")) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Missing URL\"}");
    return;
  }
  
  String url = doc["url"].as<String>();
  
  xTaskCreatePinnedToCore([](void *parameter) {
    String* urlPtr = (String*)parameter;
    otaManager.updateFromHTTP(*urlPtr);
    delete urlPtr;
    vTaskDelete(NULL);
  }, "OTAUpdate", 4096, new String(url), 5, NULL, 1);
  
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebUIManager::handleOTAUpload() {
  HTTPUpload& upload = server.upload();
  static bool uploadFailed = false;
  
  if (upload.status == UPLOAD_FILE_START) {
    uploadFailed = false;
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      uploadFailed = true;
    }
    otaManager.setStatus(OTA_STATUS_UPDATING);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!uploadFailed) {
      Update.write(upload.buf, upload.currentSize);
      int progress = (upload.totalSize > 0) ? (Update.progress() * 100) / upload.totalSize : 0;
      otaManager.setProgress(progress);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      otaManager.setStatus(OTA_STATUS_COMPLETED);
      server.send(200, "application/json", "{\"status\":\"ok\"}");
      delay(100);
      ESP.restart();
    } else {
      otaManager.setStatus(OTA_STATUS_FAILED);
      server.send(500, "application/json", "{\"status\":\"error\"}");
    }
  }
}

void WebUIManager::handleOTAStatus() {
  StaticJsonDocument<100> doc;
  
  OTAManagerStatus status = otaManager.getStatus();
  int progress = otaManager.getProgress();
  
  switch (status) {
    case OTA_STATUS_IDLE: doc["status"] = "idle"; break;
    case OTA_STATUS_UPDATING: doc["status"] = "updating"; doc["progress"] = progress; break;
    case OTA_STATUS_COMPLETED: doc["status"] = "completed"; break;
    case OTA_STATUS_FAILED: doc["status"] = "failed"; break;
  }
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void WebUIManager::handleGetDeviceConfig() {
  String json = configAPI.getDeviceConfig();
  server.send(200, "application/json", json);
}

void WebUIManager::handleSetDeviceConfig() {
  String body = server.arg("plain");
  if (configAPI.setDeviceConfig(body)) {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\"}");
  }
}

void WebUIManager::handleGetPinConfig() {
  String json = configAPI.getPinConfig();
  server.send(200, "application/json", json);
}

void WebUIManager::handleSetPinConfig() {
  String body = server.arg("plain");
  if (configAPI.setPinConfig(body)) {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\"}");
  }
}

void WebUIManager::handleGetNetworkConfig() {
  String json = configAPI.getNetworkConfig();
  server.send(200, "application/json", json);
}

void WebUIManager::handleSetNetworkConfig() {
  String body = server.arg("plain");
  if (configAPI.setNetworkConfig(body)) {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\"}");
  }
}

void WebUIManager::handleGetLoraParams() {
  String json = configAPI.getLoraParams();
  server.send(200, "application/json", json);
}

void WebUIManager::handleSetLoraParams() {
  String body = server.arg("plain");
  if (configAPI.setLoraParams(body)) {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\"}");
  }
}

void WebUIManager::handleGetPowerConfig() {
  String json = configAPI.getPowerConfig();
  server.send(200, "application/json", json);
}

void WebUIManager::handleSetPowerConfig() {
  String body = server.arg("plain");
  if (configAPI.setPowerConfig(body)) {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\"}");
  }
}

void WebUIManager::handleGetControlStrategy() {
  String json = configAPI.getControlStrategy();
  server.send(200, "application/json", json);
}

void WebUIManager::handleSetControlStrategy() {
  String body = server.arg("plain");
  if (configAPI.setControlStrategy(body)) {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\"}");
  }
}

void WebUIManager::handleGetSystemConfig() {
  String json = configAPI.getSystemConfig();
  server.send(200, "application/json", json);
}

void WebUIManager::handleSetSystemConfig() {
  String body = server.arg("plain");
  if (configAPI.setSystemConfig(body)) {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\"}");
  }
}

void WebUIManager::handleGetTransmissionMode() {
  String json = configAPI.getTransmissionMode();
  server.send(200, "application/json", json);
}

void WebUIManager::handleSetTransmissionMode() {
  String body = server.arg("plain");
  if (configAPI.setTransmissionMode(body)) {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\"}");
  }
}
