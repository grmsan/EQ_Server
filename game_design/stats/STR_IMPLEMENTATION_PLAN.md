# STR Attribute Implementation Plan

> **⚙️ BALANCE CONFIGURATION**: All tunable values are centralized in `zone/combat_balance_config.h`.
> **📋 DESIGN DOCUMENT**: See `game_design/stats/STR.md` for design philosophy and formulas.
> **🏗️ ARCHITECTURE**: Option B (Additive Base Damage) - see section below for full rationale.

## Executive Summary
This document outlines the implementation approach for the Strength-based damage scaling system. The goal is to **enhance** the current weapon damage system with STR-based scaling that works for all classes, while keeping delay-based bonuses for weapon balance.

**Architecture Choice**: **Option B - Additive Base Damage** ✅
**Core Formula**: `BaseDamage = WeaponDamage + DelayBonus + StrengthBonus`
**Final Damage**: `BaseDamage * (all existing percent modifiers: crits, discs, buffs, etc.)`

**Why Option B?**
- ✅ Predictable, linear math - STR adds to base, mods multiply the whole thing
- ✅ Clear separation: weapon identity (delay) + stat scaling (STR) + modifiers (%)
- ✅ Safer interaction with large percent buffs (no multiplicative explosions)
- ✅ Easy to tune via centralized config constants
- ✅ Simple mental model for players and developers

---

## Architecture Decision: Why Option B?

### The Candidates

We evaluated two primary approaches for integrating STR into the damage pipeline:

**Option B: Additive Base Damage** (SELECTED)
```cpp
BaseDamage = WeaponDamage + DelayBonus + StrengthBonus
FinalDamage = BaseDamage * (crits * discs * buffs * other_mods)
```

**Option C: Percent Modifier** (Rejected)
```cpp
FinalDamage = BaseDamage * (1 + StrengthFactor) * (crits * discs * buffs * other_mods)
```

### Why Option B Wins

#### 1. Predictable Math & Safer Scaling

**Option B**: STR is a big, visible additive term in the base damage calculation.
- If STR increases 50%, that part of the base increases 50%
- All percent effects still scale the total, but nothing new multiplies together
- Easy to reason about: "My hit is 100 weapon + 50 delay + 300 STR = 450 base, then x2 crit = 900"

**Option C**: STR becomes its own percent multiplier that stacks with discs, crits, class mods, etc.
- Risk of multiplicative explosions: `base * STR_mult * crit * disc * buff` can spiral out of control
- Tuning becomes a balancing act: make STR_mult too small (boring), too large (explosive)
- Harder to predict: "Is this 100k hit from STR, crit, or the disc?"

**Conclusion**: For an offline solo server where we want big numbers but still want to understand them, Option B is much more predictable.

#### 2. Clear Mental Model

**Option B** gives a simple story:
- **Weapon**: Provides base damage and influences delay bonus
- **Delay**: Slow weapons add more to base (weapon identity matters)
- **Strength**: Adds flat bonus that scales with level (stat progression)
- **Modifiers**: Crits, discs, buffs multiply the whole base (gameplay moments)

Players can easily understand: "I upgraded from 300 STR to 400 STR, so my base damage went up by (100 * 6.0) = 600. My crits went from 5000 to 6200."

**Option C** blurs these lines:
- STR becomes "just another buff" in the modifier soup
- Harder to explain why damage increased
- Debugging combat logs becomes a maze of multipliers

#### 3. Better Support for Class-Specific Features

The broader STR system includes:
- Pet inheritance (50% of owner STR)
- Tank interrupt immunity (`Channeling + STR/4`)
- Caster stun resistance (`STR/50%`)
- Monk weight limits (`STR * 10`)

These features want a simple "how much STR is this entity worth?" calculation.

**Option B** supports this cleanly: everything calls `GetStrengthDamageBonus()`, and that same logic feeds into class features.

**Option C** makes this awkward because STR is entangled with the modifier pipeline instead of being a clean stat-to-effect conversion.

### Balance Goals & Risk Areas

The design targets three goals:

**Goal 1: Low levels feel good when gaining STR**
- If a gnome starts with 60 STR and finds a +20 STR ring, it should feel noticeable
- Solution: `STR_MIN_LEVEL_MULTIPLIER = 0.1f` ensures level 1 gets 10% effectiveness minimum

**Goal 2: Racial differences matter but don't dominate at level 1**
- Ogre (150 STR) should hit harder than gnome (60 STR), but not 2.5x harder
- Solution: Optional `ENABLE_RACIAL_STR_COMPRESSION` to gently compress extreme gaps
- Rationale: All races should be playable and fun early game; gaps can widen with levels/gear

**Goal 3: High levels can reach massive DPS without uncontrollable explosions**
- Level 70 with 1000 STR should hit for 100k+ DPS
- But combining STR with 100% melee discs shouldn't produce 10 million DPS randomly
- Solution: STR is additive to base (big number), percent mods multiply (predictable scaling)
- If top-end feels too wild, adjust a few large buffs rather than clamping STR

### Risk Mitigation

**Risk 1: Low-level racial gaps**
- Problem: 150 STR ogre vs. 60 STR gnome = 2.5x damage at level 1
- **Mitigation**: `STR_MIN_LEVEL_MULTIPLIER` ensures both get minimum scaling; optional `ENABLE_RACIAL_STR_COMPRESSION` to reduce gap to ~1.5x
- **Config**: `STR_BASELINE_FOR_BALANCE = 100.0f`, `STR_RACIAL_BONUS_SCALAR = 0.7f`

**Risk 2: Interaction with large percent modifiers**
- Problem: Big base damage (weapon + delay + STR) × large percent buffs = huge numbers
- **Mitigation**: This is **intentional** for solo server power fantasy; if too extreme, tune down specific buffs (not STR)
- **Config**: Leave STR scaling strong; adjust disc/buff values if needed

**Risk 3: Offhand and pets**
- Problem: Dual-wield or pets getting full STR bonus may overperform
- **Mitigation**: `OFFHAND_STR_PENALTY = 0.5f` (offhand gets 50%), `PET_STR_DAMAGE_SCALAR = 1.0f` (tunable separately from inheritance)
- **Config**: Easy to adjust if testing shows imbalance

---

## Tuning Knobs Reference

All constants live in `zone/combat_balance_config.h`. Here's a quick guide to what each does:

### Core Scaling
- **`STR_LEVEL_DIVISOR`** (10.0f): PRIMARY TUNING KNOB - raise to reduce all STR damage, lower to increase
- **`STR_MIN_LEVEL_MULTIPLIER`** (0.1f): Minimum effectiveness at low levels (prevents zero damage)

### Optional Advanced Tuning
- **`ENABLE_STR_DIMINISHING_RETURNS`** (false): Enable curved scaling for ultra-high STR values
  - `STR_DIMINISHING_START` (500.0f): STR value where curve begins
  - `STR_DIMINISHING_POWER` (0.85f): Strength of curve (< 1.0 = diminishing)
- **`ENABLE_RACIAL_STR_COMPRESSION`** (false): Compress racial gaps at low levels
  - `STR_BASELINE_FOR_BALANCE` (100.0f): "Average" STR for compression
  - `STR_RACIAL_BONUS_SCALAR` (0.7f): How much to compress (0.7 = 30% reduction)

### Hand/Pet/NPC Penalties
- **`OFFHAND_STR_PENALTY`** (0.5f): Offhand gets 50% of STR bonus
- **`PET_STR_DAMAGE_SCALAR`** (1.0f): Multiply pet STR damage (independent from inheritance)
- **`NPC_STR_DAMAGE_SCALAR`** (0.0f): NPCs don't get STR scaling (player power fantasy)

### Weapon Delay Bonuses
- **`ENABLE_WEAPON_DELAY_BONUS`** (true): Keep delay bonuses alongside STR
- **`DELAY_BONUS_GLOBAL_SCALAR`** (1.0f): Multiply all delay bonuses for easy tuning

### Tuning Strategy (Recommended Order)

1. **Start Linear**: Use only `STR_LEVEL_DIVISOR` and `STR_MIN_LEVEL_MULTIPLIER`, leave all optional features OFF
2. **Test Progression**: Create characters at levels 1/10/50/70 with low/mid/high STR, log damage
3. **Adjust Primary Knob**: If damage too high/low across the board, adjust `STR_LEVEL_DIVISOR`
4. **Check Racial Balance**: If level 1 gaps between ogre/gnome feel too extreme, enable `ENABLE_RACIAL_STR_COMPRESSION`
5. **Fine-Tune Top End**: If level 70 damage feels explosive but lower levels feel good, enable `ENABLE_STR_DIMINISHING_RETURNS` with high start value
6. **Buff Audit**: Only AFTER STR feels good, audit the largest percent buffs (100% discs, etc.) and consider toning down outliers

### Design Decisions ✅
1. **Delay-based bonuses**: KEPT (slow weapons should hit harder)
2. **Offhand scaling**: 50% penalty (`OFFHAND_STR_PENALTY = 0.5f`)
3. **NPC scaling**: Players/pets only (`NPCS_USE_STR_SCALING = false`)
4. **Rule toggles**: Supported for legacy servers
5. **Armor interaction**: STR damage affected by AC (`STR_DAMAGE_AFFECTED_BY_AC = true`)

---

## Current System Analysis

### Existing Damage Calculation Pipeline
1. **`Mob::Attack()` (zone/attack.cpp:1678-1778)**
   - Main entry point for all melee attacks
   - Current bonus: `ucDamageBonus = GetWeaponDamageBonus(weapon)`
   - **Limitation**: Only warriors level 28+, main hand only
   - Applied to `my_hit.min_damage` and hate

2. **`GetWeaponDamageBonus()` (zone/attack.cpp:3414-3460)**
   - **Current Formula (1H)**: `4 + ((level-28)/3) + ((delay-40)/3)` for delay 45+
   - **Current Formula (2H)**: Different delay-based calculation
   - **Critical Finding**: STR is NOT used in any damage calculation currently

3. **`Tuneoffense()` (zone/tune.cpp:1124-1164)**
   - Calculates offense for hit chance (NOT damage)
   - **Does use STR**: `offense += (2*STR - 150)/3` if STR >= 75
   - **But**: This is for accuracy, not damage amount

4. **Damage Modifiers**
   - `ApplyMeleeDamageMods()`: Spell/item/AA bonuses
   - `GetMeleeDamageMod_SE()`: Spell effect modifiers
   - None currently based on STR

### Key Findings
- ✅ STR affects hit chance (via offense calculation)
- ❌ STR does NOT affect damage amount
- ❌ Current weapon bonus is delay-based, not stat-based
- ❌ Current bonus only applies to warriors 28+, main hand
- ✅ Pet bonus system exists: `GetPetATKBonusFromOwner()`, `GetPetACBonusFromOwner()`

---

## Implementation Options

### Option A: Modify GetWeaponDamageBonus() Directly
**Approach**: Replace the delay-based formula in `GetWeaponDamageBonus()` with STR-based calculation.

#### Code Changes
```cpp
// zone/attack.cpp (modify lines 3414-3460)
int Mob::GetWeaponDamageBonus(const EQ::ItemData* weapon, bool offhand)
{
    // NEW: STR-based damage bonus (replaces delay-based formula)
    int level = GetLevel();
    int str_stat = GetSTR();

    // Formula: Strength * (Level / 10), minimum 0.1x
    float level_multiplier = std::max(0.1f, static_cast<float>(level) / 10.0f);
    int str_bonus = static_cast<int>(str_stat * level_multiplier);

    return str_bonus;
}
```

#### Modify Attack() to Apply to All Classes/Hands
```cpp
// zone/attack.cpp (modify lines 1689-1720)
int Mob::Attack(Mob* other, int Hand, bool bRiposte, bool IsStrikethrough, ...)
{
    // ... existing code ...

    // OLD: Only warriors 28+, main hand
    // if (Hand == EQ::invslot::slotPrimary && GetLevel() >= 28 && IsWarriorClass()) {

    // NEW: All classes, both hands
    ucDamageBonus = GetWeaponDamageBonus(weapon, Hand == EQ::invslot::slotSecondary);
    my_hit.min_damage = ucDamageBonus;
    hate += ucDamageBonus;

    // ... rest of attack logic ...
}
```

#### Pros
- ✅ Minimal code changes (single function)
- ✅ Reuses existing infrastructure
- ✅ Automatically applies to all attack paths
- ✅ Preserves existing damage modifier pipeline

#### Cons
- ❌ Function name "GetWeaponDamageBonus" is misleading (not weapon-based anymore)
- ❌ Could confuse developers expecting delay-based formula
- ❌ Loses backward compatibility with old bonus system
- ⚠️ May need rule toggle for servers wanting old behavior

---

### Option B: New GetStrengthDamageBonus() Function ⭐ SELECTED
**Approach**: Create separate STR and weapon damage functions, apply both to all melee.

#### Code Changes
```cpp
// zone/mob.h (add declaration)
class Mob {
    // ...
    int GetStrengthDamageBonus(bool offhand = false);
    // Keep old function, make it work for all classes (not just warriors 28+)
    int GetWeaponDamageBonus(const EQ::ItemData* weapon, bool offhand = false);
};

// zone/attack.cpp (add new function with centralized config)
int Mob::GetStrengthDamageBonus(bool offhand)
{
    // Only players and player pets get STR scaling
    if (!IsClient() && !IsPet()) {
        return 0; // NPCs don't get STR bonuses (NPCS_USE_STR_SCALING = false)
    }

    // Charmed NPCs don't get STR scaling (already powerful)
    if (IsNPC() && CastToNPC()->IsCharmed() && !CombatBalance::CHARMED_NPCS_USE_STR_SCALING) {
        return 0;
    }

    int level = GetLevel();
    int str_stat = GetSTR();

    // Use centralized config constants
    float level_multiplier = std::max(
        CombatBalance::STR_MIN_LEVEL_MULTIPLIER,
        static_cast<float>(level) / CombatBalance::STR_LEVEL_DIVISOR
    );

    int str_bonus = static_cast<int>(str_stat * level_multiplier);

    // Apply offhand penalty if applicable
    if (offhand) {
        str_bonus = static_cast<int>(str_bonus * CombatBalance::OFFHAND_STR_PENALTY);
    }

    return str_bonus;
}

// zone/attack.cpp (modify Attack() function to use BOTH bonuses)
int Mob::Attack(Mob* other, int Hand, bool bRiposte, ...)
{
    // ... existing code ...

    bool is_offhand = (Hand == EQ::invslot::slotSecondary);

    // NEW: All players/pets get STR bonus
    int str_bonus = GetStrengthDamageBonus(is_offhand);

    // NEW: All melee get weapon delay bonus (if enabled)
    int weapon_bonus = 0;
    if (CombatBalance::ENABLE_WEAPON_DELAY_BONUS && weapon) {
        weapon_bonus = GetWeaponDamageBonus(weapon, is_offhand);
    }

    // Total damage bonus = STR + Weapon Delay
    ucDamageBonus = str_bonus + weapon_bonus;
    my_hit.min_damage = ucDamageBonus;
    hate += ucDamageBonus;

    // ... rest of attack logic ...
}
```

#### Pros
- ✅ Clear separation: STR bonus vs. weapon bonus
- ✅ Backward compatible (old function still exists)
- ✅ Could add rule to toggle between old/new system
- ✅ Self-documenting code (function name describes behavior)
- ✅ Easier to tune/balance (modify one function)

#### Cons
- ❌ More code to maintain (two functions)
- ❌ Need to decide: Replace old system or add on top?
- ⚠️ If adding on top, may need to rebalance total damage

---

### Option C: Inject into ApplyMeleeDamageMods() Pipeline
**Approach**: Add STR bonus as a damage modifier alongside spell/item/AA bonuses.

#### Code Changes
```cpp
// common/common.h (add to StatBonuses struct, line ~484)
struct StatBonuses {
    // ... existing bonuses ...
    int StrengthDamageBonus; // NEW: STR-based damage bonus
};

// zone/attack.cpp (modify ApplyMeleeDamageMods or create new modifier)
void Mob::ApplySTRDamageBonus(int &damage)
{
    int level = GetLevel();
    int str_stat = GetSTR();

    float level_multiplier = std::max(0.1f, static_cast<float>(level) / 10.0f);
    int str_bonus = static_cast<int>(str_stat * level_multiplier);

    damage += str_bonus;
}

// zone/attack.cpp (call in damage pipeline)
int Mob::Attack(Mob* other, int Hand, ...)
{
    // ... calculate base damage ...

    // Apply STR bonus as a damage modifier
    ApplySTRDamageBonus(max_hit);
    ApplySTRDamageBonus(min_dmg);

    // ... rest of damage calculation ...
}
```

#### Pros
- ✅ Follows existing modifier pattern (spell/item/AA)
- ✅ Could be buffable/debuffable via StatBonuses
- ✅ Integrates with existing damage pipeline
- ✅ Could apply to special attacks (bash, backstab) automatically

#### Cons
- ❌ More complex code path (modifier system)
- ❌ Need to ensure it stacks correctly with other modifiers
- ❌ May apply in unexpected places (need careful testing)
- ⚠️ Harder to tune (affects all damage sources)

---

### Option D: Hybrid Approach (Base + STR Separate)
**Approach**: Keep old weapon bonus for warriors, add STR bonus for everyone else.

#### Code Changes
```cpp
// zone/attack.cpp (modify Attack() function)
int Mob::Attack(Mob* other, int Hand, ...)
{
    // ... existing code ...

    int damage_bonus = 0;

    // Warriors 28+ get old delay-based bonus (main hand only)
    if (Hand == EQ::invslot::slotPrimary && GetLevel() >= 28 && IsWarriorClass()) {
        damage_bonus = GetWeaponDamageBonus(weapon);
    }
    // All other classes/hands get STR-based bonus
    else {
        int level = GetLevel();
        int str_stat = GetSTR();
        float level_multiplier = std::max(0.1f, static_cast<float>(level) / 10.0f);
        damage_bonus = static_cast<int>(str_stat * level_multiplier);
    }

    ucDamageBonus = damage_bonus;
    my_hit.min_damage = ucDamageBonus;
    hate += ucDamageBonus;
}
```

#### Pros
- ✅ Maintains backward compatibility for warriors
- ✅ Adds STR scaling for other classes
- ✅ Minimal disruption to existing balance

#### Cons
- ❌ Inconsistent system (warriors use different formula)
- ❌ Warriors may be underpowered (delay-based < STR-based)
- ❌ Doesn't meet design goal of universal STR scaling
- ❌ Confusing for players (why different formulas?)

---

## Special Attack Considerations

### Backstab Scaling
**Design**: `Damage = (Weapon + STR_Bonus) * Backstab_Multiplier`

#### Current System
```cpp
// zone/special_attacks.cpp:160
int Mob::GetBaseSkillDamage(EQ::skills::SkillType skill, Mob *target)
{
    // Backstab: uses weapon damage, ignores current bonuses
    if (skill == EQ::skills::SkillBackstab) {
        // ... weapon damage calculation ...
    }
}
```

#### Implementation
```cpp
// zone/special_attacks.cpp (modify GetBaseSkillDamage or DoMeleeSkillAttackDmg)
int Mob::GetBackstabDamage(Mob *target)
{
    // Get weapon damage
    int weapon_damage = GetWeaponDamage(target, weapon);

    // Add STR bonus
    int str_bonus = GetStrengthDamageBonus();

    // Total base damage before multiplier
    int base_damage = weapon_damage + str_bonus;

    // Apply backstab multiplier (from AA, level, etc.)
    int backstab_mult = GetBackstabMultiplier();

    return base_damage * backstab_mult;
}
```

---

### Pet STR Inheritance
**Design**: Pets inherit 50% of owner's STR

#### Current System
```cpp
// zone/tune.cpp:1151-1160 (existing pet ATK bonus)
if (IsOfClientBotMerc()) {
    offense += (GetATK() / 2 + GetPetATKBonusFromOwner()) * RuleI(Combat, PCAttackPowerScaling) / 100;
}
```

#### Implementation Options

**Option 1: Modify Pet::CalcBonuses()**
```cpp
// zone/pets.cpp or zone/npc.cpp
void Pet::CalcBonuses()
{
    NPC::CalcBonuses(); // Call base class

    // Inherit 50% owner STR
    Mob* owner = GetOwner();
    if (owner) {
        int owner_str = owner->GetSTR();
        int inherited_str = owner_str / 2;

        // Add to pet's STR (via itembonuses)
        itembonuses.STR += inherited_str;
    }
}
```

**Option 2: New GetPetSTRBonusFromOwner() Function** ⭐ SELECTED
```cpp
// zone/mob.h
class Mob {
    int GetPetSTRBonusFromOwner(); // Add alongside GetPetATKBonusFromOwner()
};

// zone/mob.cpp (implementation with centralized config)
int Mob::GetPetSTRBonusFromOwner()
{
    if (!HasOwner()) return 0;

    Mob* owner = GetOwner();
    if (!owner) return 0;

    // Only if pets are allowed to use STR scaling
    if (!CombatBalance::PETS_USE_STR_SCALING) return 0;

    // Inherit owner's STR at configured percentage
    int owner_str = owner->GetSTR();
    return static_cast<int>(owner_str * CombatBalance::PET_STR_INHERITANCE);
}

// Use in GetSTR() or GetStrengthDamageBonus()
int Mob::GetSTR()
{
    int base_str = STR + itembonuses.STR + spellbonuses.STR + aabonuses.STR;

    // Pets inherit owner STR
    if (IsPet()) {
        base_str += GetPetSTRBonusFromOwner();
    }

    return base_str;
}
```

---

## Class-Specific Feature Implementation

### Tanks: Interrupt Immunity
**Design**: `Channeling + STR/4` for interrupt resistance

```cpp
// zone/spell_effects.cpp or zone/mob.cpp
bool Mob::CheckInterruptImmunity(int damage)
{
    // Base channeling skill
    int channeling = GetSkill(EQ::skills::SkillChanneling);

    // Add STR bonus for tanks
    if (IsWarriorClass()) { // Or check for tank classes specifically
        channeling += GetSTR() / 4;
    }

    // Roll vs damage
    return (zone->random.Int(0, channeling) > damage);
}
```

### Rogues: Already handled by backstab implementation above

### Casters: Stun Resistance
**Design**: `STR/50` percent chance to resist stun

```cpp
// zone/spell_effects.cpp
bool Mob::TryStunResist(int spell_id)
{
    // Base resist chance
    int resist_chance = 0;

    // Casters get STR-based stun resist
    if (IsWisdomCasterClass() || IsIntelligenceCasterClass()) {
        resist_chance = GetSTR() / 50; // 1% per 50 STR
    }

    // Roll
    return (zone->random.Int(1, 100) <= resist_chance);
}
```

### Monks: Weight Limit
**Design**: `MaxWeight = STR * 10`

```cpp
// zone/client.cpp or zone/mob.cpp
int Client::GetMaxWeight()
{
    // NEW: STR-based weight limit
    return GetSTR() * 10;
}

// Check in inventory functions
bool Client::CanCarryItem(const EQ::ItemData* item)
{
    int current_weight = GetCarriedWeight();
    int max_weight = GetMaxWeight();

    return (current_weight + item->Weight <= max_weight);
}
```

### Priests: Melee When OOM
**Design**: Priests can melee effectively when out of mana

**Note**: This is already handled by universal STR scaling. Priests will do damage based on STR * Level/10 just like other classes. No special implementation needed beyond ensuring they have decent STR gear available.

### Bards: Interrupt Resist for Songs
**Design**: Similar to tank channeling, but for bard songs

```cpp
// zone/spell_effects.cpp
bool Mob::CheckSongInterruptImmunity(int damage)
{
    // Base singing skill
    int singing = GetSkill(EQ::skills::SkillSinging);

    // Add STR bonus for bards
    if (GetClass() == Class::Bard) {
        singing += GetSTR() / 4; // Same as tank channeling
    }

    return (zone->random.Int(0, singing) > damage);
}
```

---

## Testing Strategy

### Phase 1: Core STR Damage System
1. **Test Level 1 Character (75 STR)**
   - Expected: ~5 DPS (75 * 0.1 = 7.5 damage per hit)
   - Verify formula: `damage = STR * (Level/10)`

2. **Test Level 10 Character (100 STR)**
   - Expected: ~50 DPS (100 * 1.0 = 100 damage per hit)

3. **Test Level 50 Character (250 STR)**
   - Expected: ~5,000 DPS (250 * 5.0 = 1,250 damage per hit × 4 attacks/sec)

4. **Test Level 70 Character (1000 STR)**
   - Expected: ~100,000 DPS (1000 * 7.0 = 7,000 damage per hit × ~14 attacks/sec)

### Phase 2: Class-Specific Features
5. **Rogue Backstab**
   - Verify formula: `(Weapon + STR_Bonus) * BS_Mult`
   - Test at multiple levels

6. **Pet STR Inheritance**
   - Check pet GetSTR() returns owner_str / 2
   - Verify pet damage scales correctly

7. **Tank Interrupt Immunity**
   - Cast spells while taking damage
   - Verify `Channeling + STR/4` calculation

8. **Caster Stun Resistance**
   - Test stun spell at various STR levels
   - Verify `STR/50` percent resistance

9. **Monk Weight Limits**
   - Test inventory weight at various STR levels
   - Verify `STR * 10` calculation

### Phase 3: Balance Validation
10. **DPS Curve Verification**
    - Graph actual DPS vs. expected DPS (5 → 100,000)
    - Identify outliers (too high/low)
    - Adjust level multiplier if needed

11. **Cross-Class Balance**
    - Compare DPS across all classes at same level/STR
    - Ensure no class dominates unfairly
    - Verify tanks, healers, casters all viable

12. **Edge Cases**
    - No weapon equipped (hand-to-hand)
    - Dual-wield vs. 2H weapons
    - Charmed NPCs (do they get STR bonus?)
    - Swarm pets (should they inherit?)

---

## Final Implementation Approach ✅

### **Option B + Pet Inheritance Option 2 + Centralized Config**

**Rationale**:
1. **Centralized Balance**: All tuning knobs in one place (`combat_balance_config.h`)
2. **Clean Code Structure**: Separate `GetStrengthDamageBonus()` function is self-documenting
3. **Hybrid System**: Keep weapon delay bonuses + add STR scaling (both matter)
4. **Player-Only Scaling**: NPCs don't get STR bonuses (player power fantasy)
5. **Easy Tuning**: Change one constant to rebalance entire system
6. **Pet System**: Reuse existing pattern (`GetPetATKBonusFromOwner()`) for consistency

### Implementation Timeline

**Week 1: Core Damage System + Balance Config**
- ✅ Create `zone/combat_balance_config.h` with all tunable constants
- Create `GetStrengthDamageBonus()` function (uses config constants)
- Modify `GetWeaponDamageBonus()` to work for all classes (remove warrior 28+ restriction)
- Modify `Attack()` to apply both STR + weapon bonuses
- Test basic damage calculation at levels 1, 10, 50, 70
- Add rule toggles: `Combat:UseStrengthDamageBonus`, `Combat:UseWeaponDelayBonus`

**Week 2: Pet System**
- Implement `GetPetSTRBonusFromOwner()`
- Modify `GetSTR()` to include pet bonus
- Test pet damage inheritance
- Verify swarm pets, charmed NPCs behavior

**Week 3: Class-Specific Features**
- Backstab scaling (rogues)
- Interrupt immunity (tanks, bards)
- Stun resistance (casters)
- Weight limits (monks)
- Test each feature independently

**Week 4: Balance & Polish**
- DPS curve validation
- Cross-class balance testing
- Edge case testing
- Performance profiling
- Documentation updates

---

## Design Philosophy & Decisions

### Hybrid System: STR + Weapon Delay
**Decision**: ✅ Keep BOTH bonuses (not either/or)

**Reasoning**:
- STR provides level-scaled damage growth (1 → 70 progression)
- Weapon delay provides weapon choice depth (slow weapons hit harder)
- Together they create interesting itemization (high STR + slow weapon = best)
- Easy to tune independently via `combat_balance_config.h`

### Player-Only Power Scaling
**Decision**: ✅ NPCs don't get STR bonuses (`NPCS_USE_STR_SCALING = false`)

**Reasoning**:
- Player power fantasy (players scale infinitely, NPCs have fixed power)
- Prevents NPCs from one-shotting players at high levels
- Easier to balance (control NPC damage via stats, not formulas)
- Charmed NPCs already powerful (don't need STR scaling on top)

### Pet Inheritance at 50%
**Decision**: ✅ Keep 50% (`PET_STR_INHERITANCE = 0.5f`)

**Reasoning**:
- Prevents pets from being stronger than players
- Makes STR valuable for summoners without overshadowing INT/WIS
- Follows precedent from AC/ATK pet bonuses
- Tunable if pets feel weak/strong

### STR Damage Affected by AC
**Decision**: ✅ Normal AC mitigation (`STR_DAMAGE_AFFECTED_BY_AC = true`)

**Reasoning**:
- STR is raw physical damage, not armor-penetrating
- Maintains tank value (high AC still matters)
- Consistent with game design (DEX may get AC penetration for "skilled attacks")
- Makes fight mechanics matter (don't just stack STR, need positioning/debuffs)

---

## User Decisions ✅ (Answered)

1. **Warrior Bonus**: ✅ Keep delay-based bonuses for ALL melee (slow weapons hit harder)
2. **Offhand Scaling**: ✅ 50% STR damage (`OFFHAND_STR_PENALTY = 0.5f` - tunable)
3. **NPC Scaling**: ✅ Players and player pets only (`NPCS_USE_STR_SCALING = false`)
4. **Rule Toggle**: ✅ Support legacy servers with rule toggles
5. **Damage Type**: ✅ STR damage affected by AC (raw damage, not penetration - DEX may get that)

---

## Centralized Balance Configuration

**All tunable constants live in `zone/combat_balance_config.h`:**

```cpp
namespace CombatBalance {
    // STR Scaling
    constexpr float STR_LEVEL_DIVISOR = 10.0f;         // Level/10 multiplier
    constexpr float STR_MIN_LEVEL_MULTIPLIER = 0.1f;   // Min 10% effectiveness
    constexpr float OFFHAND_STR_PENALTY = 0.5f;        // Offhand gets 50%

    // Weapon Delay Bonuses
    constexpr bool ENABLE_WEAPON_DELAY_BONUS = true;   // Keep delay bonuses
    constexpr float DELAY_BONUS_DIVISOR_1H = 3.0f;     // 1H scaling
    constexpr float DELAY_BONUS_DIVISOR_2H = 2.5f;     // 2H scaling (better)

    // Pet Inheritance
    constexpr float PET_STR_INHERITANCE = 0.5f;        // Pets get 50% owner STR

    // Class Features
    constexpr float TANK_INTERRUPT_DIVISOR = 4.0f;     // STR/4 for channeling
    constexpr float CASTER_STUN_DIVISOR = 50.0f;       // STR/50 for stun resist
    constexpr float MONK_WEIGHT_MULTIPLIER = 10.0f;    // STR*10 weight limit

    // Applicability
    constexpr bool NPCS_USE_STR_SCALING = false;       // Only players/pets
    constexpr bool PETS_USE_STR_SCALING = true;        // Pets inherit
    constexpr bool CHARMED_NPCS_USE_STR_SCALING = false; // Charm doesn't get it

    // Mitigation
    constexpr bool STR_DAMAGE_AFFECTED_BY_AC = true;   // Normal AC mitigation
}
```

**Benefits**:
- 🎛️ Single file to adjust all balance values
- 📝 Well-documented with rationale for each constant
- 🔧 Easy to tune if damage is too high/low
- 📊 Version control tracks balance history
- 💡 Self-documenting code (constants explain themselves)

---

## Files to Modify

### Core Damage System
- `zone/attack.cpp` - Add `GetStrengthDamageBonus()`, modify `Attack()`
- `zone/mob.h` - Declare new function
- `zone/mob.cpp` - Implement stat calculations if needed

### Pet System
- `zone/mob.cpp` - Add `GetPetSTRBonusFromOwner()`
- `zone/pets.cpp` - Modify `CalcBonuses()` or `GetSTR()`
- `zone/mob.h` - Declare new pet function

### Special Attacks
- `zone/special_attacks.cpp` - Modify backstab, bash, kick damage
- `zone/spell_effects.cpp` - Add interrupt immunity, stun resistance

### Client Features
- `zone/client.cpp` - Weight limit calculations

### Rule System
- `common/ruletypes.h` - Add new combat rules
- `utils/sql/git/required/2025_XX_XX_strength_damage_rules.sql` - DB migration

---

## Conclusion

**Final Approach**: **Option B + Centralized Config + Hybrid Bonus System** ✅

This provides:
- 🎛️ **Easy Tuning**: All balance knobs in `zone/combat_balance_config.h`
- 📝 **Self-Documenting**: Constants explain their purpose and rationale
- 🔄 **Hybrid Bonuses**: STR scaling + weapon delay bonuses (both matter)
- 👥 **Player-Focused**: Only players/pets get new scaling (NPC balance preserved)
- 🧪 **Easy Testing**: Toggle features independently via config
- 📚 **Maintainable**: Clean separation of concerns, follows existing patterns

**Estimated Effort**: 4 weeks (1 developer)
**Risk Level**: Medium (significant combat system changes)
**Testing Required**: Extensive (all classes, all levels, all attack types)

**Implementation Status**:
1. ✅ Centralized config created (`zone/combat_balance_config.h`)
2. ✅ Design decisions finalized (user answered all questions)
3. ✅ Documentation updated (`STR.md` + `STR_IMPLEMENTATION_PLAN.md`)
4. ⏳ Next: Begin coding core damage system

**Key Benefits of Centralized Config**:
- Adjust `STR_LEVEL_DIVISOR` if damage too high → change one number, affects entire system
- Adjust `OFFHAND_STR_PENALTY` if dual-wield too strong → instant rebalance
- Adjust `PET_STR_INHERITANCE` if pets too weak/strong → one constant change
- All changes documented with rationale → future devs understand "why"
- Version control tracks balance history → see what was changed and when

