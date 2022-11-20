#pragma once

#include <pebble.h>
#include <stdint.h>
#include <stdbool.h>

#include "../data.h"
#include "string_repository.h"

void            metrics_init();
void            metrics_tear_down();

Metrics*        metrics_get(const uint16_t id);
Metrics*        metrics_get_all();
uint16_t        metrics_add(Metrics* metrics);
Metrics*        metrics_new();
void            metrics_delete(const uint16_t delete_id);
uint32_t        metrics_count();
void            metrics_set_title(Metrics* metrics, char* title);
void            metrics_save();

void            registration_add(Registration* registration);
Registration*   registrations_get_for_metric(uint16_t metric_id);
Registration*   registrations_get_all();
uint32_t        registrations_count();

MetricsGroup*   metrics_group_get(const uint16_t id);
MetricsGroup*   metrics_groups_get_all();
uint16_t        metrics_group_add(MetricsGroup* metrics_group);
MetricsGroup*   metrics_group_new();
void            metrics_group_delete(const uint16_t delete_id);
uint32_t        metrics_groups_count();
void            metrics_groups_set_title(MetricsGroup* metrics_group, char* title);
void            metrics_groups_save();