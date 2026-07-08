#pragma once

#include <pebble.h>
#include <stdint.h>
#include <stdbool.h>

#include "../data.h"

// True until config_mark_started() has run once. Read-only: call
// config_mark_started() after first-start work (seeding) has COMPLETED, so a
// crash mid-seed retries on the next launch instead of leaving an empty app.
bool        config_first_start();
void        config_mark_started();
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
uint16_t    config_get_snoozed_group_id();
void        config_set_snoozed_group_id(uint16_t group_id);
bool        config_alarms_suspended();
void        config_set_alarms_suspended(bool suspended);