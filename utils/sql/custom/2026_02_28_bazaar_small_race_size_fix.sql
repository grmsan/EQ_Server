-- Fix Bazaar small-race NPC size scaling to canonical server defaults from GetRaceGenderDefaultHeight:
--   Dwarf (8)    -> 4.0
--   Halfling (11)-> 3.5
--   Gnome (12)   -> 3.0

UPDATE npc_types n
JOIN (
	SELECT DISTINCT se.npcID AS npc_id
	FROM spawn2 s
	JOIN spawnentry se ON se.spawngroupID = s.spawngroupID
	WHERE s.zone = 'bazaar'
) b ON b.npc_id = n.id
SET n.size = CASE n.race
	WHEN 8 THEN 4.0
	WHEN 11 THEN 3.5
	WHEN 12 THEN 3.0
	ELSE n.size
END
WHERE n.race IN (8, 11, 12);

-- Verification
SELECT n.race, n.size, COUNT(*) cnt
FROM npc_types n
JOIN spawnentry se ON se.npcID = n.id
JOIN spawn2 s ON s.spawngroupID = se.spawngroupID
WHERE s.zone = 'bazaar' AND n.race IN (8, 11, 12)
GROUP BY n.race, n.size
ORDER BY n.race, n.size;
