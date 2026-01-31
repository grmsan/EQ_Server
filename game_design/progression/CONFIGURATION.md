# Account Progression - Configuration Guide

How to configure the account progression system without code changes.

## Table of Contents

1. [Quick Configuration](#quick-configuration)
2. [Zone Locks](#zone-locks)
3. [Level Caps](#level-caps)
4. [Perk Definitions](#perk-definitions)
5. [Veteran Bonus Tuning](#veteran-bonus-tuning)
6. [Server Rules Integration](#server-rules-integration)
7. [Examples by Server Type](#examples-by-server-type)

---

## Quick Configuration

The system is entirely database-driven. No code changes needed for:

- Adding/removing zone locks
- Changing level cap progression
- Creating new perks
- Tuning veteran bonus formulas

### Configuration Priority

1. **account_veteran_rules** - Core formula tuning
2. **account_unlock_definitions** - What unlocks exist
3. **account_zone_locks** - Which zones require unlocks
4. **account_level_caps** - How unlocks affect level cap
5. **account_perk_definitions** - What perks are available

---

## Zone Locks

### Adding a Zone Lock

```sql
-- Lock a zone behind an unlock
INSERT INTO account_zone_locks
(zone_id, zone_short_name, required_unlock_key, bypass_with_max_level, lock_message, enabled)
VALUES
(89, 'sebilis', 'sebilis_key', 1, 'You need the Key to Sebilis.', 1);
```

### Configuration Options

| Column | Description | Values |
|--------|-------------|--------|
| `zone_id` | Zone's ID from the zone table | Integer |
| `zone_short_name` | Zone's short name (for reference) | String |
| `required_unlock_key` | The unlock needed to enter | String (must exist in definitions) |
| `bypass_with_max_level` | Allow max-level accounts to bypass | 0 or 1 |
| `bypass_level` | Specific level for bypass (0 = server max) | 0-255 |
| `lock_message` | Message shown when denied | String |
| `enabled` | Whether this lock is active | 0 or 1 |

### Temporarily Disable a Lock

```sql
-- Disable without deleting
UPDATE account_zone_locks SET enabled = 0 WHERE zone_short_name = 'sebilis';

-- Re-enable
UPDATE account_zone_locks SET enabled = 1 WHERE zone_short_name = 'sebilis';
```

### Adjust Bypass Behavior

```sql
-- Require level 55+ specifically to bypass (not just "max level")
UPDATE account_zone_locks
SET bypass_level = 55
WHERE zone_short_name = 'sebilis';

-- Disable bypass entirely (must always have unlock)
UPDATE account_zone_locks
SET bypass_with_max_level = 0
WHERE zone_short_name = 'sebilis';
```

---

## Level Caps

### How Level Caps Work

1. Base level cap applies to all accounts (e.g., 50)
2. Unlocks grant higher caps (e.g., kill dragons → 60)
3. Higher priority caps override lower ones

### Setting Up Level Cap Progression

```sql
-- Base cap (no unlock required)
INSERT INTO account_level_caps (required_unlock_key, level_cap, priority, description)
VALUES ('', 50, 0, 'Classic content - base cap');

-- After killing Nagafen and Vox
INSERT INTO account_level_caps (required_unlock_key, level_cap, priority, description)
VALUES ('classic_complete', 60, 10, 'Kunark content');

-- Alternative path: specific raid completions
INSERT INTO account_level_caps (required_unlock_key, level_cap, priority, description)
VALUES ('killed_trakanon', 60, 10, 'Kunark via Trakanon');
```

### Checking an Account's Cap

```sql
-- Find the highest level cap an account qualifies for
SELECT MAX(lc.level_cap) as effective_cap
FROM account_level_caps lc
WHERE lc.required_unlock_key = ''
   OR lc.required_unlock_key IN (
       SELECT unlock_key FROM account_unlocks WHERE account_id = ?
   );
```

---

## Perk Definitions

### Creating a Non-Stackable Perk

```sql
-- Teleport ability - either you have it or you don't
INSERT INTO account_perk_definitions
(perk_type, perk_key, name, description, base_value, max_value, stackable, enabled)
VALUES
('teleport', 'kelethin', 'Gate to Kelethin',
 'Teleport to Kelethin from anywhere', 1, 1, 0, 1);
```

### Creating a Stackable Perk (No Diminishing)

```sql
-- +2 STR per completion, max +10 total, no diminishing
INSERT INTO account_perk_definitions
(perk_type, perk_key, name, description, base_value, max_value, max_stacks,
 diminishing_type, stackable, enabled)
VALUES
('stat', 'str', 'Strength Bonus', 'Account-wide strength',
 2, 10, 5, 'none', 1, 1);
```

### Creating a Perk with Linear Diminishing Returns

```sql
-- +2 WIS first time, loses 0.5 effectiveness each time
-- Stack 1: +2, Stack 2: +1.5→+1, Stack 3: +1, Stack 4: +0.5→+0
INSERT INTO account_perk_definitions
(perk_type, perk_key, name, description, base_value, max_value, max_stacks,
 diminishing_type, diminishing_factor, stackable, enabled)
VALUES
('stat', 'wis', 'Wisdom Bonus', 'Account-wide wisdom (diminishing)',
 2, 10, NULL, 'linear', 0.5, 1, 1);
```

### Creating a Perk with Exponential Diminishing Returns

```sql
-- Regen bonus: 2 * (0.5 ^ times_completed)
-- Stack 1: +2, Stack 2: +1, Stack 3: +0.5→+0, etc.
INSERT INTO account_perk_definitions
(perk_type, perk_key, name, description, base_value, max_value, max_stacks,
 diminishing_type, diminishing_factor, stackable, enabled)
VALUES
('regen', 'hp', 'HP Regeneration', 'Account-wide HP regen (rapidly diminishing)',
 2, 5, 10, 'exponential', 0.5, 1, 1);
```

### Diminishing Returns Formula Reference

| Type | Formula | Example (base=2, factor=0.5) |
|------|---------|------------------------------|
| `none` | `base_value` always | +2, +2, +2, +2... |
| `linear` | `base_value - (times_completed * factor)` | +2, +1.5, +1, +0.5, +0... |
| `exponential` | `base_value * (factor ^ times_completed)` | +2, +1, +0.5, +0.25... |

---

## Veteran Bonus Tuning

### XP Bonus Configuration

```sql
-- Adjust XP bonus per level difference
UPDATE account_veteran_rules
SET rule_value = '0.03' -- 3% per level instead of 2%
WHERE rule_key = 'xp_bonus_per_level';

-- Adjust maximum XP bonus
UPDATE account_veteran_rules
SET rule_value = '0.75' -- Cap at 75% instead of 100%
WHERE rule_key = 'xp_bonus_max';
```

### AA Bonus Configuration

```sql
-- Adjust what fraction of other chars' AAs count
UPDATE account_veteran_rules
SET rule_value = '0.75' -- 75% instead of 50%
WHERE rule_key = 'aa_veteran_pool_ratio';

-- Adjust maximum AA bonus
UPDATE account_veteran_rules
SET rule_value = '0.30' -- Cap at 30% instead of 50%
WHERE rule_key = 'aa_bonus_max';
```

### Inherited Progress Configuration

```sql
-- Disable max-level zone bypass
UPDATE account_veteran_rules
SET rule_value = '0'
WHERE rule_key = 'inherit_zones_at_max_level';

-- Adjust cache refresh rate
UPDATE account_veteran_rules
SET rule_value = '30' -- Refresh every 30 minutes instead of 60
WHERE rule_key = 'veteran_cache_ttl_minutes';
```

### All Veteran Rules Reference

| Rule Key | Default | Description |
|----------|---------|-------------|
| `xp_bonus_per_level` | 0.02 | XP multiplier per level below highest char |
| `xp_bonus_max` | 1.0 | Maximum XP bonus (1.0 = 100%) |
| `aa_veteran_pool_ratio` | 0.5 | Fraction of other chars' AAs that count |
| `aa_bonus_max` | 0.5 | Maximum AA gain bonus |
| `inherit_zones_at_max_level` | 1 | Whether max-level unlocks all zones |
| `veteran_cache_ttl_minutes` | 60 | How often to refresh veteran data cache |

---

## Server Rules Integration

If you want to integrate with the existing rules system:

```sql
-- Example: Add to server rules table if you have one
INSERT INTO rule_values (ruleset_id, rule_name, rule_value) VALUES
(1, 'AccountProgression:XPBonusPerLevel', '0.02'),
(1, 'AccountProgression:XPBonusMax', '1.0'),
(1, 'AccountProgression:AAPoolRatio', '0.5'),
(1, 'AccountProgression:AABonusMax', '0.5'),
(1, 'AccountProgression:InheritZonesAtMaxLevel', 'true'),
(1, 'AccountProgression:Enabled', 'true');
```

### C++ Rules Integration

```cpp
// In common/ruletypes.h, add:
RULE_BOOL(AccountProgression, Enabled, true)
RULE_REAL(AccountProgression, XPBonusPerLevel, 0.02)
RULE_REAL(AccountProgression, XPBonusMax, 1.0)
RULE_REAL(AccountProgression, AAPoolRatio, 0.5)
RULE_REAL(AccountProgression, AABonusMax, 0.5)
RULE_BOOL(AccountProgression, InheritZonesAtMaxLevel, true)

// Then in AccountProgressionManager, prefer rules over database config:
float GetVeteranRule(const std::string& key, float default_val) {
    if (key == "xp_bonus_per_level") {
        return RuleR(AccountProgression, XPBonusPerLevel);
    }
    // ... etc
}
```

---

## Examples by Server Type

### Classic Progression Server

Focus on content gates, minimal catchup mechanics.

```sql
-- Strict progression: must earn everything
UPDATE account_veteran_rules SET rule_value = '0'
WHERE rule_key = 'inherit_zones_at_max_level';

-- Minimal XP bonus
UPDATE account_veteran_rules SET rule_value = '0.01'
WHERE rule_key = 'xp_bonus_per_level';
UPDATE account_veteran_rules SET rule_value = '0.25'
WHERE rule_key = 'xp_bonus_max';

-- No AA bonus
UPDATE account_veteran_rules SET rule_value = '0'
WHERE rule_key = 'aa_veteran_pool_ratio';

-- Lock all keyed zones
INSERT INTO account_zone_locks VALUES
(89, 'sebilis', 'sebilis_key', 0, 0, 'You must earn the Key to Sebilis.', 1),
(105, 'charasis', 'charasis_key', 0, 0, 'You must earn the Howling Stones Key.', 1),
(108, 'veeshan', 'veeshan_key', 0, 0, 'You must earn the Veeshan Peak Key.', 1);
```

### Alt-Friendly Server

Strong catchup mechanics, reduced friction.

```sql
-- Full inherited progress
UPDATE account_veteran_rules SET rule_value = '1'
WHERE rule_key = 'inherit_zones_at_max_level';

-- Generous XP bonus
UPDATE account_veteran_rules SET rule_value = '0.04'
WHERE rule_key = 'xp_bonus_per_level';
UPDATE account_veteran_rules SET rule_value = '2.0' -- 200% max!
WHERE rule_key = 'xp_bonus_max';

-- Strong AA catchup
UPDATE account_veteran_rules SET rule_value = '0.75'
WHERE rule_key = 'aa_veteran_pool_ratio';
UPDATE account_veteran_rules SET rule_value = '1.0'
WHERE rule_key = 'aa_bonus_max';

-- All zones bypass with max level
UPDATE account_zone_locks SET bypass_with_max_level = 1;
```

### Hardcore / Permadeath Server

No account progression benefits.

```sql
-- Disable all veteran bonuses
UPDATE account_veteran_rules SET rule_value = '0' WHERE rule_key LIKE '%bonus%';
UPDATE account_veteran_rules SET rule_value = '0' WHERE rule_key LIKE '%ratio%';
UPDATE account_veteran_rules SET rule_value = '0'
WHERE rule_key = 'inherit_zones_at_max_level';

-- Disable all perks
UPDATE account_perk_definitions SET enabled = 0;

-- Only account unlocks work (zone keys)
-- Zones still require keys, no bypass
UPDATE account_zone_locks SET bypass_with_max_level = 0;
```

### Custom Event Server

Temporary bonuses for events.

```sql
-- Double XP catchup during event
UPDATE account_veteran_rules SET rule_value = '0.04'
WHERE rule_key = 'xp_bonus_per_level';

-- After event, revert
UPDATE account_veteran_rules SET rule_value = '0.02'
WHERE rule_key = 'xp_bonus_per_level';

-- Grant temporary perk via event script (Lua)
-- eq.grant_account_perk(client, "event", "double_xp", 1, "Summer Event 2024")
```

---

## Hot-Reload Configuration

The system should support reloading configuration without server restart:

### Via GM Command

```
#reloadaccountconfig
```

### Via Code

```cpp
// Force reload all configuration
AccountProgressionManager::Instance().LoadConfiguration();
```

### Via Lua

```lua
eq.reload_account_progression_config()
```

This reloads:
- Veteran rules
- Perk definitions
- Zone locks
- Level caps

**Note:** Does NOT reload already-granted unlocks/perks. Those are persistent.
