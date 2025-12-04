# Complete Spell Development Guide

## Table of Contents
1. [Overview](#overview)
2. [Database Structure](#database-structure)
3. [Spell Effects (SPAs)](#spell-effects-spas)
4. [Creating a New Spell](#creating-a-new-spell)
5. [Implementing Custom Spell Effects](#implementing-custom-spell-effects)
6. [Focus Effects](#focus-effects)
7. [Spell Stacking](#spell-stacking)
8. [Client Synchronization](#client-synchronization)
9. [Testing and Debugging](#testing-and-debugging)

---

## Overview

### What is a Spell?

In EQEmulator, spells are the foundation of all magical abilities, buffs, debuffs, heals, and damage effects. Each spell is a database entry with up to **236 fields** that define its behavior.

**Spells are loaded into shared memory at server startup** and accessed by all zone processes.

### Spell Lifecycle

```
1. Spell Entry Created in Database (spells_new table)
2. Shared Memory Loads Spell Data (shared_memory.exe)
3. Player Casts Spell (Client sends OP_CastSpell packet)
4. Server Validates Cast (zone/spells.cpp::CastSpell())
5. Spell Effects Applied (zone/spell_effects.cpp::SpellEffect())
6. Buffs Tracked (Mob::AddBuff() if duration > 0)
7. Effects Wear Off (Mob::BuffFadeBySlot())
```

---

## Database Structure

### Primary Table: `spells_new`

**Location**: Database table `spells_new`
**Columns**: ~236 fields
**Repository**: `common/repositories/spells_new_repository.h`

### Key Fields

#### Basic Information
```sql
id                INT PRIMARY KEY    -- Unique spell ID (1-65535)
name              VARCHAR(64)        -- Spell name
player_1          TEXT               -- Description shown to player
descnum           INT                -- Description string ID (db_str)
typedescnum       INT                -- Type/primary category string ID (db_str)
effectdescnum     INT                -- Effect/secondary category string ID (db_str)
effectdescnum2    INT                -- Additional effect description (db_str)
```

#### Cast Properties
```sql
cast_time         INT                -- Cast time in milliseconds
recovery_time     INT                -- Time before next action (ms)
recast_time       INT                -- Cooldown before recasting (ms)
mana              INT                -- Mana cost
EndurCost         INT                -- Endurance cost (disciplines)
```

#### Targeting & Range
```sql
targettype        INT                -- Who can be targeted (see TargetTypes enum)
range             INT                -- Max casting range
aoerange          INT                -- AE effect radius
```

#### Duration & Formulas
```sql
buffduration      INT                -- Base duration in TICKS (6 seconds each)
buffdurationformula INT              -- Formula ID for scaling duration
```

#### Effects (12 Slots)
```sql
effectid1         INT                -- Spell Effect ID (SPA)
effect_base_value1 INT               -- Base value for effect
effect_limit_value1 INT              -- Limit/modifier value
effect_max1       INT                -- Max value at high levels
formula1          INT                -- Scaling formula

-- Repeat for effectid2-12, effect_base_value2-12, etc.
```

#### Restrictions
```sql
classes1-16       INT                -- Level required per class (255 = can't use)
skill             INT                -- Associated skill (Evocation, Conjuration, etc.)
resisttype        INT                -- Resist check type
```

#### Flags
```sql
IsDiscipline      INT                -- 1 = Discipline, 0 = Spell
goodEffect        INT                -- 0 = Detrimental, 1 = Beneficial
uninterruptable   INT                -- 1 = Can't be interrupted
not_focusable     INT                -- 1 = Can't be improved by focus effects
```

### Example Spell Entry

```sql
-- Heal spell example
INSERT INTO spells_new (
    id, name, player_1, teleport_zone,
    cast_time, recast_time, recovery_time,
    mana, range, targettype, buffduration,
    effectid1, effect_base_value1, effect_limit_value1, formula1,
    classes2, classes6, classes10  -- CLR, DRU, SHM
) VALUES (
    90000,                          -- Custom spell ID
    'Greater Heal',                 -- Spell name
    'Heals your target',            -- Description
    '',                             -- No teleport
    2500,                           -- 2.5 second cast
    0,                              -- No recast timer
    1500,                           -- 1.5s recovery
    250,                            -- 250 mana cost
    200,                            -- 200 unit range
    5,                              -- Single target
    0,                              -- Instant (no duration)
    0,                              -- SpellEffect::CurrentHP (heal)
    500,                            -- Base 500 HP heal
    0,                              -- No limit
    100,                            -- Formula 100 (scales with level)
    15, 19, 22                      -- CLR@15, DRU@19, SHM@22
);
```

---

## Spell Effects (SPAs)

### What are Spell Effects?

**Spell Effect Attributes (SPAs)** are the atomic building blocks of all spells. Each spell can have up to **12 effects** applied simultaneously.

Effects are defined in **`common/spdat.h`** in the `SpellEffect` namespace.

### Common Effects

| SPA ID | Name | Description | Parameters |
|--------|------|-------------|------------|
| 0 | CurrentHP | Direct heal/damage | base1 = HP (+ heal, - damage) |
| 1 | ArmorClass | Modify AC | base1 = AC amount |
| 2 | ATK | Modify attack | base1 = ATK amount |
| 3 | MovementSpeed | Run speed | base1 = % (positive faster, negative slower) |
| 4-10 | STR, DEX, AGI, STA, INT, WIS, CHA | Stat modifications | base1 = stat points |
| 11 | AttackSpeed | Haste/slow | base1 = % (positive haste, negative slow) |
| 15 | CurrentMana | Direct mana | base1 = mana (+ gain, - drain) |
| 21 | Stun | Stun target | base1 = duration (milliseconds) |
| 22 | Charm | Charm mob | base1 = duration modifier |
| 31 | Mez | Mesmerize | base1 = duration modifier |
| 55 | Rune | Absorb damage | base1 = damage absorbed |
| 59 | DamageShield | DS on target | base1 = damage per hit |
| 79 | CurrentHPOnce | Non-repeating heal/nuke | base1 = HP |
| 99 | Root | Immobilize | base1 = duration modifier |
| 100 | HealOverTime | HoT | base1 = HP per tick |

See **SPELL_EFFECTS_REFERENCE.md** for complete list of all ~500 effects.

### Effect Value Fields

Each effect slot has **4 value fields**:

1. **base_value** (base1): Primary value
2. **limit_value** (base2): Secondary value or restriction
3. **max** (max1): Maximum value at high levels
4. **formula**: Scaling formula ID

### Formulas

Formulas scale spell effects based on caster level:

- **Formula 0**: No scaling (flat value)
- **Formula 100**: Linear scaling per level
- **Formula 101**: Level * 2
- **Formula 102**: Level * 3
- Etc. (see `common/spdat.cpp::CalcSpellEffectValue()`)

**Example**: Heal with formula 100
```
Base = 100, Formula = 100
Level 50: Heal = 100 + (50 * scaling_factor)
```

---

## Creating a New Spell

### Step-by-Step Process

#### 1. Design the Spell

**Questions to answer**:
- What does it do? (Damage, heal, buff, debuff, utility?)
- Who can cast it? (Class restrictions)
- What's the cost? (Mana, endurance, reagents)
- How long does it last? (Instant, duration, permanent)
- Can it stack with other spells?

#### 2. Choose Spell ID

**Spell ID Ranges**:
- **1-40000**: Live EQ spells (avoid conflicts)
- **40000-60000**: PEQ custom spells
- **60000+**: Safe for custom server spells

**Find next available ID**:
```sql
SELECT MAX(id) + 1 FROM spells_new;
```

#### 3. Insert Database Entry

```sql
INSERT INTO spells_new (
    id, name, player_1,
    cast_time, recast_time, mana,
    range, targettype, buffduration,
    effectid1, effect_base_value1,
    classes1  -- Warrior level requirement
) VALUES (
    90001,
    'Test Buff',
    'Increases your strength',
    3000,      -- 3 second cast
    60000,     -- 60 second recast
    100,       -- 100 mana
    0,         -- Self only
    6,         -- Self
    600,       -- 600 ticks = 1 hour
    4,         -- SpellEffect::STR
    50,        -- +50 STR
    255        -- Warriors can't use (255 = blocked)
);
```

#### 4. Test In-Game

```
/castspell 90001
```

If it works, you're done! Most spells require no C++ code.

#### 5. Create Scroll/Tome (Optional)

```sql
INSERT INTO items (id, Name, itemtype, scrolleffect, classes) VALUES (
    100000,
    'Spell: Test Buff',
    11,        -- ItemType 11 = Spell Scroll
    90001,     -- Spell ID
    65535      -- All classes can scribe
);
```

---

## Implementing Custom Spell Effects

### When You Need C++ Code

Most spells work purely from database values. You need custom C++ only if:

1. **Creating a new spell effect** (new SPA)
2. **Custom mechanics** not covered by existing effects
3. **Complex interactions** between multiple systems
4. **Server-side calculations** beyond simple formulas

### Adding a New Spell Effect

**Location**: `zone/spell_effects.cpp`

#### 1. Add Effect Constant

**File**: `common/spdat.h`

```cpp
namespace SpellEffect {
    // ... existing effects ...
    constexpr int CustomEffect = 500;  // Use next available ID
}
```

#### 2. Implement Effect Logic

**File**: `zone/spell_effects.cpp::SpellEffect()`

This is a massive switch statement handling all effects:

```cpp
bool Mob::SpellEffect(Mob* caster, uint16 spell_id, float partial, ...) {
    // ... setup code ...

    for (i = 0; i < EFFECT_COUNT; i++) {
        if (IsBlankSpellEffect(spell_id, i))
            continue;

        effect = spell.effect_id[i];
        effect_value = CalcSpellEffectValue(spell_id, i, ...);

        switch(effect) {

            // ... hundreds of cases ...

            case SpellEffect::CustomEffect: {
                // Your custom logic here
                int custom_value = effect_value;
                int limit_value = spell.effect_limit_value[i];

                // Example: Apply a custom bonus
                if (IsClient()) {
                    CastToClient()->Message(
                        Chat::Yellow,
                        "Custom effect applied: %d",
                        custom_value
                    );
                }

                // Modify stats, add bonuses, etc.
                bonuses.CustomBonus += custom_value;
                break;
            }
        }
    }
}
```

#### 3. Apply Bonuses (For Buffs)

If your effect is a stat bonus that should persist:

**File**: `zone/bonuses.cpp::ApplySpellsBonuses()`

```cpp
void Mob::ApplySpellsBonuses(...) {
    // ... existing code ...

    // Your custom bonus handling
    if (spellbonuses.CustomBonus) {
        // Apply the bonus somewhere relevant
        ATK += spellbonuses.CustomBonus;
    }
}
```

#### 4. Update StatBonuses Structure

**File**: `zone/client.h` or `zone/mob.h`

```cpp
struct StatBonuses {
    // ... existing bonuses ...
    int CustomBonus;  // Add your new bonus field
};
```

### Example: Damage Shield that Scales with STR

```cpp
case SpellEffect::DamageShield: {
    // Base DS value
    int ds_value = effect_value;

    // If limit = 1, scale with caster's STR
    if (spell.effect_limit_value[i] == 1 && caster) {
        int caster_str = caster->GetSTR();
        ds_value += (caster_str / 10);  // +1 DS per 10 STR
    }

    newbon->DamageShield += ds_value;
    break;
}
```

---

## Focus Effects

### What are Focus Effects?

**Focus effects** are spells/items/AAs that **modify other spells** you cast. They use two types of SPAs:

1. **Limit SPAs** (Spell Focus Filters): Define which spells can be affected
2. **Modification SPAs** (Spell Focus Changers): Define how they're modified

### Common Focus Limits

| SPA | Name | Description |
|-----|------|-------------|
| 134 | LimitMaxLevel | Max spell level to affect |
| 135 | LimitResist | Limit by resist type |
| 136 | LimitTarget | Limit by target type |
| 137 | LimitEffect | Limit by spell effect |
| 139 | LimitSpell | Specific spell ID |
| 142 | LimitMinLevel | Min spell level to affect |

### Common Focus Modifications

| SPA | Name | Description |
|-----|------|-------------|
| 124 | ImprovedDamage | Increase damage % |
| 125 | ImprovedHeal | Increase healing % |
| 127 | IncreaseSpellHaste | Reduce cast time % |
| 132 | ReduceManaCost | Reduce mana cost % |
| 155 | SpellCritDmgIncrease | Increase crit damage |

### Example: Improved Healing Focus

```sql
-- +10% healing on all heal spells level 1-60
INSERT INTO spells_new (
    id, name,
    buffduration,
    effectid1, effect_base_value1,  -- Improved Heal
    effectid2, effect_base_value2,  -- Limit: Max Level
    effectid3, effect_base_value3   -- Limit: Effect Type
) VALUES (
    90002,
    'Improved Healing Focus',
    -1,              -- Permanent (worn item effect)
    125,             -- ImprovedHeal
    10,              -- 10% bonus
    134,             -- LimitMaxLevel
    60,              -- Level 60 max
    137,             -- LimitEffect
    0                -- Limit to CurrentHP (heal) effects
);
```

### How Focus Effects Work

**Code Location**: `zone/spells.cpp::CalcFocusEffect()`

1. When you cast a spell, server iterates through all your active buffs/item effects
2. For each focus effect found:
   - Check all Limit SPAs (all must pass)
   - If limits pass, apply Modification SPAs
3. All passing focus effects stack additively

**Example Flow**:
```
Cast Heal (500 HP)
→ Check Focus: Improved Healing +10%
  → Limit Check: Spell Level ≤ 60? YES
  → Limit Check: Effect is CurrentHP? YES
  → Apply: 500 * 1.10 = 550 HP
→ Check Focus: Healing Adept +5%
  → Apply: 550 * 1.05 = 577 HP
Final: 577 HP heal
```

---

## Spell Stacking

### Stacking Rules

EQEmulator uses **effect-based stacking**. Buffs stack if:

1. Different spell effects (SPA IDs)
2. Same effect from items + spells (different sources)
3. Manually configured as stackable

Buffs **DO NOT** stack if:
- Same effect, same source type (spell/item/AA)
- One buff is strictly better in all aspects

### Stacking Commands

Special SPAs control stacking:

- **148**: StackingCommand_Block - Prevents certain buffs
- **149**: StackingCommand_Overwrite - Forces overwrite
- **446-449**: AStacker, BStacker, CStacker, DStacker - Stackability groups

### Example: Block Haste Stacking

```sql
-- Create a slow that blocks all haste
effectid1 = 11,        -- AttackSpeed (slow)
effect_base_value1 = -30,  -- 30% slow
effectid2 = 148,       -- StackingCommand_Block
effect_base_value2 = 11    -- Block all AttackSpeed buffs
```

### Testing Stacking

```
/buff [spell_id]    # Apply buff to yourself
/nobuff             # Remove all buffs
```

Try casting conflicting buffs and observe which persists.

---

## Client Synchronization

### Spell Data Flow

1. **Shared Memory** loads spells from database
2. **Client** receives spell data in two ways:
   - **Embedded**: Client has internal spell file
   - **Dynamic**: Server sends spell data packets

Most servers use **client-embedded** spell files. Server just references spell IDs.

### Spell Packets

**OP_MemorizeSpell**: Client memorizes spell
**OP_CastSpell**: Client initiates cast
**OP_SpellEffect**: Server sends visual effects
**OP_Action**: Server sends combat animations
**OP_Buff**: Server sends buff updates

### Custom Spell Compatibility

**Problem**: Custom spell IDs > client's max spell ID won't display correctly.

**Solutions**:
1. **Overwrite unused spells**: Find unused spell IDs in client range
2. **Use spell editor**: Modify client's spell file
3. **Dynamic spell loading**: Send spell data via packets (advanced)

### Spell File Editing

**Tool**: EQ Spell Editor (community tool)
**File**: `spells_us.txt` or `spells_us_new.txt` in client directory

You can edit client spell file to match server database.

---

## Testing and Debugging

### GM Commands

```bash
# Cast spells
/castspell [spell_id]         # Cast on yourself
/cast [spell_id]              # Cast on target
/targetspell [spell_id]       # Cast on target from target

# Buff management
/buff [spell_id]              # Apply buff (bypasses casting)
/buffme [spell_id]            # Apply buff on self
/nobuff                       # Remove all buffs

# Spell info
/spellinfo [spell_id]         # Show spell details
/spellinfo [spell_name]       # Search by name

# Memory slots
/memspell [spell_id] [slot]   # Memorize spell in slot (0-15)
/unscribespells               # Clear all memorized spells
```

### Debugging Spell Effects

#### Enable Spell Effect Logging

**File**: `zone/spell_effects.cpp`

Uncomment this at the top:
```cpp
#define SPELL_EFFECT_SPAM
```

Rebuild server. Spell effect applications will log to console.

#### Check Spell Application

```cpp
// Add temporary logging
Log(Logs::General, Logs::Spells,
    "Spell %d effect %d applied: value=%d",
    spell_id, effect, effect_value
);
```

#### Verify Bonuses

```
/showstats    # Show your current stat bonuses
/mystats      # Show detailed stats breakdown
```

### Common Issues

#### Spell Doesn't Cast
- Check `classes1-16` fields (255 = blocked)
- Verify mana cost
- Check skill requirements
- Ensure `targettype` allows your target

#### Spell Effect Not Working
- Verify effect ID is valid
- Check if effect is implemented (see `spell_effects.cpp`)
- Ensure `effect_base_value` is non-zero
- Check for focus effects modifying it

#### Buff Doesn't Stack
- Review stacking rules
- Check for StackingCommand effects
- Ensure different effect types

#### Wrong Damage/Healing Amount
- Check `formula` field
- Verify focus effects aren't modifying it
- Check for resist/partial resists
- Review `CalcSpellEffectValue()` in `spdat.cpp`

### Spell Testing Checklist

- [ ] Spell casts successfully
- [ ] Mana cost is correct
- [ ] Cast time feels right
- [ ] Recast timer works
- [ ] Spell lands on target
- [ ] Effects apply correctly
- [ ] Buff icon shows (if applicable)
- [ ] Duration is correct
- [ ] Stacking rules work as intended
- [ ] Focus effects modify as expected
- [ ] Resists work properly
- [ ] No server crashes or errors

---

## Advanced Topics

### Proc Spells

Weapon procs are triggered by **SpellEffect::WeaponProc** (85):

```sql
effectid1 = 85,              -- WeaponProc
effect_base_value1 = 90003,  -- Spell to proc
effect_limit_value1 = 250    -- Proc rate (higher = more often)
```

Apply this to a weapon or cast as a buff.

### Auras

Auras are spells that pulse effects in a radius:

```sql
AEDuration = 1800,  -- How long aura lasts (ticks)
aoerange = 50,      -- Pulse radius
effectid1 = 0,      -- Heal nearby allies each pulse
effect_base_value1 = 50
```

### Triggered Spells

Spells that cast other spells:

- **SpellEffect::SpellTrigger** (340): Random chance to trigger
- **SpellEffect::TriggerOnCast** (339): Trigger on spell cast
- **SpellEffect::CastOnFadeEffect** (289): Trigger when buff fades

---

## Summary

Spells are the most flexible and powerful system in EQEmulator:

1. **99% of spells** need only database entries
2. **Custom effects** require C++ but follow clear patterns
3. **Focus effects** let you create powerful item/AA modifiers
4. **Testing is easy** with GM commands

Next Steps:
- Create your first custom spell
- Review **SPELL_EFFECTS_REFERENCE.md** for effect details
- Examine live spells in database for examples
- Read **DISCIPLINES_GUIDE.md** for combat abilities
