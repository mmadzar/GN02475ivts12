#include "configtypes.h"

SensorConfig::SensorConfig(devicet device, const char *key, uint8_t pin, sensort sensortype)
{
    this->device = device;
    this->name = key;
    this->pin = pin;
    this->sensortype = sensortype;
}

SensorConfig::SensorConfig(devicet device, const char *key, uint8_t pin, int R1, int R2, sensort sensortype)
{
    this->device = device;
    this->name = key;
    this->pin = pin;
    this->R1 = R1;
    this->R2 = R2;
    this->sensortype = sensortype;
}

SwitchConfig::SwitchConfig(devicet device, const char *key, uint8_t pin, uint8_t channel, switcht switchtype)
{
    this->device = device;
    this->name = key;
    this->pin = pin;
    this->switchtype = switchtype;
    this->channel = channel;
}

SwitchConfig::SwitchConfig(devicet device, const char *key, uint8_t pin, switcht switchtype)
{
    this->device = device;
    this->name = key;
    this->pin = pin;
    this->switchtype = switchtype;
}

CollectorConfig::CollectorConfig(const char *name, int sendRate)
{
    this->name = name;
    this->sendRate = sendRate;
}