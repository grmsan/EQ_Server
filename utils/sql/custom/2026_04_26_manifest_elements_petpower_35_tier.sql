START TRANSACTION;

-- Give Manifest Elements an explicit 35-power focused tier so it lands near the same
-- level 65 band as the other permanent pets that already use pets-table focus rows.

DROP PROCEDURE IF EXISTS copy_npc_type_template;
DELIMITER //
CREATE PROCEDURE copy_npc_type_template(IN p_template_id INT, IN p_new_id INT)
BEGIN
	SET @cols := (
		SELECT GROUP_CONCAT(CONCAT('`', COLUMN_NAME, '`') ORDER BY ORDINAL_POSITION SEPARATOR ',')
		FROM INFORMATION_SCHEMA.COLUMNS
		WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'npc_types'
	);

	SET @select_cols := (
		SELECT GROUP_CONCAT(
			CASE
				WHEN COLUMN_NAME = 'id' THEN CAST(p_new_id AS CHAR)
				ELSE CONCAT('`', COLUMN_NAME, '`')
			END
			ORDER BY ORDINAL_POSITION SEPARATOR ','
		)
		FROM INFORMATION_SCHEMA.COLUMNS
		WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'npc_types'
	);

	SET @copy_sql := CONCAT(
		'REPLACE INTO `npc_types` (', @cols, ') ',
		'SELECT ', @select_cols, ' FROM `npc_types` WHERE `id` = ', p_template_id
	);

	PREPARE stmt FROM @copy_sql;
	EXECUTE stmt;
	DEALLOCATE PREPARE stmt;
END//
DELIMITER ;

CALL copy_npc_type_template(598, 1120001598);
DROP PROCEDURE IF EXISTS copy_npc_type_template;

UPDATE `npc_types`
SET
	`name` = 'SumMageMultiElement_Focus35',
	`level` = 65,
	`hp` = 5805,
	`mindmg` = 61,
	`maxdmg` = 99,
	`AC` = 304,
	`size` = 7.05
WHERE `id` = 1120001598;

REPLACE INTO `pets` (
	`type`,
	`petpower`,
	`npcID`,
	`temp`,
	`petcontrol`,
	`petnaming`,
	`monsterflag`,
	`equipmentset`
)
SELECT
	`type`,
	35,
	1120001598,
	`temp`,
	`petcontrol`,
	`petnaming`,
	`monsterflag`,
	`equipmentset`
FROM `pets`
WHERE `type` = 'SumMageMultiElement'
ORDER BY `petpower` DESC
LIMIT 1;

COMMIT;