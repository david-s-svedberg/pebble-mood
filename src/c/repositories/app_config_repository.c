#include "app_config_repository.h"

#include "../data.h"
#include "persist_blob.h"

static AppConfig m_app_config;

bool config_first_start()
{
    return !persist_exists(DataKeys_FIRST_START);
}

void config_mark_started()
{
    persist_write_bool(DataKeys_FIRST_START, false);
}

static void seed_default_config()
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

void config_init()
{
    // Only trust a stored config that matches the current size AND version.
    // A layout change (e.g. an added field) shifts later fields like the
    // theme colors; reading such a blob would yield garbage (a transparent
    // GColorClear background), so we re-seed defaults instead.
    bool valid = persist_exists(DataKeys_APP_CONFIG)
        && persist_get_size(DataKeys_APP_CONFIG) == (int)sizeof(AppConfig);
    if(valid)
    {
        persist_read_data(DataKeys_APP_CONFIG, &m_app_config, sizeof(m_app_config));
        valid = (m_app_config.data_version == CURRENT_DATA_VERSION);
    }
    if(!valid)
    {
        seed_default_config();
        persist_read_data(DataKeys_APP_CONFIG, &m_app_config, sizeof(m_app_config));
    }
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

uint16_t config_get_snoozed_group_id()
{
    return m_app_config.snoozed_group_id;
}

bool config_alarms_suspended()
{
    return m_app_config.alarms_suspended;
}

void config_set_alarms_suspended(bool suspended)
{
    m_app_config.alarms_suspended = suspended;
    config_save();
}

uint8_t config_favorite_count()
{
    uint8_t count = 0;
    for(uint8_t i = 0; i < MAX_FAVORITES; i++)
    {
        if(m_app_config.favorite_metrics[i] != 0) count++;
    }
    return count;
}

bool config_is_favorite(uint16_t metric_id)
{
    if(metric_id == 0) return false;
    for(uint8_t i = 0; i < MAX_FAVORITES; i++)
    {
        if(m_app_config.favorite_metrics[i] == metric_id) return true;
    }
    return false;
}

bool config_toggle_favorite(uint16_t metric_id)
{
    if(metric_id == 0) return false;
    for(uint8_t i = 0; i < MAX_FAVORITES; i++)
    {
        if(m_app_config.favorite_metrics[i] == metric_id)
        {
            m_app_config.favorite_metrics[i] = 0;   // remove
            config_save();
            return true;
        }
    }
    for(uint8_t i = 0; i < MAX_FAVORITES; i++)
    {
        if(m_app_config.favorite_metrics[i] == 0)
        {
            m_app_config.favorite_metrics[i] = metric_id;   // add
            config_save();
            return true;
        }
    }
    return false;   // full
}

void config_get_favorites(uint16_t* out)
{
    for(uint8_t i = 0; i < MAX_FAVORITES; i++)
    {
        out[i] = m_app_config.favorite_metrics[i];
    }
}

void config_prune_favorite(uint16_t deleted_metric_id)
{
    if(config_is_favorite(deleted_metric_id))
    {
        config_toggle_favorite(deleted_metric_id);   // removes it
    }
}

void config_factory_reset()
{
    // Delete every persisted store. Clearing FIRST_START means the next launch
    // takes the first-start path and re-seeds defaults from scratch — so the
    // caller should exit the app right after (no in-memory re-init dance).
    persist_delete(DataKeys_APP_CONFIG);
    persist_delete(DataKeys_FIRST_START);
    persist_delete(DataKeys_STRING_META_DATA);
    persist_delete(DataKeys_METRICS_GROUP_META_DATA);
    persist_delete(DataKeys_METRICS_META_DATA);
    persist_delete(DataKeys_REGISTRATIONS_META_DATA);
    persist_delete(DataKeys_GROUP_METRICS_META_DATA);

    persist_blob_delete(DataKeys_STRINGS_DATA);
    persist_blob_delete(DataKeys_STRING_CHAR_DATA);
    persist_blob_delete(DataKeys_METRICS_GROUP_DATA);
    persist_blob_delete(DataKeys_METRICS_DATA);
    persist_blob_delete(DataKeys_REGISTRATIONS_DATA);
    persist_blob_delete(DataKeys_GROUP_METRICS_DATA);

    APP_LOG(APP_LOG_LEVEL_INFO, "factory reset: all stores wiped");
}

void config_set_snoozed_group_id(uint16_t group_id)
{
    m_app_config.snoozed_group_id = group_id;
    config_save();
}