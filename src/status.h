#ifndef STATUS_H_
#define STATUS_H_

#include <stdint.h>
#include <Arduino.h>

struct Status
{
  int rpm = 600; // minimum value to keep DSC working
  int coolant_temp = 90;
  int ikeFuelLevel = -1;

  long loops = 0;
  long missedSend = 0;
  long bootedMillis = 0;
  long currentMillis = 0;
  uint32_t freeMem = 0;
  uint32_t minFreeMem = 0;
  const char *ipAddress = "255.255.255.255";
  const char *gatewayAddress = "255.255.255.255";
  byte canBytes[2048];
  int canBytesSize = 0;
  String SSID = "";
  int8_t rssi = 0;

  long receivedCount = 0;
  long ivtsCurrent = 0;  // mA
  long ivtsVoltage1 = 0; // mV
  long ivtsTemp = 0;
  String ivtsCommand = "";
  int sensors[SensorCount];
  int switches[SwitchCount];
  int collectors[CollectorCount];
};

extern Status status;

#endif /* STATUS_H_ */
