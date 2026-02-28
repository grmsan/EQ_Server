# Mechanics Test Tracker

**Status**: Active Testing
**Tracker Area**: Mechanics
**Tracker State**: Active
**Last Updated**: 2026-02-28

## Purpose

Tracks combat-system and proc-engine behavior that is cross-class and not multiclass-specific.

## How To Use

1. Fill out `Session Header`.
2. Run `Smoke Run` first.
3. Mark pass/fail and capture direct combat evidence.

## Session Header

**Date**: __________
**Tester**: __________
**Build/Branch**: __________
**Rules Snapshot**: `MultipleTwoHandedProcs=__`, `UseNewDexFormulas=__`

## Run Packs

### Smoke Run

Run: `MECH-01, MECH-03, MECH-04`

### Full Regression

Run all tests in this document.

---

## 1) Proc System Parity

### [MECH-01] Proc Parity: 2H and Bow

**Goal**: Verify THJ-aligned proc behavior.
**Legacy ID**: `P-05`
**Steps**:

1. Verify `MultipleTwoHandedProcs = true`.
2. Test 2H weapon + proc aug rounds.
3. Test bow/ranged proc rounds.
**Expected**: 2H and bow proc behavior follows configured parity expectations.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [MECH-02] Pet/NPC Weapon Instance Proc Parity

**Goal**: Verify pet/NPC attacks evaluate weapon-instance proc paths (including aug-based procs).
**Legacy ID**: `P-10`
**Setup**:

1. Use a pet or controlled NPC with an equipped weapon instance (not just raw item ID fallback).
2. Ensure weapon/aug proc effects are valid for level and proc rules.
**Steps**:

1. Engage a target and let pet/NPC perform sustained melee rounds.
2. Observe combat output for innate weapon procs and augment procs.
3. Repeat with offhand if applicable.
**Expected**: Proc checks include instance-backed weapon data, and expected weapon/aug procs can fire during pet/NPC attacks.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 2) Lore and Item Socket Mechanics

### [MECH-03] Lore-Equipped-Only Carry Behavior

**Goal**: Verify lore duplicates are allowed in inventory when `Items:LoreEquippedOnly=true`.
**Steps**:

1. Confirm rule with `#rules get Items LoreEquippedOnly` (or DB query).
2. Obtain two copies of the same lore item (example: cast `Summon Orb` twice).
3. Keep both copies in inventory/cursor/bank without equipping both.
**Expected**: No lore pickup rejection; no duplicate-lore destroy behavior while carrying.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [MECH-04] Lore-Equipped-Only Equip Gate

**Goal**: Verify duplicate lore items are blocked only at equip time.
**Steps**:

1. Carry two copies of the same lore item.
2. Equip one copy.
3. Attempt to equip the second copy in another valid slot.
**Expected**: Equip is denied with lore message (`You may only equip one of that lore item.`); inventory item remains intact.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [MECH-05] Global Weapon Aug Slot Expansion Coverage

**Goal**: Verify DB migration coverage for new weapon aug slot policy.
**Setup**:

1. Apply `utils/sql/custom/2026_02_28_aug_slot_expansion_pass.sql`.
**Steps**:

1. Validate 1H weapons have at least one type-4 slot.
2. Validate 2H/bow weapons have at least two type-4 slots where free slots exist.
3. Spot-check in-game by linking/equipping random 1H, 2H, and bow items and inspecting aug slots.
**Expected**: 1H policy and 2H/bow policy are broadly satisfied; exceptions are limited to fully occupied-slot legacy items.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [MECH-06] Stat Gear Additional Socket Coverage

**Goal**: Verify stat-bearing gear gained at least one extra type-7 slot where free slots exist.
**Setup**:

1. Apply `utils/sql/custom/2026_02_28_aug_slot_expansion_pass.sql`.
**Steps**:

1. Spot-check armor, jewelry, shield, and weapon pieces with stats.
2. Confirm at least one type-7 slot exists on sampled items unless all 6 aug slots were already populated.
**Expected**: Most stat-bearing equippable items have at least one type-7 slot after migration.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________
