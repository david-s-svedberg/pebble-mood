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
    IconChoice_SUN,
    IconChoice_MOON,
    IconChoice_DROPLET,
    IconChoice_HEART,
    IconChoice_BOLT,
    IconChoice_COFFEE,
    IconChoice_GLASS,
    IconChoice_THERMO,
    IconChoice_PHONE,
    IconChoice_CLOUD,
    IconChoice_DUMBBELL,
    IconChoice_BUBBLE,
    IconChoice_CHECKBOX,
    IconChoice_APPLE,
    IconChoice_TARGET,
    IconChoice_PULSE,
    IconChoice_COUNT,
} IconChoice;

// Returns the bitmap for an IconChoice, or NULL for IconChoice_NONE / unknown.
// `light` picks the white variant (for drawing on a dark/highlighted background);
// otherwise the black variant (for a light background) is returned.
GBitmap* get_icon_by_choice_ex(uint8_t choice, bool light);

// Convenience: the black (light-background) variant.
GBitmap* get_icon_by_choice(uint8_t choice);

// A menu-row-sized (20px) bitmap for an IconChoice. Main icons use a dedicated
// small variant; smaller choices fall back to their normal bitmap. `light`
// picks the white variant. NULL for IconChoice_NONE / unknown.
GBitmap* get_icon_row_by_choice(uint8_t choice, bool light);

// Human-readable name for an IconChoice (e.g. "Happy face"), "None" for unknown.
const char* icon_choice_name(uint8_t choice);

// True for icons small enough to sit in a menu row / the narrow action bar.
// Large display icons (mood/exercise + the main-icon set) are main-icon only.
bool icon_choice_is_small(uint8_t choice);

void destroy_all_icons();