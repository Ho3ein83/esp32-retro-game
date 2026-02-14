#pragma once

#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/ledc.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#define INPUT           0
#define OUTPUT          1
#define INPUT_PULLUP    2
#define INPUT_PULLDOWN  3
}
#endif

class IOHelper {
private:

    // ---- ADC ---- //
    static bool gpioToAdcUnitChannel(gpio_num_t gpio, adc_unit_t &unit, adc1_channel_t &channel, adc2_channel_t &channel2);

public:
    
    // ---- Setup ---- //
    void pinMode(gpio_num_t pin, uint8_t mode);
    void pinMode(int pin, uint8_t mode);

    // ---- Digital ---- //
    void digitalWrite(gpio_num_t pin, uint8_t value);
    void digitalWrite(int pin, uint8_t value);
    int digitalRead(gpio_num_t pin);
    int digitalRead(int pin);

    // ---- Analog ---- //
    void analogInit();
    int analogRead(gpio_num_t gpio);

    // ---- PWM ---- //
    esp_err_t pwmInit(int pin, ledc_channel_t* out_channel);
    esp_err_t pwmWrite(ledc_channel_t channel, int duty);
    esp_err_t pwmStop(ledc_channel_t channel);

};

extern IOHelper io;