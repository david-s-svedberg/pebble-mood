#include "app.h"

#include "config_menu_window.h"
#include "main_window.h"
#include "repositories/app_config_repository.h"
#include "repositories/metrics_repository.h"
#include "repositories/string_repository.h"
#include "edit_alarm_window.h"
#include "alarm_window.h"

#include "icons.h"
#include "app_glance.h"
#include "scheduler.h"
#include "data_export.h"

static void wakeup_handler(WakeupId id, int32_t alarm_index)
{
    setup_alarm_window(alarm_index);
}

void init()
{
    bool first_start = config_first_start();
    if(first_start)
    {
        APP_LOG(APP_LOG_LEVEL_INFO, "Removing old wake ups");
        // Mostly for development
        // But old wake ups are not removed when reinstalling app
        // So we remove them so no collisions happen
        wakeup_cancel_all();
    }
    config_init();
    strings_init();
    metrics_init();

    if(first_start)
    {
        // Seed a usable default set and schedule its group alarms. The flag is
        // written only once seeding completed, so a crash mid-seed retries on
        // the next launch instead of leaving an empty app.
        metrics_seed_defaults();
        ensure_all_alarms_scheduled();
        config_mark_started();
    }

    if(launch_reason() == APP_LAUNCH_WAKEUP)
    {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Starting from Wakeup");

        WakeupId id = 0;
        int32_t alarm_index = 0;
        wakeup_get_launch_event(&id, &alarm_index);

        if(alarm_index == SUMMER_TIME_ALARM_ID)
        {
            wakeup_cancel_all();
            ensure_all_alarms_scheduled();
        } else
        {
            // Group / snooze alarm. setup_alarm_window re-arms the alarm for
            // tomorrow and skips the UI (no window, no vibration) when the group
            // is already fully answered today — so a completed group exits
            // quietly here just as it stays quiet when the app is already open.
            setup_alarm_window(alarm_index);
        }
    } else
    {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Starting from non Wakeup");

        // Revive any dead group alarms. Rescheduling normally happens when an
        // alarm fires, so a wakeup that expired while the watch was off would
        // otherwise never come back. Idempotent: already-scheduled alarms are
        // left untouched (wakeup_query guard).
        ensure_all_alarms_scheduled();

        wakeup_service_subscribe(wakeup_handler);

        // Open the AppMessage channel so a connected phone can pull
        // registrations for the companion app (see data_export.c / pkjs).
        data_export_init();

        setup_main_window();
    }
}

void deinit()
{
    APP_LOG(APP_LOG_LEVEL_INFO, "Deiniting Mood");

    // Windows are intentionally not destroyed: the OS reclaims the whole heap
    // at exit, and the reuse-one-Window pattern keeps them alive on purpose.
    setup_app_glance();     // launcher glance: "Next time: HH:MM"
    destroy_all_icons();
}