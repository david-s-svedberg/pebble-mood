#include "metrics_config_window.h"

#include <pebble.h>

#include "metrics_repository.h"
#include "format.h"
#include "icons.h"
#include "edit_alarm_window.h"

static Window *m_config_window;

static StatusBarLayer* m_status_bar;

static SimpleMenuLayer* m_config_menu_layer = NULL;
static SimpleMenuSection m_menu_sections[2];
static SimpleMenuSection m_metrics_items;
static SimpleMenuSection m_metrics_items_section;
static SimpleMenuItem m_metrics_items[3];
static SimpleMenuItem m_title_item =
{
    .title = "Title"
};
static SimpleMenuItem m_type_item =
{
    .title = "Type"
};
static SimpleMenuItem m_max_value_item =
{
    .title = "Max"
};
static SimpleMenuItem* m_metrics_items = NULL;
static uint16_t* m_metrics_id_index_map = NULL;

static DictationSession* m_dictation_session;

static Metrics* m_metrics;

static void update_status_bar()
{
    status_bar_layer_set_colors(m_status_bar, get_background_color(), get_foreground_color());
}

static void toggle_connected_to_metrics_group(int index, void *context)
{
    uint16_t metrics_group_id = m_metrics_id_index_map[index];
    MetricsGroup* current_metrics_group = metrics_groups_get(metrics_group_id);
    m_metrics->group_id = current_metrics_group->id;
    m_metrics->group = current_metrics_group;
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
        metrics_set_title(m_metrics, transcription);
        update_ui();
    }
}

static void change_title(int index, void *context)
{
    dictation_session_start(m_dictation_session);
}

static void change_type(int index, void *context)
{
    m_metrics->type = m_metrics->type == MetricsType_BOOL ? MetricsType_INTERVAL : MetricsType_BOOL;
    metrics_save();
    update_ui();
}

static void change_max_value(int index, void *context)
{
    m_metrics->max_value++;
    if(m_metrics->max_value > 10)
    {
        m_metrics->max_value = 2;
    }
    metrics_save();
    update_ui();
}

static void update_metrics_items()
{
    m_title_item.subtitle = m_metrics->title->value;
    m_type_item.subtitle = m_metrics->type == MetricsType_BOOL ? "Yes/No" : "Interval";
    static char max_value[2];
    snprintf(max_value, 2, "%d", m_metrics->max_value);
    m_max_value_item.subtitle = max_value;
    m_metrics_items.num_items = m_metrics->type == MetricsType_INTERVAL ? 3 : 2;
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

    size_t number_of_metrics_groups = metrics_group_count();
    m_metrics_items = (SimpleMenuItem*)malloc(number_of_metrics_groups * sizeof(SimpleMenuItem));
    m_metrics_id_index_map = (uint16_t*)malloc(number_of_metrics_groups * sizeof(uint16_t));

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
        metrics_group_menu_item->callback = toggle_connected_to_metrics_group;
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
    m_metrics_items.title = "Metrics";
    m_metrics_items_section.title = "Metrics Groups";

    m_title_item.callback = change_title;
    m_type_item.callback = change_type;
    m_max_value_item.callback = change_max_value;

    m_metrics_items[0] = m_title_item;
    m_metrics_items[1] = m_type_item;
    m_metrics_items[2] = m_max_value_item;

    m_metrics_items.items = m_metrics_items;

    m_menu_sections[0] = m_metrics_items;
    m_menu_sections[1] = m_metrics_items_section;

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

    m_dictation_session = dictation_session_create(
        512,
        dictation_session_callback,
        NULL);
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

void void setup_metrics_config_window(Metrics* metrics)
{
    m_metrics = metrics;
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
