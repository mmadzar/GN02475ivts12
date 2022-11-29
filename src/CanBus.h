#ifndef CANBUS_H_
#define CANBUS_H_

#include <Arduino.h>
#include "shared/base/Collector.h"
#include "shared/configtypes/configtypes.h"
#include <esp32_can.h>
#include <CAN_config.h>
#include "appconfig.h"
#include <ArduinoJson.h>
#include "status.h"
#include "shared/MqttPubSub.h"
#include "shared/Bytes2WiFi.h"

class CanBus
{
private:
  Collector *collectors[CollectorCount];
  CollectorConfig *configs[CollectorCount];
  Bytes2WiFi *b2w;
  Bytes2WiFi *b2wdebug;

  PinsSettings pinsSettings;
  void init();
  CAN_device_t CAN_cfg;    // CAN Config
  long previousMillis = 0; // will store last time a CAN Message was send
  int handle521(CAN_FRAME frame);
  int handle522(CAN_FRAME frame);
  int handle525(CAN_FRAME frame);
  int handle680(CAN_FRAME frame);

public:
  CanBus();
  void handle();
  void setup(class MqttPubSub &mqtt_client, Bytes2WiFi &wifiport, Bytes2WiFi &portDebug);
  void initializeIVTS();
  void stopIVTS();
  void stopIVTS500();
  void stopIVTS511();
  void stopIVTS600();
  void stopIVTS611();
  void stopIVTS700();
  void stopIVTS711();
  void storeIVTS();
  void startIVTS();
  void initCurrentIVTS();
  void defaultIVTS();
  void restartIVTS();
  CAN_FRAME outframe;
};

#endif /* CANBUS_H_ */
