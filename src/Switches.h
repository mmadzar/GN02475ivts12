#ifndef SWITCHES_H_
#define SWITCHES_H_

#include <Arduino.h>
#include "configtypes/configtypes.h"
#include "base/Switch.h"
#include "appconfig.h"
#include "MqttPubSub.h"
#include "status.h"

class Switches
{
public:
  // callback event related
  typedef void (*THandlerFunction_Change)(const char *name, devicet devicetype, int value);
  int &onChange(THandlerFunction_Change fn); // This callback will be called when reading changes or timeout is exceeded

  Switches();
  void setup(class MqttPubSub &mqtt_client);
  void handle();

private:
  PinsSettings pinsSettings;
  Switch *devices[SwitchCount];
  SwitchConfig *configs[SwitchCount];
};

#endif /* SWITCHES_H_ */
