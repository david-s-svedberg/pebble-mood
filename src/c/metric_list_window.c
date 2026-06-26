#include "metric_list_window.h"

#include <pebble.h>

#include "main_window_logic.h"   // main_window_format_average
#include "icons.h"
#include "menu_theme.h"
#include "register_mood_window.h"
#include "repositories/metrics_repository.h"
#include "repositories/app_config_repository.h"

#define SUBTITLE_LEN (24)

static Window* m_window;
static StatusBarLayer* m_status_bar;
static MenuLayer* m_menu_layer = NULL;
static MetricListMode m_mode;

// Per-row data, rebuilt on load/appear. A row with id 0 is the non-selectable
// "empty" placeholder.
static const char** m_titles = NULL;
static char (*m_subtitles)[SUBTITLE_LEN] = NULL;
static bool* m_check = NULL;
static uint16_t* m_id_map = NULL;
static uint16_t m_count = 0;

static void free_dynamic()
{
    if(m_titles != NULL) { free(m_titles); m_titles = NULL; }
    if(m_subtitles != NULL) { free(m_subtitles); m_subtitles = NULL; }
    if(m_check != NULL) { free(m_check); m_check = NULL; }
    if(m_id_map != NULL) { free(m_id_map); m_id_map = NULL; }
}

static void build_items()
{
    free_dynamic();

    uint32_t total = metrics_count();
    uint32_t cap = (total == 0) ? 1 : total;
    m_titles = (const char**)malloc(cap * sizeof(const char*));
    m_subtitles = malloc(cap * SUBTITLE_LEN);
    m_check = (bool*)malloc(cap * sizeof(bool));
    m_id_map = (uint16_t*)malloc(cap * sizeof(uint16_t));

    Metrics* metrics = metrics_get_all();
    uint16_t n = 0;
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
        m_titles[n] = metric->title != NULL ? metric->title->value : "";
        main_window_format_average(metric, m_subtitles[n], SUBTITLE_LEN);
        m_check[n] = (m_mode == MetricList_TODAY && done_today);
        n++;
    }

    if(n == 0)
    {
        m_id_map[0] = 0;
        m_titles[0] = (m_mode == MetricList_TODAY) ? "Nothing scheduled" : "No metrics yet";
        snprintf(m_subtitles[0], SUBTITLE_LEN, "%s",
            (m_mode == MetricList_TODAY) ? "" : "Add one in Settings");
        m_check[0] = false;
        n = 1;
    }

    m_count = n;
}

static uint16_t menu_get_num_rows(MenuLayer* menu_layer, uint16_t section, void* context)
{
    return m_count;
}

static int16_t menu_get_header_height(MenuLayer* menu_layer, uint16_t section, void* context)
{
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header(GContext* ctx, const Layer* cell_layer, uint16_t section, void* context)
{
    menu_cell_basic_header_draw(ctx, cell_layer,
        (m_mode == MetricList_TODAY) ? "Today" : "Register");
}

static void menu_draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* index, void* context)
{
    GBitmap* icon = NULL;
    if(m_check[index->row])
    {
        icon = menu_theme_icon_light(cell_layer) ? get_check_icon_white() : get_check_icon_black();
    }
    const char* subtitle = m_subtitles[index->row][0] != '\0' ? m_subtitles[index->row] : NULL;
    menu_cell_basic_draw(ctx, cell_layer, m_titles[index->row], subtitle, icon);
}

static void menu_select_click(MenuLayer* menu_layer, MenuIndex* index, void* context)
{
    uint16_t id = m_id_map[index->row];
    if(id == 0)
    {
        return;  // placeholder row
    }
    Metrics* metric = metrics_get(id);
    if(metric != NULL)
    {
        setup_register_mood_window_for_metric(metric);
    }
}

static void create_menu()
{
    Layer* window_layer = window_get_root_layer(m_window);
    GRect bounds = layer_get_bounds(window_layer);

    build_items();

    if(m_menu_layer == NULL)
    {
        m_menu_layer = menu_layer_create(GRect(0, STATUS_BAR_LAYER_HEIGHT,
            bounds.size.w, bounds.size.h - STATUS_BAR_LAYER_HEIGHT));
        menu_layer_set_callbacks(m_menu_layer, NULL, (MenuLayerCallbacks) {
            .get_num_rows = menu_get_num_rows,
            .get_header_height = menu_get_header_height,
            .draw_header = menu_draw_header,
            .draw_row = menu_draw_row,
            .select_click = menu_select_click,
        });
        menu_theme_apply_colors(m_menu_layer);
        menu_layer_set_click_config_onto_window(m_menu_layer, m_window);
        layer_add_child(window_layer, menu_layer_get_layer(m_menu_layer));
    } else
    {
        menu_layer_reload_data(m_menu_layer);
    }
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
    if(m_menu_layer != NULL)
    {
        create_menu();
    }
}

static void unload_window(Window* window)
{
    if(m_menu_layer != NULL)
    {
        menu_layer_destroy(m_menu_layer);
        m_menu_layer = NULL;
    }
    status_bar_layer_destroy(m_status_bar);
    free_dynamic();
}

void setup_metric_list_window(MetricListMode mode)
{
    m_mode = mode;
    if(m_window == NULL)
    {
        m_window = window_create();
        window_set_window_handlers(m_window, (WindowHandlers) {
            .load = load_window,
            .unload = unload_window,
            .appear = appear_window,
        });
    }
    window_stack_push(m_window, true);
}

void tear_down_metric_list_window()
{
    window_destroy(m_window);
}
