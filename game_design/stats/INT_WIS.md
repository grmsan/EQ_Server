# Intelligence (INT) & Wisdom (WIS)

## Current Implementation

### 1. Mana Pool
**File:** `zone/client_mods.cpp`
**Function:** `Client::CalcBaseMana()`

INT (for Int-casters) and WIS (for Wis-casters) determine Max Mana.

**Formula:**
```cpp
// Simplified
if (Stat > 200) {
    // Diminishing returns logic
    Stat = (Stat - 200) / -2 + Stat;
}
MaxMana = ((5 * (Stat + 20)) / 2) * 3 * Level / 100;
```
*   **Impact:** The penalty after 200 makes stacking INT/WIS less rewarding.

### 2. Skill Ups
High INT/WIS increases the chance to skill up in Tradeskills and Spells.

---

## Proposed Redesign

### Goal
Make INT/WIS the "Infinite Power" stats.

### 1. Power Mana
**New Formula:**
```cpp
// Linear Scaling
MaxMana = (Stat * Level * ClassMultiplier);
```
*   **Change:** Remove the diminishing returns.
*   **Impact:**
    *   **Level 60, 300 INT:** Massive mana pool compared to 200 INT.

### 2. Spell Damage (INT) / Healing (WIS)
**New Feature:**
Add direct Spell Power / Heal Power based on stats.
*   **INT Formula:** `SpellDmg += INT / 10`
*   **WIS Formula:** `HealAmt += WIS / 10`
*   **Why:** Stats should make your spells stronger, not just let you cast more of them.
