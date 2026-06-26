#include "metrics_config_window.h"

#include <pebble.h>

#include "repositories/metrics_repository.h"
#include "repositories/app_config_repository.h"
#include "format.h"
#include "icons.h"
#include "edit_alarm_window.h"

#define MAX_METRIC_ROWS (3 + MAX_METRIC_OPTIONS)  // Title, Type, Main icon + Max OR per-option icons
#define ROW_TEXT_LEN (20)

typedef enum
{
    RowKind_TITLE,
    RowKind_TYPE,
    RowKind_MAIN_ICON,
    RowKind_MAX,
    RowKind_OPTION_ICON,
} RowKind;

static Window *m_config_window;
static StatusBarLayer* m_status_bar;
static SimpleMenuLayer* m_config_menu_layer = NULL;

static SimpleMenuSection m_menu_sections[2];
static SimpleMenuSection m_metric_items_section = { .title = "Metric" };
static SimpleMenuSection m_metric_group_items_section = { .title = "In groups" };

// Dynamic "Metric" section rows.
static void metric_row_selected(int index, void *context);
static bool icon_is_small(uint8_t choice);
static GBitmap* row_icon(uint8_t choice);
static SimpleMenuItem m_metric_items[MAX_METRIC_ROWS];
static char m_row_title[MAX_METRIC_ROWS][ROW_TEXT_LEN];
static char m_row_subtitle[MAX_METRIC_ROWS][ROW_TEXT_LEN];
static RowKind m_row_kind[MAX_METRIC_ROWS];
static uint8_t m_row_option[MAX_METRIC_ROWS];

// "In groups" section rows (toggle membership).
static SimpleMenuItem* m_group_items = NULL;
static uint16_t* m_group_id_index_map = NULL;

static DictationSession* m_dictation_session;
static Metrics* m_metric = NULL;

static void create_menu();

static void update_status_bar()
{
    status_bar_layer_set_colors(m_status_bar, config_get_background_color(), config_get_foreground_color());
}

static void mark_menu_dirty()
{
    layer_mark_dirty(simple_menu_layer_get_layer(m_config_menu_layer));
    update_status_bar();
}

static const char* icon_name(uint8_t choice)
{
    switch(choice)
    {
        case IconChoice_CHECK:    return "Check";
        case IconChoice_CROSS:    return "Cross";
        case IconChoice_UP:       return "Up";
        case IconChoice_DOWN:     return "Down";
        case IconChoice_MOOD:     return "Mood";
        case IconChoice_EXERCISE: return "Exercise";
        case IconChoice_PILL:     return "Pill";
        default:                  return "None";
    }
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
    const char* subtitle, GBitmap* icon)
{
    m_row_kind[r] = kind;
    m_row_option[r] = option;
    snprintf(m_row_title[r], ROW_TEXT_LEN, "%s", title);

    m_metric_items[r] = (SimpleMenuItem){0};
    m_metric_items[r].title = m_row_title[r];
    m_metric_items[r].callback = metric_row_selected;
    m_metric_items[r].icon = icon;
    if(subtitle != NULL)
    {
        snprintf(m_row_subtitle[r], ROW_TEXT_LEN, "%s", subtitle);
        m_metric_items[r].subtitle = m_row_subtitle[r];
    }
}

static void build_metric_rows()
{
    int r = 0;
    set_row(r++, RowKind_TITLE, 0, "Title",
        m_metric->title != NULL ? m_metric->title->value : "", NULL);
    set_row(r++, RowKind_TYPE, 0, "Type", type_label(m_metric->type), NULL);
    set_row(r++, RowKind_MAIN_ICON, 0, "Main icon",
        icon_name(m_metric->main_icon), row_icon(m_metric->main_icon));

    if(m_metric->type == MetricsType_INTERVAL)
    {
        char buffer[ROW_TEXT_LEN];
        snprintf(buffer, sizeof(buffer), "%d", m_metric->max_value);
        set_row(r++, RowKind_MAX, 0, "Max", buffer, NULL);
    } else
    {
        int options = (m_metric->type == MetricsType_THREE_OPTION) ? 3 : 2;
        for(int option = 0; option < options; option++)
        {
            char title[ROW_TEXT_LEN];
            snprintf(title, sizeof(title), "Option %d icon", option + 1);
            set_row(r++, RowKind_OPTION_ICON, option, title,
                icon_name(m_metric->option_icons[option]),
                row_icon(m_metric->option_icons[option]));
        }
    }

    m_metric_items_section.items = m_metric_items;
    m_metric_items_section.num_items = r;
}

static void free_group_rows()
{
    if(m_group_items != NULL)
    {
        free(m_group_items);
        m_group_items = NULL;
    }
    if(m_group_id_index_map != NULL)
    {
        free(m_group_id_index_map);
        m_group_id_index_map = NULL;
    }
}

static void toggle_connected_to_metrics_group(int index, void *context)
{
    metrics_group_toggle_metric(m_group_id_index_map[index], m_metric->id);
    create_menu();
}

static void build_group_rows()
{
    free_group_rows();

    size_t count = metrics_groups_count();
    size_t items_size = count * sizeof(SimpleMenuItem);
    m_group_items = (SimpleMenuItem*)malloc(items_size);
    memset(m_group_items, 0, items_size);
    m_group_id_index_map = (uint16_t*)malloc(count * sizeof(uint16_t));

    MetricsGroup* groups = metrics_groups_get_all();
    for(uint16_t i = 0; i < count; i++)
    {
        MetricsGroup* group = &groups[i];
        m_group_id_index_map[i] = group->id;
        if(metrics_group_has_metric(group->id, m_metric->id))
        {
            m_group_items[i].icon = config_is_dark_theme() ? get_check_icon_white() : get_check_icon_black();
        }
        m_group_items[i].title = group->title->value;
        m_group_items[i].callback = toggle_connected_to_metrics_group;
    }
    m_metric_group_items_section.items = m_group_items;
    m_metric_group_items_section.num_items = count;
}

static bool icon_is_small(uint8_t choice)
{
    // mood/exercise are 50x50 display icons; everything else fits a menu row
    // and the action bar.
    return choice != IconChoice_MOOD && choice != IconChoice_EXERCISE;
}

// Icon to show on a config row: only small icons (large ones would overflow the
// row and cover the text), otherwise just the name in the subtitle.
static GBitmap* row_icon(uint8_t choice)
{
    return icon_is_small(choice) ? get_icon_by_choice(choice) : NULL;
}

static void cycle_main_icon(uint8_t* field)
{
    *field = (*field + 1) % IconChoice_COUNT;
}

static void cycle_option_icon(uint8_t* field)
{
    // Option icons render in the narrow action bar, so skip the oversized
    // display icons.
    do
    {
        *field = (*field + 1) % IconChoice_COUNT;
    } while(!icon_is_small(*field));
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
            m_metric->option_icons[0] = IconChoice_DOWN;
            m_metric->option_icons[1] = IconChoice_CHECK;
            m_metric->option_icons[2] = IconChoice_UP;
            break;
        default:
            m_metric->type = MetricsType_BOOL;
            m_metric->option_icons[0] = IconChoice_CROSS;
            m_metric->option_icons[1] = IconChoice_CHECK;
            m_metric->option_icons[2] = IconChoice_NONE;
            break;
    }
}

static void metric_row_selected(int index, void *context)
{
    switch(m_row_kind[index])
    {
        case RowKind_TITLE:
            dictation_session_start(m_dictation_session);
            break;
        case RowKind_TYPE:
            change_type();
            metrics_save();
            create_menu();          // row count changes with type
            break;
        case RowKind_MAIN_ICON:
            cycle_main_icon(&m_metric->main_icon);
            metrics_save();
            m_metric_items[index].icon = row_icon(m_metric->main_icon);
            snprintf(m_row_subtitle[index], ROW_TEXT_LEN, "%s", icon_name(m_metric->main_icon));
            mark_menu_dirty();
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
        {
            uint8_t option = m_row_option[index];
            cycle_option_icon(&m_metric->option_icons[option]);
            metrics_save();
            m_metric_items[index].icon = row_icon(m_metric->option_icons[option]);
            snprintf(m_row_subtitle[index], ROW_TEXT_LEN, "%s", icon_name(m_metric->option_icons[option]));
            mark_menu_dirty();
            break;
        }
    }
}

static void dictation_session_callback(
    DictationSession *session,
    DictationSessionStatus status,
    char *transcription,
    void *context)
{
    if(status == DictationSessionStatusSuccess)
    {
        metrics_set_title(m_metric, transcription);
        create_menu();
    }
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

    build_metric_rows();
    build_group_rows();

    m_menu_sections[0] = m_metric_items_section;
    m_menu_sections[1] = m_metric_group_items_section;

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

static void load_main_window(Window *window)
{
    window_set_background_color(window, config_get_background_color());
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    setup_status_bar(window_layer, bounds);
    create_menu();

    m_dictation_session = dictation_session_create(512, dictation_session_callback, NULL);
}

static void unload_main_window(Window *window)
{
    status_bar_layer_destroy(m_status_bar);
    simple_menu_layer_destroy(m_config_menu_layer);
    m_config_menu_layer = NULL;
    dictation_session_destroy(m_dictation_session);
    free_group_rows();
}

void setup_metrics_config_window(Metrics* metric)
{
    m_metric = metric;
    m_config_window = window_create();

    window_set_window_handlers(m_config_window, (WindowHandlers) {
        .load = load_main_window,
        .unload = unload_main_window,
    });

    window_stack_push(m_config_window, true);
}

void tear_down_metrics_config_window()
{
    free_group_rows();
    window_destroy(m_config_window);
}
