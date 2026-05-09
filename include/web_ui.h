#ifndef WEB_UI_H
#define WEB_UI_H

#include <Arduino.h>
#include <WebServer.h>

class WebUIManager {
private:
  WebServer server;
  bool isStarted = false;
  
  void handleRoot();
  void handleGetConfig();
  void handleSetConfig();
  void handleGetStatus();
  void handleGetTime();
  void handleRelayControl();
  void handleManualMode();
  void handleGetCondition();
  void handleSetCondition();
  void handleGetTimer();
  void handleSetTimer();
  void handleRestart();
  void handleResetConfig();
  void handleExportConfig();
  void handleImportConfig();
  
  void handleOTAURL();
  void handleOTAUpload();
  void handleOTAStatus();

  void handleGetDeviceConfig();
  void handleSetDeviceConfig();
  void handleGetLoraParams();
  void handleSetLoraParams();
  void handleGetPowerConfig();
  void handleSetPowerConfig();
  void handleGetPinConfig();
  void handleSetPinConfig();
  void handleGetNetworkConfig();
  void handleSetNetworkConfig();
  void handleGetControlStrategy();
  void handleSetControlStrategy();
  void handleGetSystemConfig();
  void handleSetSystemConfig();
  void handleGetTransmissionMode();
  void handleSetTransmissionMode();
  
public:
  WebUIManager(uint16_t port = 80);
  void begin();
  void handleClient();
  bool isRunning() const { return isStarted; }
};

extern WebUIManager webUIManager;

#endif
