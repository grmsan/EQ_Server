# Implementation Checklist

## Quick Reference for Developers

This checklist provides step-by-step instructions for implementing spells, disciplines, and AAs from concept to deployment.

---

## Spell Implementation

### ☑ Phase 1: Design
- [ ] Define spell purpose and mechanics
- [ ] Choose target type (single, AE, group, self)
- [ ] Determine cost (mana, reagents)
- [ ] Set duration (instant, buff, DoT)
- [ ] List all effects needed
- [ ] Check for existing spell effects that match

### ☑ Phase 2: Database Entry
- [ ] Choose unused spell ID `< 65535` (client-safe); prefer checking `SELECT MAX(id) FROM spells_new`
- [ ] Create `spells_new` entry with all fields
- [ ] Set `classes1-16` to appropriate levels
- [ ] Configure `effectid1-12` with spell effects
- [ ] Set `effect_base_value1-12` for magnitudes
- [ ] Configure formulas for scaling (if needed)
- [ ] Set `targettype`, `range`, `aoerange`
- [ ] Configure `cast_time`, `recast_time`, `recovery_time`
- [ ] Set `mana` cost
- [ ] Set `buffduration` and `buffdurationformula`
- [ ] Set resist type in `resisttype`
- [ ] Set string IDs to non-NULL (use 0 if you don’t have custom strings): `descnum`, `typedescnum`, `effectdescnum`, `effectdescnum2`

### ☑ Phase 3: Testing
- [ ] Reload shared memory: `shared_memory.exe` (or Server Manager "Shared Memory")
- [ ] Test cast: `#castspell [spell_id]`
- [ ] Verify mana cost correct
- [ ] Verify cast time feels right
- [ ] Verify effects apply correctly
- [ ] Test on different level characters
- [ ] Test resist mechanics
- [ ] Test spell stacking with similar buffs
- [ ] Verify duration is correct

### ☑ Phase 4: Scroll Creation (Optional)
- [ ] Choose unused item ID
- [ ] Create `items` entry
- [ ] Set `itemtype = 11` (spell scroll)
- [ ] Set `scrolleffect = [spell_id]`
- [ ] Set `classes` bitmask for who can scribe
- [ ] Set `reqlevel` for minimum level
- [ ] Test: `#summonitem [item_id]`
- [ ] Right-click scroll to scribe
- [ ] Verify spell in spell book

### ☑ Phase 5: Custom Code (If Needed)
- [ ] Identify which effect needs custom logic
- [ ] Add effect constant to `common/spdat.h`
- [ ] Implement in `zone/spell_effects.cpp::SpellEffect()`
- [ ] Add bonus field to `StatBonuses` if persistent
- [ ] Apply bonus in `zone/bonuses.cpp::ApplySpellsBonuses()`
- [ ] Rebuild server: `cmake --build build`
- [ ] Test custom logic works
- [ ] Check for memory leaks or crashes
- [ ] Add debug logging for troubleshooting

---

## Discipline Implementation

### ☑ Phase 1: Design
- [ ] Define discipline purpose
- [ ] Choose endurance cost
- [ ] Determine duration
- [ ] Select recast timer group
- [ ] List all effects needed
- [ ] Identify which classes can use it

### ☑ Phase 2: Spell Entry
- [ ] Choose unused spell ID
- [ ] Create `spells_new` entry
- [ ] **Set `IsDiscipline = 1`** ← Critical!
- [ ] Set `EndurCost` instead of `mana`
- [ ] Set `EndurTimerIndex` (1-20)
- [ ] Set `recast_time` in milliseconds
- [ ] Configure `classes1-16` (melee classes)
- [ ] Add spell effects as normal
- [ ] Set `cast_time = 0` (instant)
- [ ] Set `targettype = 6` (self, usually)

### ☑ Phase 3: Tome Creation
- [ ] Choose unused item ID
- [ ] Create `items` entry
- [ ] **Set `itemtype = 20`** ← Critical!
- [ ] **Set `scrolltype = 2`** ← Critical!
- [ ] Set `scrolleffect = [discipline_spell_id]`
- [ ] Set `classes` to appropriate melee classes
- [ ] Set `reqlevel` for minimum level
- [ ] Test: `#summonitem [item_id]`
- [ ] Right-click tome to learn

### ☑ Phase 4: Testing
- [ ] Reload shared memory
- [ ] Summon tome and learn discipline
- [ ] Check endurance cost is correct
- [ ] Use discipline from combat abilities window (or `/disc [spell_id]` client command)
- [ ] Verify effects apply
- [ ] Test recast timer works
- [ ] Verify can't use without endurance
- [ ] Test with multiple timer groups

---

## AA Implementation

### ☑ Phase 1: Design
- [ ] Define AA purpose and progression
- [ ] Choose passive or active
- [ ] Determine number of ranks
- [ ] Plan cost per rank
- [ ] List effects per rank
- [ ] Identify prerequisites (if any)
- [ ] Choose appropriate classes

### ☑ Phase 2: Ability Entry
- [ ] Choose unused AA ability ID (query DB first; do not assume a fixed band)
- [ ] Create `aa_ability` entry
- [ ] Set `name` (display name)
- [ ] Set `category` (1-4)
- [ ] Set `classes` bitmask using AA mask rules from `AA_GUIDE.md` (`1 << class_id`, all classes `131070`)
- [ ] Set `type` (1=General, 2=Archetype, 3=Class)
- [ ] Set `charges` (0 for unlimited)
- [ ] Set `grant_only` (0 for purchasable)
- [ ] Set `first_rank_id` to first rank ID (must exist in `aa_ranks.id`)
- [ ] Set `enabled = 1`
- [ ] Set `reset_on_death` if applicable
- [ ] Leave `races` / `deities` / `status` defaults unless you are intentionally gating the AA

### ☑ Phase 3: Rank Entries

**For Each Rank**:
- [ ] Choose unique rank ID
- [ ] Create `aa_ranks` entry
- [ ] Set `upper_hotkey_sid` / `lower_hotkey_sid` (-1 for none)
- [ ] Set `title_sid` / `desc_sid` (db_str ids)
- [ ] Set `cost` (AA points)
- [ ] Set `level_req` (minimum level)
- [ ] Set `prev_id` (previous rank or -1)
- [ ] Set `next_id` (next rank or -1)
- [ ] Set `expansion` (0 for base)

**For Passive Ranks**:
- [ ] Set `spell = -1`
- [ ] Set `spell_type = 0`
- [ ] Set `recast_time = 0`

**For Active Ranks**:
- [ ] Create spell entry first
- [ ] Set `spell = [spell_id]`
- [ ] Set `spell_type` (timer group 1-20)
- [ ] Set `recast_time` (seconds)

### ☑ Phase 4: Effect Entries

**For Each Rank**:
- [ ] Create `aa_rank_effects` entries
- [ ] Set `rank_id` to rank ID
- [ ] Set `slot` (1-12)
- [ ] Set `effect_id` (SPA number)
- [ ] Set `base1` (effect value)
- [ ] Set `base2` (limit value, if needed)
- [ ] Verify effects are implemented in `ApplyAABonuses()`

### ☑ Phase 5: Prerequisites (Optional)
- [ ] Create `aa_rank_prereqs` entries
- [ ] Set `rank_id` (rank requiring prereq)
- [ ] Set `aa_id` (required ability)
- [ ] Set `points` (points needed)

### ☑ Phase 6: Testing
- [ ] Reload AA data: `#reload aa_data`
- [ ] Set test AA points: `#set aa_points aa [amount]`
- [ ] Open AA window and purchase rank
- [ ] Verify AA appears
- [ ] Purchase ranks (or grant them)
- [ ] Check effects apply: `#showstats`
- [ ] Test active AAs activate correctly
- [ ] Test recast timers work
- [ ] Verify prerequisites block correctly
- [ ] Test rank progression
- [ ] If title/desc/spell text changed: export `spells_us.txt` + `dbstr_us.txt` via Server Manager, then fully restart client

### ☑ Phase 7: Custom Code (If Needed)
- [ ] Add effect to `common/spdat.h` namespace
- [ ] Implement in `zone/bonuses.cpp::ApplyAABonuses()`
- [ ] Add bonus field to `StatBonuses` structure
- [ ] Rebuild server
- [ ] Test custom effect works
- [ ] Add logging for debugging

---

## Focus Effect Implementation

### ☑ Phase 1: Design
- [ ] Define what spells to modify
- [ ] Choose modification type (damage, healing, mana, etc.)
- [ ] Determine limits (level range, resist type, effect type)
- [ ] Decide magnitude of modification

### ☑ Phase 2: Focus Spell
- [ ] Create spell entry or AA rank
- [ ] Add **Limit SPAs** (effect slots 1-6):
  - [ ] LimitMaxLevel (134)
  - [ ] LimitMinLevel (142)
  - [ ] LimitResist (135)
  - [ ] LimitTarget (136)
  - [ ] LimitEffect (137)
  - [ ] LimitSpell (139)
  - [ ] LimitCastingSkill (414)
- [ ] Add **Modification SPAs** (remaining slots):
  - [ ] ImprovedDamage (124)
  - [ ] ImprovedHeal (125)
  - [ ] ReduceManaCost (132)
  - [ ] IncreaseSpellHaste (127)
  - [ ] FcDamageAmt (286)
- [ ] Set `buffduration = -1` for item focus

### ☑ Phase 3: Testing
- [ ] Buff yourself with focus effect
- [ ] Cast spells that should be affected
- [ ] Verify damage/healing increases
- [ ] Cast spells that shouldn't be affected
- [ ] Verify no modification
- [ ] Test focus stacking (multiple foci)
- [ ] Check mana cost reduction works

---

## Custom Spell Effect Implementation

### ☑ Phase 1: Design
- [ ] Define new effect clearly
- [ ] Check if existing SPAs can achieve it
- [ ] Document effect parameters (base1, base2)
- [ ] Plan integration with existing systems

### ☑ Phase 2: Code Changes

**Add Constant**:
- [ ] Open `common/spdat.h`
- [ ] Find `namespace SpellEffect`
- [ ] Add: `constexpr int MyEffect = [next_id];`
- [ ] Add comment describing effect

**Implement in SpellEffect()**:
- [ ] Open `zone/spell_effects.cpp`
- [ ] Find `Mob::SpellEffect()` function
- [ ] Locate massive switch statement
- [ ] Add new case:
```cpp
case SpellEffect::MyEffect: {
    int value = effect_value;
    int limit = spell.effect_limit_value[i];
    // Your logic here
    break;
}
```

**Add Bonus Field (if persistent)**:
- [ ] Open `zone/client.h` or `zone/mob.h`
- [ ] Find `struct StatBonuses`
- [ ] Add: `int MyEffectBonus;`

**Apply Bonus**:
- [ ] Open `zone/bonuses.cpp`
- [ ] Find `ApplySpellsBonuses()` or `ApplyAABonuses()`
- [ ] Add bonus application logic

### ☑ Phase 3: Testing
- [ ] Rebuild server: `cmake --build build`
- [ ] Create test spell with new effect
- [ ] Cast spell: `#castspell [spell_id]`
- [ ] Verify effect applies
- [ ] Check bonus field updates
- [ ] Test edge cases (0 value, negative, max)
- [ ] Test with different characters/levels
- [ ] Check for crashes or errors

### ☑ Phase 4: Documentation
- [ ] Document effect behavior in this folder's guides (`SPELLS_GUIDE.md` / `AA_GUIDE.md`)
- [ ] Add example spell usage
- [ ] Note any special behaviors

---

## Deployment Checklist

### ☑ Pre-Deployment
- [ ] Test all new abilities thoroughly
- [ ] Verify no server crashes
- [ ] Check database syntax is correct
- [ ] Back up current database
- [ ] Document all changes made

### ☑ Database Updates
- [ ] Export new spell entries:
  ```sql
  SELECT * FROM spells_new WHERE id >= 60000 INTO OUTFILE 'custom_spells.sql';
  ```
- [ ] Export new AA entries:
  ```sql
  SELECT * FROM aa_ability WHERE id >= 1000 INTO OUTFILE 'custom_aas.sql';
  ```
- [ ] Export new item entries:
  ```sql
  SELECT * FROM items WHERE id >= 100000 INTO OUTFILE 'custom_items.sql';
  ```
- [ ] Test import on staging server
- [ ] Sync client files used by this repo:
  - [ ] Export `spells_us.txt` from Server Manager
  - [ ] Export `dbstr_us.txt` from Server Manager
  - [ ] Verify "Client Asset Status" shows up to date

### ☑ Code Deployment
- [ ] Commit C++ changes to version control
- [ ] Tag release version
- [ ] Build release binaries
- [ ] Test release build
- [ ] Deploy to production server

### ☑ Server Updates
- [ ] Stop server: `python stop_server.py`
- [ ] Import database changes
- [ ] Replace server binaries
- [ ] Start server: `python start_server.py`
- [ ] Verify shared_memory loads
- [ ] Check for errors in logs

### ☑ Post-Deployment
- [ ] Test new abilities in-game
- [ ] Monitor for crashes
- [ ] Check player feedback
- [ ] Fix any bugs discovered
- [ ] Document known issues

---

## Troubleshooting Guide

### Spell Won't Cast
1. Check class requirements (`classes1-16`)
2. Verify mana cost vs. current mana
3. Check skill requirements
4. Verify targettype allows your target
5. Check spell row exists and is valid in `spells_new`

### Spell Effect Not Working
1. Verify effect ID is implemented
2. Check `effect_base_value` is non-zero
3. Review `zone/spell_effects.cpp` for effect
4. Enable `#define SPELL_EFFECT_SPAM`
5. Check server console for errors

### AA Doesn't Appear
1. Check `aa_ability.enabled = 1`
2. Verify `classes` bitmask includes your class
3. Check `level_req` isn't too high
4. Ensure `first_rank_id` is correct
5. Try `#reload aa_data`
6. Verify AA class mask is correct for this branch's AA rules

### AA Effects Not Applying
1. Verify effect in `zone/bonuses.cpp::ApplyAABonuses()`
2. Check `aa_rank_effects.rank_id` is correct
3. Ensure `effect_id` is valid SPA
4. Check `#showstats` for AA bonuses
5. Try removing and re-granting AA

### Discipline Won't Learn
1. Check `IsDiscipline = 1` in spell
2. Verify tome `itemtype = 20` and `scrolltype = 2`
3. Check class requirements
4. Verify level requirement
5. Check discipline slots (100 max)

### Focus Effect Not Working
1. Verify Limit SPAs are correct
2. Check spell meets all limits
3. Test without limits to isolate issue
4. Check focus stacking order
5. Enable spell logging

---

## Performance Considerations

### Database Queries
- [ ] Index frequently queried columns
- [ ] Avoid SELECT * when possible
- [ ] Use prepared statements

### Shared Memory
- [ ] Limit custom spells to reasonable amount
- [ ] Test shared_memory load time
- [ ] Monitor memory usage

### Code Efficiency
- [ ] Avoid expensive calculations in tight loops
- [ ] Cache frequently used values
- [ ] Use const references for large objects
- [ ] Profile code for bottlenecks

---

## Best Practices

### Naming Conventions
- **Spells**: Descriptive names ("Greater Heal", "Fireball")
- **Disciplines**: Action-oriented ("Defensive Strike", "Furious Rage")
- **AAs**: Benefit-focused ("Combat Fury", "Planar Power")

### ID Ranges
- **Spells**: keep `< 65535` (client limit)
- **Spells**: reserve/track custom bands in SQL docs; do not assume old global ranges
- **Items**: 100000-199999 (scrolls/tomes)
- **Items**: 200000-299999 (other custom items)
- **AAs**: use unused IDs from DB; many custom IDs are already in low and mid ranges

### Documentation
- Comment all custom code
- Document spell/AA effects clearly
- Keep changelogs updated
- Maintain design documents

### Testing
- Test with different classes
- Test at different levels
- Test edge cases
- Test stacking with existing abilities
- Test in group scenarios

---

## Summary

**For Most Abilities**:
1. Design → Database → Test → Deploy
2. No C++ code required

**For Custom Effects**:
1. Design → Database → Code → Build → Test → Deploy

**Always**:
- Back up before changes
- Test thoroughly
- Document everything
- Monitor for issues

---

**Reference Other Guides**:
- Spell details → **SPELLS_GUIDE.md**
- Discipline specifics → **DISCIPLINES_GUIDE.md**
- AA details → **AA_GUIDE.md**
- Effect IDs/code locations → `common/spdat.h` and `zone/spell_effects.cpp`
