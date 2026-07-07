#include "metrics_repository.h"

#include "../data.h"
#include "../icons.h"
#include "dynamic_repository.h"

// next_id starts at 1 across these stores so that id 0 is a reserved sentinel
// ("no group / spontaneous", "no metric / placeholder row"), the same convention
// string_repository uses for "no string".
static DynamicData m_metrics_groups =
{
    .meta_data_storage_key = DataKeys_METRICS_GROUP_META_DATA,
    .items_storage_key = DataKeys_METRICS_GROUP_DATA,
    .item_size = sizeof(MetricsGroup),
    .number_of_items = 0,
    .next_id = 1,
    .items = NULL,
};

static DynamicData m_metrics =
{
    .meta_data_storage_key = DataKeys_METRICS_META_DATA,
    .items_storage_key = DataKeys_METRICS_DATA,
    .item_size = sizeof(Metrics),
    .number_of_items = 0,
    .next_id = 1,
    .items = NULL,
};

static DynamicData m_registrations =
{
    .meta_data_storage_key = DataKeys_REGISTRATIONS_META_DATA,
    .items_storage_key = DataKeys_REGISTRATIONS_DATA,
    .item_size = sizeof(Registration),
    .number_of_items = 0,
    .next_id = 1,
    .items = NULL,
};

// Many-to-many membership between groups and metrics (a metric may be in
// several groups, or none).
static DynamicData m_group_metrics =
{
    .meta_data_storage_key = DataKeys_GROUP_METRICS_META_DATA,
    .items_storage_key = DataKeys_GROUP_METRICS_DATA,
    .item_size = sizeof(GroupMetric),
    .number_of_items = 0,
    .next_id = 1,
    .items = NULL,
};

static void set_group_metric_id(uint16_t id, byte* item)
{
    ((GroupMetric*)item)->id = id;
}

static uint16_t get_group_metric_id(byte* item)
{
    return ((GroupMetric*)item)->id;
}

static GroupMetric* group_metric_at(int i)
{
    return (GroupMetric*)&m_group_metrics.items[i * m_group_metrics.item_size];
}

static void set_group_id(uint16_t id, byte* item)
{
    MetricsGroup* metrics_group = (MetricsGroup*)item;
    metrics_group->id = id;
    metrics_group->alarm.index = id;
}

static uint16_t get_group_id(byte* item)
{
    return ((MetricsGroup*)item)->id;
}

static bool group_is_same_id(uint16_t id, byte* item)
{
    return ((MetricsGroup*)item)->id == id;
}

static void set_metrics_id(uint16_t id, byte* item)
{
    Metrics* metric = (Metrics*)item;
    metric->id = id;
}

static uint16_t get_metrics_id(byte* item)
{
    return ((Metrics*)item)->id;
}

static bool metrics_is_same_id(uint16_t id, byte* item)
{
    return ((Metrics*)item)->id == id;
}

static void set_registration_id(uint16_t id, byte* item)
{
    ((Registration*)item)->id = id;
}

static uint16_t get_registration_id(byte* item)
{
    return ((Registration*)item)->id;
}

// Re-points every cached title at the string store. Must run after ANY string
// mutation: string_add/string_delete realloc the String array (dynamic_add/
// dynamic_delete free the old one), so every String* handed out before the
// mutation dangles — including the titles cached on all OTHER metrics/groups.
static void reconnect_titles()
{
    for(int i = 0; i < m_metrics_groups.number_of_items; i++)
    {
        MetricsGroup* current_group = (MetricsGroup*)&m_metrics_groups.items[i * m_metrics_groups.item_size];
        current_group->title = string_get(current_group->title_id);
    }

    for(int i = 0; i < m_metrics.number_of_items; i++)
    {
        Metrics* current_metrics = (Metrics*)&m_metrics.items[i * m_metrics.item_size];
        current_metrics->title = string_get(current_metrics->title_id);
    }
}

void metrics_init()
{
    dynamic_init(&m_metrics_groups);
    dynamic_init(&m_metrics);
    dynamic_init(&m_registrations);
    dynamic_init(&m_group_metrics);

    reconnect_titles();
}

void metrics_tear_down()
{
    free(m_metrics_groups.items);
    free(m_metrics.items);
    free(m_registrations.items);
    free(m_group_metrics.items);
}

bool metrics_group_has_metric(uint16_t group_id, uint16_t metric_id)
{
    for(int i = 0; i < m_group_metrics.number_of_items; i++)
    {
        GroupMetric* gm = group_metric_at(i);
        if(gm->group_id == group_id && gm->metric_id == metric_id)
        {
            return true;
        }
    }
    return false;
}

void metrics_group_toggle_metric(uint16_t group_id, uint16_t metric_id)
{
    for(int i = 0; i < m_group_metrics.number_of_items; i++)
    {
        GroupMetric* gm = group_metric_at(i);
        if(gm->group_id == group_id && gm->metric_id == metric_id)
        {
            dynamic_delete(gm->id, &m_group_metrics, get_group_metric_id);
            return;
        }
    }
    GroupMetric new_membership =
    {
        .group_id = group_id,
        .metric_id = metric_id,
    };
    dynamic_add(&m_group_metrics, (byte*)&new_membership, set_group_metric_id);
}

// Drops every membership row matching the predicate (group or metric removed).
static void remove_memberships(bool by_group, uint16_t id)
{
    bool removed = true;
    while(removed)
    {
        removed = false;
        for(int i = 0; i < m_group_metrics.number_of_items; i++)
        {
            GroupMetric* gm = group_metric_at(i);
            if((by_group && gm->group_id == id) || (!by_group && gm->metric_id == id))
            {
                dynamic_delete(gm->id, &m_group_metrics, get_group_metric_id);
                removed = true;
                break;
            }
        }
    }
}

MetricsGroup* metrics_group_get(const uint16_t id)
{
    return (MetricsGroup*)dynamic_get(id, &m_metrics_groups, group_is_same_id);
}

MetricsGroup* metrics_groups_get_all()
{
    return (MetricsGroup*)m_metrics_groups.items;
}

uint16_t metrics_group_add(MetricsGroup* metrics_group)
{
    dynamic_add(&m_metrics_groups, (byte*)metrics_group, set_group_id);
    return metrics_group->id;
}

void metrics_group_delete(const uint16_t delete_id)
{
    // Cancel the group's scheduled wakeup first, or it fires later with a group
    // id that no longer resolves (crash in the wakeup launch path). Raw wakeup
    // call rather than scheduler.h to keep the layering repo <- scheduler.
    MetricsGroup* group = metrics_group_get(delete_id);
    if(group != NULL && group->alarm.wakeup_id > 0)
    {
        wakeup_cancel(group->alarm.wakeup_id);
    }
    remove_memberships(true, delete_id);
    dynamic_delete(delete_id, &m_metrics_groups, get_group_id);
}

uint32_t metrics_groups_count()
{
    return m_metrics_groups.number_of_items;
}

void metrics_set_title(Metrics* metrics, char* title)
{
    if(metrics->title != NULL)
    {
        string_delete(metrics->title_id);
    }

    String* new_string = string_add(title);
    metrics->title_id = new_string->id;
    metrics->title = new_string;
    reconnect_titles();
    metrics_save();
}

void metrics_set_option_text(Metrics* metrics, uint8_t option, char* text)
{
    if(option >= MAX_METRIC_OPTIONS)
    {
        return;
    }
    if(metrics->option_text_ids[option] != 0)
    {
        string_delete(metrics->option_text_ids[option]);
    }
    String* new_string = string_add(text);
    metrics->option_text_ids[option] = new_string->id;
    reconnect_titles();
    metrics_save();
}

const char* metrics_get_option_text(Metrics* metrics, uint8_t option)
{
    if(option >= MAX_METRIC_OPTIONS || metrics->option_text_ids[option] == 0)
    {
        return "";
    }
    String* string = string_get(metrics->option_text_ids[option]);
    return string != NULL ? string->value : "";
}

void metrics_groups_set_title(MetricsGroup* metrics_group, char* title)
{
    if(metrics_group->title != NULL)
    {
        string_delete(metrics_group->title_id);
    }
    String* new_string = string_add(title);
    metrics_group->title_id = new_string->id;
    metrics_group->title = new_string;
    reconnect_titles();
    metrics_groups_save();
}

Metrics* metrics_get(const uint16_t id)
{
    return (Metrics*)dynamic_get(id, &m_metrics, metrics_is_same_id);
}

Metrics* metrics_get_all()
{
    return (Metrics*)m_metrics.items;
}

uint16_t metrics_add(Metrics* metrics)
{
    dynamic_add(&m_metrics, (byte*)metrics, set_metrics_id);
    return metrics->id;
}

Metrics* metrics_new()
{
    Metrics new =
    {
        .max_value = 1,
        .min_value = 1,     // intervals default to 1..max (toggle to 0 in config)
        .type = MetricsType_BOOL,
        .main_icon = IconChoice_NONE,
        // Default 2-option icons: value 0 = No (cross), value 1 = Yes (check).
        .option_icons = { IconChoice_CROSS, IconChoice_CHECK, IconChoice_NONE },
    };
    uint16_t new_id = metrics_add(&new);
    Metrics* stored = metrics_get(new_id);
    metrics_set_title(stored, "New Metric");
    return stored;
}

MetricsGroup* metrics_group_new()
{
    MetricsGroup new =
    {
        .alarm =
        {
            .active = true,
            .time =
            {
                .hour = 8,
                .minute = 0,
            },
            .wakeup_id = 0,
        },
    };
    uint16_t new_id = metrics_group_add(&new);
    MetricsGroup* stored = metrics_group_get(new_id);
    metrics_groups_set_title(stored, "New Group");
    return stored;
}

void metrics_delete(const uint16_t delete_id)
{
    // Release the metric's interned strings and its registrations, or they
    // linger in storage forever (and count against the persist budget).
    Metrics* metric = metrics_get(delete_id);
    if(metric != NULL)
    {
        if(metric->title_id != 0)
        {
            string_delete(metric->title_id);
        }
        for(int option = 0; option < MAX_METRIC_OPTIONS; option++)
        {
            if(metric->option_text_ids[option] != 0)
            {
                string_delete(metric->option_text_ids[option]);
            }
        }
    }

    bool removed = true;
    while(removed)
    {
        removed = false;
        for(int i = 0; i < m_registrations.number_of_items; i++)
        {
            Registration* reg = (Registration*)&m_registrations.items[i * m_registrations.item_size];
            if(reg->metrics_id == delete_id)
            {
                dynamic_delete(reg->id, &m_registrations, get_registration_id);
                removed = true;
                break;
            }
        }
    }

    remove_memberships(false, delete_id);
    dynamic_delete(delete_id, &m_metrics, get_metrics_id);
    reconnect_titles();
}

uint32_t metrics_count()
{
    return m_metrics.number_of_items;
}

void metrics_save()
{
    dynamic_save(&m_metrics);
}

void metrics_groups_save()
{
    dynamic_save(&m_metrics_groups);
}

void registration_add(Registration* registration)
{
    dynamic_add(&m_registrations, (byte*)registration, set_registration_id);
    APP_LOG(APP_LOG_LEVEL_INFO, "Registration saved: metric=%d value=%d (total=%d)",
        registration->metrics_id, registration->value, (int)m_registrations.number_of_items);
}

void registration_update(Registration* registration, uint8_t value)
{
    registration->value = value;
    registration->time_stamp = time(NULL);
    dynamic_save(&m_registrations);
}

Registration* registrations_get_all()
{
    return (Registration*)m_registrations.items;
}

uint32_t registrations_count()
{
    return m_registrations.number_of_items;
}

static time_t start_of_today()
{
    time_t now = time(NULL);
    struct tm* local = localtime(&now);
    local->tm_hour = 0;
    local->tm_min = 0;
    local->tm_sec = 0;
    return mktime(local);
}

bool metric_registered_today_in_group(uint16_t group_id, uint16_t metric_id)
{
    return registration_today_for_group_metric(group_id, metric_id) != NULL;
}

Registration* registration_today_for_group_metric(uint16_t group_id, uint16_t metric_id)
{
    // A group slot has at most one registration per day (answers update in
    // place), but spontaneous (group 0) can have several — return the latest.
    time_t since = start_of_today();
    Registration* latest = NULL;
    for(int i = 0; i < m_registrations.number_of_items; i++)
    {
        Registration* reg = (Registration*)&m_registrations.items[i * m_registrations.item_size];
        if(reg->metrics_id == metric_id && reg->group_id == group_id && reg->time_stamp >= since)
        {
            if(latest == NULL || reg->time_stamp >= latest->time_stamp)
            {
                latest = reg;
            }
        }
    }
    return latest;
}

// Registrations are pruned when BOTH hold: the companion has stored them
// (time_stamp <= acked_through) AND they're past the on-watch retention window.
typedef struct
{
    time_t acked_through;
    time_t keep_after;
} PruneBounds;

static bool registration_is_prunable(byte* item, void* context)
{
    Registration* reg = (Registration*)item;
    PruneBounds* bounds = (PruneBounds*)context;
    return reg->time_stamp <= bounds->acked_through && reg->time_stamp < bounds->keep_after;
}

#define REGISTRATION_RETENTION_DAYS (7)

uint16_t registrations_prune_synced(time_t acked_through)
{
    PruneBounds bounds =
    {
        .acked_through = acked_through,
        .keep_after = time(NULL) - (REGISTRATION_RETENTION_DAYS * SECONDS_PER_DAY),
    };
    uint16_t removed = dynamic_delete_where(&m_registrations, registration_is_prunable, &bounds);
    if(removed > 0)
    {
        APP_LOG(APP_LOG_LEVEL_INFO, "prune: removed %d synced registrations (%d left)",
            removed, (int)m_registrations.number_of_items);
    }
    return removed;
}

bool registrations_last_value(uint16_t metric_id, uint8_t* out_value)
{
    bool found = false;
    time_t latest = 0;
    for(int i = 0; i < m_registrations.number_of_items; i++)
    {
        Registration* reg = (Registration*)&m_registrations.items[i * m_registrations.item_size];
        if(reg->metrics_id == metric_id && (!found || reg->time_stamp >= latest))
        {
            latest = reg->time_stamp;
            *out_value = reg->value;
            found = true;
        }
    }
    return found;
}

bool metrics_group_complete_today(uint16_t group_id)
{
    uint32_t total = metrics_count();
    Metrics* all_metrics = metrics_get_all();
    for(uint32_t i = 0; i < total; i++)
    {
        Metrics* metric = &all_metrics[i];
        if(metrics_group_has_metric(group_id, metric->id) && !metric_registered_today_in_group(group_id, metric->id))
        {
            return false;
        }
    }
    return true;
}

static uint16_t seed_group(const char* name, uint8_t hour, uint8_t minute)
{
    MetricsGroup* group = metrics_group_new();   // active alarm, default title
    uint16_t id = group->id;
    metrics_groups_set_title(group, (char*)name);
    group->alarm.time.hour = hour;
    group->alarm.time.minute = minute;
    metrics_groups_save();
    return id;
}

static uint16_t seed_metric(const char* name, MetricsType type, uint8_t main_icon,
    uint8_t min_value, uint8_t max_value)
{
    Metrics* metric = metrics_new();             // BOOL default (cross/check)
    uint16_t id = metric->id;
    metrics_set_title(metric, (char*)name);
    metric = metrics_get(id);
    metric->type = type;
    metric->main_icon = main_icon;
    if(type == MetricsType_INTERVAL)
    {
        metric->min_value = min_value;
        metric->max_value = max_value;
    } else if(type == MetricsType_THREE_OPTION)
    {
        metric->option_icons[0] = IconChoice_FACE_SAD;
        metric->option_icons[1] = IconChoice_FACE_NEUTRAL;
        metric->option_icons[2] = IconChoice_FACE_HAPPY;
    }
    metrics_save();
    return id;
}

void metrics_seed_defaults()
{
    uint16_t morning = seed_group("Morning", 7, 30);
    uint16_t lunch = seed_group("Lunch", 12, 30);
    uint16_t evening = seed_group("Evening", 21, 30);

    // Concrete emotion metrics (replacing the old vague "Mood").
    uint16_t joy = seed_metric("Joy", MetricsType_INTERVAL, IconChoice_MOOD, 1, 5);
    uint16_t anxiety = seed_metric("Anxiety", MetricsType_INTERVAL, IconChoice_CLOUD, 0, 5);
    uint16_t irritation = seed_metric("Irritation", MetricsType_INTERVAL, IconChoice_BOLT, 0, 5);
    uint16_t stress = seed_metric("Stress", MetricsType_INTERVAL, IconChoice_THERMO, 0, 5);
    uint16_t love = seed_metric("Love", MetricsType_BOOL, IconChoice_HEART, 0, 0);
    uint16_t sleep = seed_metric("Sleep quality", MetricsType_THREE_OPTION, IconChoice_NONE, 0, 0);
    uint16_t rested = seed_metric("Rested", MetricsType_BOOL, IconChoice_NONE, 0, 0);
    uint16_t exercised = seed_metric("Exercised", MetricsType_BOOL, IconChoice_EXERCISE, 0, 0);

    // Unscheduled metrics (no group) — registered on demand.
    seed_metric("Sick", MetricsType_BOOL, IconChoice_NONE, 0, 0);
    seed_metric("Medication", MetricsType_BOOL, IconChoice_PILL, 0, 0);

    // Morning: joy, sleep, rested, love, irritation.
    metrics_group_toggle_metric(morning, joy);
    metrics_group_toggle_metric(morning, sleep);
    metrics_group_toggle_metric(morning, rested);
    metrics_group_toggle_metric(morning, love);
    metrics_group_toggle_metric(morning, irritation);

    // Lunch: joy, irritation, stress.
    metrics_group_toggle_metric(lunch, joy);
    metrics_group_toggle_metric(lunch, irritation);
    metrics_group_toggle_metric(lunch, stress);

    // Evening: joy, exercised, stress, anxiety, irritation.
    metrics_group_toggle_metric(evening, joy);
    metrics_group_toggle_metric(evening, exercised);
    metrics_group_toggle_metric(evening, stress);
    metrics_group_toggle_metric(evening, anxiety);
    metrics_group_toggle_metric(evening, irritation);
}