# Disciplines Development Guide

## Table of Contents
1. [Overview](#overview)
2. [How Disciplines Work](#how-disciplines-work)
3. [Database Structure](#database-structure)
4. [Creating a Discipline](#creating-a-discipline)
5. [Discipline Tomes](#discipline-tomes)
6. [Endurance System](#endurance-system)
7. [Recast Timers](#recast-timers)
8. [Class-Specific Mechanics](#class-specific-mechanics)
9. [Testing and Debugging](#testing-and-debugging)

---

## Overview

### What are Disciplines?

**Disciplines** are combat abilities that:
- Cost **endurance** instead of mana
- Are learned from **tomes** (items) instead of scrolls
- Have **shared recast timers** between similar abilities
- Are primarily used by **melee classes** (Warrior, Rogue, Monk, Berserker)

**Examples from Live EQ**:
- **Defensive Strike** (Warrior): Reduce damage taken
- **Resistant Discipline** (All): Increase all resists
- **Furious Discipline** (Berserker): Increase attack speed
- **Anatomy** (Rogue): Increase backstab damage

---

## How Disciplines Work

### Disciplines vs. Spells

| Feature | Disciplines | Spells |
|---------|-------------|--------|
| **Resource** | Endurance | Mana |
| **Learning** | Discipline tomes | Spell scrolls |
| **Memorization** | Not required | Must memorize in gem |
| **Casting** | Use from combat abilities window | Cast from spell gem |
| **Classes** | Primarily melee | Primarily casters |
| **Database Flag** | `IsDiscipline = 1` | `IsDiscipline = 0` |

### Under the Hood

**Disciplines are actually spells** with special flags:

1. Spell entry with `IsDiscipline = 1`
2. `EndurCost` instead of `mana`
3. Learned and stored in character disciplines (not spell book)
4. Activated via `/disc` or combat abilities window

---

## Database Structure

### Spell Entry (spells_new table)

Disciplines use the same `spells_new` table as spells:

```sql
SELECT * FROM spells_new WHERE IsDiscipline = 1;
```

**Key Fields for Disciplines**:

```sql
id                 INT              -- Spell ID
name               VARCHAR(64)      -- Discipline name
IsDiscipline       INT              -- **MUST BE 1**
EndurCost          INT              -- Endurance cost
EndurTimerIndex    INT              -- Timer group ID
cast_time          INT              -- Cast time (usually 0 for instant)
recast_time        INT              -- Recast timer (milliseconds)
buffduration       INT              -- Duration (ticks)
classes1-16        INT              -- Level per class (255 = can't use)
effectid1-12       INT              -- Spell effects
effect_base_value1-12 INT           -- Effect values
```

### Discipline Storage (character_disciplines table)

```sql
CREATE TABLE character_disciplines (
    id        INT,           -- Character ID
    slot_id   INT,           -- Discipline slot (0-99)
    disc_id   INT            -- Spell ID of discipline
);
```

Each character can know up to **100 disciplines** (MAX_PP_DISCIPLINES).

---

## Creating a Discipline

### Example: Defensive Strike (Warrior)

**Goal**: 30-second buff that reduces melee damage taken by 25%, costs 100 endurance, 10-minute recast.

#### Step 1: Create Spell Entry

```sql
INSERT INTO spells_new (
    id, name, player_1,
    IsDiscipline,              -- **KEY: Set to 1**
    EndurCost,                 -- Endurance cost
    EndurTimerIndex,           -- Timer group
    cast_time, recast_time,
    buffduration, buffdurationformula,
    targettype, range_,
    effectid1, effect_base_value1,
    classes1                   -- Warrior level requirement
) VALUES (
    95000,                     -- Discipline spell ID
    'Defensive Strike',        -- Name
    'Reduces damage taken',    -- Description

    1,                         -- **IsDiscipline = 1**
    100,                       -- 100 endurance cost
    1,                         -- Timer group 1

    0,                         -- Instant cast
    600000,                    -- 10 minutes recast (milliseconds)

    300,                       -- 300 ticks = 30 minutes duration
    0,                         -- No formula scaling

    6,                         -- Self only
    0,                         -- Range 0 (self)

    168,                       -- SpellEffect::MeleeMitigation
    25,                        -- 25% mitigation

    25                         -- Warriors @ level 25
);
```

#### Step 2: Set Class Requirements

```sql
UPDATE spells_new SET
    classes1 = 25,    -- Warrior
    classes2 = 255,   -- Cleric (blocked)
    classes3 = 255,   -- Paladin (blocked)
    -- ... set all other classes to 255 ...
    classes16 = 255   -- Berserker (blocked)
WHERE id = 95000;
```

Or set multiple classes:
```sql
classes1 = 25,    -- Warrior @ 25
classes5 = 30,    -- Shadow Knight @ 30
classes9 = 28     -- Monk @ 28
```

#### Step 3: Create Discipline Tome

```sql
INSERT INTO items (
    id, Name, itemtype, scrolleffect, scrolltype, classes, reqlevel
) VALUES (
    200000,                    -- Item ID
    'Tome: Defensive Strike',  -- Item name
    20,                        -- **ItemType 20 = Discipline**
    95000,                     -- Discipline spell ID
    2,                         -- scrolltype 2 = Discipline
    1,                         -- Class bitmask (1 = Warrior)
    25                         -- Level requirement
);
```

#### Step 4: Test

```
/summonitem 200000       # Get the tome
Right-click tome         # Learn discipline
/disc 95000              # Use discipline
/showbuffs               # Verify buff active
```

Done!

---

## Discipline Tomes

### Item Structure

**ItemType**: 20 (Discipline)
**scrolltype**: 2 (Discipline)
**scrolleffect**: Discipline spell ID

```sql
INSERT INTO items (
    id, Name,
    itemtype,      -- **20 = Discipline**
    scrolleffect,  -- Discipline spell ID
    scrolltype,    -- **2 = Discipline**
    classes,       -- Class bitmask
    reqlevel       -- Minimum level
) VALUES (
    [item_id],
    'Tome: [Discipline Name]',
    20,
    [discipline_spell_id],
    2,
    [class_bitmask],
    [level]
);
```

### Learning Disciplines

**Code**: `zone/effects.cpp::TrainDiscipline()`

When a discipline tome is right-clicked:

1. Check if spell is valid
2. Check if player's class can use it
3. Check if player meets level requirement
4. Check if already known
5. Add to character disciplines (max 100)
6. Delete tome

### Discipline Slots

Characters have **100 discipline slots** (0-99).

Disciplines are stored in order learned and can't be removed (without GM command).

---

## Endurance System

### Endurance Pool

**Endurance** is the resource pool for disciplines and combat abilities.

**Max Endurance Formula**:
```cpp
max_end = (STR + STA + DEX + AGI) / 4  + level * 15
```

**Regeneration**:
- Base regen: 1 endurance per tick
- Modified by items/spells/AAs

### Endurance Cost

Set in `spells_new.EndurCost`:

```sql
EndurCost = 100   -- Costs 100 endurance
```

### Endurance Upkeep

Some disciplines have **upkeep** cost (drained each tick):

```sql
EndurUpkeep = 10  -- Drains 10 endurance per tick while active
```

**Example**: Berserker frenzy that drains endurance continuously.

### Code Implementation

**File**: `zone/effects.cpp::UseDiscipline()`

```cpp
bool Client::UseDiscipline(uint32 spell_id, uint32 target) {
    const auto& spell = spells[spell_id];

    // Check endurance cost
    if (GetEndurance() < spell.endurance_cost) {
        Message(Chat::Red, "Insufficient endurance.");
        return false;
    }

    // Deduct endurance
    SetEndurance(GetEndurance() - spell.endurance_cost);

    // Cast the discipline
    CastSpell(spell_id, target, EQ::spells::CastingSlot::Discipline);

    return true;
}
```

---

## Recast Timers

### Timer Groups

Disciplines use **shared timer groups** to prevent stacking:

```sql
EndurTimerIndex   -- Timer group (0-20)
recast_time       -- Recast duration (milliseconds)
```

**Timer Groups**:
- **0**: No timer (always available)
- **1-10**: Short timers (seconds to minutes)
- **11-15**: Medium timers (5-15 minutes)
- **16-20**: Long timers (15+ minutes)

### Example Timer Setup

```sql
-- Fast discipline (30s recast, timer 1)
EndurTimerIndex = 1,
recast_time = 30000

-- Medium discipline (5min recast, timer 2)
EndurTimerIndex = 2,
recast_time = 300000

-- Epic discipline (15min recast, timer 3)
EndurTimerIndex = 3,
recast_time = 900000
```

**Shared Timers**: All disciplines with same `EndurTimerIndex` share a cooldown.

### Code Implementation

**File**: `zone/effects.cpp::UseDiscipline()`

```cpp
// Check discipline timer
uint32 remain_time = GetDisciplineTimer(spell.timer_id);
if (remain_time > 0) {
    Message(Chat::Red, "You can use this in %d seconds.", remain_time);
    return false;
}

// Set discipline timer after use
SendDisciplineTimer(spell.timer_id, spell.recast_time);
```

---

## Class-Specific Mechanics

### Warrior Disciplines

**Focus**: Defense and threat

**Common Effects**:
- MeleeMitigation (reduce damage taken)
- Hate (increase threat)
- ArmorClass (increase AC)
- StunResist (reduce stun duration)

**Example**: Defensive Proficiency
```sql
effectid1 = 168,  -- MeleeMitigation
effect_base_value1 = 30  -- 30% reduction
```

### Rogue Disciplines

**Focus**: Burst damage and stealth

**Common Effects**:
- FrontalBackstabChance (backstab from front)
- CriticalHitChance (increase crits)
- DamageModifier (increase damage)
- Invisibility (stealth)

**Example**: Anatomy
```sql
effectid1 = 252,  -- FrontalBackstabChance
effect_base_value1 = 100  -- 100% chance frontal backstab
```

### Monk Disciplines

**Focus**: Defense and counterattacks

**Common Effects**:
- RiposteChance (increase riposte)
- DodgeChance (increase dodge)
- DoubleAttackChance (increase double attack)
- Flurry (extra attacks)

**Example**: Innerflame
```sql
effectid1 = 279,  -- Flurry
effect_base_value1 = 20  -- 20% flurry chance
```

### Berserker Disciplines

**Focus**: Offense and rage

**Common Effects**:
- AttackSpeed (haste)
- DamageModifier (damage bonus)
- CriticalHitChance (crit chance)
- Rampage (AE melee)

**Example**: Savage Rage
```sql
effectid1 = 11,   -- AttackSpeed
effect_base_value1 = 30,  -- 30% haste
effectid2 = 185,  -- DamageModifier
effect_base_value2 = 20   -- 20% more damage
```

### Paladin/Shadowknight/Ranger

**Hybrids** also get some disciplines:

- **Paladin**: Holy-themed defensive disciplines
- **Shadow Knight**: Unholy-themed offensive disciplines
- **Ranger**: Archery and tracking disciplines

---

## Testing and Debugging

### GM Commands

```bash
# Learn disciplines
/summonitem [tome_id]         # Get discipline tome
Right-click tome              # Learn

# Use disciplines
/disc [spell_id]              # Use discipline on target
/discipline [spell_id]        # Alias for /disc

# Manage disciplines
/untraindiscs                 # Remove all disciplines
/traindiscs [level]           # Learn all discs up to level

# Check timers
/showdiscs                    # Show learned disciplines
/memspells                    # Also shows discs

# Endurance
/setendurance [amount]        # Set endurance
/setenduranceregen [amount]   # Set regen rate
```

### Debug Logging

Enable discipline logging in `eqemu_config.json`:

```json
{
  "logging": {
    "Spells": "debug",
    "Combat": "debug"
  }
}
```

### Common Issues

#### Discipline Doesn't Learn
- Check `IsDiscipline = 1` in spell
- Verify tome has `itemtype = 20` and `scrolltype = 2`
- Check class requirements in spell
- Verify level requirement met
- Check discipline slot limit (100 max)

#### Can't Use Discipline
- Check endurance cost vs. current endurance
- Verify timer isn't on cooldown (`/showdiscs`)
- Check if already have discipline buff active
- Ensure targettype allows your target

#### Discipline Has No Effect
- Check spell effects are implemented
- Verify `effect_base_value` is non-zero
- Check for focus effects interfering
- Review `zone/spell_effects.cpp` for effect

#### Timer Doesn't Work
- Verify `EndurTimerIndex` is set (1-20)
- Check `recast_time` is in milliseconds
- Ensure timer group isn't shared unintentionally

---

## Advanced Patterns

### Stance Disciplines

Long-duration buffs that replace each other:

```sql
-- Defensive Stance
buffduration = -1,  -- Permanent until removed
effectid1 = 168,    -- MeleeMitigation
effect_base_value1 = 20,
effectid2 = 148,    -- StackingCommand_Block (prevent other stances)
effect_base_value2 = 999  -- Block stance group
```

### Reactive Disciplines

Trigger effects when hit:

```sql
effectid1 = 323,         -- DefensiveProc
effect_base_value1 = 500,  -- Proc chance
effect_limit_value1 = 90500  -- Spell to proc
```

### Resource-Draining Disciplines

Continuously drain endurance:

```sql
EndurUpkeep = 15,   -- Drains 15 endurance per tick
buffduration = 600  -- Lasts 60 minutes or until endurance depleted
```

### Multi-Effect Disciplines

Combine multiple bonuses:

```sql
effectid1 = 11,    -- AttackSpeed (30% haste)
effect_base_value1 = 30,
effectid2 = 169,   -- CriticalHitChance (10%)
effect_base_value2 = 10,
effectid3 = 185,   -- DamageModifier (15%)
effect_base_value3 = 15
```

---

## Summary

**Disciplines** are combat abilities for melee classes:

1. Create spell entry with `IsDiscipline = 1`
2. Set `EndurCost` and `EndurTimerIndex`
3. Create tome item (`itemtype = 20`, `scrolltype = 2`)
4. Add spell effects for desired bonuses
5. Test with `/summonitem` and `/disc`

**No C++ code needed** for most disciplines - they work like spells.

**Key Differences from Spells**:
- Use endurance instead of mana
- Learned from tomes instead of scrolls
- Not memorized in spell gems
- Shared recast timers

---

**Next Steps**:
- Create your first discipline
- Review live disciplines for ideas
- Read **SPELLS_GUIDE.md** for spell effects
- See **AA_GUIDE.md** for permanent abilities
