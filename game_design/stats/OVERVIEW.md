# EQ Server Stat System: Analysis & Redesign Proposal

## 1. Current System Analysis
Based on the source code analysis of `zone/client_mods.cpp` and `zone/client.cpp`, the current system uses a mix of hard caps, diminishing returns, and formulas that often minimize the impact of high stats.

### A. The "Hard" Cap (255)
Currently, base stats (STR, STA, DEX, AGI, INT, WIS, CHA) are strictly clamped based on level.

**Logic in `Client::GetMaxStat()`:**
*   **Level 1-60:** Hard cap of **255**.
*   **Level 61-70:** Cap increases by 5 per level (Max 305).
*   **Level 71+:** Cap is 330.

### B. Weak Scaling & Diminishing Returns
Even if you raise the cap, the *benefit* you get from stats is often underwhelming or diminishes.

**1. Attack Rating (Strength)**
In `Client::GetATKRating()`:
*   **Formula:** `(OffenseSkill * 1.345) + ((STR - 66) * 0.9) + (WeaponSkill * 2.69)`
*   **Issue:** Strength provides **less than 1 ATK per point** (0.9). Gaining 100 STR only gives you 90 ATK, which is a minor damage increase. It does not scale with level directly (only via skills).

**2. Hit Points (Stamina)**
In `Client::CalcBaseHP()`:
*   **STA > 255:** Points are effectively worth **50% less**.
*   **Issue:** High stamina builds are punished.

**3. Mana (Intelligence/Wisdom)**
In `Client::CalcBaseMana()`:
*   **INT/WIS > 200:** Diminishing returns kick in.
*   **Issue:** You fight against a divisor that reduces the impact of high stats.

---

## 2. Proposed Redesign: "Unbound & Level-Scaled"
To achieve "drastic improvement" and "meaningful upgrades," we propose a system where **Stats × Level** drives power.

### A. Core Philosophy
1.  **No Caps:** `GetMaxStat()` returns **2000+**.
2.  **Level Multipliers:** A Level 60 with 2000 STR should be significantly stronger than a Level 1 with 2000 STR.
3.  **High Impact:** Gaining 100 STR should feel like a major power spike.

### B. New Formulas

#### 1. Titanic Strength (Damage & ATK)
**Current:** `STR * 0.9` (Weak, flat).

**Proposed (OPTION B - Additive Base Damage):**
*   **Architecture:** STR bonus is added to base damage alongside weapon delay bonus, then all percent modifiers scale the total.
*   **Formula:** `StrengthDamageBonus = STR * (Level / STR_LEVEL_DIVISOR)` where `STR_LEVEL_DIVISOR = 10.0f`
*   **Total Base Damage:** `BaseDamage = WeaponDamage + DelayBonus + StrengthBonus`
*   **Impact:**
    *   **Level 1, 100 STR:** 10 bonus damage (100 * 0.1)
    *   **Level 10, 100 STR:** 100 bonus damage (100 * 1.0)
    *   **Level 60, 100 STR:** 600 bonus damage (100 * 6.0)
    *   **Level 60, 500 STR:** 3000 bonus damage (500 * 6.0)
    *   *Result:* Strength becomes a primary driver of damage at high levels. The large base damage (weapon + delay + STR) then multiplies through existing crit/disc/buff systems for massive end-game DPS.

**Why Option B Instead of Option C (Percent Modifier)?**
- **Predictable Math:** STR is a flat bonus added to base; percent mods multiply the whole thing. No unexpected multiplicative explosions.
- **Clear Mental Model:** "My weapon hits for X, delay adds Y, STR adds Z" - easy to understand and debug.
- **Safer Scaling:** Large percent buffs (100% melee disc) scale the whole base, but STR itself doesn't multiply with other percent effects.
- **Weapon Identity Preserved:** Delay bonuses keep weapon choice meaningful alongside STR scaling.

**Tuning Strategy (see `zone/combat_balance_config.h`):**
1. Start with linear formula using `STR_LEVEL_DIVISOR` and `STR_MIN_LEVEL_MULTIPLIER`
2. Test at levels 1, 10, 50, 70 with low/mid/high STR values
3. If top-end feels explosive, raise `STR_LEVEL_DIVISOR` (gentler scaling)
4. If racial gaps at level 1 feel too large, enable `ENABLE_RACIAL_STR_COMPRESSION`
5. Only add diminishing returns (`ENABLE_STR_DIMINISHING_RETURNS`) if linear tuning isn't enough

#### 2. Super Stamina (HP)
**Current:** Diminishing returns after 255.
**Proposed:**
*   **Formula:** `BaseHP = Level * ClassFactor + (STA * Level * ClassFactor / 10)`
*   **Impact:**
    *   We remove the `/ 1000` divisor and use a smaller one (e.g., `/ 10`) or a direct multiplier to make STA huge.
    *   *Example:* `HP += STA * Level`.
    *   **Level 60, 100 STA:** +6,000 HP.
    *   **Level 60, 200 STA:** +12,000 HP.
    *   *Result:* Tanks become actual raid bosses.

#### 3. Power Mana (INT/WIS)
**Current:** Complex formula with drop-offs after 200.
**Proposed:**
*   **Formula:** `MaxMana = (INT_or_WIS * Level * ClassMultiplier)`
*   **Impact:**
    *   **Level 60, 200 INT:** 12,000 Mana (assuming multiplier ~1).
    *   **Level 60, 300 INT:** 18,000 Mana.
    *   *Result:* Casters never run out of mana if they invest heavily in stats.

#### 4. Heroic Dexterity (Crit/Proc)
**Current:** DEX affects Bard songs and weapon procs slightly.
**Proposed:**
*   **Formula:** `CritChance += (DEX * Level) / 2000`
*   **Impact:**
    *   **Level 1, 100 STR:** Negligible crit.
    *   **Level 60, 255 DEX:** ~7.6% Crit Chance.
    *   **Level 60, 500 DEX:** ~15% Crit Chance.
    *   *Result:* High level rogues/rangers with god-tier gear become crit machines.

---

## 3. Implementation Examples

### Example 1: Removing the Cap
**File:** `zone/client_mods.cpp`
**Function:** `Client::GetMaxStat()`

**Old Code:**
```cpp
int32 Client::GetMaxStat() const {
    // ... complex level checks ...
    if (level < 61) return 255;
    // ...
}
```

**New Code:**
```cpp
int32 Client::GetMaxStat() const {
    // Simple, massive cap
    return 2000;
}
```

### Example 2: Redoing ATK (Strength)
**File:** `zone/client.cpp`
**Function:** `Client::GetATKRating()`

**Old Code:**
```cpp
AttackRating = (GetSkill(EQ::skills::SkillOffense) * 1.345) + ((GetSTR() - 66) * 0.9) + (GetPrimarySkillValue() * 2.69);
```

**New Code:**
```cpp
// Massive scaling based on Level * STR
uint32 str_bonus = (GetSTR() * GetLevel()) / 5;
AttackRating = (GetSkill(EQ::skills::SkillOffense) * 1.345) + str_bonus + (GetPrimarySkillValue() * 2.69);
```

## 4. Summary of Impact
| Stat | Current Behavior | New Behavior |
| :--- | :--- | :--- |
| **Cap** | 255 (at Lv 60) | 2000+ (Uncapped) |
| **Strength** | +0.9 ATK per point (Weak) | **+20 ATK per point** (at Lv 60) |
| **Stamina** | 50% penalty > 255 | **Linear Scaling** (HP = STA * Level) |
| **Gameplay** | Gear upgrades feel meaningless after hitting cap. | Gaining 100 STR is a **massive** power spike. |
