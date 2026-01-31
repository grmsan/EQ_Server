# Account Progression - Database Schema

Complete SQL schema for the account progression system. Run these in order.

## Table of Contents

1. [Core Tables](#core-tables)
2. [Configuration Tables](#configuration-tables)
3. [Indexes and Performance](#indexes-and-performance)
4. [Sample Data](#sample-data)
5. [Migration Scripts](#migration-scripts)

---

## Core Tables

### account_unlocks

Stores one-time account achievements (zone keys, expansion unlocks, etc.)

```sql
CREATE TABLE `account_unlocks` (
    `id` INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    `account_id` INT UNSIGNED NOT NULL,
    `unlock_key` VARCHAR(64) NOT NULL,
    `unlocked_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
    `unlocked_by_char_id` INT UNSIGNED DEFAULT 0,
    `unlocked_by_char_name` VARCHAR(64) DEFAULT '',
    UNIQUE KEY `account_unlock` (`account_id`, `unlock_key`),
    KEY `idx_account_id` (`account_id`),
    KEY `idx_unlock_key` (`unlock_key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### account_perks

Stores permanent bonuses earned by the account.

```sql
CREATE TABLE `account_perks` (
    `id` INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    `account_id` INT UNSIGNED NOT NULL,
    `perk_type` VARCHAR(32) NOT NULL,
    `perk_key` VARCHAR(64) NOT NULL,
    `perk_value` INT NOT NULL DEFAULT 0,
    `times_completed` INT UNSIGNED DEFAULT 1,
    `max_value` INT DEFAULT NULL,
    `source_description` VARCHAR(128) DEFAULT '',
    `first_earned_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
    `last_updated_at` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY `account_perk` (`account_id`, `perk_type`, `perk_key`),
    KEY `idx_account_id` (`account_id`),
    KEY `idx_perk_type` (`perk_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

**perk_type values:**
- `stat` - Stat bonuses (str, sta, agi, dex, wis, int, cha)
- `aa` - Granted AAs
- `teleport` - Teleport abilities
- `faction_fix` - Faction reset abilities
- `skill` - Skill bonuses
- `resist` - Resist bonuses
- `regen` - HP/Mana/End regen bonuses

### account_veteran_cache

Cached veteran bonus data (updated periodically for performance).

```sql
CREATE TABLE `account_veteran_cache` (
    `account_id` INT UNSIGNED PRIMARY KEY,
    `highest_level` TINYINT UNSIGNED DEFAULT 1,
    `highest_level_char_id` INT UNSIGNED DEFAULT 0,
    `total_aa_points` INT UNSIGNED DEFAULT 0,
    `total_play_time_minutes` INT UNSIGNED DEFAULT 0,
    `character_count` INT UNSIGNED DEFAULT 0,
    `last_updated` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

---

## Configuration Tables

### account_unlock_definitions

Defines what unlocks exist and their requirements (admin-configurable).

```sql
CREATE TABLE `account_unlock_definitions` (
    `id` INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    `unlock_key` VARCHAR(64) NOT NULL UNIQUE,
    `unlock_type` ENUM('zone_key', 'level_cap', 'expansion', 'feature', 'achievement') NOT NULL,
    `name` VARCHAR(128) NOT NULL,
    `description` TEXT,
    `unlock_message` VARCHAR(256) DEFAULT '',
    `required_unlocks` VARCHAR(256) DEFAULT '',  -- Comma-separated prerequisite unlock_keys
    `enabled` TINYINT(1) DEFAULT 1,
    KEY `idx_unlock_type` (`unlock_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### account_zone_locks

Defines which zones require account unlocks.

```sql
CREATE TABLE `account_zone_locks` (
    `zone_id` INT UNSIGNED PRIMARY KEY,
    `zone_short_name` VARCHAR(32) NOT NULL,
    `required_unlock_key` VARCHAR(64) NOT NULL,
    `bypass_with_max_level` TINYINT(1) DEFAULT 1,  -- If true, max level chars bypass
    `bypass_level` TINYINT UNSIGNED DEFAULT 0,     -- Level at which bypass kicks in (0 = use server max)
    `lock_message` VARCHAR(256) DEFAULT 'You have not unlocked access to this zone.',
    `enabled` TINYINT(1) DEFAULT 1,
    KEY `idx_unlock_key` (`required_unlock_key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### account_level_caps

Defines level cap progression.

```sql
CREATE TABLE `account_level_caps` (
    `id` INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    `required_unlock_key` VARCHAR(64) NOT NULL,
    `level_cap` TINYINT UNSIGNED NOT NULL,
    `priority` INT DEFAULT 0,  -- Higher priority caps override lower
    `description` VARCHAR(128) DEFAULT '',
    KEY `idx_unlock_key` (`required_unlock_key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### account_perk_definitions

Defines available perks and their stacking rules.

```sql
CREATE TABLE `account_perk_definitions` (
    `id` INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    `perk_type` VARCHAR(32) NOT NULL,
    `perk_key` VARCHAR(64) NOT NULL,
    `name` VARCHAR(128) NOT NULL,
    `description` TEXT,
    `base_value` INT NOT NULL DEFAULT 1,
    `max_value` INT DEFAULT NULL,              -- NULL = no cap
    `max_stacks` INT DEFAULT NULL,             -- NULL = no stack limit
    `diminishing_type` ENUM('none', 'linear', 'exponential') DEFAULT 'none',
    `diminishing_factor` FLOAT DEFAULT 0,      -- Amount reduced per stack
    `stackable` TINYINT(1) DEFAULT 0,
    `enabled` TINYINT(1) DEFAULT 1,
    UNIQUE KEY `perk_type_key` (`perk_type`, `perk_key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### account_veteran_rules

Configures veteran bonus calculations.

```sql
CREATE TABLE `account_veteran_rules` (
    `rule_key` VARCHAR(64) PRIMARY KEY,
    `rule_value` VARCHAR(128) NOT NULL,
    `description` VARCHAR(256) DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Default veteran rules
INSERT INTO `account_veteran_rules` VALUES
('xp_bonus_per_level', '0.02', 'XP bonus multiplier per level difference from highest char'),
('xp_bonus_max', '1.0', 'Maximum XP bonus (1.0 = 100%)'),
('aa_veteran_pool_ratio', '0.5', 'Fraction of other chars total AAs that count as veteran pool'),
('aa_bonus_max', '0.5', 'Maximum AA gain bonus (0.5 = 50%)'),
('inherit_zones_at_max_level', '1', '1 = Max level char unlocks all zones for account'),
('veteran_cache_ttl_minutes', '60', 'How often to refresh veteran cache');
```

---

## Indexes and Performance

### Additional Indexes

```sql
-- Fast lookup for character's account
ALTER TABLE `character_data` ADD INDEX `idx_account_id` (`account_id`);

-- Fast AA totals query
ALTER TABLE `character_alt_currency` ADD INDEX `idx_char_currency` (`char_id`, `currency_id`);
```

### Recommended Queries Should Use

```sql
-- Always query account_unlocks by account_id + unlock_key (uses unique index)
-- Always query account_perks by account_id (uses index)
-- Use account_veteran_cache instead of computing on-the-fly
```

---

## Sample Data

### Unlock Definitions

```sql
INSERT INTO `account_unlock_definitions` (`unlock_key`, `unlock_type`, `name`, `description`, `unlock_message`) VALUES
-- Zone Keys
('sebilis_key', 'zone_key', 'Key to Sebilis', 'Complete the Trakanon Idol quest', 'All your characters can now enter Sebilis!'),
('veeshan_key', 'zone_key', 'Veeshan Peak Key', 'Complete the VP medallion quest', 'All your characters can now enter Veeshan Peak!'),
('charasis_key', 'zone_key', 'Howling Stones Key', 'Obtain key from Kaesora', 'All your characters can now enter Charasis!'),

-- Expansion Unlocks  
('classic_complete', 'expansion', 'Classic Complete', 'Kill Lord Nagafen AND Lady Vox', 'Kunark content is now unlocked!'),
('kunark_complete', 'expansion', 'Kunark Complete', 'Complete Kunark progression', 'Velious content is now unlocked!'),

-- Level Caps
('killed_nagafen', 'achievement', 'Nagafen Slain', 'Defeated Lord Nagafen', ''),
('killed_vox', 'achievement', 'Vox Slain', 'Defeated Lady Vox', ''),

-- Features
('bazaar_access', 'feature', 'Bazaar Access', 'Reach level 10 on any character', 'You can now use the Bazaar!');
```

### Zone Locks

```sql
INSERT INTO `account_zone_locks` (`zone_id`, `zone_short_name`, `required_unlock_key`, `bypass_with_max_level`, `lock_message`) VALUES
-- Sebilis requires key
(89, 'sebilis', 'sebilis_key', 1, 'You need the Key to Sebilis. Complete the Trakanon Idol quest.'),
-- Veeshan Peak requires key
(108, 'veeshan', 'veeshan_key', 1, 'You need the Veeshan Peak Key.'),
-- Charasis requires key
(105, 'charasis', 'charasis_key', 1, 'You need the Howling Stones Key.'),
-- Lower Guk requires intro quest
(66, 'gukbottom', 'lower_guk_access', 1, 'Complete the Upper Guk introduction quest first.');
```

### Level Caps

```sql
INSERT INTO `account_level_caps` (`required_unlock_key`, `level_cap`, `priority`, `description`) VALUES
('', 50, 0, 'Base level cap - Classic'),
('classic_complete', 60, 10, 'Kunark level cap'),
('kunark_complete', 60, 20, 'Velious level cap');
-- Add more as expansions are added
```

### Perk Definitions

```sql
INSERT INTO `account_perk_definitions` 
(`perk_type`, `perk_key`, `name`, `description`, `base_value`, `max_value`, `max_stacks`, `diminishing_type`, `diminishing_factor`, `stackable`) VALUES
-- Non-stackable perks
('teleport', 'neriak', 'Gate to Neriak', 'Teleport to Neriak', 1, 1, 1, 'none', 0, 0),
('teleport', 'qeynos', 'Gate to Qeynos', 'Teleport to Qeynos', 1, 1, 1, 'none', 0, 0),
('faction_fix', 'neriak', 'Neriak Faction Reset', 'Reset Neriak faction to neutral', 1, 1, 1, 'none', 0, 0),

-- Stackable stat perks
('stat', 'str', 'Strength Bonus', 'Account-wide strength bonus', 2, 10, 5, 'none', 0, 1),
('stat', 'sta', 'Stamina Bonus', 'Account-wide stamina bonus', 2, 10, 5, 'none', 0, 1),
('stat', 'wis', 'Wisdom Bonus', 'Account-wide wisdom bonus', 2, 10, 5, 'linear', 0.25, 1),
('stat', 'int', 'Intelligence Bonus', 'Account-wide intelligence bonus', 2, 10, 5, 'linear', 0.25, 1),

-- Regen perks with diminishing returns
('regen', 'hp', 'HP Regeneration', 'Account-wide HP regen bonus', 1, 5, 10, 'exponential', 0.5, 1);
```

---

## Migration Scripts

### For Existing Servers

If you have existing characters who should have retroactive unlocks:

```sql
-- Grant sebilis key to accounts that have a char with the key item
INSERT IGNORE INTO account_unlocks (account_id, unlock_key, unlocked_by_char_id)
SELECT DISTINCT cd.account_id, 'sebilis_key', cd.id
FROM character_data cd
JOIN inventory i ON i.charid = cd.id
WHERE i.itemid = 20884;  -- Sebilis key item ID

-- Grant classic_complete to accounts that killed both dragons
-- (Requires kill tracking - adjust based on your system)

-- Update veteran cache for all accounts
INSERT INTO account_veteran_cache (account_id, highest_level, total_aa_points, character_count)
SELECT 
    account_id,
    MAX(level) as highest_level,
    SUM(aa_points_spent) as total_aa_points,
    COUNT(*) as character_count
FROM character_data
GROUP BY account_id
ON DUPLICATE KEY UPDATE
    highest_level = VALUES(highest_level),
    total_aa_points = VALUES(total_aa_points),
    character_count = VALUES(character_count),
    last_updated = NOW();
```

### Refresh Veteran Cache (Scheduled Task)

```sql
-- Run this periodically (every hour or on login)
DELIMITER //
CREATE PROCEDURE RefreshVeteranCache(IN p_account_id INT UNSIGNED)
BEGIN
    INSERT INTO account_veteran_cache (account_id, highest_level, highest_level_char_id, total_aa_points, character_count)
    SELECT 
        account_id,
        MAX(level),
        (SELECT id FROM character_data WHERE account_id = p_account_id ORDER BY level DESC LIMIT 1),
        COALESCE(SUM(aa_points_spent), 0),
        COUNT(*)
    FROM character_data
    WHERE account_id = p_account_id AND deleted_at IS NULL
    GROUP BY account_id
    ON DUPLICATE KEY UPDATE
        highest_level = VALUES(highest_level),
        highest_level_char_id = VALUES(highest_level_char_id),
        total_aa_points = VALUES(total_aa_points),
        character_count = VALUES(character_count),
        last_updated = NOW();
END //
DELIMITER ;
```

---

## Cleanup Queries

```sql
-- Remove unlocks for deleted accounts
DELETE au FROM account_unlocks au
LEFT JOIN account a ON au.account_id = a.id
WHERE a.id IS NULL;

-- Remove perks for deleted accounts
DELETE ap FROM account_perks ap
LEFT JOIN account a ON ap.account_id = a.id
WHERE a.id IS NULL;
```
