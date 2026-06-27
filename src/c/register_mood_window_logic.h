#pragma once

#include <pebble.h>

#include "data.h"

void register_mood_set_group(MetricsGroup* group);
// Spontaneous single-metric registration: always adds a new registration.
void register_mood_set_metric(Metrics* metric);
// Today single-metric registration for a specific group slot: updates today's
// existing registration for that (group, metric) if present, else adds one.
void register_mood_set_metric_in_group(Metrics* metric, uint16_t group_id);
void register_mood_set_layers(
    Window* window,
    ActionBarLayer* action_bar,
    TextLayer* title_layer,
    TextLayer* value_layer,
    BitmapLayer* icon_layer);
void register_mood_start();
void register_mood_tear_down();
