#include "Collector.h"

Collector::Collector(CollectorConfig &Collectorconfig)
{
    config = &Collectorconfig;
}

void Collector::setup()
{
}

int &Collector::onChange(THandlerFunction_Change fn)
{
    _change_callback = fn;
    return lastAverage;
}

void Collector::handle(int value)
{
    collectedSamples++;
    this->value = this->value + value;
    if (status.currentMillis - lastSend > config->sendRate)
    {
        lastSend = status.currentMillis;
        lastAverage = (int)((double)this->value / (double)collectedSamples);
        _change_callback(config->name, lastAverage);
        collectedSamples = 0;
        this->value = 0;
    }
}
