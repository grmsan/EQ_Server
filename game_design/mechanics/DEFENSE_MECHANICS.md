# Defense and Mitigation Mechanics

How defensive stats work: AC, Avoidance, Mitigation, and Damage Reduction.

## Table of Contents

1. [Defense Overview](#defense-overview)
2. [Armor Class (AC)](#armor-class-ac)
3. [Avoidance](#avoidance)
4. [Mitigation](#mitigation)
5. [Damage Shields](#damage-shields)
6. [Defensive Abilities](#defensive-abilities)

---

## Defense Overview

### The Defense Pipeline

```
Incoming Attack
      ↓
┌─────────────────────────────────────────┐
│ 1. AVOIDANCE CHECK                      │
│    Miss → Dodge → Parry → Block → Riposte │
└─────────────────────────────────────────┘
      ↓ (if not avoided)
┌─────────────────────────────────────────┐
│ 2. HIT LANDS                            │
│    Base damage calculated               │
└─────────────────────────────────────────┘
      ↓
┌─────────────────────────────────────────┐
│ 3. MITIGATION                           │
│    AC reduces damage (0.1x to 2.0x)     │
└─────────────────────────────────────────┘
      ↓
┌─────────────────────────────────────────┐
│ 4. DAMAGE REDUCTION                     │
│    % reduction from shielding/runes     │
└─────────────────────────────────────────┘
      ↓
Final Damage Applied
      ↓
┌─────────────────────────────────────────┐
│ 5. DAMAGE SHIELDS                       │
│    Reflect damage back to attacker      │
└─────────────────────────────────────────┘
```

---

## Armor Class (AC)

### How AC Works

AC (Armor Class) serves two purposes:
1. **Avoidance** - Higher AC = higher chance to avoid hits
2. **Mitigation** - Higher AC = reduced damage when hit

### AC Sources

| Source | Typical Range | Notes |
|--------|---------------|-------|
| Base (class/level) | 50-200 | Warriors get most |
| Armor items | 100-1000+ | Chest/legs highest |
| Shield | 50-200 | Knights, classes with shield |
| AGI stat | 0-100 | ~1 AC per 3 AGI |
| DEF skill | 0-50 | Scales with skill level |
| Buffs | 0-500 | Shielding, protection spells |
| AAs | 0-200 | Combat stability, etc. |

### AC Calculation

```cpp
int Mob::GetAC() const
{
    int base_ac = GetACFromItems();
    int stat_ac = GetAGI() / 3;  // Agility contribution
    int skill_ac = GetSkill(EQ::skills::SkillDefense) / 3;
    int spell_ac = GetSpellAC();  // From buffs
    int aa_ac = GetAABonuses().AC;

    return base_ac + stat_ac + skill_ac + spell_ac + aa_ac;
}
```

### AC vs Level Difference

AC effectiveness changes based on attacker level:
```
Attacker 10 levels below: AC very effective
Attacker same level: AC normal effectiveness
Attacker 10 levels above: AC reduced effectiveness
```

---

## Avoidance

### Avoidance Types

| Type | Skill | Classes | Effect |
|------|-------|---------|--------|
| Dodge | Defense | All | Avoid hit completely |
| Parry | Parry | Melee | Avoid with weapon |
| Block | Block | Shield users | Avoid with shield |
| Riposte | Riposte | Melee | Avoid and counter-attack |

### Avoidance Order

Checks happen in this order:
1. **Miss** - Attacker misses (accuracy vs avoidance)
2. **Dodge** - Defender dodges
3. **Parry** - Defender parries (if has weapon)
4. **Block** - Defender blocks (if has shield)
5. **Riposte** - Defender ripostes (if skilled)

### Avoidance Formulas

```cpp
// Dodge chance
int dodge_chance = GetSkill(SkillDodge) / 4 + GetAGI() / 25;
// Level 65 with 300 dodge, 400 AGI: 75 + 16 = 91 (softcapped)

// Parry chance (requires weapon equipped)
int parry_chance = GetSkill(SkillParry) / 4;
// Level 65 with 300 parry: 75

// Block chance (requires shield equipped)
int block_chance = GetSkill(SkillBlock) / 4;
// Level 65 with 300 block: 75

// Riposte chance (requires weapon)
int riposte_chance = GetSkill(SkillRiposte) / 4;
// Level 65 with 300 riposte: 75
```

### Soft Caps

Each avoidance type has diminishing returns after certain thresholds:

| Avoidance | Soft Cap | Hard Cap |
|-----------|----------|----------|
| Dodge | ~30% | 50% |
| Parry | ~25% | 45% |
| Block | ~25% | 45% |
| Riposte | ~20% | 35% |

---

## Mitigation

### Mitigation Roll

When a hit lands, mitigation determines how much of the base damage applies:

```cpp
// Mitigation range: 0.1 (10% of base) to 2.0 (200% of base)
// Higher defender AC = lower average roll
// Higher attacker ATK = higher average roll

float GetMitigationMultiplier(int attacker_atk, int defender_ac)
{
    float diff = attacker_atk - defender_ac;

    // Base roll
    float roll = RandomFloat(0.1f, 2.0f);

    // Adjust based on ATK vs AC
    // Positive diff = attacker advantage = higher roll
    // Negative diff = defender advantage = lower roll

    return roll;
}
```

### Mitigation Examples

| ATK vs AC | Average Mitigation | Damage Range |
|-----------|-------------------|--------------|
| ATK >> AC | ~1.5x | 100-200% |
| ATK = AC | ~1.0x | 10-200% |
| ATK << AC | ~0.5x | 10-100% |

### AC Soft Cap

AC has diminishing returns at high values:

```cpp
// Example soft cap implementation
int GetEffectiveAC(int raw_ac)
{
    int soft_cap = 350;  // Level-dependent

    if (raw_ac <= soft_cap) {
        return raw_ac;
    }

    // Above soft cap: reduced benefit
    int over = raw_ac - soft_cap;
    return soft_cap + (over / 3);  // 33% effectiveness over cap
}
```

---

## Damage Shields

### How Damage Shields Work

Damage shields reflect damage back to melee attackers:

```cpp
void Mob::ApplyDamageShield(Mob* attacker, int damage_taken)
{
    int ds_value = GetDS();  // Total damage shield value

    if (ds_value > 0) {
        // Reflect damage back
        attacker->Damage(this, ds_value, 0, SkillHand2Hand);
    }
}
```

### Damage Shield Sources

| Source | Typical Value | Notes |
|--------|---------------|-------|
| Druid/SK buffs | 10-50 | Shield of Thorns, etc. |
| Item procs | 5-20 | Thorny items |
| AAs | 5-15 | Innate damage shield |

### Reverse Damage Shield

Some mobs have reverse damage shields that hurt the attacker more than normal:

```cpp
// Reverse DS: damages attacker when they attack
// Used for special mobs, hazards
int reverse_ds = mob->GetSpellBonuses().RevDS;
if (reverse_ds > 0) {
    attacker->Damage(mob, reverse_ds, 0, SkillHand2Hand);
}
```

---

## Defensive Abilities

### Disciplines

| Discipline | Class | Effect |
|------------|-------|--------|
| Defensive | Warrior | Large AC boost |
| Evasive | Rogue | High avoidance boost |
| Stonestance | Monk | AC and resist boost |
| Furious | Warrior | Defensive with counter |

### AAs

| AA | Effect |
|----|--------|
| Combat Stability | Passive AC increase |
| Physical Enhancement | Passive HP increase |
| Lightning Reflexes | Dodge chance increase |
| Combat Agility | Avoidance increase |

### Defensive Buffs

| Spell Type | Effect |
|------------|--------|
| Shielding | AC increase |
| Rune | Absorbs X damage |
| Stoneskin | Blocks X hits |
| Mirror | Reflects spells |

---

## Tuning Defensive Stats

### For High-Stat Server

With stats scaling to 1000+:

```cpp
// Adjust AGI contribution
int stat_ac = GetAGI() / 5;  // Reduced from /3

// Adjust soft cap
int soft_cap = 350 + (GetLevel() * 10);  // Scale with level

// Adjust mitigation bounds
float min_mit = 0.2f;  // Raise floor
float max_mit = 1.5f;  // Lower ceiling
```

### Balance Considerations

```
Too Tanky:
- Fights last too long
- Healers feel useless
- Content becomes trivial

Too Squishy:
- Players die constantly
- Healing can't keep up
- Content feels impossible

Target Balance:
- Tank can survive 30-60 seconds without heals vs on-level content
- Heals make survival indefinite
- Named mobs require active tanking/healing
```
