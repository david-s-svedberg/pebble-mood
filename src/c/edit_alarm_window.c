#include "edit_alarm_window.h"

#include <pebble.h>

#include "repositories/app_config_repository.h"
#include "edit_alarm_window_logic.h"
#include "persistance.h"
#include "icons.h"

static Window *edit_alarm_window;

static StatusBarLayer* status_bar;

static TextLayer *edit_alarm_active_layer;
static TextLayer *edit_alarm_time_hour_layer;
static TextLayer *edit_alarm_time_colon_layer;
static TextLayer *edit_alarm_time_minute_layer;
static ActionBarLayer* edit_alarm_action_bar_layer;

static void setup_edit_alarm_action_bar_layer(Layer *window_layer, GRect bounds)
{
    edit_alarm_action_bar_layer = action_bar_layer_create();
    action_bar_layer_set_background_color(edit_alarm_action_bar_layer, config_get_foreground_color());
    action_bar_layer_add_to_window(edit_alarm_action_bar_layer, edit_alarm_window);
    action_bar_layer_set_click_config_provider(edit_alarm_action_bar_layer, edit_alarm_action_bar_click_config_provider);

    action_bar_layer_set_icon_animated(edit_alarm_action_bar_layer, BUTTON_ID_UP, get_bar_icon(get_edit_alarm()->active ? BarIcon_ALARM : BarIcon_NO_ALARM), true);
    action_bar_layer_set_icon_animated(edit_alarm_action_bar_layer, BUTTON_ID_SELECT, get_bar_icon(BarIcon_EDIT), true);
    action_bar_layer_set_icon_animated(edit_alarm_action_bar_layer, BUTTON_ID_DOWN, get_bar_icon(BarIcon_CHECK), true);
}

static void setup_edit_alarm_active_layer(Layer *window_layer, GRect bounds)
{
    edit_alarm_active_layer = text_layer_create(GRect(0, 14, bounds.size.w - ACTION_BAR_WIDTH, 30));

    text_layer_set_background_color(edit_alarm_active_layer, config_get_background_color());

    text_layer_set_text_color(edit_alarm_active_layer, config_get_foreground_color());
    text_layer_set_font(edit_alarm_active_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));

    text_layer_set_text_alignment(edit_alarm_active_layer, GTextAlignmentCenter);

    text_layer_set_text(edit_alarm_active_layer, get_edit_alarm()->active ? "Active" : "Inactive");

    layer_add_child(window_layer, text_layer_get_layer(edit_alarm_active_layer));
}

static void update_edit_alarm_window(Window* window)
{
    update_edit_alarm_time_layers();
}

static void setup_edit_alarm_time_layer(Layer *window_layer, GRect bounds)
{
    // Center the HH:MM group in the content area (left of the action bar) and
    // scale up on the larger emery screen.
    uint16_t content_w = bounds.size.w - ACTION_BAR_WIDTH;
    bool big = bounds.size.w >= 180;

    const char* font_key = big ? FONT_KEY_BITHAM_42_BOLD : FONT_KEY_GOTHIC_28_BOLD;
    uint16_t height = big ? 50 : 37;
    uint16_t hour_w = big ? 56 : 40;
    uint16_t colon_w = big ? 20 : 16;
    uint16_t minute_w = big ? 56 : 40;
    uint16_t total_w = hour_w + colon_w + minute_w;
    int16_t x = (content_w - total_w) / 2;
    uint16_t y = (bounds.size.h - height) / 2;

    edit_alarm_time_hour_layer = text_layer_create(GRect(x, y, hour_w, height));
    edit_alarm_time_colon_layer = text_layer_create(GRect(x + hour_w, y - 2, colon_w, height));
    edit_alarm_time_minute_layer = text_layer_create(GRect(x + hour_w + colon_w, y, minute_w, height));

    GColor background_color = config_get_background_color();
    GColor foreground_color = config_get_foreground_color();
    text_layer_set_background_color(edit_alarm_time_hour_layer, background_color);
    text_layer_set_background_color(edit_alarm_time_colon_layer, background_color);
    text_layer_set_background_color(edit_alarm_time_minute_layer, background_color);

    text_layer_set_text_color(edit_alarm_time_hour_layer, foreground_color);
    text_layer_set_text_color(edit_alarm_time_colon_layer, foreground_color);
    text_layer_set_text_color(edit_alarm_time_minute_layer, foreground_color);

    text_layer_set_font(edit_alarm_time_hour_layer, fonts_get_system_font(font_key));
    text_layer_set_font(edit_alarm_time_colon_layer, fonts_get_system_font(font_key));
    text_layer_set_font(edit_alarm_time_minute_layer, fonts_get_system_font(font_key));

    text_layer_set_text_alignment(edit_alarm_time_hour_layer, GTextAlignmentCenter);
    text_layer_set_text_alignment(edit_alarm_time_colon_layer, GTextAlignmentCenter);
    text_layer_set_text_alignment(edit_alarm_time_minute_layer, GTextAlignmentCenter);

    text_layer_set_text(edit_alarm_time_colon_layer, ":");

    layer_add_child(window_layer, text_layer_get_layer(edit_alarm_time_hour_layer));
    layer_add_child(window_layer, text_layer_get_layer(edit_alarm_time_colon_layer));
    layer_add_child(window_layer, text_layer_get_layer(edit_alarm_time_minute_layer));
}

static void setup_status_bar(Layer *window_layer, GRect bounds)
{
    status_bar = status_bar_layer_create();

    status_bar_layer_set_colors(status_bar, config_get_background_color(), config_get_foreground_color());
    status_bar_layer_set_separator_mode(status_bar, StatusBarLayerSeparatorModeDotted);

    layer_add_child(window_layer, status_bar_layer_get_layer(status_bar));
}

static void load_edit_alarm_window(Window *edit_alarm_window)
{
    window_set_background_color(edit_alarm_window, config_get_background_color());
    Layer *edit_alarm_window_layer = window_get_root_layer(edit_alarm_window);
    GRect edit_alarm_window_bounds = layer_get_bounds(edit_alarm_window_layer);

    setup_edit_alarm_action_bar_layer(edit_alarm_window_layer, edit_alarm_window_bounds);
    setup_edit_alarm_active_layer(edit_alarm_window_layer, edit_alarm_window_bounds);
    setup_edit_alarm_time_layer(edit_alarm_window_layer, edit_alarm_window_bounds);
    setup_status_bar(edit_alarm_window_layer, edit_alarm_window_bounds);

    set_edit_alarm_layers(
        edit_alarm_active_layer,
        edit_alarm_time_hour_layer,
        edit_alarm_time_minute_layer,
        edit_alarm_action_bar_layer);
}

static void unload_edit_alarm_window(Window *window)
{
    text_layer_destroy(edit_alarm_active_layer);
    text_layer_destroy(edit_alarm_time_hour_layer);
    text_layer_destroy(edit_alarm_time_colon_layer);
    text_layer_destroy(edit_alarm_time_minute_layer);
    action_bar_layer_remove_from_window(edit_alarm_action_bar_layer);
    action_bar_layer_destroy(edit_alarm_action_bar_layer);
    status_bar_layer_destroy(status_bar);
}

void setup_edit_alarm_window(MetricsGroup* metrics_group)
{
    set_metrics_group(metrics_group);

    if(edit_alarm_window == NULL)
    {
        edit_alarm_window = window_create();
        window_set_window_handlers(edit_alarm_window, (WindowHandlers) {
            .load = load_edit_alarm_window,
            .unload = unload_edit_alarm_window,
            .appear = update_edit_alarm_window
        });
    }

    window_stack_push(edit_alarm_window, true);
}

void tear_down_edit_alarm_window()
{
    window_destroy(edit_alarm_window);
}