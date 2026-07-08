#include "metric_group_config_window.h"

#include <pebble.h>

#include "repositories/metrics_repository.h"
#include "repositories/app_config_repository.h"
#include "format.h"
#include "icons.h"
#include "menu_theme.h"
#include "edit_alarm_window.h"

#define SECTION_GROUP (0)
#define SECTION_METRICS (1)

static Window *m_config_window;
static StatusBarLayer* m_status_bar;
static MenuLayer* m_config_menu_layer = NULL;

static char m_alarm_text_buffer[6];

static DictationSession* m_dictation_session;
static MetricsGroup* m_metric_group;
// Two-tap delete: the first tap arms (relabels the row), the second deletes.
// Touching any other row disarms — so a stray scroll+select can't wipe a group.
static bool m_delete_armed = false;

static void update_status_bar()
{
    status_bar_layer_set_colors(m_status_bar, config_get_background_color(), config_get_foreground_color());
}

static void refresh_alarm_text()
{
    fill_time_string(m_alarm_text_buffer, m_metric_group->alarm.time.hour, m_metric_group->alarm.time.minute);
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
        menu_layer_reload_data(m_config_menu_layer);
    }
}

// --- MenuLayer callbacks ---------------------------------------------------

static uint16_t menu_get_num_sections(MenuLayer* menu_layer, void* context)
{
    return 2;
}

static uint16_t menu_get_num_rows(MenuLayer* menu_layer, uint16_t section, void* context)
{
    return (section == SECTION_GROUP) ? 3 : (uint16_t)metrics_count();
}

static int16_t menu_get_header_height(MenuLayer* menu_layer, uint16_t section, void* context)
{
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header(GContext* ctx, const Layer* cell_layer, uint16_t section, void* context)
{
    menu_cell_basic_header_draw(ctx, cell_layer,
        section == SECTION_GROUP ? "Metric group" : "Metrics in group");
}

static void menu_draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* index, void* context)
{
    if(index->section == SECTION_GROUP)
    {
        if(index->row == 0)
        {
            const char* title = (m_metric_group->title != NULL) ? m_metric_group->title->value : NULL;
            menu_cell_basic_draw(ctx, cell_layer, "Title", title, NULL);
        } else if(index->row == 1)
        {
            menu_cell_basic_draw(ctx, cell_layer, "Registration time", m_alarm_text_buffer, NULL);
        } else
        {
            menu_cell_basic_draw(ctx, cell_layer,
                m_delete_armed ? "Tap again to delete" : "Delete group", NULL, NULL);
        }
    } else
    {
        Metrics* metrics = metrics_get_all();
        Metrics* metric = &metrics[index->row];
        bool member = metrics_group_has_metric(m_metric_group->id, metric->id);
        GBitmap* icon = NULL;
        if(member)
        {
            icon = menu_theme_icon_light(cell_layer) ? get_check_icon_white() : get_check_icon_black();
        }
        const char* title = metric->title != NULL ? metric->title->value : "";
        menu_cell_basic_draw(ctx, cell_layer, title, NULL, icon);
    }
}

static void menu_select_click(MenuLayer* menu_layer, MenuIndex* index, void* context)
{
    if(index->section == SECTION_GROUP)
    {
        if(index->row == 0)
        {
            m_delete_armed = false;
            dictation_session_start(m_dictation_session);
        } else if(index->row == 1)
        {
            m_delete_armed = false;
            setup_edit_alarm_window(m_metric_group);
        } else
        {
            if(m_delete_armed)
            {
                uint16_t id = m_metric_group->id;
                metrics_group_delete(id);   // m_metric_group dangles after this
                window_stack_pop(true);
            } else
            {
                m_delete_armed = true;
                layer_mark_dirty(menu_layer_get_layer(m_config_menu_layer));
            }
        }
    } else
    {
        m_delete_armed = false;
        Metrics* metrics = metrics_get_all();
        metrics_group_toggle_metric(m_metric_group->id, metrics[index->row].id);
        layer_mark_dirty(menu_layer_get_layer(m_config_menu_layer));
    }
}

static void create_menu()
{
    Layer* window_layer = window_get_root_layer(m_config_window);
    GRect bounds = layer_get_bounds(window_layer);

    refresh_alarm_text();

    if(m_config_menu_layer == NULL)
    {
        m_config_menu_layer = menu_layer_create(GRect(0, STATUS_BAR_LAYER_HEIGHT,
            bounds.size.w, bounds.size.h - STATUS_BAR_LAYER_HEIGHT));
        menu_layer_set_callbacks(m_config_menu_layer, NULL, (MenuLayerCallbacks) {
            .get_num_sections = menu_get_num_sections,
            .get_num_rows = menu_get_num_rows,
            .get_header_height = menu_get_header_height,
            .draw_header = menu_draw_header,
            .draw_row = menu_draw_row,
            .select_click = menu_select_click,
        });
        menu_theme_apply_colors(m_config_menu_layer);
        menu_layer_set_click_config_onto_window(m_config_menu_layer, m_config_window);
        layer_add_child(window_layer, menu_layer_get_layer(m_config_menu_layer));
    } else
    {
        menu_layer_reload_data(m_config_menu_layer);
    }
    update_status_bar();
}

static void load_metric_group_window(Window *window)
{
    m_delete_armed = false;
    window_set_background_color(window, config_get_background_color());
    m_status_bar = status_bar_create_themed(window_get_root_layer(window));
    create_menu();

    m_dictation_session = dictation_session_create(512, dictation_session_callback, NULL);
}

static void unload_metric_group_window(Window *window)
{
    status_bar_layer_destroy(m_status_bar);
    menu_layer_destroy(m_config_menu_layer);
    m_config_menu_layer = NULL;
    dictation_session_destroy(m_dictation_session);
}

static void appear_metric_group_window(Window *window)
{
    // Refresh after returning from the alarm editor (time may have changed).
    m_delete_armed = false;
    if(m_config_menu_layer != NULL)
    {
        create_menu();
    }
}

void setup_metric_group_config_window(MetricsGroup* metrics_group)
{
    m_metric_group = metrics_group;
    if(m_config_window == NULL)
    {
        m_config_window = window_create();
        window_set_window_handlers(m_config_window, (WindowHandlers) {
            .load = load_metric_group_window,
            .unload = unload_metric_group_window,
            .appear = appear_metric_group_window,
        });
    }

    window_stack_push(m_config_window, true);
}

void tear_down_metric_group_config_window()
{
    window_destroy(m_config_window);
}
