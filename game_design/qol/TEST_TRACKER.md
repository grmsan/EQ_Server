# QoL Test Tracker

**Status**: Active Testing
**Tracker Area**: QoL
**Tracker State**: Active
**Last Updated**: 2026-03-13

## Purpose

Tracks quality-of-life systems that are not tightly scoped to one feature domain.

## Session Header

**Date**: __________
**Tester**: __________
**Build/Branch**: __________

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
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

## 2) Waypoint UI POCs (Custom DLL)

### [QOL-02] Build+Copy DLL + UI XML Sync

**Goal**: Verify Server Manager `Build+Copy DLL` updates client `dinput8.dll` and required UI XML assets for custom waypoint UI testing.
**Steps**:

1. Open `server_manager.py`, set `EQ Folder`, and click `Build+Copy DLL`.
2. Confirm client has `<EQ>/dinput8.dll` with recent timestamp.
3. Confirm client has:
   - `<EQ>/uifiles/default/EQUI_WaypointPOCWnd.xml`
   - `<EQ>/uifiles/default/EQUI_PowerSlotWnd.xml`
   - `<EQ>/scripts/waypoint_imgui_poc.lua`
4. Confirm client UI manifest (`EQUI.xml` or `default.xml`) includes:
   - `<Include>EQUI_WaypointPOCWnd.xml</Include>`
   - `<Include>EQUI_PowerSlotWnd.xml</Include>`
**Expected**: DLL, required XML files, and Lua/ImGui waypoint script are copied to client, with manifest includes present after one button press.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [QOL-03] Generic SIDL Tool Window - Waypoints (`/waypointpoc`, `/toolwnd`)

**Goal**: Verify the generic SIDL tool host can switch into waypoint mode, display server data, and trigger travel action.
**Steps**:

1. In game, run `#wppoc list` once (or use window refresh).
2. Run `/toolwnd` and verify the shared tool window opens.
3. Click `Waypoints` or run `/waypointpoc`.
4. Click `Refresh` and verify list populates with waypoint rows.
5. Select one row and click `Travel`.
6. Optionally run `/waypointpoc travel` after selecting a row.
**Expected**: Shared window renders waypoint list and selected waypoint travel triggers server-side teleport (`#wppoc travel <id>` path).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [QOL-03A] Generic SIDL Tool Window - GM Dashboard (`/gmdashboard`, `/toolwnd`)

**Goal**: Verify the same SIDL host can switch into GM/dev dashboard mode and run command-button style actions.
**Steps**:

1. Run `/gmdashboard` or open `/toolwnd` and click `GM Dash`.
2. Verify the list shows dashboard rows such as `Print Location`, `Reload Quests`, and `Repop Zone`.
3. Select a runnable row and click `Run`.
4. Select a template/manual row and click `Details`.
5. Verify the info text updates for the selected row.
**Expected**: Same shared window hosts non-waypoint functionality, runnable rows execute commands, and manual rows show guidance without issuing commands.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [QOL-04] Waypoint Overlay POC (`/waypointoverlay`)

**Goal**: Verify non-SIDL HUD overlay path can render waypoint list and travel.
**Steps**:

1. Run `/waypointoverlay toggle`.
2. Run `/waypointoverlay refresh`.
3. Use `/waypointoverlay next` and `/waypointoverlay prev` to move selection.
4. Run `/waypointoverlay travel`.
**Expected**: HUD list appears, selection changes, and travel command teleports to currently selected waypoint.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [QOL-05] Waypoint Lua/ImGui Runtime POC (`/waypointimgui`)

**Goal**: Verify Lua + Dear ImGui runtime path renders interactive waypoint UI and executes refresh/travel actions.
**Steps**:

1. Run `/waypointimgui toggle`.
2. Run `/waypointimgui status`.
3. Run `/waypointimgui refresh` and verify the ImGui host window renders with waypoint entries.
4. Select a waypoint row in the ImGui window and click `Travel`.
5. Run `/waypointimgui reload` and verify script reload succeeds.
**Expected**: Status reports runtime/hooks active, the separate ImGui host window is interactive, and refresh/travel work through the shared waypoint server bridge.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________
