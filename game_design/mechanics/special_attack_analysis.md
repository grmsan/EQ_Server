# Special Attacks Analysis

## Overview
This document analyzes the damage formulas for special attacks, specifically comparing how they scale with weapon damage.

## 1. Frenzy (Berserker)

**Formula:**
```cpp
Base = RuleI(Combat, FrenzyBaseDamage); // Default 10
if (Level > 15) Base += (Level - 15);
if (Base > 23) Base = 23; // HARD CAP
// Small level bonuses (+1 to +4) for levels 50+
```

**Scenario: Level 60 Berserker**
*   **Base Damage:** ~27 (23 + bonuses).
*   **Weapon Damage (150):** **IGNORED**. The code checks if a weapon is equipped, but does *not* use its damage value.
*   **Multipliers:**
    *   Mitigation (0.1 - 2.0) -> Max 54.
    *   Damage Table (~2.85) -> Max 154.
    *   Critical Hit (2.0) -> Max 308.
*   **Result:** Even with a "God Slayer" 150 DMG weapon, Frenzy hits for ~300 max.

## 2. Backstab (Rogue)

**Formula:**
```cpp
Base = WeaponDamage * ((Skill * 0.02) + 2.0);
```

**Scenario: Level 60 Rogue (Skill 250)**
*   **Multiplier:** (250 * 0.02) + 2.0 = **7.0x**.
*   **Weapon Damage (10):**
    *   Base: 70.
    *   Max Hit (Mitigation * Table * Crit): ~800.
*   **Weapon Damage (150):**
    *   Base: 1050.
    *   Max Hit: **~12,000**.

## 3. Monk Skills (Flying Kick)

**Formula:**
```cpp
Base = (Skill / 9) + (BootAC / 25);
```

**Scenario: Level 60 Monk (Skill 250, 50 AC Boots)**
*   **Base:** (27) + (2) = **29**.
*   **Weapon Damage (150):** **IGNORED**.
*   **Result:** Similar to Frenzy, capped at ~300-400 damage.

## Conclusion
*   **Backstab** scales massively with weapon damage.
*   **Frenzy & Monk Skills** are completely disconnected from weapon damage in the current code.
*   **The Fix:** We must inject `Strength` (or Weapon Damage) into the Frenzy/Monk formulas to make them scale like Backstab.
