# Special Attack Formulas (Updated)

This document outlines the **new** formulas for special attacks, designed for a high-stat/solo server environment where Weapon Damage is king.

## Core Philosophy
All special attacks now scale with **Weapon Damage** (or Hand-to-Hand damage) instead of static level-based caps. This ensures that as you get better gear, your special attacks grow in power proportionally.

## Formulas

### 1. Frenzy (Berserker)
**Old Formula:** `Base = Level - 15` (Capped at 23).
**New Formula:**
```cpp
Base = Primary_Weapon_Damage + (Level / 10)
```
*   **Scaling:** Directly uses your weapon's damage. A 150 DMG weapon means 150 Base Damage.
*   **Multipliers:** This Base is then multiplied by the Damage Table (~3.0x) and Crits (2.0x), allowing for hits of 1000+.

### 2. Backstab (Rogue)
**Formula (Unchanged):**
```cpp
Base = Weapon_Damage * ((Skill * 0.02) + 2)
```
*   **Scaling:** Already scaled perfectly with weapon damage.

### 3. Flying Kick (Monk)
**Old Formula:** `Skill / 9` + `Boot_AC / 25`.
**New Formula:**
```cpp
Base = Weapon_Damage (or H2H) + (Skill / 9) + (Boot_AC / 25)
```
*   **Scaling:** Now adds the full damage of your weapon (or fists) to the kick.

### 4. Kick / Round Kick (Warrior/Ranger/Paladin/SK/Monk)
**Old Formula:** `Skill / 10` + `Boot_AC / 25`.
**New Formula:**
```cpp
Base = Weapon_Damage (or H2H) + (Skill / 10) + (Boot_AC / 10)
```
*   **Scaling:** Uses Weapon Damage as a baseline.
*   **Buff:** Boot AC contribution increased (Divisor 25 -> 10).

### 5. Dragon Punch / Eagle Strike / Tiger Claw (Monk)
**Old Formula:** `Base = 1` (plus +1 at skill milestones). Max ~6.
**New Formula:**
```cpp
Base = Weapon_Damage (or H2H) + (Skill / 15)
```
*   **Scaling:** Now hits as hard as a primary weapon swing, plus a skill bonus.

### 6. Bash (Tank Classes)
**Old Formula:** `Shield_AC / Divisor` (or Weapon AC for 2H).
**New Formula:**
*   **2H Bash:** `Base = Weapon_Damage + (Skill / 10)`
*   **Shield Bash:** `Base = (Shield_AC / Divisor) + (Skill / 10)`
*   **Scaling:** 2H Bash now uses the weapon's damage instead of its (non-existent) AC.

## Summary of Impact
| Attack | Old Max Base (Lvl 70) | New Base (150 Dmg Weapon) |
| :--- | :--- | :--- |
| **Frenzy** | 23 | ~157 |
| **Flying Kick** | ~30 | ~180 |
| **Tiger Claw** | 6 | ~165 |
| **2H Bash** | ~1 (Weapon AC) | ~157 |

These changes ensure that Special Attacks are always worth using and contribute significantly to DPS.
