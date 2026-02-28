# Server Operations Work Tracker

**Status**: Active
**Tracker Area**: Operations
**Tracker State**: Active
**Last Updated**: 2026-02-28
**Runbook**: [COMMAND_RUNBOOK.md](COMMAND_RUNBOOK.md)

## Scope

Tracks non-multiclass engineering work:

1. Server/bootstrap setup tasks
2. Bazaar NPC migration and placement
3. Command tooling verification
4. SQL/data export/import operational tasks

## How To Use

1. Pick a scope (`Smoke Ops` or `Full Ops`).
2. Run tests/tasks in ID order.
3. Mark each entry `Pass` or `Fail`.
4. Add exact evidence (command output, chat lines, logs, SQL script names).
5. If failed, include the next action and owner.

## Run Packs

### Smoke Ops (15-20 min)

Run: `O-01, O-02, B-01, B-02, B-03, C-01, C-02`

### Full Ops (45-90 min)

Run all entries.

---

## Session Header

**Date**: __________
**Operator**: __________
**Build/Branch**: __________
**Zone/Instance**: __________
**DB Name**: __________
**Scripts Applied This Session**: ______________________________

---

## 1) Environment & Preflight

### [O-01] Command Access Baseline

**Goal**: Confirm operator account can run required setup commands.
**Steps**:

1. Run `#help bazaarpull`.
2. Run `#help spawnfix`.
3. Run `#help npcspawn`.
**Expected**: All commands show usage/help text without permission denial.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [O-02] Bazaar Zone Preflight

**Goal**: Confirm Bazaar zone is loaded and usable for placement workflow.
**Steps**:

1. Zone to Bazaar.
2. Run `#loc`.
3. Run `#list npcs`.
**Expected**: Zone is stable; commands return expected output.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [O-03] SQL Baseline Inventory

**Goal**: Confirm required bootstrap/repair scripts exist in repo before ops run.
**Steps**:

1. Verify scripts exist in `utils/sql/custom` for current task.
2. Record target scripts in session header.
**Expected**: Required scripts are present and identified before execution.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 2) Bazaar Placement Workflow

### [B-01] Staged NPC Count Discovery

**Goal**: Determine current backlog of staged Bazaar NPCs.
**Steps**:

1. In Bazaar, run `#bazaarpull status`.
**Expected**: Command returns staged/unplaced count.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [B-02] Pull Single NPC

**Goal**: Pull one staged NPC to operator for deterministic placement.
**Steps**:

1. Run `#bazaarpull`.
2. Confirm one NPC appears near operator.
**Expected**: Exactly one staged NPC is pulled and identified.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [B-03] Persist Placement With Spawnfix

**Goal**: Save target NPC location to database.
**Steps**:

1. Move targeted NPC to desired spot.
2. Run `#spawnfix`.
3. Move away and `#repop` (or relog/zone) to verify persistence.
**Expected**: NPC respawns at saved location.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [B-04] Batch Placement Loop

**Goal**: Execute full staged-NPC placement cycle without misses.
**Steps**:

1. Repeat `#bazaarpull` -> move NPC -> `#spawnfix`.
2. Re-run `#bazaarpull status` after each batch.
**Expected**: Staged count steadily decreases with no stranded NPCs.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [B-05] Completion Check (Zero Staged)

**Goal**: Confirm all staged NPCs have been placed.
**Steps**:

1. Run `#bazaarpull status` until count reaches 0.
2. Run once more after `#repop`.
**Expected**: Staged count remains 0 after refresh.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [B-06] Bazaar Spawn Sanity Walk

**Goal**: Confirm key Bazaar service NPC groups are physically accessible.
**Steps**:

1. Walk main hub and vendor corridors.
2. Verify class add/remove NPCs and pet vendor are reachable.
**Expected**: No key scripted NPC is stuck behind geometry/out of reach.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [B-07] Bazaar Return Point Validation

**Goal**: Verify operational return/arrival coordinates are usable.
**Steps**:

1. Use return feature/AA or direct zone-in path.
2. Validate arrival around target staging area.
3. Confirm no clipping/wall obstruction.
**Expected**: Arrival point is usable as an operations staging location.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 3) Command Tooling Validation

### [C-01] `#bazaarpull` Usage Modes

**Goal**: Validate `status`, default pull, and bounded count behavior.
**Steps**:

1. Run `#bazaarpull status`.
2. Run `#bazaarpull`.
3. Run `#bazaarpull 5`.
**Expected**: Status reports count, default pulls one, count pulls bounded batch.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [C-02] `#spawnfix` Target Validation

**Goal**: Confirm `#spawnfix` updates only valid targeted NPC spawns.
**Steps**:

1. Target a valid spawned NPC and run `#spawnfix`.
2. Try with invalid/no target to confirm safe failure messaging.
**Expected**: Valid target updates DB; invalid target returns clear error.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [C-03] `#npcspawn` Add/Remove Flow

**Goal**: Confirm create/add/remove spawn tooling works and is reversible.
**Steps**:

1. Use `#npcspawn add` or `#npcspawn clone` on a safe test target.
2. Confirm spawn appears and persists.
3. Remove via `#npcspawn remove`.
**Expected**: Spawn lifecycle operations complete without orphaning critical data.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [C-04] Zone Refresh Commands

**Goal**: Verify `#depop`, `#depopzone`, and `#repop` behavior during ops.
**Steps**:

1. Depop a test target.
2. Repop zone and confirm expected entities return.
**Expected**: Refresh commands are reliable for placement iterations.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [C-05] NPC Appearance Randomizer Workflow

**Goal**: Validate appearance tooling loop for rapid Bazaar NPC polish.
**Steps**:

1. Target a Bazaar NPC and run `#randomize`.
2. Re-run with `#randomize gear` and verify race/gender remain unchanged.
3. Run `#randomfeatures` and confirm facial changes apply.
4. Finalize with `#npcedit save` to persist appearance + spawn in one step.
5. Run `#repop`, and verify look and placement both remain correct.
**Expected**: Randomizer commands provide fast iteration without breaking placement workflow.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 4) Data & Script Operations

### [D-01] SQL Script Application Record

**Goal**: Track what SQL scripts were applied and when.
**Steps**:

1. Execute required scripts for current ops task.
2. Record each script filename in session notes with timestamp.
**Expected**: Session has explicit script execution audit trail.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [D-02] Client Export Dependency Check

**Goal**: Confirm required client exports are regenerated after DB changes.
**Steps**:

1. Run server manager export tasks for changed assets (e.g., `dbstr`, `spells`).
2. Confirm files exist in client folder and have updated timestamps.
**Expected**: Client assets match database state.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [D-03] Post-Ops Restart/Reboot Validation

**Goal**: Verify world/zone reload path after operations work.
**Steps**:

1. Reload or restart required services.
2. Confirm no startup/runtime errors in logs.
3. Run quick smoke: `O-02`, `B-01`, `C-01`.
**Expected**: Services come up cleanly and command workflow remains functional.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [D-04] Lore-Equip Mode + Aug Expansion Migration Record

**Goal**: Ensure this migration set is both applied and documented for reproducible environments.
**Steps**:

1. Execute `utils/sql/custom/2026_02_28_enable_lore_equipped_only.sql`.
2. Execute `utils/sql/custom/2026_02_28_aug_slot_expansion_pass.sql`.
3. Record execution time and DB target in session notes.
4. Trigger/queue rule reload and run a quick in-game sanity check (`MECH-03`, `MECH-04`).
**Expected**: Scripts are applied once per environment, recorded in notes, and behavior is verified in-game.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________
