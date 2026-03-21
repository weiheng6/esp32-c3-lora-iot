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

  void loadConfig();
  void saveConfig();

public:
  WiFiManager();
  void begin();
  void connect();
  void startAPMode();
  bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
  bool isConfigured() const { return configured; }
  void setCredentials(const char* newSsid, const char* newPassword);
  void resetConfig();
  const char* getSSID() const { return ssid; }
  const char* getPassword() const { return password; }
};

extern WiFiManager wifiManager;
extern Preferences preferences;

#endif // WIFI_MANAGER_H
