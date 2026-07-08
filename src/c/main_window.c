#include "main_window.h"

#include <pebble.h>

#include "config_menu_window.h"
#include "metric_list_window.h"
#include "scheduler.h"
#include "icons.h"
#include "menu_theme.h"
#include "trend.h"
#include "repositories/app_config_repository.h"
#include "repositories/metrics_repository.h"

static Window* m_main_window;
static StatusBarLayer* m_status_bar;
static TextLayer* m_title_layer;
static TextLayer* m_next_layer;
static BitmapLayer* m_icon_layer;
static Layer* m_graph_layer;
static ActionBarLayer* m_action_bar;
static char m_next_buffer[24];

static void open_settings(ClickRecognizerRef recognizer, void* context)
{
    setup_config_window();
}

static void open_register(ClickRecognizerRef recognizer, void* context)
{
    // Spontaneous registration: pick any metric and register it now.
    setup_metric_list_window(MetricList_ALL);
}

static void open_today(ClickRecognizerRef recognizer, void* context)
{
    // Today: scheduled + spontaneously-registered metrics with answered status.
    setup_metric_list_window(MetricList_TODAY);
}

static void click_config_provider(void* context)
{
    window_single_click_subscribe(BUTTON_ID_UP, open_today);
    window_single_click_subscribe(BUTTON_ID_SELECT, open_register);
    window_single_click_subscribe(BUTTON_ID_DOWN, open_settings);
}

static void update_next_time()
{
    snprintf(m_next_buffer, sizeof(m_next_buffer), "Next: %s", get_next_alarm_time_string());
    text_layer_set_text(m_next_layer, m_next_buffer);
}


static void setup_title_layer(Layer* window_layer, GRect bounds)
{
    m_title_layer = text_layer_create(
        GRect(0, STATUS_BAR_LAYER_HEIGHT + 20, bounds.size.w - ACTION_BAR_WIDTH, 44));
    text_layer_set_background_color(m_title_layer, config_get_background_color());
    text_layer_set_text_color(m_title_layer, config_get_foreground_color());
    text_layer_set_font(m_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text_alignment(m_title_layer, GTextAlignmentCenter);
    text_layer_set_text(m_title_layer, "Mood");
    layer_add_child(window_layer, text_layer_get_layer(m_title_layer));
}

static void setup_icon_layer(Layer* window_layer, GRect bounds)
{
    int content_w = bounds.size.w - ACTION_BAR_WIDTH;
    // Centered horizontally; vertically centered between the title and the
    // bottom-left "Next" line so it never overlaps the title on short screens.
    int title_bottom = STATUS_BAR_LAYER_HEIGHT + 20 + 44;
    int next_top = bounds.size.h - 28;
    int icon_y = (title_bottom + next_top) / 2 - 24;
    GRect icon_rect = GRect((content_w - 48) / 2, icon_y, 48, 48);
    m_icon_layer = bitmap_layer_create(icon_rect);
    bitmap_layer_set_background_color(m_icon_layer, GColorClear);
    bitmap_layer_set_compositing_mode(m_icon_layer, GCompOpSet);
    // White on the dark theme background, black on the light one.
    bitmap_layer_set_bitmap(m_icon_layer,
        get_icon_by_choice_ex(IconChoice_MOOD, config_is_dark_theme()));
    layer_add_child(window_layer, bitmap_layer_get_layer(m_icon_layer));
}

// Two-letter weekday labels, indexed by struct tm's tm_wday (0 = Sunday).
static const char* const WEEKDAY2[7] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };

#define AXIS_STRIP_H (15)

// x for day-slot d (0 = oldest / 6 days ago, TREND_DAYS-1 = today) across the
// shared plot span [left, right]. Today sits at the right edge.
static int day_x(int d, int left, int right)
{
    if(TREND_DAYS <= 1) return left;
    return left + (d * (right - left)) / (TREND_DAYS - 1);
}

// One favourite's sparkline: metric name label, then a 7-day normalized line.
// Days without data are simply left blank (the line breaks across gaps); the
// shared weekday axis below still shows every day so the window is legible.
static void draw_sparkline(GContext* ctx, GRect row_rect, int plot_left, int plot_right, uint16_t metric_id)
{
    Metrics* metric = metrics_get(metric_id);
    if(metric == NULL) return;

    GColor fg = config_get_foreground_color();
    graphics_context_set_text_color(ctx, fg);
    graphics_context_set_stroke_color(ctx, fg);
    graphics_context_set_fill_color(ctx, fg);

    const char* name = (metric->title != NULL) ? metric->title->value : "";
    graphics_draw_text(ctx, name, fonts_get_system_font(FONT_KEY_GOTHIC_14),
        GRect(row_rect.origin.x + 2, row_rect.origin.y, row_rect.size.w - 4, 16),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

    TrendSeries series;
    trend_build(metric_id, &series);

    int plot_top = row_rect.origin.y + 16;
    int plot_bottom = row_rect.origin.y + row_rect.size.h - 2;
    int plot_h = plot_bottom - plot_top;
    if(plot_h < 4) return;

    int span = series.max_v - series.min_v;   // effective_range guarantees >= 1
    graphics_context_set_antialiased(ctx, true);
    graphics_context_set_stroke_width(ctx, 2);

    GPoint prev = GPointZero;
    bool have_prev = false;
    for(int d = 0; d < TREND_DAYS; d++)
    {
        if(!series.has[d])
        {
            have_prev = false;   // gap: don't bridge missing days
            continue;
        }
        int x = day_x(d, plot_left, plot_right);
        int y = plot_bottom - ((series.value[d] - series.min_v) * plot_h) / span;
        GPoint p = GPoint(x, y);
        if(have_prev) graphics_draw_line(ctx, prev, p);
        graphics_fill_circle(ctx, p, 2);
        prev = p;
        have_prev = true;
    }
    graphics_context_set_stroke_width(ctx, 1);
}

// The one x-axis shared by every sparkline: a baseline and a two-letter weekday
// under each of the 7 day-slots. Labels roll with the date (today on the right,
// 6 days ago on the left), and all 7 show even when most days have no data.
static void draw_axis(GContext* ctx, int plot_left, int plot_right, int strip_top)
{
    GColor fg = config_get_foreground_color();
    graphics_context_set_stroke_color(ctx, fg);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, GPoint(plot_left, strip_top), GPoint(plot_right, strip_top));

    graphics_context_set_text_color(ctx, fg);
    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14);

    time_t now = time(NULL);
    struct tm* lt = localtime(&now);
    int today_wday = lt->tm_wday;   // 0..6

    // Size each label to the slot spacing so labels never overlap on narrow
    // screens (144 px leaves ~17 px/slot); capped so they don't sprawl on emery.
    int spacing = (TREND_DAYS > 1) ? (plot_right - plot_left) / (TREND_DAYS - 1) : 24;
    int label_w = spacing < 26 ? spacing : 26;

    for(int d = 0; d < TREND_DAYS; d++)
    {
        int days_ago = (TREND_DAYS - 1) - d;
        int wday = ((today_wday - days_ago) % 7 + 7) % 7;
        int x = day_x(d, plot_left, plot_right);
        graphics_draw_text(ctx, WEEKDAY2[wday], font,
            GRect(x - label_w / 2, strip_top, label_w, AXIS_STRIP_H),
            GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    }
}

static void graph_update_proc(Layer* layer, GContext* ctx)
{
    GRect bounds = layer_get_bounds(layer);

    uint16_t favorites[MAX_FAVORITES];
    config_get_favorites(favorites);

    // Collect the favourites whose metric still exists.
    uint16_t valid[MAX_FAVORITES];
    uint8_t n = 0;
    for(uint8_t i = 0; i < MAX_FAVORITES; i++)
    {
        if(favorites[i] != 0 && metrics_get(favorites[i]) != NULL)
        {
            valid[n++] = favorites[i];
        }
    }
    if(n == 0) return;

    // Reserve the bottom strip for the shared weekday axis; stack the sparklines
    // in the space above it, all sharing the same left/right plot span.
    int plot_left = 4;
    int plot_right = bounds.size.w - 4;
    int rows_bottom = bounds.size.h - AXIS_STRIP_H;
    int row_h = rows_bottom / n;
    for(uint8_t i = 0; i < n; i++)
    {
        GRect row = GRect(0, i * row_h, bounds.size.w, row_h);
        draw_sparkline(ctx, row, plot_left, plot_right, valid[i]);
    }
    draw_axis(ctx, plot_left, plot_right, rows_bottom);
}

static void setup_graph_layer(Layer* window_layer, GRect bounds)
{
    int content_w = bounds.size.w - ACTION_BAR_WIDTH;
    int top = STATUS_BAR_LAYER_HEIGHT + 4;
    int bottom = bounds.size.h - 28 - 2;
    m_graph_layer = layer_create(GRect(2, top, content_w - 4, bottom - top));
    layer_set_update_proc(m_graph_layer, graph_update_proc);
    layer_add_child(window_layer, m_graph_layer);
}

// A sparkline only reads as a graph once it has a few days behind it. Below
// this, the home screen keeps the icon so a lone dot never looks like the UI.
#define MIN_GRAPH_DAYS (3)

// True when at least one favourite has >= MIN_GRAPH_DAYS days of data.
static bool graph_has_enough_data()
{
    uint16_t favorites[MAX_FAVORITES];
    config_get_favorites(favorites);
    for(uint8_t i = 0; i < MAX_FAVORITES; i++)
    {
        if(favorites[i] == 0 || metrics_get(favorites[i]) == NULL) continue;
        TrendSeries series;
        if(trend_build(favorites[i], &series) && series.points >= MIN_GRAPH_DAYS)
        {
            return true;
        }
    }
    return false;
}

// Home screen shows either the big mood icon or the favourite sparklines. The
// window stays on the stack under Settings, so this is re-evaluated on every
// appear. The graph only takes over once there's enough history to read as one.
static void update_home_content()
{
    bool graph = config_favorite_count() > 0 && graph_has_enough_data();
    layer_set_hidden(bitmap_layer_get_layer(m_icon_layer), graph);
    layer_set_hidden(text_layer_get_layer(m_title_layer), graph);
    layer_set_hidden(m_graph_layer, !graph);
    layer_mark_dirty(m_graph_layer);
}

static void setup_next_layer(Layer* window_layer, GRect bounds)
{
    // Bottom-left corner, left aligned.
    m_next_layer = text_layer_create(
        GRect(4, bounds.size.h - 28, bounds.size.w - ACTION_BAR_WIDTH - 4, 24));
    text_layer_set_background_color(m_next_layer, config_get_background_color());
    text_layer_set_text_color(m_next_layer, config_get_foreground_color());
    text_layer_set_font(m_next_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(m_next_layer, GTextAlignmentLeft);
    layer_add_child(window_layer, text_layer_get_layer(m_next_layer));
}

static void setup_action_bar()
{
    m_action_bar = action_bar_layer_create();
    action_bar_layer_add_to_window(m_action_bar, m_main_window);
    action_bar_layer_set_click_config_provider(m_action_bar, click_config_provider);
}

// Re-applies every theme-dependent colour/bitmap. The main window stays alive
// under Settings on the window stack, so a theme toggle there must re-theme
// this window on appear (all other windows re-read the theme on load).
static void apply_theme()
{
    window_set_background_color(m_main_window, config_get_background_color());
    status_bar_layer_set_colors(m_status_bar, config_get_background_color(), config_get_foreground_color());

    text_layer_set_background_color(m_title_layer, config_get_background_color());
    text_layer_set_text_color(m_title_layer, config_get_foreground_color());
    text_layer_set_background_color(m_next_layer, config_get_background_color());
    text_layer_set_text_color(m_next_layer, config_get_foreground_color());

    bitmap_layer_set_bitmap(m_icon_layer,
        get_icon_by_choice_ex(IconChoice_MOOD, config_is_dark_theme()));

    action_bar_layer_set_background_color(m_action_bar, config_get_foreground_color());
    action_bar_layer_set_icon_animated(m_action_bar, BUTTON_ID_UP, get_bar_icon(BarIcon_CHECK), true);
    action_bar_layer_set_icon_animated(m_action_bar, BUTTON_ID_SELECT, get_bar_icon(BarIcon_EDIT), true);
    action_bar_layer_set_icon_animated(m_action_bar, BUTTON_ID_DOWN, get_bar_icon(BarIcon_CONFIG), true);
}

static void load_main_window(Window* window)
{
    Layer* window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    m_status_bar = status_bar_create_themed(window_layer);
    setup_title_layer(window_layer, bounds);
    setup_icon_layer(window_layer, bounds);
    setup_graph_layer(window_layer, bounds);
    setup_next_layer(window_layer, bounds);
    setup_action_bar();
    apply_theme();
    update_home_content();
}

static void appear_main_window(Window* window)
{
    apply_theme();
    update_next_time();
    // Favourites and registrations may have changed under Settings/registration.
    update_home_content();
}

static void unload_main_window(Window* window)
{
    status_bar_layer_destroy(m_status_bar);
    text_layer_destroy(m_title_layer);
    text_layer_destroy(m_next_layer);
    bitmap_layer_destroy(m_icon_layer);
    layer_destroy(m_graph_layer);
    action_bar_layer_remove_from_window(m_action_bar);
    action_bar_layer_destroy(m_action_bar);
}

void setup_main_window()
{
    if(m_main_window == NULL)
    {
        m_main_window = window_create();
        window_set_window_handlers(m_main_window, (WindowHandlers) {
            .load = load_main_window,
            .unload = unload_main_window,
            .appear = appear_main_window,
        });
    }
    window_stack_push(m_main_window, true);
}

void tear_down_main_window()
{
    window_destroy(m_main_window);
}
