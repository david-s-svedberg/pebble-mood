#include "metrics_config_window.h"

#include <pebble.h>

#include "repositories/metrics_repository.h"
#include "repositories/app_config_repository.h"
#include "format.h"
#include "icons.h"
#include "icon_picker_window.h"
#include "edit_alarm_window.h"
#include "menu_theme.h"

// Title, Type, Main icon + Max OR per-option (icon + text) rows.
#define MAX_METRIC_ROWS (3 + 2 * MAX_METRIC_OPTIONS)
#define ROW_TEXT_LEN (20)
#define NO_ICON (0xFF)

#define SECTION_METRIC (0)
#define SECTION_GROUPS (1)

typedef enum
{
    RowKind_TITLE,
    RowKind_TYPE,
    RowKind_MAIN_ICON,
    RowKind_MAX,
    RowKind_OPTION_ICON,
    RowKind_OPTION_TEXT,
} RowKind;

static Window *m_config_window;
static StatusBarLayer* m_status_bar;
static MenuLayer* m_menu_layer = NULL;

// "Metric" section rows, rebuilt whenever the metric or its type changes. The
// icon is stored as an IconChoice (NO_ICON if the row has none) and resolved to
// a bitmap at draw time, so the same row can render its black or white variant
// depending on whether it is highlighted.
static RowKind m_row_kind[MAX_METRIC_ROWS];
static uint8_t m_row_option[MAX_METRIC_ROWS];
static uint8_t m_row_icon_choice[MAX_METRIC_ROWS];
static char m_row_title[MAX_METRIC_ROWS][ROW_TEXT_LEN];
static char m_row_subtitle[MAX_METRIC_ROWS][ROW_TEXT_LEN];
static int m_metric_row_count = 0;

static DictationSession* m_dictation_session;
static Metrics* m_metric = NULL;
static int m_dictation_target = -1;  // -1 = title, 0..N = option text
static uint8_t m_pending_option = 0; // option index awaiting an icon pick
static int m_icon_row_index = 0;     // row that opened the icon picker

static void metric_row_selected(int index);
static void reload_menu();

static void update_status_bar()
{
    status_bar_layer_set_colors(m_status_bar, config_get_background_color(), config_get_foreground_color());
}

static void mark_menu_dirty()
{
    layer_mark_dirty(menu_layer_get_layer(m_menu_layer));
    update_status_bar();
}

static const char* type_label(MetricsType type)
{
    switch(type)
    {
        case MetricsType_INTERVAL:     return "Interval";
        case MetricsType_THREE_OPTION: return "3 options";
        default:                       return "Yes/No";
    }
}

static void set_row(int r, RowKind kind, uint8_t option, const char* title,
    const char* subtitle, uint8_t icon_choice)
{
    m_row_kind[r] = kind;
    m_row_option[r] = option;
    m_row_icon_choice[r] = icon_choice;
    snprintf(m_row_title[r], ROW_TEXT_LEN, "%s", title);
    if(subtitle != NULL)
    {
        snprintf(m_row_subtitle[r], ROW_TEXT_LEN, "%s", subtitle);
    } else
    {
        m_row_subtitle[r][0] = '\0';
    }
}

static void build_metric_rows()
{
    int r = 0;
    set_row(r++, RowKind_TITLE, 0, "Title",
        m_metric->title != NULL ? m_metric->title->value : "", NO_ICON);
    set_row(r++, RowKind_TYPE, 0, "Type", type_label(m_metric->type), NO_ICON);
    set_row(r++, RowKind_MAIN_ICON, 0, "Main icon",
        icon_choice_name(m_metric->main_icon), m_metric->main_icon);

    if(m_metric->type == MetricsType_INTERVAL)
    {
        char buffer[ROW_TEXT_LEN];
        snprintf(buffer, sizeof(buffer), "%d", m_metric->max_value);
        set_row(r++, RowKind_MAX, 0, "Max", buffer, NO_ICON);
    } else
    {
        int options = (m_metric->type == MetricsType_THREE_OPTION) ? 3 : 2;
        for(int option = 0; option < options; option++)
        {
            char title[ROW_TEXT_LEN];
            snprintf(title, sizeof(title), "Option %d icon", option + 1);
            set_row(r++, RowKind_OPTION_ICON, option, title,
                icon_choice_name(m_metric->option_icons[option]),
                m_metric->option_icons[option]);

            const char* text = metrics_get_option_text(m_metric, option);
            char text_title[ROW_TEXT_LEN];
            snprintf(text_title, sizeof(text_title), "Option %d text", option + 1);
            set_row(r++, RowKind_OPTION_TEXT, option, text_title,
                (text[0] != '\0') ? text : "(none)", NO_ICON);
        }
    }

    m_metric_row_count = r;
}

// Refresh just the row that opened the picker, so the cursor stays put (the row
// count doesn't change, so no reload is needed). The icon is resolved from the
// choice at draw time, so storing the choice is enough.
static void refresh_icon_row(uint8_t choice)
{
    int r = m_icon_row_index;
    m_row_icon_choice[r] = choice;
    snprintf(m_row_subtitle[r], ROW_TEXT_LEN, "%s", icon_choice_name(choice));
    mark_menu_dirty();
}

static void main_icon_picked(uint8_t choice, void* context)
{
    m_metric->main_icon = choice;
    metrics_save();
    refresh_icon_row(choice);
}

static void option_icon_picked(uint8_t choice, void* context)
{
    m_metric->option_icons[m_pending_option] = choice;
    metrics_save();
    refresh_icon_row(choice);
}

static void change_type()
{
    // Cycle Yes/No -> Interval -> 3 options -> Yes/No, seeding sensible default
    // option icons for the discrete types.
    switch(m_metric->type)
    {
        case MetricsType_BOOL:
            m_metric->type = MetricsType_INTERVAL;
            break;
        case MetricsType_INTERVAL:
            m_metric->type = MetricsType_THREE_OPTION;
            m_metric->option_icons[0] = IconChoice_FACE_SAD;
            m_metric->option_icons[1] = IconChoice_FACE_NEUTRAL;
            m_metric->option_icons[2] = IconChoice_FACE_HAPPY;
            break;
        default:
            m_metric->type = MetricsType_BOOL;
            m_metric->option_icons[0] = IconChoice_CROSS;
            m_metric->option_icons[1] = IconChoice_CHECK;
            m_metric->option_icons[2] = IconChoice_NONE;
            break;
    }
}

static void metric_row_selected(int index)
{
    switch(m_row_kind[index])
    {
        case RowKind_TITLE:
            m_dictation_target = -1;
            dictation_session_start(m_dictation_session);
            break;
        case RowKind_TYPE:
            change_type();
            metrics_save();
            reload_menu();          // row count changes with type
            break;
        case RowKind_MAIN_ICON:
            m_icon_row_index = index;
            setup_icon_picker_window(false, m_metric->main_icon, main_icon_picked, NULL);
            break;
        case RowKind_MAX:
            m_metric->max_value++;
            if(m_metric->max_value > 10)
            {
                m_metric->max_value = 2;
            }
            metrics_save();
            snprintf(m_row_subtitle[index], ROW_TEXT_LEN, "%d", m_metric->max_value);
            mark_menu_dirty();
            break;
        case RowKind_OPTION_ICON:
            m_pending_option = m_row_option[index];
            m_icon_row_index = index;
            setup_icon_picker_window(true, m_metric->option_icons[m_pending_option],
                option_icon_picked, NULL);
            break;
        case RowKind_OPTION_TEXT:
            m_dictation_target = m_row_option[index];
            dictation_session_start(m_dictation_session);
            break;
    }
}

static void toggle_group_membership(int index)
{
    MetricsGroup* groups = metrics_groups_get_all();
    metrics_group_toggle_metric(groups[index].id, m_metric->id);
    // The checkmark is derived from live membership at draw time, so a repaint
    // (cursor preserved) is enough.
    mark_menu_dirty();
}

// --- MenuLayer callbacks ---------------------------------------------------

static uint16_t menu_get_num_sections(MenuLayer* menu_layer, void* context)
{
    return 2;
}

static uint16_t menu_get_num_rows(MenuLayer* menu_layer, uint16_t section, void* context)
{
    if(section == SECTION_METRIC)
    {
        return (uint16_t)m_metric_row_count;
    }
    return (uint16_t)metrics_groups_count();
}

static int16_t menu_get_header_height(MenuLayer* menu_layer, uint16_t section, void* context)
{
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header(GContext* ctx, const Layer* cell_layer, uint16_t section, void* context)
{
    menu_cell_basic_header_draw(ctx, cell_layer,
        section == SECTION_METRIC ? "Metric" : "In groups");
}

static void menu_draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* index, void* context)
{
    // MenuLayer paints the cell background and sets the context text colour from
    // its normal/highlight colours before calling us; we only have to hand
    // menu_cell_basic_draw an icon whose colour matches that background. On the
    // highlighted (or dark-theme) cell that's the white variant, else black.
    bool light = menu_theme_icon_light(cell_layer);

    if(index->section == SECTION_METRIC)
    {
        uint8_t choice = m_row_icon_choice[index->row];
        GBitmap* icon = (choice == NO_ICON) ? NULL : get_icon_row_by_choice(choice, light);
        const char* subtitle = m_row_subtitle[index->row][0] != '\0'
            ? m_row_subtitle[index->row] : NULL;
        menu_cell_basic_draw(ctx, cell_layer, m_row_title[index->row], subtitle, icon);
    } else
    {
        MetricsGroup* groups = metrics_groups_get_all();
        MetricsGroup* group = &groups[index->row];
        bool member = metrics_group_has_metric(group->id, m_metric->id);
        GBitmap* icon = member
            ? (light ? get_check_icon_white() : get_check_icon_black())
            : NULL;
        menu_cell_basic_draw(ctx, cell_layer, group->title->value, NULL, icon);
    }
}

static void menu_select_click(MenuLayer* menu_layer, MenuIndex* index, void* context)
{
    if(index->section == SECTION_METRIC)
    {
        metric_row_selected(index->row);
    } else
    {
        toggle_group_membership(index->row);
    }
}

static void reload_menu()
{
    build_metric_rows();
    menu_layer_reload_data(m_menu_layer);
    update_status_bar();
}

static void dictation_session_callback(
    DictationSession *session,
    DictationSessionStatus status,
    char *transcription,
    void *context)
{
    if(status == DictationSessionStatusSuccess)
    {
        if(m_dictation_target < 0)
        {
            metrics_set_title(m_metric, transcription);
        } else
        {
            metrics_set_option_text(m_metric, (uint8_t)m_dictation_target, transcription);
        }
        reload_menu();
    }
}

static void load_main_window(Window *window)
{
    window_set_background_color(window, config_get_background_color());
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    m_status_bar = status_bar_layer_create();
    status_bar_layer_set_separator_mode(m_status_bar, StatusBarLayerSeparatorModeDotted);
    layer_add_child(window_layer, status_bar_layer_get_layer(m_status_bar));
    update_status_bar();

    build_metric_rows();

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
    menu_layer_set_click_config_onto_window(m_menu_layer, window);
    layer_add_child(window_layer, menu_layer_get_layer(m_menu_layer));

    m_dictation_session = dictation_session_create(512, dictation_session_callback, NULL);
}

static void unload_main_window(Window *window)
{
    status_bar_layer_destroy(m_status_bar);
    menu_layer_destroy(m_menu_layer);
    m_menu_layer = NULL;
    dictation_session_destroy(m_dictation_session);
}

void setup_metrics_config_window(Metrics* metric)
{
    m_metric = metric;
    if(m_config_window == NULL)
    {
        m_config_window = window_create();
        window_set_window_handlers(m_config_window, (WindowHandlers) {
            .load = load_main_window,
            .unload = unload_main_window,
        });
    }

    window_stack_push(m_config_window, true);
}

void tear_down_metrics_config_window()
{
    window_destroy(m_config_window);
}
