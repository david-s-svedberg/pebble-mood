#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pebble.h>

#define CURRENT_DATA_VERSION (1)

#define SNOOZED_ALARM_ID (1)
#define SUMMER_TIME_ALARM_ID (2)

enum DataKeys {
    DataKeys_NONE,
    DataKeys_STRING_META_DATA,
    DataKeys_STRINGS_DATA,
    DataKeys_STRING_CHAR_DATA,
    DataKeys_METRICS_GROUP_META_DATA,
    DataKeys_METRICS_GROUP_DATA,
    DataKeys_METRICS_META_DATA,
    DataKeys_METRICS_DATA,
}

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



enum MetricType {
    NONE,
    Interval,
    Bool
}

typedef struct {
    uint8_t index;
    uint8_t group_id;
    Alarm alarm;
    String name;
    MetricType type;
} Metric;

typedef struct {
    uint8_t data_version;

    Alarm snooze_alarm;
    Alarm summer_time_alarm;

    GColor8 background_color;
    GColor8 foreground_color;

    uint8_t number_of_groups;
    uint8_t number_of_strings;
    uint8_t number_of_metrics;


    uint8_t next_group_id;
    uint8_t next_metric_id;
    uint8_t next_string_id;
} Data;