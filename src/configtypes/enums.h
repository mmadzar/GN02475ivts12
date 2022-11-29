#ifndef ENUMS_H_
#define ENUMS_H_

enum class switcht
{
    on_off,
    click_once,
    pwm_signal
};

enum class sensort
{
    adc,
    temperature,
    voltage
};

enum class devicet
{
    // switches
    msft_vacuum,
    msft_servo,
    msft_coolant_pwm,
    msft_inv_start,

    // sensors
    adc_ntc,
    adc_vacuum,
    adc_motor1,
    adc_motor2,
    adc_heater1,
    adc_heater2,
};

#endif