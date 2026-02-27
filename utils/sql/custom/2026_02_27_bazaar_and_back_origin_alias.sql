-- THJ-style Bazaar and Back alias using Origin AA slot (/alt activate 331)
-- Keeps server-side teleport logic in code, but guarantees client-visible AA slot.

START TRANSACTION;

-- Client-visible strings for Origin rank sid=1000
INSERT INTO db_str (id, type, value) VALUES
	(1000, 1, 'Bazaar and Back'),
	(1000, 2, 'Bazaar Portal'),
	(1000, 4, '(ALL) [/alt activate 331]<br>Upon using this ability, you will be transported to the Bazaar. If you use this ability while already in Bazaar, it will take you back to where you were before entering. /alt activate 331')
ON DUPLICATE KEY UPDATE value = VALUES(value);

-- Align recast/shared timer semantics to Bazaar and Back expectations.
UPDATE aa_ranks
SET recast_time = 60,
	spell_type = 250
WHERE id = 1000;

-- Optional spell naming parity (harmless because server intercepts the AA before spell cast).
UPDATE spells_new
SET name = 'Bazaar Portal',
	player_1 = 'bazaar'
WHERE id = 5824;

-- Ensure every character has rank ownership so activation succeeds server-side.
INSERT INTO character_alternate_abilities (id, aa_id, aa_value, charges)
SELECT cd.id, 1000, 1, 0
FROM character_data cd
LEFT JOIN character_alternate_abilities caa
	ON caa.id = cd.id AND caa.aa_id = 1000
WHERE caa.id IS NULL;

COMMIT;

