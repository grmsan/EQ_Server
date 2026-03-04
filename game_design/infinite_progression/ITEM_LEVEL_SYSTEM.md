# Item Level (iLevel) System

**Version:** 2.0
**Date:** March 2, 2026
**Status:** Final Specification

---

## What Is iLevel?

**Item Level (iLevel)** is a single number that represents an item's overall power. It is
calculated directly from the item's stats — damage, AC, HP, attributes, heroics, and other
combat stats. There is **no cap**; iLevel scales naturally with item power across all eras.

A Rusty Long Sword has iLevel 14. A top-tier OoW raid weapon has iLevel 1,100+.

### Why We Need iLevel

The Infinite Item Progression system uses iLevel for:

1. **Essence yields:** How much Essence you get for salvaging an item.
2. **Economy scaling:** Higher-era items yield more Essence, keeping the economy
   meaningful as content unlocks.
3. **Drop quality identification:** Is this a newbie item, mid-tier, or raid weapon?

**iLevel drives Essence — with two safeguards.** First, only **MAGIC** items can be
salvaged (the `magic` flag in the items DB). Rusty weapons, Cloth Caps, and most cheap
vendor gear aren't magic and simply can't enter the Salvage Satchel. Second, Essence
yield uses an offset: `max(1, iLevel - 100)`. This creates a smooth ramp — a low magic
item might yield 1–5 Essence, mid-tier yields grow gradually, and endgame items barely
notice the offset. No cliff, no exploit, clean progression.

---

## How iLevel Is Calculated

### Step 1: Classify the Item

| Category | Item Types | Primary Driver |
|----------|------------|----------------|
| **Weapons** | 1H/2H Slash, Pierce, Blunt, Bow, Martial | DPS, secondary stats |
| **Armor** | Armor, Shield | AC, HP, Mana |

### Step 2: Calculate Power Score

The power score is the item's iLevel. No mapping step, no curves — power IS iLevel.

#### Weapons
```
DPS = (damage × 100) / delay
power = DPS + (AC × 3) + (HP × 0.3) + (Mana × 0.3)
      + (attributes × 2) + attack + (haste × 10) + (heroics × 5)
```

Where `attributes` = sum of STR + STA + AGI + DEX + WIS + INT + CHA,
and `heroics` = sum of all Heroic stats.

#### Armor
```
power = (AC × 15) + (HP × 0.5) + (Mana × 0.5)
      + (attributes × 3) + (resists × 0.5) + (heroics × 5)
```

Where `resists` = sum of MR + FR + CR + DR + PR.

### Step 3: iLevel = max(1, floor(power))

That's it. No piecewise curves, no cap, no interpolation.

---

## Essence Yield Formula

Salvaging has two gates:

### Gate 1: Magic Items Only

Only items with the **MAGIC** flag (`items.magic = 1`) can be salvaged. Non-magic items
(Rusty weapons, basic vendor armor, most common junk) are rejected by the Salvage Satchel
entirely. This uses an existing, well-understood game concept — players already know which
items are "magic" — and it's tunable per-item in the database. If you want a specific
vendor item to be salvageable, just set its `magic` flag to 1.

### Gate 2: Essence Offset

For magic items that pass gate 1, Essence yield uses an **offset** to create a smooth
ramp from worthless to valuable:

```
Essence = max(1, iLevel - ESSENCE_OFFSET) × tier_bonus × era_multiplier
```

- **ESSENCE_OFFSET:** Default 100. Subtracted from iLevel before calculating Essence.
- **tier_bonus:** Base ×1.0, Enchanted ×1.15, Legendary ×1.35, Mythic (unsalvageable)
- **era_multiplier:** Server-configurable per expansion (default 1.0 for all eras)

**Why offset instead of a floor?** A floor creates an awkward cliff — items go from 1
Essence to 100+ Essence with nothing in between. The offset creates a natural gradient:
an iLevel 110 item gives 10 Essence, an iLevel 150 item gives 50, and iLevel 1,106 gives
1,006. High-end items barely notice the 100-point offset; low-end items are naturally
worthless. Players experience a smooth ramp as their gear improves.

**Why magic gate?** The offset alone would still allow mass-purchasing of cheap magic
vendor items for 1 Essence each. The magic gate eliminates that entirely — non-magic
vendor trash can't even be salvaged. Together, the two mechanisms cover all exploit vectors.

The era multiplier is the primary tuning knob for progression servers. If Classic-era
Essence income feels too fast, set `ERA_MULT_CLASSIC = 0.8`. If OoW feels too slow,
set `ERA_MULT_LDON_GOD = 1.3`. See §Per-Era Tuning Knobs below.

---

## Examples

### Example 1: Rusty Long Sword (Classic Newbie)

**Stats:** 1HS, Damage 5, Delay 35. No other stats.

```
DPS = (5 × 100) / 35 = 14.3
power = 14.3
iLevel = 14
Essence = N/A  (not a magic item — cannot be salvaged)
```

### Example 2: Lamentation (Classic — Permafrost/Nagafen)

**Stats:** 1HS, Damage 9, Delay 19. HP 20, STR 6, STA 6.

```
DPS = (9 × 100) / 19 = 47.4
power = 47.4 + 0 + (20 × 0.3) + 0 + (12 × 2) + 0 + 0 + 0 = 77.4
iLevel = 77
Essence = 1  (magic item, max(1, 77 - 100) = 1)
```

### Example 3: Hategiver (Velious — Avatar of War)

**Stats:** 1HS, Damage 15, Delay 19. AC 25, HP 85, STR 10, STA 15, AGI 10, DEX 10.

```
DPS = (15 × 100) / 19 = 78.9
power = 78.9 + (25 × 3) + (85 × 0.3) + 0 + (45 × 2) + 0 + 0 + 0 = 269.4
iLevel = 269
Essence = 169  (magic item, 269 - 100 = 169)
```

### Example 4: Blade of War (PoP — Rallos Zek the Warlord)

**Stats:** 2HS, Damage 48, Delay 32. AC 50, HP 175, Mana 175.
STR 30, STA 30, AGI 30, WIS 15, INT 15.

```
DPS = (48 × 100) / 32 = 150.0
power = 150.0 + (50 × 3) + (175 × 0.3) + (175 × 0.3) + (120 × 2) + 0 + 0 + 0 = 645.0
iLevel = 645
Essence = 545  (645 - 100)
```

### Example 5: Bone Staff of Wickedness (PoP 2HB)

**Stats:** 2HB, Damage 33, Delay 29. AC 30, HP 150, Mana 150.
STR 20, AGI 15, DEX 15, STA 20, WIS 10, INT 10.

```
DPS = (33 × 100) / 29 = 113.8
power = 113.8 + (30 × 3) + (150 × 0.3) + (150 × 0.3) + (90 × 2) + 0 + 0 + 0 = 473.8
iLevel = 473
Essence = 373  (473 - 100)
```

### Example 6: Blood-Polished Hammer (OoW Anguish)

**Stats:** 1HB, Damage 40, Delay 24. AC 30, HP 225, Mana 225.
STR 20, STA 20, AGI 15, DEX 15, WIS 10, INT 10. Heroic STR 5, Heroic STA 5.

```
DPS = (40 × 100) / 24 = 166.7
power = 166.7 + (30 × 3) + (225 × 0.3) + (225 × 0.3) + (90 × 2) + 0 + 0 + (10 × 5) = 651.7
iLevel = 651
Essence = 551  (651 - 100)

(Note: Anguish weapons range from ~700 to ~2,100 iLevel depending on the specific item.)
```

### Example 7: Cloth Cap (Vendor Armor)

**Stats:** Armor, AC 2. No other stats.

```
power = (2 × 15) = 30.0
iLevel = 30
Essence = N/A  (not a magic item — cannot be salvaged)
```

### Example 8: Cobalt Breastplate (Velious Plate)

**Stats:** Armor, AC 45. HP 50, STR 12, STA 12, DEX 12.

```
power = (45 × 15) + (50 × 0.5) + 0 + (36 × 3) + 0 + 0 = 808.0
iLevel = 808
Essence = 708  (808 - 100)
```

---

## iLevel Ranges by Era

These are representative ranges from actual database analysis. Not every item in an era
falls in this range — quest rewards and special drops spread widely.

| Era | Expansion | Typical iLevel Range | Avg Raid Item |
|-----|-----------|---------------------|---------------|
| Classic | Original, Kunark | 14 – 300 | ~220 |
| Velious | Scars of Velious | 100 – 1,660 | ~350 |
| Luclin | Shadows of Luclin | 150 – 1,600 | ~580 |
| PoP | Planes of Power | 200 – 1,800 | ~800 |
| OoW | Omens of War | 300 – 2,100 | ~1,100 |
| TSS/SoF | The Serpent's Spine, Secrets of Faydwer | 500 – 4,500 | ~2,700 |
| UF/HoT | Underfoot, House of Thule | 800 – 8,800 | ~4,300 |
| RoF/TDS | Rain of Fear, The Darkened Sea | 1,000 – 12,600 | ~5,400 |

### Raid Zone Totals (Full Clear, Loot Table Items)

These are the total Essence available from one full clear of a raid zone, based on
all equippable items in the zone's loot tables:

| Zone | Era | Magic Items | Avg iLevel | Total Essence |
|------|-----|-------------|-----------|---------------|
| Solusek C (Nagafen) | Classic | 24 | 303 | 4,863 |
| Plane of Fear | Classic | 120 | 212 | 14,075 |
| Plane of Hate | Classic | 175 | 258 | 28,503 |
| Temple of Veeshan | Velious | 141 | 346 | 35,787 |
| Vex Thal | Luclin | 137 | 581 | 65,938 |
| Plane of Fire | PoP | 28 | 529 | 12,001 |
| Anguish | OoW | 29 | 1,106 | 29,272 |
| Txevu | OoW | 38 | 1,013 | 34,692 |

> **Note:** Only magic items are counted (non-magic items cannot be salvaged).
> Some raid zones (Plane of Time, Anguish) also have token turn-in rewards
> that do not appear in loot tables. These add roughly 25-30% more Essence in practice.
> A full Anguish clear including turn-ins is estimated at ~38,000 Essence.

---

## Essence Economy

### Tier Transition Costs (iLevel² Power Curve)

Essence costs scale with the **square** of the item's iLevel — the gap between a Rusty
Sword and an Anguish weapon is enormous, exactly matching the power gap. A new player
can enchant a Rusty Sword for pocket change; an Anguish weapon takes a full raid night.

**Formula:** `tier_cost = max(1, TIER_FLOOR + round(iLevel² × TIER_SCALE))`

Each tier has a **floor** (minimum cost even for the weakest item) and a **scale** factor
applied to iLevel squared:

| Transition | Floor | Scale | Formula |
|---|---|---|---|
| Base → Enchanted | 1 | 0.008 | `1 + round(iLevel² × 0.008)` |
| Enchanted → Legendary | 25 | 0.033 | `25 + round(iLevel² × 0.033)` |
| Legendary → Mythic | 1,500 | 0.121 | `1500 + round(iLevel² × 0.121)` |

The floor ensures that even a trash-tier item pays a meaningful cost for the higher
tiers — Mythic is expensive for *everyone*, not just endgame players.

**Representative costs (Classic → OoW):**

| Item | Era | iLevel | Base→Ench | Ench→Leg | Leg→Myth | Total |
|---|---|---|---|---|---|---|
| Rusty Sword | Classic | 14 | **3** | 31 | 1,524 | 1,558 |
| Primal Velium 2H | Velious | 310 | 770 | 3,197 | 13,124 | 17,091 |
| Blade of War | Luclin | 645 | 3,330 | 13,753 | 51,826 | 68,909 |
| Greatblade of Chaos | PoP | 735 | 4,322 | 17,854 | 66,856 | 89,032 |
| Anguish avg | OoW | 1,106 | **9,788** | **40,387** | 149,478 | 199,653 |

> **Note:** These costs are independent of the Kill XP system. Players can tier up through
> EITHER path: grinding kills to reach the XP threshold (free), or spending Essence (instant).
> See §Integration with Progression System below.

### What This Feels Like Per Era

How many raid clears to fund an Enchanted → Legendary upgrade for a typical item from
that era? (One clear = all magic loot salvaged into Essence.)

| Raid Zone | Era | Clear Essence | Avg iLevel | Ench→Leg Cost | Clears Needed |
|---|---|---|---|---|---|
| Plane of Fear | Classic | ~14k | 212 | 1,508 | 0.1 |
| Plane of Hate | Classic | ~28k | 258 | 2,221 | 0.1 |
| Temple of Veeshan | Velious | ~36k | 346 | 3,975 | 0.1 |
| Vex Thal | Luclin | ~66k | 581 | 11,159 | 0.2 |
| Anguish | OoW | ~29k | 1,106 | **40,387** | **~1.4** |

**Design target:** Roughly one to two Anguish raid clears funds a Legendary upgrade on an
Anguish-tier item. Earlier-era items are dramatically cheaper — a single Classic/Velious
raid clear funds many upgrades, appropriate for weaker gear that serves as a stepping
stone. The squared cost curve means early content feels generous while endgame content
requires genuine investment.

### Salvage Yields by Tier

| iLevel | Example Item | Magic? | Base | Enchanted (+15%) | Legendary (+35%) |
|--------|-------------|--------|------|------------------|------------------|
| 14 | Rusty Sword | No | — | — | — |
| 30 | Cloth Cap | No | — | — | — |
| 77 | Lamentation | Yes | 1 | 1 | 1 |
| 269 | Hategiver | Yes | 169 | 194 | 228 |
| 645 | Blade of War | Yes | 545 | 626 | 735 |
| 1,106 | Anguish avg | Yes | 1,006 | 1,156 | 1,358 |

---

## Per-Era Tuning Knobs

The server runs as a **progression server** where expansions unlock sequentially. Each
era has independent multipliers so the economy can be tuned based on live player data
without affecting other eras.

### Era Essence Multipliers

These multiply the Essence yield for items that drop in a zone tagged with the era's
expansion ID.

| Knob | Default | Effect |
|------|---------|--------|
| `ERA_MULT_CLASSIC` | 1.0 | Essence multiplier for Classic-era items (expansions 0-1) |
| `ERA_MULT_KUNARK` | 1.0 | Kunark-era items (expansion 2) |
| `ERA_MULT_VELIOUS` | 1.0 | Velious-era items (expansion 3) |
| `ERA_MULT_LUCLIN` | 1.0 | Luclin-era items (expansion 4) |
| `ERA_MULT_POP` | 1.0 | Planes of Power items (expansion 5) |
| `ERA_MULT_LDON_GOD` | 1.0 | LDoN/Gates/OoW items (expansions 6-8) |

**Example:** If Classic Essence feels too generous during the Classic-only era:
```
ERA_MULT_CLASSIC = 0.7   -- 30% Essence reduction
```
Plane of Hate would then yield `28,503 × 0.7 = 19,952` Essence instead of 28,503.

**Example:** If OoW feels too stingy after unlocking:
```
ERA_MULT_LDON_GOD = 1.3  -- 30% Essence bonus
```
Anguish would yield `~38,000 × 1.3 = 49,400` Essence per clear.

### Era Tier Cost Multipliers

Optionally, the iLevel-scaled tier costs can be further multiplied per era. This lets you
make early-era upgrades cheaper or later-era upgrades more expensive.

| Knob | Default | Effect |
|------|---------|--------|
| `ERA_TIER_COST_CLASSIC` | 1.0 | Tier cost multiplier during Classic era |
| `ERA_TIER_COST_KUNARK` | 1.0 | During Kunark era |
| `ERA_TIER_COST_VELIOUS` | 1.0 | During Velious era |
| `ERA_TIER_COST_LUCLIN` | 1.0 | During Luclin era |
| `ERA_TIER_COST_POP` | 1.0 | During PoP era |
| `ERA_TIER_COST_OOW_PLUS` | 1.0 | OoW and beyond |

**Example:** During the Classic-only era, a Hategiver (iLevel 269) Ench→Leg costs
269 × 27 = 7,263 Essence. If that feels too cheap for Classic:
```
ERA_TIER_COST_CLASSIC = 1.5   -- 7,263 × 1.5 = 10,895 Essence
```

### Era XP Multipliers

Item XP earned per kill can also be tuned per era for Power Slot progression:

| Knob | Default | Effect |
|------|---------|--------|
| `ERA_ITEM_XP_MULT_CLASSIC` | 1.0 | Item XP multiplier during Classic era |
| `ERA_ITEM_XP_MULT_KUNARK` | 1.0 | During Kunark era |
| `ERA_ITEM_XP_MULT_VELIOUS` | 1.0 | During Velious era |
| `ERA_ITEM_XP_MULT_POP` | 1.0 | During PoP and later |

### Implementation

Era multipliers are stored in a `rule_values` table (or equivalent server config) and
loaded at zone boot. They can be changed at runtime via `#rules reload` without restarting.

```cpp
// Determine era from item's expansion tag or zone's expansion field
int era = GetItemEra(item->GetExpansion());

// Apply era multiplier to Essence yield
double era_mult = RuleR(EssenceEra, GetEraMultiplierKey(era));
int offset = RuleI(Essence, EssenceOffset);  // default 100
int essence = std::max(1, (int)((ilevel - offset) * tier_bonus * era_mult));
```

Zone-based era tagging is preferred over iLevel-range-based tagging because:
1. The item's source zone is unambiguous (no overlapping iLevel ranges between eras)
2. Quest rewards and token items inherit their zone's era automatically
3. Rules changes don't need iLevel curve knowledge

---

## Integration with Progression System

### Two Paths to Tier Up

Players can tier up an item through **either** path — whichever they reach first:

**Path A — Kill XP (passive, free):** Earn flat XP from kills with the item in the
Power Slot. When the XP threshold is reached, the item tiers up for free.

| Transition | XP Required | Time (white-con, 40 XP/kill) |
|---|---|---|
| Base → Enchanted | 1,000 | ~25 min |
| Enchanted → Legendary | 20,000 | ~8.3 hrs |
| Legendary → Mythic | 150,000 | ~62.5 hrs |

**Path B — Essence (active, costs resources):** Pay an iLevel²-scaled Essence cost for
an instant tier-up. No XP grind required.

| Transition | Cost |
|---|---|
| Base → Enchanted | `1 + round(iLevel² × 0.008)` |
| Enchanted → Legendary | `25 + round(iLevel² × 0.033)` |
| Legendary → Mythic | `1500 + round(iLevel² × 0.121)` |

Kill XP times are the same for all items (a Rusty Sword and an Anguish weapon take the
same number of kills). Essence costs scale with iLevel — cheap items are cheap to buy,
expensive items require serious salvage investment.

Both paths progress simultaneously. A player who raids AND grinds fills both bars.

### C++ Implementation

```cpp
int CalculateILevel(const EQ::ItemData* item) {
    // Power score IS the iLevel. No curves, no cap.
    double power = 0.0;

    if (IsWeapon(item->ItemType)) {
        double dps = (item->Damage * 100.0) / std::max(1, (int)item->Delay);
        double attrs = item->AStr + item->ASta + item->AAgi + item->ADex
                     + item->AWis + item->AInt + item->ACha;
        double heroics = item->HeroicStr + item->HeroicSta + item->HeroicAgi
                       + item->HeroicDex + item->HeroicWis + item->HeroicInt
                       + item->HeroicCha;
        power = dps + (item->AC * 3.0) + (item->HP * 0.3) + (item->Mana * 0.3)
              + (attrs * 2.0) + item->Attack + (item->Haste * 10.0)
              + (heroics * 5.0);
    } else {
        double attrs = item->AStr + item->ASta + item->AAgi + item->ADex
                     + item->AWis + item->AInt + item->ACha;
        double resists = item->MR + item->FR + item->CR + item->DR + item->PR;
        double heroics = item->HeroicStr + item->HeroicSta + item->HeroicAgi
                       + item->HeroicDex + item->HeroicWis + item->HeroicInt
                       + item->HeroicCha;
        power = (item->AC * 15.0) + (item->HP * 0.5) + (item->Mana * 0.5)
              + (attrs * 3.0) + (resists * 0.5) + (heroics * 5.0);
    }

    return std::max(1, (int)power);
}

int CalculateEssenceYield(const EQ::ItemData* item, int tier, int era) {
    // Gate 1: Only magic items can be salvaged
    if (!item->Magic) {
        return 0;  // Non-magic items cannot be salvaged
    }

    int ilevel = CalculateILevel(item);

    // Gate 2: Offset — subtract ESSENCE_OFFSET, minimum 1
    int offset = RuleI(Essence, EssenceOffset);  // default 100
    int base_essence = std::max(1, ilevel - offset);

    // Tier bonus: Base ×1.0, Enchanted ×1.15, Legendary ×1.35
    static const double tier_bonus[] = {1.0, 1.15, 1.35, 0.0};

    // Era multiplier from rules
    double era_mult = RuleR(EssenceEra, GetEraMultiplierKey(era));

    return std::max(1, (int)(base_essence * tier_bonus[tier] * era_mult));
}

int CalculateTierCost(int ilevel, int target_tier, int era) {
    // iLevel² power curve — floor + iLevel² × scale per tier
    // Floor ensures even trash items pay meaningful costs for higher tiers
    struct TierParams { int floor; double scale; };
    static const TierParams params[] = {
        {1,    0.008},   // Base → Enchanted
        {25,   0.033},   // Enchanted → Legendary
        {1500, 0.121},   // Legendary → Mythic
    };
    int transition = target_tier - 1;  // 0=B→E, 1=E→L, 2=L→M
    if (transition < 0 || transition > 2) return 0;

    auto& p = params[transition];
    double base_cost = p.floor + (double)(ilevel * ilevel) * p.scale;
    double era_mult = RuleR(EssenceTierCost, GetEraCostKey(era));
    return std::max(1, (int)std::round(base_cost * era_mult));
}
```

### Database Schema

```sql
-- iLevel is calculated at runtime, but can be cached for queries
ALTER TABLE items ADD COLUMN calculated_ilevel INT UNSIGNED DEFAULT 0;
CREATE INDEX idx_calculated_ilevel ON items(calculated_ilevel);
```

### Admin Commands

```
#item ilevel <item_id>           Show calculated iLevel for an item
#item ilevel_all                 Batch calculate iLevel for all items
#item salvage <item_id> [tier]   Show Essence yield for an item at a given tier
#era multiplier <era> <value>    Set an era Essence multiplier at runtime
#era show                        Display all current era multipliers
```

---

## Tuning Knobs Summary

### Core Formula

| Knob | Default | Effect |
|------|---------|--------|
| `WEAPON_AC_WEIGHT` | 3.0 | AC contribution to weapon power score |
| `WEAPON_HP_WEIGHT` | 0.3 | HP contribution to weapon power score |
| `WEAPON_MANA_WEIGHT` | 0.3 | Mana contribution to weapon power score |
| `WEAPON_ATTR_WEIGHT` | 2.0 | Attribute contribution to weapon power score |
| `WEAPON_HASTE_WEIGHT` | 10.0 | Haste contribution to weapon power score |
| `WEAPON_HEROIC_WEIGHT` | 5.0 | Heroic contribution to weapon power score |
| `ARMOR_AC_WEIGHT` | 15.0 | AC contribution to armor power score |
| `ARMOR_HP_WEIGHT` | 0.5 | HP contribution to armor power score |
| `ARMOR_MANA_WEIGHT` | 0.5 | Mana contribution to armor power score |
| `ARMOR_ATTR_WEIGHT` | 3.0 | Attribute contribution to armor power score |
| `ARMOR_RESIST_WEIGHT` | 0.5 | Resist contribution to armor power score |
| `ARMOR_HEROIC_WEIGHT` | 5.0 | Heroic contribution to armor power score |

### Salvage Economy

| Knob | Default | Effect |
|------|---------|--------|
| `ESSENCE_PER_ILEVEL` | 1.0 | Essence-to-iLevel ratio (1.0 = 1:1) |
| `ESSENCE_OFFSET` | 100 | Subtracted from iLevel before Essence calculation |
| `SALVAGE_REQUIRE_MAGIC` | true | Only items with magic flag can be salvaged |
| `SALVAGE_TIER_BONUS_ENCHANTED` | 1.15 | Enchanted salvage bonus (+15%) |
| `SALVAGE_TIER_BONUS_LEGENDARY` | 1.35 | Legendary salvage bonus (+35%) |
| `SALVAGE_RARE_CHANCE_NAMED` | 10% | Rare Essence chance from named drops |

### Tier Transition Costs (iLevel² Power Curve)

| Knob | Default | Effect |
|------|---------|--------|
| `TIER_FLOOR_BASE_TO_ENCHANTED` | 1 | Fixed floor added to iLevel² cost for Base → Ench |
| `TIER_FLOOR_ENCHANTED_TO_LEGENDARY` | 25 | Fixed floor for Ench → Leg |
| `TIER_FLOOR_LEGENDARY_TO_MYTHIC` | 1,500 | Fixed floor for Leg → Myth |
| `TIER_SCALE_BASE_TO_ENCHANTED` | 0.008 | Scale factor applied to iLevel² for Base → Ench |
| `TIER_SCALE_ENCHANTED_TO_LEGENDARY` | 0.033 | Scale factor applied to iLevel² for Ench → Leg |
| `TIER_SCALE_LEGENDARY_TO_MYTHIC` | 0.121 | Scale factor applied to iLevel² for Leg → Myth |

---

## Validation Results

Algorithm tested against database items spanning all eras:

| Item | Era | Expected Range | Calculated iLevel | Status |
|------|-----|---------------|-------------------|--------|
| Rusty Long Sword | Classic | Low single-digit | 14 | ✅ |
| Lamentation | Classic | Low raid | 77 | ✅ |
| Hategiver | Velious | Mid raid | 269 | ✅ |
| Blade of War | PoP | High raid | 645 | ✅ |
| Blood-Polished Hammer | OoW | Top raid | 796 | ✅ |
| Anguish average | OoW | 1,000+ | 1,106 | ✅ |

The uncapped system naturally distinguishes quality within and across eras without
artificial curve-fitting.

---

## Testing Tools

```bash
# Run the economy model against live database
python tools/model_raid_economy.py

# Validate iLevel calculations for sample items
python tools/validate_ilevel.py
```

---

## Related File: item_scaling.json

The `item_scaling.json` file in this directory is a **separate runtime configuration** for
the existing item stat scaling system (SpellDmgFromInt, HealFromWis curves, attribute
preferences, weapon curves). It is **not** part of the iLevel algorithm or the Infinite
Item Progression design — it predates both and controls how base item stats are scaled
before the tier system applies.

When implementing the tier system, the iLevel is calculated from the item's **original
database stats**, not from item_scaling.json-adjusted values. Tier multipliers then apply
on top of the base.
