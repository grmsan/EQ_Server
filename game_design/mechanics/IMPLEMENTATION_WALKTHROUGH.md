# Combat Mechanics Implementation Walkthrough

Step-by-step guide to implementing and modifying combat mechanics with accurate code references.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Understanding the Code Flow](#understanding-the-code-flow)
3. [Tutorial: Modify Frenzy Formula](#tutorial-modify-frenzy-formula)
4. [Tutorial: Add a New Special Attack](#tutorial-add-a-new-special-attack)
5. [Tutorial: Modify Damage Pipeline](#tutorial-modify-damage-pipeline)
6. [Common Modifications](#common-modifications)
7. [Troubleshooting](#troubleshooting)

---

## Prerequisites

Before making combat changes:

1. **Read the design docs:**
   - [README.md](README.md) - Overview
   - [API_REFERENCE.md](API_REFERENCE.md) - Key functions
   - [SPECIAL_ATTACKS.md](SPECIAL_ATTACKS.md) - Current formulas

2. **Understand the key files:**
   - `zone/special_attacks.cpp` - Special attack formulas (`GetBaseSkillDamage()` at line 33)
   - `zone/attack.cpp` - Main attack processing (`MeleeMitigation()` at line 1163)
   - `common/ruletypes.h` - Combat rules (lines 556-690)

3. **Set up test environment:**
   - Follow [TESTING_GUIDE.md](TESTING_GUIDE.md)
   - Create test dummy NPCs
   - Enable combat logging

---

## Understanding the Code Flow

### Special Attack Damage Flow

```
1. Player uses skill (e.g., Frenzy button)
       ↓
2. Client sends OP_CombatAbility packet
       ↓
3. zone/client_packet.cpp handles packet
       ↓
4. Calls DoMeleeSkillAttackDmg() [zone/special_attacks.cpp#L2589]
       ↓
5. DoMeleeSkillAttackDmg() calls GetBaseSkillDamage() [zone/special_attacks.cpp#L33]
       ↓
6. GetBaseSkillDamage() has switch statement for each skill type
       ↓
7. Base damage goes through mitigation (MeleeMitigation)
       ↓
8. Damage tables applied (ApplyDamageTable)
       ↓
9. Critical hits checked (TryCriticalHit)
       ↓
10. Final damage dealt
```

### Key Function: GetBaseSkillDamage

This is the central function for all special attack base damage. Located at `zone/special_attacks.cpp` line 33:

```cpp
int Mob::GetBaseSkillDamage(EQ::skills::SkillType skill, Mob *target)
{
    int base = EQ::skills::GetBaseDamage(skill);  // From rules
    auto skill_level = GetSkill(skill);

    switch (skill) {
        case EQ::skills::SkillFrenzy:
            // Frenzy formula here (lines 63-97)
            break;
        case EQ::skills::SkillFlyingKick:
            // Flying kick formula here (lines 99-130)
            break;
        case EQ::skills::SkillBackstab:
            // Backstab formula here (lines 219-256)
            break;
        // etc.
    }
    return base;
}
```

---

## Tutorial: Modify Frenzy Formula

**Goal:** Understand how Frenzy works and make adjustments.

### Step 1: Find Current Formula

Open `zone/special_attacks.cpp` and look at lines 63-97:

```cpp
case EQ::skills::SkillFrenzy:
    if (IsClient()) {
        // Current implementation: weapon scaling for clients
        auto primary = CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary);
        if (primary && primary->GetItem()) {
            base = primary->GetItem()->Damage;
            base += (GetLevel() / 10);  // Small level bonus
        } else {
            // Fallback for unarmed
            if (GetLevel() > 15) base += GetLevel() - 15;
            if (base > 23) base = 23;
        }
    } else {
        // NPCs use old logic
        if (GetLevel() > 15) base += GetLevel() - 15;
        if (base > 23) base = 23;
    }
    return base;
```

### Step 2: Design Your Change

The current formula already includes weapon scaling. If you want to change it:

**Example: Add skill scaling**
```
New Formula = Weapon_Damage + (Level / 10) + (Skill / 50)
```

### Step 3: Implement the Change

Edit `zone/special_attacks.cpp` around line 70:

```cpp
case EQ::skills::SkillFrenzy:
    if (IsClient()) {
        auto primary = CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary);
        if (primary && primary->GetItem()) {
            base = primary->GetItem()->Damage;
            base += (GetLevel() / 10);
            base += (skill_level / 50);  // NEW: Add skill scaling

            // Debug logging
            Log(Logs::Detail, Logs::Combat,
                "Frenzy: weapon={} level_bonus={} skill_bonus={} total={}",
                primary->GetItem()->Damage, GetLevel()/10, skill_level/50, base);
        } else {
            // Fallback for unarmed (keep existing logic)
            if (GetLevel() > 15) base += GetLevel() - 15;
            if (base > 23) base = 23;
        }
    }
    return base;
```

### Step 4: Alternative - Use Rules

Instead of code changes, adjust the rule value in the database:

```sql
-- Increase frenzy base damage (default is 10)
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
VALUES (1, 'Combat:FrenzyBaseDamage', '25', 'Buffed frenzy base');
```

The `EQ::skills::GetBaseDamage(skill)` function reads from rules (defined in common/ruletypes.h#L681).

### Step 5: Build and Test

```powershell
# Build zone
cmake --build build --target zone --config RelWithDebInfo --parallel

# Test in-game:
# 1. #spawn a test dummy
# 2. Use Frenzy as a Berserker
# 3. Check logs/zone/*.log for Combat output
```

---

## Tutorial: Add a New Special Attack

**Goal:** Add "Power Strike" - a warrior ability that scales with STR.

### Step 1: Choose a Skill ID

Look at `common/skills.h` for existing skills. Use an unused ID or one appropriate for your needs.

### Step 2: Add to GetBaseSkillDamage

In `zone/special_attacks.cpp`, add a new case in the switch statement (around line 200):

```cpp
case EQ::skills::SkillBash:  // Or your chosen skill
    // ... existing code ...

// ADD YOUR NEW CASE HERE (if using a new skill ID)
case EQ::skills::SkillPowerStrike:  // Assuming you defined this
    if (IsClient()) {
        int str_bonus = GetSTR() / 10;
        int weapon_bonus = 0;

        auto primary = CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary);
        if (primary && primary->GetItem()) {
            weapon_bonus = primary->GetItem()->Damage / 2;
        }

        base = str_bonus + weapon_bonus;

        Log(Logs::Detail, Logs::Combat,
            "PowerStrike: STR={} str_bonus={} weapon_bonus={} total={}",
            GetSTR(), str_bonus, weapon_bonus, base);
    }
    return base;
```

### Step 3: Add Rule for Base Damage (Optional)

In `common/ruletypes.h` around line 685:

```cpp
RULE_INT(Combat, PowerStrikeBaseDamage, 10, "Power Strike base damage, default is 10")
```

Then use it:
```cpp
base = RuleI(Combat, PowerStrikeBaseDamage);
```

### Step 4: Add Skill to Database

```sql
-- Add skill_caps entry for warriors
INSERT INTO skill_caps (class, skill, level, cap)
VALUES
(1, YOUR_SKILL_ID, 1, 200),
(1, YOUR_SKILL_ID, 51, 250),
(1, YOUR_SKILL_ID, 60, 300);
```

### Step 5: Hook Ability Activation

This depends on how the ability is triggered. Most combat abilities go through `zone/client_packet.cpp`.

For disc-style abilities, look at `zone/client.cpp` for discipline handling.

---

## Tutorial: Modify Damage Pipeline

### Key Points in Pipeline

1. **MeleeMitigation** (zone/attack.cpp#L1163)
   - Applies AC-based damage reduction
   - Uses `RollD20()` and `GetMitigationAC()`

2. **ApplyDamageTable** (zone/attack.cpp#L6265)
   - Applies level-based multipliers
   - Higher level = higher multiplier

3. **TryCriticalHit** (zone/attack.cpp#L5654)
   - Checks for critical hits
   - Uses `MeleeCritDifficulty` rule (default 8900)
   - When `UseNewDexFormulas` is true, DEX affects crit chance

### Example: Add Global Damage Multiplier

In `zone/attack.cpp`, find where damage is finalized and add:

```cpp
// After mitigation, before sending to client
float global_mult = 1.0f;

// Example: Time of day multiplier
if (IsNight()) {
    global_mult = 1.2f;  // 20% more damage at night
}

hit.damage_done = static_cast<int64>(hit.damage_done * global_mult);
```

### Example: Modify Crit System

To adjust crit difficulty without code changes:

```sql
-- Make crits easier (lower = easier, default 8900)
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
VALUES (1, 'Combat:MeleeCritDifficulty', '5000', 'Easier melee crits');
```

---

## Common Modifications

### Increase All Special Attack Damage

Adjust base damage rules in database:

```sql
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
VALUES
    (1, 'Combat:FrenzyBaseDamage', '20', 'Doubled'),
    (1, 'Combat:FlyingKickBaseDamage', '50', 'Doubled'),
    (1, 'Combat:KickBaseDamage', '6', 'Doubled'),
    (1, 'Combat:BackstabBaseDamage', '5', 'Added base');
```

### Enable New Stat Formulas

```sql
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
VALUES
    (1, 'Combat:UseNewDexFormulas', 'true', 'DEX affects crits'),
    (1, 'Combat:UseNewAgiFormulas', 'true', 'AGI affects avoidance'),
    (1, 'Combat:UseNewStrDamageFormula', 'true', 'STR affects damage');
```

### Modify Backstab Multiplier

In `zone/special_attacks.cpp` around line 230:

```cpp
// Current: skill * 0.02 + 2.0 = max 8.0x at 300 skill
float multiplier = GetSkill(EQ::skills::SkillBackstab) * 0.02f + 2.0f;

// To reduce: change 0.02 to 0.015 = max 6.5x at 300 skill
float multiplier = GetSkill(EQ::skills::SkillBackstab) * 0.015f + 2.0f;
```

---

## Troubleshooting

### Damage Values Wrong

1. Add debug logging to `GetBaseSkillDamage()`:
```cpp
Log(Logs::Detail, Logs::Combat, "GetBaseSkillDamage: skill={} base={}", skill, base);
```

2. Check what rules are loaded:
```sql
SELECT * FROM rule_values WHERE rule_name LIKE '%BaseDamage%';
```

3. Verify weapon damage:
```sql
SELECT id, name, damage FROM items WHERE id = YOUR_WEAPON_ID;
```

### Changes Not Taking Effect

1. **Rebuild zone:** `cmake --build build --target zone`
2. **Restart zone:** Changes only apply after zone restart
3. **Rule changes:** May require `#rules reload` or zone restart

### Crashes

1. Check for NULL pointers:
```cpp
if (!primary || !primary->GetItem()) {
    return base;  // Early return with safe default
}
```

2. Check log files for stack traces
3. Run with debugger attached

### Performance Issues

For mass combat, avoid expensive operations in tight loops:

```cpp
// BAD: String formatting in hot path
Log(Logs::Detail, Logs::Combat, "Damage: {}", damage);

// GOOD: Only log when debugging
#ifdef DEBUG_COMBAT
Log(Logs::Detail, Logs::Combat, "Damage: {}", damage);
#endif
```

---

## Quick Reference

| File | Purpose | Key Functions |
|------|---------|---------------|
| zone/special_attacks.cpp | Special attack formulas | `GetBaseSkillDamage()` (L33), `DoMeleeSkillAttackDmg()` (L2589) |
| zone/attack.cpp | Main combat processing | `MeleeMitigation()` (L1163), `TryCriticalHit()` (L5654), `ApplyDamageTable()` (L6265) |
| common/ruletypes.h | Combat rules | Lines 556-690 |
| zone/common.h | DamageHitInfo struct | Line 837 |

**Rule Prefix Format:** `Combat:RuleName` in database, `RuleI(Combat, RuleName)` in code.
