-- Enable cross-class trainer interaction so guildmaster-class NPC hails
-- do not block multiclass class-add dialog flows.
UPDATE `rule_values`
SET `rule_value` = 'true'
WHERE `rule_name` = 'Character:AllowCrossClassTrainers';

INSERT INTO `rule_values` (`ruleset_id`, `rule_name`, `rule_value`, `notes`)
SELECT 1, 'Character:AllowCrossClassTrainers', 'true', 'Required for multiclass guildmaster class-add interactions'
WHERE NOT EXISTS (
	SELECT 1
	FROM `rule_values`
	WHERE `ruleset_id` = 1
	  AND `rule_name` = 'Character:AllowCrossClassTrainers'
);
