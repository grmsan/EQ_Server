-- ============================================================================
-- Warrior AA Spell Repair (Heroic Throw / Colossal Smash)
-- Date: 2026-02-27
--
-- Purpose:
-- 1) Restore/normalize custom warrior spell rows (65000, 65010)
-- 2) Ensure AA rows reference the expected spell IDs/timers
-- 3) Repair db_str text SID wiring for Heroic Throw
--
-- IMPORTANT:
-- After applying this SQL, rebuild shared memory spells and restart world/zone:
--   shared_memory.exe spells
-- ============================================================================

START TRANSACTION;

INSERT INTO spells_new (
	id,
	name,
	player_1,
	you_cast,
	other_casts,
	cast_on_other,
	cast_time,
	buffdurationformula,
	buffduration,
	`range`,
	targettype,
	skill,
	effectid1,
	effect_base_value1,
	effect_limit_value1,
	effectid2,
	effect_base_value2,
	effect_limit_value2,
	effectid3,
	effect_base_value3,
	effect_limit_value3,
	goodEffect,
	ResistDiff,
	TargetAnim
) VALUES (
	65000,
	'Heroic Throw',
	'You hurl your weapon!',
	'You hurl your weapon!',
	'hurls their weapon!',
	'are struck by a hurled weapon!',
	0,
	0,
	60,
	200,
	5,
	51,
	79,
	-500,
	0,
	1,
	-5,
	0,
	11,
	-50,
	0,
	0,
	-150,
	45
)
ON DUPLICATE KEY UPDATE
	name = VALUES(name),
	player_1 = VALUES(player_1),
	you_cast = VALUES(you_cast),
	other_casts = VALUES(other_casts),
	cast_on_other = VALUES(cast_on_other),
	cast_time = VALUES(cast_time),
	buffdurationformula = VALUES(buffdurationformula),
	buffduration = VALUES(buffduration),
	`range` = VALUES(`range`),
	targettype = VALUES(targettype),
	skill = VALUES(skill),
	effectid1 = VALUES(effectid1),
	effect_base_value1 = VALUES(effect_base_value1),
	effect_limit_value1 = VALUES(effect_limit_value1),
	effectid2 = VALUES(effectid2),
	effect_base_value2 = VALUES(effect_base_value2),
	effect_limit_value2 = VALUES(effect_limit_value2),
	effectid3 = VALUES(effectid3),
	effect_base_value3 = VALUES(effect_base_value3),
	effect_limit_value3 = VALUES(effect_limit_value3),
	goodEffect = VALUES(goodEffect),
	ResistDiff = VALUES(ResistDiff),
	TargetAnim = VALUES(TargetAnim);

INSERT INTO spells_new (
	id,
	name,
	player_1,
	you_cast,
	other_casts,
	cast_on_other,
	cast_time,
	buffdurationformula,
	buffduration,
	`range`,
	targettype,
	skill,
	effectid1,
	effect_base_value1,
	effect_limit_value1,
	effectid2,
	effect_base_value2,
	effect_limit_value2,
	goodEffect,
	ResistDiff,
	TargetAnim
) VALUES (
	65010,
	'Colossal Smash',
	'You unleash a colossal smash!',
	'You unleash a colossal smash!',
	'unleashes a colossal smash!',
	'are smashed by a colossal blow!',
	0,
	0,
	60,
	50,
	5,
	51,
	79,
	-500,
	0,
	1,
	-10,
	0,
	0,
	-150,
	45
)
ON DUPLICATE KEY UPDATE
	name = VALUES(name),
	player_1 = VALUES(player_1),
	you_cast = VALUES(you_cast),
	other_casts = VALUES(other_casts),
	cast_on_other = VALUES(cast_on_other),
	cast_time = VALUES(cast_time),
	buffdurationformula = VALUES(buffdurationformula),
	buffduration = VALUES(buffduration),
	`range` = VALUES(`range`),
	targettype = VALUES(targettype),
	skill = VALUES(skill),
	effectid1 = VALUES(effectid1),
	effect_base_value1 = VALUES(effect_base_value1),
	effect_limit_value1 = VALUES(effect_limit_value1),
	effectid2 = VALUES(effectid2),
	effect_base_value2 = VALUES(effect_base_value2),
	effect_limit_value2 = VALUES(effect_limit_value2),
	goodEffect = VALUES(goodEffect),
	ResistDiff = VALUES(ResistDiff),
	TargetAnim = VALUES(TargetAnim);

INSERT INTO db_str (id, type, value) VALUES
	(10000, 1, 'Heroic Throw'),
	(10000, 2, 'Heroic Throw'),
	(10000, 3, 'Heroic Throw'),
	(10000, 4, 'Hurl your weapon at a distant enemy, dealing melee weapon damage. 10 second recast.'),
	(10010, 1, 'Colossal Smash'),
	(10011, 4, 'A guaranteed crippling strike using your equipped weapons. Damage scales with strength and both weapons. Cooldown decreases with each rank.')
ON DUPLICATE KEY UPDATE value = VALUES(value);

INSERT INTO aa_ability (
	id,
	name,
	category,
	classes,
	type,
	charges,
	grant_only,
	first_rank_id,
	enabled
) VALUES
	(10000, 'Heroic Throw', -1, 1, 3, 0, 0, 10000, 1),
	(12000, 'Colossal Smash', -1, 1, 3, 0, 0, 12000, 1)
ON DUPLICATE KEY UPDATE
	name = VALUES(name),
	category = VALUES(category),
	classes = VALUES(classes),
	type = VALUES(type),
	charges = VALUES(charges),
	grant_only = VALUES(grant_only),
	first_rank_id = VALUES(first_rank_id),
	enabled = VALUES(enabled);

INSERT INTO aa_ranks (
	id,
	upper_hotkey_sid,
	lower_hotkey_sid,
	title_sid,
	desc_sid,
	cost,
	level_req,
	spell,
	spell_type,
	recast_time,
	expansion,
	prev_id,
	next_id
) VALUES (
	10000,
	10000,
	-1,
	10000,
	10000,
	0,
	5,
	65000,
	20,
	10,
	0,
	0,
	0
)
ON DUPLICATE KEY UPDATE
	upper_hotkey_sid = VALUES(upper_hotkey_sid),
	lower_hotkey_sid = VALUES(lower_hotkey_sid),
	title_sid = VALUES(title_sid),
	desc_sid = VALUES(desc_sid),
	cost = VALUES(cost),
	level_req = VALUES(level_req),
	spell = VALUES(spell),
	spell_type = VALUES(spell_type),
	recast_time = VALUES(recast_time),
	expansion = VALUES(expansion),
	prev_id = VALUES(prev_id),
	next_id = VALUES(next_id);

UPDATE aa_ranks
SET
	spell = 65010,
	spell_type = 21
WHERE id BETWEEN 12000 AND 12008;

COMMIT;

