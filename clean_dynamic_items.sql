-- Clean up old dynamic items from database
-- Run this in HeidiSQL or mysql client before starting the server

USE peq;

-- Remove dynamic items from items table (ID >= 1 billion)
DELETE FROM items WHERE id >= 1000000000;

-- Remove dynamic items from inventory (players can't have items that don't exist)
DELETE FROM inventory WHERE itemid >= 1000000000;

-- Remove dynamic items from shared bank
DELETE FROM sharedbank WHERE itemid >= 1000000000;

-- Clear custom_data from inventory (no longer needed for dynamic items)
UPDATE inventory SET custom_data = '' WHERE custom_data LIKE 'dynamic_level%';

-- Check results
SELECT 'Cleanup complete' AS status;
SELECT COUNT(*) AS remaining_dynamic_items_in_items FROM items WHERE id >= 1000000000;
SELECT COUNT(*) AS remaining_dynamic_items_in_inventory FROM inventory WHERE itemid >= 1000000000;
