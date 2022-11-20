#pragma once

#include <pebble.h>
#include "repositories/metrics_repository.h"

void setup_metrics_config_window(Metrics* metrics);
void tear_down_metrics_config_window();