-- Cleanup script to purge all generated (dynamic) items.
-- Dynamic items are stored in the main `items` table with IDs >= 1,000,000,000.
-- This variant deletes ONLY the dynamic item rows from `items` and reports where
-- they exist in inventories/banks/etc. to avoid silently wiping player items.
-- You MUST follow up in-game or with tooling to regenerate/replace any dynamic
-- items still referenced in those tables.
-- BACK UP your database before running.
--
-- Run: mysql -h 127.0.0.1 -P 3308 -u root -p peq < utils/sql/custom/cleanup_dynamic_items.sql

USE peq;

SET @dyn_min_id = 1000000000;

START TRANSACTION;

-- Report where dynamic items are referenced (no deletes here)
SELECT 'inventory'                       AS table_name, COUNT(*) AS rows_affected FROM inventory                       WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'sharedbank',                      COUNT(*) FROM sharedbank                      WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'character_corpse_items',          COUNT(*) FROM character_corpse_items          WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'character_potionbelt',            COUNT(*) FROM character_potionbelt            WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'character_bandolier',             COUNT(*) FROM character_bandolier             WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'character_pet_inventory',         COUNT(*) FROM character_pet_inventory         WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'character_evolving_items',        COUNT(*) FROM character_evolving_items        WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'items_evolving_details',          COUNT(*) FROM items_evolving_details          WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'character_parcels',               COUNT(*) FROM character_parcels               WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'character_parcels_containers',    COUNT(*) FROM character_parcels_containers    WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'keyring',                         COUNT(*) FROM keyring                         WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'discovered_items',                COUNT(*) FROM discovered_items                WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'inventory_snapshots',             COUNT(*) FROM inventory_snapshots             WHERE itemid   >= @dyn_min_id
UNION ALL
SELECT 'trader',                          COUNT(*) FROM trader                          WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'buyer_buy_lines',                 COUNT(*) FROM buyer_buy_lines                 WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'buyer_trade_items',               COUNT(*) FROM buyer_trade_items               WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'guild_bank',                      COUNT(*) FROM guild_bank                      WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'alternate_currency',              COUNT(*) FROM alternate_currency              WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'object',                          COUNT(*) FROM object                          WHERE itemid   >= @dyn_min_id
UNION ALL
SELECT 'object_contents',                 COUNT(*) FROM object_contents                 WHERE itemid   >= @dyn_min_id
UNION ALL
SELECT 'merchantlist_temp',               COUNT(*) FROM merchantlist_temp               WHERE itemid   >= @dyn_min_id
UNION ALL
SELECT 'starting_items',                  COUNT(*) FROM starting_items                  WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'fishing',                         COUNT(*) FROM fishing                         WHERE `Itemid` >= @dyn_min_id
UNION ALL
SELECT 'forage',                          COUNT(*) FROM forage                          WHERE `Itemid` >= @dyn_min_id
UNION ALL
SELECT 'pets_equipmentset_entries',       COUNT(*) FROM pets_equipmentset_entries       WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'player_event_loot_items',         COUNT(*) FROM player_event_loot_items         WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'player_event_merchant_purchase',  COUNT(*) FROM player_event_merchant_purchase  WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'player_event_merchant_sell',      COUNT(*) FROM player_event_merchant_sell      WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'player_event_npc_handin_entries', COUNT(*) FROM player_event_npc_handin_entries WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'player_event_trade_entries',      COUNT(*) FROM player_event_trade_entries      WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'progression_event_log',           COUNT(*) FROM progression_event_log           WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'titles',                          COUNT(*) FROM titles                          WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'tradeskill_recipe_entries',       COUNT(*) FROM tradeskill_recipe_entries       WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'tribute_levels',                  COUNT(*) FROM tribute_levels                  WHERE item_id  >= @dyn_min_id
UNION ALL
SELECT 'veteran_reward_templates',        COUNT(*) FROM veteran_reward_templates        WHERE item_id  >= @dyn_min_id;

-- Finally, delete the dynamic item rows themselves
DELETE FROM items WHERE id >= @dyn_min_id;

COMMIT;

-- Verification
SELECT COUNT(*) AS remaining_dynamic_items FROM items WHERE id >= @dyn_min_id;
