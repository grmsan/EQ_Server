START TRANSACTION;

-- THJ-style bow healing augments with level gates.
-- Template item: 82458 (Turquoise Lifetap Sieve)

DROP PROCEDURE IF EXISTS copy_item_template;
DELIMITER //
CREATE PROCEDURE copy_item_template(IN p_template_id INT, IN p_new_item_id INT)
BEGIN
	SET @cols := (
		SELECT GROUP_CONCAT(CONCAT('`', COLUMN_NAME, '`') ORDER BY ORDINAL_POSITION SEPARATOR ',')
		FROM INFORMATION_SCHEMA.COLUMNS
		WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'items'
	);

	SET @select_cols := (
		SELECT GROUP_CONCAT(
			CASE
				WHEN COLUMN_NAME = 'id' THEN CAST(p_new_item_id AS CHAR)
				ELSE CONCAT('`', COLUMN_NAME, '`')
			END
			ORDER BY ORDINAL_POSITION SEPARATOR ','
		)
		FROM INFORMATION_SCHEMA.COLUMNS
		WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'items'
	);

	SET @copy_sql := CONCAT(
		'REPLACE INTO `items` (', @cols, ') ',
		'SELECT ', @select_cols, ' FROM `items` WHERE `id` = ', p_template_id
	);

	PREPARE stmt FROM @copy_sql;
	EXECUTE stmt;
	DEALLOCATE PREPARE stmt;
END//
DELIMITER ;

CALL copy_item_template(82458, 1152012000);
CALL copy_item_template(82458, 1152012001);
CALL copy_item_template(82458, 1152012002);
CALL copy_item_template(82458, 1152012003);

DROP PROCEDURE IF EXISTS copy_item_template;

UPDATE items
SET
	Name = CASE id
		WHEN 1152012000 THEN 'Lesser Bowstone of Mending'
		WHEN 1152012001 THEN 'Bowstone of Mending'
		WHEN 1152012002 THEN 'Greater Bowstone of Mending'
		WHEN 1152012003 THEN 'Grand Bowstone of Mending'
	END,
	lore = CASE id
		WHEN 1152012000 THEN 'A crystalline shard tuned to mend the archer.'
		WHEN 1152012001 THEN 'A crystalline shard tuned to mend the archer.'
		WHEN 1152012002 THEN 'A crystalline shard tuned to mend the archer.'
		WHEN 1152012003 THEN 'A crystalline shard tuned to mend the archer.'
	END,
	augtype = 8,
	augrestrict = 12,
	itemtype = 54,
	itemclass = 0,
	slots = 26624,
	classes = 65535,
	races = 65535,
	reqlevel = CASE id
		WHEN 1152012000 THEN 20
		WHEN 1152012001 THEN 40
		WHEN 1152012002 THEN 60
		WHEN 1152012003 THEN 80
	END,
	reclevel = CASE id
		WHEN 1152012000 THEN 20
		WHEN 1152012001 THEN 40
		WHEN 1152012002 THEN 60
		WHEN 1152012003 THEN 80
	END,
	proceffect = CASE id
		WHEN 1152012000 THEN 24335
		WHEN 1152012001 THEN 24337
		WHEN 1152012002 THEN 24339
		WHEN 1152012003 THEN 33321
	END,
	procname = CASE id
		WHEN 1152012000 THEN 'Remote Healing Surge VI'
		WHEN 1152012001 THEN 'Remote Healing Surge VIII'
		WHEN 1152012002 THEN 'Remote Healing Surge X'
		WHEN 1152012003 THEN 'Remote Healing Surge XI'
	END,
	proctype = 0,
	proclevel = 1,
	proclevel2 = CASE id
		WHEN 1152012000 THEN 20
		WHEN 1152012001 THEN 40
		WHEN 1152012002 THEN 60
		WHEN 1152012003 THEN 80
	END,
	procrate = 100,
	price = CASE id
		WHEN 1152012000 THEN 2500
		WHEN 1152012001 THEN 10000
		WHEN 1152012002 THEN 30000
		WHEN 1152012003 THEN 75000
	END,
	sellrate = 1.000000
WHERE id IN (1152012000, 1152012001, 1152012002, 1152012003);

-- Enable THJ-style multi-augment bow testing on custom Ykeshan bow tier line.
UPDATE items
SET
	augslot3type = 4,
	augslot3visible = 1,
	augslot4type = 4,
	augslot4visible = 1,
	augslot5type = 4,
	augslot5visible = 1
WHERE Name LIKE 'Ykeshan Spirit Bow +%';

-- Add new bow heal augs to existing augment merchants with level gates.
REPLACE INTO merchantlist (
	merchantid,
	slot,
	item,
	faction_required,
	level_required,
	min_status,
	max_status,
	alt_currency_cost,
	classes_required,
	probability,
	bucket_name,
	bucket_value,
	bucket_comparison,
	min_expansion,
	max_expansion,
	content_flags,
	content_flags_disabled
)
VALUES
	(52099, 28, 1152012000, -100, 20, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(52099, 29, 1152012001, -100, 40, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(52099, 30, 1152012002, -100, 60, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(52099, 31, 1152012003, -100, 80, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(382051, 21, 1152012000, -100, 20, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(382051, 22, 1152012001, -100, 40, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(382051, 23, 1152012002, -100, 60, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(382051, 24, 1152012003, -100, 80, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(394174, 21, 1152012000, -100, 20, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(394174, 22, 1152012001, -100, 40, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(394174, 23, 1152012002, -100, 60, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(394174, 24, 1152012003, -100, 80, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL);

COMMIT;
