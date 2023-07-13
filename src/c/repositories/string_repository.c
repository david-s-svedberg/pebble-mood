#include "string_repository.h"
#include "dynamic_repository.h"
#include "../data.h"

static DynamicData strings =
{
    .meta_data_storage_key = DataKeys_STRING_META_DATA,
    .items_storage_key = DataKeys_STRINGS_DATA,
    .item_size = sizeof(String),
    .number_of_items = 0,
    .next_id = 0,
    .items = NULL,
};

static size_t m_total_string_char_length = 0;
static char* m_string_values_data = NULL;

static void set_string_id(uint16_t id, byte* item)
{
    ((String*)item)->id = id;
}

static uint16_t get_string_id(byte* item)
{
    return ((String*)item)->id;
}

void connect_string_values()
{
    uint32_t current_string_value_index = 0;
    for(int i = 0; i < strings.number_of_items; i++)
    {
        String* current_string = (String*)&strings.items[i * strings.item_size];
        char* value = &m_string_values_data[current_string_value_index];
        current_string->value = value;
        current_string_value_index += current_string->length;
    }
}

void strings_init()
{
    dynamic_init(&strings);

    if(persist_exists(DataKeys_STRING_CHAR_DATA))
    {
        m_total_string_char_length = persist_get_size(DataKeys_STRING_CHAR_DATA);
        m_string_values_data = (char*) malloc(m_total_string_char_length);
        if(m_total_string_char_length > 0)
        {
            persist_read_data(DataKeys_STRING_CHAR_DATA, m_string_values_data, m_total_string_char_length);
        }
    }

    connect_string_values();
}

String* strings_get_all()
{
    return (String*)strings.items;
}

String* string_get(const uint16_t id)
{
    String* found = NULL;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "strings.number_of_items:%d", strings.number_of_items);
    for(int i = 0; i < strings.number_of_items; i++)
    {
        String* current_string = (String*)&strings.items[i * strings.item_size];
        if(current_string->id == id)
        {
            found = current_string;
            break;
        }
    }
    return found;
}

String* string_add(char* value)
{
    size_t string_length = strlen(value) + 1;
    String new_string =
    {
        .length = string_length,
    };
    dynamic_add(&strings, (byte*)&new_string, set_string_id);
    size_t old_char_size = m_total_string_char_length;
    m_total_string_char_length += string_length;
    char* new_string_values_data = (char*)malloc(m_total_string_char_length);
    memcpy(new_string_values_data, m_string_values_data, old_char_size);
    memcpy(&new_string_values_data[old_char_size], value, new_string.length);

    int current_string_values_index = 0;
    for(int i = 0; i < strings.number_of_items; i++)
    {
        String* current_string = (String*)&strings.items[i * strings.item_size];
        current_string->value = &new_string_values_data[current_string_values_index];
        current_string_values_index += current_string->length;
    }
    free(m_string_values_data);
    m_string_values_data = new_string_values_data;
    persist_write_data(DataKeys_STRING_CHAR_DATA, m_string_values_data, m_total_string_char_length);
    return string_get(new_string.id);
}

void string_delete(const uint16_t delete_id)
{
    String* delete_string = string_get(delete_id);
    if(delete_string != NULL)
    {
        m_total_string_char_length -= delete_string->length;
        char* new_string_values_data = (char*)malloc(m_total_string_char_length);

        uint32_t value_destination_index = 0;
        uint32_t value_source_index = 0;

        for(int i = 0; i < strings.number_of_items; i++)
        {
            String* current_string = (String*)&strings.items[i * strings.item_size];
            if(current_string->id != delete_id)
            {
                memcpy(&new_string_values_data[value_destination_index], &m_string_values_data[value_source_index], current_string->length);
                value_destination_index += current_string->length;
            }
            value_source_index += current_string->length;
        }

        dynamic_delete(delete_id, &strings, get_string_id);

        size_t current_string_values_index = 0;
        for(int i = 0; i < strings.number_of_items; i++)
        {
            String* current_string = (String*)&strings.items[i * strings.item_size];
            current_string->value = &new_string_values_data[current_string_values_index];
            current_string_values_index += current_string->length;
        }

        free(m_string_values_data);

        m_string_values_data = new_string_values_data;

        if(m_string_values_data == NULL)
        {
            persist_delete(DataKeys_STRING_CHAR_DATA);
        } else
        {
            persist_write_data(DataKeys_STRING_CHAR_DATA, m_string_values_data, m_total_string_char_length);
        }
    }
}

uint32_t strings_count()
{
    return strings.number_of_items;
}

void strings_tear_down()
{
    free(strings.items);
    free(m_string_values_data);
}
