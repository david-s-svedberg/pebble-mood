#include "app_config_repository.h"

#include "../data.h"

static AppConfig m_app_config;

bool config_first_start()
{
    bool is_first_start = !persist_exists(DataKeys_FIRST_START);
    if(is_first_start)
    {
        persist_write_bool(DataKeys_FIRST_START, false);
    }
    return is_first_start;
}

void config_init()
{
    if(!persist_exists(DataKeys_APP_CONFIG))
    {
        AppConfig config =
        {
            .alarm_timeout_sec = SECONDS_PER_MINUTE,
            .background_color = GColorBlack,
            .foreground_color = GColorWhite,
            .data_version = CURRENT_DATA_VERSION,
            .snooze_alarm =
            {
                .active = false,
                .index = SNOOZED_ALARM_ID,
                .time =
                {
                    .hour = 0,
                    .minute = 0,
                },
                .wakeup_id = 0,
            },
            .summer_time_alarm =
            {
                .active = true,
                .index = SUMMER_TIME_ALARM_ID,
                .time =
                {
                    .hour = 3,
                    .minute = 31,
                },
                .wakeup_id = 0,
            },
        };
        persist_write_data(DataKeys_APP_CONFIG, &config, sizeof(config));
    }
    persist_read_data(DataKeys_APP_CONFIG, &m_app_config, sizeof(m_app_config));
}

AppConfig* config_get()
{
    return &m_app_config;
}

void config_save()
{
    persist_write_data(DataKeys_APP_CONFIG, &m_app_config, sizeof(m_app_config));
}

GColor8 config_get_background_color()
{
    return m_app_config.background_color;
}

GColor8 config_get_foreground_color()
{
    return m_app_config.foreground_color;
}

uint8_t config_get_alarm_timeout()
{
    return m_app_config.alarm_timeout_sec;
}

bool config_is_dark_theme()
{
    uint8_t background = config_get_background_color().argb;
    uint8_t black = GColorBlack.argb;
    return (background == black);
}

void config_toggle_theme()
{
    GColor8 previous_background_color = m_app_config.background_color;
    GColor8 previous_foreground_color = m_app_config.foreground_color;

    m_app_config.background_color = previous_foreground_color;
    m_app_config.foreground_color = previous_background_color;

    config_save();
}

Alarm* config_get_snooze_alarm()
{
    return &m_app_config.snooze_alarm;
}

Alarm* config_get_summer_time_alarm()
{
    return &m_app_config.summer_time_alarm;
}

void config_set_alarm_timeout(uint8_t sec)
{
    m_app_config.alarm_timeout_sec = sec;
    config_save();
}