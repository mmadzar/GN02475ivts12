#ifndef CANBUS_H_
#define CANBUS_H_

#include <Arduino.h>
#include "base/Collector.h"
#include "configtypes/configtypes.h"
#include <esp32_can.h>
#include <CAN_config.h>
#include "appconfig.h"
#include <ArduinoJson.h>
#include "status.h"
#include "MqttPubSub.h"
//#include <SimpleISA.h>

class CanBus
{
private:
  Collector *collectors[CollectorCount];
  CollectorConfig *configs[CollectorCount];

  PinsSettings pinsSettings;
  void init();
  CAN_device_t CAN_cfg;    // CAN Config
  long previousMillis = 0; // will store last time a CAN Message was send
  int handle521(CAN_FRAME frame);
  int handle522(CAN_FRAME frame);
  int handle525(CAN_FRAME frame);

public:
  CanBus();
  void handle();
  void setup(class MqttPubSub &mqtt_client);
  void sendMessageSet();
  void initializeIVTS();
  void stopIVTS();
  void storeIVTS();
  void startIVTS();
  void initCurrentIVTS();
  void defaultIVTS();
  void restartIVTS();
  CAN_FRAME outframe;
};

#endif /* CANBUS_H_ */
