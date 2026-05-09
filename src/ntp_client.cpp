#include "ntp_client.h"
#include <WiFi.h>

NTPClient ntpClient;

NTPClient::NTPClient() : ntpServer("ntp.aliyun.com"), ntpPort(123), gmtOffset(8 * 3600), initialized(false) {}

void NTPClient::begin(const char* server, int port, long offset) {
  ntpServer = server;
  ntpPort = port;
  gmtOffset = offset;
  initialized = true;
  Serial.println("✅ NTP客户端已初始化");
}

bool NTPClient::getTime(struct tm* timeinfo, unsigned long timeout) {
  if (!initialized || WiFi.status() != WL_CONNECTED) {
    return false;
  }

  if (!udp.begin(ntpPort)) {
    Serial.println("❌ UDP初始化失败");
    return false;
  }

  byte packetBuffer[48] = {0};
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  udp.beginPacket(ntpServer, ntpPort);
  udp.write(packetBuffer, 48);
  udp.endPacket();

  unsigned long startTime = millis();
  while (millis() - startTime < timeout) {
    int packetSize = udp.parsePacket();
    if (packetSize >= 48) {
      udp.read(packetBuffer, 48);
      
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      
      const unsigned long seventyYears = 2208988800UL;
      unsigned long epoch = secsSince1900 - seventyYears + gmtOffset;
      
      time_t rawTime = (time_t)epoch;
      *timeinfo = *localtime(&rawTime);
      
      udp.stop();
      return true;
    }
    delay(50);
  }

  udp.stop();
  return false;
}

bool NTPClient::getTime(int& year, int& month, int& day, int& hour, int& minute, int& second, unsigned long timeout) {
  struct tm timeinfo;
  if (getTime(&timeinfo, timeout)) {
    year = timeinfo.tm_year + 1900;
    month = timeinfo.tm_mon + 1;
    day = timeinfo.tm_mday;
    hour = timeinfo.tm_hour;
    minute = timeinfo.tm_min;
    second = timeinfo.tm_sec;
    return true;
  }
  return false;
}