START TRANSACTION;

-- THJ-style Bazaar and Back AA
-- - Granted to all characters on connect (server-side in zone code)
-- - 60 second cooldown
-- - First use: saves current location and ports to Bazaar
-- - Second use in Bazaar: returns to saved location (instance-aware)

SET @aa_id := 32001;
SET @rank_id := 50001;
SET @spell_id := 65040;
SET @name_sid := 62001;
SET @desc_sid := 62002;

REPLACE INTO db_str (id, type, value) VALUES
	(@name_sid, 1, 'Bazaar and Back'),
	(@name_sid, 2, 'Bazaar and Back'),
	(@name_sid, 3, 'Bazaar and Back'),
	(@desc_sid, 4, 'Teleports you to the Bazaar. Use it again in the Bazaar to return to your saved location (including instances). Recast: 60 seconds.');

REPLACE INTO spells_new (
	id,
	name,
	player_1,
	you_cast,
	other_casts,
	cast_on_you,
	spell_fades,
	`range`,
	cast_time,
	recovery_time,
	recast_time,
	buffdurationformula,
	buffduration,
	mana,
	effectid1,
	effect_base_value1,
	targettype,
	skill,
	goodEffect
) VALUES (
	@spell_id,
	'Bazaar and Back',
	'You bend space around you.',
	'You bend space around you.',
	'bends space around them.',
	'You feel space shift around you.',
	'The spatial shift fades.',
	200,
	0,
	0,
	0,
	0,
	0,
	0,
	254,
	0,
	6,
	98,
	1
);

REPLACE INTO aa_ability (
	id,
	name,
	category,
	classes,
	races,
	drakkin_heritage,
	deities,
	status,
	type,
	charges,
	grant_only,
	first_rank_id,
	enabled,
	reset_on_death,
	auto_grant_enabled
) VALUES (
	@aa_id,
	'Bazaar and Back',
	5,
	65535,
	65535,
	127,
	131071,
	0,
	4,
	0,
	0,
	@rank_id,
	1,
	0,
	1
);

REPLACE INTO aa_ranks (
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
	@rank_id,
	@name_sid,
	@name_sid,
	@name_sid,
	@desc_sid,
	0,
	1,
	@spell_id,
	250,
	60,
	0,
	-1,
	-1
);

COMMIT;
