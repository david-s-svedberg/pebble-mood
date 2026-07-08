#include "favorites_window.h"

#include <pebble.h>

#include "menu_theme.h"
#include "icons.h"
#include "repositories/metrics_repository.h"
#include "repositories/app_config_repository.h"

static Window* m_window;
static StatusBarLayer* m_status_bar;
static MenuLayer* m_menu_layer = NULL;

static uint16_t menu_get_num_sections(MenuLayer* menu_layer, void* context)
{
    return 1;
}

static uint16_t menu_get_num_rows(MenuLayer* menu_layer, uint16_t section, void* context)
{
    uint16_t count = (uint16_t)metrics_count();
    return count == 0 ? 1 : count;
}

static int16_t menu_get_header_height(MenuLayer* menu_layer, uint16_t section, void* context)
{
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header(GContext* ctx, const Layer* cell_layer, uint16_t section, void* context)
{
    static char header[24];
    snprintf(header, sizeof(header), "Home graph (%d/%d)", config_favorite_count(), MAX_FAVORITES);
    menu_cell_basic_header_draw(ctx, cell_layer, header);
}

static void menu_draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* index, void* context)
{
    if(metrics_count() == 0)
    {
        menu_cell_basic_draw(ctx, cell_layer, "No metrics yet", NULL, NULL);
        return;
    }
    Metrics* metric = &metrics_get_all()[index->row];
    const char* title = (metric->title != NULL) ? metric->title->value : "";
    bool fav = config_is_favorite(metric->id);
    // A check icon marks the chosen favourites; light/dark variant per row state.
    GBitmap* icon = fav
        ? get_icon_row_by_choice(IconChoice_CHECK, menu_theme_icon_light(cell_layer))
        : NULL;
    menu_cell_basic_draw(ctx, cell_layer, title, fav ? "On home screen" : NULL, icon);
}

static void menu_select_click(MenuLayer* menu_layer, MenuIndex* index, void* context)
{
    if(metrics_count() == 0) return;
    Metrics* metric = &metrics_get_all()[index->row];
    if(!config_toggle_favorite(metric->id))
    {
        // Full: briefly nudge so the "can't add" is felt.
        vibes_short_pulse();
    }
    menu_layer_reload_data(m_menu_layer);
}

static void create_menu(Layer* window_layer, GRect bounds)
{
    m_menu_layer = menu_layer_create(GRect(0, STATUS_BAR_LAYER_HEIGHT,
        bounds.size.w, bounds.size.h - STATUS_BAR_LAYER_HEIGHT));
    menu_layer_set_callbacks(m_menu_layer, NULL, (MenuLayerCallbacks) {
        .get_num_sections = menu_get_num_sections,
        .get_num_rows = menu_get_num_rows,
        .get_header_height = menu_get_header_height,
        .draw_header = menu_draw_header,
        .draw_row = menu_draw_row,
        .select_click = menu_select_click,
    });
    menu_theme_apply_colors(m_menu_layer);
    menu_layer_set_click_config_onto_window(m_menu_layer, m_window);
    layer_add_child(window_layer, menu_layer_get_layer(m_menu_layer));
}

static void load_window(Window* window)
{
    window_set_background_color(window, config_get_background_color());
    Layer* window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    m_status_bar = status_bar_create_themed(window_layer);
    create_menu(window_layer, bounds);
}

static void unload_window(Window* window)
{
    status_bar_layer_destroy(m_status_bar);
    menu_layer_destroy(m_menu_layer);
    m_menu_layer = NULL;
}

void setup_favorites_window()
{
    if(m_window == NULL)
    {
        m_window = window_create();
        window_set_window_handlers(m_window, (WindowHandlers) {
            .load = load_window,
            .unload = unload_window,
        });
    }
    window_stack_push(m_window, true);
}

void tear_down_favorites_window()
{
    window_destroy(m_window);
}
