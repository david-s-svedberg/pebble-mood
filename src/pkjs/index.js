// PebbleKit JS phone-side for Mood.
//
// On launch it asks the watch to export all registrations, receives them one
// message at a time, assembles them into a list and delivers the batch to the
// companion app's local import listener (see design/companion_app_plan.md —
// the companion runs an HTTP listener on localhost; nothing leaves the phone).

// The companion app's import listener (companion/: ImportServer).
// NOTE: must be the literal hostname `localhost` — the Core Devices app's XHR
// bridge errors on raw IPs (http://127.0.0.1:... gives onerror, verified on
// coreapp 1.5.0.2), while `localhost` resolves and connects fine.
var COMPANION_IMPORT_URL = 'http://localhost:9099/import';

var registrations = [];

function deliverToCompanion(received) {
  var req = new XMLHttpRequest();
  req.open('POST', COMPANION_IMPORT_URL, true);
  req.setRequestHeader('Content-Type', 'application/json');
  req.onload = function() {
    console.log('Companion delivery: HTTP ' + req.status);
  };
  req.onerror = function() {
    // Normal when the companion app isn't installed/running — the data stays
    // on the watch and is re-exported on the next launch.
    console.log('Companion delivery failed (listener not running?)');
  };
  req.send(JSON.stringify({ version: 1, exportedAt: Date.now(), registrations: received }));
}

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
    deliverToCompanion(received);
  }
});
