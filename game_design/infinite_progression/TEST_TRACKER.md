# Infinite Item Progression Test Tracker

**Status**: Active Development
**Tracker Area**: Infinite Item Progression
**Tracker State**: Active
**Last Updated**: 2026-03-06
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
- `#powerslot`: Show Power Slot item, tier, XP, progress bar
- `#powerslot reset`: Zero out XP on current Power Slot item
- `#powerslot setxp <N>`: Set exact XP value on Power Slot item (for testing)
- `#salvage`: (GM-only) Manually trigger Salvage Satchel processing *(Step 4 — Implemented)*
- `#essence`: (GM-only) Show/modify Essence balance *(Step 4 — Implemented)*
  - Note: Players view Essence via **Alternate Currency tab**, salvage via **Combine button**
- **Consume Item AA** (ability 32100): Put matching item on cursor, activate AA → item consumed, XP added *(Step 5 — Implemented)*
- **Consume Essence AA** (ability 32101): Activate AA → Essence auto-consumed, XP added *(Step 5 — Implemented)*
- **Ghost Copy**: Automatic — Power Source progression item is cloned into native equipment slot when empty *(Step 6 — Implemented)*
  - `#powerslot` shows ghost status (ACTIVE → slot name, or INACTIVE)
  - Pick up from either slot removes both and places original on cursor
- `#override set <item_id> <tier> <stat> <value>`: Set a stat override *(Deferred — Backlog)*
- `#override remove <item_id> <tier> <stat>`: Remove a stat override *(Deferred — Backlog)*
- `#override list [item_id]`: List overrides *(Deferred — Backlog)*
- `#regenerate items`: Recalculate all tier-scaled items from current rules *(Deferred — Backlog)*

---

## Run Packs

### Smoke Run (Step 1-3 Core)

Run these first: `IL-01, IL-02, IL-03, TS-01, TS-02, PX-01, PX-03, PX-04`

### Economy Smoke (Steps 4-5)

Run after core: `ES-01, ES-02, ES-03, CI-01, CI-04, CE-01, CE-03`

### Ghost Copy Smoke (Step 6)

Run after economy: `GC-01, GC-04, GC-06, GC-07, GC-13, GC-14`

### Drop Tier Smoke (Step 7)

Quick validation: `DT-02, DT-03` (DT-01 is a soak test — run separately)

### Augment Smoke (Steps 8-9)

Vendors + merge: `AV-01, AV-02, AV-05, AV-08, AM-01, AM-02, AM-07, AM-10`

### Zone Augment Smoke (Step 10)

Zone drops + quests: `ZD-02, ZD-05, ZD-08, ZD-09` (ZD-01 is a soak test — run separately)

### Infusion Smoke (Step 11)

Infusion + transmute: `IF-01, IF-02, IF-04, TR-01, TR-02`

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

**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [IL-02] Basic iLevel Calculation — Armor

**Goal**: Verify armor formula produces correct values.
**Steps**:

1. `#ilevel <cloth_cap_id>` (Cloth Cap)
2. `#ilevel <cobalt_bp_id>` (Cobalt Breastplate)

**Expected**:
- Cloth Cap: iLevel ≈ 30
- Cobalt Breastplate: iLevel ≈ 800+ (AC 45, HP 50, stats)

**Status**: [ ] Pass  [ ] Fail
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

**Expected**: Item swapped to "Hategiver (Enchanted)" (ID 278854). Stats: DMG 30, AC 50, HP 170.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TS-02] Legendary Tier Scaling

**Goal**: Verify Legendary applies ×2.6 combat, ×2 attrs, heroics appear.
**Steps**:

1. `#itemtier 13 2` (Legendary)
2. Inspect item stats in inventory window

**Expected**: "Hategiver (Legendary)" (ID 528854). DMG ≈39, AC ≈65, HP ≈221, heroics visible.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TS-03] Mythic Tier (No Stat Increase, +1 Aug Slot)

**Goal**: Verify Mythic adds aug slot but no stat increase over Legendary.
**Steps**:

1. `#itemtier 13 3` (Mythic)
2. Inspect stats and aug slots in item window

**Expected**: "Hategiver (Mythic)" (ID 778854). Same stats as Legendary. +1 aug slot.
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

**Goal**: Verify `#powerslot` shows correct item info, XP, and progress bar.
**Steps**:

1. Place a Base weapon in Power Slot
2. `#powerslot setxp 500` — set XP to 500
3. `#powerslot` — view info display

**Expected**: Shows item name, tier (Base/0), XP 500/1000, 50% progress, bar `[||||||||||----------]`.
Also shows base XP/kill rate, named mult, raid mult.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [PX-05] XP Persistence Across Swap

**Goal**: Verify swapping items preserves each item's XP independently.
**Steps**:

1. Place Item A in Power Slot, `#powerslot setxp 500`
2. Swap to Item B in Power Slot, `#powerslot setxp 200`
3. Swap back to Item A
4. `#powerslot` — check XP

**Expected**: Item A still has 500 XP. Item B had 200 XP.
`Observed:` Data bucket key is `power_xp_{base_item_id}` — per-item, not per-slot.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [PX-06] Named NPC Multiplier

**Goal**: Verify rare_spawn NPCs give ×3 XP (default NamedXPMult).
**Steps**:

1. Find or spawn a rare_spawn NPC at white-con level
2. Kill it with a Base item in Power Slot
3. Check message for XP gain

**Expected**: `+120 item XP` (40 × 3.0). If also raid-tier (level 55+), `+600` (40 × 3.0 × 5.0).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [PX-07] Raid-Tier NPC Multiplier

**Goal**: Verify high-level NPCs (≥55) give ×5 XP (default RaidTierXPMult).
**Steps**:

1. Spawn or find a level 55+ non-named NPC
2. Kill it with a Base item in Power Slot
3. Check message for XP gain

**Expected**: `+200 item XP` (40 × 5.0).
`Commands:` `#npcedit level 55` on a target NPC, then kill it.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [PX-08] Milestone Messages

**Goal**: Verify milestone messages fire at 25%, 50%, 75%, and 90%.
**Steps**:

1. Place Base item in Power Slot, `#powerslot reset`
2. `#powerslot setxp 240` (24% of 1,000)
3. Kill a white-con mob (+40 = 280 = 28%) — should trigger 25% milestone
4. `#powerslot setxp 490`
5. Kill white-con (+40 = 530 = 53%) — should trigger 50% milestone

**Expected**: Yellow messages: "** Your Power Source item is 25% of the way to Enchanted! **" and similar for 50%.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [PX-09] Admin Reset and SetXP Commands

**Goal**: Verify `#powerslot reset` and `#powerslot setxp N` work correctly.
**Steps**:

1. Earn some XP on a Power Slot item
2. `#powerslot reset` — verify XP reset to 0
3. `#powerslot setxp 999` — verify XP set to 999
4. `#powerslot` — confirm display shows 999

**Expected**: Reset zeroes XP, setxp sets exact value, both persist.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [PX-10] Max Tier (Mythic) No XP

**Goal**: Verify items at Mythic tier earn no further XP.
**Steps**:

1. Place a Mythic-tier item in Power Slot
2. Kill a white-con mob
3. Check for XP message

**Expected**: No item XP message displayed. `#powerslot` shows "Maximum tier reached!".
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [PX-11] Group/Raid XP Distribution

**Goal**: Verify all group/raid members with Power Slot items get item XP.
**Steps**:

1. Form a group (or use a bot with Power Slot item)
2. Each member has a Base item in Power Slot
3. Kill a white-con mob

**Expected**: All group members receive `+40 item XP` message independently.
Each member's XP tracked per their own data bucket.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Step 4: Essence Currency + Salvage System

### [ES-01] Salvage Magic Item

**Goal**: Verify salvaging a magic item yields correct Essence via Combine button.
**Steps**:

1. Place Hategiver (magic, iLevel 269) in Salvage Satchel
2. Click **Combine** button on the Satchel

**Expected**: +169 Common Essence (269 - 100 = 169).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [ES-02] Reject Non-Magic Item

**Goal**: Verify non-magic items are rejected by salvage.
**Steps**:

1. Place Rusty Long Sword (non-magic) in Salvage Satchel
2. Click **Combine** button on the Satchel

**Expected**: Rejected with "not magic" message.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [ES-03] Essence Balance Display

**Goal**: Verify Alternate Currency tab shows Essence balance.
**Steps**:

1. Salvage a few items via Combine button
2. Open **Alternate Currency tab** in Character Sheet
3. (Optional) Use GM `#essence` command to verify

**Expected**: Alt Currency tab shows Common + Rare Essence balance with correct icons.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [ES-04] Tier Bonus on Salvage

**Goal**: Verify Enchanted items yield 15% more Essence.
**Steps**:

1. Salvage Base Hategiver (Combine button) — note yield
2. Create Enchanted Hategiver, salvage (Combine button) — note yield

**Expected**: Enchanted yields ≈ 194 (169 × 1.15).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Step 5: Consume Item / Consume Essence AAs

### [CI-01] Consume Matching Same-Tier Item

**Goal**: Verify Consume Item AA works with a same-tier duplicate on cursor.
**Steps**:

1. Place Base Hategiver in Power Slot
2. Summon another Base Hategiver, put on cursor
3. Activate Consume Item AA (ability 32100)

**Expected**: Cursor item destroyed. +33% of threshold XP (default `ConsumeItemSameTierPct`).
Message: "Consumed Hategiver (same tier) → +N item XP".
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [CI-02] Consume Higher-Tier Item

**Goal**: Verify higher-tier item grants 100% XP.
**Steps**:

1. Place Base Hategiver in Power Slot
2. Summon Enchanted Hategiver (ID 278854), put on cursor
3. Activate Consume Item AA

**Expected**: +100% of threshold XP (default `ConsumeItemHigherTierPct`).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [CI-03] Consume Lower-Tier Item

**Goal**: Verify lower-tier item grants reduced XP.
**Steps**:

1. `#itemtier <slot> 1` to make Power Slot item Enchanted
2. Put a Base-tier copy on cursor
3. Activate Consume Item AA

**Expected**: +7% of threshold XP (default `ConsumeItemLowerTierPct`).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [CI-04] Reject Non-Matching Item

**Goal**: Verify Consume Item AA rejects items that don't match Power Slot base ID.
**Steps**:

1. Place Hategiver in Power Slot
2. Put a different weapon on cursor
3. Activate Consume Item AA

**Expected**: Rejected: "Only duplicates of your Power Source item can be consumed."
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [CI-05] Reject No Cursor Item

**Goal**: Verify Consume Item AA rejects when cursor is empty.
**Steps**:

1. Ensure cursor is empty
2. Activate Consume Item AA

**Expected**: Rejected: "Place an item on your cursor to consume."
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [CI-06] Reject Max Tier Item

**Goal**: Verify Consume Item AA rejects when Power Slot is Mythic.
**Steps**:

1. Place a Mythic item in Power Slot
2. Put a matching item on cursor
3. Activate Consume Item AA

**Expected**: Rejected: "already at maximum tier."
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [CE-01] Consume Essence Full Fill

**Goal**: Verify Consume Essence AA fills remaining XP from Essence balance.
**Steps**:

1. Place Base item in Power Slot, `#powerslot setxp 500` (threshold 1000)
2. Ensure ≥ 500 Common Essence in balance
3. Activate Consume Essence AA (ability 32101)

**Expected**: 500 Essence deducted (at 1:1 ratio), +500 XP, item tiers up to Enchanted.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [CE-02] Consume Essence Partial Balance

**Goal**: Verify Consume Essence with insufficient balance consumes all available.
**Steps**:

1. Place Base item in Power Slot, `#powerslot setxp 0` (need 1000)
2. Set Essence balance to 300
3. Activate Consume Essence AA

**Expected**: 300 Essence deducted, +300 XP. Item still Base tier, 300/1000 XP.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [CE-03] Consume Essence Zero Balance

**Goal**: Verify Consume Essence rejects when player has no Essence.
**Steps**:

1. Set Essence balance to 0
2. Activate Consume Essence AA

**Expected**: Rejected: "You have no Common Essence to consume."
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [CE-04] Consume Essence + Kill XP Integration

**Goal**: Verify Consume Essence and Kill XP stack to trigger tier-up.
**Steps**:

1. Place Base item in Power Slot, `#powerslot setxp 800` (threshold 1000)
2. Ensure ≥ 200 Common Essence
3. Kill a mob for some XP (e.g., +40)
4. `#powerslot` — verify 840 XP
5. Activate Consume Essence AA

**Expected**: 160 Essence consumed (only what's needed), +160 XP → 1000/1000, tier-up to Enchanted.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [CI-07] Powerslot Dual-Path Display

**Goal**: Verify `#powerslot` shows consume rates and Essence cost.
**Steps**:

1. Place Base item in Power Slot with partial XP
2. `#powerslot`

**Expected**: Shows progress bar + "Essence to next tier: N Common" + "Consume Item: 100%/33%/7% XP (higher/same/lower)".
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Step 6: Ghost Copy (Power Source → Equipment)

### [GC-01] Ghost Copy Appears (1H Weapon)

**Goal**: Verify ghost copy is placed when 1H weapon is in PS and Primary is empty.
**Steps**:

1. Ensure Primary hand is empty
2. Equip a 1H weapon in Power Source slot
3. Check Primary slot — should show the same weapon

**Expected**: Client sees weapon in BOTH Power Source and Primary. Message: "Your Power Source item appears in your Primary slot." Weapon visually appears in character's hand.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [GC-02] Ghost Removed When Primary Occupied

**Goal**: Verify ghost is removed when a real item is equipped in the native slot.
**Steps**:

1. With ghost active in Primary (weapon from PS), equip a different weapon directly to Primary
2. Check Primary and PS slots

**Expected**: Ghost removed from Primary, real weapon now in Primary. PS weapon stays in PS (no stats from PS).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [GC-03] Ghost Re-Created When Primary Cleared

**Goal**: Verify removing Primary weapon re-creates ghost from PS item.
**Steps**:

1. With real weapon in Primary and progression item in PS (no ghost)
2. Remove Primary weapon to cursor
3. Check Primary slot

**Expected**: Ghost copy of PS item reappears in Primary. Message about ghost placement.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [GC-04] Pick Up From Ghost Slot

**Goal**: Verify clicking the ghost copy removes both and places original on cursor.
**Steps**:

1. With ghost in Primary, click to pick up from Primary
2. Check cursor, PS slot, and Primary slot

**Expected**: Primary empty. PS empty. Original PS item is on cursor. Single pickup action.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [GC-05] Pick Up From PS While Ghost Active

**Goal**: Verify picking up PS item also removes ghost.
**Steps**:

1. With ghost active, click to pick up from Power Source slot
2. Check cursor, PS slot, and Primary slot

**Expected**: PS item on cursor. Primary empty (ghost removed). Both slots clear.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [GC-06] 2H Weapon — Both Hands Empty

**Goal**: Verify 2H weapon ghost is placed when both Primary AND Secondary are empty.
**Steps**:

1. Ensure both Primary and Secondary are empty
2. Place 2H weapon in Power Source
3. Check Primary slot and `#powerslot`

**Expected**: Ghost copy in Primary. `#powerslot` shows "Ghost Copy: ACTIVE → Primary slot". 2H weapon visible in character model.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [GC-07] 2H Weapon — Shield Blocks Ghost

**Goal**: Verify 2H weapon ghost does NOT appear when Secondary has a shield.
**Steps**:

1. Place 2H weapon in Power Source
2. Equip a shield in Secondary (Primary empty)
3. Check `#powerslot`

**Expected**: Ghost Copy: INACTIVE. No ghost in Primary.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [GC-08] Armor Ghost Copy

**Goal**: Verify non-weapon items (armor) ghost correctly.
**Steps**:

1. Place a chest piece in Power Source, leave Chest slot empty
2. Check Chest slot and character model

**Expected**: Ghost copy in Chest slot. Armor appears on character model. `#powerslot` shows "ACTIVE → Chest slot".
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [GC-09] Multi-Slot Item (Ring)

**Goal**: Verify multi-slot items find the first empty eligible slot.
**Steps**:

1. Place a ring in Power Source with both Finger slots occupied
2. `#powerslot` — verify INACTIVE
3. Remove one ring from Finger1
4. Check `#powerslot`

**Expected**: After removing ring, ghost appears in Finger1. "Ghost Copy: ACTIVE → Finger1 slot".
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [GC-10] Equip Over Ghost Slot

**Goal**: Verify equipping a different item over the ghost slot works cleanly.
**Steps**:

1. With ghost in Primary (from PS), put a different weapon on cursor
2. Click Primary to equip cursor item

**Expected**: Ghost removed. New weapon equipped in Primary normally. PS item stays in PS.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [GC-11] Tier Up Refreshes Ghost

**Goal**: Verify ghost refreshes when PS item tiers up (item ID changes).
**Steps**:

1. Place Base tier weapon in PS (ghost in Primary)
2. Use `#powerslot setxp` to set XP to threshold - 1
3. Kill a mob to trigger tier-up
4. Check Primary ghost — should be Enchanted tier now

**Expected**: Ghost refreshes to Enchanted version. New item stats visible.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [GC-12] Zone With Ghost Active

**Goal**: Verify ghost regenerates after zoning.
**Steps**:

1. With ghost active, zone to another zone
2. Check Primary slot after zone-in

**Expected**: Ghost copy reappears in Primary (regenerated by CalcBonuses on zone-in).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [GC-13] Rule Toggle

**Goal**: Verify `StatProjectionEnabled = false` disables ghost copy system.
**Steps**:

1. Set `StatProjectionEnabled = false` via `#rules set`
2. `#rules reload`
3. Place weapon in PS with Primary empty

**Expected**: No ghost copy placed. `#powerslot` shows INACTIVE.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [GC-14] Stats From Ghost, Not PS

**Goal**: Verify stats come from the ghost copy slot, not from the Power Source slot.
**Steps**:

1. Note stats with nothing in PS and Primary
2. Place weapon in PS (ghost appears in Primary)
3. Check stats — should gain weapon's HP/AC/STR etc.
4. Also verify no double-counting (stats aren't 2x)

**Expected**: Stats match what the weapon would give if equipped normally in Primary. Not double.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Step 7: Drop Tier Chances

### [DT-01] Tier Distribution on Drops [Soak Test]

**Goal**: Verify drops follow 80/15/4/1 distribution.
**Classification**: Soak test — requires 100+ kills and statistical analysis. Not suitable for quick smoke runs.
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

### [DT-03] Rule Toggle — DropTierEnabled Off

**Goal**: Verify `DropTierEnabled = false` causes all drops to be Base tier.
**Steps**:

1. `#rules set ItemProgression:DropTierEnabled false`
2. `#rules reload`
3. Kill 30+ mobs, loot all drops
4. Inspect item tiers

**Expected**: Every dropped item is Base tier (no Enchanted/Legendary/Mythic).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [DT-04] Tier Announcement on Rare Drop

**Goal**: Verify Legendary/Mythic drops trigger an announcement to the killer and their group/raid.
**Steps**:

1. Ensure `DropTierEnabled = true`
2. Kill mobs until a Legendary or Mythic drops (or use GM tools to force one)
3. Watch killer, group, and raid chat

**Expected**: Killer sees a color-coded message: "[PlayerName] has found a [Legendary/Mythic] [ItemName]!" Group/raid members also see it. Other players in the zone do NOT see the announcement.
**Status**: [ ] Pass  [ ] Fail
**Notes**: Announcement scope: killer + group + raid members (not zone-wide).

### [DT-05] Quest Reward Tier Promotion

**Goal**: Verify quest rewards are promoted to the tier set by `QuestItemDefaultTier` rule.
**Steps**:

1. Set `QuestItemDefaultTier` to a tier value (e.g. 1 = Enchanted)
2. Complete a quest that gives an item reward
3. Check the tier of the rewarded item

**Expected**: Quest reward is always promoted to the configured tier (explicit assignment, not RNG). With `QuestItemDefaultTier = 1`, all quest rewards become Enchanted. Auto-tier only applies in quest/task contexts — GM `#summonitem` bypasses it.
**Status**: [ ] Pass  [ ] Fail
**Notes**: Uses `quest_manager.QuestsRunning()` context check to limit scope.

### [DT-06] NPC With Empty Loot Table

**Goal**: Verify mobs with no loot table don't crash the tier roll system.
**Steps**:

1. Find or spawn an NPC with no loot table assigned
2. Kill it

**Expected**: No crash, no loot. Tier roll is simply skipped.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Step 8: Essence Vendors + Basic Augments

### [AV-01] Weaponsmith Vendor — Leveling Weapon Augs (Platinum)

**Goal**: Verify Augment Weaponsmith sells leveling weapon proc augs for platinum.
**Steps**:

1. Go to Bazaar, find "Augment_Weaponsmith" NPC (ID 181200)
2. Right-click to open merchant window
3. Verify 27 items visible (9 level tiers × 3 types: Combat/Lifetap/Mana Stone)
4. Buy a "Combat Stone I" (Level 1, cheapest)
5. Check inventory — item should appear

**Expected**: Merchant opens, items priced in platinum, purchase deducts plat and grants item.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [AV-02] Essence Provisioner — Stat Augs (Common Essence)

**Goal**: Verify Essence Provisioner sells stat augs for Common Essence.
**Steps**:

1. Give yourself Common Essence: `#altcurrency add 100 5000`
2. Find "Essence_Provisioner" NPC (ID 181201) in Bazaar
3. Open merchant — should show alternate currency purchase mode
4. Verify 7 stat augs visible (Stone of Might, Fortitude, Precision, Agility, Insight, Sagacity, Presence)
5. Buy "Stone of Might" (1000 CE)
6. Check inventory and Essence balance

**Expected**: Alt currency merchant works, 1000 CE deducted, Stone of Might in inventory.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [AV-03] Essence Artificer — Proc Augs (Rare Essence)

**Goal**: Verify Essence Artificer sells proc augs for Rare Essence.
**Steps**:

1. Give yourself Rare Essence: `#altcurrency add 101 5000`
2. Find "Essence_Artificer" NPC (ID 181202) in Bazaar
3. Open merchant — should show alternate currency purchase mode
4. Verify 5 proc augs visible (Shard of Flame, Frost, Venom, Mending, Siphoning)
5. Buy "Shard of Flame" (500 RE)
6. Check inventory and Rare Essence balance

**Expected**: Alt currency merchant works, 500 RE deducted, Shard of Flame in inventory.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [AV-04] Weapon Proc Aug — Socket and Fire

**Goal**: Verify a weapon proc aug can be socketed into a weapon and the proc fires in combat.
**Steps**:

1. Summon a weapon with an aug slot (tier ≥ Enchanted)
2. Buy a "Combat Stone V" (Level 50 tier) from the Weaponsmith
3. Socket the aug into the weapon via inventory
4. Equip the weapon and engage a mob in melee combat
5. Watch combat log for proc messages (DD damage)

**Expected**: Aug sockets successfully (`augtype=8` fits weapon aug slot). Proc fires periodically during combat, dealing damage matching the spell effect (spell ID 65100-series, ~100 damage at lv50 tier).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [AV-05] Stat Aug — Socket and Verify Stats

**Goal**: Verify a stat aug sockets into equipment and applies heroic stats.
**Steps**:

1. Summon an armor piece with an aug slot (tier ≥ Enchanted)
2. Buy "Stone of Might" from the Provisioner
3. Socket the aug into the armor piece
4. Inspect the item — hover tooltip should show heroic STR bonus
5. Check character sheet for STR increase

**Expected**: Aug sockets successfully (`augtype=1` fits general aug slot). Tooltip shows +heroic_str. Character STR increases by the aug's value.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [AV-06] Augment Solvent — Remove Aug Safely

**Goal**: Verify Purified Solvent removes a socketed aug without destroying it.
**Steps**:

1. Socket any aug into an item (from AV-04 or AV-05)
2. Buy "Purified Solvent" from the Provisioner (100 CE)
3. Use the solvent on the augmented item
4. Check inventory — aug should be returned, item intact

**Expected**: Aug removed safely, returned to inventory. Item retains its original stats minus the aug bonus. Solvent consumed.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [AV-07] Leveling Tier Gating — Level Requirement

**Goal**: Verify leveling augs enforce their minimum level requirement.
**Steps**:

1. Create or use a low-level character (e.g., level 5)
2. Buy "Combat Stone VII" (Level 60 requirement) from Weaponsmith
3. Attempt to socket it into a weapon
4. Level character to 60 and try again

**Expected**: Cannot socket (or equip/use) the aug at level 5 due to `reqlevel=60`. Can socket at level 60+.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [AV-08] Proc Scaling — Non-Linear Damage Curve

**Goal**: Verify weapon proc damage scales non-linearly across level tiers.
**Steps**:

1. Buy Combat Stones at tiers V (lv50), VII (lv60), VIII (lv65), IX (lv70)
2. Inspect each item's proc effect (via `#showitem` or DB query)
3. Verify damage values match expected scaling

**Expected**: Lv50 ≈ 100 damage, Lv60 ≈ 250, Lv65 ≈ 500, Lv70 ≈ 1000. Scaling is non-linear, ramping steeply at endgame tiers.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [AV-09] Vendor NPC Spawns — Bazaar Locations

**Goal**: Verify all 4 vendor NPCs spawn in the Bazaar zone.
**Steps**:

1. Enter Bazaar zone
2. Use `/target Augment_Weaponsmith` — should find the NPC
3. Use `/target Essence_Provisioner` — should find the NPC
4. Use `/target Essence_Artificer` — should find the NPC
5. Use `/target Augment_Forgemaster` — should find the NPC
6. Verify each is targetable and has merchant interaction

**Expected**: All 4 NPCs present at their spawn points, visible, and interactable as merchants.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Steps 9-12: Merge, Zone Augs, Infusion & Polish Tests

### Step 9: Augment Merge System

### [AM-01] Basic L1→L2 Merge — Stat Aug

**Goal**: Verify 3× L1 stat aug + Lesser Catalyst = 1× L2 stat aug.
**Steps**:

1. Buy 3× "Stone of Might" (L1) from Essence Provisioner
2. Buy 1× "Lesser Merge Catalyst" from Augment Forgemaster
3. Buy 1× "Augment Forge" container from Augment Forgemaster
4. Place all 4 items in the Augment Forge
5. Click Combine

**Expected**: 3 augs + catalyst consumed; 1× "Stone of Might II" appears on cursor with +6 Heroic STR and +10 Attack.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [AM-02] Full Merge Chain — L1 Through L5

**Goal**: Verify the complete merge chain works for a stat augment.
**Steps**:

1. Acquire 81× L1 "Stone of Might" (3^4 = 81 needed for one L5)
2. Merge 27 groups of 3 → 27× L2 (uses 27 Lesser Catalysts)
3. Merge 9 groups of 3 → 9× L3 (uses 9 Standard Catalysts)
4. Merge 3 groups of 3 → 3× L4 (uses 3 Greater Catalysts)
5. Merge 1 group of 3 → 1× L5 (uses 1 Superior Catalyst)
6. Inspect final L5 augment

**Expected**: Final "Stone of Might V" has +21 Heroic STR and +30 Attack.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [AM-03] Mismatched Augs Rejected

**Goal**: Verify merge rejects mixing different aug types.
**Steps**:

1. Place 2× "Stone of Might" + 1× "Stone of Agility" + 1 catalyst in forge
2. Click Combine

**Expected**: Error message "Augments must be the same type and level to merge." — items remain.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [AM-04] Mixed Levels Rejected

**Goal**: Verify merge rejects augs of different levels.
**Steps**:

1. Place 2× L1 "Stone of Might" + 1× L2 "Stone of Might II" + catalyst in forge
2. Click Combine

**Expected**: Error message about same type and level — items remain.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [AM-05] No Catalyst Rejected

**Goal**: Verify merge requires a catalyst.
**Steps**:

1. Place 3× L1 "Stone of Might" in forge (no catalyst, 4th slot empty)
2. Click Combine

**Expected**: Error message about needing exactly 3 augs and 1 catalyst.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [AM-06] Wrong Catalyst Tier Rejected

**Goal**: Verify wrong catalyst tier is rejected.
**Steps**:

1. Place 3× L2 stat augs + 1× Lesser Catalyst (tier 1, intended for L1→2)
2. Click Combine

**Expected**: Error message about needing the correct catalyst for L2→L3 merge.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [AM-07] Max Level Merge Rejected

**Goal**: Verify L5 augs cannot be merged further.
**Steps**:

1. Place 3× L5 stat augs + 1× Superior Catalyst in forge
2. Click Combine

**Expected**: Error message "These augments are already at maximum level (5)."
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [AM-08] Proc Aug Merge — Damage Scaling

**Goal**: Verify proc aug merge scales spell damage correctly.
**Steps**:

1. Merge 3× L1 "Flame Strike Stone" + Lesser Catalyst → L2
2. Socket L2 aug in a weapon, attack a mob, observe proc damage
3. Compare to L1 proc damage

**Expected**: L2 proc damage ≈ 1.75× L1 damage (per AUGMENT_SYSTEM.md §6.4).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [AM-09] Leveling Aug Merge — Procrate Bonus

**Goal**: Verify leveling aug merge increases procrate without changing damage.
**Steps**:

1. Merge 3× L1 "Chipped Combat Stone" + Lesser Catalyst → L2
2. Compare L1 and L2 item stats: procrate should increase by +50
3. Verify proc spell is the same ID (damage unchanged)

**Expected**: L2 "Chipped Combat Stone II" has +50 procrate vs L1, same combat proc spell.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [AM-10] Forgemaster Vendor — Catalyst Prices

**Goal**: Verify Forgemaster sells catalysts at correct CE prices.
**Steps**:

1. Target Augment_Forgemaster in Bazaar, open merchant window
2. Verify Lesser Merge Catalyst: 250 CE
3. Verify Merge Catalyst: 750 CE
4. Verify Augment Forge container available

**Expected**: All items present at correct prices. Container is ~1 CE.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [AM-11] Artificer Vendor — RE Catalysts + L2 Stat Augs

**Goal**: Verify Artificer sells RE catalysts and L2 stat augs.
**Steps**:

1. Target Essence_Artificer in Bazaar, open merchant window
2. Verify Greater Merge Catalyst: 50 RE
3. Verify Superior Merge Catalyst: 125 RE
4. Verify L2 stat augs are available at expected prices

**Expected**: High-tier catalysts and L2 skip-merge stat augs present.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [AM-12] Weaponsmith — PP Catalysts

**Goal**: Verify Weaponsmith also sells low-tier catalysts for platinum.
**Steps**:

1. Target Augment_Weaponsmith in Bazaar, open merchant window
2. Verify Lesser Merge Catalyst and Merge Catalyst are available for PP

**Expected**: Catalysts purchasable with platinum as an alternative to CE.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### Step 10: Zone Drop Augs + Named Mob Augs + Daily/Weekly Quests

*Boss augs deferred. Scope: Classic through Velious (expansion ≤ 2).*

### [ZD-01] Zone Flavor Aug — Blackburrow Drop [Soak Test]

**Goal**: Verify zone flavor augs drop from mobs in the target zone.
**Classification**: Soak test — requires hundreds of kills to confirm low drop rate (~0.3%). Not suitable for quick smoke runs.
**Steps**:

1. Enter Blackburrow (or use `#grantaa` / test with adjusted drop rate)
2. Kill numerous gnolls
3. Check for "Gnoll Fang Chip" augment in loot

**Expected**: Gnoll Fang Chip (item 201000) drops at ~0.3% rate. Type 54 (aug), +2 heroic STR, +5 HP.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [ZD-02] Zone Aug — Verified in DB

**Goal**: Confirm all 59 zone flavor augs exist in items table with correct stats.
**Steps**:

1. `SELECT id, Name, heroic_str, heroic_sta, heroic_int, heroic_wis, heroic_dex, heroic_agi, hp, ac, attack FROM items WHERE id BETWEEN 201000 AND 201058;`
2. Verify 59 rows returned, each with distinct name and non-zero stat values

**Expected**: 59 rows, all itemtype=54, each with zone-themed stats.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [ZD-03] Global Loot — Zone Targeting

**Goal**: Verify global_loot entries correctly target specific zones.
**Steps**:

1. `SELECT id, description, zone FROM global_loot WHERE id BETWEEN 100 AND 158;`
2. Confirm each entry has a zone value matching its description

**Expected**: 59 entries, each targeting one specific zone (e.g., 'blackburrow', 'befallen', etc.)
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [ZD-04] Named Mob Aug — Rare Flag

**Goal**: Verify named mob augs only drop from rare/named mobs.
**Steps**:

1. `SELECT id, description, rare, min_level, max_level FROM global_loot WHERE id BETWEEN 170 AND 172;`
2. Confirm all 3 entries have rare=1
3. Kill a named mob in level-appropriate zone, check for multi-stat aug

**Expected**: 3 global_loot entries with rare=1, targeting level tiers (1-30, 20-50, 40-255).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [ZD-05] Named Augs — Stat Verification

**Goal**: Confirm all 11 named mob augs have correct multi-stat values.
**Steps**:

1. `SELECT id, Name, heroic_str, heroic_sta, heroic_dex, heroic_agi, heroic_int, heroic_wis, heroic_cha, attack, avoidance, manaregen, spelldmg, healamt FROM items WHERE id BETWEEN 201500 AND 201510;`
2. Verify "Shard of Balance" has +2 all heroics
3. Verify "Brilliant Shard of Balance" has +4 all heroics
4. Verify "Champion's Fragment" has +4 STR/STA, +12 ATK, +10 HP

**Expected**: 11 rows with correct multi-stat distributions across archetypes.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [ZD-06] Daily Quest — Task System

**Goal**: Verify daily kill quest is correctly configured.
**Steps**:

1. `SELECT id, title, repeatable, replay_timer_seconds FROM tasks WHERE id = 600000;`
2. Confirm: repeatable=1, replay_timer_seconds=86400
3. `SELECT activitytype, goalcount FROM task_activities WHERE taskid = 600000;`
4. Confirm: activitytype=2 (Kill), goalcount=25
5. Request task in-game, kill 25 creatures, complete

**Expected**: Task "Daily: Essence Hunt" repeatable with 24h cooldown. Rewards 500 CE + 20 RE via Lua hook.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [ZD-07] Weekly Quest — Task System

**Goal**: Verify weekly kill quest is correctly configured.
**Steps**:

1. `SELECT id, title, repeatable, replay_timer_seconds FROM tasks WHERE id = 600001;`
2. Confirm: repeatable=1, replay_timer_seconds=604800
3. `SELECT activitytype, goalcount FROM task_activities WHERE taskid = 600001;`
4. Confirm: activitytype=2 (Kill), goalcount=150

**Expected**: Task "Weekly: Champion's Bounty" repeatable with 7d cooldown. Rewards 2000 CE + 150 RE via Lua hook.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [ZD-08] Lua Reward Script — Currency Award

**Goal**: Verify Lua EVENT_TASK_COMPLETE hook awards correct currencies.
**Steps**:

1. Check `quests/global/player_task_rewards.lua` exists and has handlers for tasks 600000 and 600001
2. Complete daily task in-game, verify message: "You receive 500 Common Essence and 20 Rare Essence!"
3. Check alternate currency balances increased by correct amounts

**Expected**: Both currency types awarded on task completion. Message displayed.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [ZD-09] Velious Aug — Cobalt Scar

**Goal**: Verify a Velious-expansion zone aug exists and works.
**Steps**:

1. Enter Cobalt Scar
2. Kill drakes/ulthorks
3. Check for "Cobalt Drake Scale" aug
4. Or verify via: `SELECT * FROM items WHERE id = 201049;`

**Expected**: Item 201049 "Cobalt Drake Scale" — +3 heroic STA, +5 AC, itemtype=54.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

### [ZD-10] No Augs in Already-Covered Zones

**Goal**: Confirm zones that already had aug drops don't get duplicate global_loot entries.
**Steps**:

1. `SELECT * FROM global_loot WHERE zone IN ('mistmoore','sebilis','chardok','velketor') AND id BETWEEN 100 AND 999;`
2. Confirm 0 rows returned

**Expected**: These zones (which already have aug drops in existing loot tables) have no Step 10 global_loot entries.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Step 11 — Augment Infusion + Transmutation (Container-Based)

### [IF-01] Basic Infusion via Infusion Pool

**Goal**: Placing aug + correct catalyst in Infusion Pool and clicking Combine applies +1 to primary stat.
**Steps**:

1. `#si 200530` to get Infusion Pool, `#si 200300` for Stone of Might I (L1, +3 STR)
2. `#si 200520` for Infusion Catalyst I
3. Place aug in slot 0 of Infusion Pool, catalyst in slot 1
4. Click Combine
5. Confirm: catalyst consumed, aug still in Pool with +1 Heroic STR, "infused to level 1" message

**Expected**: Aug gains +1 heroic STR (3→4), catalyst consumed, `infuse_level` custom_data = 1.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [IF-02] Wrong Catalyst Tier Rejected

**Goal**: Catalyst tier must match the aug's next infusion level.
**Steps**:

1. Get a fresh Stone of Might I (0 infusions) and an Infusion Catalyst III
2. Place both in Infusion Pool, click Combine
3. Confirm error: "Wrong catalyst tier. This augment needs Infusion Catalyst I"

**Expected**: Combine refused, both items remain.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [IF-03] Infuse Cap Enforcement

**Goal**: Cannot infuse beyond L5's maximum primary stat.
**Steps**:

1. `#si 200304` for Stone of Might V (L5, +21 STR)
2. Place in Infusion Pool with Infusion Catalyst I, click Combine
3. Confirm error: "At maximum power" (base 21 = cap 21)
4. `#si 200303` for Stone of Might IV (L4, +15 STR)
5. Infuse 5 times (Catalysts I through V) — 5th should succeed (15+5=20 ≤ 21)
6. Try 6th infuse — should refuse (max 5 infusions reached)

**Expected**: Infusion blocked at stat cap or 5-infusion hard limit.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [IF-04] Non-Aug in Infusion Pool Rejected

**Goal**: Only augments can be infused.
**Steps**:

1. Place a regular weapon in Infusion Pool with an Infusion Catalyst
2. Click Combine
3. Confirm error about item not usable in Pool

**Expected**: Combine refused with clear message.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [IF-05] Infusion Pool Vendor Purchase

**Goal**: Infusion Pool and all 5 catalysts available from Augment Forgemaster.
**Steps**:

1. Visit the Augment Forgemaster NPC
2. Confirm Infusion Pool shows for 1 CE
3. Confirm Infusion Catalyst I–V shows for 250/500/1000/2000/4000 CE

**Expected**: All items purchasable with Common Essence.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TR-01] Transmute Aug via Salvage Satchel

**Goal**: Augs in the Salvage Satchel are transmuted using the level-based return table.
**Steps**:

1. `#si 200302` for Stone of Might III (L3)
2. Place in Salvage Satchel, click Combine
3. Confirm: aug destroyed, +1000 CE, +25 RE, "[TRANSMUTED]" in breakdown

**Expected**: Aug removed, correct L3 Essence returned.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TR-02] Mixed Salvage + Transmute

**Goal**: Satchel handles both normal items and augs in a single Combine.
**Steps**:

1. Place a magic sword (normal item) and an L1 aug in the Salvage Satchel
2. Click Combine
3. Confirm: sword salvaged normally (iLevel-based CE), aug transmuted (125 CE)
4. Summary shows both "Salvaged X items" and "transmuted Y augments"

**Expected**: Both paths execute, totals combined correctly.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TR-03] Transmute Zone Drop Aug (Non-Mergeable)

**Goal**: Zone drop augs transmute as L1 (125 CE, 0 RE).
**Steps**:

1. Obtain a zone-flavor aug (from Step 10 drops)
2. Place in Salvage Satchel, click Combine
3. Confirm: +125 CE, 0 RE

**Expected**: Non-mergeable augs return L1 value.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [TR-04] Transmute L5 Aug

**Goal**: L5 aug returns maximum recovery (6000 CE, 200 RE).
**Steps**:

1. `#si 200304` for Stone of Might V (L5)
2. Place in Salvage Satchel, Combine
3. Confirm: +6000 CE, +200 RE

**Expected**: Full L5 recovery amount.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

*Step 12 tests will be added when that step is implemented.*

---

## Automated Validation Snapshot

*To be filled after first full build and test pass.*

- Build status: __________
- DB migration status: __________
- Python tool parity: __________
- Smoke test results: __________
