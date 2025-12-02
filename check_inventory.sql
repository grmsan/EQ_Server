-- Check for dynamic items in inventory
USE peq;

SELECT
    charid,
    slotid,
    itemid,
    CASE
        WHEN itemid >= 1000000000 THEN 'DYNAMIC'
        ELSE 'NORMAL'
    END AS item_type,
    custom_data
FROM inventory
WHERE itemid >= 1000000000
ORDER BY charid, slotid
LIMIT 20;
