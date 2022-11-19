#pragma once

#include <pebble.h>
#include <stdint.h>
#include <stdbool.h>

#include "string_repository.h"

enum MetricsType
{
    MetricsType_NONE,
    MetricsType_BOOL,
    MetricsType_INTERVAL,
}

typedef struct
{
    uint16_t id;
    Alarm alarm;
    uint16_t title_id;
    String* title;
} MetricsGroup;

typedef struct
{
    uint16_t id;
    uint16_t group_id;
    MetricsGroup* group;
    uint16_t title_id;
    String* title;
    MetricsType type;
    uint8_t max_value;
} Metrics;

void            metrics_init();
void            metrics_tear_down();

Metrics*        metrics_get(const uint16_t id);
Metrics*        metrics_get_all();
uint16_t        metrics_add(Metrics* metrics);
void            metrics_delete(const uint16_t delete_id);
uint32_t        metrics_count();
void            metrics_set_title(Metrics* metrics, char* title);
void            metrics_save();

MetricsGroup*   metrics_group_get(const uint16_t id);
MetricsGroup*   metrics_groups_get_all();
uint16_t        metrics_group_add(MetricsGroup* metrics_group);
void            metrics_group_delete(const uint16_t delete_id);
uint32_t        metrics_groups_count();
void            metrics_groups_set_title(MetricsGroup* metrics_group, char* title);
void            metrics_groups_save();