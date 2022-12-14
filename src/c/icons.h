#pragma once

#include <pebble.h>

GBitmap* get_check_icon();
GBitmap* get_check_icon_black();
GBitmap* get_check_icon_white();
GBitmap* get_config_icon();
GBitmap* get_alarm_icon();
GBitmap* get_no_alarm_icon();
GBitmap* get_alarm_icon_trans();
GBitmap* get_no_alarm_icon_trans();
GBitmap* get_edit_icon();
GBitmap* get_up_icon();
GBitmap* get_down_icon();
GBitmap* get_play_icon();
GBitmap* get_snooze_icon();
GBitmap* get_silence_icon();
GBitmap* get_pill_icon();
GBitmap* get_mood_icon();
GBitmap* get_exercise_icon();
GBitmap* get_cross_icon();

void destroy_all_icons();