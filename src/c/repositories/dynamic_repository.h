#pragma once

#include <pebble.h>
#include <stdint.h>
#include <stdbool.h>

typedef char byte;

typedef struct {
    uint32_t meta_data_storage_key;
    uint32_t items_storage_key;
    size_t item_size;
    uint16_t number_of_items;
    uint16_t next_id;
    byte* items;
} DynamicData;

typedef void (*SetItemId)(uint16_t id, byte* item);
typedef uint16_t (*GetItemId)(byte* item);
typedef bool (*SameIdPredicate)(uint16_t id, byte* item);

void    dynamic_init(DynamicData* data);
void    dynamic_add(DynamicData* data, byte* new_item, SetItemId set_item_id_function);
void    dynamic_delete(const uint16_t delete_id, DynamicData* data, GetItemId get_item_id_function);
byte*   dynamic_get(const uint16_t id, DynamicData* data, SameIdPredicate same_id_predicate_function);
void    dynamic_save(DynamicData* data);