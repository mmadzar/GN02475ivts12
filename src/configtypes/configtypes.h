#ifndef CONFIGTYPES_H_
#define CONFIGTYPES_H_

#include <stdint.h>
#include <driver/gpio.h>
#include "enums.h"
#include <Arduino.h>

class SensorConfig
{

public:
    SensorConfig(devicet device_type, const char *name, uint8_t pin, sensort sensortype);
    SensorConfig(devicet device_type, const char *name, uint8_t pin, int resistor1, int resistor2, sensort sensortype);

    devicet device;
    sensort sensortype;
    const char *name;
    uint8_t pin;
    // 330/440 for 5 volts sensors
    // 10k/2.8k for 15 volts sensors
    int R1 = 10000;            // voltage only
    int R2 = 2000;             // temperature and voltage
    int timeout = 30;          // report value as changed after 30 seconds if there even is no change
    double VCC = 3.3;          // NodeMCU on board 3.3v vcc
    int adc_resolution = 4095; // 12-bit adc
    int adc_ignore_points = 0; // adc points to ignore as a change
};

class SwitchConfig
{
public:
    SwitchConfig(devicet device_type, const char *name, uint8_t pin, uint8_t channel, switcht switch_type);
    SwitchConfig(devicet device_type, const char *name, uint8_t pin, switcht switch_type);

    devicet device;
    switcht switchtype;
    const char *name;
    uint8_t pin;
    uint8_t channel; // pwm_signal type only
};

class CollectorConfig
{
public:
    CollectorConfig(const char *name, int sendRate);
    const char *name;
    int sendRate; // number of ms between sends. collects between
};

#endif