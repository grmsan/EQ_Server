# Combat Mechanics Tuning Guide

How to balance and tune combat formulas for your server using actual code references.

## Table of Contents

1. [Tuning Philosophy](#tuning-philosophy)
2. [Key Tuning Points](#key-tuning-points)
3. [Rules-Based Tuning](#rules-based-tuning)
4. [Formula Tuning](#formula-tuning)
5. [Database Tuning](#database-tuning)
6. [Testing Methodology](#testing-methodology)
7. [Common Scenarios](#common-scenarios)

---

## Tuning Philosophy

### Goals for This Server

1. **High-Stat Scaling** - Combat should feel powerful with 1000+ stats
2. **Weapon Matters** - Better weapons = noticeably more damage
3. **Solo Viable** - Single players can progress (slowly)
4. **Group Rewarding** - Groups progress faster
5. **No Hard Caps** - Damage should scale, not plateau

### Balance Principles

```
Time to Kill (TTK) Guidelines:
- Trivial mob:    1-3 seconds
- Normal mob:     10-30 seconds solo
- Named mob:      1-3 minutes solo, 30-60 seconds grouped
- Raid boss:      5-15 minutes with raid
```

### Stat Expectations

| Level | STR/STA | ATK | AC | HP |
|-------|---------|-----|----|----|
| 10 | 100-150 | 50-100 | 100-200 | 500-1000 |
| 30 | 200-300 | 150-300 | 300-500 | 2000-4000 |
| 50 | 300-500 | 300-500 | 500-800 | 5000-10000 |
| 65 | 500-800 | 500-800 | 800-1200 | 10000-20000 |
| 65+ (geared) | 1000-2000 | 1000-2000 | 1500-3000 | 25000-50000 |

---

## Key Tuning Points

### 1. Base Damage (Special Attacks)

Base damage for special attacks is controlled by rules in `common/ruletypes.h` lines 675-686:

```cpp
// Actual rules from common/ruletypes.h
RULE_INT(Combat, ArcheryBaseDamage, 0, "Archery base damage, default is 0")
RULE_INT(Combat, BackstabBaseDamage, 0, "Backstab base damage, default is 0")
RULE_INT(Combat, BashBaseDamage, 2, "Bash base damage, default is 2")
RULE_INT(Combat, DragonPunchBaseDamage, 12, "Dragon Punch base damage, default is 12")
RULE_INT(Combat, EagleStrikeBaseDamage, 7, "Eagle Strike base damage, default is 7")
RULE_INT(Combat, FlyingKickBaseDamage, 25, "Flying Kick base damage, default is 25")
RULE_INT(Combat, FrenzyBaseDamage, 10, "Frenzy base damage, default is 10")
RULE_INT(Combat, KickBaseDamage, 3, "Kick base damage, default is 3")
RULE_INT(Combat, RoundKickBaseDamage, 5, "Round Kick base damage, default is 5")
RULE_INT(Combat, ThrowingBaseDamage, 0, "Throwing base damage, default is 0")
RULE_INT(Combat, TigerClawBaseDamage, 4, "Tiger Claw base damage, default is 4")
```

**Tuning Lever:** Change these rule values in the `rule_values` database table.

### 2. Critical Hit Difficulty

Critical hits use a "difficulty" system, not a percentage. Lower = easier to crit.

From `common/ruletypes.h` lines 562-565:
```cpp
RULE_INT(Combat, MeleeCritDifficulty, 8900, "Value against which is rolled to check if a melee crit is triggered. Lower is easier")
RULE_INT(Combat, ArcheryCritDifficulty, 3400, "Value against which is rolled to check if an archery crit is triggered. Lower is easier")
RULE_INT(Combat, ThrowingCritDifficulty, 1100, "Value against which is rolled to check if a throwing crit is triggered. Lower is easier")
RULE_BOOL(Combat, NPCCanCrit, false, "Setting whether an NPC can land critical hits")
```

**How It Works (from `zone/attack.cpp` line 5654):**
```cpp
// TryCriticalHit() - actual code
int32 dex = GetDEX();
if (RuleB(Combat, UseNewDexFormulas)) {
    // DEX-based calculation: higher DEX = lower effective difficulty
    // This makes the system scale with character progression
}
```

**Tuning Lever:** Adjust `MeleeCritDifficulty`. Default 8900 is hard. Try 5000 for more crits.

### 3. Mitigation

Mitigation is handled by `MeleeMitigation()` in `zone/attack.cpp` line 1163.

```cpp
// Actual structure (zone/attack.cpp#L1163)
void Mob::MeleeMitigation(Mob *attacker, DamageHitInfo &hit, ExtraAttackOptions *opts, bool riposte)
{
    // Uses RollD20() at line 1254 for mitigation roll
    // Uses GetMitigationAC() for AC-based damage reduction
    // Level difference matters: LevelDifferenceRollCheck rule
}
```

**Related Rules:**
```cpp
// common/ruletypes.h lines 566-570
RULE_BOOL(Combat, UseIntervalAC, true, "Switch whether bonuses, armour class, multipliers...")
RULE_INT(Combat, LevelDifferenceRollCheck, -1, "Level Difference to enable LeverDifferenceRollBonus...")
RULE_REAL(Combat, LevelDifferenceRollBonus, 0.5, "Roll Bonus/Detrement if using LevelDifferenceRollCheck")
```

### 4. Damage Tables

Level-based multipliers applied after mitigation via `ApplyDamageTable()` at `zone/attack.cpp` line 6265.

```cpp
// ApplyDamageTable calculates multipliers based on attacker/defender levels
// This affects final damage output significantly at higher levels
```

**Tuning:** The damage table code is complex and level-dependent. To adjust, modify the formulas in `ApplyDamageTable()`.

### 5. Proc Rates

Proc system uses per-minute averaging. From `common/ruletypes.h` lines 577-581:

```cpp
RULE_BOOL(Combat, AdjustProcPerMinute, true, "Adapt the average proc rate to the speed of the weapon")
RULE_REAL(Combat, AvgProcsPerMinute, 2.0, "Average proc rate per minute")
RULE_REAL(Combat, ProcPerMinDexContrib, 0.075, "Increases the probability of a proc increased by DEX")
RULE_REAL(Combat, BaseProcChance, 0.035, "Base chance for procs")
RULE_REAL(Combat, ProcDexDivideBy, 11000, "Divisor for the probability of a proc increased by dexterity")
```

**Tuning Lever:** Increase `AvgProcsPerMinute` from 2.0 to 3.0 for more procs.

---

## Rules-Based Tuning

Rules in `common/ruletypes.h` provide tuning without recompiling. Set values in the `rule_values` database table.

### Critical Hits (Actual Rules)

```sql
-- Make melee crits easier (lower = easier)
-- Default is 8900, try 5000 for more crits
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
VALUES (1, 'Combat:MeleeCritDifficulty', '5000', 'Easier melee crits');

-- Enable NPC crits (default false)
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
VALUES (1, 'Combat:NPCCanCrit', 'true', 'Allow NPCs to crit');

-- Pet base crit chance
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
VALUES (1, 'Combat:PetBaseCritChance', '5', 'Give pets 5% base crit');
```

### Stat System Toggles

```cpp
// common/ruletypes.h lines 636-639 - Enable/disable new stat formulas
RULE_BOOL(Combat, UseNewStrDamageFormula, false, "Enable new STR-based damage system...")
RULE_BOOL(Combat, UseNewStaminaFormula, false, "Enable new STA-based sustainability system...")
RULE_BOOL(Combat, UseNewDexFormulas, true, "Enable new DEX-based precision system...")
RULE_BOOL(Combat, UseNewAgiFormulas, true, "Enable new AGI-based speed/avoid formulas...")
```

**Set in database:**
```sql
-- Enable new STR damage formula
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
VALUES (1, 'Combat:UseNewStrDamageFormula', 'true', 'Enable STR scaling');
```

### Melee Push Settings

```cpp
// common/ruletypes.h lines 606-610
RULE_BOOL(Combat, MeleePush, true, "Enable melee push")
RULE_INT(Combat, MeleePushChance, 50, "NPC chance the target will be pushed")
RULE_REAL(Combat, MeleePushForceClientPercent, 0.00, "Percent to add/remove from push for players")
RULE_REAL(Combat, MeleePushForcePetPercent, 0.00, "Percent to add/remove from push for pets")
RULE_BOOL(Combat, NPCtoNPCPush, false, "NPC to NPC pushing")
```

### Attack Speed / Haste

```cpp
// common/ruletypes.h line 591
RULE_INT(Combat, MinHastedDelay, 400, "Minimum hasted combat delay")
RULE_INT(Combat, QuiverHasteCap, 1000, "Quiver haste cap")
```

### Flurry and Rampage

```cpp
// common/ruletypes.h lines 587-594
RULE_INT(Combat, MaxRampageTargets, 3, "Maximum number of people hit with rampage")
RULE_INT(Combat, DefaultRampageTargets, 1, "Default number of people to hit with rampage")
RULE_BOOL(Combat, RampageHitsTarget, false, "Rampage will hit the target if it still has targets left")
RULE_INT(Combat, MaxFlurryHits, 2, "Maximum number of extra hits from flurry")
RULE_INT(Combat, NPCFlurryChance, 20, "Chance for NPC to flurry")
```

### Archery Settings

```cpp
// common/ruletypes.h lines 582-586, 644-646
RULE_INT(Combat, MinRangedAttackDist, 25, "Minimum Distance to use Ranged Attacks")
RULE_BOOL(Combat, ArcheryBonusRequiresStationary, true, "does the 2x archery bonus require stationary npc")
RULE_INT(Combat, ArcheryBonusLevelRequirement, 51, "Level requirement for 2x archery bonus")
RULE_REAL(Combat, ArcheryNPCMultiplier, 1.0, "Value multiplied by dmg for archery dmg")
RULE_REAL(Combat, ArcheryHitPenalty, 0, "Archery hit penalty")
RULE_REAL(Combat, ArcheryBaseDamageBonus, 1, "Percentage modifier to base archery Damage")
```

### Backstab Settings

```cpp
// common/ruletypes.h lines 619, 632-634, 640-642
RULE_BOOL(Combat, ClassicNPCBackstab, false, "True disables NPC facestab")
RULE_BOOL(Combat, AssassinateOnlyHumanoids, true, "Assassinate only on Humanoids")
RULE_INT(Combat, AssassinateLevelRequirement, 60, "Level requirement for assassinate")
RULE_BOOL(Combat, BackstabIgnoresElemental, false, "Elemental damage affecting backstab")
RULE_BOOL(Combat, BackstabIgnoresBane, false, "Bane damage affecting backstab")
RULE_INT(Combat, DoubleBackstabLevelRequirement, 55, "Level requirement for double backstab")
```

---

## Formula Tuning (Code Changes)

### Special Attack Formulas

The main function is `GetBaseSkillDamage()` in `zone/special_attacks.cpp` line 33.

**Frenzy (lines 63-97):**
```cpp
// Actual code - already includes weapon scaling
case EQ::skills::SkillFrenzy:
    return RuleI(Combat, FrenzyBaseDamage) +
           GetLevel() / 10 +
           GetSkill(EQ::skills::SkillFrenzy) / 50 +
           (weapon_damage / 4);  // weapon scaling
```

**Flying Kick (lines 99-130):**
```cpp
// Actual code - includes weapon, skill, and boot AC
case EQ::skills::SkillFlyingKick:
    return RuleI(Combat, FlyingKickBaseDamage) +
           weapon_damage +
           GetSkill(EQ::skills::SkillFlyingKick) / 9 +
           (boot_ac / 25);
```

**Backstab (lines 219-256):**
```cpp
// Actual multiplier formula
float multiplier = GetSkill(EQ::skills::SkillBackstab) * 0.02f + 2.0f;
// At 300 skill: 300 * 0.02 + 2.0 = 8.0x multiplier
```

### Tuning Special Attack Coefficients

To change these formulas, edit `zone/special_attacks.cpp`:

```cpp
// Example: Increase weapon scaling for Frenzy
// Change from: (weapon_damage / 4)
// To: (weapon_damage / 2)  // doubles weapon contribution

// Example: Reduce backstab multiplier cap
// Change from: GetSkill(EQ::skills::SkillBackstab) * 0.02f + 2.0f
// To: GetSkill(EQ::skills::SkillBackstab) * 0.015f + 2.0f
// Max becomes 6.5x instead of 8.0x at 300 skill
```

---

## Database Tuning

### Adjusting Special Attack Base Damage

```sql
-- Increase Frenzy base damage (default 10)
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
VALUES (1, 'Combat:FrenzyBaseDamage', '20', 'Doubled frenzy base');

-- Increase Flying Kick base (default 25)
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
VALUES (1, 'Combat:FlyingKickBaseDamage', '40', 'Buffed monk flying kick');

-- Adjust all monk skills
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
VALUES (1, 'Combat:DragonPunchBaseDamage', '20', 'Buffed dragon punch'),
       (1, 'Combat:EagleStrikeBaseDamage', '12', 'Buffed eagle strike'),
       (1, 'Combat:TigerClawBaseDamage', '8', 'Buffed tiger claw');
```

### NPC Difficulty

```sql
-- Make all mobs in a zone tougher
UPDATE npc_types
SET hp = hp * 1.5, mindmg = mindmg * 1.2, maxdmg = maxdmg * 1.2
WHERE id IN (SELECT npcid FROM spawnentry
             WHERE spawngroupid IN (SELECT spawngroupid FROM spawn2 WHERE zone = 'guk'));

-- Scale specific mob
UPDATE npc_types SET
    hp = 50000,
    ac = 400,
    mindmg = 100,
    maxdmg = 200,
    attack_speed = 80  -- Lower = faster
WHERE id = 12345;
```

### Skill Caps

```sql
-- Increase backstab cap for rogues
UPDATE skill_caps SET cap = 350
WHERE class = 9 AND skill = 8 AND level >= 60;

-- Check current caps
SELECT class, skill, level, cap FROM skill_caps
WHERE skill = 8  -- Backstab (SkillBackstab)
ORDER BY class, level;
```

---

## Testing Methodology

### Baseline Testing

Before tuning, establish baselines:

```
Test Setup:
- Character: Level 65, 300 skill, 500 STR
- Weapon: 50 damage sword
- Target: Test dummy with 500 AC, 100000 HP

Record:
- Average damage per hit
- DPS over 1 minute
- Critical hit rate (watch for yellow damage numbers)
- Minimum/maximum hits
```

### Using Debug Logs

Enable combat logging in `zone/attack.cpp`:
```cpp
// Look for Log calls like:
Log(Logs::Detail, Logs::Combat, ...)
```

Enable in `eqemu_config.json`:
```json
"logs": {
    "Combat": "1"
}
```

### Incremental Changes

Change one variable at a time:

```
Round 1: Baseline with default MeleeCritDifficulty (8900)
  Crit rate: ~5%

Round 2: Change MeleeCritDifficulty to 5000
  Crit rate: ~12%

Round 3: Change MeleeCritDifficulty to 3000
  Crit rate: ~20%
```

---

## Common Scenarios

### Scenario: Crits Too Rare

**Symptoms:** Players complain about never seeing crits

**Solution:**
```sql
-- Lower crit difficulty (default 8900)
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
VALUES (1, 'Combat:MeleeCritDifficulty', '5000', 'Easier melee crits');
```

### Scenario: Special Attacks Feel Weak

**Symptoms:** Frenzy/Flying Kick do less than auto-attack

**Solution:**
```sql
-- Increase base damages
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
VALUES
    (1, 'Combat:FrenzyBaseDamage', '20', 'Double base'),
    (1, 'Combat:FlyingKickBaseDamage', '40', '60% increase');
```

### Scenario: Weapon Damage Doesn't Matter

**Symptoms:** 100 dmg weapon not much better than 50 dmg

**Diagnosis:** Check if weapon scaling is in formulas

**Solution (code change in `zone/special_attacks.cpp`):**
```cpp
// Increase weapon coefficient
// Current Frenzy: weapon_damage / 4
// Changed: weapon_damage / 2
return RuleI(Combat, FrenzyBaseDamage) +
       GetLevel() / 10 +
       GetSkill(EQ::skills::SkillFrenzy) / 50 +
       (weapon_damage / 2);  // increased from /4
```

### Scenario: Combat Too Slow

**Symptoms:** Fights take too long

**Solutions:**
```sql
-- Reduce mob HP globally
UPDATE npc_types SET hp = hp * 0.75;

-- Enable easier crits
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
VALUES (1, 'Combat:MeleeCritDifficulty', '4000', 'Much easier crits');

-- Increase proc rate
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
VALUES (1, 'Combat:AvgProcsPerMinute', '3.0', 'More procs');
```

---

## Quick Tuning Cheat Sheet

| Want | Rule | Default | Code Location |
|------|------|---------|---------------|
| More melee crits | `Combat:MeleeCritDifficulty` | 8900 | ruletypes.h#L562 |
| More archery crits | `Combat:ArcheryCritDifficulty` | 3400 | ruletypes.h#L563 |
| NPC crits | `Combat:NPCCanCrit` | false | ruletypes.h#L565 |
| More procs | `Combat:AvgProcsPerMinute` | 2.0 | ruletypes.h#L578 |
| More flurry hits | `Combat:MaxFlurryHits` | 2 | ruletypes.h#L590 |
| Better frenzy | `Combat:FrenzyBaseDamage` | 10 | ruletypes.h#L681 |
| Better flying kick | `Combat:FlyingKickBaseDamage` | 25 | ruletypes.h#L680 |
| Faster attacks | `Combat:MinHastedDelay` | 400 | ruletypes.h#L591 |
| DEX scaling | `Combat:UseNewDexFormulas` | true | ruletypes.h#L638 |
| AGI scaling | `Combat:UseNewAgiFormulas` | true | ruletypes.h#L639 |

**Key Files:**
- Rules definitions: `common/ruletypes.h` lines 556-690
- Special attack formulas: `zone/special_attacks.cpp` lines 33-300
- Critical hit logic: `zone/attack.cpp` line 5654 (`TryCriticalHit`)
- Mitigation: `zone/attack.cpp` line 1163 (`MeleeMitigation`)
- Damage tables: `zone/attack.cpp` line 6265 (`ApplyDamageTable`)
