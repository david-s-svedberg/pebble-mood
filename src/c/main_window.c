#include "main_window.h"

#include <pebble.h>

#include "config_menu_window.h"
#include "metric_list_window.h"
#include "scheduler.h"
#include "icons.h"
#include "repositories/app_config_repository.h"

static Window* m_main_window;
static StatusBarLayer* m_status_bar;
static TextLayer* m_title_layer;
static TextLayer* m_next_layer;
static ActionBarLayer* m_action_bar;
static char m_next_buffer[24];

static void open_settings(ClickRecognizerRef recognizer, void* context)
{
    setup_config_window();
}

static void open_register(ClickRecognizerRef recognizer, void* context)
{
    // Spontaneous registration: pick any metric and register it now.
    setup_metric_list_window(MetricList_ALL);
}

static void open_today(ClickRecognizerRef recognizer, void* context)
{
    // Today: scheduled + spontaneously-registered metrics with answered status.
    setup_metric_list_window(MetricList_TODAY);
}

static void click_config_provider(void* context)
{
    window_single_click_subscribe(BUTTON_ID_UP, open_today);
    window_single_click_subscribe(BUTTON_ID_SELECT, open_register);
    window_single_click_subscribe(BUTTON_ID_DOWN, open_settings);
}

static void update_next_time()
{
    snprintf(m_next_buffer, sizeof(m_next_buffer), "Next: %s", get_next_alarm_time_string());
    text_layer_set_text(m_next_layer, m_next_buffer);
}

static void setup_status_bar(Layer* window_layer)
{
    m_status_bar = status_bar_layer_create();
    status_bar_layer_set_colors(m_status_bar, config_get_background_color(), config_get_foreground_color());
    status_bar_layer_set_separator_mode(m_status_bar, StatusBarLayerSeparatorModeDotted);
    layer_add_child(window_layer, status_bar_layer_get_layer(m_status_bar));
}

static void setup_title_layer(Layer* window_layer, GRect bounds)
{
    m_title_layer = text_layer_create(
        GRect(0, STATUS_BAR_LAYER_HEIGHT + 20, bounds.size.w - ACTION_BAR_WIDTH, 44));
    text_layer_set_background_color(m_title_layer, config_get_background_color());
    text_layer_set_text_color(m_title_layer, config_get_foreground_color());
    text_layer_set_font(m_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text_alignment(m_title_layer, GTextAlignmentCenter);
    text_layer_set_text(m_title_layer, "Mood");
    layer_add_child(window_layer, text_layer_get_layer(m_title_layer));
}

static void setup_next_layer(Layer* window_layer, GRect bounds)
{
    m_next_layer = text_layer_create(
        GRect(0, bounds.size.h / 2, bounds.size.w - ACTION_BAR_WIDTH, 30));
    text_layer_set_background_color(m_next_layer, config_get_background_color());
    text_layer_set_text_color(m_next_layer, config_get_foreground_color());
    text_layer_set_font(m_next_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
    text_layer_set_text_alignment(m_next_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(m_next_layer));
}

static void setup_action_bar()
{
    m_action_bar = action_bar_layer_create();
    action_bar_layer_set_background_color(m_action_bar, config_get_foreground_color());
    action_bar_layer_add_to_window(m_action_bar, m_main_window);
    action_bar_layer_set_click_config_provider(m_action_bar, click_config_provider);
    action_bar_layer_set_icon_animated(m_action_bar, BUTTON_ID_UP, get_check_icon(), true);
    action_bar_layer_set_icon_animated(m_action_bar, BUTTON_ID_SELECT, get_edit_icon(), true);
    action_bar_layer_set_icon_animated(m_action_bar, BUTTON_ID_DOWN, get_config_icon(), true);
}

static void load_main_window(Window* window)
{
    window_set_background_color(window, config_get_background_color());
    Layer* window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    setup_status_bar(window_layer);
    setup_title_layer(window_layer, bounds);
    setup_next_layer(window_layer, bounds);
    setup_action_bar();
}

static void appear_main_window(Window* window)
{
    update_next_time();
}

static void unload_main_window(Window* window)
{
    status_bar_layer_destroy(m_status_bar);
    text_layer_destroy(m_title_layer);
    text_layer_destroy(m_next_layer);
    action_bar_layer_remove_from_window(m_action_bar);
    action_bar_layer_destroy(m_action_bar);
}

void setup_main_window()
{
    m_main_window = window_create();
    window_set_window_handlers(m_main_window, (WindowHandlers) {
        .load = load_main_window,
        .unload = unload_main_window,
        .appear = appear_main_window,
    });
    window_stack_push(m_main_window, true);
}

void tear_down_main_window()
{
    window_destroy(m_main_window);
}
