#include "metrics_config_window.h"

#include <pebble.h>

#include "repositories/metrics_repository.h"
#include "repositories/app_config_repository.h"
#include "format.h"
#include "icons.h"
#include "edit_alarm_window.h"
#include "metric_group_config_window.h"
#include "metrics_config_window.h"
#include "metric_group_config_window.h"

static Window *m_config_window;
static StatusBarLayer* m_status_bar;
static SimpleMenuLayer* m_config_menu_layer = NULL;

static SimpleMenuSection m_menu_sections[3];
static SimpleMenuSection m_metrics_section = {0};
static SimpleMenuSection m_metrics_groups_section = {0};
static SimpleMenuSection m_app_config_section = {0};

static SimpleMenuItem m_app_config_items[2];
static SimpleMenuItem m_alarm_timeout_item = {0};
static SimpleMenuItem m_theme_item = {0};

static SimpleMenuItem* m_metric_items = NULL;
static SimpleMenuItem* m_metrics_group_items = NULL;

static uint16_t* m_metric_ids_index_map = NULL;
static uint16_t* m_metrics_group_id_index_map = NULL;

static void update_status_bar()
{
    status_bar_layer_set_colors(m_status_bar, config_get_background_color(), config_get_foreground_color());
}

static void config_metrics_group(int index, void *context)
{
    MetricsGroup* metrics_group = metrics_group_get(m_metrics_group_id_index_map[index]);
    setup_metric_group_config_window(metrics_group);
}

static void config_metrics(int index, void *context)
{
    Metrics* metrics = metrics_get(m_metric_ids_index_map[index]);
    setup_metrics_config_window(metrics);
}

static void add_metrics(int index, void *context)
{
    setup_metrics_config_window(metrics_new());
}

static void add_metrics_group(int index, void *context)
{
    setup_metric_group_config_window(metrics_group_new());
}

static void tick_alarm_timeout(int index, void *context)
{
    // static uint16_t timeouts[] = {SECONDS_PER_MINUTE, 2 * SECONDS_PER_MINUTE, 3 * SECONDS_PER_MINUTE};
    // setup_metric_group_config_window(metrics_group_new());
}

static void free_dynamic_metric_data()
{
    if(m_metric_items != NULL)
    {
        free(m_metric_items);
    }
    if(m_metric_ids_index_map != NULL)
    {
        free(m_metric_ids_index_map);
    }
}

static char* get_current_theme()
{
    static char theme_buffer[6];

    char* theme = config_is_dark_theme() ? "Dark" : "Light";
    strncpy(theme_buffer, theme, 6);

    return theme_buffer;
}

static void update_app_config_items()
{
    static char timeout_buffer[2];
    snprintf(timeout_buffer, 1, "%d", config_get_alarm_timeout() / 60);
    m_alarm_timeout_item.subtitle = timeout_buffer;

    m_theme_item.subtitle = get_current_theme();
}

static void update_metric_items()
{
    free_dynamic_metric_data();

    size_t number_of_metrics = metrics_count();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "number_of_metrics:%d", number_of_metrics);
    size_t metric_items_size = (number_of_metrics + 1) * sizeof(SimpleMenuItem);
    m_metric_items = (SimpleMenuItem*)malloc(metric_items_size);
    memset(m_metric_items, 0, metric_items_size);
    m_metric_ids_index_map = (uint16_t*)malloc(number_of_metrics * sizeof(uint16_t));

    Metrics* metrics = metrics_get_all();

    for(uint16_t i = 0; i < number_of_metrics; i++)
    {
        Metrics* current_metrics = &metrics[i];
        SimpleMenuItem* metric_menu_item = &m_metric_items[i];
        m_metric_ids_index_map[i] = current_metrics->id;
        metric_menu_item->title = current_metrics->title->value;
        metric_menu_item->callback = config_metrics;
    }
    SimpleMenuItem* add_item = &m_metric_items[number_of_metrics];
    add_item->title = "+";
    add_item->callback = add_metrics;
    m_metrics_section.items = m_metric_items;
    m_metrics_section.num_items = number_of_metrics + 1;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "m_metrics_section.num_items:%d", (int)m_metrics_section.num_items);
}

static void free_dynamic_metric_group_data()
{
    if(m_metrics_group_items != NULL)
    {
        free(m_metrics_group_items);
    }
    if(m_metrics_group_id_index_map != NULL)
    {
        free(m_metrics_group_id_index_map);
    }
}

static void free_dynamically_allocated_data()
{
    free_dynamic_metric_data();
    free_dynamic_metric_group_data();
}

static void update_metric_groups_items()
{
    free_dynamic_metric_group_data();

    size_t number_of_metrics_groups = metrics_groups_count();
    size_t metric_group_items_size = (number_of_metrics_groups + 1) * sizeof(SimpleMenuItem);
    m_metrics_group_items = (SimpleMenuItem*)malloc(metric_group_items_size);
    memset(m_metrics_group_items, 0, metric_group_items_size);
    m_metrics_group_id_index_map = (uint16_t*)malloc(number_of_metrics_groups * sizeof(uint16_t));

    MetricsGroup* metrics_groups = metrics_groups_get_all();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "metrics_groups:%d", (int)metrics_groups);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Number of metric groups:%d", (int)number_of_metrics_groups);
    for(uint16_t i = 0; i < number_of_metrics_groups; i++)
    {
        MetricsGroup* current_metrics_group = &metrics_groups[i];
        SimpleMenuItem* metrics_group_menu_item = &m_metrics_group_items[i];
        APP_LOG(APP_LOG_LEVEL_DEBUG, "current_metrics_group:%d", (int)current_metrics_group);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "metrics_group_menu_item:%d", (int)metrics_group_menu_item);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "current_metrics_group->id:%d", current_metrics_group->id);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "current_metrics_group->title*:%d", (int)current_metrics_group->title);
        m_metrics_group_id_index_map[i] = current_metrics_group->id;
        metrics_group_menu_item->title = current_metrics_group->title->value;
        metrics_group_menu_item->callback = config_metrics_group;
    }
    SimpleMenuItem* add_item = &m_metrics_group_items[number_of_metrics_groups];
    add_item->title = "+";
    add_item->callback = add_metrics_group;

    m_metrics_groups_section.num_items = number_of_metrics_groups + 1;
    m_metrics_groups_section.items = m_metrics_group_items;

    APP_LOG(APP_LOG_LEVEL_DEBUG, "m_metrics_groups_section.num_items:%d", (int)m_metrics_groups_section.num_items);
}

static void update_ui()
{
    update_status_bar();
    layer_mark_dirty(simple_menu_layer_get_layer(m_config_menu_layer));
}

static void setup_config_menu(Layer *window_layer, GRect bounds)
{
    m_alarm_timeout_item.title = "Alarm timeout";
    m_alarm_timeout_item.callback = tick_alarm_timeout;

    m_theme_item.title = "Theme";
    m_theme_item.callback = NULL;

    m_app_config_items[0] = m_alarm_timeout_item;
    m_app_config_items[1] = m_theme_item;

    m_app_config_section.items = m_app_config_items;
    m_app_config_section.num_items = 2;
    m_app_config_section.title = "App Config";

    m_metrics_section.title = "Metrics";
    m_metrics_groups_section.title = "Metric Groups";

    update_metric_groups_items();
    update_metric_items();
    update_app_config_items();

    m_menu_sections[0] = m_metrics_groups_section;
    m_menu_sections[1] = m_metrics_section;
    m_menu_sections[2] = m_app_config_section;

    m_config_menu_layer = simple_menu_layer_create(
        GRect(0, STATUS_BAR_LAYER_HEIGHT, bounds.size.w, bounds.size.h - STATUS_BAR_LAYER_HEIGHT),
        m_config_window,
        m_menu_sections, 3, NULL);

    layer_add_child(window_layer, simple_menu_layer_get_layer(m_config_menu_layer));
}

static void setup_status_bar(Layer *window_layer, GRect bounds)
{
    m_status_bar = status_bar_layer_create();

    status_bar_layer_set_separator_mode(m_status_bar, StatusBarLayerSeparatorModeDotted);

    layer_add_child(window_layer, status_bar_layer_get_layer(m_status_bar));
}

static void load_config_window(Window *window)
{
    window_set_background_color(window, config_get_background_color());
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    setup_status_bar(window_layer, bounds);
    setup_config_menu(window_layer, bounds);
}

static void unload_config_window(Window *window)
{
    status_bar_layer_destroy(m_status_bar);
    simple_menu_layer_destroy(m_config_menu_layer);
    free_dynamically_allocated_data();
}

static void appear_metrics_config_windows(Window *window)
{
    update_metric_groups_items();
    update_metric_items();
    update_app_config_items();
    update_ui();
}

void setup_config_window()
{
    m_config_window = window_create();

    window_set_window_handlers(m_config_window, (WindowHandlers) {
        .load = load_config_window,
        .unload = unload_config_window,
        .appear = appear_metrics_config_windows,
    });

    window_stack_push(m_config_window, true);
}

void tear_down_config_window()
{
    if(m_metric_items != NULL)
    {
        free(m_metric_items);
    }
    window_destroy(m_config_window);
}
