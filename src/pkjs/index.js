// PebbleKit JS phone-side for Mood.
//
// On launch it asks the watch to export its config (metric/group definitions)
// and all registrations, receives them one message at a time, and delivers the
// batch to the companion app's local import listener (see
// design/companion_app_plan.md — nothing leaves the phone).

// The companion app's import listener (companion/: ImportServer).
// NOTE: must be the literal hostname `localhost` — the Core Devices app's XHR
// bridge errors on raw IPs (http://127.0.0.1:... gives onerror, verified on
// coreapp 1.5.0.2), while `localhost` resolves and connects fine.
var COMPANION_IMPORT_URL = 'http://localhost:9099/import';

var OPTION_TEXT_SEPARATOR = '\x1f';

var registrations = [];
var metrics = [];
var groups = [];

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

function deliverToCompanion() {
  var received = registrations.filter(function(r) { return r !== undefined; });
  var payload = {
    version: 2,
    exportedAt: Date.now(),
    config: { metrics: metrics, groups: groups },
    registrations: received
  };

  var req = new XMLHttpRequest();
  req.open('POST', COMPANION_IMPORT_URL, true);
  req.setRequestHeader('Content-Type', 'application/json');
  req.onload = function() {
    console.log('Companion delivery: HTTP ' + req.status);
    if (req.status === 200) {
      ackToWatch(req.responseText);
    }
  };
  req.onerror = function() {
    // Normal when the companion app isn't installed/running — the data stays
    // on the watch and is re-exported on the next launch.
    console.log('Companion delivery failed (listener not running?)');
  };
  req.send(JSON.stringify(payload));
}

// Tell the watch how far the companion has safely stored the export, so it can
// prune old synced registrations (its persist budget is finite). Losing this
// message is harmless — the next export re-acks.
function ackToWatch(responseText) {
  var ackedThrough = 0;
  try {
    ackedThrough = JSON.parse(responseText).ackedThrough || 0;
  } catch (e) {
    console.log('Ack parse failed: ' + e);
    return;
  }
  if (ackedThrough <= 0) {
    return;
  }
  Pebble.sendAppMessage(
    { 'EXPORT_ACK_THROUGH': ackedThrough },
    function() { console.log('Ack sent to watch: through ' + ackedThrough); },
    function(e) { console.log('Ack to watch failed: ' + JSON.stringify(e)); }
  );
}

Pebble.addEventListener('ready', function() {
  console.log('Mood PebbleKit JS ready — requesting export');
  registrations = [];
  metrics = [];
  groups = [];
  Pebble.sendAppMessage(
    { 'EXPORT_REQUEST': 1 },
    function() {},
    function(e) { console.log('Failed to request export: ' + JSON.stringify(e)); }
  );
});

Pebble.addEventListener('appmessage', function(e) {
  var p = e.payload;

  if (p.CFG_METRIC_ID !== undefined) {
    var texts = (p.CFG_METRIC_OPTION_TEXTS || '').split(OPTION_TEXT_SEPARATOR);
    metrics.push({
      metricId: p.CFG_METRIC_ID,
      name: p.CFG_METRIC_NAME,
      type: metricTypeName(p.CFG_METRIC_TYPE),
      min: p.CFG_METRIC_MIN,
      max: p.CFG_METRIC_MAX,
      mainIcon: p.CFG_METRIC_MAIN_ICON,
      optionIcons: p.CFG_METRIC_OPTION_ICONS || [],   // byte array -> number array
      optionTexts: texts
    });
    return;
  }

  if (p.CFG_GROUP_ID !== undefined) {
    groups.push({
      groupId: p.CFG_GROUP_ID,
      name: p.CFG_GROUP_NAME,
      hour: p.CFG_GROUP_HOUR,
      minute: p.CFG_GROUP_MINUTE,
      active: p.CFG_GROUP_ACTIVE === 1,
      members: p.CFG_GROUP_MEMBERS || []
    });
    return;
  }

  if (p.REG_INDEX !== undefined) {
    registrations[p.REG_INDEX] = {
      metricId: p.REG_METRIC_ID,
      metricName: p.REG_METRIC_NAME,
      metricType: metricTypeName(p.REG_METRIC_TYPE),
      min: p.REG_METRIC_MIN,
      max: p.REG_METRIC_MAX,
      value: p.REG_VALUE,
      timestamp: p.REG_TIMESTAMP,                              // unix seconds
      isoTime: new Date(p.REG_TIMESTAMP * 1000).toISOString(),
      groupId: p.REG_GROUP_ID,
      groupName: p.REG_GROUP_NAME
    };
    return;
  }

  if (p.EXPORT_DONE !== undefined) {
    var received = registrations.filter(function(r) { return r !== undefined; });
    console.log('Export complete: ' + metrics.length + ' metrics, ' +
      groups.length + ' groups, ' + received.length + ' registrations');
    deliverToCompanion();
  }
});
