-- Infinite Progression System - Database Schema
-- Creates tables for storing dynamic items

USE peq;

-- Main dynamic items table
CREATE TABLE IF NOT EXISTS dynamic_items (
    item_id BIGINT UNSIGNED PRIMARY KEY COMMENT 'Dynamic ID (5LLLLLIIIIII format)',
    base_item_id INT NOT NULL COMMENT 'Original item ID from items table',
    item_level INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Item level (+0 to +99999)',
    random_stats JSON DEFAULT NULL COMMENT 'Future: JSON object with random stat modifiers',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT 'When item was first generated',
    last_accessed TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Cache tracking',

    INDEX idx_base_item (base_item_id),
    INDEX idx_level (item_level),
    INDEX idx_created (created_at),
    FOREIGN KEY (base_item_id) REFERENCES items(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Stores persistent dynamic items';

-- Player dynamic item inventory (extends character_inventory)
CREATE TABLE IF NOT EXISTS character_dynamic_items (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    character_id INT UNSIGNED NOT NULL COMMENT 'Character ID',
    slot_id INT NOT NULL COMMENT 'Inventory slot (same as character_inventory)',
    dynamic_item_id BIGINT UNSIGNED NOT NULL COMMENT 'Reference to dynamic_items.item_id',
    charges INT NOT NULL DEFAULT 1 COMMENT 'Item charges',
    augment_1 INT UNSIGNED DEFAULT 0,
    augment_2 INT UNSIGNED DEFAULT 0,
    augment_3 INT UNSIGNED DEFAULT 0,
    augment_4 INT UNSIGNED DEFAULT 0,
    augment_5 INT UNSIGNED DEFAULT 0,
    augment_6 INT UNSIGNED DEFAULT 0,

    UNIQUE KEY unique_char_slot (character_id, slot_id),
    INDEX idx_character (character_id),
    INDEX idx_dynamic_item (dynamic_item_id),
    FOREIGN KEY (dynamic_item_id) REFERENCES dynamic_items(item_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Character inventory for dynamic items';

-- Progression event log (for debugging and analytics)
CREATE TABLE IF NOT EXISTS progression_event_log (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    event_timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    event_type ENUM('create', 'upgrade', 'fuse', 'drop', 'delete') NOT NULL,
    character_id INT UNSIGNED DEFAULT NULL COMMENT 'Character involved (null for NPC drops)',
    item_id BIGINT UNSIGNED DEFAULT NULL COMMENT 'Dynamic item ID',
    base_item_id INT UNSIGNED DEFAULT NULL,
    old_level INT DEFAULT NULL,
    new_level INT DEFAULT NULL,
    details JSON DEFAULT NULL COMMENT 'Additional event data',

    INDEX idx_timestamp (event_timestamp),
    INDEX idx_character (character_id),
    INDEX idx_event_type (event_type),
    INDEX idx_item (item_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Audit log for progression events';

-- Configuration table
CREATE TABLE IF NOT EXISTS progression_config (
    config_key VARCHAR(64) PRIMARY KEY,
    config_value TEXT NOT NULL,
    description TEXT DEFAULT NULL,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Runtime configuration for progression system';

-- Insert default configuration
INSERT INTO progression_config (config_key, config_value, description) VALUES
    ('enabled', 'true', 'Enable/disable infinite progression system'),
    ('tier_size', '10', 'Levels per tier'),
    ('ac_base_increment', '1', 'AC per level (tier 0)'),
    ('hp_base_increment', '2', 'HP per level (tier 0)'),
    ('stat_base_increment', '1', 'Stat per level (tier 0)'),
    ('base_stat_cap', '127', 'Client stat cap (overflow to heroic)'),
    ('haste_start_level', '25', 'Level when haste begins'),
    ('heroic_start_level', '50', 'Level when milestone heroics begin'),
    ('cache_max_size', '1000', 'Maximum cached dynamic items'),
    ('debug_logging', 'true', 'Enable verbose logs to logs/inf/'),
    ('drop_dynamic_items', 'true', 'NPCs drop dynamic items instead of base items')
ON DUPLICATE KEY UPDATE config_value=VALUES(config_value);

-- Schema creation complete.
