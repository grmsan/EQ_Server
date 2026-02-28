# Testing System

## Goal

Provide one consistent test workflow across all development areas without forcing every area to have a tracker before it is test-ready.

Use [tooling/DLL_AUTOMATION_MATRIX.md](tooling/DLL_AUTOMATION_MATRIX.md) for DLL automation coverage and rollout priorities.

## Current Model

- One `Test Manager` UI in `server_manager.py`.
- Multiple markdown trackers, one per area.
- Status/notes are isolated per tracker path, so overlapping IDs (for example `C-01`) do not collide.

## Tracker Discovery

The Test Manager auto-discovers tracker files under `game_design`:

- `TEST_TRACKER.md`
- `WORK_TRACKER.md`
- `*_TRACKER.md`

Use the tracker picker in Test Manager to switch between areas.
By default, trackers marked `Tracker State: Concept` are hidden (toggle `Show Concept Trackers` in UI to include them).
Use quick area buttons for high-traffic trackers (`Multiclass`, `Operations`, `Classes`, `Mechanics`, `Quests`, `QoL`, `Tooling`) when iterating rapidly.

## When To Create A Tracker

- `Concept`: design docs only, no tracker needed yet.
- `WIP`: create tracker only when there is executable validation to run.
- `Active`: maintain tracker as source of truth for pass/fail and evidence.

## Recommended Layout

For each area that is test-ready:

1. Place tracker in that area folder.
2. Add metadata header:
   - `Tracker Area`
   - `Tracker State` (`Active`, `WIP`, or `Concept`)
3. Use stable IDs. Prefer area prefixes for non-core domains (`CL-01`, `MECH-01`, `QST-01`, `QOL-01`, `TOOL-01`, `IP-101`, etc.).
4. Keep each case structured as:
   - `Goal`
   - `Steps`
   - `Expected`
   - `Status`
   - `Notes`

Examples:

- `game_design/multiclass/TEST_TRACKER.md`
- `game_design/operations/WORK_TRACKER.md`
- `game_design/classes/TEST_TRACKER.md`
- `game_design/mechanics/TEST_TRACKER.md`
- `game_design/quests/TEST_TRACKER.md`
- `game_design/qol/TEST_TRACKER.md`
- `game_design/tooling/TEST_TRACKER.md`

## Practical Guidance

- Keep concept-heavy systems (for example `infinite_progression`, `evercraft`) out of active testing until core behavior is runnable.
- Once runnable, add a tracker in that folder and it will appear automatically in Test Manager.
