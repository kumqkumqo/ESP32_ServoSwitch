#include <Arduino.h>
#include <esp32-hal-ledc.h>
#include "ESP32_ServoSwitch.h"

/* prottype */
uint32_t deg_to_pwm(int);
/* prottype */

ESP32_ServoSwitch::ESP32_ServoSwitch(uint8_t pin, uint8_t channel, int switch_off_deg, int switch_on_deg, bool is_suspend_enabled)
{
    _channel = channel;
    _pin = pin;
    _switch_off_deg = switch_off_deg;
    _switch_on_deg = switch_on_deg;
    _switch_mid_deg = (switch_on_deg + switch_off_deg) / 2;

    _target_deg = switch_off_deg;
    _current_deg = switch_off_deg;

    _is_suspend_enabled = is_suspend_enabled;
    _time_to_suspend = SERVO_SUSPEND_DELAY;
    _is_suspend = false;

    ledcSetup(_channel, PWM_HZ, DUTY_BITRATE); // 16 bit = 65535
    ledcAttachPin(_pin, _channel);
}

ESP32_ServoSwitch::~ESP32_ServoSwitch()
{
    ESP32_ServoSwitch::suspend();
}

void ESP32_ServoSwitch::suspend()
{
    if (!_is_suspend)
    {
        ledcDetachPin(_pin);
        _time_to_suspend = 0;
    }
    _is_suspend = true;
}

void ESP32_ServoSwitch::resume()
{
    if (_is_suspend)
    {
        ledcAttachPin(_pin, _channel);
        _time_to_suspend = SERVO_SUSPEND_DELAY;
    }
    _is_suspend = false;
}

void ESP32_ServoSwitch::on()
{

    _target_deg = _switch_on_deg;
}

void ESP32_ServoSwitch::off()
{
    _target_deg = _switch_off_deg;
}

void ESP32_ServoSwitch::toggle()
{
    if (_current_deg <= _switch_mid_deg)
    {
        _target_deg = _switch_on_deg;
    }
    else
    {
        _target_deg = _switch_off_deg;
    }
}

void ESP32_ServoSwitch::update()
{
    if (_current_deg == _target_deg)
    {
        _time_to_suspend = (((_time_to_suspend - 1) >= 0) ? (_time_to_suspend - 1) : 0);
    }
    else
    {
        _time_to_suspend = SERVO_SUSPEND_DELAY;
    }

    if (_current_deg < _target_deg)
    {

        if (_current_deg + SERVO_MOVINGSTEP_DEG <= _switch_on_deg)
        {
            _current_deg = _current_deg + SERVO_MOVINGSTEP_DEG;
        }
        else
        {
            _current_deg = _switch_on_deg;
        }
    }
    else if (_current_deg > _target_deg)
    {
        if (_current_deg - SERVO_MOVINGSTEP_DEG > _switch_off_deg)
        {
            _current_deg = _current_deg - SERVO_MOVINGSTEP_DEG;
        }
        else
        {
            _current_deg = _switch_off_deg;
        }
    }
    else
    {
        _current_deg = _target_deg;
    }

    if (!_is_suspend_enabled)
    {
        ledcWrite(_channel, deg_to_pwm(_current_deg));
    }
    else
    {
        if (_is_suspend)
        {
            if (_time_to_suspend > 0)
            {
                /* resume */
                ESP32_ServoSwitch::resume();
                ledcWrite(_channel, deg_to_pwm(_current_deg));
            }
            else
            {
                /* in suspend, nothing to do*/
            }
        }
        else
        {
            if (_time_to_suspend <= 0)
            {
                /* go suspend */
                ESP32_ServoSwitch::suspend();
            }
            else
            {
                /* normal output */
                ledcWrite(_channel, deg_to_pwm(_current_deg));
            }
        }
    }
}

bool ESP32_ServoSwitch::Is_state_on()
{
    return (_current_deg == _switch_on_deg);
}

bool ESP32_ServoSwitch::Is_state_off()
{
    return (_current_deg == _switch_off_deg);
}

bool ESP32_ServoSwitch::Is_suspend()
{
    return _is_suspend;
}

uint32_t deg_to_pwm(int deg)
{
    float duty;
    uint32_t duty_bin;

    duty = (((float)deg * (SERVO_MIN_DUTY + SERVO_MAX_DUTY)) / (SERVO_MAX_DEG - SERVO_MIN_DEG)) + SERVO_MIN_DUTY;
    duty_bin = uint32_t((duty * DUTY_100_BIN) / 100);

    return duty_bin;
}
