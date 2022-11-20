#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pebble.h>

#define CURRENT_DATA_VERSION (1)

#define SNOOZED_ALARM_ID (-1)
#define SUMMER_TIME_ALARM_ID (-2)

typedef enum {
    DataKeys_NONE,
    DataKeys_STRING_META_DATA,
    DataKeys_STRINGS_DATA,
    DataKeys_STRING_CHAR_DATA,
    DataKeys_METRICS_GROUP_META_DATA,
    DataKeys_METRICS_GROUP_DATA,
    DataKeys_METRICS_META_DATA,
    DataKeys_METRICS_DATA,
    DataKeys_REGISTRATIONS_META_DATA,
    DataKeys_REGISTRATIONS_DATA,
    DataKeys_APP_CONFIG,
    DataKeys_FIRST_START,
} DataKeys;

typedef struct {
    uint8_t hour;
    uint8_t minute;
} TimeOfDay;

typedef struct {
    uint8_t index;
    TimeOfDay time;
    bool active;
    WakeupId wakeup_id;
} Alarm;

typedef struct {
    uint8_t data_version;

    Alarm snooze_alarm;
    Alarm summer_time_alarm;
    uint8_t alarm_timeout_sec;

    GColor8 background_color;
    GColor8 foreground_color;
} AppConfig;

typedef enum
{
    MetricsType_NONE,
    MetricsType_BOOL,
    MetricsType_INTERVAL,
} MetricsType;

typedef struct {
    uint16_t id;
    size_t length;
    char* value;
} String;

typedef struct
{
    uint16_t id;
    Alarm    alarm;
    uint16_t title_id;
    String*  title;
} MetricsGroup;

typedef struct
{
    uint16_t        id;
    uint16_t        group_id;
    MetricsGroup*   group;
    uint16_t        title_id;
    String*         title;
    MetricsType     type;
    uint8_t         max_value;
} Metrics;

typedef struct
{
    uint16_t    id;
    uint16_t    metrics_id;
    Metrics*    metric;
    uint8_t     value;
    time_t      time_stamp;
} Registration;