#include "metrics_repository.h"

#include "data.h"
#include "dynamic_repository.h"

static DynamicData m_metrics_groups =
{
    .meta_data_storage_key = DataKeys_METRICS_GROUP_META_DATA,
    .items_storage_key = DataKeys_METRICS_GROUP_DATA,
    .item_size = sizeof(MetricsGroup),
    .number_of_items = 0,
    .next_id = 0,
    .items = NULL,
};

static DynamicData m_metrics =
{
    .meta_data_storage_key = DataKeys_METRICS_META_DATA,
    .items_storage_key = DataKeys_METRICS_DATA,
    .item_size = sizeof(Metrics),
    .number_of_items = 0,
    .next_id = 0,
    .items = NULL,
};

static void set_group_id(uint16_t id, byte* item)
{
    ((MetricsGroup*)item)->id = id;
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
    ((Metrics*)item)->id = id;
}

static uint16_t get_metrics_id(byte* item)
{
    return ((Metrics*)item)->id;
}

static bool metrics_is_same_id(uint16_t id, byte* item)
{
    return ((Metrics*)item)->id == id;
}

void metrics_init()
{
    dynamic_init(&m_metrics_groups)
    dynamic_init(&m_metrics)

    for(int i = 0; i < m_metrics_groups.number_of_items; i++)
    {
        MetricsGroup* current_group = &m_metrics_groups->items[i];
        current_group->title = string_get(current_group->title_id);
    }

    for(int i = 0; i < m_metrics.number_of_items; i++)
    {
        Metrics* current_metrics = &m_metrics->items[i];
        current_metrics->title = string_get(current_metrics->title_id);
        current_metrics->group = metrics_group_get(current_metrics->group_id);
    }
}

void metrics_tear_down()
{
    free(m_metrics_groups->items);
    free(m_metrics->items);
}

MetricsGroup* metrics_group_get(const uint16_t id)
{
    return (MetricsGroup*)dynamic_get(id, m_metrics_groups, group_is_same_id);
}

MetricsGroup* metrics_groups_get_all()
{
    return (MetricsGroup*)m_metrics_groups->items;
}

uint16_t metrics_group_add(MetricsGroup* metrics_group);
{
    metrics_group->title_id = string_add(metrics_group->title);
    dynamic_add(&m_metrics_groups, (byte*)metrics_group, set_group_id);
}

void metrics_group_delete(const uint16_t delete_id);
{
    dynamic_delete(delete_id, &m_metrics_groups, get_group_id);
}

uint32_t metrics_groups_count();
{
    return m_metrics_groups.number_of_items;
}

void metrics_set_title(Metrics* metrics, char* title)
{
    string_delete(metrics->title_id);

    String* new_string = string_add(title);
    metrics->title_id = new_string->id;
    metrics->title = new_string;
    metrics_save();
}

void metrics_groups_set_title(MetricsGroup* metrics_group, char* title)
{
    string_delete(metrics_group->title_id);

    String* new_string = string_add(title);
    metrics_group->title_id = new_string->id;
    metrics_group->title = new_string;
    metrics_groups_save();
}

Metrics* metrics_get(const uint16_t id)
{
    return (Metrics*)dynamic_get(id, m_metrics, metrics_is_same_id);
}

Metrics* metrics_get_all()
{
    return (Metrics*)m_metrics->items;
}

uint16_t metrics_add(Metrics* metrics);
{
    dynamic_add(&m_metrics, (byte*)metrics, set_metrics_id);
}

void metrics_delete(const uint16_t delete_id);
{
    dynamic_delete(delete_id, &m_metrics, get_metrics_id);
}

uint32_t metrics_count();
{
    return m_metrics.number_of_items;
}

void metrics_save()
{
    persist_write_data(DataKeys_METRICS_DATA, m_metrics, m_metrics.number_of_items * m_metrics.item_size);
}

void metrics_groups_save()
{
    persist_write_data(DataKeys_METRICS_GROUP_DATA, m_metrics_groups, m_metrics_groups.number_of_items * m_metrics_groups.item_size);
}