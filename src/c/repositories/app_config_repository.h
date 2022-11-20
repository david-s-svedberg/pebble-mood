#pragma once

#include <pebble.h>
#include <stdint.h>
#include <stdbool.h>

#include "../data.h"

bool        config_first_start();
void        config_init();
AppConfig*  config_get();
void        config_save();
GColor8     config_get_background_color();
GColor8     config_get_foreground_color();
uint8_t     config_get_alarm_timeout();
bool        config_is_dark_theme();
void        config_toggle_theme();
Alarm*      config_get_snooze_alarm();
Alarm*      config_get_summer_time_alarm();
void        config_set_alarm_timeout(uint8_t sec);