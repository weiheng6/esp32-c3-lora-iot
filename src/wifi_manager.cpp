#include "wifi_manager.h"
#include "config.h"
#include "web_server.h"

WiFiManager wifiManager;
Preferences preferences;

WiFiManager::WiFiManager() : configured(false), connecting(false), lastAttempt(0), ntpSynced(false), lastNtpSyncTime(0) {
  memset(ssid, 0, sizeof(ssid));
  memset(password, 0, sizeof(password));
}

void WiFiManager::begin() {
  preferences.begin("wifi_config", true);
  String savedSsid = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");
  configured = preferences.getBool("configured", false);
  preferences.end();
  
  if (savedSsid.length() > 0) {
    savedSsid.toCharArray(ssid, sizeof(ssid));
    savedPassword.toCharArray(password, sizeof(password));
    Serial.printf("✅ 已加载 WiFi 配置：SSID=%s\n", ssid);
  }
  
  connect();
}

void WiFiManager::connect() {
  if (WiFi.status() != WL_CONNECTED) {
    if (!connecting) {
      if (strlen(ssid) == 0 || !configured) {
        Serial.println("🔄 WiFi 未配置，启动 AP 模式...");
        startAPMode();
        return;
      }
      
      Serial.print("🔄 正在连接 WiFi：");
      Serial.print(ssid);
      Serial.print("...");
      WiFi.begin(ssid, password);
      connecting = true;
      lastAttempt = millis();
    } else {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("✅");
        Serial.print("📶 WiFi 已连接，IP 地址：");
        IPAddress localIP = WiFi.localIP();
        Serial.println(localIP);
        connecting = false;
        
        // 保存本地 IP 到 Preferences，供 Web 服务器显示
        preferences.begin("device_info", false);
        preferences.putString("local_ip", localIP.toString());
        preferences.end();
        
        if (WiFi.getMode() & WIFI_AP) {
          WiFi.mode(WIFI_STA);
        }
        
        // WiFi连接成功后同步NTP时间
        syncNtpTime();
      } else if (millis() - lastAttempt > CONNECTION_TIMEOUT) {
        Serial.println("❌ 连接超时");
        connecting = false;
        Serial.println("🔄 WiFi 连接失败，启动 AP 模式...");
        startAPMode();
      } else if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL) {
        Serial.println("❌ 连接失败");
        connecting = false;
        Serial.println("🔄 启动 AP 模式...");
        startAPMode();
      }
    }
  }
}

void WiFiManager::startAPMode() {
  Serial.println("🔄 启动 AP 模式...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print("📶 AP 已启动，IP 地址：");
  Serial.println(WiFi.softAPIP());
  Serial.print("📱 请连接 WiFi：");
  Serial.println(AP_SSID);
  Serial.print("🔑 密码：");
  Serial.println(AP_PASSWORD);
  Serial.println("🌐 然后在浏览器中访问：http://192.168.4.1");
  
  webServerManager.begin();
}

void WiFiManager::setCredentials(const char* newSsid, const char* newPassword) {
  snprintf(ssid, sizeof(ssid), "%s", newSsid);
  snprintf(password, sizeof(password), "%s", newPassword);
  configured = true;
  saveConfig();
}

void WiFiManager::resetConfig() {
  memset(ssid, 0, sizeof(ssid));
  memset(password, 0, sizeof(password));
  configured = false;
  
  preferences.begin("wifi_config", false);
  preferences.remove("ssid");
  preferences.remove("password");
  preferences.putBool("configured", false);
  preferences.end();
  
  Serial.println("✅ WiFi 配置已重置");
}

void WiFiManager::loadConfig() {
  preferences.begin("wifi_config", true);
  String savedSsid = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");
  configured = preferences.getBool("configured", false);
  preferences.end();
  
  if (savedSsid.length() > 0) {
    savedSsid.toCharArray(ssid, sizeof(ssid));
    savedPassword.toCharArray(password, sizeof(password));
  }
}

void WiFiManager::saveConfig() {
  preferences.begin("wifi_config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putBool("configured", configured);
  preferences.end();
  Serial.println("✅ WiFi 配置已保存");
}

void WiFiManager::syncNtpTime() {
  if (ntpSynced && (millis() - lastNtpSyncTime < 3600000)) {
    return;
  }
  
  Serial.println("🌐 正在同步NTP时间...");
  
  configTime(8 * 3600, 0, "ntp.aliyun.com", "time.pool.aliyun.com", "cn.pool.ntp.org");
  
  struct tm timeinfo;
  int retryCount = 0;
  const int maxRetry = 10;
  
  while (retryCount < maxRetry) {
    if (getLocalTime(&timeinfo)) {
      ntpSynced = true;
      lastNtpSyncTime = millis();
      
      char timeStr[64];
      strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
      Serial.printf("✅ NTP时间同步成功: %s (UTC+8)\n", timeStr);
      Serial.printf("   服务器: ntp.aliyun.com / time.pool.aliyun.com\n");
      return;
    }
    delay(500);
    retryCount++;
    Serial.printf("   NTP同步尝试 %d/%d...\n", retryCount, maxRetry);
  }
  
  Serial.println("⚠️ NTP时间同步失败，将使用系统默认时间");
  ntpSynced = false;
}
