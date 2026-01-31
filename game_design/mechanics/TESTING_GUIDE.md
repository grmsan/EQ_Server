# Combat Mechanics Testing Guide

How to test and verify combat system changes.

## Table of Contents

1. [Testing Environment Setup](#testing-environment-setup)
2. [GM Commands for Testing](#gm-commands-for-testing)
3. [Combat Logging](#combat-logging)
4. [Test Scenarios](#test-scenarios)
5. [Validation Checklists](#validation-checklists)
6. [Debugging Combat Issues](#debugging-combat-issues)

---

## Testing Environment Setup

### Create a Test Character

```sql
-- Create a level 65 test character (via GM commands or DB)
-- Use #level 65 in-game

-- Grant all skills max
-- #setskillall 300
```

### Create Test NPCs

```sql
-- Insert a test dummy NPC (high HP, no damage)
INSERT INTO npc_types (id, name, level, hp, mindmg, maxdmg, ac, class)
VALUES (999001, 'Combat_Test_Dummy', 65, 10000000, 0, 0, 100, 1);

-- Insert NPCs at various AC levels
INSERT INTO npc_types (id, name, level, hp, mindmg, maxdmg, ac, class)
VALUES
(999002, 'Test_AC_100', 65, 100000, 10, 20, 100, 1),
(999003, 'Test_AC_500', 65, 100000, 10, 20, 500, 1),
(999004, 'Test_AC_1000', 65, 100000, 10, 20, 1000, 1),
(999005, 'Test_AC_2000', 65, 100000, 10, 20, 2000, 1);

-- Spawn them
-- In-game: #spawn 999001
```

### Test Items

```sql
-- Create a test weapon with known damage
INSERT INTO items (id, name, itemtype, damage, delay, skillmodtype)
VALUES (999001, 'Test_Sword_10dmg', 0, 10, 20, 0);

INSERT INTO items (id, name, itemtype, damage, delay, skillmodtype)
VALUES (999002, 'Test_Sword_50dmg', 0, 50, 20, 0);

INSERT INTO items (id, name, itemtype, damage, delay, skillmodtype)
VALUES (999003, 'Test_Sword_100dmg', 0, 100, 20, 0);

-- Give to character: #summonitem 999001
```

---

## GM Commands for Testing

### Character Modification

| Command | Description | Example |
|---------|-------------|---------|
| `#level <n>` | Set character level | `#level 65` |
| `#setskill <id> <value>` | Set specific skill | `#setskill 8 300` (Backstab) |
| `#setskillall <value>` | Set all skills | `#setskillall 300` |
| `#setstat <stat> <value>` | Set base stat | `#setstat str 500` |
| `#setstats <value>` | Set all stats | `#setstats 400` |

### Target Modification

| Command | Description | Example |
|---------|-------------|---------|
| `#heal` | Fully heal target | `#heal` |
| `#damage <amount>` | Deal damage to target | `#damage 1000` |
| `#npcstats` | Show NPC stats | `#npcstats` |
| `#npcedit hp <value>` | Set NPC max HP | `#npcedit hp 1000000` |
| `#npcedit ac <value>` | Set NPC AC | `#npcedit ac 500` |

### Combat Commands

| Command | Description | Example |
|---------|-------------|---------|
| `#attack <on/off>` | Toggle auto-attack | `#attack on` |
| `#kill` | Kill target instantly | `#kill` |
| `#invulnerable` | Toggle invulnerability | `#invulnerable` |
| `#godmode` | Toggle god mode | `#godmode` |

### Debug Commands

| Command | Description |
|---------|-------------|
| `#showstats` | Display your combat stats |
| `#showbonuses` | Display item/spell bonuses |
| `#peqzone <zone>` | Teleport to test zone |

---

## Combat Logging

### Enable Detailed Combat Logs

In `eqemu_config.json`:
```json
{
  "server": {
    "logging": {
      "categories": {
        "Combat": {
          "enabled": true,
          "log_to_file": true,
          "log_to_console": true
        },
        "Attack": {
          "enabled": true,
          "log_to_file": true
        }
      }
    }
  }
}
```

### Log Categories

| Category | What It Logs |
|----------|--------------|
| `Logs::Combat` | General combat events |
| `Logs::Attack` | Attack rolls, hit/miss |
| `Logs::Skills` | Skill usage and values |
| `Logs::Spells` | Spell damage and effects |

### Adding Debug Output

In C++ code:
```cpp
// Temporary debug in GetBaseSkillDamage() (zone/special_attacks.cpp#L33)
Log(Logs::Detail, Logs::Combat,
    "GetBaseSkillDamage DEBUG: skill={} level={} weapon_damage={} base={}",
    skillinuse, GetLevel(), weapon_damage, base);
```

### Reading Combat Logs

```
logs/zone/Combat.log
logs/zone/Attack.log
```

Example output:
```
[Combat] GetBaseSkillDamage: skill=Backstab level=65 weapon_damage=50 base=450
[Combat] TryCriticalHit: rolled 8500 vs difficulty 8900 - NO CRIT
[Combat] DoMeleeSkillAttackDmg: attacker=TestPlayer target=Combat_Test_Dummy damage=892
```

---

## Test Scenarios

### Scenario 1: Backstab Damage Scaling

**Purpose:** Verify backstab scales with weapon damage

**Setup:**
1. Create Rogue level 65
2. Max backstab skill: `#setskill 8 300`
3. Spawn test dummy: `#spawn 999001`

**Test Steps:**
| Weapon | Expected Base | Test Command |
|--------|---------------|--------------|
| 10 dmg sword | ~80 | Equip, backstab, record |
| 50 dmg sword | ~400 | Equip, backstab, record |
| 100 dmg sword | ~800 | Equip, backstab, record |

**Formula:** `weapon_damage * (skill * 0.02 + 2.0)`
- 10 * (300 * 0.02 + 2) = 10 * 8 = 80
- 50 * 8 = 400
- 100 * 8 = 800

**Pass Criteria:** Damage increases linearly with weapon damage

---

### Scenario 2: Frenzy Damage (Stock vs Modified)

**Purpose:** Compare stock formula vs weapon-scaled formula

**Setup:**
1. Create Berserker level 65
2. Max frenzy skill: `#setskill 74 300`
3. Spawn test dummy

**Stock Formula Test:**
- Expected base: `level - 15 = 50`, capped at 23
- Final damage ~23 before multipliers

**Modified Formula Test:**
- Expected: `weapon_damage + (level/10)`
- With 50 dmg weapon: 50 + 6 = 56 base

---

### Scenario 3: Monk Kick Scaling

**Purpose:** Verify flying kick with boot AC

**Setup:**
1. Create Monk level 65
2. Max flying kick: `#setskill 30 300`

**Test Matrix:**

| Boot AC | Skill | Expected Base | Notes |
|---------|-------|---------------|-------|
| 0 | 300 | 33 | skill/9 = 33 |
| 50 | 300 | 35 | 33 + 50/25 = 35 |
| 100 | 300 | 37 | 33 + 4 = 37 |
| 250 | 300 | 43 | 33 + 10 = 43 |

---

### Scenario 4: Critical Hit Rate

**Purpose:** Verify crit chance and multiplier

**Setup:**
1. Character with known crit chance
2. Test dummy
3. Enable detailed logging

**Test:**
```
Attack 1000 times, count crits
Expected: (crit_chance%) ± 2%

Verify crit multiplier:
Non-crit damage: X
Crit damage: X * 2.0 (or configured mult)
```

---

### Scenario 5: AC Mitigation Curve

**Purpose:** Verify AC reduces damage appropriately

**Setup:**
1. Same attacker, same weapon
2. NPCs with varying AC (100, 500, 1000, 2000)

**Expected Pattern:**
```
AC 100:  Average ~90% of max damage
AC 500:  Average ~70% of max damage
AC 1000: Average ~50% of max damage
AC 2000: Average ~30% of max damage
```

---

## Validation Checklists

### Pre-Change Baseline

Before modifying combat code, record:

- [ ] Backstab average damage with 10/50/100 dmg weapons
- [ ] Frenzy average damage at level 30/50/65
- [ ] Flying kick average with 0/100/200 boot AC
- [ ] Auto-attack average with same weapons
- [ ] Crit rate over 500 attacks
- [ ] Time to kill test dummy

### Post-Change Verification

After changes, verify:

- [ ] All baseline tests still pass (or intentionally changed)
- [ ] No crashes on:
  - [ ] Normal attacks
  - [ ] Special attacks
  - [ ] PvP combat
  - [ ] Pet attacks
  - [ ] Ripostes
- [ ] Logs show expected values
- [ ] No memory leaks (extended test)
- [ ] Performance acceptable (mass combat test)

### Edge Cases

- [ ] Level 1 character attacking
- [ ] Level 65 character attacking level 1 mob
- [ ] 0 weapon damage (unarmed)
- [ ] 0 skill value
- [ ] Negative damage (should floor to 0 or 1)
- [ ] Integer overflow (very high stats)
- [ ] NULL target handling
- [ ] Dead attacker/target

---

## Debugging Combat Issues

### Common Issues

#### Damage Too Low

**Symptoms:** Attacks doing 1 damage or very low

**Debug Steps:**
1. Check weapon damage: `#showstats`
2. Check skill value: `#showskills`
3. Enable combat logging
4. Look for mitigation issues
5. Check min damage floor

```cpp
// Add temporary debug
Log(Logs::Detail, Logs::Combat,
    "DAMAGE DEBUG: base={} mitigation={} table_mult={} final={}",
    base, mitigation, table_mult, final);
```

#### Damage Too High

**Symptoms:** One-shotting mobs

**Debug Steps:**
1. Check for overflow (negative wraparound)
2. Verify crit multiplier
3. Check SPA 170 bonuses
4. Look for stacked damage mods

#### Skill Not Firing

**Symptoms:** Using ability does nothing

**Debug Steps:**
1. Check skill value > 0
2. Check reuse timer
3. Check level requirement
4. Check class restriction
5. Add entry-point logging:

```cpp
// In zone/special_attacks.cpp#L2589
void Mob::DoMeleeSkillAttackDmg(Mob *other, int32 weapon_damage,
                                EQ::skills::SkillType skillinuse, ...)
{
    Log(Logs::Detail, Logs::Combat, "DoMeleeSkillAttackDmg ENTERED skill={}", skillinuse);
    // ...
}
```

#### Crash on Combat

**Symptoms:** Zone crash during combat

**Debug Steps:**
1. Check NULL pointers
2. Check array bounds
3. Run with debugger attached
4. Check recent changes

```cpp
// Defensive coding
if (!defender) {
    Log(Logs::General, Logs::Error, "DoFrenzy: NULL defender!");
    return;
}
```

### Performance Testing

For mass combat scenarios:

```cpp
// Measure combat function time
auto start = std::chrono::high_resolution_clock::now();

// Combat code here

auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
Log(Logs::Detail, Logs::Combat, "DoFrenzy took {} us", duration.count());
```

---

## Test Database

### Quick Reset Script

```sql
-- Reset test dummy HP
UPDATE npc_types SET hp = 10000000 WHERE id BETWEEN 999001 AND 999999;

-- Remove test items from inventory
DELETE FROM character_inventory WHERE item_id BETWEEN 999001 AND 999999;

-- Clean test data
DELETE FROM npc_types WHERE id BETWEEN 999001 AND 999999;
DELETE FROM items WHERE id BETWEEN 999001 AND 999999;
```

### Sample Test Session

```
1. Login as GM character
2. #peqzone arena (or test zone)
3. #spawn 999001 (test dummy)
4. #summonitem 999001 (10 dmg sword)
5. #attack on
6. Wait, record average damage
7. #kill (dummy)
8. Repeat with different weapons/skills
9. Review logs/zone/Combat.log
```
