#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <Arduino.h>
#include "config.h"

class SystemMonitor {
private:
  unsigned long lastStatsTime;
  unsigned long lastMemoryReportTime;

public:
  SystemMonitor();
  void printStats();
  void reportMemory();
  void checkMemory();
  bool isMemoryLow();
  
  unsigned long getFreeHeap() const { return ESP.getFreeHeap(); }
  unsigned long getTotalHeap() const { return ESP.getHeapSize(); }
  unsigned long getUsedHeap() const { return ESP.getHeapSize() - ESP.getFreeHeap(); }
};

extern SystemMonitor systemMonitor;

#endif // SYSTEM_MONITOR_H
