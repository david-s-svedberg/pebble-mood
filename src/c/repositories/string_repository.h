#pragma once

#include <pebble.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t id;
    size_t length;
    char* value;
} String;

void        strings_init();
String*     string_get(const uint16_t id);
String*     strings_get_all();
String*     string_add(String* new_string);
void        string_delete(const uint16_t delete_id);
uint32_t    strings_count();
void        strings_tear_down();