#include "register_mood_window_logic.h"
#include "icons.h"
#include "persistance.h"

static Window* m_mood_window;
static ActionBarLayer* m_action_bar;
static BitmapLayer* m_icon_layer;
static TextLayer* m_score_layer

enum MeasurementPointType
{
    NONE,
    BOOL,
    INTERVAL,
};

typedef struct {
    uint8_t id;
    char* title;
    GBitmap* icon;
    MeasurementPointType type;
    uint8_t min;
    uint8_t max;
} MeasurementPoint;

static MeasurementPoint measurement_points[3] = {
    {
        .id = 1,
        .title = "Sleep",
        .type = INTERVAL,
        .min = 0,
        .max = 5,
        .icon = get_snooze_icon(),
    },
    {
        .id = 2,
        .title = "Exercise",
        .type = BOOL,
        .icon = get_exercise_icon(),
    },
    {
        .id = 3,
        .title = "Mood",
        .type = INTERVAL,
        .min = 0,
        .max = 5,
        .icon = get_mood_icon(),
    },
};

static uint8_t current_measurement_point_index = 0;
static MeasurementPoint* current_measurement_point = NULL;
static uint8_t current_score = 0;

static void increase_score(ClickRecognizerRef recognizer, void* context)
{
    if(current_score < current_measurement_point->max)
    {
        current_score++;
    }
}

static void register_score()
{
    set_last_score(current_measurement_point_index, current_score);
}

static void set_score_done(ClickRecognizerRef recognizer, void* context)
{
    register_score();
    setup_next_measurement_point();
}

static void decrease_score(ClickRecognizerRef recognizer, void* context)
{
    if(current_score > current_measurement_point->min)
    {
        current_score--;
    }
}

static void set_bool_done(ClickRecognizerRef recognizer, void* context)
{

}

static void set_bool_not_done(ClickRecognizerRef recognizer, void* context)
{

}

static void update_icon()
{
    bitmap_layer_set_bitmap(m_icon_layer, current_measurement_point->icon);
}

static void interval_click_handler(void* context)
{
    window_single_click_subscribe(BUTTON_ID_UP, increase_score);
    window_single_click_subscribe(BUTTON_ID_SELECT, set_score_done);
    window_single_click_subscribe(BUTTON_ID_DOWN, decrease_score);
}

static void bool_click_handler(void* context)
{
    window_single_click_subscribe(BUTTON_ID_UP, NULL);
    window_single_click_subscribe(BUTTON_ID_SELECT, set_bool_done);
    window_single_click_subscribe(BUTTON_ID_DOWN, set_bool_not_done);
}

static void update_actionbar()
{
    if(current_measurement_point->type == INTERVAL)
    {
        action_bar_layer_set_icon_animated(m_action_bar, BUTTON_ID_UP, get_up_icon(), true);
        action_bar_layer_set_icon_animated(m_action_bar, BUTTON_ID_SELECT, get_check_icon(), true);
        action_bar_layer_set_icon_animated(m_action_bar, BUTTON_ID_DOWN, get_down_icon(), true);
        action_bar_layer_set_click_config_provider(m_action_bar, interval_click_handler);
    } else if(current_measurement_point->type == BOOL)
    {
        action_bar_layer_clear_icon(m_action_bar, BUTTON_ID_UP);
        action_bar_layer_set_icon_animated(m_action_bar, BUTTON_ID_SELECT, get_check_icon(), true);
        action_bar_layer_set_icon_animated(m_action_bar, BUTTON_ID_DOWN, get_cross_icon(), true);
        action_bar_layer_set_click_config_provider(m_action_bar, bool_click_handler);
    }
}

static void update_score()
{
    if(current_measurement_point->type == INTERVAL)
    {
        get_last_score(current_measurement_point_index);
    }
}

void setup_next_measurement_point()
{
    if(current_measurement_point_index < ARRAY_LENGTH(measurement_points))
    {
        current_measurement_point = measurement_points[current_measurement_point_index++];
        update_icon();
        update_actionbar();
        update_score();
    } else
    {
        exit_reason_set(APP_EXIT_ACTION_PERFORMED_SUCCESSFULLY);
        window_stack_remove(m_mood_window, true);
    }

}

void expose_ui(Window* mood_window, ActionBarLayer* action_bar, BitmapLayer* icon_layer, TextLayer* score_layer)
{
    m_mood_window = mood_window;
    m_action_bar = action_bar;
    m_icon_layer = icon_layer;
    m_score_layer = score_layer;
}