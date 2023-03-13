#ifndef APPCONFIG_H_
#define APPCONFIG_H_

#define HOST_NAME "GN02475ivts12"

#include "../../secrets.h"
#include <stdint.h>
#include <Arduino.h>
#include <driver/gpio.h>
#include "shared/configtypes/configtypes.h"

// IVTS shunt monitors
#define CURRENT "current"
#define VOLTAGE "voltage"
#define TEMPERATURE "temperature"
#define POWER "power"
#define CURRENTCOUNTER "currentCounter"
#define ENERGYCOUNTER "energyCounter"

struct Settings
{
  const gpio_num_t led = (gpio_num_t)2;      // status led
  const gpio_num_t can0_rx = (gpio_num_t)18; // rev1 16; // can0 transciever rx line
  const gpio_num_t can0_tx = (gpio_num_t)19; // rev1 17; // can0 transciever tx line
  const gpio_num_t canpwr = (gpio_num_t)21;  // can power pin

#define ListenChannelsCount 0
  const char *listenChannels[ListenChannelsCount] = {};

#define CollectorCount 6
  CollectorConfig collectors[CollectorCount] = {
      {CURRENT, 200},        // mA
      {VOLTAGE, 200},        // mV
      {TEMPERATURE, 200},    // 0.1 C degrees - Shunt temperature C degrees
      {POWER, 200},          // 1W
      {CURRENTCOUNTER, 200}, // 1As current counter
      {ENERGYCOUNTER, 200}}; // 1Wh energy counter

  int getCollectorIndex(const char *name)
  {
    for (size_t i = 0; i < CollectorCount; i++)
    {
      if (strcmp(collectors[i].name, name) == 0)
        return i;
    }
    return -1;
  }

#define SwitchCount 0
  SwitchConfig switches[SwitchCount] = {};

  int getSwitchIndex(devicet device)
  {
    for (size_t i = 0; i < SwitchCount; i++)
    {
      if (switches[i].device == device)
        return i;
    }
  }
};

struct Intervals
{
  int statusPublish = 1000; // interval at which status is published to MQTT
  int Can2Mqtt = 1000;      // send CAN messages to MQTT every n secconds. Accumulate messages until. Set this to 0 for forwarding all CAN messages to MQTT as they are received.
  int CANsend = 10;         // interval at which to send CAN Messages to car bus network (milliseconds)
  int click_onceDelay = 1000; // milliseconds
};

extern Settings settings;
extern Intervals intervals;

#endif /* APPCONFIG_H_ */
