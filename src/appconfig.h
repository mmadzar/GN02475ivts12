#ifndef APPCONFIG_H_
#define APPCONFIG_H_

#define HOST_NAME "GN02475ivts12"

#include "../../secrets.h"
#include <stdint.h>
#include <Arduino.h>
#include <driver/gpio.h>
#include "shared/configtypes/configtypes.h"


struct PinsSettings
{
  const gpio_num_t led = (gpio_num_t)2;      // status led
  const gpio_num_t can0_rx = (gpio_num_t)18; // can0 transciever rx line
  const gpio_num_t can0_tx = (gpio_num_t)19; // can0 transciever tx line
  const gpio_num_t canpwr = (gpio_num_t)21;  // can power pin

#define CollectorCount 3
  CollectorConfig collectors[CollectorCount] = {
      {"current", 500},  // mA
      {"voltage", 500}, // mV
      {"counter", 500}}; // As current counter

  int getCollectorIndex(const char *name)
  {
    for (size_t i = 0; i < CollectorCount; i++)
    {
      if (strcmp(collectors[i].name, name) == 0)
        return i;
    }
    return -1;
  }
};

struct Intervals
{
  int statusPublish = 1000; // interval at which status is published to MQTT
  int Can2Mqtt = 1000;      // send CAN messages to MQTT every n secconds. Accumulate messages until. Set this to 0 for forwarding all CAN messages to MQTT as they are received.
  int CANsend = 10;         // interval at which to send CAN Messages to car bus network (milliseconds)
};

extern PinsSettings pinsSettings;
extern Intervals intervals;

#endif /* APPCONFIG_H_ */
