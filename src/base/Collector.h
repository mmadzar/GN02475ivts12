#ifndef COLLECTOR_H_
#define COLLECTOR_H_

#include <Arduino.h>
#include "configtypes/configtypes.h"
#include "appconfig.h"
#include "status.h"

class Collector
{
public:
    // callback event related
    typedef void (*THandlerFunction_Change)(const char *name, int value);
    int &onChange(THandlerFunction_Change fn); // This callback will be called when reading changes or timeout is exceeded

    Collector(CollectorConfig &config);
    void setup();
    void handle(int value);

private:
    CollectorConfig *config;
    long lastSend = 0; // last miliseconds when message was send
    THandlerFunction_Change _change_callback;

    int value;                // accumulated value
    int collectedSamples = 0; // number of collected samples
    int lastAverage = 0;      // last calculated average
};

#endif