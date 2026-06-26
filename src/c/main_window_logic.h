#pragma once

#include <pebble.h>

#include "data.h"

// Writes a short summary of a metric's registrations over the last 7 days into
// buffer: "3.4 avg (5)" for INTERVAL metrics, "57% yes (7)" for BOOL metrics,
// or "No data" when there are no registrations in the window.
void main_window_format_average(Metrics* metric, char* buffer, size_t size);
