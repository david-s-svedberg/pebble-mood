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
var COMPANION_PENDING_URL = 'http://localhost:9099/pending';
var COMPANION_PENDING_ACK_URL = 'http://localhost:9099/pending-ack';

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

function requestExport() {
  registrations = [];
  metrics = [];
  groups = [];
  Pebble.sendAppMessage(
    { 'EXPORT_REQUEST': 1 },
    function() {},
    function(e) { console.log('Failed to request export: ' + JSON.stringify(e)); }
  );
}

// Config changes edited in the companion queue there; we pull and apply them
// to the watch BEFORE requesting the export, so the export mirrors the new
// config back (self-verifying round trip).
function buildChangeMessage(c) {
  if (c.kind === 'group') {
    return {
      'SET_GROUP_ID': c.groupId || 0,
      'SET_GROUP_NAME': c.name || '',
      'SET_GROUP_HOUR': c.hour || 0,
      'SET_GROUP_MINUTE': c.minute || 0,
      'SET_GROUP_ACTIVE': c.active ? 1 : 0,
      'SET_GROUP_MEMBERS': c.members || []
    };
  }
  if (c.kind === 'metric') {
    return {
      'SET_METRIC_ID': c.metricId || 0,
      'SET_METRIC_NAME': c.name || '',
      'SET_METRIC_TYPE': c.typeCode || 0,
      'SET_METRIC_MIN': c.min || 0,
      'SET_METRIC_MAX': c.max || 0,
      'SET_METRIC_MAIN_ICON': c.mainIcon || 0
    };
  }
  return null;
}

function applyPendingChanges(changes) {
  var applied = [];
  var i = 0;
  function next() {
    if (i >= changes.length) {
      ackPendingChanges(applied);
      return;
    }
    var change = changes[i++];
    var msg = buildChangeMessage(change);
    if (!msg) { next(); return; }
    Pebble.sendAppMessage(msg,
      function() { applied.push(change.id); next(); },
      function(e) {
        // Watch unreachable mid-batch: ack what landed, retry rest next time.
        console.log('Config apply failed: ' + JSON.stringify(e));
        ackPendingChanges(applied);
      });
  }
  console.log('Applying ' + changes.length + ' config change(s) to watch');
  next();
}

function ackPendingChanges(ids) {
  if (ids.length === 0) { requestExport(); return; }
  var req = new XMLHttpRequest();
  req.open('POST', COMPANION_PENDING_ACK_URL, true);
  req.setRequestHeader('Content-Type', 'application/json');
  req.onload = function() { requestExport(); };
  req.onerror = function() { requestExport(); };
  req.send(JSON.stringify({ ids: ids }));
}

Pebble.addEventListener('ready', function() {
  console.log('Mood PebbleKit JS ready');
  var req = new XMLHttpRequest();
  req.open('GET', COMPANION_PENDING_URL, true);
  req.onload = function() {
    var changes = [];
    if (req.status === 200) {
      try { changes = JSON.parse(req.responseText).changes || []; }
      catch (e) { console.log('Pending parse failed: ' + e); }
    }
    if (changes.length > 0) { applyPendingChanges(changes); }
    else { requestExport(); }
  };
  req.onerror = function() { requestExport(); };
  req.send();
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
