# Quests & Instances Test Tracker

**Status**: Active Testing
**Tracker Area**: Quests
**Tracker State**: Active
**Last Updated**: 2026-02-28

## Purpose

Tracks quest/NPC script behavior, waypoint/discovery flow, and custom instance helper functionality.

## How To Use

1. Fill out `Session Header`.
2. Run `Smoke Run` first.
3. Record pass/fail plus direct script/log evidence.

## Session Header

**Date**: __________
**Tester**: __________
**Build/Branch**: __________
**Zone Logs Captured**: `yes/no`

## Run Packs

### Smoke Run

Run: `QST-01, QST-03, QST-05, QST-06`

### Full Regression

Run all tests in this document.

---

## 1) THJ Bazaar + Waypoints

### [QST-01] Magic Map Door Opens Waypoint UI

**Goal**: Verify Bazaar map object hook is wired (`doorid=146` -> `SendWaypointList`).
**Legacy ID**: `B-01`
**Steps**:

1. Zone into `bazaar`.
2. Click the magic map object/disc at the Bazaar hub.
**Expected**: Waypoint list UI opens (no quest/script errors).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [QST-02] Tearel Group Feature Unlock

**Goal**: Verify Tearel dialogue and group-feature unlock path.
**Legacy ID**: `B-02`
**Steps**:

1. Hail `Tearel`.
2. Follow dialogue for `[anchor yourself]` and pay required EOM.
3. Re-open map UI.
**Expected**: Group toggle / expedition return options are enabled after unlock.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [QST-03] Waypoint Discovery Trigger

**Goal**: Verify `#TPTriggerN` discovery unlock flow.
**Legacy ID**: `B-03`
**Setup**:

1. Apply `utils/sql/custom/2026_02_26_thj_waypoints_bazaar_bootstrap.sql` (includes `#TPTriggerN` spawn seeding from `thj_waypoints`).
**Steps**:

1. Enter a zone with `#TPTriggerN` proximity trigger.
2. Observe discovery message.
3. Re-open map UI and confirm zone appears unlocked.
**Expected**: First pass unlocks waypoint; repeat pass shows already-known messaging.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [QST-04] Pet Armory Merchant in Bazaar

**Goal**: Verify pet bag vendor availability in Bazaar.
**Legacy ID**: `B-06`
**Steps**:

1. Open merchant window on `Pet_Armory_Quartermaster`.
2. Confirm class pet armory items are listed.
3. Purchase one bag and test active pet bag behavior with a pet class.
**Expected**: Merchant sells THJ pet bags and purchased bag works with pet equipment sync.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [QST-05] THJ Quest API Binding Smoke Test

**Goal**: Verify imported THJ quest methods are fully bound (no undefined method errors at runtime).
**Legacy ID**: `B-07`
**Setup**:

1. Use a build that includes the latest zone script binding changes.
2. Ensure zone logs are visible in the runtime log viewer.
**Steps**:

1. Trigger a script path that calls `$client->HasClass(\"Cleric\")` (example: `ikkinz/#Phantasmal_Priest.pl` dialog).
2. Trigger Bazaar pet rename path (`151061.pl`) that calls `$client->IsPetNameChangeAllowed()` and `GrantPetNameChange(class_id)`.
3. Trigger spell script `quests/global/spells/17785.pl` (`ConsumeItemOnCursor`) and `36892.pl` (`ConsumeUnspentAA`).
4. Trigger a progression/slayer script path using `IsSeasonal`, `GetKillCount`, and `CheckTitle`.
**Expected**: No Perl/Lua undefined-method errors; script behaviors execute normally.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 2) Custom Instances (THJ-Style)

### [QST-06] Non-Respawning Instance Request + Daily Lockout

**Goal**: Verify non-respawning expedition request is available and locked out for 24 hours after creation.
**Legacy ID**: `Z-01`
**Steps**:

1. Hail `Echo_of_the_Past` and choose `Non-Respawning`.
2. Confirm expedition is created and can be entered.
3. Attempt to request another non-respawning instance immediately.
**Expected**: First request succeeds; second request is blocked by replay lockout until 24 hours have elapsed.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [QST-07] Non-Respawning Spawn Behavior

**Goal**: Verify static instance kills do not naturally repop while instance is active.
**Legacy ID**: `Z-02`
**Steps**:

1. Enter a `Non-Respawning` instance and kill a normal mob.
2. Wait longer than that mob's normal respawn window.
3. Zone out/in and re-check the kill location.
**Expected**: Killed mob does not return during normal respawn windows.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [QST-08] Farming Instance Raid Suppression

**Goal**: Verify `Respawning` farming instance suppresses long-respawn/raid-style spawns.
**Legacy ID**: `Z-03`
**Steps**:

1. Enter a `Respawning` instance of a zone with known long-respawn raid targets.
2. Verify trash/XP mobs spawn as expected.
3. Verify known long-respawn raid target spawn points remain suppressed.
**Expected**: XP/trash population exists; raid-style long-respawn bosses are not present.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [QST-09] Lua Helper Availability (`eq.is_static_instance`, `eq.is_farming_instance`)

**Goal**: Verify THJ quest scripts can call custom instance helpers without runtime errors.
**Legacy ID**: `Z-04`
**Steps**:

1. Trigger a Lua quest script path that uses `eq.is_static_instance()` or `eq.is_farming_instance()`.
2. Repeat in both non-respawning and respawning instances.
**Expected**: No Lua nil-function errors; helper-based branch behavior matches instance type.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 3) Spell Script Runtime Regressions

### [QST-10] Summon Orb Script Runtime (1944.pl)

**Goal**: Verify `Summon Orb` no longer throws undefined quest API errors and still performs intended item swap behavior.
**Setup**:

1. Build includes `quests/global/spells/1944.pl` using `quest::summonitem(...)`.
**Steps**:

1. Cast `Summon Orb`.
2. Watch zone/chat logs for `QuestErrors`.
3. Confirm summoned item appears and source item removal behavior remains correct.
**Expected**: No `Undefined subroutine &quest::summonfixeditem` errors; spell path executes cleanly.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________
