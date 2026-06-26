// PebbleKit JS phone-side for Mood.
//
// On launch it asks the watch to export all registrations, receives them one
// message at a time, and assembles them into a list. This is the bridge the
// companion app will use to pull data for mood-vs-activity correlation.
// For now the assembled list is logged; sending it onward (server / companion
// app) is the next step.

var registrations = [];

function metricTypeName(type) {
  // Mirrors MetricsType in src/c/data.h (NONE=0, BOOL=1, INTERVAL=2).
  return type === 2 ? 'interval' : 'bool';
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
    value: p.REG_VALUE,
    timestamp: p.REG_TIMESTAMP,                              // unix seconds
    isoTime: new Date(p.REG_TIMESTAMP * 1000).toISOString()
  };
  registrations[p.REG_INDEX] = registration;
  console.log('Received registration ' + (p.REG_INDEX + 1) + '/' + p.REG_TOTAL +
    ': ' + JSON.stringify(registration));

  if (p.REG_INDEX === p.REG_TOTAL - 1) {
    console.log('Export complete: ' + registrations.length + ' registrations');
    console.log('FULL EXPORT: ' + JSON.stringify(registrations));
    // TODO(companion): deliver `registrations` to the companion app / server.
  }
});
