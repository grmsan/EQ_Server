# Augment System Design

**Version:** 0.1
**Status:** Brainstorm / Design
**Parent Doc:** INFINITE_ITEM_PROGRESSION_DESIGN.md

---

## 1. Purpose

Augments are the infinite progression layer. Item tiers (Base → Mythic) define an item's
power ceiling. Augments fill the slots those tiers unlock, giving endgame players long-term
customization and chase without altering item identity.

This document covers augment acquisition, categories, the currency economy, and the merge
system in detail. The parent doc covers item tiers and the power slot.

> **Solo server.** All content is solo. References to "raid-tier" and "named" mobs describe
> difficulty tiers, not group sizes. Players grow powerful enough to tackle any challenge
> over time.

---

## 2. When Augments Unlock

Augments exist at every stage of the game, but their role changes with progression:

**Note:** Augment power and Essence costs scale with item level (iLevel). See [ITEM_LEVEL_SYSTEM.md](ITEM_LEVEL_SYSTEM.md) for details.

| Phase | Level Range | What's Available |
|---|---|---|
| **Leveling** | 1 – max | Simple vendor augs (platinum), ultra-rare zone drops |
| **Endgame Chase** | Max level | Essence economy, merge system, premium augs, boss drops |

During leveling, augs are small quality-of-life bonuses — a cheap weapon proc from a
vendor, or a lucky rare drop with some zone flavor. They introduce the concept of aug
slots without overwhelming new players.

At max level, the full system opens up: salvaging loot into Essence, buying and merging
augs at the Forgemaster, chasing premium drops from named mobs and raid-tier bosses. This is
where the infinite progression depth lives.

---

## 3. Leveling Augments

### 3.1 Vendor Weapon Augs

Basic weapon augments are sold by **Weaponsmith** NPCs in starting cities and hub towns.
They are cheap, platinum-only, and give new players something to slot from the very start.

New tiers unlock every 10 levels. Each tier replaces the last — a player buys the best
one available for their level and moves on.

| Required Level | Aug Name | Effect | Platinum Cost |
|---|---|---|---|
| 1 | Chipped Combat Stone | +1 DMG, minor DD proc (5 dmg) | 5pp |
| 10 | Rough Combat Stone | +2 DMG, minor DD proc (15 dmg) | 25pp |
| 20 | Honed Combat Stone | +3 DMG, minor DD proc (30 dmg) | 75pp |
| 30 | Tempered Combat Stone | +4 DMG, minor DD proc (50 dmg) | 150pp |
| 40 | Polished Combat Stone | +5 DMG, minor DD proc (80 dmg) | 300pp |
| 50 | Reinforced Combat Stone | +6 DMG, minor DD proc (120 dmg) | 500pp |

These are weapon-only (type 4 slot).

Vendor augs follow the same merge rules as everything else (3:1, same name + level). Higher merge levels only improve proc rate slightly — no
increased damage. They salvage for 1 Common Essence (the minimum).

### 3.2 Vendor Sustain Augs

Sustain augments help builds stay in the fight longer. Sold by the same **Weaponsmith**
NPCs alongside Combat Stones. Cheap, platinum-only, tiered every 10 levels.

**Melee Sustain — Lifetap Stones** (weapon-only, type 4 slot)

Small heal-on-hit proc. Keeps melee builds healthy between pulls without replacing a
healer.

| Required Level | Aug Name | Effect | Platinum Cost |
|---|---|---|---|
| 1 | Chipped Lifetap Stone | Heal proc on hit: 3 HP | 5pp |
| 10 | Rough Lifetap Stone | Heal proc on hit: 8 HP | 25pp |
| 20 | Honed Lifetap Stone | Heal proc on hit: 15 HP | 75pp |
| 30 | Tempered Lifetap Stone | Heal proc on hit: 25 HP | 150pp |
| 40 | Polished Lifetap Stone | Heal proc on hit: 40 HP | 300pp |
| 50 | Reinforced Lifetap Stone | Heal proc on hit: 60 HP | 500pp |

**Caster Sustain — Mana Stones** (weapon-only, type 4 slot)

Small mana return on spell cast. Slotted in weapons.

| Required Level | Aug Name | Effect | Platinum Cost |
|---|---|---|---|
| 1 | Chipped Mana Stone | Restore 2 mana on spell cast | 5pp |
| 10 | Rough Mana Stone | Restore 5 mana on spell cast | 25pp |
| 20 | Honed Mana Stone | Restore 10 mana on spell cast | 75pp |
| 30 | Tempered Mana Stone | Restore 18 mana on spell cast | 150pp |
| 40 | Polished Mana Stone | Restore 28 mana on spell cast | 300pp |
| 50 | Reinforced Mana Stone | Restore 40 mana on spell cast | 500pp |

Same rules as Combat Stones: intentionally weak, mergeable.

### Weapon Aug Slot Philosophy

The weapon aug slot (type 4) is the **build-defining slot** for every archetype:

| Build | Weapon Aug Role | Leveling Examples | Endgame Examples |
|---|---|---|---|
| Melee DPS | Damage procs | Combat Stone (DD proc) | Shard of Flame, boss proc augs |
| Melee Tank/Sustain | Lifetap / heal procs | Lifetap Stone | Shard of Mending, Shard of Siphoning |
| Caster DPS | Mana sustain, spell-triggered effects | Mana Stone | (future: DoT-on-cast augs, spell DD procs) |
| Healer | Mana sustain | Mana Stone | (future: heal-boost-on-cast augs) |
| Hybrid | Mix based on role | Combat Stone or Mana Stone | Flexible — melee proc or caster sustain |

**Dual wield** characters get two weapon aug slots — they can mix and match. A hybrid might
run a Combat Stone in the mainhand and a Mana Stone in the offhand. A rogue might run two
Combat Stones for max damage, or swap one for a Lifetap Stone when soloing.

**2H / Bow** characters get more total aug slots (2/4/5/6 vs 1/2/3/4) to compensate for
one weapon. Multiple weapon aug slots on a 2H lets them stack effects or diversify.

### 3.3 Ultra-Rare Zone Drops (Flavor Augs)

Each zone has a small pool of unique augments on its drop tables as ultra-rare loot. These
are themed to the zone and give leveling players an exciting "jackpot" moment without
breaking progression.

**Drop rate:** Very low — roughly 0.1–0.5% from any mob in the zone. Finding one during a
normal leveling session is a lucky event, not an expectation.

**Examples:**

| Zone | Aug Name | Effect | Flavor |
|---|---|---|---|
| Blackburrow | Gnoll Fang Chip | +2 STR, +5 HP | Crude tooth fragment |
| Befallen | Darkened Bone Shard | +2 INT, +3 Mana Regen | Necromantic residue |
| Unrest | Flickering Spirit Gem | +2 WIS, +8 HP | Haunted essence |
| Permafrost | Frozen Shard | +2 STA, +5 AC | Ice-bound splinter |
| Solusek's Eye | Ember Sliver | +2 DEX, Fire DD proc (20 dmg) | Volcanic glass |
| Lower Guk | Swamp Crystal | +3 AGI, +5 Avoidance | Murky resonance |
| Plane of Fear | Fragment of Dread | +3 to two random Heroic Stats | Crystallized terror |
| Plane of Hate | Spite Shard | +4 STR, +10 Attack | Innoruuk's malice |

These augs are:
- **Zone-appropriate in power.**
- **Mergeable.** Same 3:1 rules as everything else. Merging gives
  marginal gains. Salvages for 1 Essence.
- **Tradeable.** Players can sell them or give them to alts.
- **Unique per zone.** Each zone has 1–3 possible flavor augs on its loot table.

---

## 4. Aug Slot Availability by Tier

| Item Tier | Aug Slots (1H / Armor) | Aug Slots (2H / Bow) |
|---|---|---|
| Base | 1 | 2 |
| Enchanted | 2 | 4 |
| Legendary | 3 | 5 |
| Mythic | 4 | 6 |

Slot types follow EQ conventions: 1H and armor use types 1/2/3/4; 2H and bows use
types 2/4/5/6.

---

## 5. Endgame Augment Categories

The following categories are the **endgame chase layer** — available at max level through
the Essence economy, mob drops, boss kills, and quests. This is where the merge system and
long-term progression live.

### 5.1 Basic Augments (Common)

Basic augs provide straightforward stat bonuses. They are purchasable from vendors using
Common Essence or in-game platinum and drop freely from endgame mobs.

**Stat Augs** — Single-stat focus, one heroic stat plus a small companion bonus.

| Aug Name Pattern | Primary Stat | Secondary Bonus | Example |
|---|---|---|---|
| Stone of Might | +3 Heroic STR | +5 Attack | Melee DPS slot filler |
| Stone of Fortitude | +3 Heroic STA | +8 AC | Tank slot filler |
| Stone of Precision | +3 Heroic DEX | +5 Attack | Accuracy/crit slot filler |
| Stone of Evasion | +3 Heroic AGI | +5 Avoidance | Avoidance slot filler |
| Stone of Insight | +3 Heroic INT | +8 Spell DMG | Caster DPS slot filler |
| Stone of Devotion | +3 Heroic WIS | +8 Heal Amt | Healer slot filler |
| Stone of Presence | +3 Heroic CHA | +15 HP | Social/utility filler |

**Simple Proc Augs** — Weapon-only (type 4 slot), add a basic proc effect.

| Aug Name Pattern | Effect | Reuse | Example |
|---|---|---|---|
| Shard of Flame | Fire DD proc, 100 dmg | 8s | Basic fire damage |
| Shard of Frost | Cold DD proc, 100 dmg | 8s | Basic cold damage |
| Shard of Venom | Poison DoT proc, 40/tick × 3 | 10s | Sustained poison |
| Shard of Mending | Heal proc, 150 HP on hit | 12s | Self-sustain for melee |
| Shard of Siphoning | Lifetap proc, 80 dmg + 80 heal | 10s | Damage + recovery |

All basic augs start at **Level 1** in the merge system.

### 5.2 Premium Augments (Rare)

Premium augs come from named mobs, raid-tier bosses, and long quest chains. They have stronger
effects, multi-stat bonuses, or unique abilities not available from vendors.

**Multi-Stat Augs** — Dropped from named mobs.

| Source | Stats | Example |
|---|---|---|
| Named mob drop | +2 to all Heroic Stats | "Glowing Shard of Balance" |
| Named mob drop | +3 Heroic STR, +3 Heroic STA | "Warlord's Fragment" |
| Named mob drop | +3 Heroic INT, +3 Heroic WIS, +10 Mana Regen | "Arcanist's Fragment" |

**Boss Proc Augs** — Dropped from raid-tier bosses. Stronger versions of basic procs with
additional effects.

| Source | Effect | Example |
|---|---|---|
| Raid-tier boss | Fire DD proc 250 dmg + Fire resist debuff -20 | "Ember of Nagafen" |
| Raid-tier boss | Lifetap proc 200 dmg + 200 heal | "Fang of Innoruuk" |
| Raid-tier boss | Heal proc 400 HP + HoT 50/tick | "Tear of Tunare" |
| Raid-tier boss | Haste proc +10% for 12s on hit | "Spark of Rallos" |

Raid-tier bosses often drop multiple **variants** of their signature aug (e.g., Nagafen
might drop "Ember of Nagafen (Fire)" or "Ember of Nagafen (Frost)"). Salvage the variants
you don't want — they yield Rare Essence. Spend that Essence at the Artificer on a
Targeted Rare Aug Box to eventually buy the exact version you're after. Failed drops fund
the correct one.

**Quest Chain Augs** — Earned through multi-step quest lines. These are unique named
augments with flavor and effects tailored to the quest story.

| Quest Type | Reward | Example |
|---|---|---|
| Zone-length chain (5–8 steps) | +1 to all Heroic Stats, named | "Blessing of the Kedge" |
| Epic-length chain (15+ steps) | +2 to all Heroic Stats + unique proc | "Heart of the Swarm" |
| Server event / rare spawn series | Unique effect aug | "Shard of the Sleeper" |

Premium augs drop at **Level 1** but have a higher stat baseline than basic augs of the
same merge level.

---

## 6. Merge System

### 6.1 Core Rule

Combine **3 augments of the same type and level + 1 Merge Catalyst** → **1 augment of the
next level**. Maximum level: **5**.

Merging is done at the **Augment Forgemaster** NPC in hub cities using a **4-slot
container**: 3 aug slots + 1 catalyst slot.

```
3× Level 1 Stone of Might + Merge Catalyst → 1× Level 2 Stone of Might
3× Level 2 Stone of Might + Merge Catalyst → 1× Level 3 Stone of Might
3× Level 3 Stone of Might + Merge Catalyst → 1× Level 4 Stone of Might
3× Level 4 Stone of Might + Merge Catalyst → 1× Level 5 Stone of Might (MAX)
```

### 6.2 Merge Catalysts

The catalyst is a consumable reagent purchased from vendors. It scales with the merge
level being attempted.

| Merge Target | Catalyst | Cost |
|---|---|---|
| Level 1 → 2 | Lesser Merge Catalyst | 250 Common Essence or 100pp |
| Level 2 → 3 | Merge Catalyst | 750 Common Essence or 300pp |
| Level 3 → 4 | Greater Merge Catalyst | 2,500 Common Essence or 50 Rare Essence |
| Level 4 → 5 | Superior Merge Catalyst | 125 Rare Essence |

The catalyst cost is what makes merging vendor leveling augs a bad deal naturally — nobody
is spending 250 Essence to merge three 5pp Combat Stones. But for endgame augs, the
catalyst is a reasonable tax on progression.

### 6.3 Basic Aug Stat Progression

| Aug Level | Single-Stat Aug (e.g., Heroic DEX) | Secondary Bonus | Total Augs Invested |
|---|---|---|---|
| 1 | +3 | +5 Attack | 1 |
| 2 | +6 | +10 Attack | 3 |
| 3 | +10 | +16 Attack | 9 |
| 4 | +15 | +22 Attack | 27 |
| 5 (MAX) | +21 | +30 Attack | 81 |

### 6.4 Basic Proc Aug Progression

| Aug Level | DD Proc (Fire) | Reuse Timer | Total Augs Invested |
|---|---|---|---|
| 1 | 100 dmg | 8s | 1 |
| 2 | 175 dmg | 7s | 3 |
| 3 | 275 dmg | 6s | 9 |
| 4 | 400 dmg | 5s | 27 |
| 5 (MAX) | 550 dmg | 4s | 81 |

### 6.5 Premium Aug Stat Progression

Premium multi-stat augs scale the same way but across all their stats simultaneously.

| Aug Level | "All Heroic Stats" Aug | Total Augs Invested |
|---|---|---|
| 1 | +2 to each | 1 |
| 2 | +4 to each | 3 |
| 3 | +7 to each | 9 |
| 4 | +10 to each | 27 |
| 5 (MAX) | +14 to each | 81 |

The per-stat value is lower than a single-stat aug at the same level, but total stat budget
is higher. A Level 5 all-stats aug gives +14 × 7 stats = 98 total heroic points vs a
Level 5 single-stat aug giving +21 to one stat.

### 6.6 Merge Rules

- Augs must be the **same named type** and **same level** to merge. You cannot merge a
  Stone of Might with a Stone of Precision.
- The 4th slot **must** contain the correct tier of Merge Catalyst. Wrong catalyst = merge
  fails.
- Premium augs merge with copies of themselves. 3× "Ember of Nagafen" Level 1 + catalyst
  → 1× "Ember of Nagafen" Level 2.
- You **cannot** merge basic into premium or vice versa.
- Quest chain augs that are unique (one per character) **cannot be merged** — they are
  already at their final power.

### 6.7 Non-Mergeable Augs

Some augs are unique rewards and designated non-mergeable. They drop/are awarded at a
fixed power level and cannot be combined. These are always clearly marked in their item
description as "(Unique — Cannot Be Merged)".

Examples: epic quest chain rewards, server event augs, one-time achievement augs.

### 6.8 Investment Curve

| Merge Level | Augs Required | Catalyst Cost | Cumulative Augs | Notes |
|---|---|---|---|---|
| 1 | 1 | — | 1 | Entry — one drop or purchase |
| 2 | 3 | 50 Common / 100pp | 3 | Casual goal |
| 3 | 9 | 750 Common / 300pp | 9 | Natural plateau for most players |
| 4 | 27 | 2,500 Common / 50 Rare | 27 | Dedicated grinder territory |
| 5 | 81 | 125 Rare | 81 | Long-term chase, max investment |

---

## 7. Currency: Essence Economy

### 7.1 Essence Types

| Essence | Source | Primary Use |
|---|---|---|
| **Common** | Salvaging any item | Buy basic aug templates, low-tier catalysts |
| **Rare** | Salvaging named/raid-tier items, or rare proc from any salvage | Buy premium aug templates, high-tier catalysts |

### 7.2 Earning Essence

**Salvage Satchel** — A permanent, 20-slot interactive container provided to the player.
Players drop unwanted items into this bag as they adventure and loot bosses. One click
(via `#salvage` command or UI button) destroys all items inside and converts them to
Essence. No catalyst required — salvaging is always a single action.

Essence yields are calculated from the item's **iLevel** (an uncapped power score derived
from item stats). See [ITEM_LEVEL_SYSTEM.md](ITEM_LEVEL_SYSTEM.md) for the complete formula.
Only **magic** items can be salvaged — non-magic vendor trash is rejected by the Salvage
Satchel. For magic items, Essence = `max(1, iLevel - 100)`, creating a smooth ramp from
near-worthless to full value. Higher-tier items give a small bonus: Enchanted +15%,
Legendary +35%. Era multipliers can adjust yields per expansion.

| iLevel | Example Item | Magic? | Common Essence (Base) |
|---|---|---|---|
| 14 | Rusty Sword | No | — (unsalvageable) |
| 77 | Lamentation | Yes | 1 |
| 269 | Hategiver (Velious) | Yes | 169 |
| 645 | Blade of War (PoP) | Yes | 545 |
| 1,106 | Anguish avg (OoW) | Yes | 1,006 |

Low-level magic items yield just 1–5 Essence — enough to teach new players the salvage flow.
The real Essence income starts at mid-tier drops and scales smoothly into endgame.

### 7.3 Bonus Essence Sources

Beyond salvaging, Essence can be earned from:

| Source | Reward | Frequency |
|---|---|---|
| Daily endgame quest | 500 Common + 20 Rare | Once per day |
| Weekly boss quest | 2,000 Common + 150 Rare | Once per week |
| Named mob bonus (first kill of day) | +250 Common | Per unique named, resets daily |
| Zone completion achievement | 1,000 Common + 50 Rare (one-time) | Once ever |
| Rare global drop: "Essence Crystal" | 500 Rare | Very rare world drop |

These supplement salvaging so players who keep gear (alts, collections) still earn Essence
at a reasonable rate.

### 7.4 Spending Essence

#### Common Essence — "The Provisioner" (vendor)

| Item | Cost | Notes |
|---|---|---|
| Level 1 Stat Aug Template (choose type) | 500 | Pick any basic stat aug |
| Level 1 Proc Aug Template (choose type) | 750 | Pick any basic proc aug |
| Salvage Satchel (if not owned) | 250 | One-time purchase |
| Augment Solvent (removes aug safely) | 100 | Non-destructive aug removal |

> **Note:** Tier-up catalysts have been replaced by iLevel-scaled Essence costs paid
> directly through the Power Slot interface. See [ITEM_LEVEL_SYSTEM.md](ITEM_LEVEL_SYSTEM.md)
> §Tier Transition Costs.

#### Rare Essence — “The Artificer” (vendor)

| Item | Cost | Notes |
|---|---|---|
| Level 2 Stat Aug Template (choose type) | 1,000 | Skip 3× Level 1 merge for basic augs |
| Random Rare Aug Box (Level 1) | 3,000 | Random premium aug — proc/multi-stat/focus |
| Targeted Rare Aug Box | 7,500 | Choose category (proc/multi-stat/focus), random within |

### 7.5 Essence Upgrade Path

This is the core loop for building aug power over time:

```
1. Kill mobs → loot items you don't need
2. Salvage items → earn Common Essence (and occasionally Rare)
3. Buy Level 1 aug templates from Provisioner with Common Essence
4. Equip augs in your gear slots for immediate power
5. Get duplicate augs (drops + purchases) → merge 3:1 at Forgemaster
6. Merged augs are stronger → replace what's equipped
7. Repeat — each cycle yields slightly more powerful augs
```

For premium augs:
```
1. Kill named mobs / raid-tier bosses → premium aug drops
2. Salvage named/boss items you don't need → earn Rare Essence
3. Buy Random Rare Aug Boxes from Artificer if needed
4. Merge 3× of the same premium aug → stronger version
5. Premium augs take longer to max but are meaningfully stronger
```

### 7.6 Platinum as Alternate Currency

Basic Level 1 stat augs should also be purchasable with **platinum** from a separate
general vendor or the Provisioner's secondary tab. This provides an entry point for
players who haven't engaged with the Essence system yet.

| Item | Platinum Cost |
|---|---|
| Level 1 Stat Aug (choose type) | 500pp |
| Level 1 Proc Aug (choose type) | 1,000pp |
| Augment Solvent | 100pp |

The platinum prices are intentionally high enough that Essence is more efficient for anyone
actively playing endgame — platinum is the convenience path, not the optimal one.

---

## 8. Aug Enhancement

Beyond merging, two additional systems let players invest in augs without needing duplicates.

### 8.1 Augment Infusion

Spend Essence directly on an equipped aug to increase its power incrementally.

| Infusion Level | Common Essence Cost | Stat Bonus Gained | Cumulative |
|---|---|---|---|
| Infusion 1 | 250 | +1 to primary stat | +1 |
| Infusion 2 | 500 | +1 to primary stat | +2 |
| Infusion 3 | 1,000 | +1 to primary stat | +3 |
| Infusion 4 | 2,000 | +1 to primary stat | +4 |
| Infusion 5 | 4,000 | +1 to primary stat | +5 |

Total cost for 5 infusions: 7,750 Common Essence for +5 to primary stat.

This stacks with merge level, **but the combined total is capped at the Level 5 maximum**
(+21 for single-stat augs). A Level 3 Stone of Might (+10 Heroic STR) with 5 infusions
becomes +15 Heroic STR. A Level 5 Stone of Might is already at the +21 cap and cannot be
infused further.

**Cap rule:** `merge_stat + infusion_stat ≤ Level 5 maximum`. This cap can be raised in
future expansions by introducing a new catalyst tier.

**Design intent:** Infusion gives a steady drip of power to players who get unlucky on
drops (can't find 3 matching augs). It doesn't replace merging — merging is still more
efficient per Essence spent — but it means no aug ever feels "stuck." The cap ensures
Level 5 remains the definitive ceiling for any given aug.

### 8.2 Augment Transmutation

Convert unwanted augs into Essence without salvaging the parent item.

| Aug Level | Common Essence Returned | Rare Essence Returned |
|---|---|---|
| 1 | 125 | — |
| 2 | 375 | — |
| 3 | 1,000 | 25 |
| 4 | 2,500 | 75 |
| 5 | 6,000 | 200 |

This prevents dead-end augs from being wasted. If you've merged up to Level 3 in a stat
you no longer want, transmutation recovers a meaningful portion of the investment.

---

## 9. Acquisition Summary

| Aug Category | When | How to Get | Currency? | Mergeable? |
|---|---|---|---|---|
| Vendor Combat Stone | Level 1+ | Weaponsmith vendors | Platinum | Yes (poor value) |
| Vendor Sustain Stone | Level 1+ | Weaponsmith vendors | Platinum | Yes (poor value) |
| Zone Flavor Drop | Level 1+ | Ultra-rare mob drops per zone | — (drop only) | Yes (poor value) |
| Basic Stat | Max level | Mob drops, Provisioner (Common Essence), PP vendor | Common Essence or PP | Yes |
| Basic Proc | Max level | Mob drops, Provisioner (Common Essence), PP vendor | Common Essence or PP | Yes |
| Multi-Stat (Premium) | Max level | Named mob drops, Artificer boxes (Rare Essence) | Rare Essence | Yes |
| Boss Proc (Premium) | Max level | Raid-tier boss drops, Artificer boxes (Rare Essence) | Rare Essence | Yes |
| Quest Chain | Max level | Quest rewards | Quest completion | Unique only — not mergeable |
| Achievement | Max level | One-time achievements | Achievement | Unique only — not mergeable |

---

## 10. Design Principles

1. **Augs exist from level 1.** Cheap vendor weapon augs and rare zone drops introduce
   the system early. Players learn what aug slots are before they matter.

2. **Chase lives at max level.** The Essence economy, merge system, and premium drops are
   endgame systems. Salvaging a session's worth of boss loot is the fuel — not something
   a level 20 encounters.

3. **Every drop has value.** Unwanted items become Essence. Unwanted augs become Essence
   via transmutation. Nothing is wasted.

4. **Multiple paths to power.** Drops, merging, vendor purchases, infusion, quests. No
   single path is mandatory.

5. **Diminishing returns on investment.** 3:1 merge ratio means each level costs 3× more
   for less-than-3× improvement. Level 3 is the natural wall; Level 5 is the prestige goal.

6. **Basic augs are accessible.** Platinum purchases and Common Essence ensure every
   endgame player can participate immediately.

7. **Premium augs are aspirational.** Boss drops and quest chains provide chase items that
   feel meaningful to earn.

8. **No mandatory grind.** The system rewards consistent play over binge sessions. Dailies,
   weeklies, and natural loot flow keep Essence income steady.

---

## 11. Design Decisions

**Infusion vs. Merge — how they differ:**
Both paths improve an aug, but they trade different resources:

| Path | Resource Cost | Stat Gained | Requires |
|---|---|---|---|
| Merge L1 → L2 | 3 augs + 250 Common Essence | +3 to primary stat | Duplicate drops |
| Infuse ×3 | 1 aug + 1,750 Common Essence | +3 to primary stat | No duplicates needed |

Merging is ~3× more Essence-efficient per stat point gained. Infusion costs more Essence
but only needs one aug — it's the safety valve for players who can't find duplicates.
They complement rather than compete: merge when you have extras, infuse when you don't.

**Boss aug acquisition:** Raid-tier bosses drop multiple variants of their signature aug.
Unwanted variants are salvaged for Rare Essence. That Essence funds Targeted Rare Aug Box
purchases until the desired variant is obtained. No permanent bad-luck gate.

**Platinum pricing:** Leveling vendor augs start at 5pp (level 1) and scale to
500pp (level 50), matching what a player can reasonably afford at each stage. Endgame
platinum pricing for L1 stat/proc augs (500pp/1,000pp) is intentionally higher than
Essence — it's a convenience option, not the optimal path.

**Reforge system:** Removed. Secondary bonuses are static at creation. Handling
dynamic item properties adds implementation complexity not worth the return.

**Transmutation rates:** Current values (~25% Essence recovery) are placeholder estimates.
Will tune in-game once the Essence economy has been felt in play. Numbers are easy to
adjust via config without any code changes.
