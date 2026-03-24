#ifndef WEB_UI_H
#define WEB_UI_H

#include <Arduino.h>
#include <WebServer.h>

// Web UI 管理器类
class WebUIManager {
private:
  WebServer server;
  bool isStarted = false;
  
  // API 端点处理函数
  void handleRoot();
  void handleGetConfig();
  void handleSetConfig();
  void handleGetStatus();
  void handleRelayControl();
  void handleRestart();
  void handleExportConfig();
  void handleImportConfig();
  
public:
  WebUIManager(uint16_t port = 80);
  void begin();
  void handleClient();
  bool isRunning() const { return isStarted; }
};

extern WebUIManager webUIManager;

#endif
