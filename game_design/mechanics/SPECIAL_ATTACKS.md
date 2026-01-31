# Special Attacks Mechanics

Current special attack mechanics with accurate code references.

## Overview

Special attacks use the `GetBaseSkillDamage()` function at [zone/special_attacks.cpp#L33](../../../zone/special_attacks.cpp) to calculate base damage. This server has **already implemented** weapon scaling for most special attacks.

## Code Flow

```
Player activates ability (button/hotkey)
       ↓
Client sends OP_CombatAbility packet
       ↓
zone/client_packet.cpp handles packet
       ↓
DoMeleeSkillAttackDmg() called [zone/special_attacks.cpp#L2589]
       ↓
GetBaseSkillDamage() calculates base [zone/special_attacks.cpp#L33]
       ↓
MeleeMitigation() applied [zone/attack.cpp#L1163]
       ↓
ApplyDamageTable() applied [zone/attack.cpp#L6265]
       ↓
TryCriticalHit() checked [zone/attack.cpp#L5654]
       ↓
Final damage dealt
```

---

## 1. Frenzy (Berserker)

**Location:** [zone/special_attacks.cpp](../../../zone/special_attacks.cpp) lines 63-97

### Current Implementation (Weapon Scaling ENABLED)

```cpp
case EQ::skills::SkillFrenzy:
    if (IsClient()) {
        auto primary = CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary);
        if (primary && primary->GetItem()) {
            // Uses weapon damage as base
            base = primary->GetItem()->Damage;
            base += (GetLevel() / 10);  // Small level bonus
        } else {
            // Fallback for unarmed
            if (GetLevel() > 15) base += GetLevel() - 15;
            if (base > 23) base = 23;
        }
    }
    return base;
```

### Formula

```
With Weapon:  Base = Weapon_Damage + (Level / 10)
Without:      Base = min(23, Level - 15)  [original formula]
```

### Example Calculation

| Weapon Dmg | Level | Base | After Mitigation (x1.5) | After Table (x2.85) | After Crit (x2) |
|------------|-------|------|-------------------------|---------------------|-----------------|
| 50 | 65 | 56 | 84 | 240 | 480 |
| 100 | 65 | 106 | 159 | 453 | 906 |
| 150 | 65 | 156 | 234 | 667 | 1334 |

### Related Rules

```cpp
// common/ruletypes.h#L681
RULE_INT(Combat, FrenzyBaseDamage, 10, "Frenzy base damage, default is 10")
```

---

## 2. Backstab (Rogue)

**Location:** [zone/special_attacks.cpp](../../../zone/special_attacks.cpp) lines 219-256

### Current Implementation

```cpp
case EQ::skills::SkillBackstab: {
    // Multiplier based on skill level
    float multiplier = GetSkill(EQ::skills::SkillBackstab) * 0.02f + 2.0f;

    if (IsClient()) {
        auto primary = CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary);
        if (primary && primary->GetItem()) {
            base = static_cast<int>(primary->GetItem()->Damage * multiplier);
        }
    }
    return base;
}
```

### Formula

```
Base = Weapon_Damage × (Skill × 0.02 + 2.0)

At 300 skill: multiplier = 300 × 0.02 + 2.0 = 8.0×
```

### Example Calculation

| Weapon Dmg | Skill | Multiplier | Base | After Full Pipeline |
|------------|-------|------------|------|---------------------|
| 20 | 300 | 8.0 | 160 | ~900 |
| 50 | 300 | 8.0 | 400 | ~2300 |
| 100 | 300 | 8.0 | 800 | ~4500 |

### Related Rules

```cpp
// common/ruletypes.h#L676
RULE_INT(Combat, BackstabBaseDamage, 0, "Backstab base damage, default is 0")

// common/ruletypes.h#L640-642
RULE_BOOL(Combat, BackstabIgnoresElemental, false, "Elemental damage affecting backstab")
RULE_BOOL(Combat, BackstabIgnoresBane, false, "Bane damage affecting backstab")
RULE_INT(Combat, DoubleBackstabLevelRequirement, 55, "Level requirement for double backstab")
```

---

## 3. Flying Kick (Monk)

**Location:** [zone/special_attacks.cpp](../../../zone/special_attacks.cpp) lines 99-130

### Current Implementation (Weapon Scaling ENABLED)

```cpp
case EQ::skills::SkillFlyingKick: {
    float skill_bonus = skill_level / 9.0f;

    if (IsClient()) {
        auto primary = CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary);
        if (primary && primary->GetItem()) {
            base = primary->GetItem()->Damage;
        } else {
            base = GetHandToHandDamage();
        }

        base += (int)skill_bonus;

        // Boot AC bonus
        auto inst = CastToClient()->GetInv().GetItem(EQ::invslot::slotFeet);
        if (inst) {
            base += (int)(inst->GetItemArmorClass(true) / 25.0f);
        }
    }
    return base;
}
```

### Formula

```
Base = Weapon_Damage (or H2H) + (Skill / 9) + (Boot_AC / 25)

At 300 skill, 100 boot AC: Base = Weapon + 33 + 4 = Weapon + 37
```

### Related Rules

```cpp
// common/ruletypes.h#L680
RULE_INT(Combat, FlyingKickBaseDamage, 25, "Flying Kick base damage, default is 25")
```

---

## 4. Kick / Round Kick

**Location:** [zone/special_attacks.cpp](../../../zone/special_attacks.cpp) lines 135-170

### Current Implementation (Weapon Scaling ENABLED)

```cpp
case EQ::skills::SkillKick:
case EQ::skills::SkillRoundKick: {
    if (IsClient()) {
        auto primary = CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary);
        if (primary && primary->GetItem()) {
            base = primary->GetItem()->Damage;
        } else {
            base = GetHandToHandDamage();
        }

        // Boot AC bonus (improved divisor)
        auto inst = CastToClient()->GetInv().GetItem(EQ::invslot::slotFeet);
        if (inst) {
            base += (int)(inst->GetItemArmorClass(true) / 10.0f);
        }

        base += (skill_level / 10);
    }
    return base;
}
```

### Formula

```
Base = Weapon_Damage (or H2H) + (Skill / 10) + (Boot_AC / 10)
```

### Related Rules

```cpp
// common/ruletypes.h#L682-683
RULE_INT(Combat, KickBaseDamage, 3, "Kick base damage, default is 3")
RULE_INT(Combat, RoundKickBaseDamage, 5, "Round Kick base damage, default is 5")
```

---

## 5. Dragon Punch / Eagle Strike / Tiger Claw (Monk)

**Location:** [zone/special_attacks.cpp](../../../zone/special_attacks.cpp) lines 37-60

### Current Implementation (Weapon Scaling ENABLED)

```cpp
case EQ::skills::SkillDragonPunch:
case EQ::skills::SkillEagleStrike:
case EQ::skills::SkillTigerClaw:
    if (IsClient()) {
        auto primary = CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary);
        if (primary && primary->GetItem()) {
            base = primary->GetItem()->Damage;
        } else {
            base = GetHandToHandDamage();
        }
        base += (skill_level / 15);
    }
    return base;
```

### Formula

```
Base = Weapon_Damage (or H2H) + (Skill / 15)
```

### Related Rules

```cpp
// common/ruletypes.h#L677-679, 685
RULE_INT(Combat, DragonPunchBaseDamage, 12, "Dragon Punch base damage")
RULE_INT(Combat, EagleStrikeBaseDamage, 7, "Eagle Strike base damage")
RULE_INT(Combat, TigerClawBaseDamage, 4, "Tiger Claw base damage")
```

---

## 6. Bash (Tank Classes)

**Location:** [zone/special_attacks.cpp](../../../zone/special_attacks.cpp) lines 172-217

### Current Implementation

```cpp
case EQ::skills::SkillBash: {
    int weapon_dmg = 0;

    if (IsClient()) {
        if (HasShieldEquipped()) {
            // Shield bash: use shield AC
            inst = CastToClient()->GetInv().GetItem(EQ::invslot::slotSecondary);
        } else if (HasTwoHanderEquipped()) {
            // 2H bash: use weapon damage
            auto weapon = CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary);
            if (weapon && weapon->GetItem()) {
                weapon_dmg = weapon->GetItem()->Damage;
            }
        }
    }

    if (weapon_dmg > 0) {
        // 2H Bash uses weapon damage
        base = weapon_dmg + (int)skill_bonus;
    } else if (inst) {
        // Shield bash uses shield AC
        base += (int)(inst->GetItemArmorClass(true) / RuleR(Combat, BashACBonusDivisor));
    }
    return base;
}
```

### Formula

```
2H Bash:     Base = Weapon_Damage + (Skill / 10)
Shield Bash: Base = Shield_AC / 25 + (Skill / 10)
```

### Related Rules

```cpp
// common/ruletypes.h#L660, 677
RULE_REAL(Combat, BashACBonusDivisor, 25.0, "Divides AC value contribution to bash damage")
RULE_INT(Combat, BashBaseDamage, 2, "Bash base damage, default is 2")
```

---

## Damage Comparison Table

| Attack | With 100 DMG Weapon | Without Weapon |
|--------|---------------------|----------------|
| **Frenzy** | 106 base | 23 base (capped) |
| **Backstab** | 800 base (at 300 skill) | N/A (needs weapon) |
| **Flying Kick** | 137 base (300 skill) | H2H + 33 + boot bonus |
| **Kick** | 130 base (300 skill) | H2H + 30 + boot bonus |
| **Tiger Claw** | 120 base (300 skill) | H2H + 20 |
| **2H Bash** | 130 base (300 skill) | N/A (uses shield AC) |

---

## Key Takeaways

1. **Weapon scaling is already implemented** for all special attacks on this server
2. The `GetBaseSkillDamage()` function (line 33) is the central point for all formulas
3. Rules like `FrenzyBaseDamage` only apply when there's no weapon equipped
4. Damage goes through full pipeline: Mitigation → Damage Table → Crits
5. Final damage can be 5-10x base damage after all multipliers
