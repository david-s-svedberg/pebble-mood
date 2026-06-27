#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pebble.h>

// Bump this whenever the AppConfig layout changes (and only append new fields
// to the end of AppConfig). config_init() re-seeds when the stored version or
// size no longer matches, so existing installs can't read a stale layout.
#define CURRENT_DATA_VERSION (3)

// Reserved Alarm.index values for the non-group alarms. They live in a uint8_t
// (Alarm.index) and are also used as the wakeup cookie (compared as int in
// app.c / alarm_window_logic), so they must fit in a uint8_t and stay outside
// the group-id range (group ids count up from 0).
#define SNOOZED_ALARM_ID (255)
#define SUMMER_TIME_ALARM_ID (254)

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
    DataKeys_GROUP_METRICS_META_DATA,
    DataKeys_GROUP_METRICS_DATA,
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

    // Append new fields below this line only, and bump CURRENT_DATA_VERSION.
    // The group whose alarm was snoozed, so the registration target survives
    // the app exit/relaunch that a snooze causes.
    uint16_t snoozed_group_id;
} AppConfig;

typedef enum
{
    MetricsType_NONE,
    MetricsType_BOOL,        // 2 options, answered with Up/Down
    MetricsType_INTERVAL,    // 0..max_value, stepped with Up/Down
    MetricsType_THREE_OPTION,// 3 options, answered with Up/Select/Down
} MetricsType;

// Max discrete answer options (BOOL uses 2, THREE_OPTION uses 3). Options are
// indexed by the value they store (0..N-1).
#define MAX_METRIC_OPTIONS (3)

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
    uint16_t        title_id;
    String*         title;
    MetricsType     type;
    uint8_t         max_value;
    // Large icon shown centered on the registration screen for quick
    // recognition (IconChoice, 0 = none).
    uint8_t         main_icon;
    // Per-option action-bar icon + optional interned text label, indexed by the
    // option value (BOOL uses [0..1], THREE_OPTION uses [0..2]). icon is an
    // IconChoice (0 = none); text_id is a String id (0 = none).
    uint8_t         option_icons[MAX_METRIC_OPTIONS];
    uint16_t        option_text_ids[MAX_METRIC_OPTIONS];
} Metrics;

typedef struct
{
    uint16_t    id;
    uint16_t    metrics_id;
    Metrics*    metric;
    uint8_t     value;
    time_t      time_stamp;
    // The group whose scheduled slot this answers (0 = spontaneous / no group).
    // Lets the same metric be tracked separately per group on a given day (e.g.
    // Mood in both Morning and Evening), and a Today answer update its own slot.
    uint16_t    group_id;
} Registration;

// Join row: a metric belongs to a group. A metric can have several of these
// (member of several groups) or none (unscheduled — registered only on demand).
typedef struct
{
    uint16_t    id;
    uint16_t    group_id;
    uint16_t    metric_id;
} GroupMetric;