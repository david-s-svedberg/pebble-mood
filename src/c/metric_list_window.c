#include "metric_list_window.h"

#include <pebble.h>

#include "main_window_logic.h"   // main_window_format_value
#include "menu_theme.h"
#include "register_mood_window.h"
#include "repositories/metrics_repository.h"
#include "repositories/app_config_repository.h"

#define SUBTITLE_LEN (16)

// Sentinel stored in m_section_group for the "Misc" section (spontaneous
// registrations made today). Its rows carry group_id 0.
#define MISC_SECTION (0xFFFF)

typedef struct
{
    uint16_t group_id;    // 0 = spontaneous / placeholder
    uint16_t metric_id;   // 0 = placeholder row
} ListRow;

static Window* m_window;
static StatusBarLayer* m_status_bar;
static MenuLayer* m_menu_layer = NULL;
static MetricListMode m_mode;

// Flat row list plus section boundaries. In TODAY mode there is one section per
// group (a metric in several groups appears once per group); in ALL mode there
// is a single "Register" section listing every metric once.
static ListRow* m_rows = NULL;
static uint16_t* m_section_group = NULL;  // group id per section
static uint16_t* m_section_rows = NULL;   // row count per section
static uint16_t* m_section_start = NULL;  // first flat-row index per section
static uint16_t m_section_count = 0;
static uint16_t m_row_count = 0;

static void free_dynamic()
{
    if(m_rows != NULL) { free(m_rows); m_rows = NULL; }
    if(m_section_group != NULL) { free(m_section_group); m_section_group = NULL; }
    if(m_section_rows != NULL) { free(m_section_rows); m_section_rows = NULL; }
    if(m_section_start != NULL) { free(m_section_start); m_section_start = NULL; }
    m_section_count = 0;
    m_row_count = 0;
}

static void set_placeholder()
{
    // Single empty-state row.
    m_rows[0] = (ListRow){ .group_id = 0, .metric_id = 0 };
    m_row_count = 1;
    m_section_group[0] = 0;
    m_section_rows[0] = 1;
    m_section_start[0] = 0;
    m_section_count = 1;
}

static void build_today()
{
    size_t groups = metrics_groups_count();
    size_t metrics_n = metrics_count();
    // group sections (groups * metrics_n) + a Misc section (metrics_n) + placeholder
    size_t max_rows = (groups * metrics_n) + metrics_n + 1;
    size_t max_sections = groups + 2;

    m_rows = malloc(max_rows * sizeof(ListRow));
    m_section_group = malloc(max_sections * sizeof(uint16_t));
    m_section_rows = malloc(max_sections * sizeof(uint16_t));
    m_section_start = malloc(max_sections * sizeof(uint16_t));
    m_section_count = 0;
    m_row_count = 0;

    MetricsGroup* all_groups = metrics_groups_get_all();
    Metrics* all_metrics = metrics_get_all();
    for(size_t g = 0; g < groups; g++)
    {
        uint16_t group_id = all_groups[g].id;
        uint16_t start = m_row_count;
        uint16_t count = 0;
        for(size_t m = 0; m < metrics_n; m++)
        {
            if(metrics_group_has_metric(group_id, all_metrics[m].id))
            {
                m_rows[m_row_count++] = (ListRow){ .group_id = group_id, .metric_id = all_metrics[m].id };
                count++;
            }
        }
        if(count > 0)
        {
            m_section_group[m_section_count] = group_id;
            m_section_rows[m_section_count] = count;
            m_section_start[m_section_count] = start;
            m_section_count++;
        }
    }

    // Misc: metrics registered spontaneously (group 0) today.
    uint16_t misc_start = m_row_count;
    uint16_t misc_count = 0;
    for(size_t m = 0; m < metrics_n; m++)
    {
        if(metric_registered_today_in_group(0, all_metrics[m].id))
        {
            m_rows[m_row_count++] = (ListRow){ .group_id = 0, .metric_id = all_metrics[m].id };
            misc_count++;
        }
    }
    if(misc_count > 0)
    {
        m_section_group[m_section_count] = MISC_SECTION;
        m_section_rows[m_section_count] = misc_count;
        m_section_start[m_section_count] = misc_start;
        m_section_count++;
    }

    if(m_section_count == 0)
    {
        set_placeholder();
    }
}

static void build_all()
{
    size_t metrics_n = metrics_count();
    size_t cap = (metrics_n == 0) ? 1 : metrics_n;

    m_rows = malloc(cap * sizeof(ListRow));
    m_section_group = malloc(sizeof(uint16_t));
    m_section_rows = malloc(sizeof(uint16_t));
    m_section_start = malloc(sizeof(uint16_t));

    Metrics* all_metrics = metrics_get_all();
    m_row_count = 0;
    for(size_t m = 0; m < metrics_n; m++)
    {
        m_rows[m_row_count++] = (ListRow){ .group_id = 0, .metric_id = all_metrics[m].id };
    }
    if(m_row_count == 0)
    {
        m_rows[0] = (ListRow){ .group_id = 0, .metric_id = 0 };
        m_row_count = 1;
    }
    m_section_group[0] = 0;
    m_section_rows[0] = m_row_count;
    m_section_start[0] = 0;
    m_section_count = 1;
}

static void build_rows()
{
    free_dynamic();
    if(m_mode == MetricList_TODAY)
    {
        build_today();
    } else
    {
        build_all();
    }
}

static const ListRow* row_at(MenuIndex* index)
{
    return &m_rows[m_section_start[index->section] + index->row];
}

static uint16_t menu_get_num_sections(MenuLayer* menu_layer, void* context)
{
    return m_section_count;
}

static uint16_t menu_get_num_rows(MenuLayer* menu_layer, uint16_t section, void* context)
{
    return m_section_rows[section];
}

static int16_t menu_get_header_height(MenuLayer* menu_layer, uint16_t section, void* context)
{
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header(GContext* ctx, const Layer* cell_layer, uint16_t section, void* context)
{
    const char* title;
    if(m_mode == MetricList_TODAY)
    {
        if(m_section_group[section] == MISC_SECTION)
        {
            title = "Misc";
        } else
        {
            MetricsGroup* group = metrics_group_get(m_section_group[section]);
            title = (group != NULL && group->title != NULL) ? group->title->value : "Today";
        }
    } else
    {
        title = "Register";
    }
    menu_cell_basic_header_draw(ctx, cell_layer, title);
}

static void menu_draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* index, void* context)
{
    const ListRow* row = row_at(index);

    if(row->metric_id == 0)
    {
        const char* text = (m_mode == MetricList_TODAY) ? "Nothing scheduled" : "No metrics yet";
        menu_cell_basic_draw(ctx, cell_layer, text, NULL, NULL);
        return;
    }

    Metrics* metric = metrics_get(row->metric_id);
    const char* metric_title = (metric != NULL && metric->title != NULL) ? metric->title->value : "";

    // Today: show the answer for this group's slot (or nothing). Spontaneous list
    // shows no subtitle.
    const char* subtitle = NULL;
    static char buffer[SUBTITLE_LEN];
    if(m_mode == MetricList_TODAY && metric != NULL)
    {
        Registration* reg = registration_today_for_group_metric(row->group_id, row->metric_id);
        if(reg != NULL)
        {
            main_window_format_value(metric, reg->value, buffer, sizeof(buffer));
            subtitle = buffer;
        }
    }

    menu_cell_basic_draw(ctx, cell_layer, metric_title, subtitle, NULL);
}

static void menu_select_click(MenuLayer* menu_layer, MenuIndex* index, void* context)
{
    const ListRow* row = row_at(index);
    if(row->metric_id == 0)
    {
        return;  // placeholder
    }
    Metrics* metric = metrics_get(row->metric_id);
    if(metric == NULL)
    {
        return;
    }
    if(m_mode == MetricList_TODAY)
    {
        setup_register_mood_window_for_metric_in_group(metric, row->group_id);
    } else
    {
        setup_register_mood_window_for_metric(metric);
    }
}

static void create_menu()
{
    Layer* window_layer = window_get_root_layer(m_window);
    GRect bounds = layer_get_bounds(window_layer);

    build_rows();

    if(m_menu_layer == NULL)
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
    } else
    {
        menu_layer_reload_data(m_menu_layer);
    }
}

static void load_window(Window* window)
{
    window_set_background_color(window, config_get_background_color());
    m_status_bar = status_bar_create_themed(window_get_root_layer(window));
    create_menu();
}

static void appear_window(Window* window)
{
    // Refresh answers/status after returning from a registration.
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
