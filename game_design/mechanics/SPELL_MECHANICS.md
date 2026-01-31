# Spell Mechanics

Spell damage, resists, focus effects, and spell-based combat.

## Table of Contents

1. [Spell Damage Overview](#spell-damage-overview)
2. [Resist Checks](#resist-checks)
3. [Focus Effects](#focus-effects)
4. [Spell Criticals](#spell-criticals)
5. [Damage over Time (DoT)](#damage-over-time-dot)
6. [Spell Bonuses](#spell-bonuses)

---

## Spell Damage Overview

### Spell Damage Flow

```
Cast Spell
    ↓
┌─────────────────────────────┐
│ RESIST CHECK                │
│ Roll vs target's resist    │
└─────────────────────────────┘
    ↓ (if not fully resisted)
┌─────────────────────────────┐
│ BASE DAMAGE                 │
│ From spells_new table       │
└─────────────────────────────┘
    ↓
┌─────────────────────────────┐
│ FOCUS EFFECTS               │
│ - Improved Damage           │
│ - Spell Damage mods         │
└─────────────────────────────┘
    ↓
┌─────────────────────────────┐
│ CRITICAL CHECK              │
│ Spell crit rate/multiplier  │
└─────────────────────────────┘
    ↓
┌─────────────────────────────┐
│ PARTIAL RESIST              │
│ Reduce based on resist roll │
└─────────────────────────────┘
    ↓
Final Damage Applied
```

### Spell Data (spells_new table)

Key columns for damage spells:

| Column | Description |
|--------|-------------|
| `effect_base_value1-12` | Base damage per effect slot |
| `formula1-12` | How damage scales |
| `max1-12` | Maximum damage cap |
| `resist_type` | Which resist applies |
| `resist_diff` | Modifier to resist check |

### Spell Effect Slots

Each spell has 12 effect slots:
```cpp
// SPA (Spell Affect) types for damage
SPA_CurrentHP = 0          // Direct damage or heal
SPA_CurrentHPOnce = 79     // One-time HP change
SPA_SpellDamage = 286      // Add to spell damage
```

---

## Resist Checks

### Resist Types

| Type | Stat | Common Sources |
|------|------|----------------|
| Magic (MR) | WIS | Enchanter spells |
| Fire (FR) | - | Wizard, Mage fire |
| Cold (CR) | - | Wizard ice |
| Poison (PR) | - | Shaman, necro |
| Disease (DR) | - | Necro, SK |
| Chromatic | Lowest | Uses lowest resist |
| Prismatic | Average | Uses average resist |

### Resist Calculation

```cpp
int CalcResistChance(Mob* caster, Mob* target, SpellData* spell)
{
    // Get target's resist value
    int resist = target->GetResist(spell->resist_type);

    // Caster's spell penetration
    int penetration = caster->GetCHA() / 25;  // Charisma helps
    penetration += caster->GetSpellPenetration();

    // Spell's resist modifier
    int spell_mod = spell->resist_diff;

    // Level difference
    int level_diff = caster->GetLevel() - target->GetLevel();

    // Calculate final resist chance
    int resist_chance = resist - penetration + spell_mod - (level_diff * 2);

    // Bounds
    resist_chance = std::clamp(resist_chance, 0, 200);

    return resist_chance;
}
```

### Resist Outcomes

```cpp
// Roll determines outcome
int roll = zone->random.Int(0, 200);

if (roll < resist_chance * 0.25) {
    // Full resist - no damage
    return RESIST_FULL;
}
else if (roll < resist_chance * 0.50) {
    // 75% resist - 25% damage
    return RESIST_PARTIAL_75;
}
else if (roll < resist_chance * 0.75) {
    // 50% resist - 50% damage
    return RESIST_PARTIAL_50;
}
else if (roll < resist_chance) {
    // 25% resist - 75% damage
    return RESIST_PARTIAL_25;
}
else {
    // No resist - full damage
    return RESIST_NONE;
}
```

### Unresistable Spells

Some spells cannot be resisted:
- `resist_type = 0` (None)
- Certain mechanics bypass resist

---

## Focus Effects

### Focus Types for Damage

| Focus Effect | SPA | Description |
|--------------|-----|-------------|
| Improved Damage | 124 | % increase to spell damage |
| Spell Damage | 286 | Flat damage increase |
| Crit Chance | 294 | Increase spell crit rate |
| Crit Damage | 302 | Increase crit multiplier |

### Focus Application

```cpp
int ApplyFocusEffects(Mob* caster, SpellData* spell, int base_damage)
{
    int damage = base_damage;

    // Improved Damage focus (%)
    int focus_pct = caster->GetFocusEffect(focusImprovedDamage, spell);
    damage = damage * (100 + focus_pct) / 100;

    // Flat damage bonus
    int flat_bonus = caster->GetFocusEffect(focusSpellDamage, spell);
    damage += flat_bonus;

    return damage;
}
```

### Focus Limitations

Focus effects have limits:
```cpp
struct FocusEffect {
    int min_level;     // Spell must be >= this level
    int max_level;     // Spell must be <= this level
    int spell_class;   // Only affects certain spell types
    int max_value;     // Cap on the focus effect
};
```

---

## Spell Criticals

### Spell Crit Chance

```cpp
int GetSpellCritChance(Mob* caster, SpellData* spell)
{
    int base_chance = 0;

    // AA-based crit chance
    base_chance += caster->GetAA(aaSpellCritical);  // Usually 2-7%

    // Focus effects
    base_chance += caster->GetFocusEffect(focusCritChance, spell);

    // Item bonuses
    base_chance += caster->GetSpellCritChanceBonus();

    return base_chance;
}
```

### Spell Crit Multiplier

```cpp
float GetSpellCritMultiplier(Mob* caster, SpellData* spell)
{
    float mult = 2.0f;  // Base: double damage

    // AA Destructive Fury increases multiplier
    mult += caster->GetAA(aaDestructiveFury) * 0.1f;  // +10% per rank

    // Focus effects
    mult += caster->GetFocusEffect(focusCritDamage, spell) / 100.0f;

    return mult;
}
```

### Crit Application

```cpp
int ApplySpellCrit(Mob* caster, SpellData* spell, int damage)
{
    int crit_chance = GetSpellCritChance(caster, spell);

    if (zone->random.Roll(crit_chance)) {
        float crit_mult = GetSpellCritMultiplier(caster, spell);
        damage = static_cast<int>(damage * crit_mult);

        // Send crit message
        caster->Message_StringID(MT_Critical, SPELL_CRIT, damage);
    }

    return damage;
}
```

---

## Damage over Time (DoT)

### DoT Mechanics

DoTs tick periodically (usually every 6 seconds):

```cpp
void ApplyDoTTick(Mob* target, Buff* buff)
{
    int base_damage = buff->base_damage_per_tick;

    // Apply caster's damage mods (if any still apply)
    if (buff->caster) {
        base_damage = ApplyFocusEffects(buff->caster, buff->spell, base_damage);
    }

    // DoT crits (if enabled)
    if (RuleB(Spells, DoTsCrit)) {
        base_damage = ApplySpellCrit(buff->caster, buff->spell, base_damage);
    }

    // Apply damage
    target->Damage(buff->caster, base_damage, buff->spell_id, SkillEvocation);
}
```

### DoT Stacking

DoTs can stack if from different spells:
```cpp
bool CanStackDoT(Mob* target, SpellData* new_dot)
{
    // Check if same spell already on target
    if (target->FindBuff(new_dot->id)) {
        return false;  // Same spell doesn't stack
    }

    // Check spell group (similar spells)
    for (auto& buff : target->buffs) {
        if (buff.spell->spell_group == new_dot->spell_group) {
            // Same group - check if new is stronger
            if (new_dot->effect_value > buff.spell->effect_value) {
                // Replace weaker
                target->BuffFadeBySlot(buff.slot);
                return true;
            }
            return false;  // Weaker doesn't land
        }
    }

    return true;  // Different group, can stack
}
```

---

## Spell Bonuses

### SPA Types for Spell Damage

| SPA | Name | Effect |
|-----|------|--------|
| 286 | SpellDamage | Flat damage bonus |
| 296 | CriticalSpellChance | % crit chance |
| 302 | CriticalDotChance | % crit for DoTs |
| 124 | ImprovedDamage | % damage increase |
| 212 | SpellDamageMod | Another % modifier |

### Getting Spell Bonuses

```cpp
int Mob::GetSpellDamageBonus(SpellData* spell)
{
    int bonus = 0;

    // From worn items
    bonus += itembonuses.SpellDmg;

    // From buffs
    bonus += spellbonuses.SpellDmg;

    // From AAs
    bonus += aabonuses.SpellDmg;

    // Spell-specific bonuses
    bonus += GetFocusEffect(focusSpellDamage, spell);

    return bonus;
}
```

### Spell Damage Formula

```cpp
int CalculateSpellDamage(Mob* caster, Mob* target, SpellData* spell)
{
    // Get base damage from spell
    int base = spell->effect_base_value;

    // Apply formula scaling (some spells scale with level)
    base = ApplySpellFormula(base, spell->formula, caster->GetLevel());

    // Apply focus effects
    int improved = caster->GetFocusEffect(focusImprovedDamage, spell);
    base = base * (100 + improved) / 100;

    // Add flat bonuses
    base += caster->GetSpellDamageBonus(spell);

    // Check for crit
    base = ApplySpellCrit(caster, spell, base);

    // Apply resist (partial)
    int resist_outcome = CalcResist(caster, target, spell);
    base = ApplyResistReduction(base, resist_outcome);

    return base;
}
```

---

## Key Spell Functions

### zone/spell_effects.cpp

```cpp
// Main spell processing
void Mob::SpellEffect(Mob* caster, uint16 spell_id, float partial = 1.0f);

// Calculate effect for a single slot
void Mob::CalcSpellEffectValue(SpellData* spell, int effect_id, Mob* caster);

// Apply buff to target
void Mob::ApplyBuff(Mob* caster, uint16 spell_id, int duration);
```

### zone/spells.cpp

```cpp
// Cast a spell
bool Mob::CastSpell(uint16 spell_id, uint16 target_id, ...);

// Check if can cast
bool Mob::CanCastSpell(uint16 spell_id);

// Spell resist check
int Mob::ResistSpell(uint8 resist_type, uint16 spell_id, Mob* caster);
```

---

## Tuning Spell Damage

### For High-Stat Server

```cpp
// Scale spell damage with INT/WIS
int stat_bonus = GetCasterStat() / 10;  // INT for wizards, WIS for clerics
base_damage += stat_bonus * 5;

// Scale crit chance with stats
int crit_bonus = GetCasterStat() / 100;  // 1% per 100 stat
crit_chance += crit_bonus;
```

### Database Tuning

```sql
-- Increase all direct damage spells by 20%
UPDATE spells_new
SET effect_base_value1 = effect_base_value1 * 1.2
WHERE effectid1 = 0 AND effect_base_value1 < 0;  -- Negative = damage

-- Adjust resist difficulty
UPDATE spells_new SET resist_diff = resist_diff + 20
WHERE resist_diff < 0;  -- Make easier to resist
```
