#include "IOHelper.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

void IOHelper::pinMode(gpio_num_t pin, uint8_t mode)
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = 1ULL << pin;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    switch (mode) {
        case OUTPUT:
            io_conf.mode = GPIO_MODE_OUTPUT;
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;

        case INPUT:
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;

        case INPUT_PULLUP:
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;

        case INPUT_PULLDOWN:
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
            break;

        default:
            return;
    }

    gpio_config(&io_conf);
}

void IOHelper::pinMode(int pin, uint8_t mode)
{
    pinMode((gpio_num_t) pin, mode);
}

void IOHelper::digitalWrite(gpio_num_t pin, uint8_t value)
{
    gpio_set_level(pin, value ? 1 : 0);
}

void IOHelper::digitalWrite(int pin, uint8_t value)
{
    digitalWrite((gpio_num_t) pin, value ? 1 : 0);
}

int IOHelper::digitalRead(gpio_num_t pin)
{
    return gpio_get_level(pin);
}

int IOHelper::digitalRead(int pin)
{
    return digitalRead((gpio_num_t) pin);
}

/* ================= ADC ================= */

void IOHelper::analogInit() {
    // Configure ADC1 width (12-bit)
    adc1_config_width(ADC_WIDTH_BIT_12);
    // ADC2 doesn't need width config; it uses 12-bit by default
}

bool IOHelper::gpioToAdcUnitChannel(gpio_num_t gpio, adc_unit_t &unit, adc1_channel_t &channel1, adc2_channel_t &channel2) {
    switch(gpio) {
        // ----- ADC1 GPIOs -----
        case GPIO_NUM_1: channel1 = ADC1_CHANNEL_0; unit = ADC_UNIT_1; return true;
        case GPIO_NUM_2: channel1 = ADC1_CHANNEL_1; unit = ADC_UNIT_1; return true;
        case GPIO_NUM_3: channel1 = ADC1_CHANNEL_2; unit = ADC_UNIT_1; return true;
        case GPIO_NUM_4: channel1 = ADC1_CHANNEL_3; unit = ADC_UNIT_1; return true;
        case GPIO_NUM_5: channel1 = ADC1_CHANNEL_4; unit = ADC_UNIT_1; return true;
        case GPIO_NUM_6: channel1 = ADC1_CHANNEL_5; unit = ADC_UNIT_1; return true;
        case GPIO_NUM_7: channel1 = ADC1_CHANNEL_6; unit = ADC_UNIT_1; return true;
        case GPIO_NUM_8: channel1 = ADC1_CHANNEL_7; unit = ADC_UNIT_1; return true;
        case GPIO_NUM_9: channel1 = ADC1_CHANNEL_8; unit = ADC_UNIT_1; return true;
        case GPIO_NUM_10: channel1 = ADC1_CHANNEL_9; unit = ADC_UNIT_1; return true;

        // ----- ADC2 GPIOs -----
        case GPIO_NUM_11:  channel2 = ADC2_CHANNEL_0; unit = ADC_UNIT_2; return true;
        case GPIO_NUM_12:  channel2 = ADC2_CHANNEL_1; unit = ADC_UNIT_2; return true;
        case GPIO_NUM_13:  channel2 = ADC2_CHANNEL_2; unit = ADC_UNIT_2; return true;
        case GPIO_NUM_14: channel2 = ADC2_CHANNEL_3; unit = ADC_UNIT_2; return true;
        case GPIO_NUM_15: channel2 = ADC2_CHANNEL_4; unit = ADC_UNIT_2; return true;
        case GPIO_NUM_16: channel2 = ADC2_CHANNEL_5; unit = ADC_UNIT_2; return true;
        case GPIO_NUM_17: channel2 = ADC2_CHANNEL_6; unit = ADC_UNIT_2; return true;
        case GPIO_NUM_18: channel2 = ADC2_CHANNEL_7; unit = ADC_UNIT_2; return true;
        // USB reserved pins
        // case GPIO_NUM_19: channel2 = ADC2_CHANNEL_8; unit = ADC_UNIT_2; return true;
        // case GPIO_NUM_20: channel2 = ADC2_CHANNEL_9; unit = ADC_UNIT_2; return true;

        default:
            return false; // invalid GPIO
    }
}

int IOHelper::analogRead(gpio_num_t gpio)
{
    adc_unit_t unit;
    adc1_channel_t ch1;
    adc2_channel_t ch2;

    if(!gpioToAdcUnitChannel(gpio, unit, ch1, ch2)) {
        ESP_LOGE("IOHelper", "Invalid ADC GPIO: %d", gpio);
        return -1;
    }

    if(unit == ADC_UNIT_1) {
        // Configure attenuation (11dB ≈ 0–3.3V)
        adc1_config_channel_atten(ch1, ADC_ATTEN_DB_11);
        return adc1_get_raw(ch1); // 0–4095
    } else { // ADC2
        int val = 0;
        // ADC2 may fail if WiFi is active
        esp_err_t ret = adc2_config_channel_atten(ch2, ADC_ATTEN_DB_11);
        if(ret != ESP_OK) {
            ESP_LOGE("IOHelper", "ADC2 config failed (maybe WiFi active)");
            return -1;
        }
        ret = adc2_get_raw(ch2, ADC_WIDTH_BIT_12, &val);
        if(ret != ESP_OK) {
            ESP_LOGE("IOHelper", "ADC2 read failed (maybe WiFi active)");
            return -1;
        }
        return val; // 0–4095
    }
}

/* ================= PWM ================= */

static bool channel_used[LEDC_CHANNEL_MAX] = {false};

esp_err_t IOHelper::pwmInit(int pin, ledc_channel_t* out_channel)
{
    // Find a free channel
    ledc_channel_t channel = LEDC_CHANNEL_0;
    for (int i = 0; i < LEDC_CHANNEL_MAX; i++) {
        if (!channel_used[i]) {
            channel = (ledc_channel_t)i;
            channel_used[i] = true;
            break;
        }
    }

    // Configure timer (we'll use the same timer for all channels here)
    ledc_timer_config_t timer_conf = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = 5000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&timer_conf);
    if (err != ESP_OK) return err;

    // Configure channel
    ledc_channel_config_t ch_conf = {
        .gpio_num       = pin,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = channel,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 0,
        .hpoint         = 0
    };
    err = ledc_channel_config(&ch_conf);
    if (err != ESP_OK) return err;

    if (out_channel) *out_channel = channel;
    return ESP_OK;
}

esp_err_t IOHelper::pwmWrite(ledc_channel_t channel, int duty)
{
    if (channel >= LEDC_CHANNEL_MAX) 
        return ESP_ERR_INVALID_ARG;

    // Clamp duty to 8-bit range
    if (duty < 0) duty = 0;
    if (duty > 255) duty = 255;

    // Set duty
    esp_err_t err = ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
    if (err != ESP_OK) return err;

    // Apply duty
    return ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

esp_err_t IOHelper::pwmStop(ledc_channel_t channel)
{
    if(channel >= LEDC_CHANNEL_MAX)
        return ESP_ERR_INVALID_ARG;
    channel_used[channel] = false;
    return ledc_stop(LEDC_LOW_SPEED_MODE, channel, 0);
}