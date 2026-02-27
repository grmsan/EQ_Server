START TRANSACTION;

-- THJ waypoint content tables (safe if already present)
CREATE TABLE IF NOT EXISTS thj_waypoints_categories (
	id int(11) NOT NULL,
	name varchar(32) NOT NULL,
	PRIMARY KEY (id),
	UNIQUE KEY idx_thj_waypoints_categories_name (name)
);

CREATE TABLE IF NOT EXISTS thj_waypoints (
	id int(11) NOT NULL AUTO_INCREMENT,
	shortname varchar(32) NOT NULL,
	long_name varchar(64) NOT NULL,
	category int(11) NOT NULL,
	x float NOT NULL,
	y float NOT NULL,
	z float NOT NULL,
	heading float NOT NULL,
	PRIMARY KEY (id),
	UNIQUE KEY idx_thj_waypoints_shortname (shortname),
	KEY idx_thj_waypoints_category (category)
);

CREATE TABLE IF NOT EXISTS thj_waypoints_default (
	id int(11) NOT NULL AUTO_INCREMENT,
	waypoint_id int(11) NOT NULL,
	race_id int(11) NOT NULL DEFAULT 0,
	class_mask int(10) unsigned NOT NULL DEFAULT 65535,
	min_level int(11) NOT NULL DEFAULT 1,
	max_level int(11) NOT NULL DEFAULT 255,
	PRIMARY KEY (id),
	KEY idx_thj_waypoints_default_waypoint (waypoint_id),
	KEY idx_thj_waypoints_default_race (race_id),
	KEY idx_thj_waypoints_default_class (class_mask),
	KEY idx_thj_waypoints_default_level (min_level, max_level)
);

CREATE TABLE IF NOT EXISTS thj_waypoints_character (
	id int(11) NOT NULL AUTO_INCREMENT,
	character_id int(11) NOT NULL,
	waypoint_id int(11) NOT NULL,
	PRIMARY KEY (id),
	UNIQUE KEY idx_thj_waypoints_character_unique (character_id, waypoint_id),
	KEY idx_thj_waypoints_character_id (character_id)
);

CREATE TABLE IF NOT EXISTS thj_waypoints_account (
	id int(11) NOT NULL AUTO_INCREMENT,
	account_id int(11) NOT NULL,
	waypoint_id int(11) NOT NULL,
	PRIMARY KEY (id),
	UNIQUE KEY idx_thj_waypoints_account_unique (account_id, waypoint_id),
	KEY idx_thj_waypoints_account_id (account_id)
);

-- Minimal THJ-style category + waypoint seed for fast-travel and discovery flow
REPLACE INTO thj_waypoints_categories (id, name) VALUES
	(0, 'Antonica'),
	(1, 'Faydwer'),
	(2, 'Odus'),
	(3, 'Kunark'),
	(4, 'Velious'),
	(5, 'Luclin'),
	(6, 'Planes'),
	(9, 'Utility');

REPLACE INTO thj_waypoints (id, shortname, long_name, category, x, y, z, heading) VALUES
	(1,  'freportw',    'West Freeport',            0,  -116.00,  -142.00,   -6.00, 140.00),
	(2,  'qeynos2',     'North Qeynos',             0,   370.00,   160.00,    5.00, 180.00),
	(3,  'halas',       'Halas',                    0,     0.00,     0.00,    3.00,   0.00),
	(4,  'erudnext',    'Erudin',                   2,  -240.00, -1216.00,   52.00, 510.00),
	(5,  'gfaydark',    'Greater Faydark',          1,  -511.00,    55.00,    0.00, 128.00),
	(6,  'felwithea',   'Northern Felwithe',        1,   120.00,   280.00,   13.00, 128.00),
	(7,  'neriakb',     'Neriak Commons',           0,   -42.00,   210.00,   -3.00, 384.00),
	(8,  'kaladima',    'South Kaladim',            1,    10.00,   -20.00,    5.00, 128.00),
	(9,  'grobb',       'Grobb',                    0,  -450.00,   100.00,    4.00, 256.00),
	(10, 'oggok',       'Oggok',                    0,   -99.00,   -22.00,    3.00, 128.00),
	(11, 'rivervale',   'Rivervale',                0,  -180.00,  -220.00,    4.00, 256.00),
	(12, 'akanon',      'Ak''anon',                 1,  -761.00,  1279.00,  -24.25, 182.25),
	(13, 'cabeast',     'East Cabilis',             3,    10.00,    10.00,    3.00, 256.00),
	(14, 'sharvahl',    'Shar Vahl',                5,   240.00,    35.00,    3.00, 256.00),
	(15, 'fieldofbone', 'The Field of Bone',        3,  1617.00, -1691.00,  -45.00,  10.00),
	(16, 'dreadlands',  'Dreadlands',               3,  9722.00,  1136.00, 2626.00,   0.00),
	(17, 'iceclad',     'Iceclad Ocean',            4,   350.00,  5300.00,   -5.00, 190.00),
	(18, 'bazaar',      'The Bazaar',               9,    20.00,   -15.00,    0.72, 256.00),
	(19, 'poknowledge', 'Plane of Knowledge',       6,   830.00,   575.00,  -64.00, 128.00),
	(20, 'ecommons',    'East Commonlands',         0, -1700.00,  -300.00,    3.00, 128.00);

DELETE FROM thj_waypoints_default
WHERE race_id = 65535
  AND class_mask = 65535
  AND waypoint_id IN (1,2,3,4,5,6,7,8,9,10,11,12,13,14,18,19,20);

INSERT INTO thj_waypoints_default (waypoint_id, race_id, class_mask, min_level, max_level) VALUES
	(1, 65535, 65535, 1, 255),
	(2, 65535, 65535, 1, 255),
	(3, 65535, 65535, 1, 255),
	(4, 65535, 65535, 1, 255),
	(5, 65535, 65535, 1, 255),
	(6, 65535, 65535, 1, 255),
	(7, 65535, 65535, 1, 255),
	(8, 65535, 65535, 1, 255),
	(9, 65535, 65535, 1, 255),
	(10, 65535, 65535, 1, 255),
	(11, 65535, 65535, 1, 255),
	(12, 65535, 65535, 1, 255),
	(13, 65535, 65535, 1, 255),
	(14, 65535, 65535, 1, 255),
	(18, 65535, 65535, 1, 255),
	(19, 65535, 65535, 1, 255),
	(20, 65535, 65535, 1, 255);

-- Helper for cloning stable bazaar NPC templates
DROP PROCEDURE IF EXISTS copy_npc_template;
DELIMITER //
CREATE PROCEDURE copy_npc_template(IN p_template_id INT, IN p_new_npc_id INT)
BEGIN
	SET @cols := (
		SELECT GROUP_CONCAT(CONCAT('`', COLUMN_NAME, '`') ORDER BY ORDINAL_POSITION SEPARATOR ',')
		FROM INFORMATION_SCHEMA.COLUMNS
		WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'npc_types'
	);

	SET @select_cols := (
		SELECT GROUP_CONCAT(
			CASE
				WHEN COLUMN_NAME = 'id' THEN CAST(p_new_npc_id AS CHAR)
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

-- Core THJ-style Bazaar NPCs
CALL copy_npc_template(151070, 990100); -- Tearel
CALL copy_npc_template(151070, 990101); -- Vision_of_Ayonae
CALL copy_npc_template(151070, 990102); -- A_Fading_Memory
CALL copy_npc_template(151070, 990103); -- Pet merchant

-- Class-add trainer NPCs (class 20..35 consumed by quests/global/global_npc.pl)
CALL copy_npc_template(151070, 990120);
CALL copy_npc_template(151070, 990121);
CALL copy_npc_template(151070, 990122);
CALL copy_npc_template(151070, 990123);
CALL copy_npc_template(151070, 990124);
CALL copy_npc_template(151070, 990125);
CALL copy_npc_template(151070, 990126);
CALL copy_npc_template(151070, 990127);
CALL copy_npc_template(151070, 990128);
CALL copy_npc_template(151070, 990129);
CALL copy_npc_template(151070, 990130);
CALL copy_npc_template(151070, 990131);
CALL copy_npc_template(151070, 990132);
CALL copy_npc_template(151070, 990133);
CALL copy_npc_template(151070, 990134);
CALL copy_npc_template(151070, 990135);
CALL copy_npc_template(151070, 990199); -- #TPTriggerN waypoint discovery trigger

DROP PROCEDURE IF EXISTS copy_npc_template;

UPDATE npc_types
SET
	name = CASE id
		WHEN 990100 THEN 'Tearel'
		WHEN 990101 THEN 'Vision_of_Ayonae'
		WHEN 990102 THEN 'A_Fading_Memory'
		WHEN 990103 THEN 'Pet_Armory_Quartermaster'
		WHEN 990120 THEN 'Warrior_Guildmaster'
		WHEN 990121 THEN 'Cleric_Guildmaster'
		WHEN 990122 THEN 'Paladin_Guildmaster'
		WHEN 990123 THEN 'Ranger_Guildmaster'
		WHEN 990124 THEN 'Shadow_Knight_Guildmaster'
		WHEN 990125 THEN 'Druid_Guildmaster'
		WHEN 990126 THEN 'Monk_Guildmaster'
		WHEN 990127 THEN 'Bard_Guildmaster'
		WHEN 990128 THEN 'Rogue_Guildmaster'
		WHEN 990129 THEN 'Shaman_Guildmaster'
		WHEN 990130 THEN 'Necromancer_Guildmaster'
		WHEN 990131 THEN 'Wizard_Guildmaster'
		WHEN 990132 THEN 'Magician_Guildmaster'
		WHEN 990133 THEN 'Enchanter_Guildmaster'
		WHEN 990134 THEN 'Beastlord_Guildmaster'
		WHEN 990135 THEN 'Berserker_Guildmaster'
		WHEN 990199 THEN '#TPTriggerN'
	END,
	lastname = CASE id
		WHEN 990100 THEN 'Keeper_of_the_Map'
		WHEN 990101 THEN 'Composer_of_Fate'
		WHEN 990102 THEN 'A_Friendly_Face'
		WHEN 990103 THEN 'Pet_Equipment'
		ELSE lastname
	END,
	class = CASE id
		WHEN 990103 THEN 41
		WHEN 990120 THEN 20
		WHEN 990121 THEN 21
		WHEN 990122 THEN 22
		WHEN 990123 THEN 23
		WHEN 990124 THEN 24
		WHEN 990125 THEN 25
		WHEN 990126 THEN 26
		WHEN 990127 THEN 27
		WHEN 990128 THEN 28
		WHEN 990129 THEN 29
		WHEN 990130 THEN 30
		WHEN 990131 THEN 31
		WHEN 990132 THEN 32
		WHEN 990133 THEN 33
		WHEN 990134 THEN 34
		WHEN 990135 THEN 35
		WHEN 990199 THEN 1
		ELSE 1
	END,
	merchant_id = CASE id
		WHEN 990103 THEN 52099
		ELSE 0
	END,
	npc_aggro = 0,
	findable = 1,
	isquest = 1
WHERE id IN (
	990100, 990101, 990102, 990103,
	990120, 990121, 990122, 990123, 990124, 990125, 990126, 990127,
	990128, 990129, 990130, 990131, 990132, 990133, 990134, 990135,
	990199
);

REPLACE INTO spawngroup (id, name) VALUES
	(990100, 'bazaar_tearel'),
	(990101, 'bazaar_vision_of_ayonae'),
	(990102, 'bazaar_a_fading_memory'),
	(990103, 'bazaar_pet_armory_quartermaster'),
	(990120, 'bazaar_warrior_master'),
	(990121, 'bazaar_cleric_master'),
	(990122, 'bazaar_paladin_master'),
	(990123, 'bazaar_ranger_master'),
	(990124, 'bazaar_shadowknight_master'),
	(990125, 'bazaar_druid_master'),
	(990126, 'bazaar_monk_master'),
	(990127, 'bazaar_bard_master'),
	(990128, 'bazaar_rogue_master'),
	(990129, 'bazaar_shaman_master'),
	(990130, 'bazaar_necromancer_master'),
	(990131, 'bazaar_wizard_master'),
	(990132, 'bazaar_magician_master'),
	(990133, 'bazaar_enchanter_master'),
	(990134, 'bazaar_beastlord_master'),
	(990135, 'bazaar_berserker_master');

REPLACE INTO spawnentry (spawngroupID, npcID, chance) VALUES
	(990100, 990100, 100),
	(990101, 990101, 100),
	(990102, 990102, 100),
	(990103, 990103, 100),
	(990120, 990120, 100),
	(990121, 990121, 100),
	(990122, 990122, 100),
	(990123, 990123, 100),
	(990124, 990124, 100),
	(990125, 990125, 100),
	(990126, 990126, 100),
	(990127, 990127, 100),
	(990128, 990128, 100),
	(990129, 990129, 100),
	(990130, 990130, 100),
	(990131, 990131, 100),
	(990132, 990132, 100),
	(990133, 990133, 100),
	(990134, 990134, 100),
	(990135, 990135, 100);

REPLACE INTO spawn2 (id, spawngroupID, zone, version, x, y, z, heading, respawntime, variance) VALUES
	(991100, 990100, 'bazaar', 0,  24.00, -20.00,  0.72, 256.00, 120, 0),
	(991101, 990101, 'bazaar', 0, -24.00, -20.00,  0.72, 256.00, 120, 0),
	(991102, 990102, 'bazaar', 0,   0.00, -20.00,  0.72, 256.00, 120, 0),
	(991103, 990103, 'bazaar', 0,  48.00, -20.00,  0.72, 256.00, 120, 0),
	(991120, 990120, 'bazaar', 0, -72.00,  48.00,  0.72, 128.00, 120, 0),
	(991121, 990121, 'bazaar', 0, -48.00,  48.00,  0.72, 128.00, 120, 0),
	(991122, 990122, 'bazaar', 0, -24.00,  48.00,  0.72, 128.00, 120, 0),
	(991123, 990123, 'bazaar', 0,   0.00,  48.00,  0.72, 128.00, 120, 0),
	(991124, 990124, 'bazaar', 0,  24.00,  48.00,  0.72, 128.00, 120, 0),
	(991125, 990125, 'bazaar', 0,  48.00,  48.00,  0.72, 128.00, 120, 0),
	(991126, 990126, 'bazaar', 0,  72.00,  48.00,  0.72, 128.00, 120, 0),
	(991127, 990127, 'bazaar', 0, -72.00,  72.00,  0.72, 128.00, 120, 0),
	(991128, 990128, 'bazaar', 0, -48.00,  72.00,  0.72, 128.00, 120, 0),
	(991129, 990129, 'bazaar', 0, -24.00,  72.00,  0.72, 128.00, 120, 0),
	(991130, 990130, 'bazaar', 0,   0.00,  72.00,  0.72, 128.00, 120, 0),
	(991131, 990131, 'bazaar', 0,  24.00,  72.00,  0.72, 128.00, 120, 0),
	(991132, 990132, 'bazaar', 0,  48.00,  72.00,  0.72, 128.00, 120, 0),
	(991133, 990133, 'bazaar', 0,  72.00,  72.00,  0.72, 128.00, 120, 0),
	(991134, 990134, 'bazaar', 0, -24.00,  96.00,  0.72, 128.00, 120, 0),
	(991135, 990135, 'bazaar', 0,  24.00,  96.00,  0.72, 128.00, 120, 0);

-- Waypoint discovery proximity triggers used by quests/global/#TPTriggerN.pl
-- Spawn one trigger per waypoint location.
REPLACE INTO spawngroup (id, name)
SELECT
	(993000 + w.id) AS id,
	CONCAT('thj_tp_trigger_', w.shortname, '_', w.id) AS name
FROM thj_waypoints w;

REPLACE INTO spawnentry (spawngroupID, npcID, chance)
SELECT
	(993000 + w.id) AS spawngroupID,
	990199 AS npcID,
	100 AS chance
FROM thj_waypoints w;

REPLACE INTO spawn2 (id, spawngroupID, zone, version, x, y, z, heading, respawntime, variance)
SELECT
	(993000 + w.id) AS id,
	(993000 + w.id) AS spawngroupID,
	w.shortname AS zone,
	0 AS version,
	w.x,
	w.y,
	w.z,
	w.heading,
	120 AS respawntime,
	0 AS variance
FROM thj_waypoints w;

-- Door hook used by quests/bazaar/player.pl (doorid 146 -> SendWaypointList)
REPLACE INTO doors (
	id, doorid, zone, version, name, pos_y, pos_x, pos_z, heading, opentype, size, client_version_mask
)
VALUES
	(990146, 146, 'bazaar', 0, 'PORTAL_DISC', -15.00, 24.00, 0.72, 256.00, 58, 100, 4294967295);

COMMIT;
