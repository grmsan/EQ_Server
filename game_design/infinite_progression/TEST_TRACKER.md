# Infinite Item Progression Test Tracker

**Status**: Active Development
**Tracker Area**: Infinite Item Progression
**Tracker State**: Active
**Last Updated**: 2026-03-03
**Implementation Plan**: [IMPLEMENTATION_STEPS.md](IMPLEMENTATION_STEPS.md)

## Purpose

This tracker is the single source of truth for infinite item progression validation.
Each test maps to an implementation step and can be run independently.

Use it in two modes:

1. `Smoke Run` (15-25 min): Core loop sanity after code changes.
2. `Full Regression` (60-120 min): Broad validation before merge/release.

## Return-After-Break Quickstart (10 Minutes)

1. Fill out `Session Header` (date, build/branch, key rules).
2. Verify database has `calculated_ilevel` column populated (if Step 1+ done).
3. Run the earliest passing smoke test to confirm wiring isn't broken.
4. Check [IMPLEMENTATION_STEPS.md](IMPLEMENTATION_STEPS.md) master tracker for current step.
5. Resume from unfinished checklist items in the current step.

## End-To-End Workflow

1. Select scope: `Smoke Run`, `Full Regression`, or one category.
2. Read each test's `Goal`, then execute listed `Steps` exactly.
3. Validate against `Expected` and collect objective evidence.
4. Mark `Status` as `Pass` or `Fail`.
5. Add concise notes with reproduction commands and what actually happened.
6. If failing, include one immediate next action (code path or SQL/data target).

## Pass/Fail Rules

- `Pass`: Expected behavior occurs consistently without manual interpretation.
- `Fail`: Any mismatch, intermittency, missing output, or dependency break.
- `Blocker`: If environment is invalid (missing SQL, stale build), mark `Fail` and note `Blocked by environment`.

## Evidence Format (Use in Notes)

`Observed:` actual behavior in one line.
`Evidence:` exact chat/log snippet or command output.
`Commands:` minimal command sequence to reproduce.
`Next:` likely code/data area if failed.

## Related Trackers

- Multiclass system: [../multiclass/TEST_TRACKER.md](../multiclass/TEST_TRACKER.md)
- Combat mechanics: [../mechanics/TEST_TRACKER.md](../mechanics/TEST_TRACKER.md)

## Global Commands (Progression System)

- `#ilevel [item_id]`: Show iLevel for an item (or equipped Power Slot item)
- `#ilevel all`: Batch-calculate and cache iLevel for all items in DB
- `#itemtier [slot_id tier]`: Show equipped item tiers, or set tier on a slot (0-3)
- `#powerslot info`: Show Power Slot item, tier, XP, progress
- `#salvage`: Process all items in the Salvage Satchel
- `#essence`: Show Common and Rare Essence balance
- `#tierup`: Promote Power Slot item via Essence
- `#tierup cost`: Preview Essence cost for next tier
- `#feed`: Feed cursor item to Power Slot
- `#override set <item_id> <tier> <stat> <value>`: Set a stat override
- `#override remove <item_id> <tier> <stat>`: Remove a stat override
- `#override list [item_id]`: List overrides
- `#regenerate items`: Recalculate all tier-scaled items from current rules

---

## Run Packs

### Smoke Run (Step 1-3 Core)

Run these first: `IL-01, IL-02, IL-03, TS-01, TS-02`

### Economy Smoke (Steps 4-5)

Run after core: `ES-01, ES-02, ES-03, TC-01`

### Full Regression

Run all tests in this document.

---

## Session Header

**Date**: __________
**Tester**: __________
**Build/Branch**: __________
**Server Rules Snapshot**:
`WEAPON_AC_WEIGHT=__`, `ARMOR_AC_WEIGHT=__`, `ESSENCE_OFFSET=__`,
`TIER_FLOOR_BASE_TO_ENCHANTED=__`, `TIER_SCALE_BASE_TO_ENCHANTED=__`

---

## Step 1: iLevel Calculation Engine

### [IL-01] Basic iLevel Calculation — Weapons

**Goal**: Verify `CalculateILevel()` produces correct values for known weapons.
**Prerequisite**: Step 1 build complete.
**Steps**:

1. `#ilevel 5019` (Rusty Long Sword)
2. `#ilevel 5157` (Lamentation)
3. `#ilevel <hategiver_id>` (look up Hategiver ID first)

**Expected**:
- Rusty Long Sword: iLevel ≈ 14 (±2)
- Lamentation: iLevel ≈ 77 (±5)
- Hategiver: iLevel ≈ 269 (±10)

**Status**: [X] Pass  [ ] Fail
**Notes**: ______________________________

### [IL-02] Basic iLevel Calculation — Armor

**Goal**: Verify armor formula produces correct values.
**Steps**:

1. `#ilevel <cloth_cap_id>` (Cloth Cap)
2. `#ilevel <cobalt_bp_id>` (Cobalt Breastplate)

**Expected**:
- Cloth Cap: iLevel ≈ 30
- Cobalt Breastplate: iLevel ≈ 800+ (AC 45, HP 50, stats)

**Status**: [X] Pass  [ ] Fail
**Notes**: ______________________________

### [IL-03] Batch iLevel Calculation

**Goal**: Verify `#ilevel all` populates `calculated_ilevel` for all items.
**Steps**:

1. `#ilevel all`
2. In DB: `SELECT COUNT(*) FROM items WHERE calculated_ilevel > 0`
3. Compare to `SELECT COUNT(*) FROM items WHERE itemtype IN (0,1,2,3,4,5,10,35) OR ac > 0 OR hp > 0`

**Expected**: Row count from (2) should be substantial and match items with meaningful stats.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [IL-04] Rule-Driven Weights

**Goal**: Verify iLevel weights are read from rules, not hardcoded.
**Steps**:

1. `#ilevel <hategiver_id>` — note value
2. Change `WEAPON_AC_WEIGHT` rule to 6.0 (from 3.0)
3. `#rules reload`
4. `#ilevel <hategiver_id>` — note new value

**Expected**: iLevel increases (AC 25 × 6.0 = +75 more power = noticeable change).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [IL-05] Python Tool Parity

**Goal**: Verify C++ matches Python implementation.
**Steps**:

1. Run `python tools/validate_ilevel.py` — note values for 5+ items
2. `#ilevel` each of the same items
3. Compare

**Expected**: Values match within ±1 (floating point rounding).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Step 2: Item Tier Storage + Stat Scaling (DB-Backed Approach)

### [TS-01] Set Tier via Admin Command

**Goal**: Verify `#itemtier` command swaps item to DB-backed tiered version.
**Steps**:

1. Summon a Hategiver: `#si 28854`
2. Equip it, note which slot it went to (e.g. slot 13 = primary)
3. `#itemtier` — verify it shows in the summary as Base tier
4. `#itemtier 13 1` (Enchanted)
5. Inspect item stats in inventory window — verify new name and stats

**Expected**: Item swapped to "Hategiver (Enchanted)" (ID 1028854). Stats: DMG 30, AC 50, HP 170.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TS-02] Legendary Tier Scaling

**Goal**: Verify Legendary applies ×2.6 combat, ×2 attrs, heroics appear.
**Steps**:

1. `#itemtier 13 2` (Legendary)
2. Inspect item stats in inventory window

**Expected**: "Hategiver (Legendary)" (ID 2028854). DMG ≈39, AC ≈65, HP ≈221, heroics visible.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TS-03] Mythic Tier (No Stat Increase, +1 Aug Slot)

**Goal**: Verify Mythic adds aug slot but no stat increase over Legendary.
**Steps**:

1. `#itemtier 13 3` (Mythic)
2. Inspect stats and aug slots in item window

**Expected**: "Hategiver (Mythic)" (ID 3028854). Same stats as Legendary. +1 aug slot.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TS-04] Tier Down (Revert to Base)

**Goal**: Verify `#itemtier <slot> 0` reverts item to base version.
**Steps**:

1. `#itemtier 13 0` (Base)
2. Inspect item

**Expected**: Item reverted to "Hategiver" (ID 28854) with original stats.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TS-05] Augment Preservation on Tier Swap

**Goal**: Verify augments are preserved when changing tiers.
**Steps**:

1. Summon Hategiver and an augment, socket the augment
2. `#itemtier 13 1` (Enchanted)
3. Inspect - verify augment is still socketed
4. `#itemtier 13 2` (Legendary)
5. Inspect - verify augment is still socketed

**Expected**: Augment preserved across all tier changes.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TS-06] DB Verification

**Goal**: Verify tiered items exist in the database with correct stats.
**Steps**:

1. `python tools/generate_tiered_items.py --verify`
2. `python tools/generate_tiered_items.py --item 28854 --dry-run`

**Expected**: 117,958 items per tier. Hategiver Enchanted: DMG 30, AC 50, HP 170.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Step 3: Power Slot XP + Kill-Based Tier-Up

### [PX-01] Basic XP on Kill

**Goal**: Verify killing a mob with an item in Power Slot grants item XP.
**Steps**:

1. Place a Base weapon in Power Slot
2. Kill a white-con mob
3. Check message for XP gain

**Expected**: Message shows `+40 item XP` (or configured BASE_ITEM_XP).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [PX-02] Con Color Multipliers

**Goal**: Verify grey = 0, green = reduced, yellow = bonus.
**Steps**:

1. Kill grey-con mob — note item XP
2. Kill green-con mob — note item XP
3. Kill yellow-con mob — note item XP

**Expected**: Grey = 0, Green ≈ 5, Yellow ≈ 45.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [PX-03] Auto Tier-Up

**Goal**: Verify item auto-tiers when XP threshold reached.
**Steps**:

1. Set BASE_ITEM_XP to 500 for fast testing
2. Kill 2 white-con mobs (should reach 1,000)
3. Observe tier-up

**Expected**: Item becomes Enchanted, message + sound plays, XP resets.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [PX-04] Power Slot Info Command

**Goal**: Verify `#powerslot info` shows correct data.
**Steps**:

1. Place item in Power Slot, earn some XP
2. `#powerslot info`

**Expected**: Shows item name, current tier, XP/threshold, percentage.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [PX-05] XP Persistence Across Swap

**Goal**: Verify swapping items preserves each item's XP.
**Steps**:

1. Earn 500 XP on Item A
2. Swap to Item B — earn 200 XP
3. Swap back to Item A

**Expected**: Item A still has 500 XP.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Step 4: Essence Currency + Salvage System

### [ES-01] Salvage Magic Item

**Goal**: Verify salvaging a magic item yields correct Essence.
**Steps**:

1. Place Hategiver (magic, iLevel 269) in Salvage Satchel
2. `#salvage`

**Expected**: +169 Common Essence (269 - 100 = 169).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [ES-02] Reject Non-Magic Item

**Goal**: Verify non-magic items are rejected by salvage.
**Steps**:

1. Place Rusty Long Sword (non-magic) in Salvage Satchel
2. `#salvage`

**Expected**: Rejected with "not magic" message.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [ES-03] Essence Balance Display

**Goal**: Verify `#essence` and Alternate Currency tab show balance.
**Steps**:

1. Salvage a few items
2. `#essence`
3. Open Alternate Currency tab

**Expected**: Both show matching Common Essence balance.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [ES-04] Tier Bonus on Salvage

**Goal**: Verify Enchanted items yield 15% more Essence.
**Steps**:

1. Salvage Base Hategiver — note yield
2. Create Enchanted Hategiver, salvage — note yield

**Expected**: Enchanted yields ≈ 194 (169 × 1.15).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Step 5: Essence Tier-Up

### [TC-01] Tier-Up Cost Preview

**Goal**: Verify `#tierup cost` shows correct iLevel² scaled cost.
**Steps**:

1. Place Rusty Sword (iLevel 14) in Power Slot at Base tier
2. `#tierup cost`

**Expected**: "Base → Enchanted: 3 Common Essence" (1 + round(196 × 0.008) = 3).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TC-02] Essence Tier-Up Execution

**Goal**: Verify `#tierup` deducts Essence and promotes item.
**Steps**:

1. Ensure > 3 Common Essence
2. `#tierup` with Rusty Sword in Power Slot
3. Check Essence balance and item tier

**Expected**: -3 Essence, item is now Enchanted.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TC-03] Insufficient Essence Rejection

**Goal**: Verify tier-up rejected when Essence is insufficient.
**Steps**:

1. Set Essence balance to 0
2. `#tierup`

**Expected**: "Not enough Essence (need X, have 0)".
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Step 6: Duplicate Feeding + Stat Projection

### [FD-01] Feed Duplicate Item

**Goal**: Verify feeding an exact duplicate grants XP.
**Steps**:

1. Place Hategiver in Power Slot (E→L, threshold 20,000)
2. Pick up another Hategiver on cursor
3. `#feed`

**Expected**: +4,000 XP (20% of 20,000), cursor item destroyed.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [FD-02] Reject Different Item

**Goal**: Verify non-duplicate is rejected.
**Steps**:

1. Place Hategiver in Power Slot
2. Pick up a different weapon on cursor
3. `#feed`

**Expected**: "Only exact duplicates can be fed".
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [SP-01] Stat Projection Active

**Goal**: Verify Power Slot item projects stats when native slot is empty.
**Steps**:

1. Equip weapon in Power Slot, leave Primary hand empty
2. Check stats

**Expected**: Player gains weapon stats (HP, AC, etc.).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [SP-02] Stat Projection Disabled

**Goal**: Verify projection disabled when native slot is occupied.
**Steps**:

1. Equip different weapon in Primary hand
2. Check stats

**Expected**: Power Slot stats no longer apply.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Step 7: Drop Tier Chances

### [DT-01] Tier Distribution on Drops

**Goal**: Verify drops follow 80/15/4/1 distribution.
**Steps**:

1. Kill 100+ mobs, note tier of each drop
2. Tally results

**Expected**: ~80% Base, ~15% Enchanted, ~4% Legendary, ~1% Mythic (within 2σ).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [DT-02] Pre-Tiered Drop Stats

**Goal**: Verify dropped Enchanted+ items arrive with scaled stats.
**Steps**:

1. Loot an Enchanted drop
2. Inspect stats

**Expected**: Stats are ×2 base, 2 aug slots visible.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Steps 8-12: Augment & Polish Tests

*Tests will be added when Steps 8-12 implementation begins.*

---

## Automated Validation Snapshot

*To be filled after first full build and test pass.*

- Build status: __________
- DB migration status: __________
- Python tool parity: __________
- Smoke test results: __________
