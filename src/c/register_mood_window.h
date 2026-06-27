#pragma once

#include <pebble.h>

#include "data.h"

void setup_register_mood_window(MetricsGroup* group);
void setup_register_mood_window_for_metric(Metrics* metric);
void setup_register_mood_window_for_metric_in_group(Metrics* metric, uint16_t group_id);
void tear_down_register_mood_window();
