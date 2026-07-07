#include "alarm_window_logic.h"

#include <pebble.h>

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
static MetricsGroup* m_registering_group = NULL;
static bool m_alarm_silenced = false;

static uint16_t snooze_progression[] = {5, 10, 15, 30, 60, 90, 120};
static uint16_t m_snooze_minutes = 5;
static AppTimer* m_snooze_selection_done = NULL;
static AppTimer* m_silenced_timedout = NULL;
static AppTimer* m_alarm_timedout = NULL;

static const uint32_t segments[] = { 50 };
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

// Cancels a pending timer and clears its handle. Handles are NULLed when their
// callback fires (a fired AppTimer handle is invalid to cancel/reschedule), so
// a non-NULL handle here is always a live timer.
static void cancel_timer(AppTimer** timer)
{
    if(*timer != NULL)
    {
        app_timer_cancel(*timer);
        *timer = NULL;
    }
}

static void close_alarm()
{
    vibes_long_pulse();
    m_alarm_silenced = true;   // stops the self-rescheduling vibration loop
    cancel_timer(&m_alarm_timedout);
    cancel_timer(&m_snooze_selection_done);
    cancel_timer(&m_silenced_timedout);
    exit_reason_set(APP_EXIT_ACTION_PERFORMED_SUCCESSFULLY);
    window_stack_remove(m_alarm_window, true);
}


static void set_timeout()
{
    uint32_t timeout_milli_sec = (uint32_t)config_get_alarm_timeout() * SEC_IN_MS;
    bool rescheduled = false;
    if(m_alarm_timedout != NULL)
    {
        rescheduled = app_timer_reschedule(m_alarm_timedout, timeout_milli_sec);
    }
    if(!rescheduled)
    {
        m_alarm_timedout = app_timer_register(timeout_milli_sec, snooze_selection_done, &m_alarm_timedout);
    }
}

static void silence_timed_out(void* data)
{
    m_silenced_timedout = NULL;   // fired — handle is dead
    m_alarm_silenced = false;
    run_vibration(NULL);
    set_timeout();
}

static void silence_alarm_handler(ClickRecognizerRef recognizer, void* context)
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "silence alarm requested");
    vibes_cancel();
    cancel_timer(&m_alarm_timedout);
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
    cancel_timer(&m_alarm_timedout);
    cancel_timer(&m_snooze_selection_done);
    cancel_timer(&m_silenced_timedout);
    window_stack_remove(m_alarm_window, true);
    if(m_registering_group != NULL)
    {
        setup_register_mood_window(m_registering_group);
    }
}

static void snooze_selection_done(void* fired_handle)
{
    // Serves both the snooze-confirm timer and the alarm timeout; `fired_handle`
    // points at whichever handle just fired. Clear it — a fired AppTimer handle
    // must not be cancelled/rescheduled (close_alarm cancels the sibling, which
    // is still live, via its handle).
    if(fired_handle != NULL)
    {
        *(AppTimer**)fired_handle = NULL;
    }

    time_t t = time(NULL);
    t += (SECONDS_PER_MINUTE * m_snooze_minutes);
    // Persist which group is being registered so the snooze wakeup (which
    // relaunches the app) can reopen registration for the right group.
    if(m_registering_group != NULL)
    {
        config_set_snoozed_group_id(m_registering_group->id);
    }
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
    static char snooze_text_buffer[8];
    snprintf(snooze_text_buffer, sizeof(snooze_text_buffer), "%dm", m_snooze_minutes);
    text_layer_set_text(m_snooze_time_layer, snooze_text_buffer);
    m_alarm_silenced = true;

    vibes_short_pulse();

    bool rescheduled = false;
    if(m_snooze_selection_done != NULL)
    {
        rescheduled = app_timer_reschedule(m_snooze_selection_done, 1000);
    }
    if(!rescheduled)
    {
        m_snooze_selection_done = app_timer_register(1000, snooze_selection_done, &m_snooze_selection_done);
    }
}

static void back_alarm_handler(ClickRecognizerRef recognizer, void* context)
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "back pressed on alarm");
    // "Remind me in 5 minutes": same path as a snooze — through the scheduler
    // (collision handling, persisted wakeup id) with the group remembered so
    // the relaunch reopens the right registration.
    if(m_registering_group != NULL)
    {
        config_set_snoozed_group_id(m_registering_group->id);
    }
    schedule_snooze_alarm(m_wakup_alarm, time(NULL) + 5 * SECONDS_PER_MINUTE);
    config_save();
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

char* get_alarm_title()
{
    // Show which group's check-in this is; fall back to a generic prompt.
    if(m_registering_group != NULL && m_registering_group->title != NULL)
    {
        return m_registering_group->title->value;
    }
    return "Time to register";
}

bool setup_alarm_state(int32_t alarm_index)
{
    if(alarm_index == SNOOZED_ALARM_ID)
    {
        m_wakup_alarm = config_get_snooze_alarm();
        m_wakup_alarm->active = false;
        // The snooze alarm carries no group of its own, so recover the group
        // that was snoozed (persisted across the relaunch the snooze caused).
        // No rescheduling here: the group's own daily alarm covers tomorrow —
        // rescheduling the snooze alarm too would add a duplicate wakeup
        // 30-60s after the real one every day.
        m_registering_group = metrics_group_get(config_get_snoozed_group_id());
    } else {
        MetricsGroup* metric_group = metrics_group_get(alarm_index);
        if(metric_group == NULL)
        {
            // The group was deleted after this wakeup was scheduled; nothing to
            // register for. Don't show anything.
            APP_LOG(APP_LOG_LEVEL_WARNING, "alarm: group %d no longer exists, skipping", (int)alarm_index);
            return false;
        }
        m_wakup_alarm = &metric_group->alarm;
        m_registering_group = metric_group;
        // Re-arm this group's alarm for tomorrow right away, so it survives
        // whatever happens to this registration session.
        schedule_alarm(m_wakup_alarm);
    }

    config_save();
    run_vibration(NULL);
    set_timeout();
    return true;
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