#include "config_menu_window.h"

#include <pebble.h>

#include "repositories/metrics_repository.h"
#include "repositories/app_config_repository.h"
#include "format.h"
#include "icons.h"
#include "menu_theme.h"
#include "edit_alarm_window.h"
#include "metric_group_config_window.h"
#include "metrics_config_window.h"

#define SECTION_GROUPS (0)
#define SECTION_METRICS (1)
#define SECTION_APP (2)

static Window *m_config_window;
static StatusBarLayer* m_status_bar;
static MenuLayer* m_config_menu_layer = NULL;

static void update_status_bar()
{
    status_bar_layer_set_colors(m_status_bar, config_get_background_color(), config_get_foreground_color());
}

// --- MenuLayer callbacks ---------------------------------------------------

static uint16_t menu_get_num_sections(MenuLayer* menu_layer, void* context)
{
    return 3;
}

static uint16_t menu_get_num_rows(MenuLayer* menu_layer, uint16_t section, void* context)
{
    switch(section)
    {
        case SECTION_GROUPS:  return (uint16_t)metrics_groups_count() + 1;  // +1 for "+"
        case SECTION_METRICS: return (uint16_t)metrics_count() + 1;
        default:              return 2;  // Alarm timeout, Theme
    }
}

static int16_t menu_get_header_height(MenuLayer* menu_layer, uint16_t section, void* context)
{
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header(GContext* ctx, const Layer* cell_layer, uint16_t section, void* context)
{
    const char* title = (section == SECTION_GROUPS) ? "Metric Groups"
                      : (section == SECTION_METRICS) ? "Metrics" : "App Config";
    menu_cell_basic_header_draw(ctx, cell_layer, title);
}

static void menu_draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* index, void* context)
{
    if(index->section == SECTION_GROUPS)
    {
        size_t count = metrics_groups_count();
        if(index->row < count)
        {
            MetricsGroup* groups = metrics_groups_get_all();
            menu_cell_basic_draw(ctx, cell_layer, groups[index->row].title->value, NULL, NULL);
        } else
        {
            menu_cell_basic_draw(ctx, cell_layer, "+", NULL, NULL);
        }
    } else if(index->section == SECTION_METRICS)
    {
        size_t count = metrics_count();
        if(index->row < count)
        {
            Metrics* metrics = metrics_get_all();
            menu_cell_basic_draw(ctx, cell_layer, metrics[index->row].title->value, NULL, NULL);
        } else
        {
            menu_cell_basic_draw(ctx, cell_layer, "+", NULL, NULL);
        }
    } else
    {
        if(index->row == 0)
        {
            static char timeout_buffer[8];
            snprintf(timeout_buffer, sizeof(timeout_buffer), "%d min", config_get_alarm_timeout() / 60);
            menu_cell_basic_draw(ctx, cell_layer, "Alarm timeout", timeout_buffer, NULL);
        } else
        {
            menu_cell_basic_draw(ctx, cell_layer, "Theme",
                config_is_dark_theme() ? "Dark" : "Light", NULL);
        }
    }
}

// Re-applies the (possibly toggled) theme to this window in place. Windows
// deeper in the stack re-read the theme on load; the main window re-themes in
// its appear handler.
static void apply_theme()
{
    window_set_background_color(m_config_window, config_get_background_color());
    update_status_bar();
    menu_theme_apply_colors(m_config_menu_layer);
    layer_mark_dirty(menu_layer_get_layer(m_config_menu_layer));
}

static void cycle_alarm_timeout()
{
    // 1..4 minutes (alarm_timeout_sec is a uint8_t, so 4 min is the ceiling).
    uint16_t next = config_get_alarm_timeout() + SECONDS_PER_MINUTE;
    if(next > 4 * SECONDS_PER_MINUTE)
    {
        next = SECONDS_PER_MINUTE;
    }
    config_set_alarm_timeout(next);
}

static void menu_select_click(MenuLayer* menu_layer, MenuIndex* index, void* context)
{
    if(index->section == SECTION_GROUPS)
    {
        size_t count = metrics_groups_count();
        if(index->row < count)
        {
            MetricsGroup* groups = metrics_groups_get_all();
            setup_metric_group_config_window(metrics_group_get(groups[index->row].id));
        } else
        {
            setup_metric_group_config_window(metrics_group_new());
        }
    } else if(index->section == SECTION_METRICS)
    {
        size_t count = metrics_count();
        if(index->row < count)
        {
            Metrics* metrics = metrics_get_all();
            setup_metrics_config_window(metrics_get(metrics[index->row].id));
        } else
        {
            setup_metrics_config_window(metrics_new());
        }
    } else
    {
        if(index->row == 0)
        {
            cycle_alarm_timeout();
            layer_mark_dirty(menu_layer_get_layer(m_config_menu_layer));
        } else
        {
            config_toggle_theme();
            apply_theme();
        }
    }
}

static void create_menu(Layer* window_layer, GRect bounds)
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
}

static void setup_status_bar(Layer *window_layer)
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

    setup_status_bar(window_layer);
    create_menu(window_layer, bounds);
    update_status_bar();
}

static void unload_config_window(Window *window)
{
    status_bar_layer_destroy(m_status_bar);
    menu_layer_destroy(m_config_menu_layer);
    m_config_menu_layer = NULL;
}

static void appear_config_windows(Window *window)
{
    // Groups/metrics may have been added or edited; repaint with fresh data.
    if(m_config_menu_layer != NULL)
    {
        menu_layer_reload_data(m_config_menu_layer);
        update_status_bar();
    }
}

void setup_config_window()
{
    if(m_config_window == NULL)
    {
        m_config_window = window_create();
        window_set_window_handlers(m_config_window, (WindowHandlers) {
            .load = load_config_window,
            .unload = unload_config_window,
            .appear = appear_config_windows,
        });
    }

    window_stack_push(m_config_window, true);
}

void tear_down_config_window()
{
    window_destroy(m_config_window);
}
