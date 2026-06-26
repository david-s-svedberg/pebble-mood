#include "metric_group_config_window.h"

#include <pebble.h>

#include "repositories/metrics_repository.h"
#include "repositories/app_config_repository.h"
#include "format.h"
#include "icons.h"
#include "edit_alarm_window.h"

static Window *m_config_window;
static StatusBarLayer* m_status_bar;
static SimpleMenuLayer* m_config_menu_layer = NULL;

static SimpleMenuSection m_menu_sections[2];
static SimpleMenuSection m_group_section = { .title = "Metric group" };
static SimpleMenuSection m_metrics_section = { .title = "Metrics in group" };

static SimpleMenuItem m_group_items[2];
static char m_alarm_text_buffer[6];

static SimpleMenuItem* m_metric_items = NULL;     // one per metric (toggle membership)
static uint16_t* m_metric_ids_index_map = NULL;

static DictationSession* m_dictation_session;
static MetricsGroup* m_metric_group;

static void create_menu();

static void update_status_bar()
{
    status_bar_layer_set_colors(m_status_bar, config_get_background_color(), config_get_foreground_color());
}

static GBitmap* member_check_icon()
{
    return config_is_dark_theme() ? get_check_icon_white() : get_check_icon_black();
}

static void change_title(int index, void *context)
{
    dictation_session_start(m_dictation_session);
}

static void change_alarm(int index, void *context)
{
    setup_edit_alarm_window(m_metric_group);
}

static void toggle_metric_in_group(int index, void *context)
{
    uint16_t metric_id = m_metric_ids_index_map[index];
    metrics_group_toggle_metric(m_metric_group->id, metric_id);
    // Update just this row's checkmark so the cursor stays put.
    bool member = metrics_group_has_metric(m_metric_group->id, metric_id);
    m_metric_items[index].icon = member ? member_check_icon() : NULL;
    layer_mark_dirty(simple_menu_layer_get_layer(m_config_menu_layer));
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
        create_menu();
    }
}

static void free_metric_items()
{
    if(m_metric_items != NULL)
    {
        free(m_metric_items);
        m_metric_items = NULL;
    }
    if(m_metric_ids_index_map != NULL)
    {
        free(m_metric_ids_index_map);
        m_metric_ids_index_map = NULL;
    }
}

static void build_group_section()
{
    m_group_items[0] = (SimpleMenuItem){ .title = "Title", .callback = change_title };
    if(m_metric_group->title != NULL)
    {
        m_group_items[0].subtitle = m_metric_group->title->value;
    }

    fill_time_string(m_alarm_text_buffer, m_metric_group->alarm.time.hour, m_metric_group->alarm.time.minute);
    m_group_items[1] = (SimpleMenuItem){
        .title = "Registration time",
        .subtitle = m_alarm_text_buffer,
        .callback = change_alarm,
    };

    m_group_section.items = m_group_items;
    m_group_section.num_items = 2;
}

static void build_metrics_section()
{
    free_metric_items();

    size_t count = metrics_count();
    size_t items_size = count * sizeof(SimpleMenuItem);
    m_metric_items = (SimpleMenuItem*)malloc(items_size);
    memset(m_metric_items, 0, items_size);
    m_metric_ids_index_map = (uint16_t*)malloc(count * sizeof(uint16_t));

    Metrics* metrics = metrics_get_all();
    for(uint16_t i = 0; i < count; i++)
    {
        Metrics* metric = &metrics[i];
        m_metric_ids_index_map[i] = metric->id;
        m_metric_items[i].title = metric->title != NULL ? metric->title->value : "";
        m_metric_items[i].callback = toggle_metric_in_group;
        if(metrics_group_has_metric(m_metric_group->id, metric->id))
        {
            m_metric_items[i].icon = member_check_icon();
        }
    }

    m_metrics_section.items = m_metric_items;
    m_metrics_section.num_items = count;
}

static void create_menu()
{
    Layer* window_layer = window_get_root_layer(m_config_window);
    GRect bounds = layer_get_bounds(window_layer);

    if(m_config_menu_layer != NULL)
    {
        simple_menu_layer_destroy(m_config_menu_layer);
        m_config_menu_layer = NULL;
    }

    build_group_section();
    build_metrics_section();

    m_menu_sections[0] = m_group_section;
    m_menu_sections[1] = m_metrics_section;

    m_config_menu_layer = simple_menu_layer_create(
        GRect(0, STATUS_BAR_LAYER_HEIGHT, bounds.size.w, bounds.size.h - STATUS_BAR_LAYER_HEIGHT),
        m_config_window,
        m_menu_sections, 2, NULL);

    layer_add_child(window_layer, simple_menu_layer_get_layer(m_config_menu_layer));
    update_status_bar();
}

static void setup_status_bar(Layer *window_layer)
{
    m_status_bar = status_bar_layer_create();
    status_bar_layer_set_separator_mode(m_status_bar, StatusBarLayerSeparatorModeDotted);
    layer_add_child(window_layer, status_bar_layer_get_layer(m_status_bar));
}

static void load_metric_group_window(Window *window)
{
    window_set_background_color(window, config_get_background_color());
    setup_status_bar(window_get_root_layer(window));
    create_menu();

    m_dictation_session = dictation_session_create(512, dictation_session_callback, NULL);
}

static void unload_metric_group_window(Window *window)
{
    status_bar_layer_destroy(m_status_bar);
    simple_menu_layer_destroy(m_config_menu_layer);
    m_config_menu_layer = NULL;
    dictation_session_destroy(m_dictation_session);
    free_metric_items();
}

static void appear_metric_group_window(Window *window)
{
    // Refresh after returning from the alarm editor (time may have changed).
    create_menu();
}

void setup_metric_group_config_window(MetricsGroup* metrics_group)
{
    m_metric_group = metrics_group;
    m_config_window = window_create();

    window_set_window_handlers(m_config_window, (WindowHandlers) {
        .load = load_metric_group_window,
        .unload = unload_metric_group_window,
        .appear = appear_metric_group_window,
    });

    window_stack_push(m_config_window, true);
}

void tear_down_metric_group_config_window()
{
    free_metric_items();
    window_destroy(m_config_window);
}
