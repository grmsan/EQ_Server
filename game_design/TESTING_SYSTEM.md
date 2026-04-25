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

## Project Dashboard

For a consolidated view of all active work, progress tracking, and session logs, see the master dashboard:

**[PROJECT_DASHBOARD.md](../PROJECT_DASHBOARD.md)** — Single entry point for picking up where you left off.

The dashboard provides:
- **Return After Break** quickstart (5-minute orientation)
- **Session Log** for recording what you worked on
- **Active Work Items** prioritized by urgency
- **Progress Overview** by domain with status indicators
- **Quick Links** to all trackers and implementation plans

Use the dashboard as your starting point after extended breaks. Use individual trackers for detailed test execution and validation.

## Management Layer

Use the management docs when work needs to be assigned to another agent or reviewed at a high level:

- **[PROJECT_MANAGEMENT.md](../PROJECT_MANAGEMENT.md)** — operating model, priority definitions, and manager review cadence.
- **[PROJECT_WORKSTREAMS.md](../PROJECT_WORKSTREAMS.md)** — executive priority board and active work packets.
- **[AGENT_HANDOFF.md](../AGENT_HANDOFF.md)** — required agent instructions, evidence template, and closeout template.

Trackers remain the validation source of truth. Work packets point agents at the right tracker cases and define the scope, done criteria, and update requirements.
