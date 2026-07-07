#pragma once

#include <pebble.h>

#include "data.h"

// Writes a single registered value as the user sees it: "Yes"/"No" for BOOL,
// the number (1..max) for INTERVAL, or the per-option text (falling back to a
// 1-based number) for THREE_OPTION.
void main_window_format_value(Metrics* metric, uint8_t value, char* buffer, size_t size);
