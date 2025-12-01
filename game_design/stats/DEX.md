# Dexterity (DEX) - Design Document

## Core Philosophy
Dexterity represents hand-eye coordination, fine motor skills, and lethal accuracy. In a Solo Server environment, Dexterity is the **Stat of Precision**. It determines not just *if* you hit, but *how effectively* you hit. It transforms standard attacks into critical strikes and single procs into cascades of magical effects.

---

## 1. "Lethal Precision" (Critical Mastery)
*Primary Beneficiaries: Everyone.*

Dexterity allows you to strike vital points (Melee) or weave spells with perfect geometric efficiency (Casters).

### A. Critical Chance & The Overflow Rule
*   **Formula:** `CritChance% = (Dexterity * Level) / 500`.
*   **The Overflow Mechanic:**
    *   If your Total Crit Chance (Stats + AAs + Items) exceeds 100%, the excess is converted into **Critical Damage Multiplier**.
    *   *Example:* You have 150% Crit Chance (from High DEX + Combat Fury AAs).
    *   *Result:* You have a 100% Chance to Crit, and the extra 50% becomes **+50% Critical Damage**.
    *   *Synergy:* This ensures AAs like "Combat Fury" or "Spell Casting Fury" remain powerful forever.

### B. Base Critical Damage
Even before you reach the cap, high Dexterity makes your crits hit harder.
*   **Formula:** `BaseCritDmg% = Dexterity / 20`.
*   **Impact:**
    *   **100 DEX:** +5% Crit Damage. (Small but noticeable).
    *   **1000 DEX:** +50% Crit Damage.
    *   **Total Power (1000 DEX):**
        *   Crit Chance Calculation: `(1000 * 70) / 500` = **140%**.
        *   **Result:** 100% Crit Chance.
        *   **Damage Bonus:** +40% (Overflow) + 50% (Base) = **+90% Critical Damage**.

### C. Spell Criticals & Twincast
*   **Spell Crits:** Follow the exact same Chance & Overflow rules as Melee.
*   **Twincast:** `Twincast% = (Dexterity * Level) / 2000`.
    *   **Level 70 (1000 DEX):** **35% Chance** to echo any direct damage spell instantly for free.

---

## 2. "Chain Reaction" (Proc Mastery)
*Primary Beneficiaries: Melee, Hybrids, Rangers.*

This is the defining feature of high-DEX builds. Instead of a flat "Procs per Minute" limit, DEX unlocks **Multi-Procs**. A single swing can trigger the weapon's effect multiple times.

### The Multi-Proc System
We calculate the chance for *each subsequent proc* independently based on DEX.

| Proc Sequence | Formula (Chance) | 100 DEX | 500 DEX | 1000 DEX |
| :--- | :--- | :--- | :--- | :--- |
| **1st Proc** | `DEX / (DEX + 100)` | **50%** | **83%** | **91%** |
| **2nd Proc** | `DEX / (DEX + 200)` | **33%** | **71%** | **83%** |
| **3rd Proc** | `DEX / (DEX + 400)` | **20%** | **55%** | **71%** |
| **4th Proc** | `DEX / (DEX + 1500)` | **6%** | **25%** | **40%** |

*   **Scenario (1000 DEX):** You swing your sword.
    *   91% chance to Fire Blast.
    *   If that fires, 83% chance to Fire Blast *again*.
    *   If that fires, 71% chance to Fire Blast *again*.
    *   *Result:* A high-DEX character is a machine gun of magical effects.

---

## 3. "The Ranger's Edge" (Bow Mastery)
*Primary Beneficiaries: Rangers.*

For Rangers, Dexterity is not just accuracy; it is **Force**.

### A. Bow Damage Scaling
*   **Rule:** For Rangers using Bows, **DEX replaces STR** in the damage formula.
*   **Formula:** `BaseDamage = WeaponDmg + (Dexterity * Level / 10)`.
*   *Result:* A Ranger stacking DEX hits as hard with a bow as a Warrior stacking STR hits with a 2H sword.

### B. Sniper's Shot (Crit Damage)
*   **Rule:** Rangers gain bonus Critical Damage based on DEX.
*   **Formula:** `CritDmgMod += Dexterity / 500`.
*   *Result:* At 1000 DEX, Rangers do **+200% Critical Damage** (3x multiplier) with bows.

---

## 4. Caster Specifics: "Spell Penetration"
*Primary Beneficiaries: Wizard, Magician, Necro, Enchanter.*

You asked for more than just Crits. High Dexterity implies hitting the "weak spot" in an enemy's magical defenses.

### The Mechanic: Resist Penetration
*   **Concept:** A fireball thrown with perfect precision slips through the gaps in a dragon's scales.
*   **Formula:** `Resist_Ignore = Dexterity / 10`.
*   **Impact:**
    *   **1000 DEX:** You ignore **100 points** of the target's Magic/Fire/Cold/Poison resistance.
    *   *Result:* Your spells land for full damage much more often, even on red-con bosses. Partial resists become rare.

---

## Summary
*   **Melee:** Proc Machine Guns.
*   **Rangers:** Bow Gods (Dmg + Crit Dmg).
*   **Casters:** 100% Crit + Twincast + Resist Penetration.
