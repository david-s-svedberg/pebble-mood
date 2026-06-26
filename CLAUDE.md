# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Mood is a **native (C) Pebble smartwatch app** for registering your mood and other contributing factors on a schedule. It targets the `aplite`, `basalt`, and `diorite` platforms (Pebble SDK 3). There is no app store backend — data lives entirely in the watch's persistent storage.

## Build & run

The toolchain is the Core Devices `pebble-tool` CLI, installed in the dev container (`.devcontainer/post-create.sh`). Develop inside the dev container ("Reopen in Container" in VS Code).

- Build: `pebble build` (VS Code: ctrl+b, the default build task)
- Build + install on a physical watch: `pebble build && pebble install --logs --phone <PHONE_IP>` (VS Code: ctrl+i). The phone IP is currently hard-coded in [.vscode/tasks.json](.vscode/tasks.json) — update it for your device.
- There are no unit tests, linter, or emulator workflow configured in this repo. JS linting (jshint) in `wscript` is gated behind `if False` and disabled.

The build is driven by the waf-based [wscript](wscript): it compiles `src/c/**/*.c` into one app binary per target platform, and bundles JS from `src/pkjs/**` (note: `src/pkjs/` does not currently exist — this is a pure on-watch app with no PebbleKit JS phone-side code yet).

## Architecture

### Entry point and launch flow

`main()` ([src/c/main.c](src/c/main.c)) calls `init()` / `deinit()` in [src/c/app.c](src/c/app.c). `init()` branches on `launch_reason()`:
- **`APP_LAUNCH_WAKEUP`** — the watch woke the app to fire a scheduled registration alarm. Routes to `setup_alarm_window(alarm_index)`. A special `SUMMER_TIME_ALARM_ID` wakeup instead reschedules all alarms (DST handling).
- **Normal launch** — subscribes the wakeup handler and opens `setup_config_window()`.

Alarms are implemented on top of Pebble's `wakeup` service via [src/c/scheduler.c](src/c/scheduler.c). The app is not always running; it is woken at scheduled times to prompt a registration, then exits.

### Window / logic split (the core pattern)

Each screen is split into two files:
- `*_window.c` — UI construction: layers, menus, click handlers, drawing. Pebble-API-facing.
- `*_window_logic.c` — the behavior/business logic invoked by the window's handlers.

Screens: `main_window`, `config_menu_window`, `register_mood_window`, `alarm_window`, `edit_alarm_window`, `metrics_config_window`, `metric_group_config_window`. Follow this split when adding or modifying screens.

### Data model

All domain types live in [src/c/data.h](src/c/data.h): `AppConfig`, `Alarm`, `MetricsGroup`, `Metrics` (type `BOOL` or `INTERVAL` with a `max_value`), `Registration`, and `String`. A `MetricsGroup` has a scheduled `Alarm` and contains `Metrics`; each `Registration` records a metric's `value` at a `time_stamp`. See [design/design_notes.txt](design/design_notes.txt) for the conceptual data shape and use cases.

The `DataKeys` enum in `data.h` enumerates the persistent-storage keys used across repositories. `CURRENT_DATA_VERSION` gates migrations.

### Repository / persistence layer

Persistence lives in [src/c/repositories/](src/c/repositories/) and is the intended data API — prefer it over touching `persist_*` directly:
- `dynamic_repository.c` — a **generic, reusable persisted dynamic array**. A `DynamicData` struct binds a meta-data key + items key + item size, and the caller supplies function pointers (`SetItemId`, `GetItemId`, `SameIdPredicate`) so the same add/delete/get/save logic works for any record type.
- `string_repository.c` — interned strings (titles), stored separately from the records that reference them by `title_id`; pointers are re-`connect`ed to their char data after load.
- `metrics_repository.c` — metrics, metric groups, and registrations, each backed by a `DynamicData`.
- `app_config_repository.c` — the singleton `AppConfig` (theme colors, snooze/DST alarms, timeout, first-start flag).

Records reference strings and groups by id (`title_id`, `group_id`) on disk, and the repositories rehydrate the in-memory pointers (`String* title`, `MetricsGroup* group`) after loading.

## Transitional state (important)

This repo is mid-refactor. The latest commit message is *"WIP compiling but runtime errors and missing registration window"*. Be aware:
- [src/c/persistance.h](src/c/persistance.h) is **fully commented out** and [src/c/main_window.h](src/c/main_window.h) / `main_window_logic.h` are commented out, while `persistance.c` still compiles. The repository layer in `repositories/` is the replacement for the old `persistance.c` monolith — new persistence code should go through the repositories, not `persistance.c`.
- Don't assume a screen or persistence path is wired up just because its file exists; verify against `app.c`'s actual flow.
