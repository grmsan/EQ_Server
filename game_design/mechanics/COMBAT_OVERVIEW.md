# Combat System Overview

Complete reference for EverQuest combat flow and systems.

## Table of Contents

1. [Combat Flow](#combat-flow)
2. [Attack Types](#attack-types)
3. [Damage Types](#damage-types)
4. [Combat Resolution](#combat-resolution)
5. [Aggro and Hate](#aggro-and-hate)
6. [Combat States](#combat-states)

---

## Combat Flow

### Complete Attack Resolution

```
┌─────────────────────────────────────────────────────────────────┐
│                      ATTACK INITIATED                            │
│  Mob::Attack() called with target and attack parameters         │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    PRE-ATTACK CHECKS                             │
│  - Is attacker alive?                                           │
│  - Is target valid?                                             │
│  - Is target attackable? (faction, PvP rules)                   │
│  - Is attacker in range?                                        │
│  - Is attacker facing target? (some attacks)                    │
│  - Is attack timer ready?                                       │
└─────────────────────────────────────────────────────────────────┘
                              │ Pass
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                     HIT DETERMINATION                            │
│  TryHit() calculates:                                           │
│  - Attacker accuracy (ATK, skills, buffs)                       │
│  - Defender avoidance (AC, AGI, skills)                         │
│  - Roll hit/miss                                                │
│                                                                  │
│  If HIT proceeds to avoidance checks...                         │
└─────────────────────────────────────────────────────────────────┘
                              │
              ┌───────────────┼───────────────┐
              │ MISS          │               │ HIT
              ▼               │               ▼
        Return: Miss          │    ┌──────────────────────┐
        Message sent          │    │  AVOIDANCE CHECKS    │
                              │    │  - Dodge             │
                              │    │  - Parry             │
                              │    │  - Block             │
                              │    │  - Riposte           │
                              │    └──────────────────────┘
                              │               │
                              │    ┌──────────┴──────────┐
                              │    │ AVOIDED             │ LANDS
                              │    ▼                     ▼
                              │  Return: Type      Continue...
                              │  (Dodge/Parry/etc)
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    BASE DAMAGE CALCULATION                       │
│  GetBaseDamage() based on attack type:                          │
│  - Melee: Weapon damage                                         │
│  - Special: Formula-based (see SPECIAL_ATTACKS.md)              │
│  - Spell: Spell base damage                                     │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                       MITIGATION                                 │
│  ApplyMitigation() calculates:                                  │
│  - Attacker ATK vs Defender AC                                  │
│  - Mitigation roll (0.1 to 2.0)                                 │
│  - Apply damage reduction                                        │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                     DAMAGE TABLES                                │
│  ApplyDamageTable() applies:                                    │
│  - Level-based multiplier (~1.0x to ~3.0x)                      │
│  - Skill-specific adjustments                                   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    CRITICAL HITS                                 │
│  TryCriticalHit() checks:                                       │
│  - Base crit chance + AA bonuses                                │
│  - If crit: apply multiplier (usually 2.0x)                     │
│  - Special crits: Crippling Blow, Deadly Strike                 │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    BONUS DAMAGE                                  │
│  Apply additional damage:                                       │
│  - SPA 170 (SkillDamageAmount) from gear/AAs                   │
│  - STR bonus damage                                             │
│  - Delay damage bonus (2H weapons)                              │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    MINIMUM DAMAGE FLOOR                          │
│  Ensure damage >= minimum:                                      │
│  - Usually 1 for most attacks                                   │
│  - Some skills have higher floors                               │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                   DAMAGE APPLICATION                             │
│  Mob::Damage() applies:                                         │
│  - Final damage to target HP                                    │
│  - Generate hate/aggro                                          │
│  - Trigger on-damage effects                                    │
│  - Death check                                                  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                   POST-DAMAGE EFFECTS                            │
│  - Damage shields (reflect damage to attacker)                  │
│  - Proc checks (weapon procs)                                   │
│  - Lifetap effects                                              │
│  - Death processing if HP <= 0                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Attack Types

### Melee Auto-Attack

Regular weapon swings, processed automatically:

| Property | Description |
|----------|-------------|
| Source | `Mob::Attack()` |
| Timer | Weapon delay (adjusted by haste) |
| Damage | Weapon damage |
| Skill | 1H/2H Blunt/Slash/Pierce |

### Special Attacks

Activated abilities with specific formulas:

| Attack | Class | Function |
|--------|-------|----------|
| Backstab | Rogue | `DoBackstab()` |
| Kick | Many | `DoKickAttack()` |
| Bash | Tank | `DoBash()` |
| Frenzy | Berserker | `DoFrenzy()` |
| Flying Kick | Monk | `DoKickAttack()` |
| Tiger Claw | Monk | Special monk attack |

### Ranged Attacks

Bows, throwing weapons:

| Property | Description |
|----------|-------------|
| Source | `RangedAttack()` |
| Range | Must be at range |
| Ammo | Requires ammunition |
| Skill | Archery or Throwing |

### Spells (Damage)

Direct damage spells:

| Property | Description |
|----------|-------------|
| Source | `SpellEffect()` |
| Resist | Can be resisted |
| Focus | Modified by focus effects |
| Crit | Spell crits separate from melee |

---

## Damage Types

### Physical Damage Types

```cpp
enum DamageType {
    DMG_1H_Blunt = 0,
    DMG_1H_Slash = 1,
    DMG_2H_Blunt = 2,
    DMG_2H_Slash = 3,
    DMG_Archery = 7,
    DMG_Backstab = 8,
    DMG_Bash = 10,
    DMG_Dragon_Punch = 26,
    DMG_Eagle_Strike = 28,
    DMG_Flying_Kick = 30,
    DMG_Kick = 36,
    DMG_Round_Kick = 38,
    DMG_Tiger_Claw = 52,
    DMG_Frenzy = 74,
};
```

### Spell Damage Types

```cpp
enum ResistType {
    RESIST_NONE = 0,
    RESIST_MAGIC = 1,
    RESIST_FIRE = 2,
    RESIST_COLD = 3,
    RESIST_POISON = 4,
    RESIST_DISEASE = 5,
    RESIST_CHROMATIC = 6,  // Lowest resist
    RESIST_PRISMATIC = 7,  // Average resist
    RESIST_PHYSICAL = 8,
    RESIST_CORRUPTION = 9,
};
```

### Damage Reduction by Type

```cpp
// Items/spells can reduce specific damage types
int damage_reduction = GetDamageReduction(damage_type);
final_damage -= damage_reduction;
if (final_damage < 0) final_damage = 0;
```

---

## Combat Resolution

### Hit Chance Calculation

```cpp
bool TryHit(Mob* defender, SkillType skill)
{
    // Attacker accuracy
    int accuracy = GetATK();
    accuracy += GetSkill(skill) / 5;
    accuracy += GetDEX() / 10;
    accuracy += GetHitChanceBonuses();

    // Defender avoidance
    int avoidance = defender->GetAC() / 4;
    avoidance += defender->GetAGI() / 10;
    avoidance += defender->GetAvoidanceBonuses();

    // Calculate chance
    int hit_chance = 50 + (accuracy - avoidance) / 5;
    hit_chance = std::clamp(hit_chance, 5, 95);  // 5-95% bounds

    return zone->random.Roll(hit_chance);
}
```

### Level Difference Effects

```cpp
// Attacking higher level mobs is harder
int level_diff = defender->GetLevel() - GetLevel();

if (level_diff > 0) {
    // Penalty to hit chance
    hit_chance -= level_diff * 2;

    // Penalty to damage (mitigation harder)
    mitigation_penalty = level_diff * 0.05;  // 5% per level
}
```

---

## Aggro and Hate

### Hate Generation

Every action generates hate (aggro):

| Action | Base Hate |
|--------|-----------|
| Melee hit | Damage * 1.0 |
| Spell damage | Damage * 0.5-1.0 |
| Heal | Amount * 0.5 |
| Buff | Varies by buff |
| Taunt | Fixed amount + level bonus |

### Hate List

Each NPC maintains a hate list:

```cpp
struct HateEntry {
    Mob* entity;
    int64 hate_amount;
    bool is_feigned;
};

// NPC targets highest hate that isn't feigned
Mob* GetTopHateTarget()
{
    // Sort by hate, filter feigned, return top
}
```

### Aggro Management

```cpp
// Taunt: attempt to become top hate
void Client::Taunt(NPC* target)
{
    int taunt_skill = GetSkill(SkillTaunt);
    int success_chance = taunt_skill / 3;

    if (zone->random.Roll(success_chance)) {
        // Move to top of hate list + buffer
        target->SetTopHate(this, current_top + 100);
    }
}

// Feign Death: reduce hate position
void Client::FeignDeath()
{
    // Chance to clear from hate lists
    // NPCs may see through based on level diff
}
```

---

## Combat States

### Mob States Affecting Combat

| State | Effect |
|-------|--------|
| Normal | Standard combat |
| Stunned | Cannot attack or cast |
| Mezzed | Cannot act, breaks on damage |
| Feared | Runs, cannot attack |
| Rooted | Cannot move, can attack |
| Charmed | Attacks former allies |
| Blinded | Accuracy penalty |
| Silenced | Cannot cast spells |

### State Checks in Combat

```cpp
bool CanAttack()
{
    if (IsStunned()) return false;
    if (IsMezzed()) return false;
    if (IsFeared()) return false;
    if (IsCharmed() && attacking_master) return false;
    return true;
}

bool CanCast()
{
    if (IsStunned()) return false;
    if (IsMezzed()) return false;
    if (IsSilenced()) return false;
    return true;
}
```

### Combat Engagement

```cpp
// Enter combat
void EnterCombat(Mob* target)
{
    SetTarget(target);
    SetAttackTimer();
    engaged = true;

    // NPCs add to hate list
    if (IsNPC()) {
        AddToHateList(target, initial_hate);
    }
}

// Exit combat
void ExitCombat()
{
    SetTarget(nullptr);
    engaged = false;

    // NPCs clear hate list and return home
    if (IsNPC()) {
        ClearHateList();
        ReturnToSpawnPoint();
    }
}
```

---

## Key Code Locations

| System | Primary File | Key Functions |
|--------|--------------|---------------|
| Attack | `zone/attack.cpp` | `Attack()`, `DoMeleeDamage()` |
| Special | `zone/special_attacks.cpp` | `DoBackstab()`, `DoFrenzy()` |
| Spells | `zone/spell_effects.cpp` | `SpellEffect()` |
| Aggro | `zone/aggro.cpp` | `AddToHateList()`, `GetTopHate()` |
| Mob | `zone/mob.cpp` | `Damage()`, `GetAC()`, `GetATK()` |
| Client | `zone/client.cpp` | `Attack()`, combat packets |
