#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include <WiFi.h>

class WebServerManager {
private:
  WebServer server;
  bool started;
  
  void handleRoot();
  void handleWiFiConfig();
  void handleSaveWiFi();
  void handleScanWiFi();
  void sendResponse(int code, const char* contentType, const String& content);

public:
  WebServerManager();
  void begin();
  void handleClient();
  bool isStarted() const { return started; }
  
  // 获取服务器实例
  WebServer& getServer() { return server; }
};

extern WebServerManager webServerManager;

#endif // WEB_SERVER_H
