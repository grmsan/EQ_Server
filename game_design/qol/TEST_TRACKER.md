# QoL Test Tracker

**Status**: Active Testing
**Tracker Area**: QoL
**Tracker State**: Active
**Last Updated**: 2026-05-02

## Purpose

Tracks quality-of-life systems that are not tightly scoped to one feature domain.

Tests marked `[DLL POC]` require a built and deployed `dinput8.dll` with the matching UI assets. Validate `TOOL-11` (Build+Copy DLL) in the Tooling tracker before running these.

## Status Rules

- `Not Run`: Ready for validation but not executed in the current build.
- `In Progress`: Actively being tested or partially executed.
- `Blocked`: Cannot be validated due to environment, data, or design dependency.
- `Pass`: Expected behavior confirmed with objective evidence.
- `Fail`: Behavior mismatches expected results or produces a regression.

## Evidence Format

`Observed:` one-line actual result.
`Evidence:` exact chat/log snippet, command output, or screenshot reference.
`Commands:` minimal command sequence to reproduce.
`Next:` immediate code/data target if blocked or failed.

## Current Validation Focus

1. **QOL-01** (Bazaar and Back AA) is the only always-run test — validate after any travel or AA system changes.
2. **QOL-03** through **QOL-05** are DLL POC experiments. Run these only after deploying the DLL via `TOOL-11`. Mark as `Blocked` if DLL/assets are not deployed.

## Session Header

**Date**: __________
**Tester**: __________
**Build/Branch**: __________

## Run Packs

### Smoke Run

Run: `QOL-01`

### Full Regression (DLL required)

Run `QOL-01`, then `QOL-03`, `QOL-03A`, `QOL-04`, `QOL-05` after DLL deployment is confirmed via `TOOL-11`.

---

## 1) Travel QoL

### [QOL-01] Bazaar and Back AA (Instance-Aware Return)

**Goal**: Verify THJ-style `Bazaar and Back` AA is auto-granted, ports to Bazaar, and returns to saved location including instances across relog.
**Legacy ID**: `B-09`
**Steps**:

1. Log in on a fresh character and open AA window; confirm `Bazaar and Back` is present.
2. In a normal zone, activate `Bazaar and Back` and verify you port to Bazaar.
3. Activate it again in Bazaar and verify you return to your original coordinates.
4. Repeat from an instance (save location in instance, port to Bazaar, camp/relog in Bazaar, then use AA again).
5. Trigger AA twice quickly and verify reuse lockout message/cooldown behavior (~60s).

**Expected**: AA is auto-granted to all characters, uses 60s cooldown, and return location persists across relog with instance-aware restore.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 2) Waypoint UI POCs (Custom DLL) — Requires DLL Deployment

> **Precondition for all tests in this section**: Run `TOOL-11` (Build+Copy DLL) in the Tooling tracker first to confirm the DLL and UI assets are deployed. Mark these as `Blocked` if that step has not been validated.

### [QOL-03] [DLL POC] Generic SIDL Tool Window - Waypoints (`/waypointpoc`, `/toolwnd`)

**Goal**: Verify the generic SIDL tool host renders waypoint data and accepts waypoint-specific input.
**Precondition**: `TOOL-11` validated; waypoint backend/list source is known-good.
**Steps**:

1. In game, run `#wppoc list` once (or use window refresh).
2. Run `/toolwnd` and verify the shared tool window opens.
3. Click `Waypoints` or run `/waypointpoc`.
4. Click `Refresh` and verify list populates with waypoint rows.
5. Select one row and click `Travel`.

**Expected**: The SIDL host renders the waypoint list correctly, maintains selection state, and invokes the shared travel action from the selected row.
**Status**: [ ] Not Run  [ ] In Progress  [x] Blocked  [ ] Pass  [ ] Fail
**Notes**: Blocked pending DLL asset deployment (TOOL-11).

### [QOL-03A] [DLL POC] Generic SIDL Tool Window - GM Dashboard (`/gmdashboard`, `/toolwnd`)

**Goal**: Verify the same SIDL host can switch into GM/dev dashboard mode and run command-button style actions.
**Precondition**: `TOOL-11` validated.
**Steps**:

1. Run `/gmdashboard` or open `/toolwnd` and click `GM Dash`.
2. Verify the list shows dashboard rows such as `Print Location`, `Reload Quests`, and `Repop Zone`.
3. Select a runnable row and click `Run`.
4. Select a template/manual row and click `Details`.
5. Verify the info text updates for the selected row.

**Expected**: Same shared window hosts non-waypoint functionality, runnable rows execute commands, and manual rows show guidance without issuing commands.
**Status**: [ ] Not Run  [ ] In Progress  [x] Blocked  [ ] Pass  [ ] Fail
**Notes**: Blocked pending DLL asset deployment (TOOL-11).

### [QOL-04] [DLL POC] Waypoint Overlay POC (`/waypointoverlay`)

**Goal**: Verify the non-SIDL HUD overlay host renders waypoint data and accepts overlay navigation input.
**Precondition**: `TOOL-11` validated; waypoint backend/list source is known-good.
**Steps**:

1. Run `/waypointoverlay toggle`.
2. Run `/waypointoverlay refresh`.
3. Use `/waypointoverlay next` and `/waypointoverlay prev` to move selection.
4. Run `/waypointoverlay travel`.

**Expected**: HUD list appears, selection changes visibly in the overlay, and the overlay invokes travel for the currently selected waypoint.
**Status**: [ ] Not Run  [ ] In Progress  [x] Blocked  [ ] Pass  [ ] Fail
**Notes**: Blocked pending DLL asset deployment (TOOL-11).

### [QOL-05] [DLL POC] Waypoint Lua/ImGui Runtime POC (`/waypointimgui`)

**Goal**: Verify the Lua + Dear ImGui host renders waypoint data and accepts interactive refresh/travel input.
**Precondition**: `TOOL-11` validated; waypoint backend/list source is known-good.
**Steps**:

1. Run `/waypointimgui toggle`.
2. Run `/waypointimgui status`.
3. Run `/waypointimgui refresh` and verify the ImGui host window renders with waypoint entries.
4. Select a waypoint row in the ImGui window and click `Travel`.
5. Run `/waypointimgui reload` and verify script reload succeeds.

**Expected**: Status reports runtime/hooks active, the ImGui host window is interactive, and the host successfully invokes the shared waypoint refresh/travel bridge.
**Status**: [ ] Not Run  [ ] In Progress  [x] Blocked  [ ] Pass  [ ] Fail
**Notes**: Blocked pending DLL asset deployment (TOOL-11).

---

## Agent Closeout Requirement

When an agent validates or changes QoL work, it must update this tracker:

1. Set one clear status per case.
2. Add concise notes using `Observed`, `Evidence`, `Commands`, and `Next`.
3. Mark blocked cases as `Blocked`, not `Fail`, unless game behavior itself failed.
4. Clear `Blocked` status on DLL POC tests only after `TOOL-11` is confirmed `Pass`.
