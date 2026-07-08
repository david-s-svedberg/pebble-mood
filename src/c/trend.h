#pragma once

#include <pebble.h>
#include <stdint.h>
#include <stdbool.h>

#include "data.h"

// A metric's last-7-days daily aggregate for the home-screen sparkline.
// Index 0 is the oldest day (6 days ago), index TREND_DAYS-1 is today.
typedef struct
{
    uint16_t metric_id;
    uint8_t  points;                 // number of days that have data
    bool     has[TREND_DAYS];
    uint8_t  value[TREND_DAYS];      // daily mean (rounded), valid where has[]
    uint8_t  min_v;                  // effective range, for normalization
    uint8_t  max_v;
} TrendSeries;

// Builds the 7-day series for metric_id from the registrations store (daily
// mean per day). Returns false if the metric no longer exists.
bool trend_build(uint16_t metric_id, TrendSeries* out);
