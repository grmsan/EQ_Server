# Combat Mechanics Documentation

This folder contains documentation for EverQuest combat systems and game mechanics.

**For Junior Developers:** Start with the overview in this file, then dive into specific topics.

## Quick Links

| Document | Purpose | Read This When... |
|----------|---------|-------------------|
| [COMBAT_OVERVIEW.md](COMBAT_OVERVIEW.md) | Complete combat flow and damage pipeline | Understanding how damage works |
| [SPECIAL_ATTACKS.md](SPECIAL_ATTACKS.md) | Special attack mechanics (existing) | Working on Frenzy, Backstab, etc. |
| [SPECIAL_ATTACKS_FORMULAS.md](SPECIAL_ATTACKS_FORMULAS.md) | Updated formulas for custom server | Implementing new special attack scaling |
| [MELEE_FORMULAS.md](MELEE_FORMULAS.md) | Auto-attack and melee calculations | Tuning melee damage |
| [SPELL_MECHANICS.md](SPELL_MECHANICS.md) | Spell damage, resists, focus effects | Working on spell systems |
| [DEFENSE_MECHANICS.md](DEFENSE_MECHANICS.md) | AC, mitigation, avoidance | Understanding defensive stats |
| [API_REFERENCE.md](API_REFERENCE.md) | C++ functions and Lua bindings | Looking up combat functions |
| [TESTING_GUIDE.md](TESTING_GUIDE.md) | Testing combat changes | Verifying your changes work |

## Combat System Overview

### The Damage Pipeline

```
Attack Initiated
       ↓
┌─────────────────────────────────────────────────────────────┐
│ 1. HIT DETERMINATION                                        │
│    - Accuracy vs Avoidance                                  │
│    - Miss, Dodge, Parry, Block, Riposte checks             │
└─────────────────────────────────────────────────────────────┘
       ↓ (if hit lands)
┌─────────────────────────────────────────────────────────────┐
│ 2. BASE DAMAGE CALCULATION                                  │
│    - Weapon damage (melee) or spell base (magic)           │
│    - Skill modifiers                                        │
│    - Special attack formulas                                │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│ 3. DAMAGE MODIFICATION                                      │
│    - Mitigation roll (attack vs AC)                        │
│    - Damage tables (level-based multipliers)               │
│    - Minimum damage floors                                  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│ 4. CRITICAL HITS                                            │
│    - Crit chance calculation                                │
│    - Crit multiplier (base + AAs)                          │
│    - Crippling Blow / Deadly Strike                        │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│ 5. BONUS DAMAGE                                             │
│    - SPA 170 (SkillDamageAmount) from gear/AAs             │
│    - Damage shields on defender                             │
│    - Proc effects                                           │
└─────────────────────────────────────────────────────────────┘
       ↓
Final Damage Applied
```

### Key Files in Codebase

| File | Contains |
|------|----------|
| `zone/attack.cpp` | Main attack processing, hit determination |
| `zone/special_attacks.cpp` | Kick, Bash, Backstab, Frenzy, etc. |
| `zone/mob.cpp` | Mob stats, damage calculations |
| `zone/client.cpp` | Client-specific combat handling |
| `zone/spell_effects.cpp` | Spell damage and effects |
| `zone/aggro.cpp` | Hate/aggro calculations |
| `common/skills.h` | Skill IDs and definitions |

### Key Combat Rules (from common/ruletypes.h)

```cpp
// Critical Hit System (lines 562-565)
RULE_INT(Combat, MeleeCritDifficulty, 8900)   // Lower = easier crits
RULE_INT(Combat, ArcheryCritDifficulty, 3400) // Ranged crit difficulty
RULE_BOOL(Combat, NPCCanCrit, false)          // Enable NPC crits
RULE_INT(Combat, PetBaseCritChance, 0)        // Pet crit chance

// Stat Formula Toggles (lines 636-639)
RULE_BOOL(Combat, UseNewDexFormulas, true)    // DEX affects crits
RULE_BOOL(Combat, UseNewAgiFormulas, true)    // AGI affects avoidance
RULE_BOOL(Combat, UseNewStrDamageFormula, false) // STR affects damage

// Special Attack Base Damage (lines 675-686)
RULE_INT(Combat, FrenzyBaseDamage, 10)        // Frenzy starting damage
RULE_INT(Combat, FlyingKickBaseDamage, 25)    // Flying Kick base
RULE_INT(Combat, KickBaseDamage, 3)           // Kick base
RULE_INT(Combat, BackstabBaseDamage, 0)       // Backstab base
// ... many more
```

## Design Philosophy (Custom Server)

This server uses modified formulas to support:

1. **High-Stat Scaling** - Stats scale into thousands, not hundreds
2. **Weapon-Based Damage** - All attacks scale with weapon damage
3. **Solo Viability** - Single players can tackle challenging content
4. **Meaningful Progression** - Better gear = noticeably more powerful

### Key Modifications

| System | Stock EQEmu | Our Server |
|--------|-------------|------------|
| Frenzy Base | Level-15, capped at 23 | Weapon Damage + Level/10 |
| Monk Kicks | Skill/9 + BootAC/25 | WeaponDmg + Skill/9 + BootAC/25 |
| Stat Caps | ~400 | ~2000+ |
| Damage Tables | Stock multipliers | Tuned for high stats |

## Quick Reference

### Skill IDs (from skills.h)

| ID | Skill | Notes |
|----|-------|-------|
| 0 | 1H Blunt | |
| 1 | 1H Slashing | |
| 2 | 2H Blunt | |
| 3 | 2H Slashing | |
| 4 | Abjuration | |
| 5 | Alteration | |
| 6 | Apply Poison | |
| 7 | Archery | |
| 8 | Backstab | Rogue special |
| 10 | Bash | Tank special |
| 26 | Dragon Punch | Monk |
| 28 | Eagle Strike | Monk |
| 30 | Flying Kick | Monk |
| 36 | Kick | Multiple classes |
| 74 | Frenzy | Berserker |
| 38 | Round Kick | Monk |
| 52 | Tiger Claw | Monk |

### Damage Bonus SPA Types

| SPA | Effect | Used By |
|-----|--------|---------|
| 170 | SkillDamageAmount | Flat damage bonus to skills |
| 185 | SpellDamageShield | Damage shield |
| 220 | SkillDamageTaken | Reduce damage taken from skill |
| 330 | SkillMinDamage | Set minimum damage for skill |

## Related Documentation

- [Stats System](../stats/README.md) - How stats affect combat
- [Classes](../classes/README.md) - Class-specific combat abilities
- [Infinite Progression](../infinite_progression/README.md) - Scaling at high levels
