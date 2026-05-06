#include "web_server.h"
#include "wifi_manager.h"
#include "config.h"
#include <functional>

WebServerManager webServerManager;

WebServerManager::WebServerManager() : server(80), started(false) {}

void WebServerManager::begin() {
  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/wifi-config", HTTP_GET, [this]() { handleWiFiConfig(); });
  server.on("/save-wifi", HTTP_POST, [this]() { handleSaveWiFi(); });
  server.on("/scan-wifi", HTTP_GET, [this]() { handleScanWiFi(); });
  server.on("/api/network-info", HTTP_GET, [this]() { handleNetworkInfo(); });
  
  server.begin();
  started = true;
  Serial.println("✅ Web 服务器已启动");
}

void WebServerManager::handleClient() {
  if (started) {
    unsigned long webServerStart = millis();
    server.handleClient();
    unsigned long webServerDuration = millis() - webServerStart;
    
    // 监测 WebServer 处理耗时
    if (webServerDuration > 50) {
      Serial.printf("⚠️  WebServer handleClient 耗时过长：%lu ms\n", webServerDuration);
    }
  }
}

void WebServerManager::handleRoot() {
  String html = R"=====(<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi配置</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }
        .container { max-width: 400px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        h1 { text-align: center; color: #333; }
        form { display: flex; flex-direction: column; }
        label { margin-top: 10px; font-weight: bold; }
        input { padding: 8px; margin: 5px 0 10px 0; border: 1px solid #ddd; border-radius: 4px; }
        button { margin-top: 15px; padding: 10px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; }
        button:hover { background-color: #45a049; }
        .scan-btn { background-color: #2196F3; }
        .scan-btn:hover { background-color: #0b7dda; }
        #wifiList { margin-top: 15px; border: 1px solid #ddd; border-radius: 4px; padding: 10px; max-height: 200px; overflow-y: auto; }
        .wifi-item { padding: 8px; margin: 5px 0; background-color: #f9f9f9; cursor: pointer; border-radius: 4px; }
        .wifi-item:hover { background-color: #e8f4f8; }
        #status { margin-top: 15px; padding: 10px; text-align: center; color: #666; }
    </style>
</head>
<body>
    <div class="container">
        <h1>智能终端配网</h1>
        <h2>WiFi配置</h2>
        <form action="/save-wifi" method="post">
            <label for="ssid">WiFi名称:</label>
            <input type="text" id="ssid" name="ssid" required>
            <label for="password">WiFi密码:</label>
            <input type="password" id="password" name="password">
            <button type="submit">保存配置</button>
        </form>
        <button class="scan-btn" onclick="scanWiFi()">扫描WiFi</button>
        <div id="wifiList"></div>
        <div id="status"></div>
    </div>
    <script>
        function scanWiFi() {
            document.getElementById('status').innerHTML = '扫描中...';
            fetch('/scan-wifi')
                .then(response => response.json())
                .then(data => {
                    let list = document.getElementById('wifiList');
                    list.innerHTML = '';
                    data.forEach(wifi => {
                        let div = document.createElement('div');
                        div.className = 'wifi-item';
                        div.innerHTML = wifi.ssid + ' (' + wifi.rssi + ' dBm)';
                        div.onclick = () => document.getElementById('ssid').value = wifi.ssid;
                        list.appendChild(div);
                    });
                    document.getElementById('status').innerHTML = '找到 ' + data.length + ' 个网络';
                })
                .catch(() => document.getElementById('status').innerHTML = '扫描失败');
        }
    </script>
</body>
</html>)=====";
  sendResponse(200, "text/html", html);
}

void WebServerManager::handleWiFiConfig() {
  handleRoot();
}

void WebServerManager::handleSaveWiFi() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  
  if (ssid.length() == 0) {
    String html = "<h1>配置失败</h1><p>WiFi 名称不能为空</p><a href='/'>返回重试</a>";
    sendResponse(400, "text/html", html);
    return;
  }
  
  wifiManager.setCredentials(ssid.c_str(), password.c_str());
  
  // 立即尝试连接 WiFi，获取 IP 地址
  Serial.println("\n📡 开始连接 WiFi...");
  WiFi.begin(ssid.c_str(), password.c_str());
  
  // 等待 WiFi 连接（最多等待 15 秒）
  int connectionAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && connectionAttempts < 30) {
    delay(500);
    connectionAttempts++;
    Serial.print(".");
  }
  Serial.println();
  
  // 检查连接结果
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("✅ WiFi 连接成功！IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("⚠️  WiFi 连接超时，但将继续尝试重启");
  }
  
  String html = R"=====(<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi 配置成功</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); 
            min-height: 100vh; 
            display: flex; 
            align-items: center; 
            justify-content: center; 
            padding: 20px;
        }
        .container { 
            background: white; 
            padding: 40px; 
            border-radius: 12px; 
            box-shadow: 0 10px 40px rgba(0,0,0,0.2); 
            max-width: 600px; 
            text-align: center;
        }
        h1 { 
            color: #4CAF50; 
            font-size: 32px; 
            margin-bottom: 20px;
        }
        .success-icon {
            font-size: 60px;
            margin-bottom: 20px;
        }
        .info-box {
            background: #f0f9f0;
            border-left: 4px solid #4CAF50;
            padding: 20px;
            margin: 20px 0;
            border-radius: 6px;
            text-align: left;
        }
        .info-label {
            color: #666;
            font-size: 14px;
            font-weight: 600;
            margin-bottom: 5px;
            text-transform: uppercase;
        }
        .info-value {
            color: #333;
            font-size: 18px;
            font-weight: 700;
            font-family: 'Courier New', monospace;
            word-break: break-all;
            padding: 10px;
            background: white;
            border-radius: 4px;
            border: 1px solid #ddd;
        }
        .web-ui-link {
            display: inline-block;
            margin-top: 20px;
            padding: 15px 40px;
            background: #4CAF50;
            color: white;
            text-decoration: none;
            border-radius: 6px;
            font-weight: 600;
            transition: all 0.3s;
        }
        .web-ui-link:hover {
            background: #45a049;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(76, 175, 80, 0.4);
        }
        .instruction {
            background: #e3f2fd;
            border-left: 4px solid #2196F3;
            padding: 20px;
            margin: 20px 0;
            border-radius: 6px;
            text-align: left;
        }
        .instruction-title {
            color: #1565c0;
            font-weight: 600;
            margin-bottom: 10px;
        }
        .instruction-steps {
            color: #555;
            line-height: 1.8;
        }
        .step {
            margin: 8px 0;
            padding-left: 20px;
            position: relative;
        }
        .step:before {
            content: attr(data-step);
            position: absolute;
            left: 0;
            background: #2196F3;
            color: white;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 12px;
            font-weight: 600;
        }
        .countdown {
            color: #f44336;
            font-weight: 700;
            margin-top: 20px;
        }
        .status-connected {
            color: #4CAF50;
            font-size: 16px;
            font-weight: 600;
        }
        .status-not-connected {
            color: #f44336;
            font-size: 16px;
            font-weight: 600;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="success-icon">✅</div>
        <h1>WiFi 配置成功！</h1>
        
        <div class="info-box">
            <div class="info-label">已连接的 WiFi 网络</div>
            <div class="info-value">)=====";
  
  html += ssid;
  
  html += R"=====(</div>
        </div>
        
        <div id="networkInfo"></div>
        
        <div class="instruction">
            <div class="instruction-title">📖 后续步骤</div>
            <div class="instruction-steps">
                <div class="step" data-step="1">关闭本页面</div>
                <div class="step" data-step="2">断开热点，连接到上面显示的 WiFi 网络</div>
                <div class="step" data-step="3">在浏览器中访问显示的地址进行高级配置</div>
                <div class="step" data-step="4">配置 MQTT、采集间隔等参数</div>
            </div>
        </div>
        
        <p class="countdown">⏱️ 设备将在 <span id="countdown">30</span> 秒后重启</p>
    </div>

    <script>
        function updateNetworkInfo() {
            fetch('/api/network-info')
                .then(response => response.json())
                .then(data => {
                    let html = '';
                    
                    if (data.connected) {
                        html += '<div class="info-box">';
                        html += '<div class="info-label">设备内网 IP 地址</div>';
                        html += '<div class="info-value">' + data.local_ip + '</div>';
                        html += '</div>';
                        
                        html += '<div class="info-box">';
                        html += '<div class="info-label">Web 管理界面</div>';
                        html += '<a href="http://' + data.local_ip + '" class="web-ui-link" target="_blank">🌐 访问管理界面</a>';
                        html += '</div>';
                        
                        html += '<p class="status-connected">✅ 设备已成功连接到 WiFi 网络</p>';
                    } else {
                        html += '<p class="status-not-connected">⏳ 设备正在连接 WiFi，请稍候...</p>';
                    }
                    
                    document.getElementById('networkInfo').innerHTML = html;
                })
                .catch(error => {
                    console.error('获取网络信息失败:', error);
                    document.getElementById('networkInfo').innerHTML = '<p class="status-not-connected">⏳ 正在获取网络信息...</p>';
                });
        }
        
        // 初始化和定时更新
        updateNetworkInfo();
        setInterval(updateNetworkInfo, 1000);
        
        // 倒计时
        let countdown = 30;
        setInterval(() => {
            countdown--;
            document.getElementById('countdown').textContent = countdown;
            if (countdown <= 0) {
                document.body.innerHTML = '<div style="text-align:center;padding:50px;"><h1>设备重启中...</h1><p>请重新连接到您的 WiFi 网络，然后访问设备的 IP 地址进行管理</p></div>';
            }
        }, 1000);
    </script>
</body>
</html>)=====";
  
  // 发送响应给客户端
  sendResponse(200, "text/html", html);
  
  // 关闭 AP 热点（因为配置已完成）
  if (WiFi.getMode() & WIFI_AP) {
    Serial.println("🔌 配置完成，正在关闭 AP 热点...");
    delay(500);  // 等待客户端接收响应
    WiFi.mode(WIFI_STA);
    Serial.println("✅ AP 热点已关闭，设备将在 3 秒后重启");
    delay(3000);
  } else {
    Serial.println("✅ WiFi 配置已保存，设备将在 3 秒后重启");
    delay(3000);
  }
  
  // 重启设备
  Serial.println("🔄 设备正在重启...");
  ESP.restart();
}

void WebServerManager::handleScanWiFi() {
  wifi_mode_t currentMode = WiFi.getMode();
  
  Serial.printf("🔍 开始 WiFi 扫描 (当前模式: %d)...\n", currentMode);
  
  // 使用同步扫描模式，确保获取完整结果
  Serial.println("🔍 开始 WiFi 扫描...");
  int n = WiFi.scanNetworks(false, false);  // async=false, show_hidden=false
  
  String json = "[";
  
  if (n < 0) {
    Serial.printf("❌ WiFi 扫描失败，错误码: %d\n", n);
  } else if (n == 0) {
    Serial.println("⚠️  未找到任何 WiFi 网络");
  } else {
    Serial.printf("✅ WiFi 扫描完成，找到 %d 个网络\n", n);
    
    // 限制返回的网络数量，避免 JSON 过大
    for (int i = 0; i < n && i < 20; i++) {
      if (i > 0) {
        json += ",";
      }
      json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + WiFi.RSSI(i) + "}";
    }
  }
  
  json += "]";
  
  // 释放扫描结果占用的内存
  WiFi.scanDelete();
  
  if (currentMode == WIFI_AP) {
    WiFi.mode(WIFI_AP);
  }
  
  sendResponse(200, "application/json", json);
}

void WebServerManager::sendResponse(int code, const char* contentType, const String& content) {
  server.send(code, contentType, content);
}

void WebServerManager::handleNetworkInfo() {
  // 构建网络信息 JSON 响应
  String json = "{";
  
  // 检查 WiFi 连接状态
  bool connected = (WiFi.status() == WL_CONNECTED);
  json += "\"connected\":" + String(connected ? "true" : "false") + ",";
  
  if (connected) {
    IPAddress localIP = WiFi.localIP();
    json += "\"local_ip\":\"" + localIP.toString() + "\",";
    json += "\"ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"signal_strength\":" + String(WiFi.RSSI());
  } else {
    json += "\"local_ip\":\"未连接\",";
    json += "\"ssid\":\"未连接\",";
    json += "\"signal_strength\":0";
  }
  
  json += "}";
  
  sendResponse(200, "application/json", json);
}
