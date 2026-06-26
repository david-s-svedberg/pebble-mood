#include "register_mood_window_logic.h"

#include <pebble.h>

#include "icons.h"
#include "repositories/metrics_repository.h"

static Window* m_window;
static ActionBarLayer* m_action_bar;
static TextLayer* m_title_layer;
static TextLayer* m_value_layer;

static MetricsGroup* m_group;
static Metrics** m_metrics = NULL;
static uint16_t m_metric_count = 0;
static uint16_t m_current_index = 0;
static uint8_t m_current_value = 0;

static char m_value_buffer[4];

static void show_current_metric();

static void collect_group_metrics()
{
    register_mood_tear_down();

    uint32_t total = metrics_count();
    m_metrics = (Metrics**)malloc(total * sizeof(Metrics*));
    m_metric_count = 0;

    Metrics* all_metrics = metrics_get_all();
    for(uint32_t i = 0; i < total; i++)
    {
        Metrics* current = &all_metrics[i];
        if(current->group_id == m_group->id)
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

static void bool_yes(ClickRecognizerRef recognizer, void* context)
{
    save_and_advance(1);
}

static void bool_no(ClickRecognizerRef recognizer, void* context)
{
    save_and_advance(0);
}

static void interval_click_config_provider(void* context)
{
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, interval_increase);
    window_single_click_subscribe(BUTTON_ID_SELECT, interval_confirm);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, interval_decrease);
}

static void bool_click_config_provider(void* context)
{
    window_single_click_subscribe(BUTTON_ID_SELECT, bool_yes);
    window_single_click_subscribe(BUTTON_ID_DOWN, bool_no);
}

static void show_current_metric()
{
    Metrics* metric = m_metrics[m_current_index];
    m_current_value = 0;

    text_layer_set_text(m_title_layer, metric->title != NULL ? metric->title->value : "");

    if(metric->type == MetricsType_INTERVAL)
    {
        action_bar_layer_set_icon_animated(m_action_bar, BUTTON_ID_UP, get_up_icon(), true);
        action_bar_layer_set_icon_animated(m_action_bar, BUTTON_ID_SELECT, get_check_icon(), true);
        action_bar_layer_set_icon_animated(m_action_bar, BUTTON_ID_DOWN, get_down_icon(), true);
        action_bar_layer_set_click_config_provider(m_action_bar, interval_click_config_provider);
        update_value_display();
    } else
    {
        action_bar_layer_clear_icon(m_action_bar, BUTTON_ID_UP);
        action_bar_layer_set_icon_animated(m_action_bar, BUTTON_ID_SELECT, get_check_icon(), true);
        action_bar_layer_set_icon_animated(m_action_bar, BUTTON_ID_DOWN, get_cross_icon(), true);
        action_bar_layer_set_click_config_provider(m_action_bar, bool_click_config_provider);
        text_layer_set_text(m_value_layer, "");
    }
}

void register_mood_start()
{
    collect_group_metrics();
    m_current_index = 0;

    APP_LOG(APP_LOG_LEVEL_INFO, "Registration started for group %d with %d metrics",
        m_group != NULL ? m_group->id : -1, (int)m_metric_count);

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
}

void register_mood_set_layers(
    Window* window,
    ActionBarLayer* action_bar,
    TextLayer* title_layer,
    TextLayer* value_layer)
{
    m_window = window;
    m_action_bar = action_bar;
    m_title_layer = title_layer;
    m_value_layer = value_layer;
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
