#pragma once

#include <pebble.h>
#include <stdint.h>
#include <stdbool.h>

#include "../data.h"
#include "string_repository.h"

void            metrics_init();
void            metrics_tear_down();

// Seeds a useful default set (Morning/Evening groups sharing a Mood metric, plus
// a few unscheduled metrics). Call once on first start, after metrics_init().
void            metrics_seed_defaults();

Metrics*        metrics_get(const uint16_t id);
Metrics*        metrics_get_all();
uint16_t        metrics_add(Metrics* metrics);
Metrics*        metrics_new();
void            metrics_delete(const uint16_t delete_id);
uint32_t        metrics_count();
void            metrics_set_title(Metrics* metrics, char* title);
void            metrics_set_option_text(Metrics* metrics, uint8_t option, char* text);
const char*     metrics_get_option_text(Metrics* metrics, uint8_t option);
void            metrics_save();

void            registration_add(Registration* registration);
void            registration_update(Registration* registration, uint8_t value);
bool            metric_registered_today_in_group(uint16_t group_id, uint16_t metric_id);
// Today's registration for a given (group, metric) slot, or NULL. group_id 0
// matches spontaneous registrations.
Registration*   registration_today_for_group_metric(uint16_t group_id, uint16_t metric_id);
bool            metrics_group_complete_today(uint16_t group_id);
// Value of the most recent registration for a metric (any group), for defaulting
// the next entry. Returns false if the metric has never been registered.
bool            registrations_last_value(uint16_t metric_id, uint8_t* out_value);
Registration*   registrations_get_all();
uint32_t        registrations_count();

bool            metrics_group_has_metric(uint16_t group_id, uint16_t metric_id);
void            metrics_group_toggle_metric(uint16_t group_id, uint16_t metric_id);

MetricsGroup*   metrics_group_get(const uint16_t id);
MetricsGroup*   metrics_groups_get_all();
uint16_t        metrics_group_add(MetricsGroup* metrics_group);
MetricsGroup*   metrics_group_new();
void            metrics_group_delete(const uint16_t delete_id);
uint32_t        metrics_groups_count();
void            metrics_groups_set_title(MetricsGroup* metrics_group, char* title);
void            metrics_groups_save();