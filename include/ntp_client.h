#ifndef NTP_CLIENT_H
#define NTP_CLIENT_H

#include <Arduino.h>
#include <WiFiUdp.h>

class NTPClient {
private:
  WiFiUDP udp;
  const char* ntpServer;
  int ntpPort;
  long gmtOffset;
  bool initialized;
  
public:
  NTPClient();
  
  void begin(const char* server = "ntp.aliyun.com", int port = 123, long offset = 8 * 3600);
  
  bool getTime(struct tm* timeinfo, unsigned long timeout = 1000);
  
  bool getTime(int& year, int& month, int& day, int& hour, int& minute, int& second, unsigned long timeout = 1000);
};

extern NTPClient ntpClient;

#endif // NTP_CLIENT_H