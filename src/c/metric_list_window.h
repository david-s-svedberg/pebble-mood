#pragma once

#include <pebble.h>

// A list of all metrics (title + 7-day average); selecting one registers it
// spontaneously. Reached from the home screen.
void setup_metric_list_window();
void tear_down_metric_list_window();
