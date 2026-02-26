START TRANSACTION;

-- THJ-style class pet bags for pet-capable classes.
-- Template item: 17306 (Glowing Backpack)

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

CALL copy_item_template(17306, 899980);
CALL copy_item_template(17306, 899981);
CALL copy_item_template(17306, 899983);
CALL copy_item_template(17306, 899984);
CALL copy_item_template(17306, 899985);
CALL copy_item_template(17306, 899986);
CALL copy_item_template(17306, 899987);
CALL copy_item_template(17306, 899988);
CALL copy_item_template(17306, 900000);
CALL copy_item_template(17306, 17725);
CALL copy_item_template(17306, 17726);
CALL copy_item_template(17306, 17727);

DROP PROCEDURE IF EXISTS copy_item_template;

UPDATE items
SET
	Name = CASE id
		WHEN 899980 THEN 'Shadowknight Pet Armory'
		WHEN 899981 THEN 'Druid Pet Armory'
		WHEN 899983 THEN 'Bard Pet Armory'
		WHEN 899984 THEN 'Shaman Pet Armory'
		WHEN 899985 THEN 'Necromancer Pet Armory'
		WHEN 899986 THEN 'Magician Pet Armory'
		WHEN 899987 THEN 'Enchanter Pet Armory'
		WHEN 899988 THEN 'Beastlord Pet Armory'
		WHEN 900000 THEN 'Magician Pet Armory (Legacy)'
		WHEN 17725 THEN 'Beastlord Pet Armory (Legacy)'
		WHEN 17726 THEN 'Enchanter Pet Armory (Legacy)'
		WHEN 17727 THEN 'Necromancer Pet Armory (Legacy)'
	END,
	lore = CASE id
		WHEN 899980 THEN 'A satchel attuned to a shadowknight\'s class pet.'
		WHEN 899981 THEN 'A satchel attuned to a druid\'s class pet.'
		WHEN 899983 THEN 'A satchel attuned to a bard\'s class pet.'
		WHEN 899984 THEN 'A satchel attuned to a shaman\'s class pet.'
		WHEN 899985 THEN 'A satchel attuned to a necromancer\'s class pet.'
		WHEN 899986 THEN 'A satchel attuned to a magician\'s class pet.'
		WHEN 899987 THEN 'A satchel attuned to an enchanter\'s class pet.'
		WHEN 899988 THEN 'A satchel attuned to a beastlord\'s class pet.'
		WHEN 900000 THEN 'Legacy satchel attuned to a magician\'s class pet.'
		WHEN 17725 THEN 'Legacy satchel attuned to a beastlord\'s class pet.'
		WHEN 17726 THEN 'Legacy satchel attuned to an enchanter\'s class pet.'
		WHEN 17727 THEN 'Legacy satchel attuned to a necromancer\'s class pet.'
	END,
	itemclass = 1,
	itemtype = 11,
	slots = 0,
	bagslots = 10,
	bagsize = 4,
	bagtype = 0,
	bagwr = 0,
	classes = 65535,
	races = 65535,
	reqlevel = 1,
	reclevel = 1,
	price = 500,
	sellrate = 1.000000,
	nodrop = 0,
	norent = 0,
	loregroup = 0
WHERE id IN (899980,899981,899983,899984,899985,899986,899987,899988,900000,17725,17726,17727);

-- Make the primary pet bag set obtainable from existing progression merchants.
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
	(52099, 200, 899980, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(52099, 201, 899981, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(52099, 202, 899983, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(52099, 203, 899984, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(52099, 204, 899985, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(52099, 205, 899986, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(52099, 206, 899987, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(52099, 207, 899988, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(382051, 200, 899980, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(382051, 201, 899981, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(382051, 202, 899983, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(382051, 203, 899984, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(382051, 204, 899985, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(382051, 205, 899986, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(382051, 206, 899987, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(382051, 207, 899988, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(394174, 200, 899980, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(394174, 201, 899981, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(394174, 202, 899983, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(394174, 203, 899984, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(394174, 204, 899985, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(394174, 205, 899986, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(394174, 206, 899987, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL),
	(394174, 207, 899988, -100, 1, 0, 255, 0, 65535, 100, '', '', 0, -1, -1, NULL, NULL);

COMMIT;
