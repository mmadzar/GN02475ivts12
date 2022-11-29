#include "Sensor.h"

Sensor::Sensor(SensorConfig &sensorconfig)
{
    config = &sensorconfig;
}

void Sensor::setup()
{
    pinMode(config->pin, INPUT);
}

int &Sensor::onChange(THandlerFunction_Change fn)
{
    _change_callback = fn;
    return lastValueRead;
}

void Sensor::handle()
{
    skipStepsCounter++;
    if (skipSteps < skipStepsCounter)
    {
        skipStepsCounter = 0;
        tempValues += analogRead(config->pin);
        samplesCollected++;
        double result = 0;
        if (samplesCollected == sumValuesCount)
        {
            bool valueChanged = false;
            // ignore adc value errors
            if (abs((tempValues / sumValuesCount) - lastValueRead) > config->adc_ignore_points)
            {
                lastValueRead = tempValues / sumValuesCount;
                valueChanged = true;
                
                if (minValue > lastValueRead)
                {
                    minValue = lastValueRead;
                }

                if (maxValue < lastValueRead)
                {
                    maxValue = lastValueRead;
                }
                switch (config->sensortype)
                {
                case sensort::temperature:
                    result = calculateTemperature(lastValueRead) * 100.0;
                    break;
                case sensort::voltage:
                    result = calculateVoltage(lastValueRead) * 1000.0;
                    break;
                case sensort::adc:
                    result = lastValueRead;
                    break;
                default:
                    break;
                }
                if (valueChanged && lastOnChangeTime + millisecondsBetweenOnChanges < status.currentMillis)
                {
                    lastOnChangeTime = status.currentMillis;
                    _change_callback(config->name, config->device, result);
                }
            }
            samplesCollected = 0;
            tempValues = 0;
        }
    }
}

double Sensor::calculateTemperature(int adc_value)
{
    double Vout, Rth, temperature;

    Vout = (adc_value * config->VCC) / config->adc_resolution;
    Rth = (config->VCC * config->R2 / Vout) - config->R2;

    /*  Steinhart-Hart Thermistor Equation:
        Temperature in Kelvin = 1 / (A + B[ln(R)] + C[ln(R)]^3)
        where A = 0.001129148, B = 0.000234125 and C = 8.76741*10^-8  */
    temperature = (1 / (A + (B * log(Rth)) + (C * pow((log(Rth)), 3)))); // Temperature in kelvin
    temperature = temperature - 273.15;                                  // Temperature in degree celsius

    return temperature;
}

double Sensor::calculateVoltage(int adc_value)
{
    double voltage = ((adc_value * config->VCC) / config->adc_resolution) * (1 + (config->R1 / config->R2));
    voltage += 0.28;       // custom correction factor for this components setup
    int r = voltage * 100; // round to two decimal places
    return r / 100.0;
}