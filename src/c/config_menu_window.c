#include "metrics_config_window.h"

#include <pebble.h>

#include "metrics_repository.h"
#include "format.h"
#include "icons.h"
#include "edit_alarm_window.h"
#include "metric_group_config_window.h"
#include "metrics_config_window.h"

static Window *m_config_window;

static StatusBarLayer* m_status_bar;

static SimpleMenuLayer* m_config_menu_layer = NULL;
static SimpleMenuSection m_menu_sections[2];
static SimpleMenuSection m_metrics_section;
static SimpleMenuSection m_metrics_groups_section;

static SimpleMenuItem* m_metrics_items = NULL;
static SimpleMenuItem* m_metrics_group_items = NULL;

static uint16_t* m_metrics_id_index_map = NULL;
static uint16_t* m_metrics_group_id_index_map = NULL;

static void update_status_bar()
{
    status_bar_layer_set_colors(m_status_bar, get_background_color(), get_foreground_color());
}

static void config_metrics_group(int index, void *context)
{
    MetricsGroup* metrics_group = metrics_group_get(m_metrics_group_id_index_map[index]);
    setup_metric_group_config_window(metrics_group);
}

static void config_metrics(int index, void *context)
{
    Metrics* metrics = metrics_get(m_metrics_id_index_map[index]);
    setup_metrics_config_window(metrics);
}

static void update_metrics_items()
{
    if(m_metrics_items != NULL)
    {
        free(m_metrics_items);
    }
    if(m_metrics_id_index_map != NULL)
    {
        free(m_metrics_id_index_map);
    }

    size_t number_of_metrics = metrics_count();
    m_metrics_items = (SimpleMenuItem*)malloc(number_of_metrics * sizeof(SimpleMenuItem));
    m_metrics_id_index_map = (uint16_t*)malloc(number_of_metrics * sizeof(uint16_t));

    Metrics* metrics = metrics_get_all();

    for(int i = 0; i < number_of_metrics; i++)
    {
        Metrics* current_metrics = &metrics->items[i];
        SimpleMenuItem* metric_menu_item = &m_metrics_items[i];
        m_metrics_id_index_map[i] = current_metrics->id;
        if(current_metrics->group_id == m_metric_group->id)
        {
            metric_menu_item->icon = is_dark_theme() ? get_check_icon_white() : get_check_icon_black();
        }
        metric_menu_item->title = current_metrics->title->value;
        metric_menu_item->callback = config_metrics;
    }
    m_metrics_items_section.items = m_metrics_items;
    m_metrics_items_section.num_items = number_of_metrics;
}

static void update_metrics_groups_items()
{
    if(m_metrics_group_items != NULL)
    {
        free(m_metrics_group_items);
    }
    if(m_metrics_group_id_index_map != NULL)
    {
        free(m_metrics_group_id_index_map);
    }

    size_t number_of_metrics_groups = metrics_group_count();
    m_metrics_group_items = (SimpleMenuItem*)malloc(number_of_metrics_groups * sizeof(SimpleMenuItem));
    m_metrics_group_id_index_map = (uint16_t*)malloc(number_of_metrics_groups * sizeof(uint16_t));

    MetricsGroup* metrics_groups = metrics_groups_get_all();

    for(int i = 0; i < number_of_metrics_groups; i++)
    {
        MetricsGroup* current_metrics_group = &metrics_groups->items[i];
        SimpleMenuItem* metrics_group_menu_item = &m_metrics_items[i];
        m_metrics_id_index_map[i] = current_metrics_group->id;
        if(current_metrics_group->id == m_metrics->group_id)
        {
            metrics_group_menu_item->icon = is_dark_theme() ? get_check_icon_white() : get_check_icon_black();
        }
        metrics_group_menu_item->title = current_metrics_group->title->value;
        metrics_group_menu_item->callback = config_metrics_group;
    }
    m_metrics_items_section.items = m_metrics_items;
    m_metrics_items_section.num_items = number_of_metrics_groups;
}

static void update_ui()
{
    update_metrics_items();
    update_metrics_items();
    layer_mark_dirty(simple_menu_layer_get_layer(m_config_menu_layer));
    update_status_bar();
}

static void setup_config_menu(Layer *window_layer, GRect bounds)
{
    m_metrics_groups_section.title = "Metrics Groups";
    m_metrics_section.title = "Metrics";

    m_menu_sections[0] = m_metrics_groups_section;
    m_menu_sections[1] = m_metrics_section;

    m_config_menu_layer = simple_menu_layer_create(
        GRect(0, STATUS_BAR_LAYER_HEIGHT, bounds.size.w, bounds.size.h - STATUS_BAR_LAYER_HEIGHT),
        m_config_window,
        &m_menu_sections, 2, NULL);

    layer_add_child(window_layer, simple_menu_layer_get_layer(m_config_menu_layer));
}

static void setup_status_bar(Layer *window_layer, GRect bounds)
{
    m_status_bar = status_bar_layer_create();

    status_bar_layer_set_separator_mode(m_status_bar, StatusBarLayerSeparatorModeDotted);

    layer_add_child(window_layer, status_bar_layer_get_layer(m_status_bar));
}

static void load_main_window(Window *window)
{
    window_set_background_color(window, get_background_color());
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    setup_status_bar(window_layer, bounds);
    setup_config_menu(window_layer, bounds);
}

static void unload_main_window(Window *window)
{
    status_bar_layer_destroy(m_status_bar);
    simple_menu_layer_destroy(m_config_menu_layer);
    dictation_session_destroy(m_dictation_session);
}

static void appear_config_windows(Window *window)
{
    update_ui();
}

void void setup_config_window()
{
    m_config_window = window_create();

    window_set_window_handlers(m_config_window, (WindowHandlers) {
        .load = load_main_window,
        .unload = unload_main_window,
        .appear = appear_config_windows,
    });

    window_stack_push(m_config_window, true);
}

void tear_down_metrics_config_window()
{
    if(m_metrics_items != NULL)
    {
        free(m_metrics_items);
    }
    window_destroy(m_config_window);
}
