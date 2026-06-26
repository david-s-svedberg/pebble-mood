#include "data_export.h"

#include <pebble.h>
#include "message_keys.auto.h"

#include "repositories/metrics_repository.h"

#define MAX_RETRIES_PER_ITEM (3)

static uint32_t m_cursor = 0;
static uint32_t m_total = 0;
static uint8_t m_retries = 0;
static bool m_sending = false;

static void send_next();

static void send_registration(uint32_t index)
{
    DictionaryIterator* iter;
    AppMessageResult result = app_message_outbox_begin(&iter);
    if(result != APP_MSG_OK)
    {
        APP_LOG(APP_LOG_LEVEL_ERROR, "export: outbox_begin failed: %d", result);
        return;
    }

    Registration* registrations = registrations_get_all();
    Registration* reg = &registrations[index];

    Metrics* metric = metrics_get(reg->metrics_id);
    const char* name = (metric != NULL && metric->title != NULL) ? metric->title->value : "";
    uint8_t type = (metric != NULL) ? (uint8_t)metric->type : 0;

    dict_write_uint16(iter, MESSAGE_KEY_REG_INDEX, (uint16_t)index);
    dict_write_uint16(iter, MESSAGE_KEY_REG_TOTAL, (uint16_t)m_total);
    dict_write_uint16(iter, MESSAGE_KEY_REG_METRIC_ID, reg->metrics_id);
    dict_write_cstring(iter, MESSAGE_KEY_REG_METRIC_NAME, name);
    dict_write_uint8(iter, MESSAGE_KEY_REG_METRIC_TYPE, type);
    dict_write_uint8(iter, MESSAGE_KEY_REG_VALUE, reg->value);
    dict_write_uint32(iter, MESSAGE_KEY_REG_TIMESTAMP, (uint32_t)reg->time_stamp);

    app_message_outbox_send();
}

static void send_next()
{
    if(m_cursor >= m_total)
    {
        m_sending = false;
        APP_LOG(APP_LOG_LEVEL_INFO, "export: done (%d registrations)", (int)m_total);
        return;
    }
    send_registration(m_cursor);
}

void data_export_send_all()
{
    if(m_sending)
    {
        return;
    }
    m_total = registrations_count();
    m_cursor = 0;
    m_retries = 0;
    if(m_total == 0)
    {
        APP_LOG(APP_LOG_LEVEL_INFO, "export: no registrations to send");
        return;
    }
    m_sending = true;
    APP_LOG(APP_LOG_LEVEL_INFO, "export: sending %d registrations", (int)m_total);
    send_next();
}

static void inbox_received(DictionaryIterator* iter, void* context)
{
    if(dict_find(iter, MESSAGE_KEY_EXPORT_REQUEST) != NULL)
    {
        data_export_send_all();
    }
}

static void outbox_sent(DictionaryIterator* iter, void* context)
{
    m_cursor++;
    m_retries = 0;
    send_next();
}

static void outbox_failed(DictionaryIterator* iter, AppMessageResult reason, void* context)
{
    if(m_retries < MAX_RETRIES_PER_ITEM)
    {
        m_retries++;
        APP_LOG(APP_LOG_LEVEL_WARNING, "export: send failed (reason=%d) at %d, retry %d",
            reason, (int)m_cursor, m_retries);
        send_next();
    } else
    {
        APP_LOG(APP_LOG_LEVEL_ERROR, "export: giving up on index %d after %d retries",
            (int)m_cursor, m_retries);
        m_cursor++;
        m_retries = 0;
        send_next();
    }
}

void data_export_init()
{
    app_message_register_inbox_received(inbox_received);
    app_message_register_outbox_sent(outbox_sent);
    app_message_register_outbox_failed(outbox_failed);
    app_message_open(256, 256);
}
