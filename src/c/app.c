#include "app.h"

#include "config_menu_window.h"
#include "metrics_config_window.h"
#include "metric_group_config_window.h"
#include "repositories/app_config_repository.h"
#include "repositories/metrics_repository.h"
#include "repositories/string_repository.h"
#include "edit_alarm_window.h"
#include "alarm_window.h"

#include "icons.h"
#include "app_glance.h"
#include "scheduler.h"

static void wakeup_handler(WakeupId id, int32_t alarm_index)
{
    setup_alarm_window(alarm_index);
}

void init()
{
    if(config_first_start())
    {
        APP_LOG(APP_LOG_LEVEL_INFO, "Removing old wake ups");
        // Mostly for development
        // But old wake ups are not removed when reinstalling app
        // So we remove them so no collisions happen
        wakeup_cancel_all();
    }
    strings_init();
    metrics_init();

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
            setup_alarm_window(alarm_index);
        }
    } else
    {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Starting from non Wakeup");

        wakeup_service_subscribe(wakeup_handler);

        setup_config_window();
    }
}

void deinit()
{
    APP_LOG(APP_LOG_LEVEL_INFO, "Deiniting Mood");

    tear_down_config_window();
    tear_down_metric_group_config_window();
    tear_down_metrics_config_window();
    // tear_down_edit_alarm_window();
    // tear_down_alarm_window();

    // destroy_all_icons();
    // setup_app_glance();
}