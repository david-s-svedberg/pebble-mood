#pragma once

#include <pebble.h>

#include "data.h"

void register_mood_set_group(MetricsGroup* group);
void register_mood_set_metric(Metrics* metric);
void register_mood_set_layers(
    Window* window,
    ActionBarLayer* action_bar,
    TextLayer* title_layer,
    TextLayer* value_layer,
    BitmapLayer* icon_layer);
void register_mood_start();
void register_mood_tear_down();
