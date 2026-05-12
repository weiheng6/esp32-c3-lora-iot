#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <Preferences.h>

class WiFiManager {
private:
  char ssid[50];
  char password[50];
  bool configured;
  bool connecting;
  unsigned long lastAttempt;
  static const unsigned long CONNECTION_TIMEOUT = 10000;
  bool ntpSynced;
  unsigned long lastNtpSyncTime;
  bool configUpdated;  // 标记配置是否已更新，用于触发立即连接
  bool apModeActive;   // 标记AP模式是否激活
  
  void loadConfig();
  void saveConfig();
  void syncNtpTime();
  void ensureAPMode(bool enable);  // 智能控制AP模式开关

public:
  WiFiManager();
  void begin();
  void connect();
  void startAPMode();
  bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
  bool isConfigured() const { return configured; }
  bool isNtpSynced() const { return ntpSynced; }
  bool isAPModeActive() const { return apModeActive; }
  void setCredentials(const char* newSsid, const char* newPassword);
  void resetConfig();
  const char* getSSID() const { return ssid; }
  const char* getPassword() const { return password; }
};

extern WiFiManager wifiManager;
extern Preferences preferences;

#endif // WIFI_MANAGER_H
