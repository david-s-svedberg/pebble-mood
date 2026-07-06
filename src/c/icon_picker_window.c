#include "icon_picker_window.h"

#include "icons.h"
#include "repositories/app_config_repository.h"

static Window* m_window = NULL;
static StatusBarLayer* m_status_bar = NULL;
static MenuLayer* m_menu_layer = NULL;

static uint8_t m_choices[IconChoice_COUNT];
static uint16_t m_choice_count = 0;
static bool m_small_only = false;
static uint8_t m_current_choice = 0;   // pre-selected row, applied in .load
static IconPickerCallback m_callback = NULL;
static void* m_context = NULL;

static void build_choices(uint8_t current)
{
    m_choice_count = 0;
    m_choices[m_choice_count++] = IconChoice_NONE;  // first row clears the icon
    for(uint8_t choice = IconChoice_NONE + 1; choice < IconChoice_COUNT; choice++)
    {
        if(m_small_only && !icon_choice_is_small(choice))
        {
            continue;
        }
        m_choices[m_choice_count++] = choice;
    }
}

static uint16_t row_for_choice(uint8_t choice)
{
    for(uint16_t i = 0; i < m_choice_count; i++)
    {
        if(m_choices[i] == choice)
        {
            return i;
        }
    }
    return 0;
}

static uint16_t get_num_rows(MenuLayer* menu_layer, uint16_t section, void* context)
{
    return m_choice_count;
}

static int16_t get_cell_height(MenuLayer* menu_layer, MenuIndex* index, void* context)
{
    // Sized by category rather than by loading the bitmap, so MenuLayer's
    // measure-every-row pass doesn't pull all ~28 icons into the (tiny, on
    // aplite) heap at once — only the visible rows' icons load, in draw_row.
    uint8_t choice = m_choices[index->row];
    if(choice == IconChoice_NONE || icon_choice_is_small(choice))
    {
        return 36;  // 20px option icons + padding
    }
    return 60;      // 48px main icons + padding
}

static void draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* index, void* context)
{
    uint8_t choice = m_choices[index->row];
    bool highlighted = menu_cell_layer_is_highlighted(cell_layer);

    // The icon must contrast with the cell's background: on a highlighted (or
    // dark-theme) cell we draw the white variant, otherwise the black one.
    bool light_icon = highlighted ? !config_is_dark_theme() : config_is_dark_theme();
    GColor bg = highlighted ? config_get_foreground_color() : config_get_background_color();
    GColor fg = highlighted ? config_get_background_color() : config_get_foreground_color();

    GRect bounds = layer_get_bounds(cell_layer);
    graphics_context_set_fill_color(ctx, bg);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    int16_t text_x = 12;
    GBitmap* icon = get_icon_by_choice_ex(choice, light_icon);
    if(icon != NULL)
    {
        GSize size = gbitmap_get_bounds(icon).size;
        GRect dst = GRect(8, (bounds.size.h - size.h) / 2, size.w, size.h);
        graphics_context_set_compositing_mode(ctx, GCompOpSet);
        graphics_draw_bitmap_in_rect(ctx, icon, dst);
        text_x = 8 + size.w + 10;
    }

    graphics_context_set_text_color(ctx, fg);
    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    GRect text_rect = GRect(text_x, 0, bounds.size.w - text_x - 4, bounds.size.h);
    graphics_draw_text(ctx, icon_choice_name(choice), font, text_rect,
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft,
        NULL);
}

static void select_click(MenuLayer* menu_layer, MenuIndex* index, void* context)
{
    uint8_t choice = m_choices[index->row];
    if(m_callback != NULL)
    {
        m_callback(choice, m_context);
    }
    window_stack_pop(true);
}

static void window_load(Window* window)
{
    window_set_background_color(window, config_get_background_color());
    Layer* window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    m_status_bar = status_bar_layer_create();
    status_bar_layer_set_separator_mode(m_status_bar, StatusBarLayerSeparatorModeDotted);
    status_bar_layer_set_colors(m_status_bar,
        config_get_background_color(), config_get_foreground_color());
    layer_add_child(window_layer, status_bar_layer_get_layer(m_status_bar));

    m_menu_layer = menu_layer_create(GRect(0, STATUS_BAR_LAYER_HEIGHT,
        bounds.size.w, bounds.size.h - STATUS_BAR_LAYER_HEIGHT));
    menu_layer_set_callbacks(m_menu_layer, NULL, (MenuLayerCallbacks) {
        .get_num_rows = get_num_rows,
        .get_cell_height = get_cell_height,
        .draw_row = draw_row,
        .select_click = select_click,
    });
    menu_layer_set_normal_colors(m_menu_layer,
        config_get_background_color(), config_get_foreground_color());
    menu_layer_set_highlight_colors(m_menu_layer,
        config_get_foreground_color(), config_get_background_color());
    menu_layer_set_click_config_onto_window(m_menu_layer, window);
    layer_add_child(window_layer, menu_layer_get_layer(m_menu_layer));

    menu_layer_set_selected_index(m_menu_layer,
        (MenuIndex) { .section = 0, .row = row_for_choice(m_current_choice) },
        MenuRowAlignCenter, false);
}

static void window_unload(Window* window)
{
    menu_layer_destroy(m_menu_layer);
    m_menu_layer = NULL;
    status_bar_layer_destroy(m_status_bar);
    m_status_bar = NULL;
}

void setup_icon_picker_window(bool small_only, uint8_t current,
    IconPickerCallback cb, void* context)
{
    m_small_only = small_only;
    m_current_choice = current;
    m_callback = cb;
    m_context = context;
    build_choices(current);

    if(m_window == NULL)
    {
        m_window = window_create();
        window_set_window_handlers(m_window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
        });
    }

    window_stack_push(m_window, true);
}

void tear_down_icon_picker_window()
{
    if(m_window != NULL)
    {
        window_destroy(m_window);
        m_window = NULL;
    }
}
