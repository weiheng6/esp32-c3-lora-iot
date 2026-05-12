#include "wifi_manager.h"
#include "config.h"
#include "web_server.h"
#include "web_ui.h"
#include "config_api.h"

extern WebUIManager webUIManager;
extern ConfigAPI configAPI;

WiFiManager wifiManager;
Preferences preferences;

WiFiManager::WiFiManager() : configured(false), connecting(false), lastAttempt(0), ntpSynced(false), lastNtpSyncTime(0), configUpdated(false), apModeActive(false) {
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
        ensureAPMode(true);
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
        
        // WiFi连接成功，关闭AP模式节省电量
        ensureAPMode(false);
        
        // 如果是通过AP配置的新WiFi，输出提示
        if (configUpdated) {
          Serial.println("🎉 WiFi配置成功！设备已从AP模式切换到Station模式");
          configUpdated = false;
        }
        
        // WiFi连接成功后同步NTP时间
        syncNtpTime();
      } else if (millis() - lastAttempt > CONNECTION_TIMEOUT) {
        Serial.println("❌ 连接超时");
        connecting = false;
        Serial.println("🔄 WiFi 连接失败，启动 AP 模式...");
        ensureAPMode(true);
      } else if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL) {
        Serial.println("❌ 连接失败");
        connecting = false;
        Serial.println("🔄 启动 AP 模式...");
        ensureAPMode(true);
      }
    }
  } else {
    // WiFi已连接，确保AP模式关闭
    ensureAPMode(false);
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
  
  // 使用统一的 WebUI，无论是否已配置 WiFi
  // 这样 AP 模式也能使用完整的仪表盘、配网、定时控制等功能
  webUIManager.begin();
}

void WiFiManager::setCredentials(const char* newSsid, const char* newPassword) {
  snprintf(ssid, sizeof(ssid), "%s", newSsid);
  snprintf(password, sizeof(password), "%s", newPassword);
  configured = true;
  configUpdated = true;  // 设置配置更新标志，触发立即连接
  saveConfig();
  
  // 如果当前在AP模式，立即尝试连接新配置的WiFi
  if (WiFi.getMode() & WIFI_AP && strlen(ssid) > 0) {
    Serial.printf("📡 检测到WiFi配置更新，立即尝试连接: %s\n", ssid);
    // 断开当前连接（如果有的话）
    if (WiFi.status() == WL_CONNECTED) {
      WiFi.disconnect(true);
      delay(50);
    }
    WiFi.begin(ssid, password);
    connecting = true;
    lastAttempt = millis();
  }
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
  
  // 同时清除 ConfigAPI 中的 WiFi 配置
  Serial.println("🔄 同时清除 ConfigAPI 中的 WiFi 配置...");
  configAPI.setNetworkConfig("{\"wifiSsid\":\"\",\"wifiPassword\":\"\"}");
  
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

/**
 * @brief 智能控制AP模式开关
 * @param enable true=开启AP模式, false=关闭AP模式
 */
void WiFiManager::ensureAPMode(bool enable) {
  bool currentAP = WiFi.getMode() & WIFI_AP;
  
  if (enable && !currentAP) {
    // 需要开启AP模式
    Serial.println("🔄 开启AP模式...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    apModeActive = true;
    Serial.print("📶 AP 已启动，IP 地址：");
    Serial.println(WiFi.softAPIP());
    Serial.print("📱 请连接 WiFi：");
    Serial.println(AP_SSID);
    Serial.print("🔑 密码：");
    Serial.println(AP_PASSWORD);
    Serial.println("🌐 然后在浏览器中访问：http://192.168.4.1");
    
    // 启动WebUI
    webUIManager.begin();
  } else if (!enable && currentAP) {
    // 需要关闭AP模式
    Serial.println("🔋 关闭AP模式以节省电量...");
    WiFi.mode(WIFI_STA);
    apModeActive = false;
    Serial.println("✅ AP模式已关闭");
  }
}
