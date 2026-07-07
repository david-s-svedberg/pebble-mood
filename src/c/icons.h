#pragma once

#include <pebble.h>

// Check marks for menu rows (membership / answered-today), one per background.
GBitmap* get_check_icon_black();
GBitmap* get_check_icon_white();

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

// Fixed action-bar icons. The bar is painted with the theme's foreground
// colour, so get_bar_icon returns the variant that contrasts with it: the
// normal (dark-on-light) bitmap in the dark theme (white bar), the inverted /
// white bitmap in the light theme (black bar).
typedef enum
{
    BarIcon_CHECK,
    BarIcon_EDIT,
    BarIcon_CONFIG,
    BarIcon_PLAY,
    BarIcon_SNOOZE,
    BarIcon_SILENCE,
    BarIcon_ALARM,
    BarIcon_NO_ALARM,
    BarIcon_UP,
    BarIcon_DOWN,
    BarIcon_COUNT,
} BarIcon;

GBitmap* get_bar_icon(BarIcon icon);

void destroy_all_icons();