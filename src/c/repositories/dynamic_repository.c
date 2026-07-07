#include "dynamic_repository.h"

#include "persist_blob.h"

// Persists the meta record (small, fits one key) and the items blob (chunked —
// a single persist key silently truncates at PERSIST_DATA_MAX_LENGTH).
static void save_all(DynamicData* data)
{
    int written = persist_write_data(data->meta_data_storage_key, data, sizeof(DynamicData));
    if(written != (int)sizeof(DynamicData))
    {
        APP_LOG(APP_LOG_LEVEL_ERROR, "store %d: meta write %d of %d B",
            (int)data->meta_data_storage_key, written, (int)sizeof(DynamicData));
    }

    size_t items_size = data->number_of_items * data->item_size;
    if(items_size == 0)
    {
        persist_blob_delete(data->items_storage_key);
    } else
    {
        persist_blob_write(data->items_storage_key, data->items, items_size);
    }
}

void dynamic_init(DynamicData* data)
{
    // The compile-time definition owns item_size and the storage keys; only the
    // dynamic meta (count + next id) is adopted from storage — and only when the
    // stored layout matches. On mismatch (a struct changed size between builds)
    // the store is reset rather than parsed as garbage. (Pre-release policy: no
    // migrations, losing the data beats corrupting it.)
    if(persist_exists(data->meta_data_storage_key))
    {
        DynamicData stored;
        persist_read_data(data->meta_data_storage_key, &stored, sizeof(DynamicData));
        if(stored.item_size == data->item_size)
        {
            data->number_of_items = stored.number_of_items;
            data->next_id = stored.next_id;
        } else
        {
            APP_LOG(APP_LOG_LEVEL_WARNING, "store %d: item size changed %d -> %d, resetting store",
                (int)data->meta_data_storage_key, (int)stored.item_size, (int)data->item_size);
            persist_delete(data->meta_data_storage_key);
            persist_blob_delete(data->items_storage_key);
        }
    }

    size_t items_size = data->number_of_items * data->item_size;
    data->items = malloc(items_size);
    if(data->number_of_items > 0)
    {
        size_t read = persist_blob_read(data->items_storage_key, data->items, items_size);
        if(read != items_size)
        {
            // Salvage the complete records; drop the truncated tail.
            APP_LOG(APP_LOG_LEVEL_ERROR, "store %d: expected %d B, read %d B — keeping %d of %d items",
                (int)data->items_storage_key, (int)items_size, (int)read,
                (int)(read / data->item_size), (int)data->number_of_items);
            data->number_of_items = read / data->item_size;
        }
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

    save_all(data);
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
    save_all(data);
}

uint16_t dynamic_delete_where(DynamicData* data, RemovePredicate should_remove, void* context)
{
    uint16_t kept = 0;
    for(uint16_t i = 0; i < data->number_of_items; i++)
    {
        byte* current_item = &data->items[i * data->item_size];
        if(!should_remove(current_item, context))
        {
            if(kept != i)
            {
                memcpy(&data->items[kept * data->item_size], current_item, data->item_size);
            }
            kept++;
        }
    }

    uint16_t removed = data->number_of_items - kept;
    if(removed > 0)
    {
        data->number_of_items = kept;
        save_all(data);
    }
    return removed;
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
    size_t items_size = data->item_size * data->number_of_items;
    if(items_size > 0)
    {
        persist_blob_write(data->items_storage_key, data->items, items_size);
    }
}
