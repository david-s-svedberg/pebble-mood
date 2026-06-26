#include "metric_list_window.h"

#include <pebble.h>

#include "main_window_logic.h"   // main_window_format_average
#include "icons.h"
#include "register_mood_window.h"
#include "repositories/metrics_repository.h"
#include "repositories/app_config_repository.h"

#define SUBTITLE_LEN (24)

static Window* m_window;
static StatusBarLayer* m_status_bar;
static SimpleMenuLayer* m_menu_layer = NULL;
static MetricListMode m_mode;

static SimpleMenuSection m_section;
static SimpleMenuItem* m_items = NULL;
static char (*m_subtitles)[SUBTITLE_LEN] = NULL;
static uint16_t* m_id_map = NULL;

static void free_dynamic()
{
    if(m_items != NULL) { free(m_items); m_items = NULL; }
    if(m_subtitles != NULL) { free(m_subtitles); m_subtitles = NULL; }
    if(m_id_map != NULL) { free(m_id_map); m_id_map = NULL; }
}

static void register_selected(int index, void* context)
{
    Metrics* metric = metrics_get(m_id_map[index]);
    if(metric != NULL)
    {
        setup_register_mood_window_for_metric(metric);
    }
}

static void build_items()
{
    free_dynamic();

    uint32_t total = metrics_count();
    uint32_t cap = (total == 0) ? 1 : total;
    m_items = (SimpleMenuItem*)malloc(cap * sizeof(SimpleMenuItem));
    memset(m_items, 0, cap * sizeof(SimpleMenuItem));
    m_subtitles = malloc(cap * SUBTITLE_LEN);
    m_id_map = (uint16_t*)malloc(cap * sizeof(uint16_t));

    Metrics* metrics = metrics_get_all();
    uint32_t n = 0;
    for(uint32_t i = 0; i < total; i++)
    {
        Metrics* metric = &metrics[i];
        bool done_today = metric_registered_today(metric->id);

        // Today view: only metrics that are scheduled (in a group) or were
        // registered spontaneously today.
        if(m_mode == MetricList_TODAY && !metric_in_any_group(metric->id) && !done_today)
        {
            continue;
        }

        m_id_map[n] = metric->id;
        main_window_format_average(metric, m_subtitles[n], SUBTITLE_LEN);
        m_items[n].title = metric->title != NULL ? metric->title->value : "";
        m_items[n].subtitle = m_subtitles[n];
        m_items[n].callback = register_selected;
        if(m_mode == MetricList_TODAY && done_today)
        {
            m_items[n].icon = config_is_dark_theme() ? get_check_icon_white() : get_check_icon_black();
        }
        n++;
    }

    if(n == 0)
    {
        m_items[0].title = (m_mode == MetricList_TODAY) ? "Nothing scheduled" : "No metrics yet";
        m_items[0].subtitle = (m_mode == MetricList_TODAY) ? "" : "Add one in Settings";
        m_items[0].callback = NULL;
        n = 1;
    }

    m_section.title = (m_mode == MetricList_TODAY) ? "Today" : "Register";
    m_section.items = m_items;
    m_section.num_items = n;
}

static void create_menu()
{
    Layer* window_layer = window_get_root_layer(m_window);
    GRect bounds = layer_get_bounds(window_layer);

    if(m_menu_layer != NULL)
    {
        simple_menu_layer_destroy(m_menu_layer);
        m_menu_layer = NULL;
    }

    build_items();

    m_menu_layer = simple_menu_layer_create(
        GRect(0, STATUS_BAR_LAYER_HEIGHT, bounds.size.w, bounds.size.h - STATUS_BAR_LAYER_HEIGHT),
        m_window, &m_section, 1, NULL);

    layer_add_child(window_layer, simple_menu_layer_get_layer(m_menu_layer));
}

static void setup_status_bar(Layer* window_layer)
{
    m_status_bar = status_bar_layer_create();
    status_bar_layer_set_colors(m_status_bar, config_get_background_color(), config_get_foreground_color());
    status_bar_layer_set_separator_mode(m_status_bar, StatusBarLayerSeparatorModeDotted);
    layer_add_child(window_layer, status_bar_layer_get_layer(m_status_bar));
}

static void load_window(Window* window)
{
    window_set_background_color(window, config_get_background_color());
    setup_status_bar(window_get_root_layer(window));
    create_menu();
}

static void appear_window(Window* window)
{
    // Refresh averages and today-status after returning from a registration.
    create_menu();
}

static void unload_window(Window* window)
{
    if(m_menu_layer != NULL)
    {
        simple_menu_layer_destroy(m_menu_layer);
        m_menu_layer = NULL;
    }
    status_bar_layer_destroy(m_status_bar);
    free_dynamic();
}

void setup_metric_list_window(MetricListMode mode)
{
    m_mode = mode;
    m_window = window_create();
    window_set_window_handlers(m_window, (WindowHandlers) {
        .load = load_window,
        .unload = unload_window,
        .appear = appear_window,
    });
    window_stack_push(m_window, true);
}

void tear_down_metric_list_window()
{
    window_destroy(m_window);
}
