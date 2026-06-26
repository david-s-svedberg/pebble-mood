#include "register_mood_window_logic.h"

#include <pebble.h>

#include "icons.h"
#include "repositories/metrics_repository.h"

static Window* m_window;
static ActionBarLayer* m_action_bar;
static TextLayer* m_title_layer;
static TextLayer* m_value_layer;
static BitmapLayer* m_icon_layer;

static MetricsGroup* m_group;
static Metrics* m_single_metric;
static Metrics** m_metrics = NULL;
static uint16_t m_metric_count = 0;
static uint16_t m_current_index = 0;
static uint8_t m_current_value = 0;

static char m_value_buffer[4];

static void show_current_metric();

static void collect_metrics()
{
    register_mood_tear_down();

    // Spontaneous single-metric registration.
    if(m_single_metric != NULL)
    {
        m_metrics = (Metrics**)malloc(sizeof(Metrics*));
        m_metrics[0] = m_single_metric;
        m_metric_count = 1;
        return;
    }

    // A group alarm: register every metric that belongs to the group.
    uint32_t total = metrics_count();
    m_metrics = (Metrics**)malloc(total * sizeof(Metrics*));
    m_metric_count = 0;

    Metrics* all_metrics = metrics_get_all();
    for(uint32_t i = 0; i < total; i++)
    {
        Metrics* current = &all_metrics[i];
        if(metrics_group_has_metric(m_group->id, current->id))
        {
            m_metrics[m_metric_count++] = current;
        }
    }
}

static void finish_registration()
{
    register_mood_tear_down();
    exit_reason_set(APP_EXIT_ACTION_PERFORMED_SUCCESSFULLY);
    window_stack_remove(m_window, true);
}

static void update_value_display()
{
    snprintf(m_value_buffer, sizeof(m_value_buffer), "%d", m_current_value);
    text_layer_set_text(m_value_layer, m_value_buffer);
}

static void save_and_advance(uint8_t value)
{
    Metrics* metric = m_metrics[m_current_index];
    Registration registration =
    {
        .metrics_id = metric->id,
        .value = value,
        .time_stamp = time(NULL),
    };
    registration_add(&registration);

    m_current_index++;
    if(m_current_index >= m_metric_count)
    {
        finish_registration();
    } else
    {
        show_current_metric();
    }
}

// --- Interval (0..max, stepped) ---

static void interval_increase(ClickRecognizerRef recognizer, void* context)
{
    Metrics* metric = m_metrics[m_current_index];
    if(m_current_value < metric->max_value)
    {
        m_current_value++;
        update_value_display();
    }
}

static void interval_decrease(ClickRecognizerRef recognizer, void* context)
{
    if(m_current_value > 0)
    {
        m_current_value--;
        update_value_display();
    }
}

static void interval_confirm(ClickRecognizerRef recognizer, void* context)
{
    save_and_advance(m_current_value);
}

static void interval_click_config_provider(void* context)
{
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, interval_increase);
    window_single_click_subscribe(BUTTON_ID_SELECT, interval_confirm);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, interval_decrease);
}

// --- Discrete options (BOOL = 2, THREE_OPTION = 3), answered directly ---

static void choose_option_0(ClickRecognizerRef recognizer, void* context)
{
    save_and_advance(0);
}

static void choose_option_1(ClickRecognizerRef recognizer, void* context)
{
    save_and_advance(1);
}

static void choose_option_2(ClickRecognizerRef recognizer, void* context)
{
    save_and_advance(2);
}

static void bool_click_config_provider(void* context)
{
    // 2 options: Up = value 1, Down = value 0.
    window_single_click_subscribe(BUTTON_ID_UP, choose_option_1);
    window_single_click_subscribe(BUTTON_ID_DOWN, choose_option_0);
}

static void three_option_click_config_provider(void* context)
{
    // 3 options: Up = value 2, Select = value 1, Down = value 0.
    window_single_click_subscribe(BUTTON_ID_UP, choose_option_2);
    window_single_click_subscribe(BUTTON_ID_SELECT, choose_option_1);
    window_single_click_subscribe(BUTTON_ID_DOWN, choose_option_0);
}

static void set_action_icon(ButtonId button, uint8_t icon_choice)
{
    GBitmap* bitmap = get_icon_by_choice(icon_choice);
    if(bitmap != NULL)
    {
        action_bar_layer_set_icon_animated(m_action_bar, button, bitmap, true);
    } else
    {
        action_bar_layer_clear_icon(m_action_bar, button);
    }
}

static void show_current_metric()
{
    Metrics* metric = m_metrics[m_current_index];
    m_current_value = 0;

    text_layer_set_text(m_title_layer, metric->title != NULL ? metric->title->value : "");
    bitmap_layer_set_bitmap(m_icon_layer, get_icon_by_choice(metric->main_icon));

    if(metric->type == MetricsType_INTERVAL)
    {
        set_action_icon(BUTTON_ID_UP, IconChoice_UP);
        set_action_icon(BUTTON_ID_SELECT, IconChoice_CHECK);
        set_action_icon(BUTTON_ID_DOWN, IconChoice_DOWN);
        action_bar_layer_set_click_config_provider(m_action_bar, interval_click_config_provider);
        update_value_display();
    } else if(metric->type == MetricsType_THREE_OPTION)
    {
        set_action_icon(BUTTON_ID_UP, metric->option_icons[2]);
        set_action_icon(BUTTON_ID_SELECT, metric->option_icons[1]);
        set_action_icon(BUTTON_ID_DOWN, metric->option_icons[0]);
        action_bar_layer_set_click_config_provider(m_action_bar, three_option_click_config_provider);
        text_layer_set_text(m_value_layer, "");
    } else
    {
        // BOOL (2 options)
        set_action_icon(BUTTON_ID_UP, metric->option_icons[1]);
        action_bar_layer_clear_icon(m_action_bar, BUTTON_ID_SELECT);
        set_action_icon(BUTTON_ID_DOWN, metric->option_icons[0]);
        action_bar_layer_set_click_config_provider(m_action_bar, bool_click_config_provider);
        text_layer_set_text(m_value_layer, "");
    }
}

void register_mood_start()
{
    collect_metrics();
    m_current_index = 0;

    APP_LOG(APP_LOG_LEVEL_INFO, "Registration started (%d metrics)", (int)m_metric_count);

    if(m_metric_count == 0)
    {
        finish_registration();
    } else
    {
        show_current_metric();
    }
}

void register_mood_set_group(MetricsGroup* group)
{
    m_group = group;
    m_single_metric = NULL;
}

void register_mood_set_metric(Metrics* metric)
{
    m_single_metric = metric;
    m_group = NULL;
}

void register_mood_set_layers(
    Window* window,
    ActionBarLayer* action_bar,
    TextLayer* title_layer,
    TextLayer* value_layer,
    BitmapLayer* icon_layer)
{
    m_window = window;
    m_action_bar = action_bar;
    m_title_layer = title_layer;
    m_value_layer = value_layer;
    m_icon_layer = icon_layer;
}

void register_mood_tear_down()
{
    if(m_metrics != NULL)
    {
        free(m_metrics);
        m_metrics = NULL;
    }
    m_metric_count = 0;
}
