#include "register_mood_window.h"

#include <pebble.h>

#include "register_mood_window_logic.h"
#include "icons.h"
#include "repositories/app_config_repository.h"

static Window* m_mood_window;
static StatusBarLayer* m_status_bar;
static TextLayer* m_title_layer;
static TextLayer* m_value_layer;
static ActionBarLayer* m_action_bar;

static void setup_status_bar(Layer *window_layer, GRect bounds)
{
    m_status_bar = status_bar_layer_create();
    status_bar_layer_set_colors(m_status_bar, config_get_background_color(), config_get_foreground_color());
    status_bar_layer_set_separator_mode(m_status_bar, StatusBarLayerSeparatorModeDotted);
    layer_add_child(window_layer, status_bar_layer_get_layer(m_status_bar));
}

static void setup_title_layer(Layer *window_layer, GRect bounds)
{
    m_title_layer = text_layer_create(
        GRect(2, STATUS_BAR_LAYER_HEIGHT + 4, bounds.size.w - ACTION_BAR_WIDTH - 4, 64));
    text_layer_set_background_color(m_title_layer, config_get_background_color());
    text_layer_set_text_color(m_title_layer, config_get_foreground_color());
    text_layer_set_font(m_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text_alignment(m_title_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(m_title_layer));
}

static void setup_value_layer(Layer *window_layer, GRect bounds)
{
    m_value_layer = text_layer_create(
        GRect(0, bounds.size.h / 2, bounds.size.w - ACTION_BAR_WIDTH, 56));
    text_layer_set_background_color(m_value_layer, config_get_background_color());
    text_layer_set_text_color(m_value_layer, config_get_foreground_color());
    text_layer_set_font(m_value_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_alignment(m_value_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(m_value_layer));
}

static void setup_action_bar(Layer *window_layer, GRect bounds)
{
    m_action_bar = action_bar_layer_create();
    action_bar_layer_set_background_color(m_action_bar, config_get_foreground_color());
    action_bar_layer_add_to_window(m_action_bar, m_mood_window);
    // Icons and click config provider are set per-metric by the logic.
}

static void load_mood_window(Window *window)
{
    window_set_background_color(window, config_get_background_color());
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    setup_status_bar(window_layer, bounds);
    setup_title_layer(window_layer, bounds);
    setup_value_layer(window_layer, bounds);
    setup_action_bar(window_layer, bounds);

    register_mood_set_layers(m_mood_window, m_action_bar, m_title_layer, m_value_layer);
    register_mood_start();
}

static void unload_mood_window(Window *window)
{
    register_mood_tear_down();
    status_bar_layer_destroy(m_status_bar);
    text_layer_destroy(m_title_layer);
    text_layer_destroy(m_value_layer);
    action_bar_layer_remove_from_window(m_action_bar);
    action_bar_layer_destroy(m_action_bar);
}

void setup_register_mood_window(MetricsGroup* group)
{
    register_mood_set_group(group);

    m_mood_window = window_create();
    window_set_window_handlers(m_mood_window, (WindowHandlers) {
        .load = load_mood_window,
        .unload = unload_mood_window
    });

    window_stack_push(m_mood_window, true);
}

void tear_down_register_mood_window()
{
    window_destroy(m_mood_window);
}
