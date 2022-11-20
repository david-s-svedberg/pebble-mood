#pragma once

#include <pebble.h>

#include "repositories/metrics_repository.h"

void setup_edit_alarm_window(MetricsGroup* metrics_group);
void tear_down_edit_alarm_window();