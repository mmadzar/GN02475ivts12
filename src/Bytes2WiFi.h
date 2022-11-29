#ifndef BYTES2WIFI_H_
#define BYTES2WIFI_H_

#define SERIAL_TCP_PORT 23 // Wifi Port UART0 - telnet
#define MAX_NMEA_CLIENTS 1
#include "appconfig.h"
#include "status.h"
#include <WiFi.h>
#include <WiFiClient.h>

class Bytes2WiFi
{
public:
    Bytes2WiFi();
    void setup();
    void send();
    void read();
    void handle();
    void addBuffer(byte b);
    void addBuffer(const char *buffer, size_t size);
private:
    WiFiUDP wifiUDPServer;
    uint32_t lastBroadcast;
    byte wifiCommand[128];
    int wifiCmdPos = 0;
    uint32_t currentMicros;
};

#endif /* BYTES2WIFI_H_ */
