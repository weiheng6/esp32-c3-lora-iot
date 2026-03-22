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
        <h1>ESP32 条件控制器</h1>
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
  
  String html = R"=====(<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>配置成功</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f5f5f5; }
        .container { max-width: 500px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 8px rgba(0,0,0,0.1); text-align: center; }
        h1 { color: #4CAF50; }
        p { color: #666; }
    </style>
</head>
<body>
    <div class="container">
        <h1>✅ WiFi 配置成功！</h1>
        <p>设备正在连接到：<strong>)=====";
  html += ssid;
  html += R"=====(</strong></p>
        <p>请关闭此页面，设备将自动重启并连接。</p>
    </div>
</body>
</html>)=====";
  
  sendResponse(200, "text/html", html);
  delay(2000);
  ESP.restart();
}

void WebServerManager::handleScanWiFi() {
  wifi_mode_t currentMode = WiFi.getMode();
  
  if (currentMode == WIFI_AP) {
    WiFi.mode(WIFI_AP_STA);
  }
  
  // 使用异步扫描模式，不会阻塞主线程
  Serial.println("🔍 开始异步 WiFi 扫描...");
  int n = WiFi.scanNetworks(true, false);  // async=true, show_hidden=false
  
  // 等待扫描完成（最多等待 200ms）
  int maxWait = 20;  // 20 * 10ms = 200ms
  while (WiFi.scanComplete() < 0 && maxWait-- > 0) {
    delay(10);  // 短延迟给扫描任务执行时间
  }
  
  n = WiFi.scanComplete();
  
  String json = "[";
  
  // 限制返回的网络数量，避免 JSON 过大
  for (int i = 0; i < n && i < 20; i++) {
    if (i > 0) {
      json += ",";
    }
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + WiFi.RSSI(i) + "}";
  }
  
  json += "]";
  
  Serial.printf("✅ WiFi 扫描完成，找到 %d 个网络\n", n);
  
  if (currentMode == WIFI_AP) {
    WiFi.mode(WIFI_AP);
  }
  
  sendResponse(200, "application/json", json);
}

void WebServerManager::sendResponse(int code, const char* contentType, const String& content) {
  server.send(code, contentType, content);
}
