#pragma once

#include <pebble.h>
#include <stdint.h>
#include <stdbool.h>

#include "../data.h"

void        strings_init();
String*     string_get(const uint16_t id);
String*     string_add(char* value);
void        string_delete(const uint16_t delete_id);
void        strings_tear_down();