#include "web_ui.h"
#include "config.h"
#include "config_api.h"
#include "mqtt_manager.h"
#include "mqtt_protocol.h"
#include "relay_control.h"
#include "sensor.h"
#include "condition_control.h"
#include "ota_manager.h"
#include "ntp_client.h"
#include "lora_manager.h"
#include "power_manager.h"
#include "wifi_manager.h"
#include <ArduinoJson.h>
#include <Update.h>

extern bool manualRelayMode;
extern ConditionControl conditionControl;
extern unsigned long mqttReportInterval;

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
    
    .modal {
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: rgba(0,0,0,0.7);
      display: flex;
      justify-content: center;
      align-items: center;
      z-index: 1000;
    }
    
    .modal-content {
      background: var(--bg-secondary);
      border-radius: 16px;
      width: 90%;
      max-width: 500px;
      max-height: 90vh;
      overflow-y: auto;
      box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.5);
    }
    
    .modal-header {
      padding: 20px 24px;
      border-bottom: 1px solid var(--gray-700);
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    
    .modal-header h3 {
      margin: 0;
      color: var(--text-primary);
    }
    
    .modal-close {
      font-size: 28px;
      color: var(--gray-400);
      cursor: pointer;
      line-height: 1;
    }
    
    .modal-close:hover {
      color: var(--text-primary);
    }
    
    .modal-body {
      padding: 24px;
    }
    
    .modal-footer {
      padding: 16px 24px;
      border-top: 1px solid var(--gray-700);
      display: flex;
      justify-content: flex-end;
      gap: 12px;
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
            <span class="card-title">🔌 设备状态概览</span>
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
            <span class="card-title">⚙️ 当前配置详情</span>
          </div>
          <div class="form-grid">
            <div>
              <div class="stat-label">采集间隔</div>
              <div class="stat-value" id="dashAcqInterval" style="font-size:20px;">-- ms</div>
            </div>
            <div>
              <div class="stat-label">上报间隔</div>
              <div class="stat-value" id="dashRptInterval" style="font-size:20px;">-- ms</div>
            </div>
            <div>
              <div class="stat-label">传输模式</div>
              <div class="stat-value" id="dashTxMode" style="font-size:16px;">--</div>
            </div>
            <div>
              <div class="stat-label">LoRa状态</div>
              <div class="stat-value" id="dashLoraStatus" style="font-size:16px;">--</div>
            </div>
            <div>
              <div class="stat-label">LoRa预设</div>
              <div class="stat-value" id="dashLoraPreset" style="font-size:16px;">--</div>
            </div>
            <div>
              <div class="stat-label">功耗模式</div>
              <div class="stat-value" id="dashPowerMode" style="font-size:16px;">--</div>
            </div>
            <div>
              <div class="stat-label">CPU频率</div>
              <div class="stat-value" id="dashCpuFreq" style="font-size:16px;">-- MHz</div>
            </div>
          </div>
        </div>
        
        <div class="card" id="timerTaskCard" style="display:none;">
          <div class="card-header">
            <span class="card-title">⏰ 定时任务详情</span>
            <span class="status-badge info" id="nextTaskBadge">下一个任务</span>
          </div>
          <div id="timerTaskList">
            <div class="info-box">暂无定时任务配置</div>
          </div>
        </div>
        
        <div class="card" id="conditionDetailCard" style="display:none;">
          <div class="card-header">
            <span class="card-title">🎯 条件控制详情</span>
          </div>
          <div class="form-grid" id="conditionDetailList">
          </div>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">🌐 网络配置</span>
          </div>
          <div class="form-grid">
            <div>
              <div class="stat-label">WiFi SSID</div>
              <div id="dashWifiSsid" style="font-size:14px;">--</div>
            </div>
            <div>
              <div class="stat-label">本地IP</div>
              <div id="dashLocalIp" style="font-size:14px;">--</div>
            </div>
            <div>
              <div class="stat-label">MQTT服务器</div>
              <div id="dashMqttServer" style="font-size:14px;">--</div>
            </div>
            <div>
              <div class="stat-label">MQTT端口</div>
              <div id="dashMqttPort" style="font-size:14px;">--</div>
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
          <div class="info-box" style="margin-top:12px;">
            <strong>当前控制模式:</strong> <span id="currentControlMode">--</span>
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
            <span class="card-title">🔌 引脚映射配置</span>
            <span class="status-badge info">支持动态配置</span>
          </div>
          <div class="info-box" style="margin-bottom:16px;">
            <strong>💡 使用说明：</strong>点击"编辑"按钮可配置GPIO引脚功能，支持继电器、传感器I2C、LoRa等模块。<br>
            <strong>⚠️ 注意：</strong>GPIO6-11为SPI Flash引脚，建议勿用；GPIO19/20为USB引脚，仅用于USB功能。
          </div>
          
          <div class="card" style="background:var(--gray-50);">
            <div class="card-title" style="margin-bottom:12px;">📋 当前引脚配置</div>
            <div class="table-container">
              <table>
                <thead>
                  <tr>
                    <th>ESP32-C3 引脚</th>
                    <th>功能</th>
                    <th>说明</th>
                    <th>模式</th>
                    <th>状态</th>
                  </tr>
                </thead>
                <tbody id="currentPinTableBody">
                </tbody>
              </table>
            </div>
          </div>
          
          <div class="card" style="margin-top:16px;background:#FEF3C7;">
            <div class="card-title" style="margin-bottom:12px;">⚠️ ESP32-C3 引脚兼容性说明</div>
            <div class="form-grid">
              <div class="form-group">
                <div class="stat-label" style="color:#92400E;">📊 传感器支持引脚</div>
                <div style="font-size:14px;">GPIO 0, 1, 2, 3, 4, 5</div>
                <div style="font-size:12px;color:#78716C;">支持I2C传感器(AHT10等)</div>
              </div>
              <div class="form-group">
                <div class="stat-label" style="color:#92400E;">📡 LoRa支持引脚</div>
                <div style="font-size:14px;">GPIO 0-5, 12-18</div>
                <div style="font-size:12px;color:#78716C;">支持SPI接口的LoRa模块</div>
              </div>
              <div class="form-group">
                <div class="stat-label" style="color:#DC2626;">⛔ 建议勿用</div>
                <div style="font-size:14px;">GPIO 6, 7, 8, 9, 10, 11</div>
                <div style="font-size:12px;color:#78716C;">SPI Flash占用引脚</div>
              </div>
              <div class="form-group">
                <div class="stat-label" style="color:#7C3AED;">🔌 USB引脚</div>
                <div style="font-size:14px;">GPIO 19, 20</div>
                <div style="font-size:12px;color:#78716C;">USB D-/D+，勿占用</div>
              </div>
            </div>
          </div>
          
          <div class="btn-group" style="margin-top:20px;">
            <button class="btn btn-primary" onclick="showPinConfigModal()">➕ 添加引脚配置</button>
            <button class="btn btn-outline" onclick="loadPinConfig()">🔄 刷新配置</button>
            <button class="btn btn-warning" onclick="resetPinConfig()">🔙 恢复默认</button>
          </div>
        </div>
        
        <!-- WiFi选择模态框 -->
        <div id="wifiModal" class="modal" style="display:none;">
          <div class="modal-content">
            <div class="modal-header">
              <h3>📡 选择WiFi网络</h3>
              <span class="modal-close" onclick="closeWifiModal()">&times;</span>
            </div>
            <div class="modal-body">
              <div id="wifiScanStatus" style="text-align:center;padding:20px;">点击"扫描"按钮开始搜索WiFi网络</div>
              <div id="wifiListContainer" style="display:none;"></div>
            </div>
            <div class="modal-footer">
              <button class="btn btn-outline" onclick="scanWiFi()">🔄 重新扫描</button>
              <button class="btn btn-outline" onclick="closeWifiModal()">取消</button>
            </div>
          </div>
        </div>
        
        <!-- 引脚配置模态框 -->
        <div id="pinConfigModal" class="modal" style="display:none;">
          <div class="modal-content">
            <div class="modal-header">
              <h3>🔧 引脚配置</h3>
              <span class="modal-close" onclick="closePinConfigModal()">&times;</span>
            </div>
            <div class="modal-body">
              <div class="form-group">
                <label>GPIO 引脚</label>
                <select class="form-control" id="configPinNumber" onchange="onPinSelectChange()">
                  <option value="">-- 选择GPIO引脚 --</option>
                  <optgroup label="✅ 通用GPIO (推荐)">
                    <option value="0">GPIO0 - 按钮/LED (启动引脚)</option>
                    <option value="1">GPIO1</option>
                    <option value="2">GPIO2</option>
                    <option value="3">GPIO3</option>
                    <option value="4">GPIO4</option>
                    <option value="5">GPIO5</option>
                  </optgroup>
                  <optgroup label="🔵 RGB LED">
                    <option value="12">GPIO12 - Neopixel RGB</option>
                    <option value="13">GPIO13 - Neopixel RGB</option>
                  </optgroup>
                  <optgroup label="🔌 其他GPIO">
                    <option value="14">GPIO14</option>
                    <option value="15">GPIO15</option>
                    <option value="16">GPIO16</option>
                    <option value="17">GPIO17</option>
                    <option value="18">GPIO18</option>
                    <option value="21">GPIO21</option>
                  </optgroup>
                  <optgroup label="⛔ 建议勿用">
                    <option value="6" disabled>GPIO6 - SPI Flash</option>
                    <option value="7" disabled>GPIO7 - SPI Flash</option>
                    <option value="8" disabled>GPIO8 - SPI Flash</option>
                    <option value="9" disabled>GPIO9 - SPI Flash</option>
                    <option value="10" disabled>GPIO10 - SPI Flash</option>
                    <option value="11" disabled>GPIO11 - SPI Flash</option>
                    <option value="19" disabled>GPIO19 - USB D-</option>
                    <option value="20" disabled>GPIO20 - USB D+</option>
                  </optgroup>
                </select>
                <div id="pinWarning" class="info-box warning" style="display:none;margin-top:8px;"></div>
              </div>
              
              <div class="form-group">
                <label>功能类型</label>
                <select class="form-control" id="configPinFunction">
                  <option value="">-- 选择功能 --</option>
                  <optgroup label="⚡ 输出控制">
                    <option value="relay">🔌 继电器控制</option>
                  </optgroup>
                  <optgroup label="📊 传感器">
                    <option value="sensor_sda">🌡️ 传感器 I2C SDA</option>
                    <option value="sensor_scl">💧 传感器 I2C SCL</option>
                  </optgroup>
                  <optgroup label="📡 LoRa模块">
                    <option value="lora_sck">📡 LoRa SPI时钟 (SCK)</option>
                    <option value="lora_miso">📡 LoRa SPI数据输入 (MISO)</option>
                    <option value="lora_mosi">📡 LoRa SPI数据输出 (MOSI)</option>
                    <option value="lora_nss">📡 LoRa SPI片选 (NSS)</option>
                    <option value="lora_reset">📡 LoRa复位</option>
                    <option value="lora_dio0">📡 LoRa中断 (DIO0)</option>
                  </optgroup>
                </select>
              </div>
              
              <div class="form-group">
                <label>引脚模式</label>
                <select class="form-control" id="configPinMode">
                  <option value="1">🔴 输出模式 (OUTPUT)</option>
                  <option value="5">📊 ADC模拟输入</option>
                  <option value="6">🌐 I2C SDA</option>
                  <option value="7">⏱️ I2C SCL</option>
                  <option value="8">📤 SPI MOSI</option>
                  <option value="9">📥 SPI MISO</option>
                  <option value="10">⏱️ SPI SCK</option>
                </select>
              </div>
              
              <div class="form-group" id="initialValueGroup">
                <label>初始值</label>
                <select class="form-control" id="configInitialValue">
                  <option value="0">⚫ 低电平 (0V)</option>
                  <option value="1">🔴 高电平 (3.3V)</option>
                </select>
              </div>
              
              <div id="pinConflictWarning" class="info-box danger" style="display:none;margin-top:12px;">
                <strong>⚠️ 冲突警告：</strong><span id="conflictDetails"></span>
              </div>
            </div>
            <div class="modal-footer">
              <button class="btn btn-outline" onclick="closePinConfigModal()">取消</button>
              <button class="btn btn-primary" onclick="saveSinglePinConfig()">💾 保存</button>
            </div>
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
              <div style="display: flex; gap: 10px;">
                <input type="text" class="form-control" id="wifiSsid" placeholder="请输入WiFi名称" style="flex: 1;">
                <button class="btn btn-primary" id="wifiScanBtn" onclick="scanWiFi()" style="white-space: nowrap;">🔍 扫描</button>
              </div>
            </div>
            <div class="form-group">
              <label>WiFi 密码</label>
              <input type="password" class="form-control" id="wifiPass" placeholder="请输入WiFi密码">
            </div>
          </div>
          <div class="btn-group" style="margin-top:16px;">
            <button class="btn btn-primary" onclick="saveWifiConfig()">💾 保存WiFi</button>
          </div>
          <div class="info-box" style="margin-top:12px;">
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
          </div>
          <div class="btn-group" style="margin-top:16px;">
            <button class="btn btn-primary" onclick="saveMqttConfig()">💾 保存MQTT</button>
          </div>
          <div class="form-group" style="margin-top:16px;">
            <label>启用MQTT</label>
            <div style="display: flex; align-items: center; gap: 10px;">
              <label class="toggle-switch">
                <input type="checkbox" id="mqttEnabled" onchange="saveMqttEnabled()">
                <span class="toggle-slider"></span>
              </label>
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
              <select class="form-control" id="txMode" onchange="saveTxMode()">
                <option value="0">MQTT优先</option>
                <option value="1">LoRa优先</option>
                <option value="2">双模并发</option>
                <option value="3">仅MQTT</option>
                <option value="4">仅LoRa</option>
              </select>
            </div>
            <div class="form-group">
              <label>启用LoRa</label>
              <div style="display: flex; align-items: center; gap: 10px;">
                <label class="toggle-switch">
                  <input type="checkbox" id="loraEnabled" onchange="saveLoraEnabled()">
                  <span class="toggle-slider"></span>
                </label>
              </div>
            </div>
          </div>
        </div>
        
        <div class="card">
          <div class="card-header">
            <span class="card-title">📨 MQTT协议模式</span>
            <span class="status-badge" id="mqttProtoModeStatus">JSON</span>
          </div>
          <div class="info-box" style="margin-bottom:16px;">
            <strong>JSON模式:</strong> 便于调试和运维人员阅读 | <strong>Protobuf模式:</strong> 节省流量，提高LoRa传输效率
          </div>
          <div class="power-mode-grid">
            <div class="power-mode-card active" id="protoCardJson" onclick="switchMqttProtoMode('json')">
              <div class="icon">📝</div>
              <div class="name">JSON 模式</div>
              <div class="desc">易于调试阅读</div>
            </div>
            <div class="power-mode-card" id="protoCardProtobuf" onclick="switchMqttProtoMode('protobuf')">
              <div class="icon">⚡</div>
              <div class="name">Protobuf 模式</div>
              <div class="desc">节省流量高效</div>
            </div>
          </div>
          <div class="btn-group" style="margin-top:20px;">
            <button class="btn btn-outline" onclick="switchMqttProtoMode()">🔄 切换协议模式</button>
            <button class="btn btn-outline" onclick="loadMqttProtoMode()">🔄 刷新状态</button>
          </div>
        </div>
        
        <div class="btn-group" style="margin-top:20px;">
          <button class="btn btn-primary" onclick="saveNetworkConfig()">💾 保存配置</button>
          <button class="btn btn-outline" onclick="loadNetworkConfig()">🔄 刷新</button>
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
              配置温度/湿度阈值，当条件满足时自动控制继电器开关。开启条件和关闭条件可以独立配置。
            </div>
            <div id="conditionContent" style="display:none;">
              <div class="card" style="background:var(--gray-50);margin-top:16px;">
                <div class="card-title" style="margin-bottom:16px;">✅ 开启条件（满足以下条件时继电器开启）</div>
                <div class="form-grid">
                  <div class="form-group">
                    <label>传感器类型</label>
                    <select class="form-control" id="onSensorType">
                      <option value="temp">温度 (°C)</option>
                      <option value="humi">湿度 (%)</option>
                    </select>
                  </div>
                  <div class="form-group">
                    <label>比较操作</label>
                    <select class="form-control" id="onCompareOp">
                      <option value="1">大于 (>)</option>
                      <option value="2">小于 (<)</option>
                      <option value="3">等于 (=)</option>
                      <option value="4">大于等于 (>=)</option>
                      <option value="5">小于等于 (<=)</option>
                      <option value="6">不等于 (!=)</option>
                    </select>
                  </div>
                  <div class="form-group">
                    <label>阈值</label>
                    <input type="number" class="form-control" id="onThreshold" step="0.1" placeholder="30.0">
                  </div>
                </div>
              </div>
              
              <div class="card" style="background:var(--gray-50);margin-top:16px;">
                <div class="card-title" style="margin-bottom:16px;">❌ 关闭条件（满足以下条件时继电器关闭）</div>
                <div class="form-grid">
                  <div class="form-group">
                    <label>传感器类型</label>
                    <select class="form-control" id="offSensorType">
                      <option value="temp">温度 (°C)</option>
                      <option value="humi">湿度 (%)</option>
                    </select>
                  </div>
                  <div class="form-group">
                    <label>比较操作</label>
                    <select class="form-control" id="offCompareOp">
                      <option value="1">大于 (>)</option>
                      <option value="2">小于 (<)</option>
                      <option value="3">等于 (=)</option>
                      <option value="4">大于等于 (>=)</option>
                      <option value="5">小于等于 (<=)</option>
                      <option value="6">不等于 (!=)</option>
                    </select>
                  </div>
                  <div class="form-group">
                    <label>阈值</label>
                    <input type="number" class="form-control" id="offThreshold" step="0.1" placeholder="25.0">
                  </div>
                </div>
              </div>
            </div>
            <div class="btn-group" style="margin-top:20px;">
              <button class="btn btn-primary" onclick="saveCondition()">💾 保存配置</button>
              <button class="btn btn-outline" onclick="loadCondition()">🔄 刷新</button>
            </div>
            <div class="info-box" style="margin-top:16px;">
              <strong>当前状态:</strong> <span id="conditionStatus">--</span>
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
            <div class="info-box">
              配置多个时间段来自动控制继电器的开关。每个时间段可以设置开始时间、结束时间和目标状态。
            </div>
            <div id="timerSlotsContent" style="display:none;">
              <div id="timerSlotsContainer"></div>
              <div class="btn-group" style="margin-top:20px;">
                <button class="btn btn-success" onclick="addTimeSlot()">➕ 添加时间段</button>
              </div>
            </div>
            <div class="btn-group" style="margin-top:20px;">
              <button class="btn btn-primary" onclick="saveTimer()">💾 保存配置</button>
              <button class="btn btn-outline" onclick="loadTimer()">🔄 刷新</button>
              <button class="btn btn-danger" onclick="clearAllTimeSlots()">🗑️ 清空全部</button>
            </div>
            <div class="info-box" style="margin-top:16px;">
              <strong>当前时间:</strong> <span id="currentTimeDisplay">--</span> | <strong>继电器状态:</strong> <span id="timerRelayState">--</span>
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
          <div class="power-mode-grid" id="powerModeGrid">
            <div class="power-mode-card active" id="powerMode0" onclick="selectPowerMode(0)">
              <div class="icon">🏃</div>
              <div class="name">活跃模式</div>
              <div class="desc">全功能运行</div>
            </div>
            <div class="power-mode-card" id="powerMode1" onclick="selectPowerMode(1)">
              <div class="icon">👂</div>
              <div class="name">监听模式</div>
              <div class="desc">低功耗监听</div>
            </div>
            <div class="power-mode-card" id="powerMode2" onclick="selectPowerMode(2)">
              <div class="icon">💤</div>
              <div class="name">轻度睡眠</div>
              <div class="desc">快速唤醒</div>
            </div>
            <div class="power-mode-card" id="powerMode3" onclick="selectPowerMode(3)">
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
    let timeSlots = [];
    
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
        case 'network': loadNetworkConfig(); loadMqttProtoMode(); break;
        case 'control': loadCondition(); loadTimer(); break;
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
      
      if (tab === 'timer') {
        loadTimer();
        updateCurrentTime();
      }
    }
    
    function updateCurrentTime() {
      const now = new Date();
      const timeStr = `${String(now.getHours()).padStart(2,'0')}:${String(now.getMinutes()).padStart(2,'0')}:${String(now.getSeconds()).padStart(2,'0')}`;
      document.getElementById('currentTimeDisplay').textContent = timeStr;
    }
    
    setInterval(updateCurrentTime, 1000);
    
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
        
        if (data.config) {
          document.getElementById('dashAcqInterval').textContent = (data.config.acqInterval || 1000) + ' ms';
          document.getElementById('dashRptInterval').textContent = (data.config.rptInterval || 5000) + ' ms';
          
          const txModes = ['MQTT优先', 'LoRa优先', '双模并发', '仅MQTT', '仅LoRa'];
          document.getElementById('dashTxMode').textContent = txModes[data.config.txMode] || '--';
          document.getElementById('dashTxMode').className = data.config.txMode === 0 ? 'stat-value success' : 'stat-value';
          
          document.getElementById('dashLoraStatus').textContent = data.config.loraEnabled ? '✅ 已启用' : '❌ 已禁用';
          document.getElementById('dashLoraStatus').className = data.config.loraEnabled ? 'stat-value success' : 'stat-value danger';

          if (data.lora) {
            const loraPresets = ['📻 标准模式', '🚀 低功耗模式', '💨 快速模式', '📡 远距离模式'];
            document.getElementById('dashLoraPreset').textContent = loraPresets[data.lora.preset] || '📻 标准模式';
          } else {
            document.getElementById('dashLoraPreset').textContent = '📻 标准模式';
          }

          const powerModes = ['🏃 活跃模式', '👂 监听模式', '💤 轻度睡眠', '🌙 深度睡眠'];
          document.getElementById('dashPowerMode').textContent = powerModes[data.config.powerMode] || '--';
          document.getElementById('dashCpuFreq').textContent = (data.config.cpuFreq || 80) + ' MHz';
          
          document.getElementById('dashWifiSsid').textContent = data.config.wifiSsid || '--';
          document.getElementById('dashLocalIp').textContent = data.ip || '--';
          document.getElementById('dashMqttServer').textContent = data.config.mqttServer || '--';
          document.getElementById('dashMqttPort').textContent = data.config.mqttPort || 1883;
        }
        
        if (data.timer_enabled && data.timer_slots && data.timer_slots.length > 0) {
          document.getElementById('timerTaskCard').style.display = 'block';
          renderTimerTasks(data.timer_slots, data.current_time);
        } else {
          document.getElementById('timerTaskCard').style.display = 'none';
        }
        
        if (data.condition_enabled) {
          document.getElementById('conditionDetailCard').style.display = 'block';
          renderConditionDetails(data);
        } else {
          document.getElementById('conditionDetailCard').style.display = 'none';
        }
      } catch (e) {
        console.error('Status refresh error:', e);
      }
    }
    
    function renderTimerTasks(slots, currentTime) {
      const container = document.getElementById('timerTaskList');
      if (!slots || slots.length === 0) {
        container.innerHTML = '<div class="info-box">暂无定时任务配置</div>';
        return;
      }
      
      const now = currentTime ? new Date(currentTime) : new Date();
      const currentMinutes = now.getHours() * 60 + now.getMinutes();
      
      let nextSlot = null;
      let nextSlotIndex = -1;
      let foundCurrent = false;
      
      for (let i = 0; i < slots.length; i++) {
        const slot = slots[i];
        if (!slot.enabled) continue;
        
        const startMin = slot.startHour * 60 + slot.startMinute;
        const endMin = slot.endHour * 60 + slot.endMinute;
        
        let isActive = false;
        if (startMin <= endMin) {
          isActive = currentMinutes >= startMin && currentMinutes <= endMin;
        } else {
          isActive = currentMinutes >= startMin || currentMinutes <= endMin;
        }
        
        if (isActive) {
          foundCurrent = true;
        }
        
        if (!foundCurrent && !nextSlot) {
          nextSlot = slot;
          nextSlotIndex = i;
        }
      }
      
      if (!nextSlot && slots.length > 0) {
        nextSlot = slots[0];
        nextSlotIndex = 0;
      }
      
      let html = '<div class="table-container"><table><thead><tr><th>序号</th><th>时间段</th><th>动作</th><th>状态</th></tr></thead><tbody>';
      
      slots.forEach((slot, idx) => {
        const startMin = slot.startHour * 60 + slot.startMinute;
        const endMin = slot.endHour * 60 + slot.endMinute;
        
        let isActive = false;
        if (startMin <= endMin) {
          isActive = currentMinutes >= startMin && currentMinutes <= endMin;
        } else {
          isActive = currentMinutes >= startMin || currentMinutes <= endMin;
        }
        
        const timeStr = `${String(slot.startHour).padStart(2,'0')}:${String(slot.startMinute).padStart(2,'0')} - ${String(slot.endHour).padStart(2,'0')}:${String(slot.endMinute).padStart(2,'0')}`;
        const action = slot.state ? '🔴 开启' : '⚫ 关闭';
        const status = isActive ? '<span class="status-badge online">● 执行中</span>' : (idx === nextSlotIndex ? '<span class="status-badge warning">○ 下一个</span>' : '<span class="status-badge offline">○ 待执行</span>');
        
        html += `<tr style="${isActive ? 'background:#D1FAE5;' : ''}">
          <td>${idx + 1}</td>
          <td>${timeStr}</td>
          <td>${action}</td>
          <td>${status}</td>
        </tr>`;
      });
      
      html += '</tbody></table></div>';
      
      if (nextSlot) {
        const nextTime = `${String(nextSlot.startHour).padStart(2,'0')}:${String(nextSlot.startMinute).padStart(2,'0')}`;
        const nextAction = nextSlot.state ? '开启' : '关闭';
        document.getElementById('nextTaskBadge').textContent = `下一任务: ${nextTime} ${nextAction}`;
      }
      
      container.innerHTML = html;
    }
    
    function renderConditionDetails(data) {
      const container = document.getElementById('conditionDetailList');
      let html = '';
      
      // 检查是否是滞回模式
      const useHysteresis = data.use_hysteresis || data.useHysteresis;
      
      if (data.on_condition) {
        const cond = data.on_condition;
        if (cond.conditions && cond.conditions[0]) {
          const c = cond.conditions[0];
          const sensor = c.sensor === 'humi' ? '湿度' : '温度';
          const compares = ['', '>', '<', '=', '>=', '<=', '!='];
          const compareText = compares[c.compare] || '>';
          
          if (useHysteresis) {
            // 滞回模式：显示高阈值开启
            const highThreshold = c.high_threshold || c.threshold;
            html += `<div class="form-group" style="background:#D1FAE5;padding:12px;border-radius:8px;">
              <div class="stat-label" style="color:#065F46;">✅ 开启条件 (滞回模式)</div>
              <div style="font-size:18px;font-weight:600;color:#065F46;">${sensor} > ${highThreshold}°C 时开启</div>
            </div>`;
          } else {
            // 普通模式：显示单阈值
            html += `<div class="form-group" style="background:#D1FAE5;padding:12px;border-radius:8px;">
              <div class="stat-label" style="color:#065F46;">✅ 开启条件</div>
              <div style="font-size:18px;font-weight:600;color:#065F46;">${sensor} ${compareText} ${c.threshold}</div>
            </div>`;
          }
        }
      }
      
      if (data.off_condition) {
        const cond = data.off_condition;
        if (cond.conditions && cond.conditions[0]) {
          const c = cond.conditions[0];
          const sensor = c.sensor === 'humi' ? '湿度' : '温度';
          const compares = ['', '>', '<', '=', '>=', '<=', '!='];
          const compareText = compares[c.compare] || '<';
          
          if (useHysteresis) {
            // 滞回模式：显示低阈值关闭
            const lowThreshold = c.low_threshold || c.threshold;
            html += `<div class="form-group" style="background:#FEE2E2;padding:12px;border-radius:8px;">
              <div class="stat-label" style="color:#991B1B;">❌ 关闭条件 (滞回模式)</div>
              <div style="font-size:18px;font-weight:600;color:#991B1B;">${sensor} < ${lowThreshold}°C 时关闭</div>
            </div>`;
          } else {
            // 普通模式：显示单阈值
            html += `<div class="form-group" style="background:#FEE2E2;padding:12px;border-radius:8px;">
              <div class="stat-label" style="color:#991B1B;">❌ 关闭条件</div>
              <div style="font-size:18px;font-weight:600;color:#991B1B;">${sensor} ${compareText} ${c.threshold}</div>
            </div>`;
          }
        }
      }
      
      container.innerHTML = html;
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
    
    let controlModeIndex = 0;
    const controlModes = ['manual', 'timer', 'condition'];
    const controlModeNames = ['手动模式', '定时控制', '条件控制'];
    const controlModeIcons = ['🖱️', '⏰', '🎯'];
    
    function toggleManualMode() {
      controlModeIndex = (controlModeIndex + 1) % controlModes.length;
      const newMode = controlModes[controlModeIndex];
      
      const modeConfig = {
        mode: newMode,
        timerEnabled: newMode === 'timer',
        conditionEnabled: newMode === 'condition'
      };
      
      fetch(`${API_BASE}/api/manual_mode`, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify(modeConfig)
      })
        .then(r => r.json())
        .then((data) => {
          if (data.status === 'ok') {
            showAlert(`🔄 已切换到${controlModeIcons[controlModeIndex]}${controlModeNames[controlModeIndex]}`, 'success');
            setTimeout(refreshStatus, 500);
          } else {
            showAlert('❌ 切换失败: ' + (data.message || '未知错误'), 'error');
            controlModeIndex = (controlModeIndex - 1 + controlModes.length) % controlModes.length;
          }
        })
        .catch(e => {
          showAlert('切换失败: ' + e, 'error');
          controlModeIndex = (controlModeIndex - 1 + controlModes.length) % controlModes.length;
        });
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
    
    let currentEditingPin = null;
    let currentPinConfigs = {};
    
    async function loadPinConfig() {
      try {
        const resp = await fetch(`${API_BASE}/api/pin/config`);
        const data = await resp.json();
        
        currentPinConfigs = {};
        
        const tbody = document.getElementById('currentPinTableBody');
        tbody.innerHTML = '';
        
        if (!data || data.length === 0) {
          tbody.innerHTML = '<tr><td colspan="5" style="text-align:center;color:#6B7280;">暂无引脚配置</td></tr>';
          return;
        }
        
        data.forEach(pin => {
          if (pin && pin.pin !== undefined) {
            currentPinConfigs[pin.pin] = pin;
            
            const funcText = getFunctionText(pin.function);
            const funcName = pin.functionName || getFunctionNameByType(pin.function);
            const modeText = getModeText(pin.mode);
            const statusText = pin.isInitialized ? '✅ 已初始化' : '⏳ 未初始化';
            const statusClass = pin.isInitialized ? 'success' : 'warning';
            
            const row = document.createElement('tr');
            row.innerHTML = `
              <td><strong>GPIO${pin.pin}</strong></td>
              <td>${funcText}</td>
              <td style="font-size:12px;color:#666;">${funcName}</td>
              <td>${modeText}</td>
              <td><span class="status-badge ${statusClass}">${statusText}</span></td>
            `;
            tbody.appendChild(row);
          }
        });
      } catch (e) {
        console.error('Load pin config error:', e);
        showAlert('加载引脚配置失败', 'error');
      }
    }
    
    function getFunctionNameByType(func) {
      const names = {
        0: '未分配',
        1: '继电器',
        2: 'SHT31 SDA',
        3: 'SHT31 SCL',
        4: 'LoRa SCK',
        5: 'LoRa MISO',
        6: 'LoRa MOSI',
        7: 'LoRa NSS',
        8: 'LoRa RESET',
        9: 'LoRa DIO0',
        10: '用户定义'
      };
      return names[func] || '未知';
    }
    
    function getFunctionText(func) {
      const funcs = {
        0: '未分配',
        1: '🔌 继电器',
        2: '🌡️ 传感器SDA',
        3: '💧 传感器SCL',
        4: '📡 LoRa SCK',
        5: '📡 LoRa MISO',
        6: '📡 LoRa MOSI',
        7: '📡 LoRa NSS',
        8: '📡 LoRa RESET',
        9: '📡 LoRa DIO0',
        10: '🔧 用户定义'
      };
      return funcs[func] || '未知';
    }
    
    function getModeText(mode) {
      const modes = {
        0: '📥 输入',
        1: '📤 输出',
        2: '📥 上拉输入',
        3: '📥 下拉输入',
        4: '⚡ PWM',
        5: '📊 ADC',
        6: '🌐 I2C SDA',
        7: '⏱️ I2C SCL',
        8: '📤 SPI MOSI',
        9: '📥 SPI MISO',
        10: '⏱️ SPI SCK',
        11: '🔌 SPI CS',
        12: '📤 UART TX',
        13: '📥 UART RX'
      };
      return modes[mode] || '未知';
    }
    
    function showPinConfigModal(pinNum = null) {
      currentEditingPin = pinNum;
      
      document.getElementById('configPinNumber').value = pinNum || '';
      document.getElementById('configPinFunction').value = '';
      document.getElementById('configPinMode').value = '1';
      document.getElementById('configInitialValue').value = '0';
      document.getElementById('pinWarning').style.display = 'none';
      document.getElementById('pinConflictWarning').style.display = 'none';
      
      if (pinNum && currentPinConfigs[pinNum]) {
        const config = currentPinConfigs[pinNum];
        document.getElementById('configPinFunction').value = getFunctionKey(config.function);
        document.getElementById('configPinMode').value = config.mode;
        document.getElementById('configInitialValue').value = config.initialValue || 0;
        onPinSelectChange();
      }
      
      document.getElementById('pinConfigModal').style.display = 'flex';
    }
    
    function closePinConfigModal() {
      document.getElementById('pinConfigModal').style.display = 'none';
      currentEditingPin = null;
    }
    
    function getFunctionKey(func) {
      const keys = {
        0: '',
        1: 'relay',
        2: 'sensor_sda',
        3: 'sensor_scl',
        4: 'lora_sck',
        5: 'lora_miso',
        6: 'lora_mosi',
        7: 'lora_nss',
        8: 'lora_reset',
        9: 'lora_dio0',
        10: 'user_defined'
      };
      return keys[func] || '';
    }
    
    function getFunctionValue(key) {
      const values = {
        '': 0,
        'relay': 1,
        'sensor_sda': 2,
        'sensor_scl': 3,
        'lora_sck': 4,
        'lora_miso': 5,
        'lora_mosi': 6,
        'lora_nss': 7,
        'lora_reset': 8,
        'lora_dio0': 9,
        'user_defined': 10
      };
      return values[key] || 0;
    }
    
    function onPinSelectChange() {
      const pinNum = parseInt(document.getElementById('configPinNumber').value);
      const funcKey = document.getElementById('configPinFunction').value;
      
      const warningDiv = document.getElementById('pinWarning');
      const conflictDiv = document.getElementById('pinConflictWarning');
      warningDiv.style.display = 'none';
      conflictDiv.style.display = 'none';
      
      if (isNaN(pinNum)) return;
      
      const warnings = [];
      const conflicts = [];
      
      if (pinNum >= 6 && pinNum <= 11) {
        warnings.push('⚠️ GPIO' + pinNum + ' 是SPI Flash引脚，修改可能导致系统无法启动！');
      }
      if (pinNum === 0) {
        warnings.push('⚠️ GPIO0 是启动引脚，高电平启动，低电平进入下载模式');
      }
      if (pinNum === 19 || pinNum === 20) {
        warnings.push('⚠️ GPIO' + pinNum + ' 是USB引脚，请勿占用');
      }
      
      if (currentPinConfigs[pinNum] && currentPinConfigs[pinNum].function !== 0) {
        const existingFunc = getFunctionText(currentPinConfigs[pinNum].function);
        conflicts.push('该引脚已被配置为: ' + existingFunc);
      }
      
      if (warnings.length > 0) {
        warningDiv.innerHTML = warnings.join('<br>');
        warningDiv.style.display = 'block';
      }
      
      if (conflicts.length > 0) {
        document.getElementById('conflictDetails').textContent = conflicts.join('<br>');
        conflictDiv.style.display = 'block';
      }
      
      autoSelectModeForFunction(pinNum, funcKey);
    }
    
    function autoSelectModeForFunction(pinNum, funcKey) {
      const modeSelect = document.getElementById('configPinMode');
      const initialValueGroup = document.getElementById('initialValueGroup');
      
      if (funcKey === 'relay') {
        modeSelect.value = '1';
        initialValueGroup.style.display = 'block';
      } else if (funcKey === 'sensor_sda') {
        modeSelect.value = '6';
        initialValueGroup.style.display = 'none';
      } else if (funcKey === 'sensor_scl') {
        modeSelect.value = '7';
        initialValueGroup.style.display = 'none';
      } else if (funcKey.startsWith('lora_')) {
        if (funcKey === 'lora_dio0') {
          modeSelect.value = '2';
        } else {
          modeSelect.value = '1';
        }
        initialValueGroup.style.display = 'none';
      }
    }
    
    function editPinConfig(pinNum) {
      showPinConfigModal(pinNum);
    }
    
    async function saveSinglePinConfig() {
      const pinNum = document.getElementById('configPinNumber').value;
      const funcKey = document.getElementById('configPinFunction').value;
      const mode = parseInt(document.getElementById('configPinMode').value);
      const initialValue = parseInt(document.getElementById('configInitialValue').value);
      
      if (!pinNum) {
        showAlert('请选择GPIO引脚', 'error');
        return;
      }
      
      if (!funcKey) {
        showAlert('请选择功能类型', 'error');
        return;
      }
      
      const func = getFunctionValue(funcKey);
      
      const config = {
        pin: parseInt(pinNum),
        mode: mode,
        function: func,
        initialValue: initialValue,
        functionName: getFunctionText(func)
      };
      
      try {
        const resp = await fetch(`${API_BASE}/api/pin/config`, {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify(config)
        });
        
        const result = await resp.json();
        
        if (result.status === 'ok' || resp.ok) {
          showAlert('✅ 引脚配置已保存', 'success');
          closePinConfigModal();
          loadPinConfig();
        } else {
          showAlert('❌ 保存失败: ' + (result.message || '未知错误'), 'error');
        }
      } catch (e) {
        showAlert('❌ 保存失败: ' + e.message, 'error');
      }
    }
    
    async function deletePinConfig(pinNum) {
      if (!confirm('确定要删除GPIO' + pinNum + '的配置吗？')) {
        return;
      }
      
      try {
        const resp = await fetch(`${API_BASE}/api/pin/config/${pinNum}`, {
          method: 'DELETE'
        });
        
        if (resp.ok) {
          showAlert('✅ 配置已删除', 'success');
          loadPinConfig();
        } else {
          showAlert('❌ 删除失败', 'error');
        }
      } catch (e) {
        showAlert('❌ 删除失败: ' + e.message, 'error');
      }
    }
    
    async function resetPinConfig() {
      if (!confirm('⚠️ 确定要恢复默认引脚配置吗？\n当前所有引脚配置将被清除！')) {
        return;
      }
      
      if (!confirm('再次确认：所有引脚配置将被重置为默认值！')) {
        return;
      }
      
      try {
        const resp = await fetch(`${API_BASE}/api/pin/config/reset`, {
          method: 'POST'
        });
        
        if (resp.ok) {
          showAlert('✅ 已恢复默认配置', 'success');
          loadPinConfig();
        } else {
          showAlert('❌ 重置失败', 'error');
        }
      } catch (e) {
        showAlert('❌ 重置失败: ' + e.message, 'error');
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
    
    // WiFi扫描功能
    let isScanning = false;
    
    /**
     * @brief 扫描WiFi网络
     */
    function scanWiFi() {
      if (isScanning) return;
      
      const modal = document.getElementById('wifiModal');
      const statusDiv = document.getElementById('wifiScanStatus');
      const listContainer = document.getElementById('wifiListContainer');
      const scanBtn = document.getElementById('wifiScanBtn');
      
      if (modal.style.display !== 'block') {
        modal.style.display = 'block';
      }
      
      isScanning = true;
      if (scanBtn) scanBtn.disabled = true;
      statusDiv.style.display = 'block';
      statusDiv.innerHTML = '<div style="font-size:24px;margin-bottom:10px;">🔍</div><div>正在扫描WiFi网络，请稍候...</div>';
      listContainer.style.display = 'none';
      
      console.log('📡 开始请求WiFi扫描...');
      
      fetch(`${API_BASE}/api/wifi/scan`)
        .then(response => {
          console.log('📡 收到响应:', response);
          return response.json();
        })
        .then(data => {
          console.log('📡 解析后的JSON数据:', data);
          statusDiv.style.display = 'none';
          listContainer.style.display = 'block';
          
          // 获取networks数组
          const networks = data && data.networks ? data.networks : data;
          
          if (!networks || networks.length === 0) {
            listContainer.innerHTML = '<div style="text-align:center;padding:20px;color:#666;">未找到任何WiFi网络</div>';
            return;
          }
          
          console.log('📡 找到', networks.length, '个WiFi网络');
          
          // 按信号强度排序
          networks.sort((a, b) => b.rssi - a.rssi);
          
          let html = '<div style="max-height:300px;overflow-y:auto;">';
          networks.forEach((wifi, index) => {
            const signalStrength = getSignalStrength(wifi.rssi);
            const icon = signalStrength.icon;
            const color = signalStrength.color;
            
            // 安全地处理SSID中的特殊字符
            const safeSsid = wifi.ssid.replace(/'/g, "\\'").replace(/"/g, '&quot;');
            
            html += `
              <div class="wifi-item" style="display:flex;align-items:center;padding:12px;cursor:pointer;border-radius:8px;margin:8px 0;border:1px solid #ddd;transition:all 0.2s;"
                   onclick="selectWifi('${safeSsid}')"
                   onmouseover="this.style.backgroundColor='#f0f7ff';this.style.borderColor='#4CAF50';"
                   onmouseout="this.style.backgroundColor='white';this.style.borderColor='#ddd';">
                <span style="font-size:24px;margin-right:12px;color:${color};">${icon}</span>
                <div style="flex:1;">
                  <div style="font-weight:600;font-size:16px;">${wifi.ssid}</div>
                  <div style="font-size:12px;color:#666;">信道: ${wifi.channel} | 信号: ${wifi.rssi} dBm</div>
                </div>
              </div>
            `;
          });
          html += '</div>';
          listContainer.innerHTML = html;
        })
        .catch(error => {
          console.error('❌ WiFi扫描失败:', error);
          statusDiv.style.display = 'block';
          listContainer.style.display = 'none';
          statusDiv.innerHTML = `<div style="color:#f44336;">❌ 扫描失败: ${error.message || '请重试'}</div>`;
        })
        .finally(() => {
          isScanning = false;
          if (scanBtn) scanBtn.disabled = false;
        });
    }
    
    function getSignalStrength(rssi) {
      if (rssi >= -50) return { icon: '📶', color: '#4CAF50' };
      if (rssi >= -60) return { icon: '📶', color: '#8BC34A' };
      if (rssi >= -70) return { icon: '📶', color: '#FFEB3B' };
      return { icon: '📶', color: '#f44336' };
    }
    
    function selectWifi(ssid) {
      document.getElementById('wifiSsid').value = ssid;
      closeWifiModal();
      showAlert(`✅ 已选择WiFi: ${ssid}`, 'success');
    }
    
    function closeWifiModal() {
      document.getElementById('wifiModal').style.display = 'none';
    }
    
    let currentMqttProtoMode = 'json';
    
    function setMqttProtoMode(mode) {
      currentMqttProtoMode = mode;
      document.getElementById('protoCardJson').classList.toggle('active', mode === 'json');
      document.getElementById('protoCardProtobuf').classList.toggle('active', mode === 'protobuf');
      document.getElementById('mqttProtoModeStatus').textContent = mode === 'json' ? 'JSON' : 'Protobuf';
      document.getElementById('mqttProtoModeStatus').className = mode === 'json' ? 'status-badge info' : 'status-badge success';
    }
    
    function switchMqttProtoMode(mode = null) {
      const targetMode = mode || (currentMqttProtoMode === 'json' ? 'protobuf' : 'json');
      
      fetch(`${API_BASE}/api/mqtt/proto_mode`, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify({mode: targetMode})
      })
        .then(r => r.json())
        .then(data => {
          if (data.status === 'ok') {
            setMqttProtoMode(targetMode);
            showAlert(`✅ 已切换到${targetMode === 'json' ? 'JSON' : 'Protobuf'}模式`, 'success');
          } else {
            showAlert('❌ 切换失败', 'error');
          }
        })
        .catch(e => showAlert('切换失败: ' + e, 'error'));
    }
    
    async function loadMqttProtoMode() {
      try {
        const resp = await fetch(`${API_BASE}/api/mqtt/proto_mode`);
        const data = await resp.json();
        
        if (data.mode) {
          setMqttProtoMode(data.mode);
        }
      } catch (e) {
        console.error('Load MQTT proto mode error:', e);
      }
    }
    
    /**
     * @brief 保存WiFi配置（SSID和密码）
     */
    function saveWifiConfig() {
      const config = {
        wifiSsid: document.getElementById('wifiSsid').value,
        wifiPassword: document.getElementById('wifiPass').value
      };
      
      fetch(`${API_BASE}/api/network/config`, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify(config)
      })
        .then(r => r.json())
        .then(() => showAlert('✅ WiFi配置已保存', 'success'))
        .catch(e => showAlert('保存失败: ' + e, 'error'));
    }
    
    /**
     * @brief 保存MQTT配置（服务器、端口、用户名、密码、Topic）
     */
    function saveMqttConfig() {
      const config = {
        mqttServer: document.getElementById('mqttServer').value,
        mqttPort: parseInt(document.getElementById('mqttPort').value),
        mqttUsername: document.getElementById('mqttUser').value,
        mqttPassword: document.getElementById('mqttPass').value,
        mqttTopic: document.getElementById('mqttTopic').value
      };
      
      fetch(`${API_BASE}/api/network/config`, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify(config)
      })
        .then(r => r.json())
        .then(() => showAlert('✅ MQTT配置已保存', 'success'))
        .catch(e => showAlert('保存失败: ' + e, 'error'));
    }
    
    /**
     * @brief 保存MQTT启用状态
     */
    function saveMqttEnabled() {
      const config = {
        mqttEnabled: document.getElementById('mqttEnabled').checked
      };
      
      fetch(`${API_BASE}/api/network/config`, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify(config)
      })
        .then(r => r.json())
        .then(() => showAlert('✅ MQTT状态已保存', 'success'))
        .catch(e => showAlert('保存失败: ' + e, 'error'));
    }
    
    /**
     * @brief 保存传输模式
     */
    function saveTxMode() {
      const config = {
        transmissionMode: parseInt(document.getElementById('txMode').value)
      };
      
      fetch(`${API_BASE}/api/network/config`, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify(config)
      })
        .then(r => r.json())
        .then(() => showAlert('✅ 传输模式已保存', 'success'))
        .catch(e => showAlert('保存失败: ' + e, 'error'));
    }
    
    /**
     * @brief 保存LoRa启用状态
     */
    function saveLoraEnabled() {
      const config = {
        loraEnabled: document.getElementById('loraEnabled').checked
      };
      
      fetch(`${API_BASE}/api/network/config`, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify(config)
      })
        .then(r => r.json())
        .then(() => showAlert('✅ LoRa状态已保存', 'success'))
        .catch(e => showAlert('保存失败: ' + e, 'error'));
    }
    
    /**
     * @brief 保存所有网络配置
     */
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

      for (let i = 0; i < 4; i++) {
        const card = document.getElementById('powerMode' + i);
        if (card) {
          card.classList.toggle('active', i === index);
        }
      }

      updatePowerModeDetails(index);
    }

    function updatePowerModeDetails(index) {
      const sleepDuration = document.getElementById('sleepDuration');
      const wakeInterval = document.getElementById('wakeInterval');
      const cpuFreq = document.getElementById('cpuFreq');

      switch(index) {
        case 0:
          sleepDuration.value = 0;
          wakeInterval.value = 0;
          cpuFreq.value = 160;
          break;
        case 1:
          sleepDuration.value = 10000000;
          wakeInterval.value = 60000000;
          cpuFreq.value = 80;
          break;
        case 2:
          sleepDuration.value = 60000000;
          wakeInterval.value = 300000000;
          cpuFreq.value = 40;
          break;
        case 3:
          sleepDuration.value = 3600000000;
          wakeInterval.value = 3600000000;
          cpuFreq.value = 40;
          break;
      }
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
          const cond = data.on_condition.conditions[0];
          document.getElementById('onSensorType').value = cond.sensor === 'humi' ? 'humi' : 'temp';
          document.getElementById('onCompareOp').value = cond.compare || 1;
          document.getElementById('onThreshold').value = cond.threshold || 30;
        }
        
        if (data.off_condition && data.off_condition.conditions && data.off_condition.conditions[0]) {
          const cond = data.off_condition.conditions[0];
          document.getElementById('offSensorType').value = cond.sensor === 'humi' ? 'humi' : 'temp';
          document.getElementById('offCompareOp').value = cond.compare || 2;
          document.getElementById('offThreshold').value = cond.threshold || 25;
        } else {
          document.getElementById('offSensorType').value = 'temp';
          document.getElementById('offCompareOp').value = '2';
          document.getElementById('offThreshold').value = '25';
        }
        
        toggleCondition();
        
        let statusText = data.enabled ? '✅ 已启用' : '❌ 已禁用';
        if (data.on_condition && data.on_condition.enabled) {
          statusText += ' | 开启条件: ' + getConditionText(data.on_condition);
        }
        if (data.off_condition && data.off_condition.enabled) {
          statusText += ' | 关闭条件: ' + getConditionText(data.off_condition);
        }
        document.getElementById('conditionStatus').textContent = statusText;
      } catch (e) {
        console.error('Load condition error:', e);
        showAlert('加载条件配置失败', 'error');
      }
    }
    
    function getConditionText(condition) {
      if (!condition || !condition.conditions || condition.conditions.length === 0) {
        return '未配置';
      }
      const cond = condition.conditions[0];
      const sensorText = cond.sensor === 'humi' ? '湿度' : '温度';
      const compareText = ['', '>', '<', '=', '>=', '<=', '!='][cond.compare] || '>';
      return `${sensorText}${compareText}${cond.threshold}`;
    }
    
    function toggleCondition() {
      const enabled = document.getElementById('conditionEnabled').checked;
      document.getElementById('conditionContent').style.display = enabled ? 'block' : 'none';
    }
    
    function saveCondition() {
      const onSensor = document.getElementById('onSensorType').value;
      const onCompare = parseInt(document.getElementById('onCompareOp').value);
      const onThreshold = parseFloat(document.getElementById('onThreshold').value);
      
      const offSensor = document.getElementById('offSensorType').value;
      const offCompare = parseInt(document.getElementById('offCompareOp').value);
      const offThreshold = parseFloat(document.getElementById('offThreshold').value);
      
      const config = {
        condition: {
          enabled: document.getElementById('conditionEnabled').checked,
          on_condition: {
            enabled: true,
            logic: 'and',
            conditions: [{
              enabled: true,
              sensor: onSensor,
              compare: onCompare,
              threshold: onThreshold
            }]
          },
          off_condition: {
            enabled: true,
            logic: 'or',
            conditions: [{
              enabled: true,
              sensor: offSensor,
              compare: offCompare,
              threshold: offThreshold
            }]
          }
        }
      };
      
      fetch(`${API_BASE}/api/condition`, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify(config)
      })
        .then(r => r.json())
        .then(() => {
          showAlert('✅ 条件控制已保存', 'success');
          setTimeout(loadCondition, 500);
        })
        .catch(e => showAlert('保存失败: ' + e, 'error'));
    }
    
    function toggleTimer() {
      const enabled = document.getElementById('timerEnabled').checked;
      document.getElementById('timerSlotsContent').style.display = enabled ? 'block' : 'none';
    }
    
    async function loadTimer() {
      try {
        const resp = await fetch(`${API_BASE}/api/timer`);
        const data = await resp.json();
        
        document.getElementById('timerEnabled').checked = data.timer && data.timer.enabled === true;
        
        timeSlots = [];
        if (data.timer && data.timer.slots) {
          data.timer.slots.forEach(slot => {
            timeSlots.push({
              index: slot.index,
              enabled: slot.enabled,
              startHour: slot.startHour || 0,
              startMinute: slot.startMinute || 0,
              endHour: slot.endHour || 0,
              endMinute: slot.endMinute || 0,
              state: slot.state || false
            });
          });
        }
        
        renderTimeSlots();
        toggleTimer();
        
        const now = new Date();
        document.getElementById('currentTimeDisplay').textContent = 
          `${String(now.getHours()).padStart(2,'0')}:${String(now.getMinutes()).padStart(2,'0')}:${String(now.getSeconds()).padStart(2,'0')}`;
        
        if (data.timer && data.timer.enabled) {
          document.getElementById('timerRelayState').textContent = '⏰ 定时控制中';
        } else {
          document.getElementById('timerRelayState').textContent = '--';
        }
      } catch (e) {
        console.error('Load timer error:', e);
        showAlert('加载定时配置失败', 'error');
      }
    }
    
    function renderTimeSlots() {
      const container = document.getElementById('timerSlotsContainer');
      container.innerHTML = '';
      
      if (timeSlots.length === 0) {
        container.innerHTML = '<div class="info-box">暂无时间段，请点击"添加时间段"创建</div>';
        return;
      }
      
      timeSlots.forEach((slot, index) => {
        const div = document.createElement('div');
        div.className = 'card';
        div.style.marginTop = '16px';
        div.innerHTML = `
          <div class="card-header">
            <span class="card-title">⏰ 时间段 ${index + 1}</span>
            <label class="toggle-switch">
              <input type="checkbox" id="slotEnabled${index}" ${slot.enabled ? 'checked' : ''} onchange="updateSlot(${index}, 'enabled', this.checked)">
              <span class="toggle-slider"></span>
            </label>
          </div>
          <div class="form-grid">
            <div class="form-group">
              <label>开始时间</label>
              <div style="display:flex;gap:8px;">
                <select class="form-control" id="slotStartH${index}" style="width:80px;" onchange="updateSlot(${index}, 'startHour', parseInt(this.value))">
                  ${generateHourOptions(slot.startHour)}
                </select>
                <span style="line-height:38px;">:</span>
                <select class="form-control" id="slotStartM${index}" style="width:80px;" onchange="updateSlot(${index}, 'startMinute', parseInt(this.value))">
                  ${generateMinuteOptions(slot.startMinute)}
                </select>
              </div>
            </div>
            <div class="form-group">
              <label>结束时间</label>
              <div style="display:flex;gap:8px;">
                <select class="form-control" id="slotEndH${index}" style="width:80px;" onchange="updateSlot(${index}, 'endHour', parseInt(this.value))">
                  ${generateHourOptions(slot.endHour)}
                </select>
                <span style="line-height:38px;">:</span>
                <select class="form-control" id="slotEndM${index}" style="width:80px;" onchange="updateSlot(${index}, 'endMinute', parseInt(this.value))">
                  ${generateMinuteOptions(slot.endMinute)}
                </select>
              </div>
            </div>
            <div class="form-group">
              <label>目标状态</label>
              <select class="form-control" id="slotState${index}" onchange="updateSlot(${index}, 'state', this.value === 'true')">
                <option value="true" ${slot.state ? 'selected' : ''}>🔴 开启继电器</option>
                <option value="false" ${!slot.state ? 'selected' : ''}>⚫ 关闭继电器</option>
              </select>
            </div>
          </div>
          <div class="btn-group" style="margin-top:12px;">
            <button class="btn btn-outline" onclick="copyTimeSlot(${index})">📋 复制</button>
            <button class="btn btn-danger" onclick="deleteTimeSlot(${index})">🗑️ 删除</button>
          </div>
        `;
        container.appendChild(div);
      });
    }
    
    function generateHourOptions(selected) {
      let html = '';
      for (let i = 0; i < 24; i++) {
        html += `<option value="${i}" ${i === selected ? 'selected' : ''}>${String(i).padStart(2, '0')}</option>`;
      }
      return html;
    }
    
    function generateMinuteOptions(selected) {
      let html = '';
      for (let i = 0; i < 60; i++) {
        html += `<option value="${i}" ${i === selected ? 'selected' : ''}>${String(i).padStart(2, '0')}</option>`;
      }
      return html;
    }
    
    function updateSlot(index, field, value) {
      if (index < timeSlots.length) {
        timeSlots[index][field] = value;
      }
    }
    
    function addTimeSlot() {
      if (timeSlots.length >= 8) {
        showAlert('⚠️ 最多只能添加8个时间段', 'warning');
        return;
      }
      
      const now = new Date();
      const newSlot = {
        index: timeSlots.length,
        enabled: true,
        startHour: now.getHours(),
        startMinute: 0,
        endHour: (now.getHours() + 1) % 24,
        endMinute: 0,
        state: true
      };
      
      timeSlots.push(newSlot);
      renderTimeSlots();
    }
    
    function copyTimeSlot(index) {
      if (timeSlots.length >= 8) {
        showAlert('⚠️ 最多只能添加8个时间段', 'warning');
        return;
      }
      
      const original = timeSlots[index];
      const copy = JSON.parse(JSON.stringify(original));
      copy.index = timeSlots.length;
      timeSlots.push(copy);
      renderTimeSlots();
      showAlert('✅ 已复制时间段', 'success');
    }
    
    function deleteTimeSlot(index) {
      if (confirm('确定要删除这个时间段吗？')) {
        timeSlots.splice(index, 1);
        timeSlots.forEach((slot, i) => slot.index = i);
        renderTimeSlots();
        
        // 同时保存到设备
        const slots = timeSlots.map((slot, idx) => ({
          index: idx,
          enabled: slot.enabled,
          start_time: `${String(slot.startHour).padStart(2,'0')}:${String(slot.startMinute).padStart(2,'0')}`,
          end_time: `${String(slot.endHour).padStart(2,'0')}:${String(slot.endMinute).padStart(2,'0')}`,
          state: slot.state
        }));
        
        const config = {
          timer: {
            enabled: document.getElementById('timerEnabled').checked,
            slots: slots
          }
        };
        
        fetch(`${API_BASE}/api/timer`, {
          method: 'POST',
          headers: {'Content-Type':'application/json'},
          body: JSON.stringify(config)
        })
          .then(() => showAlert('✅ 时间段已删除', 'success'))
          .catch(e => showAlert('删除失败: ' + e, 'error'));
      }
    }
    
    function clearAllTimeSlots() {
      if (confirm('⚠️ 确定要清空所有时间段吗？此操作不可恢复！')) {
        if (confirm('再次确认：所有时间段将被删除！')) {
          timeSlots = [];
          renderTimeSlots();
          fetch(`${API_BASE}/api/timer`, {
            method: 'POST',
            headers: {'Content-Type':'application/json'},
            body: JSON.stringify({timer: {enabled: false, slots: []}})
          })
            .then(() => showAlert('✅ 已清空所有时间段', 'success'))
            .catch(e => showAlert('清空失败: ' + e, 'error'));
        }
      }
    }
    
    function saveTimer() {
      const slots = timeSlots.map((slot, index) => ({
        index: index,
        enabled: slot.enabled,
        start_time: `${String(slot.startHour).padStart(2,'0')}:${String(slot.startMinute).padStart(2,'0')}`,
        end_time: `${String(slot.endHour).padStart(2,'0')}:${String(slot.endMinute).padStart(2,'0')}`,
        state: slot.state
      }));
      
      const config = {
        timer: {
          enabled: document.getElementById('timerEnabled').checked,
          slots: slots
        }
      };
      
      fetch(`${API_BASE}/api/timer`, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify(config)
      })
        .then(r => r.json())
        .then(() => {
          showAlert('✅ 定时控制已保存', 'success');
          setTimeout(loadTimer, 500);
        })
        .catch(e => showAlert('保存失败: ' + e, 'error'));
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
  
  server.on("/api/mqtt/proto_mode", HTTP_GET, [this]() { handleGetMqttProtoMode(); });
  server.on("/api/mqtt/proto_mode", HTTP_POST, [this]() { handleSetMqttProtoMode(); });
  server.on("/api/wifi/scan", HTTP_GET, [this]() { handleScanWiFi(); });
  
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
  server.send_P(200, PSTR("text/html; charset=utf-8"), HTML_PAGE, strlen_P(HTML_PAGE));
}

void WebUIManager::handleGetStatus() {
  StaticJsonDocument<1300> doc;
  doc["wifi"] = (WiFi.status() == WL_CONNECTED);
  doc["mqtt"] = mqttManager.isConnected();
  doc["relay"] = relayControl.getState();
  doc["lora"] = true;
  doc["mem"] = (100.0 * (ESP.getHeapSize() - ESP.getFreeHeap())) / ESP.getHeapSize();
  doc["rssi"] = WiFi.RSSI();
  doc["manual_mode"] = manualRelayMode;
  doc["ip"] = WiFi.localIP().toString();
  
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
  
  JsonObject configObj = doc.createNestedObject("config");
  configObj["acqInterval"] = DEFAULT_ACQUISITION_INTERVAL;
  configObj["rptInterval"] = mqttReportInterval;
  configObj["txMode"] = 0;
  configObj["loraEnabled"] = true;
  configObj["powerMode"] = 0;
  configObj["cpuFreq"] = 80;
  configObj["wifiSsid"] = WiFi.isConnected() ? WiFi.SSID() : "";
  configObj["mqttServer"] = MQTT_SERVER;
  configObj["mqttPort"] = MQTT_PORT;
  
  doc["timer_enabled"] = conditionControl.isTimerEnabled();
  
  JsonArray timerSlots = doc.createNestedArray("timer_slots");
  for (int i = 0; i < 8; i++) {
    TimeSlot slot = conditionControl.getTimeSlot(i);
    if (slot.enabled || (slot.startHour > 0 || slot.startMinute > 0)) {
      JsonObject slotObj = timerSlots.createNestedObject();
      slotObj["index"] = i;
      slotObj["enabled"] = slot.enabled;
      slotObj["startHour"] = slot.startHour;
      slotObj["startMinute"] = slot.startMinute;
      slotObj["endHour"] = slot.endHour;
      slotObj["endMinute"] = slot.endMinute;
      slotObj["state"] = slot.state;
    }
  }
  
  String conditionJson = conditionControl.toJSON();
  StaticJsonDocument<600> condDoc;
  deserializeJson(condDoc, conditionJson);
  doc["condition_enabled"] = conditionControl.isEnabled();
  doc["on_condition"] = condDoc["on_condition"];
  doc["off_condition"] = condDoc["off_condition"];
  
  struct tm timeinfo;
  if (ntpClient.getTime(&timeinfo, 1000)) {
    JsonObject timeObj = doc.createNestedObject("current_time");
    timeObj["year"] = timeinfo.tm_year + 1900;
    timeObj["month"] = timeinfo.tm_mon + 1;
    timeObj["day"] = timeinfo.tm_mday;
    timeObj["hour"] = timeinfo.tm_hour;
    timeObj["minute"] = timeinfo.tm_min;
    timeObj["second"] = timeinfo.tm_sec;
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
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON解析失败\"}");
    return;
  }

  if (!conditionControl.isEnabled() && !conditionControl.isTimerEnabled() && !manualRelayMode) {
    StaticJsonDocument<200> respDoc;
    respDoc["status"] = "error";
    respDoc["code"] = 403;
    respDoc["message"] = "控制已禁用，请在WebUI或MQTT上切换到手动模式/定时控制/条件控制模式";
    respDoc["current_mode"] = "disabled";
    respDoc["relay_state"] = relayControl.getState();

    String respJson;
    serializeJson(respDoc, respJson);
    server.send(403, "application/json", respJson);
    return;
  }

  if (doc.containsKey("relay")) {
    bool state = doc["relay"].as<bool>();
    if (state) {
      relayControl.turnOn();
    } else {
      relayControl.turnOff();
    }
    manualRelayMode = true;

    StaticJsonDocument<100> respDoc;
    respDoc["status"] = "ok";
    respDoc["relay_state"] = relayControl.getState();
    respDoc["mode"] = "manual";

    String respJson;
    serializeJson(respDoc, respJson);
    server.send(200, "application/json", respJson);
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"缺少relay参数\"}");
  }
}

void WebUIManager::handleManualMode() {
  StaticJsonDocument<300> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON解析失败\"}");
    return;
  }

  const char* currentMode = "manual";

  if (doc.containsKey("mode")) {
    currentMode = doc["mode"].as<const char*>();

    if (strcmp(currentMode, "manual") == 0) {
      manualRelayMode = true;
      conditionControl.setEnabled(false);
      conditionControl.setTimerEnabled(false);
      Serial.println("[WebUI] 切换到手动模式");
    } else if (strcmp(currentMode, "timer") == 0) {
      manualRelayMode = false;
      conditionControl.setEnabled(false);
      conditionControl.setTimerEnabled(true);
      Serial.println("[WebUI] 切换到定时控制模式");
    } else if (strcmp(currentMode, "condition") == 0) {
      manualRelayMode = false;
      conditionControl.setEnabled(true);
      conditionControl.setTimerEnabled(false);
      Serial.println("[WebUI] 切换到条件控制模式");
    } else if (strcmp(currentMode, "disabled") == 0) {
      manualRelayMode = false;
      conditionControl.setEnabled(false);
      conditionControl.setTimerEnabled(false);
      Serial.println("[WebUI] 控制已禁用");
    }
  }

  if (doc.containsKey("timerEnabled")) {
    conditionControl.setTimerEnabled(doc["timerEnabled"].as<bool>());
  }
  if (doc.containsKey("conditionEnabled")) {
    conditionControl.setEnabled(doc["conditionEnabled"].as<bool>());
  }
  if (doc.containsKey("disabled")) {
    bool disabled = doc["disabled"].as<bool>();
    if (disabled) {
      conditionControl.setEnabled(false);
      conditionControl.setTimerEnabled(false);
      Serial.println("[WebUI] 控制已禁用");
    }
  }

  StaticJsonDocument<200> respDoc;
  respDoc["status"] = "ok";
  respDoc["mode"] = currentMode;
  respDoc["manual_mode"] = manualRelayMode;
  respDoc["timer_enabled"] = conditionControl.isTimerEnabled();
  respDoc["condition_enabled"] = conditionControl.isEnabled();

  String respJson;
  serializeJson(respDoc, respJson);
  server.send(200, "application/json", respJson);
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
  StaticJsonDocument<600> doc;
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
        uint8_t condIndex = 0;
        for (JsonObject cond : conditions) {
          if (condIndex < 4) {
            uint8_t sensorType = (cond.containsKey("sensor") && cond["sensor"].as<String>() == "humi") ? 1 : 0;
            uint8_t compareOp = cond.containsKey("compare") ? cond["compare"].as<uint8_t>() : 1;
            float threshold = cond.containsKey("threshold") ? cond["threshold"].as<float>() : 25.0;
            bool cEnabled = cond.containsKey("enabled") ? cond["enabled"].as<bool>() : true;
            conditionControl.setCondition(true, condIndex, cEnabled, sensorType, compareOp, threshold);
            condIndex++;
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
      if (offObj.containsKey("conditions")) {
        JsonArray conditions = offObj["conditions"].as<JsonArray>();
        uint8_t condIndex = 0;
        for (JsonObject cond : conditions) {
          if (condIndex < 4) {
            uint8_t sensorType = (cond.containsKey("sensor") && cond["sensor"].as<String>() == "humi") ? 1 : 0;
            uint8_t compareOp = cond.containsKey("compare") ? cond["compare"].as<uint8_t>() : 2;
            float threshold = cond.containsKey("threshold") ? cond["threshold"].as<float>() : 25.0;
            bool cEnabled = cond.containsKey("enabled") ? cond["enabled"].as<bool>() : true;
            conditionControl.setCondition(false, condIndex, cEnabled, sensorType, compareOp, threshold);
            condIndex++;
          }
        }
      }
    }
  }
  
  server.send(200, "application/json", conditionControl.toJSON());
}

void WebUIManager::handleGetTimer() {
  StaticJsonDocument<2048> wrapperDoc;
  String timerJson = conditionControl.getTimerJSON();
  
  StaticJsonDocument<2048> timerDoc;
  deserializeJson(timerDoc, timerJson);
  wrapperDoc["timer"] = timerDoc.as<JsonObject>();
  
  String json;
  serializeJson(wrapperDoc, json);
  server.send(200, "application/json", json);
}

void WebUIManager::handleSetTimer() {
  StaticJsonDocument<2048> doc;
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
      // 【关键修复】先清空所有时间段，避免删除的时间段残留
      conditionControl.clearTimeSlots();
      
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

void WebUIManager::handleGetMqttProtoMode() {
  StaticJsonDocument<100> doc;
  doc["mode"] = mqttProtocol.isJsonMode() ? "json" : "protobuf";
  doc["status"] = "ok";

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void WebUIManager::handleSetMqttProtoMode() {
  StaticJsonDocument<200> doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"JSON parse failed\"}");
    return;
  }
  
  if (doc.containsKey("mode")) {
    const char* mode = doc["mode"].as<const char*>();
    Serial.printf("[WebUI] MQTT协议模式切换: %s\n", mode);
    
    if (strcmp(mode, "json") == 0) {
      mqttProtocol.setMode(TransmissionMode::JSON_MODE);
      Serial.println("[WebUI] 已切换到JSON模式");
    } else if (strcmp(mode, "protobuf") == 0) {
      mqttProtocol.setMode(TransmissionMode::PROTOBUF_MODE);
      Serial.println("[WebUI] 已切换到Protobuf模式");
    }
  }
  
  doc.clear();
  doc["status"] = "ok";
  doc["mode"] = mqttProtocol.isJsonMode() ? "json" : "protobuf";
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

/**
 * @brief 处理WiFi扫描请求
 * @note 在AP模式下会临时切换到AP+STA模式进行扫描
 */
void WebUIManager::handleScanWiFi() {
  wifi_mode_t currentMode = WiFi.getMode();
  
  Serial.printf("🔍 当前WiFi模式: %d\n", currentMode);
  Serial.println("🔍 开始 WiFi 扫描...");
  
  // 如果是纯AP模式，切换到AP+STA模式
  if (currentMode == WIFI_AP) {
    Serial.println("🔄 切换到AP+STA模式以进行扫描");
    WiFi.mode(WIFI_AP_STA);
    delay(100); // 等待模式切换完成
  }
  
  // 使用同步扫描模式，确保获取完整结果
  // 参数: async=false, show_hidden=false, passive=false, max_ms=10000
  int n = WiFi.scanNetworks(false, false, false, 10000);
  
  StaticJsonDocument<2048> doc;
  JsonArray wifiList = doc.createNestedArray("networks");
  
  if (n < 0) {
    Serial.printf("❌ WiFi 扫描失败，错误码: %d\n", n);
  } else if (n == 0) {
    Serial.println("⚠️ 未找到任何 WiFi 网络");
  } else {
    Serial.printf("✅ WiFi 扫描完成，找到 %d 个网络\n", n);
    
    // 限制返回的网络数量，避免 JSON 过大
    int count = 0;
    for (int i = 0; i < n; i++) {
      String ssid = WiFi.SSID(i);
      // 跳过空SSID
      if (ssid.length() == 0) continue;
      
      if (count >= 20) break; // 最多返回20个
      
      JsonObject wifiObj = wifiList.createNestedObject();
      wifiObj["ssid"] = ssid;
      wifiObj["rssi"] = WiFi.RSSI(i);
      wifiObj["channel"] = WiFi.channel(i);
      
      Serial.printf("  %d. SSID: %s, RSSI: %d dBm, 信道: %d\n", 
                    count + 1, ssid.c_str(), WiFi.RSSI(i), WiFi.channel(i));
      count++;
    }
  }
  
  // 释放扫描结果占用的内存
  WiFi.scanDelete();
  
  // 恢复原来的模式
  if (currentMode == WIFI_AP) {
    Serial.println("🔄 恢复到AP模式");
    WiFi.mode(WIFI_AP);
    delay(50);
  }
  
  String json;
  serializeJson(doc, json);
  Serial.printf("📤 返回扫描结果: %s\n", json.c_str());
  server.send(200, "application/json", json);
}
