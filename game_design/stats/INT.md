# Intelligence (INT) - Design Document

> This document defines INT's gameplay role and mechanic shapes. Exact spell-power, cooldown, mana-efficiency, mana-pool, lifetap, DoT, pet-share, curve, and cap values should be hotfixable through `zone/combat_balance.ini`; compiled constants are fallback defaults.

## Core Philosophy
Intelligence represents raw mental acuity, memory, and the ability to manipulate the fabric of reality. In a Solo Server environment, Intelligence is the **Stat of Brilliance**. It is the offensive engine for spellcasters and the efficiency engine for everyone else. While Wisdom protects, Intelligence destroys.

---

## 1. "Arcane Potency" (Spell Damage)
*Primary Beneficiaries: Casters, Hybrids, Shadowknights, Paladins.*

Just as Strength adds force to a physical swing, Intelligence adds volatility to a magical weave.

### The Formula
*   **Formula:** `SpellDamageMod% = (Intelligence / 5) * ClassMultiplier`.
*   **Class Multipliers:**
    *   **Wizard:** **1.5x** (The Nuclear Option).
    *   **Magician/Necro:** **1.0x** (Balanced by Pets/DoTs).
    *   **Healers/Hybrids:** **0.8x** (Utility focus).
*   **Impact (1000 INT):**
    *   **Wizard:** **+300%** Base Spell Damage. (5k -> 20k Base -> 60k Crit).
    *   **Mage/Necro:** **+200%** Base Spell Damage.
    *   **Cleric:** **+160%** Base Spell Damage.

### Wizard Bonus: "Arcane Overload"
*   **Concept:** Wizards are masters of destructive magic.
*   **Mechanic:** Wizards gain **Bonus Critical Damage** from Intelligence.
*   **Formula:** `WizCritDmgBonus% = Intelligence / 10`.
*   **Result:** At 1000 INT, Wizards deal **+100% MORE Critical Damage** than anyone else.
    *   *Total Multiplier:* Base (2.0) + DEX (0.9) + INT (1.0) = **x3.9 Crit Multiplier**.
    *   *Final Hit (5k Nuke):* 20k (Base) * 3.9 = **78,000 Damage**. (One shotting trash mobs).

---

## 2. "Alacrity of Mind" (Cooldown Reduction)
*Primary Beneficiaries: Everyone.*

A genius solves problems faster. High INT reduces the "Reuse Time" (Cooldowns) of Spells and Combat Disciplines.

### The Formula (Asymptotic)
We use a curve to prevent infinite spam, but allow for significant speed.

$$ ReuseReduction\% = \frac{50 \times Intelligence}{Intelligence + 500} $$

*   **Hard Cap:** 50% Reduction.

### Impact Table
| Intelligence | Reduction | Example (30s Cooldown) | Example (30m Disc) |
| :--- | :--- | :--- | :--- |
| **100** | **8.3%** | 27.5s | 27.5m |
| **500** | **25.0%** | 22.5s | 22.5m |
| **1000** | **33.3%** | 20.0s | 20.0m |
| **2000** | **40.0%** | 18.0s | 18.0m |

*   **Solo Reality:**
    *   **Casters:** Higher DPS uptime.
    *   **Melee:** "Shield Wall" or "Furious" disciplines are available for every boss fight.
    *   **Healers:** "Emergency Heals" are ready when you need them.

---

## 3. "Conservation" (Mana Efficiency)
*Primary Beneficiaries: Casters, Healers.*

Understanding the weave of magic allows you to do more with less effort.

### The Formula
*   **Formula:** `ManaCostReduction% = (Intelligence * Level) / 2000`.
*   **Impact:**
    *   **Level 70 (1000 INT):** **35% Mana Cost Reduction**.
    *   *Synergy:* Combined with Stamina (Mana Regen), this ensures that a high-INT character is effectively a "Perpetual Motion Machine" of magic.

---

## 4. "The Reservoir" (Mana Pool)
*Shared with Wisdom.*

Intelligence contributes equally to the raw size of the mana pool.

*   **Formula:** `MaxMana = ((INT + WIS) * Level) * ClassMultiplier`.
*   *Note:* This allows Hybrids (SK/Paladin/Ranger/Bard) to stack INT if they want to play more aggressively (More Damage/CDR) without losing Mana Pool size compared to stacking WIS.

---

## 5. Class-Specific Benefits

### A. The Casters (Wizard, Magician, Necro, Enchanter)
*   **The Glass Cannon:**
    *   INT is their "Strength". It is the primary scaler for their damage output.
    *   **Cooldown Reduction** allows Wizards to chain their biggest nukes.

#### Class Masteries (INT)
*   **Wizard - "Arcane Overload":**
    *   Gains **Bonus Crit Damage** from INT. (See above).
*   **Necromancer - "Necrotic Mastery":**
    *   **DoT Power:** INT provides a separate multiplier to Damage-over-Time spells (stacking with the base Spell Dmg).
    *   **Lifetap Efficiency:** INT increases the *healing* received from Lifetaps by an additional 50%.
*   **Magician - "Elemental Synergy":**
    *   **Shared Power:** A portion of the Magician's INT is added to their Pet's stats (stacking with Charisma).


### B. The Healers (Cleric, Druid, Shaman)
*   **The Efficient Medic:**
    *   While WIS makes heals *bigger*, INT makes them *faster* and *cheaper*.
    *   **Strategy:** A Cleric might stack some INT to get their "Divine Arbitration" cooldown lower for difficult fights.

### C. The Hybrids (Shadowknight, Paladin, Ranger, Bard)
*   **The Tactician:**
    *   **Shadowknights:** INT increases the damage of their Lifetaps (which heals them more) and reduces the cooldown on their Harm Touch.
    *   **Bards:** INT reduces the cooldown on their "Bellow" and "Fade" abilities.

### D. The Pure Melee (Warrior, Rogue, Monk, Berserker)
*   **The Smart Fighter:**
    *   **Discipline Haste:** The primary draw. Reducing the cooldown on defensive or offensive disciplines (like "Mighty Strike" or "Fortitude") significantly increases their power in a dungeon crawl.
