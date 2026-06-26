#include "metric_list_window.h"

#include <pebble.h>

#include "main_window_logic.h"   // main_window_format_average
#include "register_mood_window.h"
#include "repositories/metrics_repository.h"
#include "repositories/app_config_repository.h"

#define SUBTITLE_LEN (24)

static Window* m_window;
static StatusBarLayer* m_status_bar;
static SimpleMenuLayer* m_menu_layer = NULL;

static SimpleMenuSection m_section = { .title = "Register" };
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

    uint32_t count = metrics_count();
    uint32_t item_count = (count == 0) ? 1 : count;

    m_items = (SimpleMenuItem*)malloc(item_count * sizeof(SimpleMenuItem));
    memset(m_items, 0, item_count * sizeof(SimpleMenuItem));
    m_subtitles = malloc(item_count * SUBTITLE_LEN);
    m_id_map = (uint16_t*)malloc(item_count * sizeof(uint16_t));

    if(count == 0)
    {
        m_items[0].title = "No metrics yet";
        m_items[0].subtitle = "Add one in Settings";
    } else
    {
        Metrics* metrics = metrics_get_all();
        for(uint32_t i = 0; i < count; i++)
        {
            Metrics* metric = &metrics[i];
            m_id_map[i] = metric->id;
            main_window_format_average(metric, m_subtitles[i], SUBTITLE_LEN);
            m_items[i].title = metric->title != NULL ? metric->title->value : "";
            m_items[i].subtitle = m_subtitles[i];
            m_items[i].callback = register_selected;
        }
    }

    m_section.items = m_items;
    m_section.num_items = item_count;
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
    // Refresh averages after returning from a registration.
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

void setup_metric_list_window()
{
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
