#include "config_apply.h"

#include "message_keys.auto.h"
#include "data.h"
#include "scheduler.h"
#include "icons.h"
#include "repositories/metrics_repository.h"
#include "repositories/app_config_repository.h"

// Sets a group's membership to exactly the given metric-id list.
static void set_members(uint16_t group_id, const uint8_t* wanted, uint16_t wanted_count)
{
    Metrics* all_metrics = metrics_get_all();
    for(uint32_t i = 0; i < metrics_count(); i++)
    {
        uint16_t metric_id = all_metrics[i].id;
        bool want = false;
        for(uint16_t w = 0; w < wanted_count; w++)
        {
            if(wanted[w] == metric_id)
            {
                want = true;
                break;
            }
        }
        if(metrics_group_has_metric(group_id, metric_id) != want)
        {
            metrics_group_toggle_metric(group_id, metric_id);
        }
    }
}

static bool title_differs(String* title, const char* name)
{
    return title == NULL || strcmp(title->value, name) != 0;
}

static void apply_group(DictionaryIterator* iter, uint16_t group_id)
{
    MetricsGroup* group;
    if(group_id == 0)
    {
        group = metrics_group_new();
        APP_LOG(APP_LOG_LEVEL_INFO, "config: created group %d", group->id);
    } else
    {
        group = metrics_group_get(group_id);
        if(group == NULL)
        {
            APP_LOG(APP_LOG_LEVEL_WARNING, "config: unknown group %d, ignoring", group_id);
            return;
        }
    }
    uint16_t id = group->id;

    Tuple* name = dict_find(iter, MESSAGE_KEY_SET_GROUP_NAME);
    if(name != NULL && title_differs(group->title, name->value->cstring))
    {
        metrics_groups_set_title(group, name->value->cstring);
        group = metrics_group_get(id);   // set_title mutates the string store
    }

    Tuple* hour = dict_find(iter, MESSAGE_KEY_SET_GROUP_HOUR);
    Tuple* minute = dict_find(iter, MESSAGE_KEY_SET_GROUP_MINUTE);
    if(hour != NULL) group->alarm.time.hour = hour->value->uint8 % 24;
    if(minute != NULL) group->alarm.time.minute = minute->value->uint8 % 60;

    Tuple* active = dict_find(iter, MESSAGE_KEY_SET_GROUP_ACTIVE);
    if(active != NULL) group->alarm.active = active->value->uint8 != 0;

    // Time or active state may have changed: re-arm (or cancel) the wakeup.
    // In phone mode the companion owns reminders, so never arm here (but keep
    // the desired alarm.active on record for when the watch takes over again).
    if(group->alarm.active && !config_alarms_suspended())
    {
        reschedule_alarm(&group->alarm);
    } else
    {
        unschedule_alarm(&group->alarm);
    }
    metrics_groups_save();

    Tuple* members = dict_find(iter, MESSAGE_KEY_SET_GROUP_MEMBERS);
    if(members != NULL)
    {
        set_members(id, members->value->data, members->length);
    }

    APP_LOG(APP_LOG_LEVEL_INFO, "config: applied group %d (%02d:%02d active=%d)",
        id, group->alarm.time.hour, group->alarm.time.minute, group->alarm.active);
}

static void apply_metric(DictionaryIterator* iter, uint16_t metric_id)
{
    Metrics* metric;
    if(metric_id == 0)
    {
        metric = metrics_new();
        Tuple* type = dict_find(iter, MESSAGE_KEY_SET_METRIC_TYPE);
        if(type != NULL)
        {
            metric->type = (MetricsType)type->value->uint8;
            if(metric->type == MetricsType_THREE_OPTION)
            {
                metric->option_icons[0] = IconChoice_FACE_SAD;
                metric->option_icons[1] = IconChoice_FACE_NEUTRAL;
                metric->option_icons[2] = IconChoice_FACE_HAPPY;
            }
        }
        APP_LOG(APP_LOG_LEVEL_INFO, "config: created metric %d", metric->id);
    } else
    {
        metric = metrics_get(metric_id);
        if(metric == NULL)
        {
            APP_LOG(APP_LOG_LEVEL_WARNING, "config: unknown metric %d, ignoring", metric_id);
            return;
        }
    }
    uint16_t id = metric->id;

    Tuple* name = dict_find(iter, MESSAGE_KEY_SET_METRIC_NAME);
    if(name != NULL && title_differs(metric->title, name->value->cstring))
    {
        metrics_set_title(metric, name->value->cstring);
        metric = metrics_get(id);   // set_title mutates the string store
    }

    if(metric->type == MetricsType_INTERVAL)
    {
        Tuple* min = dict_find(iter, MESSAGE_KEY_SET_METRIC_MIN);
        Tuple* max = dict_find(iter, MESSAGE_KEY_SET_METRIC_MAX);
        if(min != NULL) metric->min_value = (min->value->uint8 > 1) ? 1 : min->value->uint8;
        if(max != NULL) metric->max_value = max->value->uint8;
        if(metric->max_value < metric->min_value + 1) metric->max_value = metric->min_value + 1;
        if(metric->max_value > 10) metric->max_value = 10;
    }

    Tuple* icon = dict_find(iter, MESSAGE_KEY_SET_METRIC_MAIN_ICON);
    if(icon != NULL && icon->value->uint8 < IconChoice_COUNT)
    {
        metric->main_icon = icon->value->uint8;
    }

    metrics_save();
    APP_LOG(APP_LOG_LEVEL_INFO, "config: applied metric %d", id);
}

// Applies a registration answered on the phone (phone mode). Updates the
// existing slot for this (group, metric) today if there is one, else appends —
// mirroring how the watch's own registration flow behaves, so "answered today"
// and the group-complete check stay correct wherever the answer came from.
static void apply_registration(DictionaryIterator* iter, uint16_t metric_id)
{
    Tuple* group = dict_find(iter, MESSAGE_KEY_SET_REG_GROUP_ID);
    Tuple* value = dict_find(iter, MESSAGE_KEY_SET_REG_VALUE);
    Tuple* stamp = dict_find(iter, MESSAGE_KEY_SET_REG_TIMESTAMP);
    if(value == NULL) return;

    uint16_t group_id = (group != NULL) ? group->value->uint16 : 0;
    uint8_t val = value->value->uint8;

    Registration* existing = registration_today_for_group_metric(group_id, metric_id);
    if(existing != NULL)
    {
        registration_update(existing, val);
    } else
    {
        Registration registration = {
            .metrics_id = metric_id,
            .value = val,
            .time_stamp = (stamp != NULL) ? (time_t)stamp->value->int32 : time(NULL),
            .group_id = group_id,
        };
        registration_add(&registration);
    }
    APP_LOG(APP_LOG_LEVEL_INFO, "config: applied phone registration metric %d group %d = %d",
        metric_id, group_id, val);
}

bool config_apply_handle(DictionaryIterator* iter)
{
    Tuple* suspend = dict_find(iter, MESSAGE_KEY_SET_ALARMS_SUSPENDED);
    if(suspend != NULL)
    {
        config_set_alarms_suspended(suspend->value->uint8 != 0);
        ensure_all_alarms_scheduled();   // cancel or restore group wakeups
        APP_LOG(APP_LOG_LEVEL_INFO, "config: alarms_suspended=%d", suspend->value->uint8);
        return true;
    }

    Tuple* reg_metric = dict_find(iter, MESSAGE_KEY_SET_REG_METRIC_ID);
    if(reg_metric != NULL)
    {
        apply_registration(iter, (uint16_t)reg_metric->value->uint16);
        return true;
    }

    Tuple* group_id = dict_find(iter, MESSAGE_KEY_SET_GROUP_ID);
    if(group_id != NULL)
    {
        apply_group(iter, (uint16_t)group_id->value->uint16);
        return true;
    }

    Tuple* metric_id = dict_find(iter, MESSAGE_KEY_SET_METRIC_ID);
    if(metric_id != NULL)
    {
        apply_metric(iter, (uint16_t)metric_id->value->uint16);
        return true;
    }

    return false;
}
