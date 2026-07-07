#include "register_mood_window.h"

#include <pebble.h>

#include "register_mood_window_logic.h"
#include "icons.h"
#include "menu_theme.h"
#include "repositories/app_config_repository.h"

static Window* m_mood_window;
static StatusBarLayer* m_status_bar;
static TextLayer* m_title_layer;
static TextLayer* m_value_layer;
static BitmapLayer* m_icon_layer;
static ActionBarLayer* m_action_bar;
// Per-option text labels, right-aligned next to the action bar at the Up /
// Select / Down button heights (index 0 = Up, 1 = Select, 2 = Down).
static TextLayer* m_option_labels[3];

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

static void setup_icon_layer(Layer *window_layer, GRect bounds)
{
    // Large center icon (when the metric has a main icon) for quick recognition.
    m_icon_layer = bitmap_layer_create(
        GRect(0, STATUS_BAR_LAYER_HEIGHT + 40, bounds.size.w - ACTION_BAR_WIDTH, 60));
    bitmap_layer_set_compositing_mode(m_icon_layer, GCompOpSet);
    bitmap_layer_set_alignment(m_icon_layer, GAlignCenter);
    bitmap_layer_set_background_color(m_icon_layer, GColorClear);
    layer_add_child(window_layer, bitmap_layer_get_layer(m_icon_layer));
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

static void setup_option_labels(Layer *window_layer, GRect bounds)
{
    int content_w = bounds.size.w - ACTION_BAR_WIDTH;
    int content_h = bounds.size.h - STATUS_BAR_LAYER_HEIGHT;
    // Vertical centres of the three action-bar buttons.
    int centers[3] = {
        STATUS_BAR_LAYER_HEIGHT + content_h / 4,
        STATUS_BAR_LAYER_HEIGHT + content_h / 2,
        STATUS_BAR_LAYER_HEIGHT + (3 * content_h) / 4,
    };
    for(int i = 0; i < 3; i++)
    {
        m_option_labels[i] = text_layer_create(
            GRect(4, centers[i] - 11, content_w - 8, 22));
        text_layer_set_background_color(m_option_labels[i], GColorClear);
        text_layer_set_text_color(m_option_labels[i], config_get_foreground_color());
        text_layer_set_font(m_option_labels[i], fonts_get_system_font(FONT_KEY_GOTHIC_18));
        text_layer_set_text_alignment(m_option_labels[i], GTextAlignmentRight);
        layer_add_child(window_layer, text_layer_get_layer(m_option_labels[i]));
    }
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

    m_status_bar = status_bar_create_themed(window_layer);
    setup_title_layer(window_layer, bounds);
    setup_icon_layer(window_layer, bounds);
    setup_value_layer(window_layer, bounds);
    setup_option_labels(window_layer, bounds);
    setup_action_bar(window_layer, bounds);

    register_mood_set_layers(m_mood_window, m_action_bar, m_title_layer, m_value_layer, m_icon_layer, m_option_labels);
    register_mood_start();
}

static void unload_mood_window(Window *window)
{
    register_mood_tear_down();
    status_bar_layer_destroy(m_status_bar);
    text_layer_destroy(m_title_layer);
    text_layer_destroy(m_value_layer);
    bitmap_layer_destroy(m_icon_layer);
    for(int i = 0; i < 3; i++)
    {
        text_layer_destroy(m_option_labels[i]);
    }
    action_bar_layer_remove_from_window(m_action_bar);
    action_bar_layer_destroy(m_action_bar);
}

static void push_mood_window()
{
    if(m_mood_window == NULL)
    {
        m_mood_window = window_create();
        window_set_window_handlers(m_mood_window, (WindowHandlers) {
            .load = load_mood_window,
            .unload = unload_mood_window
        });
    }
    window_stack_push(m_mood_window, true);
}

void setup_register_mood_window(MetricsGroup* group)
{
    register_mood_set_group(group);
    push_mood_window();
}

void setup_register_mood_window_for_metric(Metrics* metric)
{
    register_mood_set_metric(metric);
    push_mood_window();
}

void setup_register_mood_window_for_metric_in_group(Metrics* metric, uint16_t group_id)
{
    register_mood_set_metric_in_group(metric, group_id);
    push_mood_window();
}

void tear_down_register_mood_window()
{
    window_destroy(m_mood_window);
}
