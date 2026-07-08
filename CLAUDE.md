# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Mood is a **native (C) Pebble smartwatch app** for registering your mood and other contributing
factors on a schedule (concrete emotion metrics: Joy, Anxiety, Irritation, Stress, Love, …).
It targets **`basalt`, `diorite`, and `emery`** (Pebble Time 2) on Pebble SDK 4.x. *Aplite was
dropped* — its 24 KB app RAM could no longer fit the app. Data lives entirely in the watch's
persistent storage; a PebbleKit JS side (`src/pkjs/`) bridges to an **Android companion app**
(`companion/`, built — fas 2–7, see [ROADMAP.md](ROADMAP.md)). The companion is a full twin:
it receives registrations, edits the watch's config back (groups/metrics, incl. delete),
takes registrations in "phone mode", and does the analysis that deliberately does NOT live on
the watch — graphs, Spearman correlations, Health Connect, and an optional Claude-API pass.

The watch also has a **home-screen trend graph** (1–3 favourite metrics as 7-day sparklines,
`src/c/trend.c` + `favorites_window.c`) and **phone mode** (a global `alarms_suspended`
AppConfig flag; the companion owns reminders while the watch stays quiet).

## Build & run

The toolchain is the Core Devices `pebble-tool` CLI, installed in the dev container
(`.devcontainer/post-create.sh`). Develop inside the dev container ("Reopen in Container").

- Build: `pebble build` (VS Code: ctrl+b). Adding `messageKeys` in package.json requires a
  `pebble clean` first — `message_keys.auto.h` is not regenerated otherwise.
- Install on the physical watch: `pebble install --phone <PHONE_IP>` (the IP is hard-coded in
  [.vscode/tasks.json](.vscode/tasks.json)). The phone app must be foregrounded on its
  Developer Connection screen or the install times out / gets refused.
- Emulator: `pebble install --emulator emery` (also diorite/basalt). Notes: reinstalling
  restarts the app but PRESERVES persist storage; `pebble wipe` clears app storage;
  `pebble wipe --everything` fixes a boot-wedged emulator. Kill stray `qemu-pebble`
  processes before switching platforms — zombies wedge the tooling.
- There are no unit tests or linter. Verification = clean build on all three platforms +
  smoke test in the emulator (screenshots via `pebble screenshot`).
- After a longer implementation push, run the project's review skill:
  `/post-implementation-review` (eight passes, stops for approval between passes).

## Architecture

### Entry point and launch flow

`main()` calls `init()` / `deinit()` in [src/c/app.c](src/c/app.c). `init()` branches on
`launch_reason()`:
- **`APP_LAUNCH_WAKEUP`** — a scheduled registration alarm fired. `SUMMER_TIME_ALARM_ID`
  (254) reschedules all alarms (DST); `SNOOZED_ALARM_ID` (255) resolves the snoozed group via
  AppConfig; any other index is a group id. If the group is already fully answered today the
  app exits without UI or vibration.
- **Normal launch** — revives any dead group alarms (`ensure_all_alarms_scheduled`, idempotent),
  subscribes the wakeup handler, opens the AppMessage export channel, and shows the home screen.

`deinit()` refreshes the launcher app glance ("Next time: HH:MM") and frees icon caches.
Alarms sit on Pebble's `wakeup` service via [src/c/scheduler.c](src/c/scheduler.c); the app is
woken at scheduled times, prompts a registration, then exits. Group alarms re-arm themselves
for tomorrow when they fire.

### Screens and the window/logic split

Behaviour-heavy windows are split in two files: `*_window.c` (UI construction, Pebble-API-
facing) and `*_window_logic.c` (the behaviour invoked by handlers) — `main_window`,
`alarm_window`, `edit_alarm_window`, `register_mood_window` follow this. **Menu windows keep
their logic inline by design** (they are UI-dominant): `config_menu_window`,
`metrics_config_window`, `metric_group_config_window`, `metric_list_window`,
`icon_picker_window`.

All menus are custom `MenuLayer`s themed via [src/c/menu_theme.c](src/c/menu_theme.c)
(`menu_theme_apply_colors`, `menu_theme_icon_light`, `status_bar_create_themed`). Never use
`SimpleMenuLayer` — it can't follow the theme or recolour icons per row.

### Theme & icons

The app has a live dark/light theme (Settings → Theme, colors in AppConfig). Every window
reads the theme at `.load`; the main window re-themes in `.appear` because it stays on the
stack under Settings. Icons ship in black + white variants and are picked to contrast with
their background:
- `get_icon_by_choice_ex(choice, light)` — selectable metric icons (`IconChoice` enum,
  APPEND-ONLY: values are persisted on `Metrics`).
- `get_icon_row_by_choice(choice, light)` — 20 px menu-row variants of the large main icons.
- `get_bar_icon(BarIcon)` — fixed action-bar icons; the bar is foreground-coloured, so this
  returns the inverted variant in the light theme.

Icon PNGs are generated with a Pillow venv (black/white/20px/48px variants per icon) and
declared in package.json. `resources/images/mood_50.png`/`exercise_50.png` are generation
sources only (not packaged).

### Data model

Domain types live in [src/c/data.h](src/c/data.h): `AppConfig`, `Alarm`, `MetricsGroup`,
`Metrics` (BOOL / INTERVAL with `[min_value, max_value]` / THREE_OPTION, plus `main_icon`,
per-option icons and text ids), `Registration`, `GroupMetric` (many-to-many group membership)
and `String`. A `Registration` carries a `group_id` — which scheduled slot it answers
(0 = spontaneous) — so the same metric is tracked separately per group and day. Group slots
update in place when re-answered; spontaneous entries always append.

Id 0 is a reserved sentinel in every store (`next_id` starts at 1). `CURRENT_DATA_VERSION`
gates the AppConfig blob; the dynamic stores gate on `item_size` instead and reset on
mismatch (pre-release policy: reset beats corruption; no migrations yet).

### Repository / persistence layer

Persistence lives in [src/c/repositories/](src/c/repositories/) and is the only permitted
path to `persist_*`:
- `persist_blob.c` — **chunked storage**: `persist_write_data` silently truncates at
  `PERSIST_DATA_MAX_LENGTH` (256 B), so blobs are split across consecutive keys. The
  `DataKeys` enum therefore uses sparse base keys (32 apart) for the `*_DATA` blobs.
- `dynamic_repository.c` — generic persisted dynamic array (`DynamicData` + function
  pointers). `dynamic_add`/`dynamic_delete` REALLOC the items array — any held item pointer
  dangles afterwards; re-fetch by id or reconnect.
- `string_repository.c` — interned strings referenced by `title_id`/`option_text_ids`.
  Any string mutation reallocs the String array, so `metrics_repository` re-points every
  cached `title` afterwards (`reconnect_titles`). Never cache a `String*` across mutations.
- `metrics_repository.c` — metrics, groups, registrations, memberships + seeding.
- `app_config_repository.c` — the singleton `AppConfig` (theme, snooze/DST alarms, timeout,
  first-start flag — set only after seeding completes).

Accepted debt: persisted structs still contain runtime pointer fields (`String* title`,
`char* value`); they are ignored/rehydrated on load and cost a few bytes per record.

### Watch ↔ phone sync (pkjs + companion)

The link is **two-way**, bridged by [src/pkjs/index.js](src/pkjs/index.js):

- **Watch → phone (export):** [src/c/data_export.c](src/c/data_export.c) is a state machine
  that streams config (metrics + groups + memberships) then all registrations over AppMessage
  on request (serial, 3 retries per item; `outbox_failed` **skips** an item after retries and
  still sends `EXPORT_DONE`). The DONE message carries authoritative
  `EXPORT_METRIC_COUNT`/`EXPORT_GROUP_COUNT` so the companion only reconciles deletions
  (drops absent metrics/groups) on a *complete* export — a skipped item never causes a false
  delete.
- **Phone → watch (apply):** [src/c/config_apply.c](src/c/config_apply.c) applies queued
  companion edits relayed as `SET_*` messages: group/metric create+edit (id 0 = create, watch
  allocates the id), `SET_ALARMS_SUSPENDED` (phone mode), `SET_REG_*` (a registration answered
  on the phone), and `SET_DELETE_GROUP_ID`/`SET_DELETE_METRIC_ID`.

Keep the 4-way contract in sync: `messageKeys` (package.json) ↔ `MESSAGE_KEY_*` (C) ↔ pkjs
payload keys ↔ companion (`ConfigSync`/`ImportRepository`). **Gotcha (bitten twice):** editing
`src/pkjs/index.js` or `messageKeys` after a build does NOT regenerate the JS bundle —
`pebble install` ships stale JS. Always `pebble clean && pebble build` when either changes;
verify with `grep <new-string> build/pebble-js-app.js`.

### Companion app (`companion/`)

Kotlin + Jetpack Compose + Room (`./gradlew :app:assembleDebug`; install on the phone over
adb). A foreground service owns a loopback-only `ServerSocket` that pkjs POSTs to; imports are
idempotent (unique `metricId,groupId,timestamp` + IGNORE). Phone-side config edits queue as
`PendingChangeEntity` and are pulled+applied by pkjs on the next watch launch. Health Connect
data (steps/sleep/heart-rate the Core Devices app writes) is read via a WorkManager worker and
exposed as synthetic auto-metrics (ids 9001–9003, kept out of the watch id-space). Nothing
leaves the phone except the optional Claude-API analysis call. Tabs: Graf, Insikter, Konfig,
Telefon, Status.
