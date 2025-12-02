-- Clean up broken dynamic items from database
-- Remove bad custom_data entries (levels > 150 are encoded values, not actual levels)
UPDATE inventory SET custom_data = '' WHERE custom_data LIKE 'dynamic_level^%';

-- Delete all dynamic items from items table (they'll be recreated on demand)
DELETE FROM items WHERE id >= 1000000000;
