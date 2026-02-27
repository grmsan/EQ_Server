-- Ensure #test appears in command_settings for environments that have not yet
-- restarted world/zone since the command was added in source.
INSERT INTO `command_settings` (`command`, `access`, `aliases`)
VALUES ('test', 0, '')
ON DUPLICATE KEY UPDATE
	`access` = LEAST(`access`, VALUES(`access`));
