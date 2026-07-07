#pragma once

#include <pebble.h>

// Applies a phone-side config change (SET_GROUP_* / SET_METRIC_* AppMessage,
// queued in the companion and relayed by pkjs). Returns true when the message
// was a config change (handled), false otherwise. id 0 = create new entity —
// the watch allocates the id and the next config export mirrors it back, so
// the phone never needs id mapping.
bool config_apply_handle(DictionaryIterator* iter);
