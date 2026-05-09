#ifndef LORA_ADAPTIVE_H
#define LORA_ADAPTIVE_H

#include <Arduino.h>
#include "lora_params.h"
#include "lora_manager.h"

class LoRaAdaptiveController {
private:
  bool enabled;
  unsigned long lastAdjustTime;
  unsigned long adjustCooldown;
  
  int16_t currentRSSI;
  float currentSNR;
  float packetLossRate;
  uint8_t batteryLevel;
  uint32_t reportInterval;
  
  int rssiHistory[10];
  float snrHistory[10];
  float lossHistory[10];
  int historyIndex;
  int historyCount;
  
  TransmissionQuality currentQuality;
  
  LoRaParams targetParams;
  LoRaParams appliedParams;
  float transitionFactor;
  
  unsigned long totalPackets;
  unsigned long lostPackets;
  unsigned long successfulPackets;
  
  float calculateAverageRSSI();
  float calculateAverageSNR();
  float calculateAverageLossRate();
  
  void updateHistory(int16_t rssi, float snr, float loss);
  bool shouldAdjust();
  LoRaParams calculateOptimalParams();
  
  void applySmoothTransition();
  void incrementAdjustCounter();
  
public:
  LoRaAdaptiveController();
  
  void begin();
  
  void adjustByRSSI(int16_t rssi);
  void adjustBySNR(float snr);
  void adjustByPacketLoss(float lossRate);
  void adjustByBattery(uint8_t level);
  void adjustByFrequency(uint32_t interval);
  
  void performAdaptiveAdjustment();
  
  TransmissionQuality getCurrentQuality() const;
  const char* getQualityString() const;
  
  void enable();
  void disable();
  bool isEnabled() const { return enabled; }
  
  LoRaParams getTargetParams() const { return targetParams; }
  float getTransitionProgress() const { return transitionFactor; }
  
  float getPacketLossRate() const { return packetLossRate; }
  int16_t getAverageRSSI() const;
  float getAverageSNR() const;
  
  void recordPacketSent();
  void recordPacketReceived();
  void recordPacketLost();
  
  void resetStatistics();
  void printStatistics() const;
  void printCurrentStatus() const;
};

extern LoRaAdaptiveController loraAdaptiveController;

#endif
