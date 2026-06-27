#pragma once

#include <pebble.h>

#include "data.h"

// Writes a short summary of a metric's registrations over the last 7 days into
// buffer: "3.4 avg (5)" for INTERVAL metrics, "57% yes (7)" for BOOL metrics,
// or "No data" when there are no registrations in the window.
void main_window_format_average(Metrics* metric, char* buffer, size_t size);

// Writes a single registered value as the user sees it: "Yes"/"No" for BOOL,
// the number (1..max) for INTERVAL, or the per-option text (falling back to a
// 1-based number) for THREE_OPTION.
void main_window_format_value(Metrics* metric, uint8_t value, char* buffer, size_t size);
