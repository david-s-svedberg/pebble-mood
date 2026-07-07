// PebbleKit JS phone-side for Mood.
//
// On launch it asks the watch to export all registrations, receives them one
// message at a time, and assembles them into a list. This is the bridge the
// companion app will use to pull data for mood-vs-activity correlation.
// For now the assembled list is logged; sending it onward (server / companion
// app) is the next step.

var registrations = [];

function metricTypeName(type) {
  // Mirrors MetricsType in src/c/data.h:
  // NONE=0, BOOL=1, INTERVAL=2, THREE_OPTION=3.
  switch (type) {
    case 1: return 'bool';
    case 2: return 'interval';
    case 3: return 'three_option';
    default: return 'unknown';
  }
}

Pebble.addEventListener('ready', function() {
  console.log('Mood PebbleKit JS ready — requesting registration export');
  registrations = [];
  Pebble.sendAppMessage(
    { 'EXPORT_REQUEST': 1 },
    function() {},
    function(e) { console.log('Failed to request export: ' + JSON.stringify(e)); }
  );
});

Pebble.addEventListener('appmessage', function(e) {
  var p = e.payload;
  if (p.REG_INDEX === undefined) {
    return;
  }

  var registration = {
    metricId: p.REG_METRIC_ID,
    metricName: p.REG_METRIC_NAME,
    metricType: metricTypeName(p.REG_METRIC_TYPE),
    // Value range for interval metrics (e.g. Joy 1-5 vs Anxiety 0-5), so the
    // consumer can normalize scales.
    min: p.REG_METRIC_MIN,
    max: p.REG_METRIC_MAX,
    value: p.REG_VALUE,
    timestamp: p.REG_TIMESTAMP,                              // unix seconds
    isoTime: new Date(p.REG_TIMESTAMP * 1000).toISOString(),
    // Which scheduled slot this answers (0 / '' = spontaneous).
    groupId: p.REG_GROUP_ID,
    groupName: p.REG_GROUP_NAME
  };
  registrations[p.REG_INDEX] = registration;
  console.log('Received registration ' + (p.REG_INDEX + 1) + '/' + p.REG_TOTAL +
    ': ' + JSON.stringify(registration));

  if (p.REG_INDEX === p.REG_TOTAL - 1) {
    // The watch gives up on an item after a few retries, which leaves a hole
    // at that index — deliver only the entries that actually arrived.
    var received = registrations.filter(function(r) { return r !== undefined; });
    var missing = p.REG_TOTAL - received.length;
    console.log('Export complete: ' + received.length + ' registrations' +
      (missing > 0 ? ' (' + missing + ' missing)' : ''));
    console.log('FULL EXPORT: ' + JSON.stringify(received));
    // TODO(companion): deliver `received` to the companion app / server.
  }
});
