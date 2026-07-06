# STA Attribute Implementation Plan

> **Balance configuration:** Keep legacy STA behavior unless `Combat:UseNewStaminaFormula` is enabled. Tuning should be hotfixable through `zone/combat_balance.ini`; `zone/combat_balance_config.h` should hold fallback defaults only.
> **📋 DESIGN DOCUMENT**: See `game_design/stats/STA.md` for design philosophy and formulas.
> **🏗️ ARCHITECTURE**: Multiple independent systems - HP scaling, regen engine, damage mitigation.

## Executive Summary
This document outlines the implementation approach for the Stamina-based sustainability systems. The goal is to transform STA from a passive "HP bar" stat into an active **Resource Engine** that fuels combat through health pools, regeneration (HP/Mana/Endurance), and damage mitigation.

**FINAL ARCHITECTURE DECISIONS** ✅:
1. **HP Scaling**: Option B (Direct Scaling) - Tuned conservatively with full knob control
2. **Damage Mitigation**: Hybrid AC×STA Model - AC provides base, STA amplifies up to cap
3. **Scaling Type**: Linear across all systems - Predictable, tunable, no exponential spikes

**Core Systems**:
1. **Iron Constitution** - Linear HP scaling with class multipliers (conservative tuning)
2. **Undying Vitality** - HP/Mana/Endurance regeneration for self-sufficiency
3. **Thick Skin** - Hybrid AC×STA percent-based mitigation (synergy between defensive stats)
4. **Environmental Resistance** - Poison/Disease/Drowning bonuses

**Philosophy**: In solo play, every class tanks. STA ensures you can survive long enough for your DPS to win the fight, whether through raw HP pools (tanks), massive regen (priests), or sustained resource generation (casters). All systems use **linear scaling** for predictability and avoid exponential power spikes.

---

## System 1: Iron Constitution (Health Pool)

### Design Goal (REVISED - Conservative Tuning)
**Target HP Pools** (Unbuffed, Base STA from CalcBaseHP only):
- **Level 1**: ~100-150 HP (all classes survivable)
- **Level 70 Tank (1000 STA)**: ~12,000-15,000 HP (base, before gear HP bonuses)
- **Level 70 Melee (800 STA)**: ~8,000-10,000 HP
- **Level 70 Priest (600 STA)**: ~6,000-8,000 HP
- **Level 70 Caster (500 STA)**: ~5,000-6,000 HP

**Rationale**: These are BASE HP values from STA scaling alone. Gear will add substantial HP bonuses on top. We tune conservatively to prevent 50-70k HP pools that would require massive regen values and trivialize content.

### Current System Analysis
**Location**: `zone/client_mods.cpp::CalcBaseHP()`

**Current Formula** (Pre-SoF):
```cpp
uint32 Post255;
uint32 lm = GetClassLevelFactor();
if ((GetSTA() - 255) / 2 > 0) {
    Post255 = (GetSTA() - 255) / 2;
}
else {
    Post255 = 0;
}
base_hp = (5) + (GetLevel() * lm / 10) +
          (((GetSTA() - Post255) * GetLevel() * lm / 3000)) +
          ((Post255 * GetLevel()) * lm / 6000);
```

**Issues**:
- **Diminishing Returns**: STA > 255 is worth **50% less** (Post255 divisor 6000 vs 3000)
- **Weak Scaling**: Division by 3000 makes high STA feel underwhelming
- **Class Factor**: `lm` (Level Multiplier) ranges 20-55, but doesn't align with our tank/caster vision
- **Low Level**: Formula produces ~100 HP at level 1, which is good, but doesn't scale well to end-game

### Architecture Options

#### **Option A: Enhanced Classic Formula** (Recommended for Low Risk)
Keep the existing structure but remove penalties and scale up the impact.

**Formula**:
```cpp
base_hp = BASE_HP_CONSTANT +
          (GetLevel() * ClassMultiplier) +
          (GetSTA() * GetLevel() * ClassMultiplier / STA_HP_DIVISOR);
```

**Runtime/fallback configuration**:
```cpp
namespace STA_HP {
    static constexpr int BASE_HP_CONSTANT = 5;
    static constexpr float STA_HP_DIVISOR = 50.0f;  // Lower = more HP per STA

    // Class multipliers for HP scaling
    static constexpr float WARRIOR_MULTIPLIER = 1.6f;
    static constexpr float SHADOWKNIGHT_MULTIPLIER = 1.3f;
    static constexpr float PALADIN_MULTIPLIER = 1.5f;
    static constexpr float RANGER_MULTIPLIER = 1.2f;
    static constexpr float MONK_MULTIPLIER = 1.2f;
    static constexpr float BARD_MULTIPLIER = 1.1f;
    static constexpr float BEASTLORD_MULTIPLIER = 1.0f;
    static constexpr float BERSERKER_MULTIPLIER = 1.0f;
    static constexpr float ROGUE_MULTIPLIER = 1.0f;
    static constexpr float CLERIC_MULTIPLIER = 1.0f;
    static constexpr float DRUID_MULTIPLIER = 1.0f;
    static constexpr float SHAMAN_MULTIPLIER = 1.1f;
    static constexpr float WIZARD_MULTIPLIER = 0.8f;
    static constexpr float MAGICIAN_MULTIPLIER = 0.8f;
    static constexpr float ENCHANTER_MULTIPLIER = 0.9f;
    static constexpr float NECROMANCER_MULTIPLIER = 1.3f;
}
```

**Example Calculations**:
- **Level 1 Warrior (100 STA, 1.5x)**: `5 + (1 * 1.5) + (100 * 1 * 1.5 / 50)` = **8.5** ≈ **9 HP** ❌ Too low!
- **Level 70 Warrior (1000 STA, 1.5x)**: `5 + (70 * 1.5) + (1000 * 70 * 1.5 / 50)` = **2210 HP** ❌ Way too low!

**This won't work - divisor needs to be MUCH smaller or we need a different approach.**

#### **Option B: Direct Scaling Formula** (Recommended - Simple & Powerful)
Completely replace the classic formula with level-scaled STA that matches our design goals.

**Formula**:
```cpp
base_hp = BASE_HP +
          (BASE_HP_PER_LEVEL * GetLevel() * ClassMultiplier) +
          (GetSTA() * GetLevel() * ClassMultiplier / STA_DIVISOR);
```

**Configuration** (Conservative Tuning):
```cpp
namespace STA_HP {
    static constexpr bool ENABLE_STA_HP_SCALING = true;

    // Base HP constants (tuning knobs)
    static constexpr int BASE_HP = 100;           // Starting HP for all classes
    static constexpr int BASE_HP_PER_LEVEL = 5;   // Flat HP per level (REDUCED from 10)
    static constexpr float STA_DIVISOR = 10.0f;   // STA * Level / Divisor (INCREASED from 5)

    // Optional: Global scalar for entire formula (1.0 = normal, 0.8 = 80% of calculated HP)
    static constexpr float HP_LEVEL_SCALAR = 1.0f;

    // Class multipliers (applied to both level AND STA scaling)
    static constexpr float WARRIOR_MULTIPLIER = 1.6f;
    static constexpr float CLERIC_MULTIPLIER = 1.1f;
    static constexpr float PALADIN_MULTIPLIER = 1.5f;
    static constexpr float RANGER_MULTIPLIER = 1.2f;
    static constexpr float SHADOWKNIGHT_MULTIPLIER = 1.4f;
    static constexpr float DRUID_MULTIPLIER = 1.0f;
    static constexpr float MONK_MULTIPLIER = 1.1f;
    static constexpr float BARD_MULTIPLIER = 1.1f;
    static constexpr float ROGUE_MULTIPLIER = 1.0f;
    static constexpr float SHAMAN_MULTIPLIER = 1.1f;
    static constexpr float NECROMANCER_MULTIPLIER = 1.3f;    // Higher HP for caster
    static constexpr float WIZARD_MULTIPLIER = 0.8f;
    static constexpr float MAGICIAN_MULTIPLIER = 0.8f;
    static constexpr float ENCHANTER_MULTIPLIER = 0.9f;
    static constexpr float BEASTLORD_MULTIPLIER = 1.2f;
    static constexpr float BERSERKER_MULTIPLIER = 1.0f;
}
```

**Key Tuning Knobs**:
1. **STA_DIVISOR** (Primary) - Larger = less HP from STA (10 is conservative, can reduce to 7-8 if too low)
2. **BASE_HP_PER_LEVEL** - Controls smooth curve (5 is conservative, original was 10)
3. **HP_LEVEL_SCALAR** - Global dial to scale entire formula up/down without changing ratios
4. **Class Multipliers** - Adjust inter-class spread (tanks vs casters)**Example Calculations** (Conservative Tuning: STA_DIVISOR=10, BASE_HP_PER_LEVEL=5):

- **Level 1 Warrior (100 STA, 1.5x)**:
  - `100 + (5 * 1 * 1.5) + (100 * 1 * 1.5 / 10)`
  - `100 + 7.5 + 15` = **122.5 HP** ✅ Survivable start

- **Level 10 Warrior (150 STA, 1.5x)**:
  - `100 + (5 * 10 * 1.5) + (150 * 10 * 1.5 / 10)`
  - `100 + 75 + 225` = **400 HP** ✅ Smooth early progression

- **Level 70 Warrior (1000 STA, 1.5x)**:
  - `100 + (5 * 70 * 1.5) + (1000 * 70 * 1.5 / 10)`
  - `100 + 525 + 10,500` = **11,125 HP** ✅ Base HP (gear adds more)

- **Level 70 Warrior (1500 STA, 1.5x)** (High gear):
  - `100 + 525 + (1500 * 70 * 1.5 / 10)` = **16,400 HP** ✅ Strong but not excessive

- **Level 70 Wizard (500 STA, 0.8x)**:
  - `100 + (5 * 70 * 0.8) + (500 * 70 * 0.8 / 10)`
  - `100 + 280 + 2,800` = **3,180 HP** ✅ Lower base, relies on gear HP

- **Level 70 Wizard (800 STA, 0.8x)**:
  - `100 + 280 + (800 * 70 * 0.8 / 10)` = **4,860 HP** ✅ With STA investment

**Analysis**:
- ✅ Level 1 viable for all classes (100-150 HP)
- ✅ Level 70 base HP is 10-16k (conservative, gear will add more)
- ✅ Tanks get ~2x HP of casters at same STA
- ✅ Room for gear HP bonuses without hitting 50k+ total
- ⚠️ If too low, reduce STA_DIVISOR (10 → 8 or 7)
- ⚠️ If too high, increase STA_DIVISOR (10 → 12 or 15)**Tuning Strategy**:
1. **Start Conservative**: Begin with STA_DIVISOR=10, BASE_HP_PER_LEVEL=5
2. **Test with Gear**: Remember gear HP bonuses multiply on top of base
3. **Check Regen Impact**: Regen scales with max HP - high HP = high regen requirements
4. **Primary Dial**: `STA_DIVISOR`
   - Too much HP? Increase (10 → 12 → 15)
   - Too little HP? Decrease (10 → 8 → 7)
5. **Secondary Dials**:
   - `BASE_HP_PER_LEVEL` - Smooth curve vs flat start (5 is good baseline)
   - `HP_LEVEL_SCALAR` - Global multiplier for all HP (emergencies only)
   - Class multipliers - Adjust tank/caster spread if needed
6. **Watch For**:
   - Total HP pools > 50k (excessive, requires massive regen/mitigation tuning)
   - Regen recovering >20% max HP per tick (too strong)
   - Gear HP becoming irrelevant compared to STA scaling

### Recommended Approach: **Option B with Conservative Tuning** ✅**Why Option B with Conservative Tuning**:
- ✅ Simple, predictable **linear** math - no exponential surprises
- ✅ Achieves target HP goals with conservative divisors
- ✅ Class fantasy intact (tanks ~2x HP of casters at same STA)
- ✅ Scales smoothly from level 1 to 70
- ✅ Easy to debug and tune with clear knobs
- ✅ Leaves room for gear HP bonuses without hitting 50-70k pools
- ✅ Regen balance easier with moderate base HP
- ✅ **Linear scaling commitment** - predictable growth at all levels

**Implementation Plan**:
1. Add fallback `ENABLE_STA_HP_SCALING` flag and runtime override key.
2. Create helper function `GetSTAHPMultiplier()` to return class multiplier
3. Modify `Client::CalcBaseHP()` to check flag and use new formula
4. Test at levels 1, 10, 30, 50, 70 with low/mid/high STA values
5. Tune `STA_DIVISOR` based on actual HP vs target HP

---

## System 2: Undying Vitality (Regeneration)

### Design Goal
**Self-Sufficiency**: In solo play, you have no healer, no clarity, no backup. Regen must sustain you through long boss fights.

**Target Regen Rates** (Per Tick, ~6 seconds):
- **HP Regen**: Recover **5-10% of max HP** per tick in combat
- **Mana Regen**: Sustain spell rotation without running OOM (1-2% of max mana per tick)
- **Endurance Regen**: Near-permanent discipline uptime (high regen for melee)

### Architecture: Three Independent Systems

#### **A. HP Regeneration (The Wolverine Factor)**

**Current System**: `zone/client_mods.cpp::CalcHPRegen()`
- Base regen from class tables (~1-10 HP/tick)
- Sitting bonus, meditation bonus, innate skills
- Item/AA/Spell bonuses added on top
- Fast regen when sitting/medding out of combat

**Proposed Addition**:
```cpp
// STA-based HP regen bonus
int64 sta_hp_regen = 0;
if (CombatBalance::STA_REGEN::ENABLE_STA_HP_REGEN) {
    sta_hp_regen = (GetSTA() * GetLevel()) /
                   CombatBalance::STA_REGEN::HP_REGEN_DIVISOR;
}
base += sta_hp_regen;
```

**Configuration**:
```cpp
namespace STA_REGEN {
    static constexpr bool ENABLE_STA_HP_REGEN = true;
    static constexpr float HP_REGEN_DIVISOR = 10.0f;  // STA * Level / 10 = HP/tick
}
```

**Example Calculations**:
- **Level 1 (100 STA)**: `(100 * 1) / 10` = **10 HP/tick** (100% of HP at level 1!)
- **Level 10 (150 STA)**: `(150 * 10) / 10` = **150 HP/tick** (~20% of 700 HP)
- **Level 70 (1000 STA, 22k HP)**: `(1000 * 70) / 10` = **7000 HP/tick** (~32% of max HP)
- **Level 70 (1500 STA, 33k HP)**: `(1500 * 70) / 10` = **10,500 HP/tick** (~32% of max HP)

**Analysis**:
- ✅ Low levels heal very fast (great for new players)
- ✅ High levels sustain through boss mechanics
- ⚠️ May be **too strong** - 32% HP per tick = full heal in 3 ticks (18 seconds)
- **Tuning**: Increase divisor to 15 or 20 for ~5-10% per tick

**Recommended Divisor**: **20.0f** for ~5% HP per tick at end-game

#### **B. Mana Regeneration (The Arcane Well)**

**Current System**: `zone/client_mods.cpp::CalcManaRegen()`
- Base regen from meditation skill
- Sitting/horse bonuses
- Item/AA/Spell bonuses

**Proposed Addition**:
```cpp
// STA-based mana regen bonus (casters need resources)
int64 sta_mana_regen = 0;
if (CombatBalance::STA_REGEN::ENABLE_STA_MANA_REGEN &&
    (IsIntelligenceCasterClass() || IsWisdomCasterClass())) {
    sta_mana_regen = (GetSTA() * GetLevel()) /
                     CombatBalance::STA_REGEN::MANA_REGEN_DIVISOR;
}
regen += sta_mana_regen;
```

**Configuration**:
```cpp
namespace STA_REGEN {
    static constexpr bool ENABLE_STA_MANA_REGEN = true;
    static constexpr float MANA_REGEN_DIVISOR = 50.0f;  // STA * Level / 50 = Mana/tick
}
```

**Example Calculations** (Assuming ~20k max mana at L70):
- **Level 1 (100 STA)**: `(100 * 1) / 50` = **2 Mana/tick**
- **Level 70 (500 STA)**: `(500 * 70) / 50` = **700 Mana/tick** (~3.5% of max mana)
- **Level 70 (800 STA)**: `(800 * 70) / 50` = **1120 Mana/tick** (~5.6% of max mana)
- **Level 70 (1000 STA)**: `(1000 * 70) / 50` = **1400 Mana/tick** (~7% of max mana)

**Analysis**:
- ✅ Sustains spell rotations without Clarity buffs
- ✅ Scales with both STA investment and level
- ⚠️ May trivialize mana management at high STA
- **Tuning**: Increase divisor to 70 or 100 for tighter resource management

**Recommended Divisor**: **50.0f** (as designed) for ~3-7% mana per tick

#### **C. Endurance Regeneration (The Infinite Warrior)**

**Current System**: `zone/client_mods.cpp::CalcEnduranceRegen()` (doesn't exist for Client, only Merc)
- Need to create this function for Client class
- Currently endurance regen is hardcoded or rule-based

**Proposed Implementation**:
```cpp
int64 Client::CalcEnduranceRegen() {
    int64 regen = 0;

    // Base endurance regen (existing logic if any)
    regen += itembonuses.EnduranceRegen;
    regen += spellbonuses.EnduranceRegen;
    regen += aabonuses.EnduranceRegen;

    // STA-based endurance regen (NEW)
    if (CombatBalance::STA_REGEN::ENABLE_STA_ENDURANCE_REGEN) {
        regen += (GetSTA() * GetLevel()) /
                 CombatBalance::STA_REGEN::ENDURANCE_REGEN_DIVISOR;
    }

    return (regen * RuleI(Character, EnduranceRegenMultiplier) / 100);
}
```

**Configuration**:
```cpp
namespace STA_REGEN {
    static constexpr bool ENABLE_STA_ENDURANCE_REGEN = true;
    static constexpr float ENDURANCE_REGEN_DIVISOR = 20.0f;  // STA * Level / 20 = End/tick
}
```

**Example Calculations**:
- **Level 70 (1000 STA)**: `(1000 * 70) / 20` = **3500 End/tick**
- **Level 70 (1500 STA)**: `(1500 * 70) / 20` = **5250 End/tick**

**Analysis**:
- ✅ Allows near-permanent discipline uptime
- ✅ Critical for melee DPS to sustain 100k DPS rotations
- ✅ Scales with gear investment
- **Tuning**: Adjust based on discipline endurance costs

**Recommended Divisor**: **20.0f** (as designed)

---

## System 3: Thick Skin (Damage Mitigation)

### Design Goal
**Universal Tanking with AC Synergy**: Every class tanks in solo play. STA **amplifies** AC-based mitigation rather than replacing it, creating synergy between defensive stats.

**Target Mitigation** (Combined AC + STA):
- **Level 70 Tank (1000 STA, High AC)**: 60-70% total mitigation
- **Level 70 Caster (500 STA, Low AC)**: 30-40% total mitigation
- **Hard Cap**: 90% maximum total mitigation (prevent immunity)

### Final Architecture: Hybrid AC×STA Percent Model ✅

**Why Hybrid Model**:
- ✅ AC is primary source of mitigation (existing gear remains valuable)
- ✅ STA amplifies AC effectiveness (synergy between defensive stats)
- ✅ STA never produces mitigation alone (prevents STA from replacing AC)
- ✅ Percent-based scales well against all damage ranges
- ✅ Clear tuning knobs for AC and STA contributions independently
- ❌ Rejected: Flat reduction (too blunt, trivializes small hits)
- ❌ Rejected: AC conversion (opaque, hard to predict)
- ❌ Rejected: Pure STA percent (STA would replace AC entirely)

**Philosophy**: AC does the heavy lifting, STA makes AC work better. Together they form a unified defensive identity without making either stat obsolete.

#### **Implementation: Hybrid AC×STA Percentage Formula**

**High-Level Concept**:
1. Calculate AC-based mitigation percent (existing AC formulas)
2. Allow STA to **amplify** that mitigation, up to a STA-based ceiling
3. Cap total mitigation at a configurable maximum (prevent immunity)

**Pseudocode**:
```cpp
// Step 1: Get AC-based mitigation from existing system
float ac_mitigation_percent = ComputeACMitigation();  // e.g., 30% from AC

// Step 2: Calculate STA amplification ceiling
float sta_bonus_cap = (GetSTA() * GetLevel()) / STA_MIT_DIVISOR;  // e.g., +15% max

// Step 3: STA can amplify AC mitigation up to its own ceiling
float sta_bonus_final = std::min(sta_bonus_cap, ac_mitigation_percent);

// Step 4: Combine AC and STA mitigation
float total_mitigation_percent = ac_mitigation_percent + sta_bonus_final;

// Step 5: Apply hard cap
total_mitigation_percent = std::min(total_mitigation_percent, MAX_MIT_PERCENT);  // e.g., 90%

// Step 6: Apply mitigation to damage
damage = damage * (1.0f - (total_mitigation_percent / 100.0f));
```

**Example Scenarios**:

**Scenario 1: High AC Tank**
- AC provides: **40%** mitigation
- STA (1000, L70): `(1000 * 70) / 2000 = 35%` ceiling
- STA amplifies AC: `min(35%, 40%) = 35%`
- **Total**: `40% + 35% = 75%` (capped at 90%)
- Boss hits for 2000 → `2000 * 0.25 = 500` damage taken

**Scenario 2: Low AC Caster**
- AC provides: **15%** mitigation
- STA (500, L70): `(500 * 70) / 2000 = 17.5%` ceiling
- STA amplifies AC: `min(17.5%, 15%) = 15%` (limited by low AC)
- **Total**: `15% + 15% = 30%`
- Boss hits for 2000 → `2000 * 0.70 = 1400` damage taken

**Scenario 3: Medium AC/STA Balanced**
- AC provides: **25%** mitigation
- STA (800, L70): `(800 * 70) / 2000 = 28%` ceiling
- STA amplifies AC: `min(28%, 25%) = 25%`
- **Total**: `25% + 25% = 50%`
- Boss hits for 2000 → `2000 * 0.50 = 1000` damage taken

**Configuration**:
```cpp
namespace STA_MITIGATION {
    static constexpr bool ENABLE_HYBRID_AC_STA_MITIGATION = true;

    // AC-side controls (tune existing AC system)
    static constexpr float AC_MITIGATION_SOFTCAP_SCALAR = 1.0f;  // Multiplier for AC effectiveness
    static constexpr float AC_MITIGATION_HARD_CAP = 50.0f;  // Max percent AC alone can provide
    // Optional: static constexpr float AC_MITIGATION_CURVE_STEEPNESS = 1.0f;

    // STA-side controls
    static constexpr float STA_MIT_DIVISOR = 2000.0f;  // STA * Level / Divisor = STA ceiling %
    static constexpr float STA_MIT_MAX_PERCENT = 50.0f;  // Absolute max STA contribution
    static constexpr bool STA_MATCH_AC = true;  // If true, STA can only amplify existing AC

    // Global cap
    static constexpr float COMBAT_MAX_MITIGATION_PERCENT = 90.0f;  // Total mitigation hard cap
}
```

**Key Tuning Knobs**:
1. **STA_MIT_DIVISOR** (Primary STA control)
   - Larger = weaker STA contribution (2000 is conservative)
   - `(STA * Level) / 2000` at L70 w/ 1000 STA = 35% ceiling
   - Reduce to 1500 for stronger STA, increase to 3000 for weaker

2. **AC_MITIGATION_HARD_CAP** (Primary AC control)
   - Caps how much AC alone can provide (50% is balanced)
   - Prevents AC stacking from reaching immunity without STA

3. **COMBAT_MAX_MITIGATION_PERCENT** (Safety valve)
   - Global cap prevents immunity (90% recommended)
   - 90% = take 10% damage minimum

4. **STA_MATCH_AC** (Boolean flag)
   - `true`: STA limited by AC percent (synergy enforced)
   - `false`: STA provides full ceiling even with low AC

5. **STA_MIT_MAX_PERCENT** (Backup cap)
   - Absolute max STA can contribute regardless of formula
   - Prevents weird edge cases at extreme STA values

**Analysis**:
- ✅ AC remains valuable - STA can't exceed AC's mitigation
- ✅ STA investment worthwhile - amplifies AC effectiveness
- ✅ Low AC classes (casters) get some benefit but can't tank like warriors
- ✅ High AC classes (tanks) get maximum STA amplification
- ✅ Percent-based works against all damage ranges (no small-hit trivialize problem)
- ✅ Linear scaling - predictable, tunable
- ⚠️ Requires hooking into AC mitigation calculation (find where AC converts to damage reduction)
- ⚠️ Must avoid re-running AC diminishing returns (use final AC mit percent output)

**Implementation Location**:
Likely in `zone/attack.cpp` in damage calculation pipeline, after AC mitigation is computed but before final damage is applied.

**Implementation Steps**:
1. Find where AC-based mitigation percent is calculated
2. Extract that value (e.g., `float ac_mit_percent`)
3. Calculate STA bonus ceiling: `(STA * Level) / STA_MIT_DIVISOR`
4. Apply STA matching rule: `min(sta_ceiling, ac_mit_percent)` if matching enabled
5. Combine: `total_mit = ac_mit + sta_bonus`
6. Cap: `min(total_mit, COMBAT_MAX_MITIGATION_PERCENT)`
7. Apply to damage: `damage *= (1.0 - total_mit/100.0)`

**Testing Strategy**:
1. **Low AC Test**: Character with 500 AC, verify STA bonus is limited
2. **High AC Test**: Character with 2000 AC, verify STA amplifies significantly
3. **Cap Test**: Stack AC + STA to 100%+, verify 90% cap holds
4. **Damage Range Test**: Take hits from 100, 500, 1000, 2000, 5000 - verify percent scaling works
5. **Class Balance Test**: Compare tank vs caster mitigation at same STA but different AC

**Risks & Mitigation**:

**Risk 1: High-end gear stacking → 80-90% mitigation too easily**
- Mitigation: Tune `STA_MIT_DIVISOR` higher (2000 → 3000)
- Mitigation: Lower `AC_MITIGATION_HARD_CAP` (50% → 40%)

**Risk 2: Low AC characters feel bad (no STA benefit)**
- Mitigation: Disable `STA_MATCH_AC` to allow base STA contribution
- Mitigation: Ensure HP and regen compensate for low mitigation

**Risk 3: AC formulas are complex/opaque**
- Mitigation: Hook at final AC mit % output, not raw AC values
- Mitigation: Log AC mit %, STA bonus %, and total % for debugging

**Risk 4: Trivializes damage at 90% cap**
- Mitigation: 90% is not immunity - still take 10% damage
- Mitigation: Can lower cap to 80% or 85% if needed
- Mitigation: Boss mechanics can bypass mitigation (magic damage, DoTs, etc.)

---

## System 4: Environmental Resistance### Poison/Disease Resistance

**Current System**: Likely in `zone/client_mods.cpp` resist calculations

**Proposed**:
```cpp
int32 Client::GetPoisonResist() const {
    int32 resist = base_poison_resist + itembonuses.PR + spellbonuses.PR + aabonuses.PR;

    if (CombatBalance::STA_ENVIRONMENT::ENABLE_STA_POISON_RESIST) {
        resist += GetSTA() / CombatBalance::STA_ENVIRONMENT::POISON_RESIST_DIVISOR;
    }

    return resist;
}

// Same for Disease
int32 Client::GetDiseaseResist() const {
    int32 resist = base_disease_resist + itembonuses.DR + spellbonuses.DR + aabonuses.DR;

    if (CombatBalance::STA_ENVIRONMENT::ENABLE_STA_DISEASE_RESIST) {
        resist += GetSTA() / CombatBalance::STA_ENVIRONMENT::DISEASE_RESIST_DIVISOR;
    }

    return resist;
}
```

**Configuration**:
```cpp
namespace STA_ENVIRONMENT {
    static constexpr bool ENABLE_STA_POISON_RESIST = true;
    static constexpr bool ENABLE_STA_DISEASE_RESIST = true;
    static constexpr float POISON_RESIST_DIVISOR = 5.0f;  // 1 resist per 5 STA
    static constexpr float DISEASE_RESIST_DIVISOR = 5.0f;
}
```

**Impact**: 1000 STA = +200 poison/disease resist

### Drowning Timer

**Current System**: Likely in breath timer mechanics

**Proposed**:
```cpp
int32 Client::GetBreathTimer() const {
    int32 base_breath = BASE_BREATH_TIMER;

    if (CombatBalance::STA_ENVIRONMENT::ENABLE_STA_BREATH_BONUS) {
        base_breath += (GetSTA() / CombatBalance::STA_ENVIRONMENT::BREATH_BONUS_DIVISOR);
    }

    return base_breath;
}
```

**Configuration**:
```cpp
namespace STA_ENVIRONMENT {
    static constexpr bool ENABLE_STA_BREATH_BONUS = true;
    static constexpr float BREATH_BONUS_DIVISOR = 10.0f;  // 1 second per 10 STA
}
```

**Impact**: 1000 STA = +100 seconds underwater

### Food/Drink Consumption

**Quality of Life feature** - lower priority, can implement later.

---

## Implementation Roadmap

### Phase 1: Core HP Scaling (Highest Priority)
**Goal**: Get massive HP pools working

1. ✅ **Add fallback configuration** to `zone/combat_balance_config.h` and runtime keys to `combat_balance.ini`
   ```cpp
   namespace STA_HP {
       static constexpr bool ENABLE_STA_HP_SCALING = true;
       static constexpr int BASE_HP = 100;
       static constexpr int BASE_HP_PER_LEVEL = 10;
       static constexpr float STA_DIVISOR = 5.0f;
       // ... class multipliers ...
   }
   ```

2. ✅ **Create helper function** in `zone/client.h` / `zone/client.cpp`
   ```cpp
   float Client::GetSTAHPMultiplier() const;
   ```

3. ✅ **Modify `CalcBaseHP()`** in `zone/client_mods.cpp`
   - Add feature flag check at start
   - Calculate new formula
   - Return enhanced HP value

4. ✅ **Test HP values**
   - Spawn characters at levels 1, 10, 30, 50, 70
   - Check each class (Warrior, Rogue, Cleric, Wizard)
   - Verify HP matches target ranges
   - Tune `STA_DIVISOR` as needed

### Phase 2: Regeneration Engine (High Priority)
**Goal**: Self-sufficient healing and resource generation

5. ✅ **Add regen fallback configuration** and runtime override keys.
   ```cpp
   namespace STA_REGEN {
       static constexpr bool ENABLE_STA_HP_REGEN = true;
       static constexpr bool ENABLE_STA_MANA_REGEN = true;
       static constexpr bool ENABLE_STA_ENDURANCE_REGEN = true;
       static constexpr float HP_REGEN_DIVISOR = 20.0f;
       static constexpr float MANA_REGEN_DIVISOR = 50.0f;
       static constexpr float ENDURANCE_REGEN_DIVISOR = 20.0f;
   }
   ```

6. ✅ **Modify `CalcHPRegen()`** in `zone/client_mods.cpp`
   - Add STA bonus to base regen

7. ✅ **Modify `CalcManaRegen()`** in `zone/client_mods.cpp`
   - Add STA bonus for casters only

8. ✅ **Create `CalcEnduranceRegen()`** for Client (if missing)
   - Copy structure from Merc version
   - Add STA bonus

9. ✅ **Test regeneration rates**
   - Sit/stand at various levels
   - Check regen in combat vs out of combat
   - Verify mana sustains spell rotations
   - Verify endurance sustains disciplines

### Phase 3: Damage Mitigation (HIGH Priority - New Hybrid Model)
**Goal**: AC×STA synergy for all classes

10. ✅ **Add hybrid mitigation configuration**
    ```cpp
    namespace STA_MITIGATION {
        static constexpr bool ENABLE_HYBRID_AC_STA_MITIGATION = true;

        // AC controls
        static constexpr float AC_MITIGATION_HARD_CAP = 50.0f;
        static constexpr float AC_MITIGATION_SOFTCAP_SCALAR = 1.0f;

        // STA controls
        static constexpr float STA_MIT_DIVISOR = 2000.0f;
        static constexpr float STA_MIT_MAX_PERCENT = 50.0f;
        static constexpr bool STA_MATCH_AC = true;

        // Global cap
        static constexpr float COMBAT_MAX_MITIGATION_PERCENT = 90.0f;
    }
    ```

11. ✅ **Find AC mitigation calculation** in `zone/attack.cpp`
    - Locate where AC converts to damage reduction percent
    - Hook after AC's diminishing returns are applied

12. ✅ **Implement hybrid AC×STA formula**
    - Extract AC mitigation percent
    - Calculate STA bonus ceiling: `(STA * Level) / STA_MIT_DIVISOR`
    - Apply matching rule: `min(sta_ceiling, ac_mit)` if enabled
    - Combine: `total_mit = ac_mit + sta_bonus`
    - Cap: `min(total_mit, COMBAT_MAX_MITIGATION_PERCENT)`
    - Apply to damage

13. ✅ **Test mitigation synergy**
    - Low AC character (caster) - verify limited STA benefit
    - High AC character (tank) - verify strong STA amplification
    - Cap test - stack to 100%+, verify 90% holds
    - Damage range test - 100, 500, 1000, 2000, 5000
    - Log AC%, STA%, and total% for debugging

### Phase 4: Environmental Resistance (Low Priority - Polish)
14. ✅ **Add resist bonuses** to poison/disease functions
15. ✅ **Extend breath timer** based on STA
16. ✅ **Test environmental effects**

### Phase 5: Balance Validation
17. ✅ **Solo tank test**: Warrior tanks zone boss, verify survival time
18. ✅ **Caster sustain test**: Wizard nukes continuously, check mana sustainability
19. ✅ **Melee endurance test**: Berserker uses disciplines, verify uptime
20. ✅ **Cross-class comparison**: Ensure tanks/melee/priests/casters all feel distinct but viable

---

## Configuration Summary

**All values should have runtime keys with fallback defaults**:

```cpp
namespace CombatBalance {

// ============================================================================
// STAMINA SYSTEM CONFIGURATION
// ============================================================================

namespace STA_HP {
    static constexpr bool ENABLE_STA_HP_SCALING = true;

    // Base HP constants (CONSERVATIVE TUNING)
    static constexpr int BASE_HP = 100;           // Starting HP for all classes
    static constexpr int BASE_HP_PER_LEVEL = 5;   // Flat HP per level (tuned down from 10)
    static constexpr float STA_DIVISOR = 10.0f;   // STA * Level / Divisor (tuned up from 5)

    // Optional: Global scalar for entire formula
    static constexpr float HP_LEVEL_SCALAR = 1.0f;  // Adjust all HP (emergencies)

    // Class multipliers (applied to level AND STA scaling)
    static constexpr float WARRIOR_MULTIPLIER = 1.5f;
    static constexpr float CLERIC_MULTIPLIER = 1.0f;
    static constexpr float PALADIN_MULTIPLIER = 1.5f;
    static constexpr float RANGER_MULTIPLIER = 1.2f;
    static constexpr float SHADOWKNIGHT_MULTIPLIER = 1.5f;
    static constexpr float DRUID_MULTIPLIER = 1.0f;
    static constexpr float MONK_MULTIPLIER = 1.2f;
    static constexpr float BARD_MULTIPLIER = 1.2f;
    static constexpr float ROGUE_MULTIPLIER = 1.2f;
    static constexpr float SHAMAN_MULTIPLIER = 1.0f;
    static constexpr float NECROMANCER_MULTIPLIER = 1.3f;    // Higher HP for caster
    static constexpr float WIZARD_MULTIPLIER = 0.8f;
    static constexpr float MAGICIAN_MULTIPLIER = 0.8f;
    static constexpr float ENCHANTER_MULTIPLIER = 0.8f;
    static constexpr float BEASTLORD_MULTIPLIER = 1.2f;
    static constexpr float BERSERKER_MULTIPLIER = 1.2f;
}

namespace STA_REGEN {
    // HP Regeneration (LINEAR SCALING)
    static constexpr bool ENABLE_STA_HP_REGEN = true;
    static constexpr float HP_REGEN_DIVISOR = 20.0f;  // STA * Level / 20 = HP/tick

    // Mana Regeneration (LINEAR SCALING, casters only)
    static constexpr bool ENABLE_STA_MANA_REGEN = true;
    static constexpr float MANA_REGEN_DIVISOR = 50.0f;  // STA * Level / 50 = Mana/tick

    // Endurance Regeneration (LINEAR SCALING)
    static constexpr bool ENABLE_STA_ENDURANCE_REGEN = true;
    static constexpr float ENDURANCE_REGEN_DIVISOR = 20.0f;  // STA * Level / 20 = End/tick
}

namespace STA_MITIGATION {
    // NEW: Hybrid AC×STA Percent-Based Model
    static constexpr bool ENABLE_HYBRID_AC_STA_MITIGATION = true;

    // AC-side controls (tune existing AC system)
    static constexpr float AC_MITIGATION_SOFTCAP_SCALAR = 1.0f;  // Multiplier for AC effectiveness
    static constexpr float AC_MITIGATION_HARD_CAP = 50.0f;  // Max percent AC alone can provide
    // Optional: static constexpr float AC_MITIGATION_CURVE_STEEPNESS = 1.0f;

    // STA-side controls (LINEAR SCALING)
    static constexpr float STA_MIT_DIVISOR = 2000.0f;  // STA * Level / Divisor = STA ceiling %
    static constexpr float STA_MIT_MAX_PERCENT = 50.0f;  // Absolute max STA contribution
    static constexpr bool STA_MATCH_AC = true;  // If true, STA can only amplify existing AC

    // Global cap (prevent immunity)
    static constexpr float COMBAT_MAX_MITIGATION_PERCENT = 90.0f;  // Total mitigation hard cap
    static constexpr float DAMAGE_REDUCTION_DIVISOR = 100.0f;  // STA * Level / 100 = damage reduced
    static constexpr int MIN_DAMAGE_PERCENT = 25;  // Always take at least 25% of original damage
}

namespace STA_ENVIRONMENT {
    // Poison/Disease Resistance
    static constexpr bool ENABLE_STA_POISON_RESIST = true;
    static constexpr bool ENABLE_STA_DISEASE_RESIST = true;
    static constexpr float POISON_RESIST_DIVISOR = 5.0f;  // 1 resist per 5 STA
    static constexpr float DISEASE_RESIST_DIVISOR = 5.0f;

    // Breath Timer
    static constexpr bool ENABLE_STA_BREATH_BONUS = true;
    static constexpr float BREATH_BONUS_DIVISOR = 10.0f;  // 1 second per 10 STA
}

} // namespace CombatBalance
```

---

## Testing Strategy

### Unit Tests (Manual)

**HP Scaling Test**:
```cpp
// Test at multiple levels with different STA values
Test: Level 1 Warrior, 100 STA
Expected: ~145 HP
Actual: ___

Test: Level 70 Warrior, 1000 STA
Expected: ~22,000 HP
Actual: ___

Test: Level 70 Wizard, 500 STA
Expected: ~6,000 HP
Actual: ___
```

**Regen Test**:
```cpp
Test: Level 70 Warrior, 1000 STA
Expected HP Regen: ~3,500 HP/tick (35% of 10k HP pool)
Actual: ___

Test: Level 70 Wizard, 800 STA
Expected Mana Regen: ~1,120 Mana/tick
Actual: ___
```

**Mitigation Test**:
```cpp
Test: Level 70, 1000 STA, Boss hits for 2000
Expected: 1300 damage taken (700 reduced)
Actual: ___

Test: Level 70, 1000 STA, Trash hits for 500
Expected: 125 damage taken (capped at 25%)
Actual: ___
```

### Integration Tests (Gameplay)

**Solo Tank Test**:
1. Spawn Level 70 Warrior with 1000 STA
2. Pull zone boss that hits for 1500-2500 per swing
3. Fight for 2 minutes without any healing spells
4. Expected: Survive through HP regen + mitigation
5. Result: ___

**Caster Sustain Test**:
1. Spawn Level 70 Wizard with 800 STA, 20k mana
2. Nuke rotation: 2000 mana per cast, 3-second casts
3. Fight for 5 minutes
4. Expected: Mana never drops below 50%
5. Result: ___

**Melee DPS Test**:
1. Spawn Level 70 Berserker with 1200 STA
2. Use disciplines continuously (500 End per use)
3. Expected: Never run out of endurance
4. Result: ___

---

## Risk Assessment & Mitigation

### Risk 1: Regeneration Too Strong (God Mode)
**Symptom**: Players heal to full in 2-3 ticks, making all damage trivial.

**Mitigation**:
- Increase `HP_REGEN_DIVISOR` from 20 to 30 or 40
- Add combat penalty: reduce regen by 50% while in combat
- Cap maximum regen at X% of max HP
- Remember: Lower base HP pools (conservative tuning) = lower regen values

**Tuning Dial**: `HP_REGEN_DIVISOR` (20 → 30 → 40)

### Risk 2: Tanks vs Casters HP Gap Too Large
**Symptom**: Tanks have 4x the HP of casters, making casters unplayable.

**Mitigation**:
- Bring class multipliers closer together (1.5 → 1.3, 0.8 → 0.9)
- Increase `BASE_HP_PER_LEVEL` to lift all classes
- **Hybrid mitigation helps**: Casters with low AC get limited mitigation naturally

**Tuning Dial**: Class multipliers and base HP values

### Risk 3: AC×STA Mitigation Creates High-End Stacking
**Symptom**: L70 tanks with high AC + high STA reach 80-90% mitigation too easily.

**Mitigation**:
- **Already addressed**: 90% hard cap prevents immunity
- Tune `STA_MIT_DIVISOR` higher (2000 → 3000) for weaker STA contribution
- Lower `AC_MITIGATION_HARD_CAP` (50% → 40%) to reduce AC ceiling
- Monitor total mitigation % in combat logs

**Tuning Dials**:
- `STA_MIT_DIVISOR` (2000 → 3000 for weaker STA)
- `AC_MITIGATION_HARD_CAP` (50 → 40 for lower AC ceiling)
- `COMBAT_MAX_MITIGATION_PERCENT` (90 → 85 → 80 for stricter cap)

### Risk 4: Low AC Characters Feel Bad (No STA Benefit)
**Symptom**: Casters with 500 AC get almost no mitigation from STA.

**Mitigation**:
- **As designed**: This enforces class identity (tanks tank better)
- Disable `STA_MATCH_AC` to allow base STA contribution without AC requirement
- Ensure HP and regen compensate for low mitigation
- Alternative: Set `STA_MATCH_AC = false` for first 20% of STA contribution

**Tuning Dial**: `STA_MATCH_AC` (true/false toggle)

### Risk 5: Mana Regen Makes Resource Management Trivial
**Symptom**: Casters never run out of mana, eliminating strategic depth.

**Mitigation**:
- Increase `MANA_REGEN_DIVISOR` from 50 to 70 or 100
- Make mana regen scale only out of combat
- Add mana costs to spells to compensate

**Tuning Dial**: `MANA_REGEN_DIVISOR` (50 → 70 → 100)

### Risk 6: Low Level Experience Broken
**Symptom**: Level 1 characters too strong or too weak compared to classic.

**Mitigation**:
- `BASE_HP = 100` ensures survivable start (conservative)
- `BASE_HP_PER_LEVEL = 5` provides steady growth (reduced from 10)
- Can add minimum level check: `if (Level < 10) apply different formula`
- **Linear scaling** prevents exponential low-level power spikes

**Tuning Dial**: `BASE_HP` and `BASE_HP_PER_LEVEL`

---

## Class-Specific Impact Analysis (UPDATED - Conservative Tuning)

### Tanks (Warrior, SK, Paladin)
**HP**: 1.5x multiplier → **11-16k base HP** at L70 (gear adds more)
**Regen**: **~3-5k HP/tick** → Sustain through long fights
**Mitigation**: **60-75% total** (AC 40% + STA 35% amplification)
**Result**: Durable juggernauts with AC×STA synergy. Can face-tank bosses with gear investment.

### Melee DPS (Rogue, Berserker, Monk, Ranger, Bard, Beastlord)
**HP**: 1.2x multiplier → **8-12k base HP** at L70
**Regen**: **~2-4k HP/tick** → Sustain through trash packs
**Endurance**: **~3-5k End/tick** → High discipline uptime
**Mitigation**: **40-55% total** (medium AC + STA amplification)
**Result**: Bruisers. Can off-tank or DPS depending on situation.

### Priests (Cleric, Druid, Shaman)
**HP**: 1.0x multiplier → **6-8k base HP** at L70
**Regen**: **~2-3k HP/tick** → Self-healing reduces heal spam need
**Mana**: **~700-1k Mana/tick** → Can nuke AND heal without OOM
**Mitigation**: **30-40% total** (low AC limits STA amplification)
**Result**: Battle medics. Can melee/tank when needed, focus mana on offense.

### Casters (Wizard, Magician, Enchanter, Necromancer)
**HP**: 0.8x multiplier → **3-6k base HP** at L70
**Regen**: **~1-2k HP/tick** → Kiting/pet tanking viable
**Mana**: **~700-1.2k Mana/tick** → Sustained DPS without downtime
**Mitigation**: **20-35% total** (very low AC = minimal STA benefit)
**Result**: Light tanks with nukes. HP/regen keep them alive, mitigation limited by design.

---

## Summary & Recommendation

**FINAL COMMITTED ARCHITECTURE** ✅:

1. ✅ **HP Scaling**: Option B (Direct Scaling) with Conservative Tuning
   - Formula: `BASE_HP + (BASE_HP_PER_LEVEL × Level × ClassMult) + (STA × Level × ClassMult / STA_DIVISOR)`
   - Tuned down: `STA_DIVISOR = 10` (was 5), `BASE_HP_PER_LEVEL = 5` (was 10)
   - Target: 10-16k base HP at L70 (gear adds more)
   - **Linear scaling** - predictable growth at all levels

2. ✅ **Regen Systems**: Three Independent Linear Systems
   - HP Regen: `(STA × Level) / 20` HP/tick
   - Mana Regen: `(STA × Level) / 50` Mana/tick (casters only)
   - Endurance Regen: `(STA × Level) / 20` End/tick
   - **All linear** - no exponential spikes

3. ✅ **Damage Mitigation**: Hybrid AC×STA Percent Model (NEW)
   - AC provides base mitigation (existing system)
   - STA amplifies AC mitigation up to ceiling: `(STA × Level) / 2000`
   - Total capped at 90% max
   - **Synergy between defensive stats** - AC + STA work together
   - **Linear STA contribution** - predictable scaling

4. ✅ **Scaling Type**: Linear Across All Systems
   - No exponential formulas
   - No hard clamps or binary breakpoints
   - Predictable, tunable, clean math

**Implementation Priority**:
1. HP Scaling (Immediate impact on survivability)
2. HP Regen (Self-sufficiency foundation)
3. **Hybrid AC×STA Mitigation** (High priority - synergy system)
4. Mana/Endurance Regen (Class balance and sustainability)
5. Environmental (Polish)

**Tuning Philosophy**:
- **Start conservative** (higher divisors = weaker effects)
- Test actual gameplay at multiple levels (1, 10, 30, 50, 70)
- **Linear scaling commitment** - increase power via divisors, not exponentials
- Watch for gear interactions (HP bonuses, AC from items)
- Monitor regen sustainability (should not trivialize all damage)
- **AC×STA synergy** - ensure both stats remain valuable
- Remember: Solo server = power fantasy is OK, but predictable > explosive

**Key Differences from Original Plan**:
- ❌ Removed: Flat damage reduction (replaced with AC×STA hybrid)
- ✅ Added: Hybrid percent-based mitigation with AC synergy
- ✅ Tuned Down: HP scaling divisors (10 vs 5, 5 per level vs 10)
- ✅ Emphasized: Linear scaling commitment (no exponentials)
- ✅ Added: Multiple tuning knobs for AC and STA interaction

**Next Steps**:
1. Review final architecture and approve
2. Implement Phase 1 (HP Scaling with conservative values)
3. Test and tune HP divisors
4. Implement Phase 2 (HP Regen)
5. **Implement Phase 3** (Hybrid AC×STA Mitigation) - **High Priority**
6. Implement Phase 4 (Mana/Endurance Regen)
7. Iterate and balance based on actual gameplay data
