#include "config_apply.h"

#include "message_keys.auto.h"
#include "data.h"
#include "scheduler.h"
#include "icons.h"
#include "repositories/metrics_repository.h"

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
    if(group->alarm.active)
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

bool config_apply_handle(DictionaryIterator* iter)
{
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
