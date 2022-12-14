#include "alarm_window_logic.h"

#include <pebble.h>

#include "persistance.h"
#include "format.h"
#include "scheduler.h"
#include "register_mood_window.h"
#include "repositories/app_config_repository.h"
#include "repositories/metrics_repository.h"

static Window *m_alarm_window;

static TextLayer *m_alarm_time_layer;
static TextLayer *m_snooze_time_layer;

static ActionBarLayer* m_alarm_window_action_bar_layer;

static Alarm* m_wakup_alarm;
static bool m_alarm_silenced = false;

static uint16_t snooze_progression[] = {5, 10, 15, 30, 60, 90, 120};
static uint16_t m_snooze_minutes = 5;
static AppTimer* m_snooze_selection_done = NULL;
static AppTimer* m_silenced_timedout = NULL;
static AppTimer* m_alarm_timedout = NULL;

static const uint32_t const segments[] = { 50 };
static const VibePattern m_vibration_pattern =
{
    .durations = segments,
    .num_segments = ARRAY_LENGTH(segments),
};

#define SEC_IN_MS (1000)

static const uint32_t FIVE_MINUTES_IN_MS = 5 * SECONDS_PER_MINUTE * SEC_IN_MS;

static void snooze_selection_done(void* data);

static void run_vibration(void* data)
{
    if(!m_alarm_silenced)
    {
        vibes_enqueue_custom_pattern(m_vibration_pattern);
        app_timer_register(2000, run_vibration, NULL);
    }
}

static void close_alarm()
{
    vibes_long_pulse();
    if(m_alarm_timedout != NULL)
    {
        app_timer_cancel(m_alarm_timedout);
    }
    exit_reason_set(APP_EXIT_ACTION_PERFORMED_SUCCESSFULLY);
    window_stack_remove(m_alarm_window, true);
}


static void set_timeout()
{
    uint16_t timeout_milli_sec = config_get_alarm_timeout() * SEC_IN_MS;
    bool rescheduled = false;
    if(m_alarm_timedout != NULL)
    {
        rescheduled = app_timer_reschedule(m_alarm_timedout, timeout_milli_sec);
    }
    if(!rescheduled)
    {
        m_alarm_timedout = app_timer_register(timeout_milli_sec, snooze_selection_done, NULL);
    }
}

static void silence_timed_out(void* data)
{
    m_alarm_silenced = false;
    run_vibration(NULL);
    set_timeout();
}

static void silence_alarm_handler(ClickRecognizerRef recognizer, void* context)
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "silence alarm requested");
    vibes_cancel();
    app_timer_cancel(m_alarm_timedout);
    m_alarm_silenced = true;
    bool rescheduled = false;
    if(m_silenced_timedout != NULL)
    {
        rescheduled = app_timer_reschedule(m_silenced_timedout, FIVE_MINUTES_IN_MS);
    }
    if(!rescheduled)
    {
        m_silenced_timedout = app_timer_register(FIVE_MINUTES_IN_MS, silence_timed_out, NULL);
    }
}

static void register_mood_handler(ClickRecognizerRef recognizer, void* context)
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "register mood selected");
    vibes_cancel();
    m_alarm_silenced = true;
    if(m_alarm_timedout != NULL)
    {
        app_timer_cancel(m_alarm_timedout);
    }
    window_stack_remove(m_alarm_window, true);
    //setup_register_mood_window();
}

static void snooze_selection_done(void* data)
{
    time_t t = time(NULL);
    t += (SECONDS_PER_MINUTE * m_snooze_minutes);
    schedule_snooze_alarm(m_wakup_alarm, t);
    config_save();
    close_alarm();
}

static uint8_t m_clicks = 0;

static void snooze_alarm_handler(ClickRecognizerRef recognizer, void* context)
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "snooze alarm requested");

    m_clicks++;
    m_snooze_minutes = snooze_progression[(m_clicks - 1) % ARRAY_LENGTH(snooze_progression)];
    static char snooze_text_buffer[6];
    snprintf(snooze_text_buffer, sizeof(snooze_text_buffer), "%dm", m_snooze_minutes);
    text_layer_set_text(m_snooze_time_layer, snooze_text_buffer);
    m_alarm_silenced = true;

    vibes_short_pulse();

    if(m_snooze_selection_done != NULL)
    {
        app_timer_reschedule(m_snooze_selection_done, 1000);
    } else
    {
        m_snooze_selection_done = app_timer_register(1000, snooze_selection_done, NULL);
    }
}

static void back_alarm_handler(ClickRecognizerRef recognizer, void* context)
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "back pressed on alarm");
    time_t t = time(NULL);
    t += (SECONDS_PER_MINUTE * 5);
    wakeup_schedule(t, m_wakup_alarm->index, true);
    close_alarm();
}


void alarm_window_click_config_provider(void* context)
{
    window_single_click_subscribe(BUTTON_ID_UP, silence_alarm_handler);
    window_single_click_subscribe(BUTTON_ID_BACK, back_alarm_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, register_mood_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 500, snooze_alarm_handler);
}

char* get_wakeup_alarm_time_string()
{
    static char alarm_time_buffer[6];
    fill_time_string(alarm_time_buffer, m_wakup_alarm->time.hour, m_wakup_alarm->time.minute);
    return alarm_time_buffer;
}

void setup_alarm_state(int32_t alarm_index)
{
    if(alarm_index == SNOOZED_ALARM_ID)
    {
        m_wakup_alarm = config_get_snooze_alarm();
        m_wakup_alarm->active = false;
    } else {
        MetricsGroup* metric_group = metrics_group_get(alarm_index);
        m_wakup_alarm = &metric_group->alarm;
    }

    schedule_alarm(m_wakup_alarm);
    config_save();
    run_vibration(NULL);
    set_timeout();
}

void set_alarm_layers(
    TextLayer* alarm_time_layer,
    TextLayer* snooze_time_layer,
    ActionBarLayer* alarm_window_action_bar_layer)
{
    m_alarm_time_layer = alarm_time_layer;
    m_snooze_time_layer = snooze_time_layer;
    m_alarm_window_action_bar_layer = alarm_window_action_bar_layer;
}

void set_alarm_window(Window* alarm_window)
{
    m_alarm_window = alarm_window;
}