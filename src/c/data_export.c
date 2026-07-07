#include "data_export.h"

#include <pebble.h>
#include "message_keys.auto.h"

#include "repositories/metrics_repository.h"

#define MAX_RETRIES_PER_ITEM (3)
// Group members are packed as one uint8 metric id per byte.
#define MAX_MEMBERS_PER_GROUP (32)
// Option texts join into one string with the ASCII unit separator.
#define OPTION_TEXT_SEPARATOR "\x1f"

// The export is a serial state machine over three phases: metric definitions,
// group definitions (with membership), then all registrations, closed by an
// EXPORT_DONE message. One AppMessage in flight at a time; the sent/failed
// callbacks drive progression.
typedef enum
{
    ExportPhase_IDLE,
    ExportPhase_METRICS,
    ExportPhase_GROUPS,
    ExportPhase_REGISTRATIONS,
    ExportPhase_DONE,
} ExportPhase;

static ExportPhase m_phase = ExportPhase_IDLE;
static uint32_t m_cursor = 0;
static uint8_t m_retries = 0;

static void send_current();

static void abort_export(const char* why)
{
    APP_LOG(APP_LOG_LEVEL_ERROR, "export: aborted (%s)", why);
    m_phase = ExportPhase_IDLE;
}

static bool begin(DictionaryIterator** iter)
{
    AppMessageResult result = app_message_outbox_begin(iter);
    if(result != APP_MSG_OK)
    {
        abort_export("outbox_begin failed");
        return false;
    }
    return true;
}

// The stored min/max only mean something for INTERVAL; export the effective
// value range per type so consumers can normalize without knowing the types.
static uint8_t effective_min(Metrics* metric)
{
    return (metric->type == MetricsType_INTERVAL) ? metric->min_value : 0;
}

static uint8_t effective_max(Metrics* metric)
{
    switch(metric->type)
    {
        case MetricsType_INTERVAL:     return metric->max_value;
        case MetricsType_THREE_OPTION: return 2;
        default:                       return 1;   // BOOL
    }
}

static void send_metric(uint32_t index)
{
    DictionaryIterator* iter;
    if(!begin(&iter)) return;

    Metrics* metric = &metrics_get_all()[index];
    const char* name = (metric->title != NULL) ? metric->title->value : "";

    // Join the (up to 3) option texts into one field; empty slots stay empty.
    static char texts[96];
    snprintf(texts, sizeof(texts), "%s" OPTION_TEXT_SEPARATOR "%s" OPTION_TEXT_SEPARATOR "%s",
        metrics_get_option_text(metric, 0),
        metrics_get_option_text(metric, 1),
        metrics_get_option_text(metric, 2));

    dict_write_uint16(iter, MESSAGE_KEY_CFG_METRIC_ID, metric->id);
    dict_write_cstring(iter, MESSAGE_KEY_CFG_METRIC_NAME, name);
    dict_write_uint8(iter, MESSAGE_KEY_CFG_METRIC_TYPE, (uint8_t)metric->type);
    dict_write_uint8(iter, MESSAGE_KEY_CFG_METRIC_MIN, effective_min(metric));
    dict_write_uint8(iter, MESSAGE_KEY_CFG_METRIC_MAX, effective_max(metric));
    dict_write_uint8(iter, MESSAGE_KEY_CFG_METRIC_MAIN_ICON, metric->main_icon);
    dict_write_data(iter, MESSAGE_KEY_CFG_METRIC_OPTION_ICONS, metric->option_icons, MAX_METRIC_OPTIONS);
    dict_write_cstring(iter, MESSAGE_KEY_CFG_METRIC_OPTION_TEXTS, texts);

    app_message_outbox_send();
}

static void send_group(uint32_t index)
{
    DictionaryIterator* iter;
    if(!begin(&iter)) return;

    MetricsGroup* group = &metrics_groups_get_all()[index];
    const char* name = (group->title != NULL) ? group->title->value : "";

    uint8_t members[MAX_MEMBERS_PER_GROUP];
    uint16_t member_count = 0;
    Metrics* all_metrics = metrics_get_all();
    for(uint32_t m = 0; m < metrics_count() && member_count < MAX_MEMBERS_PER_GROUP; m++)
    {
        if(metrics_group_has_metric(group->id, all_metrics[m].id))
        {
            members[member_count++] = (uint8_t)all_metrics[m].id;
        }
    }

    dict_write_uint16(iter, MESSAGE_KEY_CFG_GROUP_ID, group->id);
    dict_write_cstring(iter, MESSAGE_KEY_CFG_GROUP_NAME, name);
    dict_write_uint8(iter, MESSAGE_KEY_CFG_GROUP_HOUR, group->alarm.time.hour);
    dict_write_uint8(iter, MESSAGE_KEY_CFG_GROUP_MINUTE, group->alarm.time.minute);
    dict_write_uint8(iter, MESSAGE_KEY_CFG_GROUP_ACTIVE, group->alarm.active ? 1 : 0);
    dict_write_data(iter, MESSAGE_KEY_CFG_GROUP_MEMBERS, members, member_count);

    app_message_outbox_send();
}

static void send_registration(uint32_t index)
{
    DictionaryIterator* iter;
    if(!begin(&iter)) return;

    Registration* reg = &registrations_get_all()[index];

    Metrics* metric = metrics_get(reg->metrics_id);
    const char* name = (metric != NULL && metric->title != NULL) ? metric->title->value : "";
    uint8_t type = (metric != NULL) ? (uint8_t)metric->type : 0;
    uint8_t min_value = (metric != NULL) ? effective_min(metric) : 0;
    uint8_t max_value = (metric != NULL) ? effective_max(metric) : 1;

    MetricsGroup* group = (reg->group_id != 0) ? metrics_group_get(reg->group_id) : NULL;
    const char* group_name = (group != NULL && group->title != NULL) ? group->title->value : "";

    dict_write_uint16(iter, MESSAGE_KEY_REG_INDEX, (uint16_t)index);
    dict_write_uint16(iter, MESSAGE_KEY_REG_TOTAL, (uint16_t)registrations_count());
    dict_write_uint16(iter, MESSAGE_KEY_REG_METRIC_ID, reg->metrics_id);
    dict_write_cstring(iter, MESSAGE_KEY_REG_METRIC_NAME, name);
    dict_write_uint8(iter, MESSAGE_KEY_REG_METRIC_TYPE, type);
    dict_write_uint8(iter, MESSAGE_KEY_REG_METRIC_MIN, min_value);
    dict_write_uint8(iter, MESSAGE_KEY_REG_METRIC_MAX, max_value);
    dict_write_uint8(iter, MESSAGE_KEY_REG_VALUE, reg->value);
    dict_write_uint32(iter, MESSAGE_KEY_REG_TIMESTAMP, (uint32_t)reg->time_stamp);
    dict_write_uint16(iter, MESSAGE_KEY_REG_GROUP_ID, reg->group_id);
    dict_write_cstring(iter, MESSAGE_KEY_REG_GROUP_NAME, group_name);

    app_message_outbox_send();
}

static void send_done()
{
    DictionaryIterator* iter;
    if(!begin(&iter)) return;
    dict_write_uint8(iter, MESSAGE_KEY_EXPORT_DONE, 1);
    app_message_outbox_send();
}

// Advances past exhausted phases, then sends the item at the cursor.
static void send_current()
{
    if(m_phase == ExportPhase_METRICS && m_cursor >= metrics_count())
    {
        m_phase = ExportPhase_GROUPS;
        m_cursor = 0;
    }
    if(m_phase == ExportPhase_GROUPS && m_cursor >= metrics_groups_count())
    {
        m_phase = ExportPhase_REGISTRATIONS;
        m_cursor = 0;
    }
    if(m_phase == ExportPhase_REGISTRATIONS && m_cursor >= registrations_count())
    {
        m_phase = ExportPhase_DONE;
    }

    switch(m_phase)
    {
        case ExportPhase_METRICS:       send_metric(m_cursor); break;
        case ExportPhase_GROUPS:        send_group(m_cursor); break;
        case ExportPhase_REGISTRATIONS: send_registration(m_cursor); break;
        case ExportPhase_DONE:          send_done(); break;
        default: break;
    }
}

void data_export_send_all()
{
    if(m_phase != ExportPhase_IDLE)
    {
        return;
    }
    m_phase = ExportPhase_METRICS;
    m_cursor = 0;
    m_retries = 0;
    APP_LOG(APP_LOG_LEVEL_INFO, "export: starting (%d metrics, %d groups, %d registrations)",
        (int)metrics_count(), (int)metrics_groups_count(), (int)registrations_count());
    send_current();
}

static void inbox_received(DictionaryIterator* iter, void* context)
{
    if(dict_find(iter, MESSAGE_KEY_EXPORT_REQUEST) != NULL)
    {
        data_export_send_all();
    }

    // The companion confirmed how far it has safely stored the export — prune
    // old synced registrations (the persist budget is finite). The recent tail
    // stays on the watch for Today / update-in-place / trends.
    Tuple* ack = dict_find(iter, MESSAGE_KEY_EXPORT_ACK_THROUGH);
    if(ack != NULL)
    {
        registrations_prune_synced((time_t)ack->value->uint32);
    }
}

static void outbox_sent(DictionaryIterator* iter, void* context)
{
    if(m_phase == ExportPhase_IDLE)
    {
        return;
    }
    if(m_phase == ExportPhase_DONE)
    {
        APP_LOG(APP_LOG_LEVEL_INFO, "export: done");
        m_phase = ExportPhase_IDLE;
        return;
    }
    m_cursor++;
    m_retries = 0;
    send_current();
}

static void outbox_failed(DictionaryIterator* iter, AppMessageResult reason, void* context)
{
    if(m_phase == ExportPhase_IDLE)
    {
        return;
    }
    if(m_retries < MAX_RETRIES_PER_ITEM)
    {
        m_retries++;
        APP_LOG(APP_LOG_LEVEL_WARNING, "export: send failed (reason=%d) phase=%d cursor=%d, retry %d",
            reason, m_phase, (int)m_cursor, m_retries);
        send_current();
    } else if(m_phase == ExportPhase_DONE)
    {
        abort_export("done message failed");
    } else
    {
        APP_LOG(APP_LOG_LEVEL_ERROR, "export: giving up on phase=%d cursor=%d", m_phase, (int)m_cursor);
        m_cursor++;
        m_retries = 0;
        send_current();
    }
}

void data_export_init()
{
    app_message_register_inbox_received(inbox_received);
    app_message_register_outbox_sent(outbox_sent);
    app_message_register_outbox_failed(outbox_failed);
    app_message_open(256, 256);
}
