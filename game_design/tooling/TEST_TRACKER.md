# Tooling Test Tracker

**Status**: Active Testing
**Tracker Area**: Tooling
**Tracker State**: Active
**Last Updated**: 2026-02-28

## Purpose

Tracks developer tooling UX and workflow surfaces (Server Manager, logs, operator controls).

## Session Header

**Date**: __________
**Tester**: __________
**Build/Branch**: __________

---

## 1) Server Manager UX

### [TOOL-01] Server Manager Main Layout

**Goal**: Verify new layout is usable for live operation.
**Legacy ID**: `T-01`
**Steps**:

1. Open `server_manager.py`.
2. Go to `Server Control`.
3. Confirm left operations rail + right live workspace are visible.
4. Confirm service cards and runtime log viewer are usable.
**Expected**: Layout is coherent and actions are discoverable quickly.
**Status**: [x] Pass  [ ] Fail
**Notes**: ______________________________

### [TOOL-02] Runtime Log Focus Buttons

**Goal**: Verify rapid log targeting.
**Legacy ID**: `T-02`
**Steps**:

1. Open Runtime Log Viewer.
2. Click `Active Zone Log`.
3. Click `Active World Log`.
4. Click `Most Recent Log`.
**Expected**: Log source changes correctly each time.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TOOL-03] Test Manager Tracker Shortcuts + Discovery

**Goal**: Verify multi-tracker workflow can be navigated quickly.
**Steps**:

1. Open `Test Manager`.
2. Use quick buttons (`Multiclass`, `Operations`, `Classes`, `Mechanics`, `Quests`, `QoL`, `Tooling`) and confirm each loads the expected tracker.
3. Use `Refresh List` and confirm tracker picker includes active trackers under `game_design`.
4. Toggle `Show Concept Trackers` and confirm concept-only trackers are shown/hidden correctly.
**Expected**: Area switching is one-click and tracker discovery stays accurate.
**Status**: [x] Pass  [ ] Fail
**Notes**: ______________________________

### [TOOL-04] Test Manager Left/Right Pane Resize Persistence

**Goal**: Verify list/details split is usable on both portrait and wide monitors and persists between app launches.
**Steps**:

1. Open `Test Manager`.
2. On a narrow/portrait layout (around `1200x1920`), confirm default split keeps both list and details readable.
3. Drag divider between `Tests` and `Test Details` to a preferred position.
4. Move to a wide layout (around `2560x1440`) and confirm split scales to remain usable.
5. Close and reopen `server_manager.py`.
6. Return to `Test Manager`.
**Expected**: Default split is sane on both widths, manual adjustment persists, and split remains readable after relaunch.
**Status**: [x] Pass  [ ] Fail
**Notes**: ______________________________

### [TOOL-05] `#test 16` Client Probe Round-Trip

**Goal**: Verify server can request client DLL probe data and receive callback for validation.
**Steps**:

1. Use a RoF2 client with current `dinput8.dll`.
2. Run `#test 16`.
3. Watch chat for initial pending line and follow-up `CLIENT PASS/FAIL` callback line.
**Expected**: Server dispatches probe via EdgeStatLabel and receives structured `#test clientreplyv2` callback from DLL.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TOOL-06] Ops Command Coverage: NPC Appearance Randomizers

**Goal**: Ensure new visual tooling commands are documented and easy to find in the operations workflow.
**Steps**:

1. Open `game_design/operations/COMMAND_RUNBOOK.md`.
2. Confirm `#randomize`, `#randomizegear`, `#randomfeatures`, and `#npcedit featuresave` are listed.
3. Open `game_design/operations/WORK_TRACKER.md` and confirm there is a command validation task for the randomizer workflow.
**Expected**: Appearance tooling is discoverable from runbook + work tracker without relying on chat history.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TOOL-07] `#test 17` Class-Mask Mutation Round-Trip + Auto-Restore

**Goal**: Verify class add/remove mutations propagate through the DLL probe path and restore character class state automatically.
**Steps**:

1. Use a RoF2 client with current `dinput8.dll`.
2. Run `#test 17`.
3. Confirm chat shows mutation dispatch line with expected class mask.
4. Confirm follow-up `CLIENT PASS/FAIL` line for class mask parity.
5. Confirm follow-up restore line shows class state restored.
**Expected**: DLL callback class mask matches server-expected mutated mask and character class bits are auto-restored after callback.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TOOL-08] `#test 18` Mana Mutation Round-Trip + Auto-Restore

**Goal**: Verify class mutation propagates mana-state changes through DLL probe callback and restores class state after validation.
**Steps**:

1. Use a RoF2 client with current `dinput8.dll`.
2. Run `#test 18`.
3. Confirm chat shows expected mana max transition in dispatch line.
4. Confirm `CLIENT PASS/FAIL` line validates callback `mana_max` against server expected value.
5. Confirm restore line reports class state was restored.
**Expected**: Callback `mana_max` matches server expected post-mutation value and mutation is auto-restored.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TOOL-09] `#test 19` Snapshot Parity Round-Trip

**Goal**: Verify DLL callback parity for key live snapshot fields (class mask, HP max, mana max, endurance max).
**Steps**:

1. Use a RoF2 client with current `dinput8.dll`.
2. Run `#test 19`.
3. Confirm callback validates class mask + HP/mana/end max parity.
**Expected**: Callback values match server expected snapshot values for requested fields.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TOOL-10] `#test 20` Persisted Class-Mask Parity Round-Trip

**Goal**: Verify runtime/profile/bucket class-mask persistence consistency and DLL callback parity in one probe flow.
**Steps**:

1. Use a RoF2 client with current `dinput8.dll`.
2. Run `#test 20`.
3. Confirm preflight checks pass (`runtime`, `PlayerProfile.classes`, `GestaltClasses`, `legacy bucket`).
4. Confirm callback class mask matches persisted/runtime expected mask.
**Expected**: Persisted sources agree and DLL callback class mask matches the same value.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TOOL-11] Build+Copy DLL + UI Asset Deployment

**Goal**: Verify Server Manager `Build+Copy DLL` deploys the required DLL and UI assets to the client install.
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
**Expected**: DLL, XML, and script assets are deployed correctly in one operator action. This validates deployment only, not the runtime behavior of the QoL waypoint hosts.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________
