# Combat Mechanics API Reference

Complete reference for combat-related C++ functions and Lua scripting bindings.

> **Note:** All code references are from the actual codebase. Line numbers may shift with updates.

## Table of Contents

1. [C++ Combat Functions](#c-combat-functions)
2. [Special Attack Functions](#special-attack-functions)
3. [Damage Calculation Functions](#damage-calculation-functions)
4. [Mob Combat Methods](#mob-combat-methods)
5. [Lua Quest Bindings](#lua-quest-bindings)
6. [Rules Reference](#rules-reference)

---

## C++ Combat Functions

### zone/attack.cpp

#### Mob::Attack()
**Location:** [zone/attack.cpp#L1672](zone/attack.cpp#L1672)

Main melee attack entry point.

```cpp
// Actual signature from code:
bool Mob::Attack(Mob* other, int Hand, bool bRiposte, bool IsStrikethrough,
                 bool IsFromSpell, ExtraAttackOptions *opts)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `other` | `Mob*` | Target of the attack |
| `Hand` | `int` | `EQ::invslot::slotPrimary` or `slotSecondary` |
| `bRiposte` | `bool` | True if this is a riposte attack |
| `IsStrikethrough` | `bool` | True if strikethrough proc'd |
| `IsFromSpell` | `bool` | True if triggered by spell |
| `opts` | `ExtraAttackOptions*` | Optional modifiers |

---

#### Mob::CheckHitChance()
**Location:** [zone/attack.cpp#L432](zone/attack.cpp#L432)

Determines if an attack hits based on accuracy vs avoidance.

```cpp
bool Mob::CheckHitChance(Mob* other, DamageHitInfo &hit)
```

---

#### Mob::AvoidDamage()
**Location:** [zone/attack.cpp#L473](zone/attack.cpp#L473)

Processes avoidance checks: Dodge, Parry, Block, Riposte.

```cpp
bool Mob::AvoidDamage(Mob *other, DamageHitInfo &hit)
```

---

#### Mob::MeleeMitigation()
**Location:** [zone/attack.cpp#L1163](zone/attack.cpp#L1163)

Applies AC-based damage mitigation using `RollD20()`.

```cpp
void Mob::MeleeMitigation(Mob *attacker, DamageHitInfo &hit, ExtraAttackOptions *opts)
```

**Actual implementation:**
```cpp
// From zone/attack.cpp lines 1163-1210
void Mob::MeleeMitigation(Mob *attacker, DamageHitInfo &hit, ExtraAttackOptions *opts)
{
    // ... Lua hook check ...

    Mob* defender = this;
    auto mitigation = defender->GetMitigationAC();
    if (IsClient() && attacker->IsClient())
        mitigation = mitigation * 80 / 100; // 2004 PvP changes

    if (opts) {
        mitigation *= (1.0f - opts->armor_pen_percent);
        mitigation -= opts->armor_pen_flat;
    }

    auto roll = RollD20(hit.offense, mitigation);

    // Level difference adjustments
    const int level_diff = attacker->GetLevel() - GetLevel();
    const int level_diff_roll_check = RuleI(Combat, LevelDifferenceRollCheck);
    // ... level diff logic adjusts roll between 0.1 and 2.0 ...

    hit.damage_done = std::max(static_cast<int>(roll * static_cast<double>(hit.base_damage) + 0.5), 1);
}
```

---

#### Mob::DoAttack()
**Location:** [zone/attack.cpp#L1582](zone/attack.cpp#L1582)

Processes a single attack after hit determination.

```cpp
void Mob::DoAttack(Mob *other, DamageHitInfo &hit, ExtraAttackOptions *opts, bool FromRiposte)
```

---

#### Mob::TryCriticalHit()
**Location:** [zone/attack.cpp#L5654](zone/attack.cpp#L5654)

Checks and applies critical hits. Supports both legacy and new DEX-based systems.

```cpp
void Mob::TryCriticalHit(Mob *defender, DamageHitInfo &hit, ExtraAttackOptions *opts)
```

**Key logic (from actual code):**
```cpp
// New DEX-based crit system (when Combat:UseNewDexFormulas is true)
if (RuleB(Combat, UseNewDexFormulas)) {
    float dexChance = CalcDexCritChanceNew(this);
    int crit_chance_bonus = GetCriticalChanceBonus(hit.skill);
    float totalChance = dexChance + static_cast<float>(crit_chance_bonus);
    // ...
    int crit_mod = 100 + GetCritDmgMod(hit.skill);
    crit_mod += static_cast<int>(CalcDexCritDamageBonus(this, overflow));
    hit.damage_done = hit.damage_done * crit_mod / 100;
}
```

---

#### Mob::ApplyDamageTable()
**Location:** [zone/attack.cpp#L6265](zone/attack.cpp#L6265)

Applies level-based damage table multipliers (clients/bots only).

```cpp
void Mob::ApplyDamageTable(DamageHitInfo &hit)
```

---

#### Mob::CommonOutgoingHitSuccess()
**Location:** [zone/attack.cpp#L6682](zone/attack.cpp#L6682)

Called after a successful hit - applies mitigation, damage tables, crits, and bonus damage.

```cpp
void Mob::CommonOutgoingHitSuccess(Mob* defender, DamageHitInfo &hit, ExtraAttackOptions *opts)
```

---

#### Mob::AddToHateList()
**Location:** [zone/attack.cpp#L3178](zone/attack.cpp#L3178)

Adds or updates hate for aggro management.

```cpp
void Mob::AddToHateList(Mob* other, int64 hate = 0, int64 damage = 0,
                        bool iYellForHelp = true, bool bFrenzy = false,
                        bool iBuffTic = false, uint16 spell_id = 0, bool pet_command = false)
```

---

#### Mob::DamageShield()
**Location:** [zone/attack.cpp#L3432](zone/attack.cpp#L3432)

Processes damage shield reflection.

```cpp
void Mob::DamageShield(Mob* attacker, bool spell_ds)
```

---

## Special Attack Functions

### zone/special_attacks.cpp

#### Mob::GetBaseSkillDamage()
**Location:** [zone/special_attacks.cpp#L33](zone/special_attacks.cpp#L33)

**This is the main function for all special attack base damage calculations.**

```cpp
int Mob::GetBaseSkillDamage(EQ::skills::SkillType skill, Mob *target)
```

**Actual formulas from the code:**

##### Frenzy (SkillFrenzy) - Lines 63-97
```cpp
case EQ::skills::SkillFrenzy:
    if (IsClient()) {
        // WEAPON SCALING (our custom implementation)
        auto primary = CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary);
        if (primary && primary->GetItem()) {
            base = primary->GetItem()->Damage;
            base += (GetLevel() / 10);  // Small level bonus
        } else {
            // Fallback (stock logic)
            if (GetLevel() > 15) base += GetLevel() - 15;
            if (base > 23) base = 23;
            if (GetLevel() > 50) base += 2;
            if (GetLevel() > 54) base++;
            if (GetLevel() > 59) base++;
        }
    }
    return base;
```

##### Flying Kick (SkillFlyingKick) - Lines 99-130
```cpp
case EQ::skills::SkillFlyingKick: {
    float skill_bonus = skill_level / 9.0f;

    if (IsClient()) {
        // WEAPON SCALING (our custom implementation)
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

##### Kick / Round Kick - Lines 136-168
```cpp
case EQ::skills::SkillKick:
case EQ::skills::SkillRoundKick: {
    if (IsClient()) {
        // Weapon Scaling
        auto primary = CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary);
        if (primary && primary->GetItem()) {
            base = primary->GetItem()->Damage;
        } else {
            base = GetHandToHandDamage();
        }
        // Boot AC Bonus
        auto inst = CastToClient()->GetInv().GetItem(EQ::invslot::slotFeet);
        if (inst) {
            base += (int)(inst->GetItemArmorClass(true) / 10.0f);
        }
        base += (skill_level / 10);
    }
    return base;
}
```

##### Backstab (SkillBackstab) - Lines 219-256
```cpp
case EQ::skills::SkillBackstab: {
    float skill_bonus = static_cast<float>(skill_level) * 0.02f;
    base = 3;  // Base for NPCs

    if (IsClient()) {
        auto *inst = CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary);
        if (inst && inst->GetItem() && inst->GetItem()->ItemType == EQ::item::ItemType1HPiercing) {
            base = inst->GetItemBackstabDamage(true);
            if (!inst->GetItemBackstabDamage()) {
                base += inst->GetItemWeaponDamage(true);
            }
            // Elemental and bane damage added if not ignored by rules
        }
    }
    // FORMULA: base * (skill * 0.02 + 2.0)
    return static_cast<int>(static_cast<float>(base) * (skill_bonus + 2.0f));
}
```

##### Monk Skills (Dragon Punch, Eagle Strike, Tiger Claw) - Lines 37-62
```cpp
case EQ::skills::SkillDragonPunch:
case EQ::skills::SkillEagleStrike:
case EQ::skills::SkillTigerClaw:
    if (IsClient()) {
        // Weapon Scaling
        auto primary = CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary);
        if (primary && primary->GetItem()) {
            base = primary->GetItem()->Damage;
        } else {
            base = GetHandToHandDamage();
        }
        base += (skill_level / 15);  // Skill Bonus
    }
    return base;
```

---

#### Mob::DoSpecialAttackDamage()
**Location:** [zone/special_attacks.cpp#L262](zone/special_attacks.cpp#L262)

Entry point for applying special attack damage.

```cpp
void Mob::DoSpecialAttackDamage(Mob *who, EQ::skills::SkillType skill,
                                 int32 base_damage, int32 min_damage,
                                 int32 hate_override, int ReuseTime)
```

---

#### Mob::TryBackstab()
**Location:** [zone/special_attacks.cpp#L762](zone/special_attacks.cpp#L762)

Processes backstab attempt - checks position, weapon, and triggers damage.

```cpp
void Mob::TryBackstab(Mob *other, int ReuseTime)
```

**Key logic:**
- Checks `BehindMob()` for position
- Checks for Frontal Backstab AA (Seized Opportunity)
- Calls `RogueBackstab()` for actual damage
- Handles Double/Triple backstab from AAs

---

#### Mob::RogueBackstab()
**Location:** [zone/special_attacks.cpp#L861](zone/special_attacks.cpp#L861)

Applies backstab damage.

```cpp
void Mob::RogueBackstab(Mob* other, bool min_damage, int ReuseTime)
{
    // ...
    int base_damage = GetBaseSkillDamage(EQ::skills::SkillBackstab, other);
    hate = base_damage;
    DoSpecialAttackDamage(other, EQ::skills::SkillBackstab, base_damage, 0, hate, ReuseTime);
}
```

---

## Damage Calculation Functions

### DamageHitInfo Structure
**Location:** [zone/common.h#L837](zone/common.h#L837)

```cpp
// Actual structure from zone/common.h
struct DamageHitInfo {
    int64 base_damage;
    int64 min_damage;
    int64 damage_done;
    int offense;
    int tohit;
    int hand;
    EQ::skills::SkillType skill;
};
```

### DamageTable Structure
**Location:** [zone/common.h#L830](zone/common.h#L830)

```cpp
struct DamageTable {
    int32 max_extra;    // max extra damage percent
    int32 chance;       // chance not to apply
    int32 minusfactor;  // difficulty of rolling
};
```

---

## Mob Combat Methods

### Stat Accessors
**Location:** [zone/mob.h#L613-618](zone/mob.h#L613-618)

```cpp
// From zone/mob.h - actual declarations
inline int32 GetAC() const { return AC; }
inline virtual int32 GetATK() const { return ATK + itembonuses.ATK + spellbonuses.ATK; }
inline virtual int32 GetATKBonus() const { return itembonuses.ATK + spellbonuses.ATK; }
virtual int32 GetSTR() const;  // In .cpp for pet inheritance logic
inline virtual int32 GetSTA() const { return STA + itembonuses.STA + spellbonuses.STA; }
inline virtual int32 GetDEX() const { return DEX + itembonuses.DEX + spellbonuses.DEX; }
inline virtual int32 GetAGI() const { return AGI + itembonuses.AGI + spellbonuses.AGI; }
inline virtual int32 GetINT() const { return INT + itembonuses.INT + spellbonuses.INT; }
inline virtual int32 GetWIS() const { return WIS + itembonuses.WIS + spellbonuses.WIS; }
inline virtual int32 GetCHA() const { return CHA + itembonuses.CHA + spellbonuses.CHA; }
```

### Critical Hit Helpers
**Location:** [zone/mob.cpp#L6947](zone/mob.cpp#L6947)

```cpp
int Mob::GetCriticalChanceBonus(uint16 skill)
{
    // Combines item, spell, and AA crit bonuses
    critical_chance += itembonuses.CriticalHitChance[EQ::skills::HIGHEST_SKILL + 1]
                     + spellbonuses.CriticalHitChance[EQ::skills::HIGHEST_SKILL + 1]
                     + aabonuses.CriticalHitChance[EQ::skills::HIGHEST_SKILL + 1]
                     + itembonuses.CriticalHitChance[skill]
                     + spellbonuses.CriticalHitChance[skill]
                     + aabonuses.CriticalHitChance[skill];
    // ...
}
```

---

## Lua Quest Bindings

### Damage Functions

```lua
-- Deal damage to a mob
-- From zone/lua_mob.cpp
mob:Damage(attacker, amount, spell_id, skill_type, avoidable, buffslot, buff_tic, special)

-- Example: Deal 500 damage
e.other:Damage(e.self, 500, 0, Skill.HandtoHand, true, -1, false, 0)

-- Heal a mob
mob:HealDamage(amount, caster, spell_id)

-- Kill mob instantly
mob:Kill()

-- Set HP directly
mob:SetHP(1000)
mob:SetMaxHP(5000)
```

### Stat Functions

```lua
-- Get stats (returns int32)
local str = e.other:GetSTR()
local sta = e.other:GetSTA()
local dex = e.other:GetDEX()
local agi = e.other:GetAGI()
local ac = e.other:GetAC()
local atk = e.other:GetATK()

-- Get skill value
local backstab_skill = e.other:GetSkill(Skill.Backstab)
```

---

## Rules Reference

**Location:** [common/ruletypes.h#L556-690](common/ruletypes.h#L556-690)

### Critical Hit Rules (Actual)

```cpp
// NOTE: There is NO "MeleeBaseCritChance" rule!
// Crits use difficulty-based rolling:
RULE_INT(Combat, MeleeCritDifficulty, 8900, "Value against which is rolled to check if a melee crit is triggered. Lower is easier")
RULE_INT(Combat, ArcheryCritDifficulty, 3400, "Value against which is rolled to check if an archery crit is triggered")
RULE_INT(Combat, ThrowingCritDifficulty, 1100, "Value against which is rolled to check if a throwing crit is triggered")
RULE_BOOL(Combat, NPCCanCrit, false, "Setting whether an NPC can land critical hits")
RULE_INT(Combat, PetBaseCritChance, 0, "Pet base crit chance")

// New DEX-based system toggle:
RULE_BOOL(Combat, UseNewDexFormulas, true, "Enable new DEX-based precision system")
```

### Special Attack Base Damage Rules

```cpp
RULE_INT(Combat, ArcheryBaseDamage, 0, "Archery base damage, default is 0")
RULE_INT(Combat, BackstabBaseDamage, 0, "Backstab base damage, default is 0")
RULE_INT(Combat, BashBaseDamage, 2, "Bash base damage, default is 2")
RULE_INT(Combat, DragonPunchBaseDamage, 12, "Dragon Punch base damage, default is 12")
RULE_INT(Combat, EagleStrikeBaseDamage, 7, "Eagle Strike base damage, default is 7")
RULE_INT(Combat, FlyingKickBaseDamage, 25, "Flying Kick base damage, default is 25")
RULE_INT(Combat, FrenzyBaseDamage, 10, "Frenzy base damage, default is 10")
RULE_INT(Combat, KickBaseDamage, 3, "Kick base damage, default is 3")
RULE_INT(Combat, RoundKickBaseDamage, 5, "Round Kick base damage, default is 5")
RULE_INT(Combat, TigerClawBaseDamage, 4, "Tiger Claw base damage, default is 4")
```

### Mitigation Rules

```cpp
RULE_INT(Combat, LevelDifferenceRollCheck, -1, "Level Difference to enable LevelDifferenceRollBonus (-1 = disabled)")
RULE_REAL(Combat, LevelDifferenceRollBonus, 0.5, "Roll Bonus/Detriment if using LevelDifferenceRollCheck")
```

### Proc Rules

```cpp
RULE_BOOL(Combat, AdjustProcPerMinute, true, "Adapt proc rate to weapon speed")
RULE_REAL(Combat, AvgProcsPerMinute, 2.0, "Average proc rate per minute")
RULE_REAL(Combat, BaseProcChance, 0.035, "Base chance for procs")
RULE_REAL(Combat, ProcDexDivideBy, 11000, "Divisor for DEX contribution to proc chance")
```

### Haste Rules

```cpp
RULE_INT(Combat, MinHastedDelay, 400, "Minimum hasted combat delay")
RULE_INT(Combat, QuiverHasteCap, 1000, "Quiver haste cap")
```

### Access in Code

```cpp
// Integer rules
int crit_difficulty = RuleI(Combat, MeleeCritDifficulty);

// Real/float rules
float proc_chance = RuleR(Combat, BaseProcChance);

// Boolean rules
bool npc_can_crit = RuleB(Combat, NPCCanCrit);
```

---

## Key File Locations Summary

| System | File | Key Functions |
|--------|------|---------------|
| Main Attack | [zone/attack.cpp](zone/attack.cpp) | `Attack()`, `DoAttack()`, `MeleeMitigation()` |
| Hit/Avoid | [zone/attack.cpp](zone/attack.cpp) | `CheckHitChance()`, `AvoidDamage()` |
| Crits | [zone/attack.cpp](zone/attack.cpp) | `TryCriticalHit()`, `GetCriticalChanceBonus()` |
| Damage Tables | [zone/attack.cpp](zone/attack.cpp) | `ApplyDamageTable()` |
| Special Attacks | [zone/special_attacks.cpp](zone/special_attacks.cpp) | `GetBaseSkillDamage()`, `DoSpecialAttackDamage()` |
| Backstab | [zone/special_attacks.cpp](zone/special_attacks.cpp) | `TryBackstab()`, `RogueBackstab()` |
| Mob Stats | [zone/mob.h](zone/mob.h) | `GetAC()`, `GetATK()`, `GetSTR()`, etc. |
| Structures | [zone/common.h](zone/common.h) | `DamageHitInfo`, `DamageTable` |
| Rules | [common/ruletypes.h](common/ruletypes.h) | All `RULE_*` definitions |
