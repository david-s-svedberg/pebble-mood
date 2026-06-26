#include "main_window.h"

#include <pebble.h>

#include "main_window_logic.h"
#include "config_menu_window.h"
#include "repositories/metrics_repository.h"
#include "repositories/app_config_repository.h"

#define SUBTITLE_LEN (24)

static Window* m_main_window;
static StatusBarLayer* m_status_bar;
static SimpleMenuLayer* m_menu_layer = NULL;

static SimpleMenuSection m_sections[2];
static SimpleMenuSection m_metrics_section = {0};
static SimpleMenuSection m_actions_section = {0};

static SimpleMenuItem* m_metric_items = NULL;       // one per metric (or a placeholder)
static char (*m_subtitles)[SUBTITLE_LEN] = NULL;    // backing store for the subtitles
static SimpleMenuItem m_settings_item = {0};

static void open_settings(int index, void* context)
{
    setup_config_window();
}

static void free_dynamic()
{
    if(m_metric_items != NULL)
    {
        free(m_metric_items);
        m_metric_items = NULL;
    }
    if(m_subtitles != NULL)
    {
        free(m_subtitles);
        m_subtitles = NULL;
    }
}

static void build_metrics_section()
{
    free_dynamic();

    uint32_t count = metrics_count();
    uint32_t item_count = (count == 0) ? 1 : count;

    m_metric_items = (SimpleMenuItem*)malloc(item_count * sizeof(SimpleMenuItem));
    memset(m_metric_items, 0, item_count * sizeof(SimpleMenuItem));
    m_subtitles = malloc(item_count * SUBTITLE_LEN);

    if(count == 0)
    {
        m_metric_items[0].title = "No metrics yet";
        m_metric_items[0].subtitle = "Add one in Settings";
    } else
    {
        Metrics* metrics = metrics_get_all();
        for(uint32_t i = 0; i < count; i++)
        {
            Metrics* metric = &metrics[i];
            main_window_format_average(metric, m_subtitles[i], SUBTITLE_LEN);
            m_metric_items[i].title = (metric->title != NULL) ? metric->title->value : "";
            m_metric_items[i].subtitle = m_subtitles[i];
            m_metric_items[i].callback = NULL;
        }
    }

    m_metrics_section.title = "Last 7 days";
    m_metrics_section.items = m_metric_items;
    m_metrics_section.num_items = item_count;
}

static void create_menu(Window* window)
{
    Layer* window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    if(m_menu_layer != NULL)
    {
        simple_menu_layer_destroy(m_menu_layer);
        m_menu_layer = NULL;
    }

    build_metrics_section();

    m_settings_item.title = "Settings";
    m_settings_item.callback = open_settings;
    m_actions_section.items = &m_settings_item;
    m_actions_section.num_items = 1;

    m_sections[0] = m_metrics_section;
    m_sections[1] = m_actions_section;

    m_menu_layer = simple_menu_layer_create(
        GRect(0, STATUS_BAR_LAYER_HEIGHT, bounds.size.w, bounds.size.h - STATUS_BAR_LAYER_HEIGHT),
        window, m_sections, 2, NULL);

    layer_add_child(window_layer, simple_menu_layer_get_layer(m_menu_layer));
}

static void setup_status_bar(Layer* window_layer)
{
    m_status_bar = status_bar_layer_create();
    status_bar_layer_set_colors(m_status_bar, config_get_background_color(), config_get_foreground_color());
    status_bar_layer_set_separator_mode(m_status_bar, StatusBarLayerSeparatorModeDotted);
    layer_add_child(window_layer, status_bar_layer_get_layer(m_status_bar));
}

static void load_main_window(Window* window)
{
    window_set_background_color(window, config_get_background_color());
    setup_status_bar(window_get_root_layer(window));
    create_menu(window);
}

static void appear_main_window(Window* window)
{
    // Rebuild so newly added/removed metrics and fresh averages are reflected
    // when returning from Settings.
    create_menu(window);
}

static void unload_main_window(Window* window)
{
    if(m_menu_layer != NULL)
    {
        simple_menu_layer_destroy(m_menu_layer);
        m_menu_layer = NULL;
    }
    status_bar_layer_destroy(m_status_bar);
    free_dynamic();
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
