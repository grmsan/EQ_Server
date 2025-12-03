# EQEmulator Abilities Development Guide

This folder contains comprehensive documentation for creating, modifying, and implementing new spells, disciplines, and alternate advancement (AA) abilities in the EQEmulator server.

## Documentation Structure

### 1. **SPELLS_GUIDE.md**
Complete guide to creating and modifying spells:
- Database structure and spell tables
- Spell effects (SPA) reference
- Implementing new spell effects in C++
- Client/server synchronization
- Testing and debugging

### 2. **DISCIPLINES_GUIDE.md**
Disciplines (combat abilities) implementation:
- How disciplines differ from spells
- Creating discipline tomes
- Endurance costs and recast timers
- Warrior/Rogue/Monk/Berserker specific mechanics

### 3. **AA_GUIDE.md**
Alternate Advancement system:
- AA database structure (abilities, ranks, effects, prerequisites)
- Creating passive vs. active AAs
- Rank progression and costs
- AA-specific spell effects
- Integration with character progression

### 4. **SPELL_EFFECTS_REFERENCE.md**
Detailed reference of all ~500 Spell Effects (SPAs):
- Effect descriptions and parameters
- Implementation status
- Code locations
- Examples from live spells

### 5. **IMPLEMENTATION_CHECKLIST.md**
Step-by-step guide for developers:
- Database changes required
- C++ code modifications
- Packet handling
- Testing procedures
- Common pitfalls

## Quick Start for Developers

### Adding a New Spell
1. Insert into `spells_new` table with effects
2. Test with `/cast` command in-game
3. If custom behavior needed, modify `zone/spell_effects.cpp`
4. Create scrolls/tomes in `items` table

### Adding a New Discipline
1. Create spell entry (set `IsDiscipline=1`)
2. Create tome item (ItemType 20)
3. Optionally: Create custom endurance costs in `zone/effects.cpp::UseDiscipline()`

### Adding a New AA
1. Insert into `aa_ability` table (base ability)
2. Insert ranks into `aa_ranks` table
3. Insert effects into `aa_rank_effects` table
4. Add prerequisites to `aa_rank_prereqs` table
5. If custom logic needed, modify `zone/aa.cpp` and `zone/bonuses.cpp::ApplyAABonuses()`

## Key Concepts

- **Spells** = Database-driven with ~500 effects, mostly handled automatically
- **Disciplines** = Special spells that cost endurance, tied to combat classes
- **AAs** = Purchasable permanent upgrades, can be passive or active
- **Spell Effects (SPAs)** = Atomic effects that make up spells/disciplines/AAs
- **Focus Effects** = Spell effects that modify other spells (limits + modifications)

## Architecture Overview

```
Database (spells_new, aa_ability, aa_ranks, etc.)
    ↓
Shared Memory (loaded by shared_memory process)
    ↓
Zone Server (applies effects via SpellEffect(), ApplyAABonuses())
    ↓
Client (receives packets with spell data, renders effects)
```

## Important Files

### Database Tables
- `spells_new` - All spell data (~236 columns)
- `aa_ability` - Base AA definitions
- `aa_ranks` - Individual AA ranks
- `aa_rank_effects` - Effects for each rank
- `aa_rank_prereqs` - AA unlock requirements
- `items` - Spell scrolls, discipline tomes

### C++ Code
- `common/spdat.h` - Spell constants and SpellEffect namespace
- `zone/spell_effects.cpp` - Spell effect implementations (~10k lines)
- `zone/spells.cpp` - Spell casting logic
- `zone/effects.cpp` - Discipline handling
- `zone/aa.cpp` - AA activation and management
- `zone/bonuses.cpp` - Applies bonuses from spells/items/AAs

### Repositories
- `common/repositories/spells_new_repository.h`
- `common/repositories/aa_ability_repository.h`
- `common/repositories/aa_ranks_repository.h`
- `common/repositories/aa_rank_effects_repository.h`

## Development Workflow

1. **Design Phase**: Define what the ability should do
2. **Database Phase**: Create entries in appropriate tables
3. **Testing Phase**: Test in-game with GM commands
4. **Implementation Phase**: Add C++ code if custom behavior needed
5. **Integration Phase**: Ensure it works with existing systems
6. **Documentation Phase**: Update these guides with your changes

## GM Commands for Testing

```
/cast [spell_id]                    # Cast spell on target
/castspell [spell_id]               # Cast spell on yourself
/discipline [spell_id]              # Use discipline
/grantaa [aa_id] [points]          # Grant AA ability
/setaaxp [aa_points]               # Set AA points
/memspell [spell_id] [slot]        # Memorize spell
```

## Common Use Cases

### Damage Over Time Spell
1. Use `SpellEffect::CurrentHP` (0) with negative value
2. Set buff duration in ticks
3. Set resist type and target type

### Passive Stat Bonus AA
1. Create AA with stat effect (e.g., `SpellEffect::STR`)
2. Set `spell = -1` in rank (passive)
3. Effect applied via `ApplyAABonuses()`

### Active Click AA
1. Create AA with spell_id in rank
2. Set recast_time and spell_type
3. Triggered via `ActivateAlternateAdvancementAbility()`

### Proc on Hit
1. Use `SpellEffect::WeaponProc` or `SpellEffect::AddMeleeProc`
2. Set base1 = proc spell ID
3. Set base2 = proc rate

## Next Steps

Read the detailed guides in this folder based on what you want to create:
- Creating spells → **SPELLS_GUIDE.md**
- Creating disciplines → **DISCIPLINES_GUIDE.md**
- Creating AAs → **AA_GUIDE.md**
- Need effect reference → **SPELL_EFFECTS_REFERENCE.md**
- Ready to implement → **IMPLEMENTATION_CHECKLIST.md**
