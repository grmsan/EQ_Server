# Infinite Item Progression — Implementation Steps

**Version:** 1.0
**Date:** March 3, 2026
**Status:** Plan — Not Yet Started

---

## Guiding Principles

- Each step produces a **testable, playable chunk** of value.
- Steps are ordered by dependency: later steps build on earlier ones.
- Each step lists **what to build**, **what to test**, and **what "done" looks like**.
- UX starts with commands and NPCs (Phase 1); DLL polish comes later (Phase 2).
- All knobs are tunable via rules/DB from the start — no hardcoded constants.

---

## Master Progress Tracker

| Step | Name | Status | Date Started | Date Completed |
| --- | --- | --- | --- | --- |
| 1 | iLevel Calculation Engine | ✅ Complete | 2025-07-15 | 2026-03-03 |
| 2 | Item Tier Storage + Stat Scaling | ✅ Complete | 2026-03-04 | 2026-03-04 |
| 3 | Power Slot XP + Kill-Based Tier-Up | ✅ Complete | 2026-03-04 | 2026-03-04 |
| 4 | Essence Currency + Salvage System | ⬜ Not Started | | |
| 5 | Essence Tier-Up (Pay Path) | ⬜ Not Started | | |
| 6 | Duplicate Feeding + Stat Projection | ⬜ Not Started | | |
| 7 | Drop Tier Chances | ⬜ Not Started | | |
| 8 | Essence Vendors + Basic Augments | ⬜ Not Started | | |
| 9 | Augment Merge System | ⬜ Not Started | | |
| 10 | Zone Drop Augs + Boss Augs | ⬜ Not Started | | |
| 11 | Augment Infusion + Transmutation | ⬜ Not Started | | |
| 12 | DLL Visual Polish | ⬜ Not Started | | |

**Legend:** ⬜ Not Started · 🔨 In Progress · ✅ Complete · ⏸️ Blocked

---

## Prerequisites

Before Step 1, ensure the following exist (most already do):

| Prerequisite | Status | Notes |
|---|---|---|
| Power Source slot accessible | ✅ Exists | `EQ::invslot::slotPowerSource` |
| `custom_data` on ItemInstance | ✅ Exists | Already used in codebase |
| Alternate currency API | ✅ Exists | `AddAlternateCurrencyValue()` etc. |
| `GetUnscaledItem()` / item scaling | ✅ Exists | `item_instance.cpp` |
| Con color calculation | ✅ Exists | `Mob::GetLevelCon()` |
| Data buckets for persistent storage | ✅ Exists | `character_data_buckets` |
| Delete + Limbo + Re-add pattern | ✅ Exists | Used in current item scaling |

---

## Step 1 — iLevel Calculation Engine + Admin Tool

**Goal:** Calculate iLevel for any item in the database and expose it to server code and admin commands.

### What to Build

1. **`CalculateILevel()` function** in `common/` (new file: `item_ilevel.h` / `item_ilevel.cpp`)
   - Weapon formula: `DPS + AC×3 + HP×0.3 + Mana×0.3 + Attrs×2 + Attack + Haste×10 + Heroics×5`
   - Armor formula: `AC×15 + HP×0.5 + Mana×0.5 + Attrs×3 + Resists×0.5 + Heroics×5`
   - Returns `max(1, floor(power_score))`
   - Weights loaded from rules (knobs from ITEM_LEVEL_SYSTEM.md §Tuning Knobs: `WEAPON_AC_WEIGHT`, `ARMOR_AC_WEIGHT`, etc.)

2. **`#item ilevel <item_id>`** admin command — show iLevel for an item
3. **`#item ilevel_all`** — batch-calculate and store `calculated_ilevel` column on items table
4. **DB migration** — `ALTER TABLE items ADD COLUMN calculated_ilevel INT UNSIGNED DEFAULT 0`

### What to Test

| Test | Expected Result | Pass Criteria |
|---|---|---|
| `#item ilevel 5019` (Rusty Long Sword) | iLevel = 14 | Match within ±2 of Python tool |
| `#item ilevel 5157` (Lamentation) | iLevel ≈ 77 | ±5 |
| `#item ilevel` on a Hategiver | iLevel ≈ 269 | ±10 |
| `#item ilevel` on a PoP raid weapon | iLevel ≈ 600–800 | Reasonable range |
| `#item ilevel_all` | Populates `calculated_ilevel` for all items | Row count matches total items |
| Change `WEAPON_AC_WEIGHT` rule → recalc | iLevel changes proportionally | Weight is not hardcoded |

### Done When

- C++ `CalculateILevel()` produces results matching `tools/validate_ilevel.py` output.
- Admin can query any item's iLevel in-game.
- All items have a cached `calculated_ilevel` column in the DB (for future SQL queries).

### Progress Checklist

- [x] Create `common/item_ilevel.h` and `common/item_ilevel.cpp`
- [x] Implement `CalculateILevel()` with weapon + armor formulas
- [x] Wire weights to rules (not hardcoded) — `RULE_CATEGORY(ItemProgression)` with 22 rules
- [x] DB migration: `calculated_ilevel` column on `items` — auto-created by `tools/populate_ilevel.py`
- [x] `#ilevel <item_id>` command (also: `#ilevel` for equipped, `#ilevel all` for batch)
- [x] `#ilevel all` batch command — iterates all items, writes `calculated_ilevel`
- [x] Verify output matches `tools/validate_ilevel.py` — 117,958 items verified, 0 mismatches
- [x] Build passes, no lint errors

### Post-Implementation Notes

> **Implementation Notes (2025-07-15, updated 2026-03-03):**
>
> - Command is `#ilevel` (not `#item ilevel`). Registered at Guide+ access level.
> - Three modes: `#ilevel` (equipped on target), `#ilevel <id>` (single item), `#ilevel all` (batch DB update).
> - New files: `common/item_ilevel.h`, `common/item_ilevel.cpp`, `zone/gm_commands/ilevel.cpp`.
> - All formula weights live in `RULE_CATEGORY(ItemProgression)` — 22 rules total
>   (12 iLevel weights, 2 salvage economy, 2 salvage tier bonuses, 6 tier cost floor/scale).
> - `IsWeaponType()` checks: 1HSlash, 2HSlash, 1HPiercing, 1HBlunt, 2HBlunt, Bow, Martial, 2HPiercing.
> - `CalculateEssenceYield()` and `CalculateTierCost()` also implemented in `item_ilevel.cpp`
>   (needed by Steps 4-5 but share the same file).
> - **Python script `tools/populate_ilevel.py`** is the preferred way to batch-populate iLevel.
>   Auto-creates column + index if missing. Supports `--dry-run`, `--verify`, `--report-only`,
>   `--config <weights.json>` for override testing. 117,958 items calc'd in <0.3s.
> - UNK columns examined — 16 are fully zero in DB, but we added a clean new column instead
>   (UNK columns are part of ItemData struct/packets, repurposing would risk client confusion).
> - The `#ilevel all` in-game command also works but Python script is better for batch ops.

---

## Step 2 — Item Tier Storage + Stat Scaling ✅ Complete (2026-03-04)

**Goal:** Items can be assigned a tier (Base/Enchanted/Legendary/Mythic) and their stats scale accordingly when sent to the client.

### What to Build

1. **Tier stored in `custom_data`** on ItemInstance — key: `"tier"`, values: `0`/`1`/`2`/`3`
2. **`ApplyTierScaling()` function** — given an ItemInstance with a tier, compute scaled stats:
   - Enchanted: all stats ×2, haste +3%
   - Legendary: combat stats ×2.6, attrs ×2, heroics = base 1:1, haste +5%, attack = base_dmg×2, spell/heal power
   - Mythic: same as Legendary (no stat increase)
   - Load multipliers from rules (`ENCHANTED_MULTIPLIER`, `LEGENDARY_COMBAT_BONUS`, etc.)
3. **Override system** — `item_scaling_overrides` table + `#override` commands
   - `CalculateScaledStat()` checks formula first, then overlay if override exists
   - Override resolution: formula value → check DB → use override if found
4. **Packet integration** — hook into `SendItemPacket()` to apply tier scaling before sending
   - Uses existing Delete + Limbo + Re-add pattern when tier changes
5. **`#item tier <item_id> <tier>`** — admin command to set an item's tier (for testing)
6. **Augment slot count** varies by tier: Base=1, Ench=2, Leg=3, Myth=4 (2H: 2/4/5/6)
   - Server validates aug insertion against current tier

### What to Test

| Test | Expected Result | Pass Criteria |
|---|---|---|
| Equip Hategiver (tier 0), check stats | Base stats (15 DMG, 25 AC, 85 HP) | Exact match |
| `#item tier <id> 1` → re-inspect | Enchanted (30 DMG, 50 AC, 170 HP) | ×2 formula |
| `#item tier <id> 2` → re-inspect | Legendary (39 DMG, 65 AC, 221 HP, heroics appear) | ×2.6 combat, heroics = base |
| `#item tier <id> 3` → re-inspect | Mythic (same stats as Leg, +1 aug slot) | No stat change, slot count +1 |
| Set override: `#override set <id> 1 attack 20` | At Enchanted, Attack shows 20 instead of formula | Override replaces formula |
| Remove override → re-inspect | Reverts to formula value | Clean revert |
| Try to socket aug into slot 2 on Base item | Rejected with message | Server validates slot availability |
| `#regenerate items` after changing `LEGENDARY_COMBAT_BONUS` | All items recalculate, overrides preserved | Override count unchanged |

### Done When

- Any item can be set to any tier and the client sees correctly scaled stats.
- Override system works (set, remove, list, filter).
- Augment slots are dynamically limited by tier.
- Changing a tuning knob + `#regenerate` updates all items correctly.

### Progress Checklist

- [x] Tier storage — DB-backed items with ID offset scheme (1M / 2M / 3M)
- [x] `ApplyTierScaling()` formulas (Enchanted/Legendary/Mythic) — in both C++ and Python batch generator
- [x] Multipliers loaded from rules (19 tier rules in `RULE_CATEGORY(ItemProgression)`)
- [ ] `item_scaling_overrides` table + DB migration (deferred to Step 12 or later)
- [ ] `#override set/remove/list` commands (deferred to Step 12 or later)
- [x] `tools/generate_tiered_items.py` — batch generates all 3 tiers for every item (353,874 rows)
- [x] ID offset helpers in `item_tier.h`: `GetTieredItemID()`, `GetBaseItemID()`, `GetTierFromItemID()`, `IsTieredItem()`
- [x] `#itemtier <slot_id> <tier>` admin command — swaps item to DB-backed tiered version
- [x] Dynamic aug slot count by tier (baked into DB rows)
- [x] Augment preservation on tier swap
- [ ] `#regenerate items` command (deferred — not needed until global retuning)
- [x] Build passes, zone.exe compiles clean

### Post-Implementation Notes

> **Completed 2026-03-04. REVISED approach 2026-03-04.**
>
> **Original approach (runtime scaling via custom_data) was abandoned** because the RoF2
> client caches item data and does not reflect `m_scaledItem` changes reliably.
> The Delete+Limbo+Re-add pattern (`SendItemScale()`) appeared to work in server logs but
> the client continued displaying base stats.
>
> **New approach: DB-backed tiered items.**
> Each tier is a real row in the `items` table with pre-computed stats and a tier tag in
> the Name field. No runtime scaling needed — the client sees a completely different item.
>
> **ID Offset Scheme:**
> - Enchanted = base_id + 1,000,000
> - Legendary = base_id + 2,000,000
> - Mythic    = base_id + 3,000,000
> - Max base item ID is ~900,000 so offsets are collision-safe.
> - Name format: "Hategiver (Enchanted)", "Hategiver (Legendary)", "Hategiver (Mythic)"
>
> **Batch Generator: `tools/generate_tiered_items.py`**
> - Reads all base items (id < 1,000,000), generates 3 tiered copies each
> - Uses UPSERT (INSERT ... ON DUPLICATE KEY UPDATE) so re-runs are safe
> - Supports: `--dry-run`, `--verify`, `--item <id>`, `--clean`, `--sample N`, `--config <json>`
> - 117,958 base items × 3 tiers = 353,874 generated rows
> - Completed in ~58 seconds. Verified: each tier has exactly 117,958 items.
>
> **ID Helpers in `common/item_tier.h`:**
> - `GetTieredItemID(base_id, tier)` — base_id + tier × 1,000,000
> - `GetBaseItemID(item_id)` — item_id % 1,000,000
> - `GetTierFromItemID(item_id)` — item_id / 1,000,000
> - `IsTieredItem(item_id)` — item_id >= 1,000,000
> - `TIER_ID_OFFSET` constexpr = 1,000,000
>
> **Command: `#itemtier [slot_id tier]`**
> - Swaps the item in the given slot to the DB-backed tiered version via:
>   DeleteItemInInventory → SummonItem with tiered ID and preserved augments
> - Shows before/after stat comparison including heroics, spell/heal power, attack
> - With no args: lists all equipped items with their current tier
>
> **Runtime scaling code removed from `ApplyCustomStats()`** — the tier scaling block
> that used `custom_data["tier"]` is replaced with a comment explaining the DB approach.
> The C++ formulas in `ApplyTierScaling()`/`ApplyTierAugSlots()` are kept as reference
> and may be useful for on-the-fly tier verification or future use.
>
> **Files modified/created:**
> - `tools/generate_tiered_items.py` — NEW batch generator
> - `common/item_tier.h` — added ID helpers + TIER_ID_OFFSET constant
> - `zone/gm_commands/itemtier.cpp` — rewritten for DB swap approach
> - `common/item_instance.cpp` — removed runtime tier scaling from ApplyCustomStats()
>
> **Deferred items:**
> - Override system, `#regenerate items` — deferred, not blockers for Steps 3-7.
> - When rules change, re-run `generate_tiered_items.py` to regenerate all tiered items.
>
> **Impact on Step 3 (Power Slot XP → Tier-Up):**
> Tier-up now means swapping the item ID: `DeleteItemInInventory` + `SummonItem(tiered_id)`.
> Use `GetTieredItemID(base_id, new_tier)` to compute the target item ID.

---

## Step 3 — Power Slot XP + Kill-Based Tier-Up

**Goal:** Items in the Power Source slot earn XP from kills and automatically tier up when thresholds are reached.

### What to Build

1. **Power Slot XP tracking** — data bucket `power_slot_xp` on the character
   - Tracks cumulative XP for the item currently in the Power Slot
   - XP resets to 0 on tier-up (each tier is an independent threshold)
2. **XP award on kill** — hook mob death event
   - `item_xp = BASE_ITEM_XP × CON_MULTIPLIER × SOURCE_MULTIPLIER`
   - Con color multipliers: Grey=0, Green=0.125, LBlue=0.375, Blue=0.75, White=1.0, Yellow=1.125, Red=1.25
   - Source multipliers: Named ×3, Raid-tier ×5
   - All from rules (knobs: `BASE_ITEM_XP`, `CON_MULT_*`, `NAMED_XP_MULT`, `RAID_TIER_XP_MULT`)
3. **Tier-up trigger** — when XP ≥ threshold, auto tier-up:
   - B→E: 1,000 XP | E→L: 20,000 XP | L→M: 150,000 XP
   - Apply tier scaling (Step 2), re-send item packet
   - Play level-up sound, send distinctive message
4. **XP display** — append to kill message: `"(+40 item XP, 65% to Enchanted)"`
   - Milestone messages at 25%/50%/75%/90%
5. **`#powerslot info`** — show current item, tier, XP, % to next tier
6. **Anti-exploit:** validate item is still in slot on each kill, combat check, grey-con = 0 XP

### What to Test

| Test | Expected Result | Pass Criteria |
|---|---|---|
| Place item in Power Slot, kill white-con mob | +40 XP message, data bucket updates | XP increments correctly |
| Kill grey-con mob | 0 item XP | No XP awarded |
| Kill named mob | +120 XP (40 × 3) | Source multiplier applied |
| Reach 1,000 XP | Auto tier-up to Enchanted, message + sound | Stats update, XP resets |
| Remove item mid-leveling, re-equip | XP preserved in data bucket | Persistent across sessions |
| `#powerslot info` | Shows item name, tier, XP/threshold, % | Accurate display |
| Change `BASE_ITEM_XP` rule → kill mob | New XP amount applied | Rule is live-tunable |
| Swap Power Slot item | Old item's XP preserved, new item starts fresh (or loads its own) | Per-item XP tracking |

### Done When

- Killing mobs with an item in the Power Slot earns XP and the item tiers up automatically.
- XP progress is visible via messages and commands.
- Tier-up feels like a moment (sound + message).
- Anti-exploit checks prevent gaming the system.

### Progress Checklist

- [x] Data bucket `power_slot_xp` per character
- [x] XP-on-kill hook (mob death event)
- [x] Con color multiplier lookup
- [x] Named/raid source multiplier logic
- [x] Tier-up threshold check + auto tier-up
- [x] Level-up sound + distinctive message
- [x] Kill message XP display (`+40 item XP, 65%`)
- [x] Milestone messages (25/50/75/90%)
- [x] `#powerslot info` command
- [x] Anti-exploit: slot validation, grey-con = 0
- [x] Per-item XP persistence on swap
- [x] Build passes, test all cases

### Post-Implementation Notes

> **Completed 2026-03-04.**
>
> **Mob death hook:** Injected into `NPC::Death()` in `zone/attack.cpp`, right after the
> regular XP distribution block (solo/group/raid). For groups/raids, iterates all members
> so everyone with a Power Source item gets item XP.
>
> **Data bucket key:** `power_xp_{base_item_id}` — scoped to the character via
> `Mob::GetScopedBucketKeys()`. XP persists across sessions and item swap/re-equip.
> When an item tiers up, XP resets to 0 for the new tier.
>
> **Con color multipliers:** Grey=0, Green=0.125, LightBlue=0.375, Blue=0.75,
> White=1.0, Yellow=1.125, Red=1.25 — all from rules, live-tunable.
>
> **Source multipliers:** Named (rare_spawn) = ×3, Raid-tier (level 55+) = ×5.
> Stack multiplicatively. A level 55+ named gives ×15.
>
> **Tier-up:** Uses same `DeleteItemInInventory` + `SummonItem` pattern as `#itemtier`.
> Preserves augments and attuned state. Plays level-up sound via `SendSound()`.
>
> **Rules added (17 new):** BaseItemXP, TierThresholdEnchanted/Legendary/Mythic,
> ConMultGrey/Green/LightBlue/Blue/White/Yellow/Red, NamedXPMult, RaidTierXPMult,
> RaidTierMinLevel, PowerSlotXPEnabled, PowerSlotXPMessages, PowerSlotMilestoneMessages.
>
> **Files created:**
> - `zone/power_slot_xp.h` — Header with `PowerSlotXP` namespace API
> - `zone/power_slot_xp.cpp` — Core implementation (XP calc, tier-up, milestones)
> - `zone/gm_commands/powerslot.cpp` — `#powerslot` command (info/reset/setxp)
>
> **Files modified:**
> - `common/ruletypes.h` — 17 new rules in `RULE_CATEGORY(ItemProgression)`
> - `zone/attack.cpp` — Hook in `NPC::Death()` after XP distribution
> - `zone/command.h` / `zone/command.cpp` — Register `#powerslot` command
> - `zone/CMakeLists.txt` — Added new source files
>
> **`#powerslot` command:** Shows item name, tier, XP/threshold, progress bar, XP rates.
> Sub-commands: `reset` (zero XP), `setxp N` (set exact XP). Guide+ access level.
>
> **Anti-exploit:** Grey-con = 0 XP (from ConMultGrey rule). Slot validated on every kill.
> Items already at Mythic get no XP. MerchantType and LDoN treasure excluded.

---

## Step 4 — Essence Currency + Salvage System

**Goal:** Players can salvage unwanted magic items into Essence (alternate currency) and see their balance.

### What to Build

1. **Alternate currencies** — register Common Essence and Rare Essence in `alternate_currency` table
   - Assign currency IDs, names, and item icons
   - Appears in the client's Alternate Currency tab automatically
2. **`CalculateEssenceYield()` function** — uses iLevel from Step 1
   - Gate 1: `magic` flag check — non-magic items rejected
   - Gate 2: `max(1, iLevel - ESSENCE_OFFSET)` — default offset = 100
   - Tier bonus: Base ×1.0, Enchanted ×1.15, Legendary ×1.35
   - Era multiplier from rules
   - Returns Common Essence amount (Rare Essence has % chance from named/raid items)
3. **Salvage Satchel** — special 20-slot container item
   - Created in the items table with a unique flag/custom_data marking it as the Satchel
   - Player receives one from a starter quest or NPC purchase (250 Common Essence or free)
4. **`#salvage` command** — processes all items in the Satchel
   - Iterates each slot, rejects non-magic, calculates Essence, deletes items
   - Grants total Common Essence (and Rare if procs)
   - Sends summary message: `"Salvaged 8 items for 1,247 Common Essence and 3 Rare Essence. 2 items rejected (not magic)."`
5. **`#essence`** — quick balance check command
6. **Rare Essence proc** — configurable chance on salvage (10% from named items, 2% from normal)

### What to Test

| Test | Expected Result | Pass Criteria |
|---|---|---|
| Put Rusty Sword in Satchel, `#salvage` | Rejected — "not magic" | Magic gate works |
| Put Hategiver (iLevel 269) in Satchel, `#salvage` | +169 Common Essence | `max(1, 269-100)` = 169 |
| Put Enchanted Hategiver in Satchel | +194 Common Essence | 169 × 1.15 = 194 |
| Put 5 mixed items in Satchel | Correct total, non-magic rejected | Batch processing works |
| `#essence` | Shows Common and Rare balances | Currency tracking works |
| Open Alternate Currency tab in client UI | Essence visible with icon and balance | Client displays correctly |
| Change `ESSENCE_OFFSET` rule → salvage | Yield changes accordingly | Rule is tunable |

### Done When

- Magic items can be salvaged into Essence through the Satchel + `#salvage`.
- Non-magic items are cleanly rejected with a message.
- Essence balance is visible in-game (command + Alt Currency tab).
- Yields match the iLevel formula from design docs.

### Progress Checklist

- [ ] Register Common Essence + Rare Essence in `alternate_currency`
- [ ] `CalculateEssenceYield()` with magic gate + offset
- [ ] Tier bonus + era multiplier
- [ ] Salvage Satchel item created in DB
- [ ] `#salvage` command (batch process Satchel)
- [ ] `#essence` balance command
- [ ] Rare Essence proc chance logic
- [ ] Verify Essence appears in Alternate Currency tab
- [ ] Build passes, test all cases

### Post-Implementation Notes

> *Fill in after completing this step. Key things to capture:
> How did alt currency registration work? Any client display quirks?
> How is the Satchel identified (item ID, flag, custom_data)?*

---

## Step 5 — Essence Tier-Up (Pay-to-Promote Path)

**Goal:** Players can spend Essence to instantly tier up an item, as an alternative to Kill XP.

### What to Build

1. **`CalculateTierCost()` function** — the iLevel² power curve
   - `cost = max(1, TIER_FLOOR + round(iLevel² × TIER_SCALE))`
   - Per-tier floor and scale from rules
   - Era multiplier from rules
2. **`#tierup` command** — promote the Power Slot item for Essence
   - Checks item is in Power Slot
   - Calculates cost based on item's iLevel and current tier
   - Checks Essence balance ≥ cost
   - Deducts Essence, applies tier-up (Step 2), resets Kill XP to 0 for that tier
   - Sends message with cost paid
3. **`#tierup cost`** — show what the next tier-up would cost without paying
4. **Dual-path display** — `#powerslot info` now shows both paths:
   - `"Kill XP: 12,450 / 20,000 (62%) | Essence cost: 40,387 Common"`

### What to Test

| Test | Expected Result | Pass Criteria |
|---|---|---|
| `#tierup cost` on Rusty Sword (tier 0) | "Base → Enchanted: 3 Common Essence" | iLevel² formula correct |
| `#tierup cost` on Anguish weapon (tier 1) | "Enchanted → Legendary: ~40,387 Common Essence" | Scaled correctly |
| `#tierup` with enough Essence | Item tiers up, Essence deducted, message sent | Balance decreases, tier increases |
| `#tierup` without enough Essence | "Not enough Essence (need X, have Y)" | Clean rejection |
| `#tierup` on Mythic item | "Item is already at maximum tier" | Edge case handled |
| Tier up via XP first → `#tierup cost` | Shows cost for next tier (not current) | Paths don't conflict |
| Change `TIER_SCALE_*` rule → `#tierup cost` | Cost changes | Rules are tunable |

### Done When

- Players have two ways to tier up: grinding kills (free) or paying Essence (instant).
- Costs scale with iLevel² — Rusty Sword costs 3 Essence, Anguish costs ~10k for B→E.
- Both paths coexist cleanly (XP progress + Essence balance visible together).

### Progress Checklist

- [ ] `CalculateTierCost()` with iLevel² power curve
- [ ] Per-tier floor + scale from rules
- [ ] `#tierup` command (Power Slot item)
- [ ] `#tierup cost` preview command
- [ ] Dual-path display in `#powerslot info`
- [ ] Edge cases: max tier, insufficient Essence
- [ ] Build passes, test all cases

### Post-Implementation Notes

> *Fill in after completing this step.*

---

## Step 6 — Duplicate Feeding + Stat Projection

**Goal:** Feeding a duplicate item to the Power Slot grants XP. The Power Slot item projects its stats if the native slot is empty.

### What to Build

1. **Feed system** — `#feed` command (or clickable "Feeding Stone" consumable)
   - Place item on cursor, target Power Slot
   - Validation: cursor item must match Power Slot item (exact `item_id` match)
   - Tier validation: fed item must be same tier or higher
   - XP grant: `FEED_SAME_ITEM_PCT` (20%) of current tier's XP threshold
   - Destroy fed item
   - Message: `"Fed Hategiver to your Power Slot Hategiver! (+4,000 XP, now 82% to Legendary)"`
2. **Stat projection (simplified)**
   - When item is equipped in Power Slot and native slot is empty:
     - Server adds Power Slot item's stats to player stats via `SendStats()`
     - No ghost item — slot appears empty to client but stats apply
   - When native slot is occupied: Power Slot only earns XP, no stat bonus
   - Track projection state so stats are removed if Power Slot item is unequipped

### What to Test

| Test | Expected Result | Pass Criteria |
|---|---|---|
| Place Hategiver on cursor + `#feed` with Hategiver in Power Slot | +4,000 XP (20% of 20,000), Hategiver on cursor destroyed | XP awarded, item consumed |
| Try to feed different item | "Only exact duplicates can be fed" | Validation works |
| Try to feed lower-tier duplicate | Rejected (fed item must be ≥ Power Slot tier) | Tier validation works |
| Equip 1H weapon in Power Slot, Primary hand empty | Player gains weapon's stats (visible in inventory window) | Stat projection active |
| Equip a different weapon in Primary hand | Power Slot stats no longer projected | Projection disabled |
| Remove Primary hand weapon | Power Slot stats re-projected | Projection re-enabled |
| Unequip Power Slot item | Projected stats removed | Clean removal |

### Done When

- Duplicate items can be fed for XP, creating a "jackpot duplicate" feeling.
- Stat projection lets players benefit from their Power Slot item while leveling it.
- Edge cases (slot occupied, tier mismatch, logout) handled cleanly.

### Progress Checklist

- [ ] `#feed` command with duplicate validation
- [ ] Tier validation (fed item ≥ Power Slot tier)
- [ ] XP grant (20% of threshold)
- [ ] Stat projection: Power Slot stats added when native slot empty
- [ ] Projection disabled when native slot occupied
- [ ] Equip/unequip/swap edge cases
- [ ] Logout/zone persistence
- [ ] Build passes, test all cases

### Post-Implementation Notes

> *Fill in after completing this step. Key things to capture:
> How does CalcBonuses() work? Where do equip events fire?
> How did stat projection interact with existing scaling?*

---

## Step 7 — Drop Tier Chances

**Goal:** Items naturally drop at higher tiers from mobs, creating jackpot moments.

### What to Build

1. **Tier roll on loot generation** — hook loot system
   - On each item drop, roll: 80% Base / 15% Enchanted / 4% Legendary / 1% Mythic
   - Apply tier to the ItemInstance via `custom_data` before adding to corpse
   - Apply stat scaling (Step 2) so pre-tiered drops arrive with full scaled stats
   - Hard-mode boss table reserved (0/60/35/5) for future content
2. **Drop announcement** — Enchanted+ drops get a message:
   - `"A Legendary Hategiver drops!"` (gold text)
3. **Quest reward tiers** — system for quests to grant items at specific tiers
   - Short quests → Base, multi-step → Enchanted, epic → Legendary, extreme → Mythic
   - Quest scripts use a helper: `GrantTieredItem(client, item_id, tier)`

### What to Test

| Test | Expected Result | Pass Criteria |
|---|---|---|
| Kill 100+ mobs, examine loot | ~80% Base, ~15% Ench, ~4% Leg, ~1% Myth | Statistical distribution within 2σ |
| Loot an Enchanted item | Stats are ×2, 2 aug slots, name reflects tier | Pre-scaled on corpse |
| Loot a Legendary item | Stats are ×2.6 combat, heroics present, 3 aug slots | Full scaling |
| Drop Legendary item → place in Power Slot | Starts at Legendary tier, only Myth XP needed | Tier preserved |
| Quest grants Enchanted item | Arrives with Enchanted stats and slots | Quest helper works |
| Change `DROP_TIER_STANDARD` rule → kill mobs | New distribution applied | Tunable |

### Done When

- Mobs randomly drop pre-tiered items — finding a Legendary drop feels like a jackpot.
- Pre-tiered items work correctly in Power Slot (skip tiers already achieved).
- Quest reward tiers work.

### Progress Checklist

- [ ] Tier roll logic in loot generation
- [ ] Standard distribution: 80/15/4/1
- [ ] Apply tier + scaling to ItemInstance before corpse add
- [ ] Drop announcement messages (Enchanted+)
- [ ] `GrantTieredItem()` quest helper
- [ ] Verify pre-tiered drops work in Power Slot
- [ ] `DROP_TIER_STANDARD` rule (tunable)
- [ ] Build passes, test all cases

### Post-Implementation Notes

> *Fill in after completing this step. Key things to capture:
> Where in the loot pipeline did we hook? How does the corpse item
> creation flow work? Useful for Step 10 zone/boss aug drops.*

---

## Step 8 — Essence Vendors + Basic Augments

**Goal:** Players can spend Essence at NPC vendors to buy augments and supplies. Basic augments are available.

### What to Build

1. **Define augment items in DB** — all basic stat augs and proc augs from AUGMENT_SYSTEM.md §5.1
   - 7 stat aug types (Stone of Might, Fortitude, Precision, etc.)
   - 5 proc aug types (Shard of Flame, Frost, Venom, Mending, Siphoning)
   - Each at Level 1, using `custom_data` key `"aug_level"` = 1
2. **Common Essence vendor — "The Provisioner"**
   - NPC with alternate currency merchant
   - Sells: L1 stat aug templates (500 CE), L1 proc augs (750 CE), Augment Solvent (100 CE)
3. **Rare Essence vendor — "The Artificer"**
   - Sells: L2 stat augs (1,000 RE), Random Rare Aug Box (3,000 RE), Targeted Rare boxes (7,500 RE)
4. **Leveling vendor augs** — Weaponsmith NPCs in starter cities
   - Combat Stones, Lifetap Stones, Mana Stones (platinum, every 10 levels)
5. **Augment Solvent** — removes an aug from an item safely (already exists in EQ, just need the vendor item)

### What to Test

| Test | Expected Result | Pass Criteria |
|---|---|---|
| Open Provisioner vendor | See aug templates priced in Common Essence | Alt currency vendor works |
| Buy Stone of Might (500 CE) | Aug appears in inventory, Essence deducted | Purchase works |
| Socket Stone of Might into Enchanted item | Fits in slot, +3 Heroic STR visible | Aug applies stats |
| Buy Augment Solvent → use on socketed aug | Aug removed safely, returned to inventory | Non-destructive removal |
| Buy from Weaponsmith (Combat Stone, 5pp) | Cheap weapon proc aug, works at level 1 | Platinum vendor works |
| Open Artificer vendor | Rare Essence items visible | Rare currency vendor works |

### Done When

- Players have a way to spend Essence on useful augments.
- Vendor augs slot into items and provide the documented stats.
- Both Common and Rare Essence economies have a spend path.

### Progress Checklist

- [ ] Define stat aug items in DB (7 types)
- [ ] Define proc aug items in DB (5 types)
- [ ] Provisioner NPC (Common Essence vendor)
- [ ] Artificer NPC (Rare Essence vendor)
- [ ] Weaponsmith leveling augs (platinum)
- [ ] Augment Solvent vendor item
- [ ] Verify aug socketing + stat application
- [ ] Build passes, test all cases

### Post-Implementation Notes

> *Fill in after completing this step. Key things to capture:
> How do alt currency merchants work? NPC creation patterns?
> Aug type/slot compatibility details for Step 9 merge system.*

---

## Step 9 — Augment Merge System

**Goal:** Players can combine 3 same-type, same-level augments into 1 higher-level augment at the Forgemaster.

### What to Build

1. **Augment Forgemaster NPC** — in hub cities, provides a 4-slot combine container
2. **Merge logic** — on combine:
   - Validate: 3 augs are same name AND same level
   - Validate: 4th slot contains correct-tier Merge Catalyst
   - Consume all 4 items, produce 1 aug of next level
   - Aug stats scale per AUGMENT_SYSTEM.md §6.3/6.4 progression tables
3. **Merge Catalysts** — vendor items at the Provisioner/Artificer
   - Lesser (L1→2): 250 CE or 100pp
   - Standard (L2→3): 750 CE or 300pp
   - Greater (L3→4): 2,500 CE or 50 RE
   - Superior (L4→5): 125 RE
4. **Aug level stored in `custom_data`** — `"aug_level"` = 1–5
   - Stats dynamically scaled when sending aug packets based on level
5. **Max level enforcement** — Level 5 is max, merge attempt at Level 5 rejected

### What to Test

| Test | Expected Result | Pass Criteria |
|---|---|---|
| Place 3× L1 Stone of Might + Lesser Catalyst → combine | Produces 1× L2 Stone of Might (+6 Heroic STR) | Merge succeeds |
| Try 2 matching + 1 different → combine | "Augments must be the same type and level" | Validation rejects |
| Try without catalyst | "Merge requires a catalyst" | Validation rejects |
| Merge L4 → L5 (max) | Produces L5 (+21 Heroic STR) | Max level works |
| Try to merge L5 augs | "Augments are already at maximum level" | Max level enforced |
| Socket L3 aug → inspect item | Stats show Level 3 values (+10 Heroic STR, +16 Attack) | Dynamic stat scaling |
| Wrong catalyst tier | "This catalyst is not powerful enough" | Tier validation |

### Done When

- The full merge loop works: buy L1 → merge to L2 → merge to L3 → etc.
- Stats scale correctly at each level per the design tables.
- Catalyst costs create a meaningful investment curve.

### Progress Checklist

- [ ] Forgemaster NPC + 4-slot combine container
- [ ] Merge validation: 3 same type + same level
- [ ] Catalyst validation (correct tier)
- [ ] Consume inputs → produce next-level aug
- [ ] Aug stat scaling per level (progression tables)
- [ ] Merge Catalyst vendor items (4 tiers)
- [ ] `aug_level` in `custom_data`, dynamic stat scaling
- [ ] Max level enforcement (Level 5)
- [ ] Build passes, test all cases

### Post-Implementation Notes

> *Fill in after completing this step. Key things to capture:
> How does the combine container system work? Validation patterns?
> How did dynamic aug stat scaling in packets work?*

---

## Step 10 — Zone Drop Augs + Boss Augs

**Goal:** Augments drop from mobs in the world — zone flavor augs during leveling, premium augs from bosses.

### What to Build

1. **Zone flavor augs** — ultra-rare drops (0.1–0.5%) per zone from AUGMENT_SYSTEM.md §3.3
   - Gnoll Fang Chip (Blackburrow), Darkened Bone Shard (Befallen), etc.
   - Add to zone loot tables with appropriate drop rates
2. **Named mob aug drops** — multi-stat augs from named mobs
   - Higher chance of premium stat augs
   - Drop at Level 1 (mergeable)
3. **Raid-tier boss augs** — signature proc augs (Ember of Nagafen, Fang of Innoruuk, etc.)
   - Multiple variants per boss
   - Drop at Level 1, mergeable
   - Unwanted variants salvage for Rare Essence
4. **Rare Essence from boss items** — guaranteed Rare Essence on salvaging items from named/raid-tier mobs
   - Controlled by `SALVAGE_RARE_CHANCE_NAMED` rule
5. **Daily/weekly bonus Essence** — quest framework (or data-bucket-based) for:
   - Daily endgame quest: 500 CE + 20 RE
   - Weekly boss quest: 2,000 CE + 150 RE

### What to Test

| Test | Expected Result | Pass Criteria |
|---|---|---|
| Farm Blackburrow for ~1,000 kills | Eventually find a Gnoll Fang Chip | Ultra-rare drop works |
| Kill named mob in endgame zone | Chance of premium stat aug drop | Named aug tables populated |
| Kill Nagafen | Drops "Ember of Nagafen" (variant) | Boss aug drops |
| Salvage unwanted boss aug | Rare Essence returned | Rare Essence source from boss loot |
| Socket boss proc aug → hit mobs | Proc fires at documented rate/damage | Proc aug functional |
| Complete daily quest | 500 CE + 20 RE awarded | Bonus Essence works |

### Done When

- World drops provide augments at every level tier.
- Boss fights have signature aug drops worth chasing.
- Multiple Essence sources exist beyond salvaging.

### Progress Checklist

- [ ] Zone flavor augs added to loot tables
- [ ] Named mob aug drop tables
- [ ] Raid boss signature proc augs created
- [ ] Rare Essence from salvaging boss items
- [ ] Daily endgame quest (500 CE + 20 RE)
- [ ] Weekly boss quest (2,000 CE + 150 RE)
- [ ] Proc augs functional (fire on hit)
- [ ] Build passes, test all cases

### Post-Implementation Notes

> *Fill in after completing this step. Key things to capture:
> Loot table patterns used. Quest framework for daily/weekly.*

---

## Step 11 — Augment Infusion + Transmutation

**Goal:** Two additional systems for aug investment: spending Essence to boost an aug, and converting unwanted augs back to Essence.

### What to Build

1. **Augment Infusion** — `#infuse` command or NPC interface
   - Spend Common Essence to add +1 to an aug's primary stat (up to 5 infusions)
   - Costs: 250 / 500 / 1,000 / 2,000 / 4,000 CE per infusion level
   - Cap: merge_stat + infusion_stat ≤ Level 5 maximum
   - Infusion level stored in `custom_data` alongside `aug_level`
2. **Augment Transmutation** — `#transmute` command or NPC
   - Convert an aug back to Essence (partial recovery)
   - L1: 125 CE, L2: 375 CE, L3: 1,000 CE + 25 RE, L4: 2,500 CE + 75 RE, L5: 6,000 CE + 200 RE
   - Destroys the aug

### What to Test

| Test | Expected Result | Pass Criteria |
|---|---|---|
| `#infuse` L3 Stone of Might (1,000 CE) | +10 → +11 Heroic STR, Essence deducted | Stat increases |
| Infuse 5 times on L3 | +10 → +15 | Caps at merge+infusion ≤ 21 |
| Try to infuse L5 at +21 | "Already at maximum power" | Cap enforced |
| `#transmute` L3 Stone of Might | +1,000 CE, aug destroyed | Essence returned |
| `#transmute` L1 vendor aug | +125 CE | Minimum recovery |

### Done When

- Players have a "bad luck protection" path via infusion.
- Unwanted augs can be recycled into Essence.
- Cap system prevents exceeding Level 5 stats.

### Progress Checklist

- [ ] `#infuse` command with per-level costs
- [ ] Infusion level in `custom_data`
- [ ] Cap enforcement: merge + infusion ≤ L5 max
- [ ] `#transmute` command with partial recovery
- [ ] Correct CE/RE returns per aug level
- [ ] Build passes, test all cases

### Post-Implementation Notes

> *Fill in after completing this step.*

---

## Step 12 — DLL Visual Polish (Phase 2)

**Goal:** Clean up UX with client-side DLL support for visual fidelity and ergonomics.

### What to Build

1. **Item name colors** — Enchanted = blue, Legendary = gold, Mythic = orange
   - DLL reads tier from `custom_data` in item packets, renders name color
2. **Salvage Satchel UI button** — "Salvage All" button in the bag window
   - DLL sends custom opcode to server, server runs salvage logic
3. **Right-click "Feed to Power Slot"** — context menu on inventory items
   - DLL sends feed request to server
4. **XP progress bar** — visual bar showing Power Slot XP progress
5. **Tier-up ceremony** — particle effects / flash on tier transition
6. **Grayed-out aug slots** — visual indicator for locked slots on lower-tier items
7. **Essence HUD** — quick display of Common/Rare Essence balance

### What to Test

| Test | Expected Result | Pass Criteria |
|---|---|---|
| Inspect Enchanted item | Name appears in blue | Color rendering works |
| Click "Salvage All" button in Satchel | Salvages all items (same as `#salvage`) | UI button functional |
| Right-click item → "Feed to Power Slot" | Same result as `#feed` | Context menu works |
| Hover Power Slot item | XP progress bar visible | Progress display |
| Tier-up occurs | Visual flash/particle plays | Ceremony effect |
| Inspect Base item | Slots 2-4 grayed out | Visual slot locking |

### Done When

- The system feels polished and integrated — like a native game feature, not a command-line system.
- All server-side commands have DLL UI equivalents.

### Progress Checklist

- [ ] Item name colors (blue/gold/orange by tier)
- [ ] Salvage Satchel "Salvage All" button
- [ ] Right-click "Feed to Power Slot" context menu
- [ ] XP progress bar on Power Slot
- [ ] Tier-up ceremony (particles/flash)
- [ ] Grayed-out aug slot indicators
- [ ] Essence HUD quick display
- [ ] Build EQ Core DLL, test all visuals

### Post-Implementation Notes

> *Fill in after completing this step.*

---

## Dependency Map

```
Step 1 ─── iLevel Engine ──────────────────────────────────────────────────┐
  │                                                                        │
Step 2 ─── Tier Storage + Scaling ─────────────────┐                       │
  │                                                 │                      │
Step 3 ─── Power Slot XP + Tier-Up ─────────────┐  │                      │
  │                                              │  │                      │
Step 4 ─── Essence Currency + Salvage ──────┐    │  │                      │
  │                                         │    │  │                      │
Step 5 ─── Essence Tier-Up (pay path) ──────┤    │  │                      │
  │                                         │    │  │                      │
Step 6 ─── Duplicate Feed + Projection ─────┤    │  │                      │
  │                                         │    │  │                      │
Step 7 ─── Drop Tier Chances ──────────────────────┘  │                    │
  │                                         │         │                    │
Step 8 ─── Essence Vendors + Basic Augs ────┘         │                    │
  │                                                    │                    │
Step 9 ─── Augment Merge System ──────────────────────┘                    │
  │                                                                        │
Step 10 ── Zone Drop Augs + Boss Augs ────────────────────────────────────┘
  │
Step 11 ── Infusion + Transmutation
  │
Step 12 ── DLL Visual Polish
```

**Critical path:** Steps 1 → 2 → 3 are strictly sequential. Steps 4–6 can be done
in any order after Step 2. Steps 7+ layer on after the earlier systems exist.

---

## Effort Estimates

| Step | Scope | Est. Days | Running Total |
|---|---|---|---|
| 1. iLevel Engine | Small — pure math + 1 table | 2 | 2 |
| 2. Tier Storage + Scaling | Medium — packet work + override system | 4 | 6 |
| 3. Power Slot XP | Medium — event hooks + data buckets | 3 | 9 |
| 4. Essence + Salvage | Medium — alt currency + satchel | 3 | 12 |
| 5. Essence Tier-Up | Small — builds on Steps 1-4 | 1 | 13 |
| 6. Feed + Projection | Medium — stat projection has edge cases | 3 | 16 |
| 7. Drop Tier Chances | Small — loot hook + RNG | 2 | 18 |
| 8. Vendors + Basic Augs | Medium — DB items + vendor setup | 3 | 21 |
| 9. Merge System | Medium — container combine + validation | 3 | 24 |
| 10. Zone/Boss Augs | Large — content creation + loot tables | 4 | 28 |
| 11. Infusion + Transmutation | Small — extends existing aug system | 2 | 30 |
| 12. DLL Polish | Large — client-side work | 5 | 35 |

**Playable milestone after Step 7:** The core loop is complete — kill mobs → earn XP → tier
up items → salvage drops → spend Essence → tier up faster. ~18 days.

**Full Phase 1 (Steps 1-11):** ~30 days. Everything works via commands and NPCs.

**Full Phase 2 (+ Step 12):** ~35 days. Polished with DLL integration.

---

## Quick Reference: Key Files to Create/Modify

| File | Step | Purpose |
|---|---|---|
| `common/item_ilevel.h/.cpp` | 1 | iLevel calculation engine |
| `common/item_tier.h/.cpp` | 2 | Tier scaling formulas + override system |
| `zone/power_slot.h/.cpp` | 3 | Power Slot XP tracking + tier-up logic |
| `zone/salvage.h/.cpp` | 4 | Salvage Satchel + Essence yield |
| `zone/command/item_commands.cpp` | 1-6 | `#item ilevel`, `#item tier`, `#override`, `#feed`, etc. |
| `zone/command/salvage_commands.cpp` | 4-5 | `#salvage`, `#essence`, `#tierup` |
| DB: `alternate_currency` | 4 | Common + Rare Essence currency definitions |
| DB: `items` | 1,8,10 | `calculated_ilevel` column, aug items, zone drop augs |
| DB: `item_scaling_overrides` | 2 | Manual stat overrides table |
| DB: `loot_drop_entries` | 10 | Zone/boss aug drop rates |

---

*End of Implementation Steps v1.0.*
