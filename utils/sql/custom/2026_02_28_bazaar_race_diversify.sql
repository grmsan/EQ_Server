-- Diversify Bazaar NPC races (role-aware) to avoid mostly-human population.
-- Scope: only npc_types that are currently race=Human (1) and are actively spawned in bazaar.
-- Safety: excludes names beginning with '#'.

UPDATE npc_types n
JOIN (
	SELECT DISTINCT se.npcID AS npc_id
	FROM spawn2 s
	JOIN spawnentry se ON se.spawngroupID = s.spawngroupID
	WHERE s.zone = 'bazaar'
) b ON b.npc_id = n.id
SET
	n.race = CASE
		WHEN n.name LIKE 'Banker_%' OR n.name LIKE 'Broker_%' THEN
			CASE MOD(n.id, 4)
				WHEN 0 THEN 12  -- Gnome
				WHEN 1 THEN 8   -- Dwarf
				WHEN 2 THEN 3   -- Erudite
				ELSE 5          -- High Elf
			END
		WHEN n.name LIKE 'Ward_%' OR n.name LIKE 'Defender_%' OR n.name LIKE 'Stable_Ward_%' THEN
			CASE MOD(n.id, 7)
				WHEN 0 THEN 1   -- Human
				WHEN 1 THEN 2   -- Barbarian
				WHEN 2 THEN 4   -- Wood Elf
				WHEN 3 THEN 8   -- Dwarf
				WHEN 4 THEN 9   -- Troll
				WHEN 5 THEN 10  -- Ogre
				ELSE 11         -- Halfling
			END
		WHEN n.name LIKE 'Stable_Hand_%' THEN
			CASE MOD(n.id, 5)
				WHEN 0 THEN 1   -- Human
				WHEN 1 THEN 2   -- Barbarian
				WHEN 2 THEN 8   -- Dwarf
				WHEN 3 THEN 11  -- Halfling
				ELSE 12         -- Gnome
			END
		WHEN n.id = 12000142 THEN 3      -- Bind_Returner: Erudite
		WHEN n.id = 12000189 THEN 5      -- Bazaar_Greeter: High Elf
		WHEN n.id = 1120001127 THEN 12   -- Server_Restorer: Gnome
		WHEN n.name = 'The_Polymorphist' THEN 330 -- Froglok2
		ELSE
			CASE MOD(n.id, 7)
				WHEN 0 THEN 3   -- Erudite
				WHEN 1 THEN 5   -- High Elf
				WHEN 2 THEN 6   -- Dark Elf
				WHEN 3 THEN 7   -- Half Elf
				WHEN 4 THEN 12  -- Gnome
				WHEN 5 THEN 128 -- Iksar
				ELSE 1          -- Human
			END
	END,
	n.gender = CASE
		WHEN n.id IN (12000142, 12000189, 1120001127) THEN 0
		ELSE MOD(n.id, 2)
	END,
	n.texture = CASE
		WHEN n.name LIKE 'Ward_%' OR n.name LIKE 'Defender_%' OR n.name LIKE 'Stable_Ward_%' THEN 2
		WHEN n.name LIKE 'Banker_%' OR n.name LIKE 'Broker_%' OR n.name LIKE 'Merchant_%' OR n.class = 41 THEN 1
		ELSE 0
	END,
	n.helmtexture = 0
WHERE
	n.race = 1
	AND n.name NOT LIKE '#%';

-- Verification
SELECT race, COUNT(*) cnt
FROM npc_types n
JOIN spawnentry se ON se.npcID = n.id
JOIN spawn2 s ON s.spawngroupID = se.spawngroupID
WHERE s.zone = 'bazaar'
GROUP BY race
ORDER BY cnt DESC;
