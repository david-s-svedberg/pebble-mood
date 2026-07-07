#pragma once

#include <pebble.h>


void set_alarm_window(Window* alarm_window);
void set_alarm_layers(
    TextLayer* alarm_time_layer,
    TextLayer* snooze_time_layer,
    ActionBarLayer* alarm_window_action_bar_layer);
// Returns false when the alarm can't be shown (its group no longer exists);
// the caller must then skip pushing the alarm window so the app exits quietly.
bool setup_alarm_state(int32_t alarm_index);
void alarm_window_click_config_provider(void* context);
char* get_wakeup_alarm_time_string();
char* get_alarm_title();