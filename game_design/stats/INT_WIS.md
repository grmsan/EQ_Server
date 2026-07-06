# Intelligence (INT) & Wisdom (WIS)

> This document describes shared INT/WIS responsibilities. Exact values, curves, caps, and contribution weights should be hotfixable through `zone/combat_balance.ini`; compiled constants are fallback defaults.

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
Make INT/WIS the long-term mana and spell-scaling stats while keeping base behavior available through legacy rule gates.

### 1. Power Mana
**Formula Shape:**
```cpp
MaxMana = Effective(INT + WIS) * LevelCurve * ClassMultiplier;
```
*   **Change:** New formula path removes old diminishing returns when enabled.
*   **Tuning:** Runtime config controls class weights, ramp, caps, and base-vs-item contribution behavior.

### 2. Spell Damage (INT) / Healing (WIS)
**New Feature:**
Add direct Spell Power / Heal Power based on stats.
*   **INT:** Scales offensive spell bonus/effectiveness.
*   **WIS:** Scales healing, warding, and defensive spell value.
*   **Current in-flight implementation:** INT/WIS can scale the extra `SpellDmg`/`HealAmt` bonus portion without changing base spell values directly.
*   **Why:** Stats should make your spells stronger, not just let you cast more of them.
