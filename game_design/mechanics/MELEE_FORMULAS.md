# Melee Combat Formulas

Auto-attack and melee damage calculations.

## Table of Contents

1. [Auto-Attack Overview](#auto-attack-overview)
2. [Damage Calculation](#damage-calculation)
3. [Attack Speed](#attack-speed)
4. [Dual Wield and Double Attack](#dual-wield-and-double-attack)
5. [Damage Bonus](#damage-bonus)
6. [DPS Calculation](#dps-calculation)

---

## Auto-Attack Overview

### The Melee Round

Each melee round:
1. Check if attack is ready (based on weapon delay)
2. Roll for hit/miss
3. Calculate base damage
4. Apply mitigation
5. Check for critical hit
6. Apply bonus damage
7. Check for double attack
8. Check for dual wield (off-hand)

### Key Stats

| Stat | Effect |
|------|--------|
| STR | Damage bonus, slight hit bonus |
| DEX | Hit chance, critical chance |
| AGI | Avoidance (defensive) |
| ATK | Overall hit chance |
| Weapon Damage | Base damage |
| Weapon Delay | Attack speed |

---

## Damage Calculation

### Base Damage

```cpp
int GetBaseMeleeDamage(int hand)
{
    int weapon_damage = 0;

    if (hand == EQ::invslot::slotPrimary) {
        weapon_damage = GetPrimaryWeaponDamage();
    } else {
        weapon_damage = GetSecondaryWeaponDamage();
    }

    // Unarmed fallback
    if (weapon_damage == 0) {
        weapon_damage = GetHandToHandDamage();
    }

    return weapon_damage;
}
```

### Hand to Hand Damage

When unarmed:
```cpp
int GetHandToHandDamage()
{
    // Base H2H damage by class
    int base = 2;  // Default

    if (GetClass() == Class::Monk || GetClass() == Class::Beastlord) {
        // Monks get bonus from H2H skill
        base = GetSkill(SkillHandToHand) / 15 + 2;
    }

    return base;
}
```

### Damage Roll

```cpp
int RollMeleeDamage(int base_damage, int min_damage)
{
    // Roll between min and max
    int damage = zone->random.Int(min_damage, base_damage);
    return damage;
}
```

### Mitigation Application

After base damage:
```cpp
int ApplyMitigation(int base_damage, Mob* defender)
{
    // Get mitigation multiplier (0.1 to 2.0)
    float mit = GetMitigationRoll(defender->GetAC());

    // Apply
    int mitigated = static_cast<int>(base_damage * mit);

    // Floor at 1
    if (mitigated < 1) mitigated = 1;

    return mitigated;
}
```

### Damage Table

Level-based multiplier:
```cpp
float GetDamageTableMult(int level)
{
    // Approximate table values
    if (level < 10) return 1.0f;
    if (level < 20) return 1.25f;
    if (level < 30) return 1.5f;
    if (level < 40) return 2.0f;
    if (level < 50) return 2.25f;
    if (level < 55) return 2.5f;
    if (level < 60) return 2.75f;
    return 3.0f;  // 60+
}
```

---

## Attack Speed

### Weapon Delay

Delay determines time between attacks:
```
Attack Timer = Weapon Delay * 100ms
Example: 30 delay = 3.0 seconds between swings
```

### Haste

Haste reduces attack timer:
```cpp
int GetEffectiveDelay(int base_delay)
{
    int haste = GetHaste();  // Total haste %

    // Haste cap (usually 100%)
    if (haste > 100) haste = 100;

    // Calculate effective delay
    // 100% haste = half delay
    float haste_mult = 100.0f / (100.0f + haste);
    int effective = static_cast<int>(base_delay * haste_mult);

    // Minimum delay floor (usually 10)
    if (effective < 10) effective = 10;

    return effective;
}
```

### Haste Sources

| Source | Typical Range |
|--------|---------------|
| Item worn haste | 21-41% |
| Spell haste | 40-70% |
| Overhaste (v3) | 0-25% |
| Bard songs | 10-40% |

Haste stacking:
```
Total = Worn + max(Spell, Bard) + Overhaste
Soft cap at ~100%
```

---

## Dual Wield and Double Attack

### Dual Wield

Off-hand attack chance:
```cpp
bool CheckDualWield()
{
    if (!HasDualWield()) return false;  // Class check
    if (!GetInventory().GetItem(EQ::invslot::slotSecondary)) return false;  // Need weapon

    int skill = GetSkill(SkillDualWield);
    int chance = skill / 4;  // 75 at 300 skill

    // AA bonus
    chance += GetAA(aaAmbidexterity) * 3;

    return zone->random.Roll(chance);
}
```

### Double Attack

Extra main-hand attack:
```cpp
bool CheckDoubleAttack()
{
    int skill = GetSkill(SkillDoubleAttack);
    if (skill == 0) return false;

    int chance = skill / 4;  // 75 at 300 skill

    // AA bonus (Ferocity, etc.)
    chance += GetAA(aaFerocity) * 2;

    return zone->random.Roll(chance);
}
```

### Triple Attack

Third main-hand attack (high skill/AA):
```cpp
bool CheckTripleAttack()
{
    // Usually requires AA
    int triple_aa = GetAA(aaTripleAttack);
    if (triple_aa == 0) return false;

    int chance = triple_aa * 5;  // Up to 25% at rank 5

    return zone->random.Roll(chance);
}
```

---

## Damage Bonus

### STR Damage Bonus

```cpp
int GetSTRDamageBonus()
{
    int str = GetSTR();

    // Threshold-based bonus
    int bonus = 0;
    if (str > 75) bonus += (str - 75) / 10;
    if (str > 100) bonus += (str - 100) / 5;
    if (str > 150) bonus += (str - 150) / 3;

    return bonus;
}
```

### Delay Damage Bonus

Slow weapons get bonus damage:
```cpp
int GetDelayDamageBonus(int delay, int level)
{
    if (level < 28) return 0;  // Low level: no bonus

    // 2H weapons get more bonus
    int bonus = 0;
    if (delay >= 40) {
        bonus = (delay - 30) / 3;
    }

    return bonus;
}
```

### SPA 170 (Skill Damage Amount)

Flat damage bonus from items/buffs:
```cpp
int GetSkillDamageBonus(EQ::skills::SkillType skill)
{
    int bonus = 0;

    // From items
    bonus += itembonuses.SkillDamageAmount[skill];

    // From spells
    bonus += spellbonuses.SkillDamageAmount[skill];

    // From AAs
    bonus += aabonuses.SkillDamageAmount[skill];

    return bonus;
}
```

---

## DPS Calculation

### Theoretical DPS Formula

```
Hits per Second = 1 / (Effective Delay / 10)

Average Hit = (Max Damage + Min Damage) / 2 * Avg Mitigation * Damage Table

DPS = Hits per Second * Average Hit * (1 + Double Attack %) * (1 + Dual Wield %)
    * (1 + Crit Rate * (Crit Mult - 1))
```

### Example Calculation

```
Weapon: 50 damage, 30 delay
Haste: 50%
Double Attack: 75%
Crit Rate: 10%
Crit Mult: 2.0
Damage Table: 2.85x
Mitigation: 1.0 average

Effective Delay = 30 * (100 / 150) = 20
Hits per Second = 1 / 2.0 = 0.5

Average Hit = 50 * 1.0 * 2.85 = 142.5

Attacks per Second = 0.5 * 1.75 = 0.875 (with double attack)

Effective DPS = 0.875 * 142.5 * 1.1 = 137 DPS
```

### DPS Comparison Tool

```sql
-- Compare weapon DPS (simplified)
SELECT
    name,
    damage,
    delay,
    ROUND(damage / (delay / 10.0), 1) as base_dps,
    ROUND(damage / (delay / 10.0) * 2.85 * 1.75 * 1.1, 1) as approx_dps
FROM items
WHERE itemtype IN (0,1,2,3,4)  -- 1H/2H melee
ORDER BY approx_dps DESC
LIMIT 20;
```

---

## Tuning Considerations

### For High-Stat Server

```cpp
// Scale STR bonus better at high values
int GetSTRDamageBonus()
{
    int str = GetSTR();

    int bonus = 0;
    if (str > 75) bonus += (str - 75) / 10;
    if (str > 100) bonus += (str - 100) / 5;
    if (str > 200) bonus += (str - 200) / 4;
    if (str > 500) bonus += (str - 500) / 3;  // NEW: high stat scaling
    if (str > 1000) bonus += (str - 1000) / 5;  // Diminishing at very high

    return bonus;
}
```

### Weapon Progression

Ensure damage progression feels meaningful:
```
Level 1-10:   1-10 damage weapons
Level 10-30:  10-30 damage
Level 30-50:  30-60 damage
Level 50-60:  50-80 damage
Level 60-65:  70-100+ damage
End-game:     100-150+ damage
```

### Attack Speed Balance

```
Slow 2H: High damage per hit, good for burst
Fast 1H: Lower damage, more consistent
Dual Wield: Highest sustained DPS with skill

Target: Similar effective DPS across styles
Differentiation: Burst vs sustained
```
