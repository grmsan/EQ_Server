# Alternate Advancement (AA) Development Guide

## Table of Contents
1. [Overview](#overview)
2. [AA System Architecture](#aa-system-architecture)
3. [Database Structure](#database-structure)
4. [Creating Passive AAs](#creating-passive-aas)
5. [Creating Active AAs](#creating-active-aas)
6. [AA Effects](#aa-effects)
7. [Rank Progression](#rank-progression)
8. [Prerequisites](#prerequisites)
9. [Testing and Debugging](#testing-and-debugging)
10. [Common Patterns](#common-patterns)

---

## Overview

### What are Alternate Advancements?

**Alternate Advancements (AAs)** are purchasable permanent character upgrades. Players spend AA points (earned like experience) to unlock and rank up abilities.

**Two Types**:
1. **Passive AAs**: Always active bonuses (stat increases, skill bonuses, etc.)
2. **Active AAs**: Clickable abilities with cooldowns (like spells)

### AA vs. Spells

| Feature | AAs | Spells |
|---------|-----|--------|
| **Duration** | Permanent | Temporary or instant |
| **Cost** | AA points (one-time) | Mana/Endurance (repeated) |
| **Activation** | Automatic or hotkey | Must memorize and cast |
| **Effects** | Can use spell effects OR custom bonuses | Spell effects only |

---

## AA System Architecture

### Data Flow

```
1. aa_ability (Base Definition)
   ↓
2. aa_ranks (Individual Ranks)
   ↓
3. aa_rank_effects (Effects Per Rank)
   ↓
4. aa_rank_prereqs (Unlock Requirements)
   ↓
5. character_alternate_abilities (Player Purchases)
   ↓
6. Zone Server Loads & Applies (zone/aa.cpp, zone/bonuses.cpp)
```

### Key Concepts

- **Ability**: The base AA (e.g., "Combat Fury")
- **Rank**: Each purchase level (Rank 1, Rank 2, Rank 3)
- **Effect**: Individual bonuses/spells in a rank
- **Prerequisite**: Required AAs before unlocking

---

## Database Structure

### Table 1: `aa_ability`

**Purpose**: Base definition of the AA ability

**Key Fields**:
```sql
id                 INT PRIMARY KEY  -- Unique ability ID
name               VARCHAR(128)     -- AA name shown to player
category           INT              -- AA category/tab
classes            INT              -- Class bitmask
type               INT              -- 1=General, 2=Archetype, etc.
charges            INT              -- Expendable AA charges (0=unlimited)
grant_only         TINYINT          -- 1=Can't purchase, only granted
first_rank_id      INT              -- ID of rank 1
enabled            TINYINT          -- 1=Active, 0=Disabled
reset_on_death     TINYINT          -- 1=Lost on death
```

### Table 2: `aa_ranks`

**Purpose**: Individual ranks for each AA

**Key Fields**:
```sql
id                 INT PRIMARY KEY  -- Unique rank ID
title_sid          INT              -- String ID for title
desc_sid           INT              -- String ID for description
cost               INT              -- AA point cost for this rank
level_req          INT              -- Minimum character level
spell              INT              -- Spell ID (active AAs) or -1 (passive)
spell_type         INT              -- Recast timer ID
recast_time        INT              -- Recast time (seconds)
expansion          INT              -- Expansion required
prev_id            INT              -- Previous rank ID (-1 for rank 1)
next_id            INT              -- Next rank ID (-1 for max rank)
```

### Table 3: `aa_rank_effects`

**Purpose**: Effects applied by each rank

**Key Fields**:
```sql
rank_id            INT              -- Links to aa_ranks.id
slot               INT              -- Effect slot (1-12)
effect_id          INT              -- Spell Effect ID (SPA)
base1              INT              -- Effect base value
base2              INT              -- Effect limit value
```

### Table 4: `aa_rank_prereqs`

**Purpose**: Requirements to unlock an AA

**Key Fields**:
```sql
rank_id            INT              -- Rank requiring prerequisite
prereq_rank_id     INT              -- Required rank ID
prereq_points      INT              -- Points in required ability
```

### Table 5: `character_alternate_abilities`

**Purpose**: Tracks player's purchased AAs

**Key Fields**:
```sql
id                 INT              -- Character ID
aa_id              INT              -- Ability ID
aa_value           INT              -- Total points spent
charges            INT              -- Remaining charges (expendable AAs)
```

---

## Creating Passive AAs

### Example: Combat Fury (Critical Hit Chance)

**Goal**: 3-rank AA that increases critical hit chance by 2% per rank.

#### Step 1: Create Ability

```sql
INSERT INTO aa_ability (
    id, name, category, classes, type, charges, grant_only, first_rank_id, enabled
) VALUES (
    1000,                          -- Custom AA ID
    'Combat Fury',                 -- Display name
    1,                             -- Category: Combat
    65534,                         -- Classes: All melee (bitmask)
    1,                             -- Type: General
    0,                             -- Unlimited charges
    0,                             -- Can purchase normally
    10001,                         -- First rank ID
    1                              -- Enabled
);
```

**Class Bitmask**:
```
Warrior     = 1 << 0  = 1
Cleric      = 1 << 1  = 2
Paladin     = 1 << 2  = 4
Ranger      = 1 << 3  = 8
...
All Classes = 65534 (excludes unknown class 0)
```

#### Step 2: Create Ranks

```sql
-- Rank 1
INSERT INTO aa_ranks (
    id, title_sid, desc_sid, cost, level_req,
    spell, spell_type, recast_time, expansion, prev_id, next_id
) VALUES (
    10001,                         -- Rank ID
    -1,                            -- Title (not used)
    -1,                            -- Description (not used)
    2,                             -- 2 AA points
    51,                            -- Level 51 required
    -1,                            -- No spell (passive AA)
    0,                             -- No timer
    0,                             -- No recast
    0,                             -- Base expansion
    -1,                            -- No previous rank
    10002                          -- Next rank ID
);

-- Rank 2
INSERT INTO aa_ranks (
    id, title_sid, desc_sid, cost, level_req,
    spell, spell_type, recast_time, expansion, prev_id, next_id
) VALUES (
    10002, -1, -1, 4, 55, -1, 0, 0, 0, 10001, 10003
);

-- Rank 3
INSERT INTO aa_ranks (
    id, title_sid, desc_sid, cost, level_req,
    spell, spell_type, recast_time, expansion, prev_id, next_id
) VALUES (
    10003, -1, -1, 6, 59, -1, 0, 0, 0, 10002, -1  -- No next rank
);
```

#### Step 3: Create Effects

```sql
-- Rank 1: +2% Critical Hit Chance
INSERT INTO aa_rank_effects (rank_id, slot, effect_id, base1, base2) VALUES
(10001, 1, 169, 2, 0);  -- SpellEffect::CriticalHitChance

-- Rank 2: +4% Critical Hit Chance
INSERT INTO aa_rank_effects (rank_id, slot, effect_id, base1, base2) VALUES
(10002, 1, 169, 4, 0);

-- Rank 3: +6% Critical Hit Chance
INSERT INTO aa_rank_effects (rank_id, slot, effect_id, base1, base2) VALUES
(10003, 1, 169, 6, 0);
```

#### Step 4: Test In-Game

```
/grantaa 1000 3    # Grant all 3 ranks
/alt list          # Show AA window
/showstats         # Verify crit chance increased
```

**Done!** The AA is fully functional.

---

## Creating Active AAs

### Example: Fury (Damage Boost Click)

**Goal**: Clickable AA that casts a 30-second damage buff with 10-minute recast.

#### Step 1: Create Spell

Active AAs require a spell entry:

```sql
INSERT INTO spells_new (
    id, name, player_1,
    cast_time, recast_time, recovery_time,
    buffduration, targettype,
    effectid1, effect_base_value1
) VALUES (
    90100,                         -- Spell ID
    'Fury',                        -- Name
    'Increases melee damage',      -- Description
    0,                             -- Instant cast
    0,                             -- Recast handled by AA
    0,                             -- No recovery
    300,                           -- 300 ticks = 30 minutes
    6,                             -- Self only
    124,                           -- SpellEffect::ImprovedDamage
    15                             -- 15% damage increase
);
```

#### Step 2: Create Ability & Rank

```sql
-- Ability
INSERT INTO aa_ability (
    id, name, category, classes, type, charges, grant_only, first_rank_id, enabled
) VALUES (
    1001, 'Fury', 1, 65534, 1, 0, 0, 10010, 1
);

-- Rank (Active AA)
INSERT INTO aa_ranks (
    id, title_sid, desc_sid, cost, level_req,
    spell, spell_type, recast_time, expansion, prev_id, next_id
) VALUES (
    10010,                         -- Rank ID
    -1, -1,
    5,                             -- 5 AA points
    60,                            -- Level 60
    90100,                         -- **Spell ID** (makes it active)
    1,                             -- **Timer ID** (1 = AA timer 1)
    600,                           -- **Recast: 600 seconds (10 min)**
    0,
    -1, -1
);
```

**Note**: `spell != -1` makes it an **active** AA.

#### Step 3: Effects (Optional)

Active AAs usually don't need `aa_rank_effects` because the spell has effects.

But you can add passive bonuses too:

```sql
-- Rank also grants +5 ATK passively
INSERT INTO aa_rank_effects (rank_id, slot, effect_id, base1, base2) VALUES
(10010, 1, 2, 5, 0);  -- SpellEffect::ATK
```

#### Step 4: Test In-Game

```
/grantaa 1001 1           # Grant the AA
/alt list                 # Find it in AA window
Click the AA button       # Activates spell
/showbuffs                # Should see "Fury" buff
```

---

## AA Effects

### Effect Application

AA effects are applied via **`zone/bonuses.cpp::ApplyAABonuses()`**.

**Code Flow**:
```cpp
void Mob::ApplyAABonuses(const AA::Rank &rank, StatBonuses *newbon) {
    for (auto &effect : rank.effects) {
        int effect_id = effect.effect_id;
        int base_value = effect.base_value;
        int limit_value = effect.limit_value;

        switch(effect_id) {
            case SpellEffect::STR:
                newbon->STR += base_value;
                break;
            case SpellEffect::CriticalHitChance:
                newbon->CriticalHitChance += base_value;
                break;
            // ... hundreds more cases ...
        }
    }
}
```

### Supported Effects

**Most spell effects work in AAs**, but some require special handling:

#### Fully Supported
- Stat bonuses (STR, DEX, AGI, STA, INT, WIS, CHA)
- Combat bonuses (ATK, AC, CriticalHitChance, etc.)
- Skill modifiers (RaiseSkillCap, etc.)
- Defensive bonuses (MeleeMitigation, DodgeChance, etc.)

#### Partially Supported
- Spell effects that need active casting (require `spell` field in rank)
- Effects with complex logic (may need C++ modifications)

#### Not Supported
- Effects that only work in spells (like CurrentHP instant heals)

### Creating Custom AA Effects

If you need a unique AA effect not covered by existing SPAs:

1. Add a new SpellEffect constant in `common/spdat.h`
2. Implement in `zone/bonuses.cpp::ApplyAABonuses()`
3. Use in `aa_rank_effects` table

**Example**: Custom AA effect for bonus tradeskill XP

```cpp
// common/spdat.h
namespace SpellEffect {
    constexpr int BonusTradeskillXP = 500;  // Custom effect
}

// zone/bonuses.cpp::ApplyAABonuses()
case SpellEffect::BonusTradeskillXP:
    newbon->TradeskillXPBonus += base_value;
    break;
```

Then use in database:
```sql
INSERT INTO aa_rank_effects (rank_id, slot, effect_id, base1, base2) VALUES
(10020, 1, 500, 10, 0);  -- +10% tradeskill XP
```

---

## Rank Progression

### Linear Progression

Most AAs have linear cost and effect increases:

```sql
-- Rank 1: 2 points, +5 STR
-- Rank 2: 4 points, +10 STR
-- Rank 3: 6 points, +15 STR
```

### Non-Linear Progression

Some AAs have accelerating costs or effects:

```sql
-- Rank 1: 1 point, +1% haste
-- Rank 2: 2 points, +2% haste
-- Rank 3: 4 points, +5% haste
-- Rank 4: 8 points, +10% haste
```

### Max Ranks

**Set `next_id = -1`** for the final rank.

Players can't purchase beyond max rank.

### Skill-Up Style AAs

Some AAs auto-rank as you use them (not implemented in base EQEmu).

---

## Prerequisites

### Simple Prerequisites

Require another AA before unlocking:

```sql
-- Must have Rank 3 of AA 1000 before buying AA 1001
INSERT INTO aa_rank_prereqs (rank_id, prereq_rank_id, prereq_points) VALUES
(10020, 10003, 3);  -- Rank 10020 requires 3 points in rank 10003's ability
```

### Multiple Prerequisites

AAs can have multiple prerequisites (all must be met):

```sql
INSERT INTO aa_rank_prereqs (rank_id, prereq_rank_id, prereq_points) VALUES
(10030, 10003, 3),   -- Requires AA 1000 rank 3
(10030, 10010, 1);   -- AND AA 1001 rank 1
```

### Level Prerequisites

Set in `aa_ranks.level_req`:

```sql
level_req = 65   -- Must be level 65
```

### Class Prerequisites

Set in `aa_ability.classes` bitmask.

---

## Testing and Debugging

### GM Commands

```bash
# Grant AAs
/grantaa [aa_id] [points]     # Grant specific AA
/setaaxp [points]             # Set total AA points
/refundaa                     # Refund all AAs

# View AAs
/alt list                     # Open AA window
/aainfo [aa_id]               # Show AA details

# Test Active AAs
Click AA in hotbar            # Activate
/reloadaa                     # Reload AA data from database
```

### Verify AA Applied

```bash
/showstats                    # Show stat bonuses
/showbonuses                  # Detailed bonus breakdown
```

Look for `AA:` bonuses in output.

### Enable AA Logging

**File**: `eqemu_config.json`

```json
{
  "logging": {
    "AA": "debug"
  }
}
```

Restart server. AA purchases/activations will log.

### Common Issues

#### AA Doesn't Appear in Window
- Check `aa_ability.enabled = 1`
- Verify `classes` bitmask includes your class
- Check `level_req` isn't too high
- Ensure `first_rank_id` is correct

#### AA Effects Not Applying
- Verify effect is implemented in `ApplyAABonuses()`
- Check `aa_rank_effects` has correct `rank_id`
- Ensure `effect_id` is valid SPA
- Try `/reloadaa` then re-purchase

#### Active AA Doesn't Cast
- Check `spell` field is valid spell ID
- Ensure spell exists in `spells_new`
- Verify `targettype` in spell allows self-cast
- Check for errors in server console

#### Can't Purchase AA
- Check prerequisites in `aa_rank_prereqs`
- Verify level requirement
- Ensure you have enough AA points
- Check if `grant_only = 1` (can't buy, only granted)

---

## Common Patterns

### Pattern 1: Incremental Stat Bonus

**Example**: Planar Power (3 ranks, +5/10/15 to all stats)

```sql
-- Ability
INSERT INTO aa_ability VALUES (2000, 'Planar Power', 1, 65534, 1, 0, 0, 20001, 1, 0, 0);

-- Ranks
INSERT INTO aa_ranks VALUES (20001, -1, -1, 3, 55, -1, 0, 0, 0, -1, 20002);
INSERT INTO aa_ranks VALUES (20002, -1, -1, 6, 60, -1, 0, 0, 0, 20001, 20003);
INSERT INTO aa_ranks VALUES (20003, -1, -1, 9, 65, -1, 0, 0, 0, 20002, -1);

-- Effects (All stats)
INSERT INTO aa_rank_effects VALUES
(20001, 1, 159, 5, 0),   -- Rank 1: +5 AllStats
(20002, 1, 159, 10, 0),  -- Rank 2: +10 AllStats
(20003, 1, 159, 15, 0);  -- Rank 3: +15 AllStats
```

### Pattern 2: Improved Skill Cap

**Example**: Planar Durability (raise HP cap by 50 per rank)

```sql
INSERT INTO aa_rank_effects VALUES
(20010, 1, 262, 50, 0);   -- SpellEffect::RaiseStatCap, +50 HP
```

### Pattern 3: Skill-Based Proc

**Example**: Furious Rampage (chance to rampage on taunt)

```sql
-- Spell to proc
INSERT INTO spells_new (id, name, cast_time, targettype, effectid1, effect_base_value1)
VALUES (90200, 'Rampage', 0, 6, 205, 3);  -- Rampage 3 targets

-- AA Rank
INSERT INTO aa_ranks VALUES (20020, -1, -1, 5, 60, -1, 0, 0, 0, -1, -1);

-- Effect: Proc on taunt use
INSERT INTO aa_rank_effects VALUES
(20020, 1, 427, 20, 38, 90200);
-- SkillProcAttempt: 20% chance, skill 38 (Taunt), spell 90200
```

### Pattern 4: Expendable AA

**Example**: Harm Touch (1 charge, resets on death)

```sql
-- Ability (1 charge)
INSERT INTO aa_ability VALUES
(2010, 'Harm Touch', 1, 65534, 1, 1, 0, 20030, 1, 1, 0);
--                                      ^ charges  ^ reset on death

-- Active AA with spell
INSERT INTO aa_ranks VALUES
(20030, -1, -1, 0, 1, 88, 1, 4320, 0, -1, -1);
--                 ^ 0 cost    ^ Spell: Harm Touch, 72min recast
```

### Pattern 5: Toggle Passive AA

**Example**: Weapon Stance (enable/disable bonus based on weapon equipped)

```sql
-- Disabled Rank (what you buy)
INSERT INTO aa_ranks VALUES (20040, -1, -1, 5, 60, 90300, 0, 0, 0, -1, 20041);
--                                                ^ spell with Buy_AA_Rank effect

-- Enabled Rank (activated when equipped)
INSERT INTO aa_ranks VALUES (20041, -1, -1, 0, 60, 90301, 0, 0, 0, 20040, -1);

-- Spell for disabled rank
INSERT INTO spells_new (id, effectid1, effect_base_value1) VALUES
(90300, 472, 1);  -- SpellEffect::Buy_AA_Rank

-- Spell for enabled rank (the buff applied)
INSERT INTO spells_new (id, effectid1, effect_base_value1) VALUES
(90301, 2, 50);   -- +50 ATK

-- Effect that detects weapon type
INSERT INTO aa_rank_effects VALUES
(20041, 1, 476, 90301, 0);  -- Weapon_Stance: spell 90301, 2H weapon (0)
```

---

## Advanced Topics

### AA Categories

Categories group AAs in the UI:

```sql
category = 1   -- General (all classes)
category = 2   -- Archetype (class type)
category = 3   -- Class (specific class)
category = 4   -- Special
```

### AA Types

```sql
type = 1       -- General
type = 2       -- Archetype
type = 3       -- Class
type = 4       -- Special
type = 5       -- Focus
```

### Grant-Only AAs

Some AAs can't be purchased, only granted by quests/events:

```sql
grant_only = 1  -- Can't purchase
```

Use `/grantaa` to give these.

### Auto-Grant AAs

```sql
auto_grant_enabled = 1  -- Automatically grant to eligible characters
```

Not widely used.

### AA Points Calculation

AA points are earned like experience. The amount required is defined in `altadv_vars`:

```sql
SELECT * FROM altadv_vars WHERE skill_id = 0;
```

---

## Summary

AA System Overview:
1. **Create ability** in `aa_ability`
2. **Create ranks** in `aa_ranks`
3. **Add effects** in `aa_rank_effects`
4. **(Optional) Add prerequisites** in `aa_rank_prereqs`

**Passive AAs**: Set `spell = -1` in rank
**Active AAs**: Set `spell = [spell_id]` in rank

Most AAs work purely from database. Custom effects require modifying `zone/bonuses.cpp::ApplyAABonuses()`.

---

**Next Steps**:
- Create your first passive AA
- Review live AAs in database for examples
- Read **DISCIPLINES_GUIDE.md** for combat abilities
- See **SPELL_EFFECTS_REFERENCE.md** for effect IDs
