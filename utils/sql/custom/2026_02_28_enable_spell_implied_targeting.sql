-- Ensure smart spell implied targeting is enabled.
-- This prevents beneficial spells from directly landing on hostile NPC targets
-- when pass-through targeting behavior is expected.

UPDATE `rule_values`
SET `rule_value` = 'true'
WHERE `rule_name` = 'Spells:UseSpellImpliedTargeting';

INSERT INTO `rule_values` (`ruleset_id`, `rule_name`, `rule_value`, `notes`)
SELECT 1, 'Spells:UseSpellImpliedTargeting', 'true', 'Enable EQ2-style spell implied targeting'
WHERE NOT EXISTS (
	SELECT 1
	FROM `rule_values`
	WHERE `ruleset_id` = 1
	  AND `rule_name` = 'Spells:UseSpellImpliedTargeting'
);
