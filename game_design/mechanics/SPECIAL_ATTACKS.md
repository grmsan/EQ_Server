# Special Attacks Mechanics

## Overview
This document details the damage calculation logic for Special Attacks (Frenzy, Backstab, Kick, etc.) based on the current codebase analysis.

## 1. Frenzy (Berserker)

**File:** `zone/special_attacks.cpp` & `zone/attack.cpp`

### The Formula
1.  **Base Damage Calculation:**
    *   Defined in `Mob::GetBaseSkillDamage`.
    *   **Formula:** `Base = Level - 15`.
    *   **Hard Cap:** `if (base > 23) base = 23;`
    *   **Level Bonuses:** Small flat adds for levels > 50, 54, 59.
    *   **Result:** A Level 60 Berserker has a **Base Damage of ~23**.

2.  **Mitigation & Randomization:**
    *   Defined in `Mob::MeleeMitigation`.
    *   The server rolls a multiplier between **0.1** and **2.0** based on Attack Rating vs AC.
    *   **Max Result:** `23 * 2.0 = 46`.

3.  **Damage Tables (The Multiplier):**
    *   Defined in `Mob::ApplyDamageTable`.
    *   If the hit lands, a "Damage Table" multiplier is applied.
    *   For Level 60+, this multiplier is approximately **285%** (Factor 2.85).
    *   **Max Result:** `46 * 2.85 = ~131`.

4.  **Critical Hits:**
    *   Defined in `Mob::TryCriticalHit`.
    *   If a crit occurs, the damage is multiplied by the Crit Modifier (default ~2.0, higher with AAs).
    *   **Max Result:** `131 * 2.0 = ~262`.

5.  **Minimum Damage Floor:**
    *   Defined in `Mob::CommonOutgoingHitSuccess`.
    *   Berserkers > Level 50 get a minimum damage floor: `4 * Level / 5`.
    *   **Level 60 Floor:** 48 Damage.

6.  **Bonus Damage (SPA 170):**
    *   `hit.min_damage += GetSkillDmgAmt(hit.skill)`.
    *   This adds flat damage from Items/AAs/Spells.
    *   **Important:** This is added *after* the Critical Hit multiplier.

### Summary
*   **Theoretical Max Hit (No Gear):** ~262.
*   **Scaling:** Completely static. Strength does **not** increase this damage.
*   **Conclusion:** While higher than the initially stated "46", it is mathematically impossible for this skill to hit for "thousands" in the current code without massive `SkillDamageAmount` bonuses on gear.

---

## 2. Backstab (Rogue)

**File:** `zone/special_attacks.cpp`

### The Formula
1.  **Base Damage:**
    *   `Base = WeaponDamage * BackstabMultiplier`.
    *   *Note:* This scales with the weapon, unlike Frenzy.
2.  **Mitigation:** Standard 0.1 - 2.0 roll.
3.  **Damage Table:** Standard multiplier (~3.0).
4.  **Crit:** Standard multiplier (~2.0).

### Why Backstab Hits Harder
Because `Base` uses `WeaponDamage`, a 40dmg dagger results in a much higher starting point than Frenzy's fixed "23".
*   `40 * 2.0 (Mitigation) * 3.0 (Table) * 2.0 (Crit) = ~480` (plus skill multipliers).

---

## 3. Monk Skills (Flying Kick, etc.)

**File:** `zone/special_attacks.cpp`

### The Formula
1.  **Base Damage:**
    *   `Base = SkillLevel / X + BootAC / Y`.
    *   Example (Flying Kick): `Skill/9 + BootAC/25`.
2.  **Scaling:** Scales with Skill Level and Boot AC, but not Strength.

---

## The "Strength Redesign" Proposal
The goal of the Strength redesign is to inject `Strength` into Step 1 (Base Damage Calculation) for all these skills.

**New Frenzy Formula:**
`Base = 23 + (Strength / 1)`.
*   **1200 STR:** Base becomes 1223.
*   **Max Hit:** `1223 * 2.0 * 2.85 * 2.0 = ~13,900`.
*   *Note:* We may need to tune the Damage Table or Divisors to prevent it from being *too* high, but this proves the mechanism works.
