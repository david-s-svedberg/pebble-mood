#pragma once

#include <pebble.h>

// Invoked when the user picks an icon. `choice` is an IconChoice value.
typedef void (*IconPickerCallback)(uint8_t choice, void* context);

// Opens a scrollable list of selectable icons, each drawn highlight-aware
// (white on the selected row, black otherwise). When `small_only` is true only
// option-sized icons are offered (for the per-option action bar); otherwise the
// full set including the large main icons is shown. `current` selects/scrolls to
// the metric's current choice. On Select, `cb(choice, context)` runs and the
// picker pops itself back to the caller.
void setup_icon_picker_window(bool small_only, uint8_t current,
    IconPickerCallback cb, void* context);

void tear_down_icon_picker_window();
