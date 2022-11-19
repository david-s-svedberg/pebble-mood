#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "data.h"

extern const uint8_t FOUR_MINUTES_IN_SEC;
extern const uint8_t ONE_MINUTES_IN_SEC;

Alarm* get_daily_alarm();
Alarm* get_snooze_alarm();
Alarm* get_next_alarm();

GColor8 get_background_color();
GColor8 get_foreground_color();

uint8_t get_alarm_timeout();
void set_alarm_timeout(uint8_t sec);

bool is_dark_theme();
void toggle_theme();

bool has_any_data();
void save_data();

uint8_t get_last_score(uint8_t measurement_point_index);
void set_last_score(uint8_t measurement_point_index, uint8_t score);

void tear_down_data();