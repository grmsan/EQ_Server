-- Enable lore-equip behavior:
-- - duplicate lore items are allowed in inventory/bank/cursor
-- - duplicate lore equip attempts are blocked server-side during item swaps

UPDATE `rule_values`
SET `rule_value` = 'true'
WHERE `rule_name` = 'Items:LoreEquippedOnly';

INSERT INTO `rule_values` (`ruleset_id`, `rule_name`, `rule_value`, `notes`)
SELECT 1, 'Items:LoreEquippedOnly', 'true', 'Allow lore duplicates in inventory; enforce lore uniqueness only on equipment swaps'
WHERE NOT EXISTS (
	SELECT 1
	FROM `rule_values`
	WHERE `ruleset_id` = 1
	  AND `rule_name` = 'Items:LoreEquippedOnly'
);
