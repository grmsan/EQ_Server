# Item Progression & Augment System

**Version:** 0.3
**Date:** March 1, 2026
**Status:** Design Proposal — 3-Tier Items + Augment Merge Economy

---

## Table of Contents

1. [Design Vision](#1-design-vision)
2. [System Overview](#2-system-overview)
3. [Item Tiers](#3-item-tiers)
4. [Power Slot & Leveling](#4-power-slot--leveling)
5. [Feeding System](#5-feeding-system)
6. [Drop Tier Chances](#6-drop-tier-chances)
7. [Augment System](#7-augment-system)
8. [Augment Merge Economy](#8-augment-merge-economy)
9. [Salvage & Essence Economy](#9-salvage--essence-economy)
10. [Item Examples](#10-item-examples)
11. [Tuning Knobs](#11-tuning-knobs)
12. [Manual Override & Regeneration System](#12-manual-override--regeneration-system)

---

## 1. Design Vision

**Solo-focused server.** All content is designed to be completed solo. Mobs that were
originally group or raid encounters exist as difficulty tiers — challenges the player
grows into over time. There is no group or raid content in the traditional sense.

**Items: few tiers, big transformations, fast satisfaction.**
Base → Enchanted → Legendary → Mythic. Each transition is dramatic and *felt*.

**Augments: infinite depth, merge economy, endgame chase.**
The long-term progression lives in the augment system. Augs come from drops, salvage,
crafting, and quests. The 3:1 merge system provides infinite theoretical depth with
exponential cost that naturally self-limits.

**Every drop has purpose.**
Feed it to your Power Slot item, equip it, or toss it in the Salvage Satchel. One decision,
no junk.

### Design Principles

1. **Moments over increments.** Every tier transition should be screenshot-worthy.
2. **Identity preserved.** Hategiver is always Hategiver. Items don't become stat soup.
3. **Clear when "done."** Legendary is the natural endpoint. Mythic is a luxury.
4. **Infinite chase in the right place.** Augment merging is the long-term depth layer.
5. **Respect the player's time.** Bulk salvage, fast leveling, meaningful choices.
6. **Wheels we can turn.** Every rate, ratio, and threshold is a tunable knob.

---

## 2. System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     THE ITEM LIFECYCLE                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Mob drops item ──┬── Equip it (if upgrade)                 │
│                   ├── Feed to Power Slot item (XP boost)    │
│                   └── Salvage Satchel → Essence             │
│                                                             │
│  Power Slot: one item at a time                             │
│  ├── Stats project if native slot is empty (no penalty)     │
│  ├── Passive XP from kills                                  │
│  ├── Active XP from feeding drops                           │
│  └── Base → Enchanted → Legendary (XP scales w/ mob level) │
│       → Mythic (optional, adds aug slot, no stats)          │
│                                                             │
│  Aug System (max level per expansion):                      │
│  ├── Augs from: drops, salvage→essence, crafting, quests    │
│  ├── Socket into any tiered item (slots scale with tier)    │
│  └── Merge 3 same-type → 1 higher level (5 levels max)     │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 3. Item Tiers

### The Four Tiers

| Tier | Color | Name Example | Aug Slots | Time in Power Slot | What Changes |
|---|---|---|---|---|---|
| **Base** | White | Hategiver | 1 | — | Item as it drops from the database |
| **Enchanted** | Blue | Enchanted Hategiver | 2 | ~25 min† | ~2× raw stats, spell damage and attack stats appear|
| **Legendary** | Gold | Legendary Hategiver | 3 | ~8 hrs† | Combat stats +30%, heroic stats appear, effects enhanced |
| **Mythic** | Orange | Mythic Hategiver | 4 | ~63 hrs† | +1 aug slot, no stat increase|

> † Power Slot times at white-con mobs, 1 kill/min. Same at every level — see §4.

> **2H weapons and bows** get double aug slots (2/4/5/6) to compensate for losing the
> dual-wield slot.

### What Changes at Each Tier

**Base → Enchanted (~2× stats)**

- All raw stats approximately double (weapon damage, AC, HP/Mana, attributes)
- Haste: +3% additive
- **Spell Power** appears if item has INT (Spell Power = base INT)
- **Heal Power** appears if item has WIS (Heal Power = base WIS)
- Procs: increase proc rate
- Name turns **blue**

**Enchanted → Legendary (combat stat bump + heroics + secondary stats)**

- **Combat stats increase by 30%** over Enchanted — Damage, AC, HP, Mana, Endurance go from ×2.0 to ×2.6
- Attributes (STR, STA, etc.) stay at ×2 — their power bump comes from heroics
- **Heroic stats appear** — each heroic equals the item's base stat (1:1)
- **Spell Power / Heal Power** double (= base INT × 2 / base WIS × 2)
- **Attack** added (= base damage × 2)
- Other combat effects (Shielding, Strikethrough) increase by small amounts (+1)
- Proc rate improved
- Name turns **gold**, +1 aug slot (total 3)

**Legendary → Mythic (aug slot only)**

- **No stat increase** — all stats identical to Legendary
- +1 augment slot
- Name turns **orange**

### Stat Scaling Formulas

```
Enchanted_stat  = base_stat × 2.0

Legendary combat stats (DMG, AC, HP, Mana, End):
  Legendary_stat  = Enchanted_stat × 1.3        (= base_stat × 2.6)

Legendary attributes (STR, STA, AGI, DEX, WIS, INT, CHA):
  Legendary_stat  = Enchanted_stat              (= base_stat × 2.0, no raw increase)

Mythic_stat     = Legendary_stat                (no increase)

Heroic stats (Legendary+):
  heroic_stat   = base_stat                     (1:1 with base)
  Example: 30 base STR → 30 Heroic STR

Spell Power (Enchanted+, items with INT):
  Enchanted     = base_INT
  Legendary     = base_INT × 2
  Example: 15 base INT → 15 Spell Power (Ench), 30 Spell Power (Leg)

Heal Power (Enchanted+, items with WIS):
  Enchanted     = base_WIS
  Legendary     = base_WIS × 2
  Example: 20 base WIS → 20 Heal Power (Ench), 40 Heal Power (Leg)

Attack (Legendary+):
  attack_bonus  = base_damage × 2.0
  Example: 48 base DMG → +96 Attack
```

Expansion-relative scaling is built in — multipliers operate on base stats, so a PoP weapon
(48 base DMG) naturally scales higher than a Classic weapon (15 base DMG). Item hierarchy
is preserved automatically.

### Haste Scaling

Haste uses additive bonuses with a worn cap of 100%:

```
Enchanted_haste = base_haste + 3%
Legendary_haste = base_haste + 5%
Mythic_haste    = Legendary_haste               (no increase)
```

A 36% haste item becomes 39% Enchanted, 41% Legendary.

---

## 4. Power Slot & Leveling

### How It Works

The Power Slot repurposes the EQ Power Source equipment slot for item leveling. It is the
**only** way to level items through tiers.

1. Place any equippable item into the Power Slot.
2. Kill mobs to earn XP for the Power Slot item.
3. Item progresses: Base → Enchanted → Legendary → (optional) Mythic.
4. Remove from Power Slot and equip when satisfied.

**One item at a time.** This creates a meaningful choice: "Which item do I level next?"

### Stat Projection & Ghost Items

An item in the Power Slot grants its stats to the player. To solve client UI slot validation (like preventing a shield from being equipped while you project a 2H weapon), the server spawns a **Ghost Duplicate** in the item's native equipment slot if it is empty.

- **Native slot empty →** Server places a non-tradeable "Ghost" duplicate of the Power Slot item into the native slot, granting its full stats (AC, HP, attributes, effects).
- **Native slot occupied →** Power Slot item does **not** project stats; XP gain continues normally.
- **Unequipping the Ghost →** Attempting to pick up, unequip, or swap the ghost item will automatically unequip the actual Power Slot item from the Power Slot.
- No stacking — you never gain stats from both the Power Slot and an equipped item in the same slot.

**Example:** Place Hategiver in Power Slot, primary hand empty → the server spawns a Ghost Hategiver in your primary hand. You fight with its stats while it levels. If you swap it for a different weapon, the Power Slot Hategiver stops projecting stats but continues earning XP.

### XP Formula

Item XP is a flat amount per kill, modified only by con color and source type.
A level 10 player fighting white-con gnolls progresses gear at the same rate as a
level 60 player fighting white-con giants. **Con color is what matters, not mob level.**

Grey mobs give **zero** item XP — just like character XP.

```
item_xp = BASE_ITEM_XP × CON_MULTIPLIER × SOURCE_MULTIPLIER
```

Default `BASE_ITEM_XP` = 40. A single white-con kill gives 40 item XP regardless of
player or mob level. This keeps item progression in lockstep with character progression
— players naturally level gear while leveling up, and don't need to reach endgame before
the system feels rewarding.

### Con Color Multipliers

| Con Color | Level Relative | Multiplier | XP per Kill | Rationale |
|---|---|---|---|---|
| Grey | Trivial | **0** | 0 | No item XP (matches game — trivial = no reward) |
| Green | Easy | 0.125 | 5 | Barely worth it — move on to harder content |
| Light Blue | Below level | 0.375 | 15 | Below level but not trivial |
| Blue | Slightly below | 0.75 | 30 | Close to even, slightly easier |
| White | Even con | 1.00 | 40 | Baseline |
| Yellow | +1 to +2 above | 1.125 | 45 | Challenging — rewarded |
| Red | +3 or more | 1.25 | 50 | Dangerous — big payoff |

### XP Thresholds

| Transition | XP Required |
|---|---|
| Base → Enchanted | 1,000 |
| Enchanted → Legendary | 20,000 |
| Legendary → Mythic | 150,000 |
| **Total Base → Mythic** | **171,000** |

### Example Timelines (1 kill/min, white-con normal mobs)

These times are the same at **every player level** — a level 10 and a level 60 progress
identically when fighting appropriately challenging (white-con) content.

| Transition | XP Required | Time (white) | Time (yellow) | Time (red) |
|---|---|---|---|---|
| Base → Enchanted | 1,000 | ~25 min | ~22 min | ~20 min |
| Enchanted → Legendary | 20,000 | ~8.3 hrs | ~7.4 hrs | ~6.7 hrs |
| Legendary → Mythic | 150,000 | ~62.5 hrs | ~55.6 hrs | ~50 hrs |
| **Total Base → Mythic** | **171,000** | **~71 hrs** | **~63 hrs** | **~57 hrs** |

**Enchanted** is achievable in a single session at any level. **Legendary** is a multi-day
project for a keeper item. **Mythic** is a long-term commitment for your best gear.

**Fighting challenging mobs accelerates progression.** Grinding yellow/red-con content
cuts tier-up times significantly — the system rewards playing at your edge.

### XP Sources

| Source | XP Amount | Notes |
|---|---|---|
| Kill mob | `BASE_ITEM_XP × CON_MULT` | Primary passive source (40 at white con) |
| Kill named mob | Kill XP × 3.0 | Named mobs = juicy XP |
| Kill raid-tier mob | Kill XP × 5.0 | Raid-tier bosses = big chunks |
| Feed exact duplicate | 20% of remaining XP | See §5 |
| Feed essence | Variable | See §5 |
| Quest completion | Quest-specific | Some quests give Power Slot XP |

---

## 5. Feeding System

The Feeding system provides the active XP boost alongside passive kill XP.

**1. Direct Feeding (Exact Duplicates Only)**
Only true duplicates (the exact same item, same tier or higher) can be directly fed to the Power Slot item. Doing so destroys the item and grants a massive percentage of the required XP (e.g., 20% of the tier requirement). Getting a duplicate of your Power Slot item is the best possible feed — flipping the "I already have one" feeling into excitement.

**2. Essence Tier-Up (Other Items)**
Different items cannot be fed directly into an item for XP. To prevent high-level players
from farming trivial low-level zones for item fodder, all non-duplicate loot must be
processed through the **Salvage Satchel** into Essence. That Essence can then be spent to
instantly tier up an item — the cost scales with the item's iLevel (see
[ITEM_LEVEL_SYSTEM.md](ITEM_LEVEL_SYSTEM.md) §Tier Transition Costs).

- Essence yields scale with the sacrificed item's inherent power via the iLevel formula.
  Only magic items can be salvaged; non-magic vendor trash is rejected. An offset of 100
  is subtracted from iLevel, creating a smooth ramp (see ITEM_LEVEL_SYSTEM.md).
- Tier-up Essence costs scale with iLevel²: a Rusty Sword (iLevel 14) costs 3 Essence
  for Base→Enchanted; an Anguish weapon (iLevel 1,106) costs ~40k for Ench→Legendary.
- This ensures low-level farming is mathematically worthless for leveling high-end gear.

---

## 6. Drop Tier Chances

Items can drop at higher tiers, creating jackpot moments:

| Drop Source | Base | Enchanted | Legendary | Mythic |
|---|---|---|---|---|
| Standard mob (all tiers) | 80% | 15% | 4% | 1% |
| Hard-mode boss (custom) | 0% | 60% | 35% | 5% |

All mobs — normal trash, named, and raid-tier bosses — use the same base drop ratios.
Bosses already reward players by dropping the best base loot in their zone; no additional
drop rate incentive is needed. Hard-mode bosses are a reserved row for future custom
encounters with intentionally skewed tier distribution.

Pre-tiered drops arrive with full stat multipliers and aug slots — a Legendary drop is
immediately usable endgame gear regardless of which mob dropped it.

> **Deferred:** Raid-zone trash mob engagement is a future design consideration. Possible
> directions: chance to drop zone-boss loot or augs, skill cooldown reduction on kill,
> or stackable zone-specific buffs. Nothing to implement now — just keep it in mind.

### Quest Rewards

- **Short quests:** Base item
- **Multi-step quests:** Enchanted item
- **Epic / long chains:** Legendary item
- **Server-first / extreme challenges:** Mythic item

---

## 7. Augment System

Augments are the **infinite progression layer**. They provide depth, customization, and
long-term chase without touching item identity.

### When Augs Unlock

Simple vendor weapon augs and ultra-rare zone flavor drops are available **from level 1**.
The full chase system — Essence economy, merge system, premium drops — unlocks at
**max expansion level**. See AUGMENT_SYSTEM.md for the complete augment design.

### Aug Slot Availability

| Item Tier | Aug Slots (1H/Armor) | Aug Slots (2H/Bow) |
|---|---|---|
| Base | 1 | 2 |
| Enchanted | 2 | 4 |
| Legendary | 3 | 5 |
| Mythic | 4 | 6 |

### Aug Types

| Category | Stat Focus | Example | Rarity |
|---|---|---|---|
| **Offensive** | Heroic STR, Attack, Heroic DEX | +4 Heroic STR, +12 Attack | Common |
| **Defensive** | Heroic STA, AC, Shielding | +3 Heroic STA, +8 AC | Common |
| **Utility** | Heroic AGI, Haste, Regen | +3 Heroic AGI, +2 Regen | Common |
| **Caster** | Heroic INT/WIS, Spell DMG, Mana Regen | +4 Heroic WIS, +15 Spell DMG | Common |
| **Combat Proc** | Proc on melee hit | Fire DD proc (150 dmg) | Rare |
| **Defensive Proc** | Shield proc or damage ward | Absorb 200 dmg ward | Rare |
| **Focus Effect** | Spell focus, heal crit | +5% Heal Crit | Rare |
| **Class-Specific** | Class ability enhancement | +10% Backstab DMG (ROG) | Rare |

### Aug Sources

| Source | What You Get |
|---|---|
| **Mob drops** | Random stat augs, scales with zone difficulty |
| **Salvage → Essence → Vendor** | Choose specific aug templates |
| **Crafting** | Specific stat augs with controlled stats |
| **Quests** | Named augs, sometimes pre-leveled |
| **Named mobs** | Higher chance of rare aug types |
| **Raid-tier bosses** | Rare proc/focus/class augs, higher base level |

---

## 8. Augment Merge Economy

### 3:1 Merge System

Combine 3 augments of the same type and level → 1 augment of the next level. Max level: 5.

```
3× Level 1 Heroic DEX → 1× Level 2 Heroic DEX
3× Level 2 Heroic DEX → 1× Level 3 Heroic DEX
3× Level 3 Heroic DEX → 1× Level 4 Heroic DEX
3× Level 4 Heroic DEX → 1× Level 5 Heroic DEX (MAX)
```

### Stat Progression Per Level

| Aug Level | Stat Augs (e.g., Heroic DEX) | Combat Proc (Fire DD) | Total Augs Invested |
|---|---|---|---|
| 1 | +3 | 100 DD, 8s reuse | 1 |
| 2 | +6 | 175 DD, 7s reuse | 3 |
| 3 | +10 | 275 DD, 6s reuse | 9 |
| 4 | +15 | 400 DD, 5s reuse | 27 |
| 5 (MAX) | +21 | 550 DD, 4s reuse | 81 |

81 augs for one max-level slot. Across 4 Mythic slots on a 1H weapon, that's 324 augs per
item (486 for a 2H/bow with 6 slots). Level 3 is the natural "sweet spot" where most
players plateau; Level 5 is for the dedicated.

The efficiency curve is steep — each level costs 3× more augs for diminishing stat returns.
This naturally self-limits without hard gates.

Merging is done via an **Augment Forgemaster** NPC in hub cities using a 4-slot container
(3 augs + 1 Merge Catalyst reagent). See AUGMENT_SYSTEM.md §6 for full details.

---

## 9. Salvage & Essence Economy

### Salvage Satchel

A permanent container for bulk salvaging. Drag items in, click "Salvage All," receive
Essence. One click handles a full session's worth of loot.

### Essence Types

| Essence | Source | Primary Use |
|---|---|---|
| **Common** | Salvaging any item | Buy stat aug templates |
| **Rare** | Salvaging named/raid-tier items, or rare proc on salvage | Buy rare aug templates, catalysts |

### Salvage Values

Essence yields are based on the item's **iLevel** (an uncapped power score derived from stats).
See [ITEM_LEVEL_SYSTEM.md](ITEM_LEVEL_SYSTEM.md) for the complete iLevel algorithm and
worked examples.

**Salvage requires two conditions:**

1. **Magic gate:** Only items with the `magic` flag can be salvaged. Non-magic vendor
   trash (Rusty weapons, basic armor) is rejected entirely. This is tunable per-item in DB.
2. **Essence offset:** `Essence = max(1, iLevel - 100) × tier_bonus × era_multiplier`

The offset creates a smooth ramp — low-magic items yield 1–5 Essence, mid-tier grows
gradually, and endgame items barely notice the 100-point deduction. No cliff, no exploit.

- Tier bonuses: Base ×1.0, Enchanted ×1.15, Legendary ×1.35, Mythic (unsalvageable)
- Era multiplier: Server-configurable per expansion (default 1.0). See §11.

**Representative yields (Base tier, era_multiplier = 1.0):**

| iLevel | Example Item | Magic? | Common Essence |
|---|---|---|---|
| 14 | Rusty Sword | No | — (unsalvageable) |
| 77 | Lamentation (Classic) | Yes | 1 |
| 269 | Hategiver (Velious) | Yes | 169 |
| 645 | Blade of War (PoP) | Yes | 545 |
| 1,106 | Anguish avg (OoW) | Yes | 1,006 |

Higher-tier drops give a small bonus when salvaged (Enchanted +15%, Legendary +35%), but
keeping and leveling items in the Power Slot is always more valuable than salvaging them
for Essence. The tier bonus prevents the edge case of salvaging being worthless for pre-tiered
drops while keeping it clear that leveling > salvaging.

### Tier Transition Costs (iLevel² Power Curve)

Essence costs scale with the **square** of the item's iLevel — the gap between a Rusty
Sword and an Anguish weapon is enormous.  See [ITEM_LEVEL_SYSTEM.md](ITEM_LEVEL_SYSTEM.md)
for the complete formula, per-era worked examples, and C++ implementation.

**Formula:** `tier_cost = max(1, TIER_FLOOR + round(iLevel² × TIER_SCALE))`

| Transition | Floor | Scale | Rusty (14) | Vel Raid (310) | PoP Raid (735) | Anguish (1,106) |
|---|---|---|---|---|---|---|
| Base → Enchanted | 1 | 0.008 | **3** | 770 | 4,322 | **9,788** |
| Enchanted → Legendary | 25 | 0.033 | 31 | 3,197 | 17,854 | **40,387** |
| Legendary → Mythic | 1,500 | 0.121 | 1,524 | 13,124 | 66,856 | **149,478** |
| **Total** | | | 1,558 | 17,091 | 89,032 | 199,653 |

**Design target:** Roughly one to two Anguish raid clears (~29k Essence each) funds a
Legendary upgrade on an Anguish-tier item. The floor per tier ensures even trash items
pay a meaningful cost for the higher tiers (Mythic is expensive for everyone).

These costs are independent of Kill XP. Players can tier up through EITHER path:
grinding kills to reach the XP threshold (free), or spending Essence (instant).

Costs are tunable per era — see §11 Era Tier Cost Multipliers.

### Essence Vendors

**Common Essence — "The Provisioner"**

| Item | Cost |
|---|---|
| Level 1 Stat Aug Template (choose type) | 500 |
| Augment Solvent (removes aug safely) | 100 |

**Rare Essence — “The Artificer”**

| Item | Cost |
|---|---|
| Level 2 Stat Aug Template | 1,000 |
| Random Rare Aug Box (Level 1 proc/focus/class) | 3,000 |

> **Note:** Tier-up costs are no longer flat-rate vendor items. Players tier up directly
> via the Power Slot interface, paying the iLevel-scaled Essence cost. See
> [ITEM_LEVEL_SYSTEM.md](ITEM_LEVEL_SYSTEM.md) §Tier Transition Costs.

> **Note:** These are abbreviated vendor tables. See [AUGMENT_SYSTEM.md](AUGMENT_SYSTEM.md)
> §7.4 for complete vendor listings including proc aug templates, augment solvents, targeted
> rare aug boxes, and platinum pricing.

---

## 10. Item Examples

> All base stats are from the live database. Scaled stats use the formulas from §3:
> Enchanted ×2 all stats; Legendary ×2.6 combat (DMG/AC/HP/Mana/End), ×2 attributes,
> heroics = base stat 1:1; Spell Power = base INT (Ench) / ×2 (Leg); Heal Power = base
> WIS (Ench) / ×2 (Leg); Attack = base DMG ×2 (Leg); Haste +3% (Ench) / +5% (Leg).

### Rusty Long Sword (Classic Starter — Newbie Drop)

**ID:** 5019 | **Type:** 1HS | **Slot:** Primary/Secondary | **Classes:** WAR/PAL/RNG/SHD/BRD

| Stat | Base | Enchanted | Legendary | Mythic |
|---|---|---|---|---|
| Damage | 5 | 10 | 13 | 13 |
| Delay | 35 | 35 | 35 | 35 |
| Attack | — | — | +10 | +10 |
| Aug Slots | 1 | 2 | 3 | 4 |

### Lamentation (Classic 1HS — Permafrost/Nagafen)

**ID:** 5157 | **Type:** 1HS | **Slot:** Primary/Secondary | **Classes:** WAR/PAL/SHD

| Stat | Base | Enchanted (×2) | Legendary (×2.6 / ×2) | Mythic |
|---|---|---|---|---|
| Damage | 9 | 18 | 23 | 23 |
| Delay | 19 | 19 | 19 | 19 |
| HP | 20 | 40 | 52 | 52 |
| STR | 6 | 12 | 12 | 12 |
| STA | 6 | 12 | 12 | 12 |
| Heroic STR | — | — | +6 | +6 |
| Heroic STA | — | — | +6 | +6 |
| Attack | — | — | +18 | +18 |
| Aug Slots | 1 | 2 | 3 | 4 |

### Cloak of Flames (Classic Back — Lord Nagafen)

**ID:** 11621 | **Type:** Armor (Back) | **Slot:** Back | **Classes:** ALL

| Stat | Base | Enchanted (×2) | Legendary (×2.6 / ×2) | Mythic |
|---|---|---|---|---|
| AC | 10 | 20 | 26 | 26 |
| HP | 50 | 100 | 130 | 130 |
| AGI | 9 | 18 | 18 | 18 |
| DEX | 9 | 18 | 18 | 18 |
| Haste | 36% | 39% | 41% | 41% |
| Heroic AGI | — | — | +9 | +9 |
| Heroic DEX | — | — | +9 | +9 |
| Aug Slots | 1 | 2 | 3 | 4 |

### Robe of the Oracle (Classic Chest — Caster Robe)

**ID:** 1354 | **Type:** Armor (Chest) | **Slot:** Chest | **Classes:** NEC/WIZ/MAG/ENC

| Stat | Base | Enchanted (×2) | Legendary (×2.6 / ×2) | Mythic |
|---|---|---|---|---|
| AC | 9 | 18 | 23 | 23 |
| Mana | 25 | 50 | 65 | 65 |
| WIS | 5 | 10 | 10 | 10 |
| INT | 5 | 10 | 10 | 10 |
| Heroic WIS | — | — | +5 | +5 |
| Heroic INT | — | — | +5 | +5 |
| Spell Power | — | 5 | 10 | 10 |
| Heal Power | — | 5 | 10 | 10 |
| Aug Slots | 1 | 2 | 3 | 4 |

### Cobalt Breastplate (Velious Plate Armor — Crafted/Dropped)

**ID:** 4516 | **Type:** Armor (Chest) | **Slot:** Chest | **Classes:** WAR

| Stat | Base | Enchanted (×2) | Legendary (×2.6 / ×2) | Mythic |
|---|---|---|---|---|
| AC | 45 | 90 | 117 | 117 |
| HP | 50 | 100 | 130 | 130 |
| STR | 12 | 24 | 24 | 24 |
| STA | 12 | 24 | 24 | 24 |
| DEX | 12 | 24 | 24 | 24 |
| Heroic STR | — | — | +12 | +12 |
| Heroic STA | — | — | +12 | +12 |
| Heroic DEX | — | — | +12 | +12 |
| Aug Slots | 1 | 2 | 3 | 4 |

### Hategiver (Velious 1H Sword — Avatar of War)

Base: 15 DMG / 19 DLY, 25 AC, 85 HP, 10 STR / 10 AGI / 10 DEX / 15 STA. Proc: Enraging Blow.

| Stat | Base | Enchanted (×2) | Legendary (×2.6 / ×2) | Mythic |
|---|---|---|---|---|
| Damage | 15 | 30 | 39 | 39 |
| AC | 25 | 50 | 65 | 65 |
| HP | 85 | 170 | 221 | 221 |
| STR | 10 | 20 | 20 | 20 |
| STA | 15 | 30 | 30 | 30 |
| Heroic STR | — | — | +10 | +10 |
| Heroic STA | — | — | +15 | +15 |
| Attack | — | — | +30 | +30 |
| Aug Slots | 1 | 2 | 3 | 4 |

### Blade of War (PoP 2H Sword — Rallos Zek the Warlord)

Base: 48 DMG / 32 DLY, 50 AC, 175 HP, 175 Mana, 30 STR / 30 STA / 30 AGI / 15 WIS / 15 INT. Proc: Enraging Blow.

| Stat | Base | Enchanted (×2) | Legendary (×2.6 / ×2) | Mythic |
|---|---|---|---|---|
| Damage | 48 | 96 | 125 | 125 |
| AC | 50 | 100 | 130 | 130 |
| HP | 175 | 350 | 455 | 455 |
| Mana | 175 | 350 | 455 | 455 |
| STR | 30 | 60 | 60 | 60 |
| STA | 30 | 60 | 60 | 60 |
| WIS | 15 | 30 | 30 | 30 |
| INT | 15 | 30 | 30 | 30 |
| Heroic STR | — | — | +30 | +30 |
| Heroic STA | — | — | +30 | +30 |
| Spell Power | — | 15 | 30 | 30 |
| Heal Power | — | 15 | 30 | 30 |
| Attack | — | — | +96 | +96 |
| Aug Slots | 2 | 4 | 5 | 6 |

### Cross-Era Comparison (Legendary)

| Item | Era | Base DMG | Legendary DMG | Base HP | Legendary HP |
|---|---|---|---|---|---|
| Rusty Long Sword | Classic | 5 | 13 | 0 | 0 |
| Lamentation | Classic | 9 | 23 | 20 | 52 |
| Hategiver | Velious | 15 | 39 | 85 | 221 |
| Blade of War | PoP | 48 | 125 | 175 | 455 |

A Legendary Classic weapon (13 DMG) doesn't threaten a Base PoP weapon (48 DMG). Expansion
hierarchy is always preserved.

---

## 11. Tuning Knobs

Every gameplay value is independently adjustable.

### Item Tier Scaling

| Knob | Default | Effect |
|---|---|---|
| `ENCHANTED_MULTIPLIER` | 2.0 | Raw stat multiplier at Enchanted |
| `LEGENDARY_MULTIPLIER` | 2.0 | Attribute (STR/STA/etc.) multiplier at Legendary |
| `LEGENDARY_COMBAT_BONUS` | 1.3 | Combat stat (DMG/AC/HP/Mana/End) multiplier over Enchanted values |
| `HEROIC_COEFFICIENT` | 1.0 | Base stat → Heroic stat ratio (1:1) |
| `ATTACK_COEFFICIENT` | 2.0 | Base damage → Attack bonus |
| `SPELL_POWER_COEFFICIENT` | 1.0 | Base INT → Spell Power (Ench), ×2 at Leg |
| `HEAL_POWER_COEFFICIENT` | 1.0 | Base WIS → Heal Power (Ench), ×2 at Leg |
| `HASTE_ENCHANTED_BONUS` | +3% | Additive haste at Enchanted |
| `HASTE_LEGENDARY_BONUS` | +5% | Additive haste at Legendary |
| `HASTE_WORN_CAP` | 100% | Hard cap on worn haste |

### Power Slot

| Knob | Default | Effect |
|---|---|---|
| `XP_BASE_TO_ENCHANTED` | 1,000 | XP for Base → Enchanted |
| `XP_ENCHANTED_TO_LEGENDARY` | 20,000 | XP for Enchanted → Legendary |
| `XP_LEGENDARY_TO_MYTHIC` | 150,000 | XP for Legendary → Mythic |
| `BASE_ITEM_XP` | 40 | Flat XP per kill (before con/source multipliers) |
| `CON_MULT_GREY` | 0 | Grey con — no item XP |
| `CON_MULT_GREEN` | 0.125 | Green con multiplier |
| `CON_MULT_LIGHT_BLUE` | 0.375 | Light blue con multiplier |
| `CON_MULT_BLUE` | 0.75 | Blue con multiplier |
| `CON_MULT_WHITE` | 1.00 | White (even) con multiplier |
| `CON_MULT_YELLOW` | 1.125 | Yellow con multiplier |
| `CON_MULT_RED` | 1.25 | Red con multiplier |
| `NAMED_XP_MULT` | 3.0 | Source multiplier for named mobs |
| `RAID_TIER_XP_MULT` | 5.0 | Source multiplier for raid-tier mobs |
| `PROJECTION_XP_MULTIPLIER` | 1.0 | XP while projecting (1.0 = no penalty) |

### Feeding

| Knob | Default | Effect |
|---|---|---|
| `FEED_SAME_ITEM_PCT` | 20% | XP% for same item at same+ tier |
| `FEED_TIER_DECAY` | 0.25 | Multiplier per tier gap below |

### Drop Tiers

| Knob | Default |
|---|---|
| `DROP_TIER_STANDARD` | 80 / 15 / 4 / 1 |
| `DROP_TIER_HARDMODE` | 0 / 60 / 35 / 5 |

### Augments

| Knob | Default | Effect |
|---|---|---|
| `AUG_MERGE_RATIO` | 3 | Augs needed per merge |
| `AUG_MAX_LEVEL` | 5 | Maximum merge level |
| `AUG_STAT_BASE` | 3 | Stat value at Level 1 |

### Salvage Economy

| Knob | Default | Effect |
|---|---|---|
| `ESSENCE_PER_ILEVEL` | 1.0 | Essence-to-iLevel ratio (1.0 = 1:1) |
| `ESSENCE_OFFSET` | 100 | Subtracted from iLevel before Essence calculation |
| `SALVAGE_REQUIRE_MAGIC` | true | Only magic-flagged items can be salvaged |
| `SALVAGE_TIER_BONUS_ENCHANTED` | 1.15 | Tier bonus for salvaging Enchanted items (+15%) |
| `SALVAGE_TIER_BONUS_LEGENDARY` | 1.35 | Tier bonus for salvaging Legendary items (+35%) |
| `SALVAGE_RARE_CHANCE_NAMED` | 10% | Rare Essence chance from named drops |
| `TIER_FLOOR_BASE_TO_ENCHANTED` | 1 | Tier cost floor: added to iLevel² × scale |
| `TIER_FLOOR_ENCHANTED_TO_LEGENDARY` | 25 | Tier cost floor for Ench → Leg |
| `TIER_FLOOR_LEGENDARY_TO_MYTHIC` | 1,500 | Tier cost floor for Leg → Myth |
| `TIER_SCALE_BASE_TO_ENCHANTED` | 0.008 | Scale factor applied to iLevel² for B → E |
| `TIER_SCALE_ENCHANTED_TO_LEGENDARY` | 0.033 | Scale factor applied to iLevel² for E → L |
| `TIER_SCALE_LEGENDARY_TO_MYTHIC` | 0.121 | Scale factor applied to iLevel² for L → M |

### Per-Era Tuning Knobs (Progression Server)

The server runs as a progression server where expansions unlock sequentially. Each
era has independent multipliers so the economy can be tuned based on live player data.
All default to 1.0. Changes take effect on `#rules reload`.

#### Era Essence Multipliers

Multiply Essence yields for items from that era's zones/expansions.

| Knob | Default | Expansion IDs |
|---|---|---|
| `ERA_MULT_CLASSIC` | 1.0 | 0-1 (Classic, Kunark) |
| `ERA_MULT_VELIOUS` | 1.0 | 3 (Scars of Velious) |
| `ERA_MULT_LUCLIN` | 1.0 | 4 (Shadows of Luclin) |
| `ERA_MULT_POP` | 1.0 | 5 (Planes of Power) |
| `ERA_MULT_LDON_GOD` | 1.0 | 6-8 (LDoN/Gates/OoW) |

#### Era Tier Cost Multipliers

Multiply Essence tier transition costs during a given era.

| Knob | Default | Effect |
|---|---|---|
| `ERA_TIER_COST_CLASSIC` | 1.0 | During Classic-only era |
| `ERA_TIER_COST_KUNARK` | 1.0 | During Kunark era |
| `ERA_TIER_COST_VELIOUS` | 1.0 | During Velious era |
| `ERA_TIER_COST_LUCLIN` | 1.0 | During Luclin era |
| `ERA_TIER_COST_POP` | 1.0 | During PoP era |
| `ERA_TIER_COST_OOW_PLUS` | 1.0 | OoW and beyond |

#### Era Item XP Multipliers

Multiply Power Slot XP earned per kill during a given era.

| Knob | Default | Effect |
|---|---|---|
| `ERA_ITEM_XP_MULT_CLASSIC` | 1.0 | During Classic era |
| `ERA_ITEM_XP_MULT_KUNARK` | 1.0 | During Kunark era |
| `ERA_ITEM_XP_MULT_VELIOUS` | 1.0 | During Velious era |
| `ERA_ITEM_XP_MULT_POP_PLUS` | 1.0 | During PoP and later |

**Example tuning scenario:** Classic-only era feels too fast. Reduce Essence income
and increase tier costs:
```
ERA_MULT_CLASSIC = 0.7         -- 30% less Essence from Classic items
ERA_TIER_COST_CLASSIC = 1.5    -- Hategiver Ench→Leg goes from 7,263 to 10,895 Essence
```
Later, when Kunark unlocks, those knobs don't change — only Kunark-era knobs apply to
new content, and Classic knobs can be readjusted independently.

---

## 12. Manual Override & Regeneration System

Stat scaling formulas produce correct results for the vast majority of items, but some
individual items need hand-tuning. An item with 50 base Attack might generate 100 Attack
at Enchanted, which is too high for its role. Rather than distort the global formula to
fix edge cases, overrides let us hand-tune specific items per stat per tier.

### How Overrides Work

1. **Formula-first.** Every item's scaled stats are computed using the standard tier
   formulas (§3). This is the default for all 118,000+ items.
2. **Per-stat overrides.** A specific item can have one or more stat values overridden
   at a specific tier. Overrides replace the formula value — they do not stack.
3. **Reasons tracked.** Each override records why it was made, so future reviewers
   understand the intent.

### Override Resolution Order

```
For each (item, tier, stat):
  1. Calculate stat using standard formula (§3 Stat Scaling Formulas)
  2. Check override table for (item_id, tier, stat_name)
  3. If override exists → use override value
  4. If no override → use formula value
```

### Database Schema

```sql
CREATE TABLE item_scaling_overrides (
    item_id      INT          NOT NULL,
    tier         TINYINT      NOT NULL,   -- 0=Base, 1=Enchanted, 2=Legendary, 3=Mythic
    stat_name    VARCHAR(32)  NOT NULL,   -- 'damage','ac','hp','mana','endurance',
                                          -- 'astr','asta','aagi','adex','awis','aint','acha',
                                          -- 'attack','haste','spelldmg','healamt',
                                          -- 'heroic_str','heroic_sta', etc.
    override_value INT        NOT NULL,   -- the hand-tuned value
    reason       VARCHAR(255) DEFAULT NULL,
    created_at   DATETIME     DEFAULT CURRENT_TIMESTAMP,
    updated_at   DATETIME     DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (item_id, tier, stat_name)
);
CREATE INDEX idx_override_item ON item_scaling_overrides(item_id);
```

### Admin Commands

```
#override set <item_id> <tier> <stat> <value> [reason]
    Set a manual override for one stat on one item at one tier.
    Example: #override set 28854 1 attack 60 "ATK too high for 1HB at Enchanted"

#override remove <item_id> <tier> <stat>
    Remove a single override (reverts to formula).

#override clear <item_id>
    Remove all overrides for an item.

#override list [item_id]
    List overrides. If item_id is given, show that item's overrides with
    formula vs. override values side-by-side. If omitted, show all overridden items.

#override filter [--tier <N>] [--stat <name>]
    List all items that have any overrides, optionally filtered by tier or stat.
    Output: item_id, item_name, tier, stat, formula_value, override_value, reason.
```

### Regeneration

When global tuning knobs change (e.g., `LEGENDARY_COMBAT_BONUS` moves from 1.3 to 1.4),
all items need recalculation. The regenerate command does this while preserving overrides:

```
#regenerate items [--dry-run] [--verbose]
    1. For each item in the database:
       a. Recalculate all scaled stats using CURRENT formula parameters
       b. Check item_scaling_overrides for per-stat overrides
       c. Apply overrides on top of recalculated values
       d. Update stored/cached scaled stats
    2. Output summary:
       "Regenerated 118,799 items. 47 items have manual overrides preserved."
    3. --dry-run: show what would change without applying
    4. --verbose: print every item with changed values
```

Regeneration **never touches override values**. If a formula change makes an override
unnecessary (the new formula already produces the desired value), the admin manually
removes the override with `#override remove`.

### Python Tool: manage_overrides.py

A CLI tool for bulk override management:

```
python tools/manage_overrides.py --list                   # all overrides
python tools/manage_overrides.py --filter --tier 2        # Legendary overrides only
python tools/manage_overrides.py --compare 28854          # formula vs override side-by-side
python tools/manage_overrides.py --export overrides.csv   # export for spreadsheet review
python tools/manage_overrides.py --import overrides.csv   # bulk import from spreadsheet
```

### Example Workflow

1. After playtesting, notice Hategiver (ID 28854) has too much Attack at Enchanted.
   Formula gives +30 Attack. Want +20.
   ```
   #override set 28854 1 attack 20 "ATK too high for 1HB, tested in-game"
   ```
2. Later, adjust `LEGENDARY_COMBAT_BONUS` from 1.3 to 1.4. Run:
   ```
   #regenerate items --dry-run
   ```
   See that Hategiver's override is preserved, all other items recalculate cleanly.
3. Run `#regenerate items` to apply.
4. Review all overrides to see if any need updating after the formula change:
   ```
   #override filter
   ```

### Tuning Knobs

| Knob | Default | Effect |
|---|---|---|
| `OVERRIDE_TABLE` | `item_scaling_overrides` | Database table name |
| `OVERRIDE_CACHE_TTL` | 300 | Seconds to cache override lookups (0 = no cache) |

---

*End of Item Progression & Augment System design v0.3.*
