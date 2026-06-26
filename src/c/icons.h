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

// Selectable icons a metric can use for its options / main icon. Stored by
// index on Metrics (option_icons / main_icon), so APPEND new entries only —
// never reorder, or persisted metrics would point at the wrong icon.
typedef enum
{
    IconChoice_NONE = 0,
    IconChoice_CHECK,
    IconChoice_CROSS,
    IconChoice_UP,
    IconChoice_DOWN,
    IconChoice_MOOD,
    IconChoice_EXERCISE,
    IconChoice_PILL,
    IconChoice_FACE_SAD,
    IconChoice_FACE_NEUTRAL,
    IconChoice_FACE_HAPPY,
    IconChoice_LEVEL_LOW,
    IconChoice_LEVEL_MID,
    IconChoice_LEVEL_HIGH,
    IconChoice_COUNT,
} IconChoice;

// Returns the bitmap for an IconChoice, or NULL for IconChoice_NONE / unknown.
GBitmap* get_icon_by_choice(uint8_t choice);

void destroy_all_icons();