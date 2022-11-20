#include "dynamic_repository.h"

void dynamic_init(DynamicData* data)
{
    if(persist_exists(data->meta_data_storage_key))
    {
        persist_read_data(data->meta_data_storage_key, data, sizeof(DynamicData));
    }
    size_t items_size = data->number_of_items * data->item_size;

    data->items = malloc(items_size);
    if(data->number_of_items > 0)
    {
        persist_read_data(data->items_storage_key, data->items, items_size);
    }
}

void dynamic_add(DynamicData* data, byte* item_to_add, SetItemId set_item_id_function)
{
    size_t old_size = data->number_of_items * data->item_size;
    data->number_of_items++;
    size_t new_size = data->number_of_items * data->item_size;

    byte* new_items = (byte*)malloc(new_size);

    set_item_id_function(data->next_id++, item_to_add);

    memcpy(new_items, data->items, old_size);
    memcpy(&new_items[old_size], item_to_add, data->item_size);

    free(data->items);

    data->items = new_items;

    persist_write_data(data->meta_data_storage_key, data, sizeof(DynamicData));
    persist_write_data(data->items_storage_key, data->items, new_size);
}

void dynamic_delete(const uint16_t delete_id, DynamicData* data, GetItemId get_item_id_function)
{
    uint16_t old_number_of_items = data->number_of_items--;
    size_t new_items_size = data->number_of_items * data->item_size;

    byte* new_items = (byte*)malloc(new_items_size);

    size_t current_insert_index = 0;

    for(uint16_t i = 0; i < old_number_of_items; i++)
    {
        byte* current_item = &data->items[i * data->item_size];
        if(get_item_id_function(current_item) != delete_id)
        {
            memcpy(&new_items[current_insert_index], current_item, data->item_size);
            current_insert_index += data->item_size;
        }
    }

    free(data->items);

    data->items = new_items;
    persist_write_data(data->meta_data_storage_key, data, sizeof(DynamicData));
    if(data->items == NULL)
    {
        persist_delete(data->items_storage_key);
    } else
    {
        persist_write_data(data->items_storage_key, data->items, new_items_size);
    }
}

byte* dynamic_get(const uint16_t id, DynamicData* data, SameIdPredicate same_id_predicate_function)
{
    byte* found = NULL;
    for(int i = 0; i < data->number_of_items; i++)
    {
        byte* current_item = &data->items[i * data->item_size];
        if(same_id_predicate_function(id, current_item))
        {
            found = current_item;
            break;
        }
    }
    return found;
}

void dynamic_save(DynamicData* data)
{
    persist_write_data(data->items_storage_key, data->items, data->item_size * data->number_of_items);
}