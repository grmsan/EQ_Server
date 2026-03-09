# Infinite Item Progression — Implementation Steps

**Version:** 1.1
**Date:** March 5, 2026
**Status:** Steps 1-11 Complete — Step 12 In Progress (POC-1 SIDL window ✅, POC-2 HUD ✅, 12C EdgeStatLabel ✅)

---

## Guiding Principles

- Each step produces a **testable, playable chunk** of value.
- Steps are ordered by dependency: later steps build on earlier ones.
- Each step lists **what to build**, **what to test**, and **what "done" looks like**.
- **Players should never need slash commands for core gameplay.** Use native EQ UI elements:
  Combine buttons, Alternate Currency tab, tradeskill containers, etc. Commands are GM-only admin tools.
- All knobs are tunable via rules/DB from the start — no hardcoded constants.

---

## Master Progress Tracker

| Step | Name | Status | Date Started | Date Completed |
| --- | --- | --- | --- | --- |
| 1 | iLevel Calculation Engine | ✅ Complete | 2025-07-15 | 2026-03-03 |
| 2 | Item Tier Storage + Stat Scaling | ✅ Complete | 2026-03-04 | 2026-03-04 |
| 3 | Power Slot XP + Kill-Based Tier-Up | ✅ Complete | 2026-03-04 | 2026-03-04 |
| 4 | Essence Currency + Salvage System | ✅ Complete | 2026-03-05 | 2026-03-05 |
| 5 | Consume Item / Consume Essence AAs | ✅ Complete | 2026-03-05 | 2026-03-05 |
| 6 | Ghost Copy (Power Source → Equipment) | ✅ Complete | 2026-03-05 | 2026-03-05 |
| 7 | Drop Tier Chances | ✅ Complete | 2026-03-06 | 2026-03-06 |
| 8 | Essence Vendors + Basic Augments | ✅ Complete | 2026-03-06 | 2026-03-06 |
| 9 | Augment Merge System | ✅ Complete | 2026-03-06 | 2026-03-06 |
| 10 | Zone Drop Augs + Boss Augs | ✅ Complete | 2026-03-06 | 2026-03-06 |
| 11 | Augment Infusion + Transmutation | ✅ Complete | 2026-03-06 | 2026-03-06 |
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
> **ID Offset Scheme (TIER_ID_OFFSET = 250,000):**
> - Enchanted = base_id + 250,000
> - Legendary = base_id + 500,000
> - Mythic    = base_id + 750,000
> - Kept within 20-bit item-link mask (0xFFFFF = 1,048,575) so chat links work.
> - Max linkable base_id = 298,575. Items above this are skipped by the generator.
> - Name format: "Hategiver (Enchanted)", "Hategiver (Legendary)", "Hategiver (Mythic)"
>
> **Batch Generator: `tools/generate_tiered_items.py`**
> - Reads all base items (id < 250,000), generates 3 tiered copies each
> - Uses UPSERT (INSERT ... ON DUPLICATE KEY UPDATE) so re-runs are safe
> - Supports: `--dry-run`, `--verify`, `--item <id>`, `--clean`, `--sample N`, `--config <json>`
> - 117,958 base items × 3 tiers = 353,874 generated rows
> - Completed in ~58 seconds. Verified: each tier has exactly 117,958 items.
>
> **ID Helpers in `common/item_tier.h`:**
> - `GetTieredItemID(base_id, tier)` — base_id + tier × 250,000
> - `GetBaseItemID(item_id)` — item_id % 250,000
> - `GetTierFromItemID(item_id)` — item_id / 250,000
> - `IsTieredItem(item_id)` — item_id >= 250,000
> - `TIER_ID_OFFSET` constexpr = 250,000
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

1. **Power Slot XP tracking** — stored as `custom_data("Exp")` on the ItemInstance
   - Each weapon/armor item carries its own cumulative XP in its custom data field
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
| Place item in Power Slot, kill white-con mob | +40 XP message, custom_data updates | XP increments correctly |
| Kill grey-con mob | 0 item XP | No XP awarded |
| Kill named mob | +120 XP (40 × 3) | Source multiplier applied |
| Reach 1,000 XP | Auto tier-up to Enchanted, message + sound | Stats update, XP resets |
| Remove item mid-leveling, re-equip | XP preserved in custom_data | Persistent across sessions |
| `#powerslot info` | Shows item name, tier, XP/threshold, % | Accurate display |
| Change `BASE_ITEM_XP` rule → kill mob | New XP amount applied | Rule is live-tunable |
| Swap Power Slot item | Old item's XP preserved, new item starts fresh (or loads its own) | Per-item XP tracking |

### Done When

- Killing mobs with an item in the Power Slot earns XP and the item tiers up automatically.
- XP progress is visible via messages and commands.
- Tier-up feels like a moment (sound + message).
- Anti-exploit checks prevent gaming the system.

### Progress Checklist

- [x] ItemInstance `custom_data("Exp")` per item
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
> **XP storage model:** Uses `custom_data("Exp")` on the ItemInstance itself — each
> item carries its own XP. XP persists across sessions and item swap/re-equip.
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

**Goal:** Players can salvage unwanted magic items into Essence (alternate currency) using a Combine button on the Salvage Satchel, and see their balance in the Alternate Currency tab.

### What to Build

1. **Alternate currencies** — register Common Essence and Rare Essence in `alternate_currency` table
   - Assign currency IDs, names, and item icons
   - Appears in the client's **Alternate Currency tab** automatically — no command needed
2. **`CalculateEssenceYield()` function** — uses iLevel from Step 1
   - Gate 1: `magic` flag check — non-magic items rejected
   - Gate 2: `max(1, iLevel - ESSENCE_OFFSET)` — default offset = 100
   - Tier bonus: Base ×1.0, Enchanted ×1.15, Legendary ×1.35
   - Era multiplier from rules
   - Returns Common Essence amount (Rare Essence has % chance from named/raid items)
3. **Salvage Satchel** — special 20-slot container item with **Combine button**
   - Created in the items table with BagType=10 (BagTypeToolBox — displays Combine button)
   - Player receives one from a starter quest or NPC purchase (250 Common Essence or free)
   - Clicking Combine triggers salvage logic via `HandleCombine()` interception
4. **HandleCombine hook** — intercepts Salvage Satchel in `Object::HandleCombine()`
   - Same pattern as TransformationMold/DetransformationMold — check container item ID, do custom logic, respond, return
   - Delegates to `Salvage::ProcessSatchel()` for all salvage logic
   - Iterates each slot, rejects non-magic, calculates Essence, deletes items
   - Grants total Common Essence (and Rare if procs)
   - Sends per-item breakdown messages
5. **GM-only commands** (Guide+ access, not for normal players)
   - `#salvage` — manually triggers Satchel processing for admin testing
   - `#essence` — shows/modifies Essence balances for admin debugging
6. **Rare Essence proc** — configurable chance on salvage (10% from named items, 2% from normal)

### What to Test

| Test | Expected Result | Pass Criteria |
|---|---|---|
| Put Rusty Sword in Satchel, click Combine | Rejected — "not magic" | Magic gate works |
| Put Hategiver (iLevel 269) in Satchel, Combine | +169 Common Essence | `max(1, 269-100)` = 169 |
| Put Enchanted Hategiver in Satchel, Combine | +194 Common Essence | 169 × 1.15 = 194 |
| Put 5 mixed items in Satchel, Combine | Correct total, non-magic rejected | Batch processing works |
| Open Alternate Currency tab in client UI | Essence visible with icon and balance | Client displays correctly |
| Change `ESSENCE_OFFSET` rule → Combine | Yield changes accordingly | Rule is tunable |
| `#salvage` (GM command) | Same result as Combine button | Admin wrapper works |
| `#essence` (GM command) | Shows Common and Rare balances | Admin tool works |

### Done When

- Magic items can be salvaged into Essence by placing them in the Satchel and clicking **Combine**.
- Non-magic items are cleanly rejected with a message.
- Essence balance is visible in the **Alternate Currency tab** — no command needed.
- Yields match the iLevel formula from design docs.
- GM commands exist for admin testing/debugging only.

### Progress Checklist

- [x] Register Common Essence + Rare Essence in `alternate_currency`
- [x] `CalculateEssenceYield()` with magic gate + offset
- [x] Tier bonus + era multiplier
- [x] Salvage Satchel item created in DB (BagType=10 for Combine button)
- [x] HandleCombine hook in tradeskills.cpp (intercepts Satchel Combine)
- [x] `Salvage::ProcessSatchel()` core logic (salvage_system.cpp)
- [x] GM-only `#salvage` command (Guide+ access)
- [x] GM-only `#essence` command (Guide+ access)
- [x] Rare Essence proc chance logic
- [ ] Verify Essence appears in Alternate Currency tab
- [ ] Verify Combine button triggers salvage in-game
- [x] Build passes, test all cases

### Post-Implementation Notes

> **Completed 2026-03-05.**
>
> **Alternate Currency Registration:**
> Two currencies registered in `alternate_currency` table: Common Essence (ID 100, item 200010)
> and Rare Essence (ID 101, item 200011). Currency IDs are rule-configurable via
> `RuleI(ItemProgression, CommonEssenceCurrencyID)` and `RuleI(ItemProgression, RareEssenceCurrencyID)`.
> Token items (200010, 200011) exist only for client icon display.
> Players view their Essence balance via the **Alternate Currency tab** in the character sheet —
> no command needed.
>
> **Salvage Satchel (Combine Button):**
> Item ID 200020 (rule-configurable via `SalvageSatchelItemID`). 20-slot bag, NODROP, ALL/ALL,
> GIANT size, 100% weight reduction, BagType=10 (BagTypeToolBox — displays a **Combine button**
> in the RoF2 client). Player gets one via `#summonitem 200020`.
>
> **Player workflow:** Place items in Satchel → click Combine → salvage logic runs automatically.
> The Combine click sends `OP_TradeSkillCombine` which is intercepted in `Object::HandleCombine()`
> (tradeskills.cpp) before any tradeskill recipe lookup. The handler checks the container's item ID
> against `RuleI(ItemProgression, SalvageSatchelItemID)` and delegates to `Salvage::ProcessSatchel()`.
>
> **Salvage logic (`Salvage::ProcessSatchel()` in salvage_system.cpp):**
> For each item in the satchel:
> - Non-magic items rejected (left in satchel)
> - Mythic items returned (unsalvageable, left in satchel)
> - Common Essence = `CalculateEssenceYield(item, tier)` (from Step 1)
> - Rare Essence: random proc based on item quality (iLevel >= RaidTierMinLevel threshold
>   uses SalvageRareChanceNamed=10%, else SalvageRareChanceNormal=2%)
> - Awards currency via `AddAlternateCurrencyValue()` and shows per-item breakdown
>
> **GM-only commands (Guide+ access):**
> - `#salvage` — manually triggers Satchel processing (thin wrapper, calls `Salvage::ProcessSatchel()`)
> - `#essence` — shows balances, `add N [rare]`/`set N [rare]` for admin currency manipulation
> These are **not intended for normal players** — the Combine button and Alt Currency tab are
> the player-facing interfaces.
>
> **New rules added (7 total):**
> `CommonEssenceCurrencyID`, `RareEssenceCurrencyID`, `SalvageSatchelItemID`,
> `SalvageRareChanceNormal`, `SalvageRareChanceNamed`, `SalvageRareAmountMin`, `SalvageRareAmountMax`
>
> **SQL migration:** `utils/sql/item_progression/step04_essence_currency_salvage.sql`
> Creates token items, registers currencies, creates Salvage Satchel (BagType=10). Uses UPSERT for idempotency.
>
> **Files created:**
> - `zone/salvage.h` — Salvage namespace header
> - `zone/salvage_system.cpp` — Core salvage logic (`Salvage::ProcessSatchel()`)
> - `zone/gm_commands/salvage.cpp` — GM-only #salvage command (thin wrapper)
> - `zone/gm_commands/essence.cpp` — GM-only #essence command
> - `utils/sql/item_progression/step04_essence_currency_salvage.sql` — DB migration
>
> **Files modified:**
> - `common/ruletypes.h` — 7 new rules in ItemProgression category
> - `zone/tradeskills.cpp` — HandleCombine hook for Salvage Satchel interception
> - `zone/command.h` — command declarations
> - `zone/command.cpp` — command registration (Guide+ access)
> - `zone/CMakeLists.txt` — new source files
>
> **Pre-existing infrastructure used:**
> - `CalculateEssenceYield()` and `CalculateTierCost()` from Step 1 (`item_ilevel.cpp`)
> - `AddAlternateCurrencyValue()` / `GetAlternateCurrencyValue()` / `SetAlternateCurrencyValue()`
> - `zone->DoesAlternateCurrencyExist()` gates currency operations
> - `EQ::InventoryProfile::CalcSlotId()` for bag sub-slot addressing
> - `Object::HandleCombine()` pattern (same as TransformationMold/DetransformationMold)
>
> **Still needs manual verification:**
> - Client Alternate Currency tab display (requires DB migration + server restart)
> - Salvage Satchel Combine button test with real items in-game

---

## Step 5 — Consume Item / Consume Essence AAs

**Goal:** Players can add XP to their Power Slot item by consuming a matching duplicate item, or by spending Common Essence. Both are activated via AAs (no slash commands needed).

### What to Build

1. **Consume Item AA** (ability 32100, rank 50100)
   - Player puts a matching item on cursor, activates the AA
   - Cursor item must match Power Slot item (same base item ID)
   - XP granted as % of current tier's threshold based on tier comparison:
     - Higher tier consumed: `ConsumeItemHigherTierPct` (100%) of threshold
     - Same tier: `ConsumeItemSameTierPct` (33%) of threshold
     - Lower tier: `ConsumeItemLowerTierPct` (7%) of threshold
   - Cursor item is destroyed, XP added via `PowerSlotXP::AddXP()`
   - Tier-up happens automatically when threshold is reached
2. **Consume Essence AA** (ability 32101, rank 50101)
   - Player activates the AA (no cursor item needed)
   - Calculates remaining XP to next tier
   - Converts to Essence cost at `ConsumeEssencePerXP` ratio (default 1:1)
   - Consumes only what is needed from Common Essence balance
   - If balance < needed, consumes partial amount for proportional XP
   - XP added via `PowerSlotXP::AddXP()`, tier-up when threshold reached
3. **AA activation hook** — custom intercept in `ActivateAlternateAdvancementAbility()`
   - Fires before `IsValidSpell()` check — AAs don't cast spells, they call C++ directly
   - Same pattern as Bazaar and Back AA intercept
4. **Shared `PowerSlotXP::AddXP()` function** — refactored from `AwardKillXP()`
   - Handles XP addition, threshold check, tier-up, celebration messages, milestones
   - Used by: `AwardKillXP()` (kills), `HandleConsumeItem()`, `HandleConsumeEssence()`
5. **Dual-path display** — `#powerslot` now shows:
   - `"Essence to next tier: N Common | Consume Item: 100%/33%/7% XP (higher/same/lower)"`

### What to Test

| Test | Expected Result | Pass Criteria |
|---|---|---|
| Consume matching same-tier item on cursor | +33% of threshold XP, cursor item destroyed | Base flow works |
| Consume higher-tier item on cursor | +100% of threshold XP | Higher-tier bonus applies |
| Consume lower-tier item on cursor | +7% of threshold XP | Lower-tier penalty applies |
| Consume non-matching item | Rejected: "Only duplicates..." | Base ID validation works |
| Consume with no cursor item | Rejected: "Place an item on your cursor" | Empty cursor handled |
| Consume Essence with enough balance | Correct Essence deducted, XP filled to threshold | Full consume works |
| Consume Essence with partial balance | All Essence consumed, proportional XP added | Partial consume works |
| Consume Essence with zero balance | Rejected: "no Common Essence" | Edge case handled |
| Consume on max-tier item | Rejected: "already at maximum tier" | Max tier handled |
| `#powerslot` display | Shows Essence cost and consume rates | Dual-path info display |
| Tier-up via Consume Item | Item tiers up, celebration message, overflow XP | Tier-up flow works |
| Kill XP + Consume stack | Both add to same XP bar, tier-up at threshold | Paths integrate |

### Done When

- Players have three ways to gain item XP: killing mobs (free), consuming duplicates (AA), or spending Essence (AA).
- No slash commands needed — both consume paths are AA-activated.
- AAs appear in the AA window and can be hotkeyed.
- `#powerslot` shows Essence cost alongside XP progress.
- All paths feed the same XP bar and trigger tier-up identically.

### Progress Checklist

- [x] `ConsumeItemSameTierPct/LowerTierPct/HigherTierPct` rules
- [x] `ConsumeEssencePerXP` rule
- [x] `PowerSlotXP::AddXP()` shared function (refactored from AwardKillXP)
- [x] `PowerSlotXP::DoTierUp()` exposed publicly
- [x] `ConsumeSystem::HandleConsumeItem()` — cursor item → Power Slot XP
- [x] `ConsumeSystem::HandleConsumeEssence()` — Common Essence → Power Slot XP
- [x] AA intercept in `ActivateAlternateAdvancementAbility()` (before IsValidSpell)
- [x] Dual-path display in `#powerslot`
- [x] SQL migration for AA entries (ability + ranks)
- [ ] Verify AAs appear in AA window (requires DB migration + server restart)
- [ ] Verify Consume Item works in-game
- [ ] Verify Consume Essence works in-game
- [ ] Add eqstr_us.txt entries for AA display names
- [x] Build passes

### Post-Implementation Notes

> **Completed 2026-03-05.**
>
> **Architecture:**
> This step replaces the original `#tierup` command design with AA-based consume abilities.
> Players never need slash commands — both consume paths are AA-activated and can be hotkeyed.
> The shared `PowerSlotXP::AddXP()` function unifies all XP sources (kills, consume item,
> consume essence) through the same threshold/tier-up logic.
>
> **Consume Item AA (ability 32100, rank 50100):**
> Cursor item must match Power Slot item (same base ID via `GetBaseItemID()`). Attuned items
> rejected (THJ parity). XP granted based on tier comparison:
> - Higher tier consumed → `ConsumeItemHigherTierPct` (default 100%) of threshold
> - Same tier → `ConsumeItemSameTierPct` (default 33%) of threshold
> - Lower tier → `ConsumeItemLowerTierPct` (default 7%) of threshold
> Cursor item destroyed after consumption.
>
> **Consume Essence AA (ability 32101, rank 50101):**
> Spends Common Essence from alternate currency balance. Calculates remaining XP to next tier,
> converts to Essence via `ConsumeEssencePerXP` rule (default 1.0 = 1:1 ratio). Consumes only
> what is needed — excess stays in balance. If balance < needed, consumes partial for proportional XP.
>
> **AA Activation Flow:**
> Both AAs are intercepted early in `ActivateAlternateAdvancementAbility()` (aa.cpp), before
> the `IsValidSpell()` check. This allows them to use `spell = -1` in the DB (no actual spell
> needed). The intercept delegates to `ConsumeSystem::HandleConsumeItem()` or
> `ConsumeSystem::HandleConsumeEssence()` and returns immediately.
>
> **Refactoring (PowerSlotXP):**
> `DoTierUp()` made public (was static). New `AddXP()` function extracted from `AwardKillXP()`,
> handling XP addition, threshold check, tier-up (via DoTierUp), celebration messages, and
> milestone messages. `AwardKillXP()` now calculates kill-specific XP, sends the per-kill
> message, then delegates to `AddXP()`.
>
> **New rules (4):**
> `ConsumeItemSameTierPct` (33), `ConsumeItemLowerTierPct` (7),
> `ConsumeItemHigherTierPct` (100), `ConsumeEssencePerXP` (1.0)
>
> **SQL migration:** `utils/sql/item_progression/step05_consume_item_essence_aa.sql`
> Creates AA ability + rank entries. Auto-grant enabled (requires `AutoGrantAAExpansion >= 0`).
> Includes client string instructions for eqstr_us.txt.
>
> **Files created:**
> - `zone/consume_system.h` — ConsumeSystem namespace header
> - `zone/consume_system.cpp` — HandleConsumeItem() and HandleConsumeEssence()
> - `utils/sql/item_progression/step05_consume_item_essence_aa.sql` — AA database entries
>
> **Files modified:**
> - `zone/aa.cpp` — AA constants + intercept before IsValidSpell
> - `zone/power_slot_xp.h` — Exposed DoTierUp, added AddXP declaration
> - `zone/power_slot_xp.cpp` — DoTierUp non-static, AddXP refactored from AwardKillXP
> - `zone/gm_commands/powerslot.cpp` — Dual-path display (Essence cost + consume rates)
> - `common/ruletypes.h` — 4 new rules in ItemProgression category
> - `zone/CMakeLists.txt` — consume_system.cpp added
>
> **Pre-existing infrastructure used:**
> - `PowerSlotXP::GetThresholdForNextTier()`, `GetCurrentXP()`
> - `ItemProgression::GetBaseItemID()`, `GetTierFromItemID()`, `GetTierName()`
> - `AddAlternateCurrencyValue()`, `GetAlternateCurrencyValue()`
> - `zone->DoesAlternateCurrencyExist()`, `EQ::invslot::slotCursor`
>
> **Client requirements:**
> - eqstr_us.txt entries for SIDs 900100-900107 (AA name/description display)
> - Rule: `Expansion:AutoGrantAAExpansion` >= 0 for auto-grant, OR manual `#grant_aa all`

---

## Step 6 — Ghost Copy (Power Source → Equipment)

**Goal:** When a progression item (weapon, armor, etc.) sits in the Power Source slot, a ghost copy of that item is placed in the item's native equipment slot so the client can SEE and USE the weapon/armor while it levels up.

*Note: The original "Feed system" from this step is fully covered by the Consume Item AA (Step 5). That AA already accepts any tier of the same base item and grants tier-dependent XP rates.*

### What to Build

1. **Ghost Copy module** — `GhostCopy::UpdateGhostCopy(Client*)`, `HandleMoveItem()`, `RemoveGhostCopy()`
   - Scans the Power Source item's `Slots` bitmask for empty native equipment slots
   - Multi-slot items (e.g., ring → Finger1 or Finger2): first empty wins
   - 2H weapons: requires BOTH Primary AND Secondary to be empty
   - Real power-source-only items (only PS slot bit) are unaffected
   - `IsProgressionItem()` distinguishes progression items from real power sources
2. **Ghost copy placement** — places an actual `ItemInstance` clone in the native slot
   - Clone has `GhostCopy` custom_data marker (debug identification)
   - Placed via `m_inv.PutItem()` + `SendItemPacket()` — NO database save
   - Client sees the item in both Power Source and native equipment slot
   - `SendWearChange()` updates the visual model (weapon in hand, armor appearance)
3. **Ghost copy pickup** — intercept in `Client::SwapItem()`
   - Pick up FROM ghost slot → remove ghost, take real PS item, put on cursor
   - Pick up FROM PS while ghost active → remove ghost first, proceed with normal move
   - Equip TO ghost slot → remove ghost first, equip normally
4. **PS stat skip** — in `Mob::CalcItemBonuses()` (bonuses.cpp)
   - Progression items in PS slot ALWAYS skip bonus contribution
   - Stats come from the ghost copy in the native equipment slot instead
   - Prevents double-counting (item is in both PS and native slot)
5. **Ghost lifecycle** — transient, regenerated automatically
   - Created/refreshed in `GhostCopy::UpdateGhostCopy()` called from `Client::CalcBonuses()`
   - Removed when PS item is picked up, ghost slot is picked up, or native slot gets a real item
   - Never saved to database — regenerated on login/zone-in via CalcBonuses path
   - Tier-up detection: if PS item ID changes (tier up), ghost is refreshed
6. **`#powerslot` ghost status** — shows "Ghost Copy: ACTIVE → Primary slot" or "INACTIVE"
7. **Rule**: `StatProjectionEnabled` (bool, default true) — master toggle for ghost copy system

### What to Test

| Test | Expected Result | Pass Criteria |
|---|---|---|
| 1H weapon in PS, Primary empty | Ghost copy appears in Primary, client sees weapon in hand | Both slots show item |
| Equip different weapon in Primary | Ghost removed, real weapon in Primary | Ghost disappears |
| Remove Primary weapon | Ghost re-placed from PS item | Ghost reappears |
| 2H weapon in PS, both hands empty | Ghost in Primary, weapon visible | 2H ghost works |
| 2H weapon in PS, shield in Secondary | NO ghost (2H requires both hands) | 2H+shield block |
| Pick up ghost from Primary | Both PS and Primary cleared, item on cursor | Single pickup |
| Pick up from PS while ghost active | Ghost removed, PS item on cursor | Paired removal |
| Equip over ghost slot | Ghost removed, new item in slot | Clean replacement |
| Armor in PS, matching slot empty | Ghost copy shows armor in that slot | Non-weapon ghost |
| Ring in PS, both fingers occupied | No ghost | Multi-slot: all occupied |
| Ring in PS, one finger empty | Ghost in empty finger slot | Multi-slot: first empty |
| Tier up while ghost active | Ghost refreshes with new tier item data | Auto-refresh |
| Zone/logout with ghost active | Ghost regenerated on re-enter | Transient lifecycle |
| `#powerslot` with ghost active | Shows "Ghost Copy: ACTIVE → Primary slot" | Display works |
| `StatProjectionEnabled = false` | No ghost regardless of slot state | Rule toggle |

### Done When

- Progression items in Power Source create a visible ghost copy in the native equipment slot.
- Client sees and can use the weapon/armor (it appears in their hand/on their body).
- Picking up from either slot removes BOTH and places the real item on cursor.
- Ghost copies are never saved to the database — they regenerate on login/zone automatically.
- No double-counting of stats (PS item bonuses are always skipped for progression items).
- `#powerslot` shows ghost status.

### Progress Checklist

- [x] `GhostCopy::IsProgressionItem()` — distinguishes progression items from real PS items
- [x] `GhostCopy::FindTargetSlot()` — scans Slots bitmask for empty native slots
- [x] `GhostCopy::UpdateGhostCopy()` — master place/remove logic with reentrance guard
- [x] `GhostCopy::HandleMoveItem()` — intercepts SwapItem for ghost-related moves
- [x] `GhostCopy::RemoveGhostCopy()` — safe removal helper
- [x] 2H weapon validation (both Primary + Secondary must be empty)
- [x] CalcItemBonuses: always skip PS bonuses for progression items
- [x] CalcBonuses: calls UpdateGhostCopy after bonus computation
- [x] SwapItem: ghost intercept at top of function
- [x] Ghost clone uses `m_inv.PutItem()` + `SendItemPacket()` + `SendWearChange()`
- [x] Ghost clone NOT saved to database (transient)
- [x] Tier-up detection (item ID mismatch → ghost refresh)
- [x] `Client::m_ghost_copy_slot` + `m_updating_ghost` state tracking
- [x] `#powerslot` ghost status display
- [x] `StatProjectionEnabled` rule (default true)
- [x] Build passes (0 errors, 0 warnings)
- [ ] Verify ghost appears in-game (equip weapon in PS, empty Primary)
- [ ] Verify ghost disappears when Primary is filled
- [ ] Verify pickup from ghost slot removes both
- [ ] Verify 2H + shield blocks ghost

### Post-Implementation Notes

> **Completed 2026-03-05.** Rewrote from stat-only projection to full ghost copy system.
>
> **Architecture:**
> Ghost copies are actual `ItemInstance` clones placed in equipment slots via the internal
> inventory API (`m_inv.PutItem()` + `SendItemPacket()`), bypassing `database.SaveInventory()`
> to keep them transient. The client sees a real item in the slot — it shows the weapon model
> in the player's hand and the armor on their body. This is fundamentally different from the
> earlier stat-only projection approach which only added bonuses via `CalcItemBonuses`.
>
> **Stat Handling:**
> Progression items in the Power Source slot ALWAYS have their bonuses skipped in
> `CalcItemBonuses()`. Stats come exclusively from the ghost copy in the native equipment
> slot, which is processed by the normal bonus loop like any equipped item. When no ghost
> exists (native slot occupied), the PS progression item provides no stats — it just gains XP.
>
> **Move Intercept:**
> `GhostCopy::HandleMoveItem()` is called at the very top of `Client::SwapItem()`, before
> any item instances are loaded. Three cases are handled:
> 1. Pick up FROM ghost slot → delete ghost, take real PS item, push to cursor
> 2. Pick up FROM PS while ghost active → delete ghost first, continue normal move
> 3. Equip TO ghost slot → delete ghost first, continue normal equip
>
> **Lifecycle:**
> `UpdateGhostCopy()` is called from `Client::CalcBonuses()` after all bonuses are computed.
> It checks whether a ghost should exist, validates the current ghost (item ID match for
> tier-up detection), and places/removes as needed. A reentrance guard (`m_updating_ghost`)
> prevents infinite loops since placing items can trigger CalcBonuses indirectly.
>
> **Files created:**
> - `zone/ghost_copy.h` — GhostCopy namespace header
> - `zone/ghost_copy.cpp` — IsProgressionItem, FindTargetSlot, UpdateGhostCopy, HandleMoveItem, RemoveGhostCopy
>
> **Files modified:**
> - `zone/bonuses.cpp` — CalcItemBonuses always-skip for PS progression items + CalcBonuses ghost update call
> - `zone/inventory.cpp` — SwapItem ghost intercept + include
> - `zone/client.h` — m_ghost_copy_slot, m_updating_ghost + accessors
> - `zone/gm_commands/powerslot.cpp` — Ghost Copy status display
> - `zone/CMakeLists.txt` — replaced stat_projection.cpp with ghost_copy.cpp
>
> **Files removed:**
> - `zone/stat_projection.h` — replaced by ghost_copy.h
> - `zone/stat_projection.cpp` — replaced by ghost_copy.cpp

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

- [x] Tier roll logic in loot generation
- [x] Standard distribution: 80/15/4/1
- [x] Apply tier via item_id swap (tiered DB entries have pre-baked stats)
- [x] Drop announcement messages (Enchanted+)
- [x] `GrantTieredItem()` quest helper (Lua + Perl)
- [ ] Verify pre-tiered drops work in Power Slot
- [x] `DropTierEnabled` / `DropChanceEnchanted/Legendary/Mythic` rules (tunable)
- [x] Build passes, test all cases

### Post-Implementation Notes

> **Hook point:** `NPC::AddLootDrop()` in `zone/loot.cpp`. After `item2` validation
> and before `LootItem` struct creation, we roll `zone->random.Real(0,100)` against
> cumulative thresholds (Mythic ≤ 1%, Legendary ≤ 5%, Enchanted ≤ 20%).
>
> **Approach:** Swap the `item2` pointer to the tiered `EQ::ItemData*` from the DB.
> Since generate_tiered_items.py pre-bakes all stats into tiered DB rows, no runtime
> scaling or custom_data is needed. All downstream code (`item->item_id = item2->ID`,
> `database.CreateItem(item2->ID, ...)`, equip logic) naturally uses the tiered version.
>
> **Announcement:** In `NPC::Death()` (`zone/attack.cpp`), after `entity_list.AddCorpse()`,
> we scan `m_loot_items` for tiered items and send a colored message to the killer's
> group/raid using `ItemProgression::GetTierChatColor()` and `GetTierName()`.
>
> **Quest API:** `quest::grant_tiered_item(base_id, tier)` in Perl,
> `eq.grant_tiered_item(base_id, tier)` in Lua. Both call
> `QuestManager::grant_tiered_item()` which validates tier range and DB existence,
> then calls `Client::SummonItem(tiered_id)`.
>
> **Rules:** `ItemProgression:DropTierEnabled` (bool), `DropChanceEnchanted` (15),
> `DropChanceLegendary` (4), `DropChanceMythic` (1). All tunable via `rule_values`.
>
> **Files modified:** `zone/loot.cpp`, `zone/attack.cpp`, `common/ruletypes.h`,
> `zone/questmgr.h`, `zone/questmgr.cpp`, `zone/lua_general.cpp`,
> `zone/embparser_api.cpp`.

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

- [x] Define stat aug items in DB (7 types)
- [x] Define proc aug items in DB (5 types)
- [x] Provisioner NPC (Common Essence vendor)
- [x] Artificer NPC (Rare Essence vendor)
- [x] Weaponsmith leveling augs (platinum)
- [x] Augment Solvent vendor item
- [ ] Verify aug socketing + stat application (runtime test pending)
- [x] Build passes, test all cases

### Post-Implementation Notes

> **Completed.** All augment items, spells, vendors, and merchant lists generated
> via `tools/generate_augments.py` — a re-runnable Python script with a CONFIG
> section at the top for easy mass-tuning of stats, costs, and scaling.
>
> **Generated SQL:** `utils/sql/item_progression/step08_09_augments_vendors_merge.sql`
> (139 statements, idempotent — DELETE-first pattern).
>
> **ID Ranges Allocated:**
> - Items: 200100–200139 (40 items: 27 leveling weapon augs, 7 endgame stat augs, 5 endgame proc augs, 1 solvent)
> - Spells: 65100–65131 (32 proc spells: DD, lifetap, heal, mana-on-cast × 8 level tiers)
> - NPCs: 181200–181202 (Augment Weaponsmith, Essence Provisioner, Essence Artificer)
> - Spawngroups: 3290000–3290002, Spawn2: 3270000–3270002
> - Merchant IDs: 181200–181202 (match NPC IDs)
>
> **Vendor Layout (all in Bazaar, zone ID 18):**
> - Augment Weaponsmith (181200): Sells leveling weapon augs for platinum. 27 items
>   across 9 tiers (lv1/10/20/30/40/50/60/65/70), 3 types each (Combat/Lifetap/Mana Stone).
> - Essence Provisioner (181201): Sells endgame stat augs for Common Essence
>   (alt_currency_id=100). 7 stat aug types.
> - Essence Artificer (181202): Sells endgame proc augs for Rare Essence
>   (alt_currency_id=101). 5 proc aug types.
>
> **Aug Types:**
> - Weapon proc augs: `augtype=8` (WeaponGeneral), `itemtype=54`, `proceffect`
>   links to generated spell. Non-linear scaling: lv50=100, lv60=250, lv65=500, lv70=1000.
> - Stat augs: `augtype=1` (GeneralSingleStat), `itemtype=54`, heroic stats.
> - All augs: `classes=65535`, `races=65535`, `slots=2097150` (all equip slots).
>
> **Key Learnings for Step 9:**
> - Alt currency merchants require `npc_types.alt_currency_id` set to the currency ID
>   AND `npc_types.class=70` (AlternateCurrencyMerchant). Regular platinum merchants use class=41.
> - `merchantlist.faction_required` is `smallint(6)` — use -100 (default), not INT_MIN.
> - `spells_new` has only 32 specific field### columns (not contiguous 142–277).
> - `range` is a SQL reserved keyword — always backtick-quote column names in INSERT.

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

- [x] Forgemaster NPC + 4-slot combine container
- [x] Merge validation: 3 same type + same level
- [x] Catalyst validation (correct tier)
- [x] Consume inputs → produce next-level aug
- [x] Aug stat scaling per level (progression tables)
- [x] Merge Catalyst vendor items (4 tiers)
- [x] Separate item IDs per level (baked-in stats, no dynamic scaling needed)
- [x] Max level enforcement (Level 5)
- [x] Build passes, test all cases

### Design Decisions Made

- **Aug Level Tracking:** Separate item IDs per level (NOT custom_data). Each L1-L5 is a distinct DB item with baked-in stats. Avoids complex dynamic packet scaling.
- **Forge Implementation:** Item ID intercept in HandleCombine (same pattern as Salvage Satchel), not a world object.
- **Catalyst Vendors:** Split — Forgemaster sells CE/PP catalysts (Lesser + Standard), Artificer sells RE catalysts (Greater + Superior).
- **Leveling Aug Merging:** Full L1-5 for all leveling augs (135 items), matching endgame aug depth.

### Post-Implementation Notes

- **ID Scheme:** `base + family×5 + (level-1)` enables arithmetical decode in C++ without lookup tables.
  - Leveling augs: 200100-200234 (27 families × 5 levels)
  - Stat augs: 200300-200334 (7 families × 5 levels)
  - Proc augs: 200400-200424 (5 families × 5 levels)
  - Catalysts: 200500-200503, Forge container: 200510, Solvent: 200520
- **Combine intercept:** Uses `RuleI(ItemProgression, ForgemasterContainerItemID)` in `tradeskills.cpp` after the Salvage Satchel intercept block. Same OP_TradeSkillCombine reply pattern.
- **C++ files:** `augment_merge.cpp/.h` (merge logic), `augment_merge_data.h` (auto-generated ID constants + lookup helpers). The data header is regenerated by `tools/generate_augments.py`.
- **Generator:** `tools/generate_augments.py` produces both SQL and the C++ header. Outputs 52 spells, 201 items, 4 NPCs. Covers Steps 8 + 9 together.
- **Proc scaling:** Endgame proc spells have separate IDs per level with scaled damage/reuse. Leveling proc augs share the same spell across all merge levels (damage unchanged) — only procrate and tiny +DMG bonus increase.

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

- [x] Zone flavor augs added to loot tables (59 augs across Classic/Kunark/Velious via global_loot)
- [x] Named mob aug drop tables (11 multi-stat augs, 3 tier levels, rare=1 global_loot)
- [ ] Raid boss signature proc augs created *(deferred — boss augs skipped for now)*
- [ ] Rare Essence from salvaging boss items *(deferred)*
- [x] Daily endgame quest (500 CE + 20 RE) — task 600000
- [x] Weekly boss quest (2,000 CE + 150 RE) — task 600001
- [ ] Proc augs functional *(deferred with boss augs)*
- [x] Build passes, test all cases

### Post-Implementation Notes

**Completed 2026-03-06.** Scope narrowed to Classic through Velious (expansion ≤ 2) only.

**Zone flavor augs:** 59 unique zone-themed augments via `global_loot` table with zone-specific targeting. Drop chance 0.1–0.5% depending on zone difficulty. Zones that already had augment drops in their loot tables were skipped (41 zones). Each aug has a unique name, lore, and stat combination themed to the zone.

**Named mob augs:** 11 multi-stat augments across 3 level tiers (low: 1-30, mid: 20-50, high: 40+). Uses `global_loot` with `rare=1` flag to auto-target all named mobs. 5% base drop chance. Archetypes: Balance (all stats), Warlord/Champion (STR/STA/ATK), Arcanist/Archmage (INT/WIS/mana), Stalker/Predator (DEX/AGI/avoid), Healer/Archpriest (WIS/STA/heal).

**Daily/weekly quests:** Two repeatable kill tasks. Daily (task 600000): 25 kills, 24h cooldown, rewards 500 CE + 20 RE. Weekly (task 600001): 150 kills, 7d cooldown, rewards 2000 CE + 150 RE. Rewards handled by Lua script `quests/global/player_task_rewards.lua` via `EVENT_TASK_COMPLETE`.

**Boss augs deferred:** Raid-tier boss proc augs and Rare Essence from boss item salvage were explicitly deferred for future work.

**Files:**
- Generator: `tools/generate_zone_augs.py`
- SQL: `utils/sql/item_progression/step10_zone_augments_quests.sql`
- Lua: `quests/global/player_task_rewards.lua`
- ID ranges: Items 201000-201058 (zone), 201500-201510 (named); global_loot 100-172; loottable/lootdrop 210000-210072; tasks 600000-600001

---

## Step 11 — Augment Infusion + Transmutation

**Goal:** Two additional systems for aug investment: spending Essence to boost an aug, and converting unwanted augs back to Essence. Both integrate into normal gameplay via container-based interactions (no chat commands).

### What to Build

1. **Augment Infusion** — Infusion Pool container (2 slots)
   - Player places aug + Infusion Catalyst in the Infusion Pool, clicks Combine
   - Five catalyst tiers (I–V) purchased from the Augment Forgemaster vendor for CE
   - Costs: 250 / 500 / 1,000 / 2,000 / 4,000 CE per catalyst tier
   - Catalyst tier must match the next infusion level (e.g., Catalyst III for 3rd infusion)
   - Cap: merge_stat + infusion_stat ≤ Level 5 maximum
   - Infusion level stored in `custom_data` alongside stat bonus
2. **Augment Transmutation** — Salvage Satchel detects augments
   - Drop augs into the existing Salvage Satchel and click Combine
   - Augs are automatically recognized and transmuted using the level-based return table
   - L1: 125 CE, L2: 375 CE, L3: 1,000 CE + 25 RE, L4: 2,500 CE + 75 RE, L5: 6,000 CE + 200 RE
   - Non-mergeable augs (zone drops, named drops) return L1 value
   - Regular items still salvage normally alongside augs

### What to Test

| Test | Expected Result | Pass Criteria |
|---|---|---|
| Infusion Pool: L3 Stone of Might + Cat III | +10 → +11 Heroic STR, catalyst consumed | Stat increases |
| Infuse 5 times on L3 (Cat I through V) | +10 → +15 | Caps at merge+infusion ≤ 21 |
| Try wrong catalyst tier | "Wrong catalyst tier" message | Tier enforcement |
| Try to infuse L5 at +21 | "At maximum power" | Cap enforced |
| Salvage Satchel: L3 aug inside | +1,000 CE + 25 RE, aug destroyed | Transmute return |
| Salvage Satchel: mixed items + augs | Items salvaged normally, augs transmuted | Both paths work |
| Salvage Satchel: L1 vendor aug | +125 CE | Minimum recovery |

### Done When

- Players have a "bad luck protection" path via infusion (container + catalyst).
- Unwanted augs can be recycled through the Salvage Satchel.
- Cap system prevents exceeding Level 5 stats.
- No chat commands needed — everything through normal container interactions.

### Progress Checklist

- [x] Infusion Pool container (2-slot, Combine-button, ID 200530)
- [x] Infusion Catalysts I–V (IDs 200520–200524) sold by Forgemaster for CE
- [x] Infusion processor with cap enforcement (merge + infusion ≤ L5 max)
- [x] Infusion level in `custom_data` (`infuse_level` + `HEROIC_*` keys)
- [x] Salvage Satchel detects augments and applies transmute return table
- [x] Correct CE/RE returns per aug level (L1:125, L2:375, L3:1000+25RE, L4:2500+75RE, L5:6000+200RE)
- [x] HandleCombine wired for Infusion Pool (rule: InfusionPoolItemID)
- [x] Removed old `#infuse` and `#transmute` commands (command-free design)
- [x] Build passes, SQL applied

### Post-Implementation Notes

**Completed 2026-03-06. Revised to container-based design (no commands).**

**Augment Infusion (Infusion Pool):** 2-slot container (ID 200530). Slot 0: augment to infuse. Slot 1: Infusion Catalyst (tier I–V). On Combine, validates catalyst tier matches next infusion level, enforces cap (base_primary + infusion ≤ L5 max via AugMergeData lookup), consumes catalyst, applies +1 to primary heroic stat via custom_data (`infuse_level`, `infuse_HEROIC_*`, `HEROIC_*` keys for ApplyCustomStats). Hard cap: 5 infusions.

**Augment Transmutation (Salvage Satchel):** Augs placed in the existing Salvage Satchel are automatically detected (ItemType == Augmentation) and processed via the transmute return table instead of the iLevel-based salvage formula. Regular items still salvage normally. Mergeable augs return CE/RE based on their merge level; non-mergeable augs (zone/named drops) return L1 value (125 CE). Mixed satchel contents (items + augs) are handled in a single Combine.

**Items created:**
- Infusion Pool (200530) — 2-slot container, 1 CE from Forgemaster
- Infusion Catalyst I–V (200520–200524) — 250/500/1000/2000/4000 CE from Forgemaster

**Files created/modified:**
- `zone/augment_infusion.cpp` — Infusion Pool processor
- `zone/augment_infusion.h` — namespace declaration
- `zone/salvage_system.cpp` — Added aug transmutation detection
- `zone/tradeskills.cpp` — HandleCombine wiring for Infusion Pool
- `common/ruletypes.h` — Added `InfusionPoolItemID` rule
- `zone/CMakeLists.txt` — Added augment_infusion.cpp
- `utils/sql/item_progression/step11_infusion_pool_catalysts.sql` — Item + vendor SQL

**Removed:**
- `zone/gm_commands/infuse.cpp` — deleted (replaced by container)
- `zone/gm_commands/transmute.cpp` — deleted (folded into salvage)

---

## Step 12 — Client Polish (DLL + Server-Side)

**Goal:** Improve visual feedback and UX using capabilities that already exist in the DLL and server, without requiring XML UI layout changes.

### DLL Capability Audit (what we actually have)

The eq-core-dll (`dinput8.dll`) is a dinput8 proxy with MQ2 subsystems. Relevant hooks for item progression:

| Hook | What it can do | Source |
|---|---|---|
| `CItemDisplayWnd::UpdateStrings` | Append text to item tooltips (already adds DPS, spell details, lore) | MQ2ItemDisplay.cpp |
| `CItemDisplayWnd::SetSpell` | Inject color-coded spell effect breakdowns into tooltips | MQ2ItemDisplay.cpp |
| `HandleWorldMessage` (0x1338) | Parse EdgeStatLabel key-value pairs, cache for label rendering | eqgame.cpp |
| `CLabel::Draw` | Override any SIDL EQType label with custom text | MQ2Labels.cpp |
| `0x1337C0DE` item packet marker | Parse custom_data KV pairs from item packets (currently logs + strips) | eqgame.cpp |
| Server-side `SendSound()` | Already used on tier-up | power_slot_xp.cpp |
| Server-side `SpellEffect()` | Send spell visual on player (no cast, just graphics) | mob.cpp |
| Server-side `Message()` | Already sends colored tier-up + XP milestone messages | power_slot_xp.cpp |

### What is NOT doable (requires XML UI layouts we don't have)

These are punted — they would need custom `.xml` UI definition files injected or modified on the client, which is a separate project:

- ~~Right-click "Feed to Power Slot"~~ — needs custom context menu XML
- ~~Grayed-out aug slot rendering~~ — needs custom item display window XML changes

### Possible but unproven — custom UI rendering paths

The DLL has two rendering mechanisms that *appear* capable of custom UI, but neither has been validated end-to-end in our environment. Before committing to building features on top of them, we need a proof-of-concept for each.

#### Path A: SIDL Custom Window (`CCustomWnd`)

The DLL's MQ2 layer can create real EQ UI windows from XML templates. There is a DPS window (`CDPSAdvWnd` in MQ2AdvDps.cpp) that constructs a `CCustomWnd("DPSAdvWnd")` and loads `EQUI_DPSAdvWnd.xml` via `AddXMLFile()`. It has tabs, lists, combos, checkboxes — a full native EQ window.

**If this works**, we could build: XP progress display, Essence HUD, Salvage All button — real interactive windows.

**What we don't know:**
- Is `CDPSAdvWnd` actually compiled into the DLL? (It's in source but may not be in the .vcxproj)
- Does `AddXMLFile()` actually load at runtime? Where does it look for the XML?
- Does the DPS window actually appear in-game right now, or is it dead code?
- What happens if the XML file is missing — crash, silent fail, or error?

#### Path B: HUD Text Overlay (`DrawHUDText`)

The DLL has `DrawHUDText()` in MQ2CleanUI.cpp (uses EQ's `CTextureFont::DrawWrappedText`) and a custom HUD in MQ2HUD.cpp that draws HP/Mana/End bars with ASCII block characters. A `/customhud` command toggles it.

**If this works**, we could build: simple XP progress text, tier label, Essence balance — text-only overlay at fixed screen positions.

**What we don't know:**
- The `DrawNetStatus` detour that powers the HUD draw pipeline is **commented out** (MQ2CleanUI.cpp line 163). Is the HUD reachable via any other code path?
- Does uncommenting the detour cause crashes or visual glitches?
- Is the font rendering stable across window resizes and fullscreen/windowed mode?

### POC Tasks (do these first, before building features)

#### POC-1: SIDL Window Smoke Test

**Goal:** Determine if the DPS window actually works, and if we can create a minimal custom window.

**Steps:**
1. Check if `MQ2AdvDps.cpp` is compiled (`.vcxproj` inclusion, or look for `CDPSAdvWnd` symbols in the built DLL)
2. Check if `EQUI_DPSAdvWnd.xml` exists anywhere in the client directory or DLL resources
3. If the DPS window IS compiled: launch client, look for it (may have a `/dps` command or auto-open). Document what happens.
4. If it works: create a minimal test — `EQUI_TestPOCWnd.xml` with a single text label, and a tiny `CCustomWnd` subclass that sets the label text to "Hello World"
5. If it doesn't work: document why (missing XML, crash, not compiled, etc.)

**Pass criteria:** A custom window visibly appears in-game with our text in it.
**Fail criteria:** Crashes, XML not loadable, or `CCustomWnd` infrastructure is broken/incomplete.

#### POC-2: HUD Text Overlay Smoke Test

**Goal:** Determine if the HUD text drawing path works when re-enabled.

**Steps:**
1. Uncomment the `DrawNetStatus` detour in MQ2CleanUI.cpp line 163
2. Add a simple test line in `DrawCustomHUD()`: `DrawHUDText("POC: HUD Active", 400, 50, 0xFFFFFF00, 2);`
3. Build DLL, launch client, enter game
4. Run `/customhud` to toggle, observe screen
5. Document: does text appear? Is it stable? Does it survive zone changes?

**Pass criteria:** Yellow "POC: HUD Active" text visible on screen, stable across zones.
**Fail criteria:** Crash on detour install, text not visible, or rendering glitches.

#### POC Decision Gate

After both POCs:

| Outcome | Path forward |
|---|---|
| Both work | Use SIDL windows for interactive UI (XP bar, Essence HUD), HUD for simple always-on status text |
| Only SIDL works | Use SIDL windows for everything |
| Only HUD works | Use HUD for text-based XP/tier/Essence display |
| Neither works | Stick with 12A-12D (server-side + tooltip only) |

Update Step 12 feature list based on which paths are actually available.

### What to Build (confirmed doable today)

#### 12A — Tier-Up Spell Visual (server-side only, no DLL)

Add a spell animation on tier-up so the player sees a visual flash, not just a chat message.

**Implementation:** In `DoTierUp()` (power_slot_xp.cpp), after `SendSound()`, add:
```cpp
c->SpellEffect(SPELL_VISUAL_ID, 10);  // visual effect, duration ~1 sec
```

Pick an appropriate existing spell visual (e.g. 43 = shimmer/glow, 44 = fire, 45 = ice). Tier could vary the effect: Enchanted = blue shimmer, Legendary = gold flash, Mythic = fire burst.

**Test:** Trigger a tier-up → visual effect plays on player character.

#### 12B — Item Tooltip: Tier + XP Progress (DLL change)

Modify `UpdateStrings_Detour` in MQ2ItemDisplay.cpp to append tier and XP info when an item has `dynamic_level` or tier custom data. Requires:

1. **Server:** Enable rule `Items:SendCustomItemStatsToClient = true`
2. **DLL:** In the `0x1337C0DE` parser, instead of just logging + stripping, cache the parsed KV pairs in a map keyed by item ID (or slot)
3. **DLL:** In `UpdateStrings_Detour`, if cached custom data exists for the displayed item, append:
   - Tier name (e.g. "Tier: Enchanted") in the tier's color
   - XP progress if `Exp` key exists (e.g. "Item XP: 450 / 1000 (45%)")
   - Dynamic level if present (e.g. "Effective Level: 35")

**Complexity:** Medium. The packet parsing already works. Need to: (a) cache instead of discard, (b) look up cached data during tooltip render, (c) format the display strings.

**Test:** Inspect a Power Slot item → tooltip shows tier name + XP progress. Inspect a scaled drop → tooltip shows effective level.

#### 12C — EdgeStatLabel: Power Slot Status (server + DLL)

Send Power Slot XP and tier via the existing EdgeStatLabel opcode (0x1338) so the DLL can cache it for label rendering.

1. **Server:** After XP gain or tier-up, send EdgeStatLabel with new keys:
   - Key 50 = Power Slot current XP
   - Key 51 = Power Slot XP threshold (next tier)
   - Key 52 = Power Slot current tier
2. **DLL:** Parse keys 50-52 in `HandleWorldMessage_Detour`, cache values
3. **DLL:** Register custom EQType handlers (≥1000) in MQ2Labels so any UI label with a matching EQType shows the data

This only renders if someone creates a custom XML label element with those EQTypes. But the data pipeline would be live, making future XML work trivial.

**Complexity:** Low-Medium. Both ends of the pipeline exist. Just new key IDs.

**Test:** Server sends stat update → DLL logs the values → verify in `dinput8_debug.log`.

#### 12D — Tier-Up Chat Color Enhancement (server-side only)

The tier-up message already uses `GetTierChatColor()`. Verify the color mapping is distinctive:
- Base → white
- Enchanted → blue (Chat::Skills or similar)
- Legendary → yellow/gold
- Mythic → orange/red

Also add the tier name as a prefix to item link messages when players inspect or link items.

**Test:** Trigger tier-ups at each level → messages use distinct, readable colors.

### What to Test

| ID | Test | Expected Result | Pass Criteria |
|---|---|---|---|
| 12A | Tier-up occurs | Spell visual plays on character | Visual flash visible |
| 12A | Different tiers | Different visual per tier | Enchanted ≠ Legendary ≠ Mythic |
| 12B | Inspect Power Slot item | Tooltip shows tier + XP progress | Text appended correctly |
| 12B | Inspect scaled drop | Tooltip shows effective level | dynamic_level displayed |
| 12C | Kill a mob | DLL debug log shows keys 50-52 | Values match server state |
| 12D | Tier-up at each level | Chat message color is distinctive | Not all white |

### Done When

- Tier-ups have a visible spell effect (not just chat text)
- Inspecting a Power Slot item shows tier and XP progress in the tooltip
- The EdgeStatLabel pipeline carries item progression data (ready for future XML)
- No aspirational features are listed that we can't actually build

### Progress Checklist

- [x] **POC-1: SIDL window smoke test** — ✅ PASSED. `CPowerSlotWnd` (CCustomWnd subclass) compiles successfully. Full window: labels, gauge, list, buttons. See `game_design/multiclass/POWER_SLOT_POC.md` for deployment details.
- [x] **POC-2: HUD text overlay smoke test** — ✅ PASSED. DrawNetStatus detour uncommented; HUD draw path is now enabled.
- [x] **POC Decision Gate** — Both paths work. SIDL windows for interactive UI, HUD for simple text overlay.
- [ ] 12A: Tier-up spell visual effects (server-side, per-tier)
- [ ] 12B: Item tooltip tier + XP display (DLL: cache custom data, render in UpdateStrings)
- [x] 12C: EdgeStatLabel Power Slot keys (server sends keys 300-343, DLL caches + displays in CPowerSlotWnd)
- [ ] 12D: Tier-up chat color verification/tuning
- [ ] 12E: (conditional) Custom UI features based on POC results — **UNLOCKED** (both POCs passed)
- [x] Build EQ Core DLL — compiles 0 errors, 5 pre-existing warnings
- [ ] Build zone server, test tier-up visuals
- [ ] In-game validation of Power Slot window (pending client XML deployment)

### Post-Implementation Notes

> **POC Results (March 2026):**
>
> **POC-1 (SIDL Window): PASSED.** Built `CPowerSlotWnd` — a full CCustomWnd subclass with:
> - Labels (title, status, info), gauge bar, CListWnd (4-column, per-row coloring), 3 buttons
> - Lifecycle management (CleanUI/ReloadUI/SetGameState), auto-refresh every 2 seconds
> - `/powerslots` slash command with show/refresh/debug subcommands
> - Compiled into DLL with 0 errors
>
> **POC-2 (HUD Text Overlay): PASSED.** DrawNetStatus detour uncommented, draw pipeline enabled.
>
> **12C (EdgeStatLabel Power Slot Keys): DONE.** Server sends keys 300-343 covering:
> - Slot count, focus tier/XP/XPMax, per-slot tier/XP/XPMax/itemID for 10 slots
> - DLL reads cache and populates window widgets in real-time
>
> **Decision: Both paths work.** SIDL windows are the primary mechanism for interactive
> UI (gear management, progression dashboards). HUD overlay available for simple
> always-on status text. All 12E sub-tasks are now unlocked.
>
> **Deployment:** See `game_design/multiclass/POWER_SLOT_POC.md` for full instructions.
>
> **Key Files Created/Modified:**
> - `extras/eq-core-dll-main/src/PowerSlotWnd.h/.cpp` — POC window
> - `extras/eq-core-dll-main/uifiles/EQUI_PowerSlotWnd.xml` — SIDL template
> - `extras/eq-core-dll-main/src/eqgame.cpp` — EdgeStat arrays non-static
> - `extras/eq-core-dll-main/src/MQ2CleanUI.cpp` — lifecycle + HUD enabled
> - `extras/eq-core-dll-main/src/MQ2Pulse.cpp` — Heartbeat hooks
> - `extras/eq-core-dll-main/src/MQ2CommandAPI.cpp` — /powerslots command
> - `zone/client.cpp` — SendEdgeStats() expanded with keys 300-343

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
Step 6 ─── Ghost Copy (PS → Equipment) ─────┤    │  │                      │
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
| 12. DLL Polish | Medium — grounded in existing hooks | 3 | 33 |

**Playable milestone after Step 7:** The core loop is complete — kill mobs → earn XP → tier
up items → salvage drops → spend Essence → tier up faster. ~18 days.

**Full Phase 1 (Steps 1-11):** ~30 days. Everything works via commands and NPCs.

**Full Phase 2 (+ Step 12):** ~33 days. Polished with DLL integration where feasible.

---

## Quick Reference: Key Files to Create/Modify

| File | Step | Purpose |
|---|---|---|
| `common/item_ilevel.h/.cpp` | 1 | iLevel calculation engine |
| `common/item_tier.h/.cpp` | 2 | Tier scaling formulas + override system |
| `zone/power_slot_xp.h/.cpp` | 3 | Power Slot XP tracking + tier-up logic |
| `zone/salvage.h` / `zone/salvage_system.cpp` | 4 | Salvage Satchel + Essence yield |
| `zone/gm_commands/itemtier.cpp` | 2,7 | `#itemtier` command |
| `zone/gm_commands/powerslot.cpp` | 3 | `#powerslot` command |
| `zone/gm_commands/salvage.cpp` | 4-5 | `#salvage` command |
| DB: `alternate_currency` | 4 | Common + Rare Essence currency definitions |
| DB: `items` | 1,8,10 | `calculated_ilevel` column, aug items, zone drop augs |
| DB: `item_scaling_overrides` | 2 | Manual stat overrides table |
| DB: `loot_drop_entries` | 10 | Zone/boss aug drop rates |

---

*End of Implementation Steps v1.1.*
