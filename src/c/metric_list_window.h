#pragma once

#include <pebble.h>

typedef enum
{
    MetricList_ALL,    // every metric — spontaneous registration (home Select)
    MetricList_TODAY,  // scheduled + spontaneously-registered metrics, with a
                       // checkmark for those already answered today (home Up)
} MetricListMode;

// A list of metrics (title + 7-day average); selecting one registers it.
void setup_metric_list_window(MetricListMode mode);
void tear_down_metric_list_window();
