-- Convert custom_data from JSON format to ^-delimited format
-- This converts {"dynamic_level":123} to dynamic_level^123

UPDATE character_inventory
SET custom_data = CONCAT('dynamic_level^',
    SUBSTRING_INDEX(SUBSTRING_INDEX(custom_data, ':', -1), '}', 1))
WHERE custom_data LIKE '%dynamic_level%'
AND custom_data LIKE '{%}';

-- Show the results
SELECT charid, slotid, itemid, custom_data
FROM character_inventory
WHERE custom_data LIKE '%dynamic_level%'
LIMIT 20;
