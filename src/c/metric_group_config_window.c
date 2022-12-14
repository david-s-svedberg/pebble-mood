#include "metric_group_config_window.h"

#include <pebble.h>

#include "repositories/metrics_repository.h"
#include "repositories/app_config_repository.h"
#include "format.h"
#include "icons.h"
#include "edit_alarm_window.h"

static void update_ui();

static Window *m_config_window;
static StatusBarLayer* m_status_bar;
static SimpleMenuLayer* m_config_menu_layer = NULL;

static SimpleMenuSection m_menu_sections[2];
static SimpleMenuSection m_metrics_items_section;
static SimpleMenuSection m_metric_group_items_section;

static SimpleMenuItem m_metric_group_items[2];
static SimpleMenuItem m_title_item = {0};
static SimpleMenuItem m_alarm_item = {0};

static SimpleMenuItem* m_metric_items = NULL;
static uint16_t* m_metric_ids_index_map = NULL;

static DictationSession* m_dictation_session;

static MetricsGroup* m_metric_group;

static void update_alarm_item()
{
    static char alarm_text_buffer[6];
    fill_time_string(alarm_text_buffer, m_metric_group->alarm.time.hour, m_metric_group->alarm.time.minute);
    m_alarm_item.subtitle = alarm_text_buffer;
}

static void update_status_bar()
{
    status_bar_layer_set_colors(m_status_bar, config_get_background_color(), config_get_foreground_color());
}

static void toggle_connected_to_metrics_group(int index, void *context)
{
    uint16_t metrics_id = m_metric_ids_index_map[index];
    Metrics* current_metric = metrics_get(metrics_id);
    current_metric->group_id = m_metric_group->id;
    current_metric->group = m_metric_group;
    metrics_save();
    update_ui();
}

static void dictation_session_callback(
    DictationSession *session,
    DictationSessionStatus status,
    char *transcription,
    void *context)
{
    if(status == DictationSessionStatusSuccess)
    {
        metrics_groups_set_title(m_metric_group, transcription);
        update_ui();
    }
}

static void change_title(int index, void *context)
{
    dictation_session_start(m_dictation_session);
}

static void change_alarm(int index, void *context)
{
    setup_edit_alarm_window(m_metric_group);
}

static void free_dynamically_allocated_data()
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

static void update_metric_items()
{
    free_dynamically_allocated_data();
    size_t number_of_metrics = metrics_count();
    m_metric_items = (SimpleMenuItem*)malloc(number_of_metrics * sizeof(SimpleMenuItem));
    m_metric_ids_index_map = (uint16_t*)malloc(number_of_metrics * sizeof(uint16_t));
    Metrics* metrics = metrics_get_all();
    for(uint16_t i = 0; i < number_of_metrics; i++)
    {
        Metrics* current_metric = &metrics[i];
        SimpleMenuItem* metric_menu_item = &m_metric_items[i];
        m_metric_ids_index_map[i] = current_metric->id;
        if(current_metric->group_id == m_metric_group->id)
        {
            metric_menu_item->icon = config_is_dark_theme() ? get_check_icon_white() : get_check_icon_black();
        }
        metric_menu_item->title = current_metric->title->value;
        metric_menu_item->callback = toggle_connected_to_metrics_group;
    }
    m_metrics_items_section.items = m_metric_items;
    m_metrics_items_section.num_items = number_of_metrics;
}

static void update_metric_group_items()
{
    if(m_metric_group->title != NULL)
    {
        m_title_item.subtitle = m_metric_group->title->value;
    }
    update_alarm_item();
}

static void update_ui()
{
    layer_mark_dirty(simple_menu_layer_get_layer(m_config_menu_layer));
    update_status_bar();
}

static void setup_config_menu(Layer *window_layer, GRect bounds)
{
    m_title_item.title = "Title";
    m_title_item.callback = change_title;

    m_alarm_item.title = "Registration time";
    m_alarm_item.callback = change_alarm;

    m_metric_group_items[0] = m_title_item;
    m_metric_group_items[1] = m_alarm_item;

    m_metric_group_items_section.num_items = 2;
    m_metric_group_items_section.items = m_metric_group_items;
    m_metrics_items_section.items = m_metric_items;

    m_metric_group_items_section.title = "Metrics Group";
    m_metrics_items_section.title = "Metrics";

    update_metric_group_items();
    update_metric_items();

    m_menu_sections[0] = m_metric_group_items_section;
    m_menu_sections[1] = m_metrics_items_section;

    m_config_menu_layer = simple_menu_layer_create(
        GRect(0, STATUS_BAR_LAYER_HEIGHT, bounds.size.w, bounds.size.h - STATUS_BAR_LAYER_HEIGHT),
        m_config_window,
        m_menu_sections, 2, NULL);

    layer_add_child(window_layer, simple_menu_layer_get_layer(m_config_menu_layer));
}

static void setup_status_bar(Layer *window_layer, GRect bounds)
{
    m_status_bar = status_bar_layer_create();

    status_bar_layer_set_separator_mode(m_status_bar, StatusBarLayerSeparatorModeDotted);

    layer_add_child(window_layer, status_bar_layer_get_layer(m_status_bar));
}

static void load_metric_group_window(Window *window)
{
    window_set_background_color(window, config_get_background_color());

    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    setup_status_bar(window_layer, bounds);
    setup_config_menu(window_layer, bounds);

    m_dictation_session = dictation_session_create(
        512,
        dictation_session_callback,
        NULL);
}

static void unload_metric_group_window(Window *window)
{
    status_bar_layer_destroy(m_status_bar);
    simple_menu_layer_destroy(m_config_menu_layer);
    dictation_session_destroy(m_dictation_session);
    free_dynamically_allocated_data();
}

static void appear_metric_group_windows(Window *window)
{
    update_metric_group_items();
    update_metric_items();
    update_ui();
}

void setup_metric_group_config_window(MetricsGroup* metrics_group)
{
    m_metric_group = metrics_group;
    m_config_window = window_create();

    window_set_window_handlers(m_config_window, (WindowHandlers) {
        .load = load_metric_group_window,
        .unload = unload_metric_group_window,
        .appear = appear_metric_group_windows,
    });

    window_stack_push(m_config_window, true);
}

void tear_down_metric_group_config_window()
{
    window_destroy(m_config_window);
}
