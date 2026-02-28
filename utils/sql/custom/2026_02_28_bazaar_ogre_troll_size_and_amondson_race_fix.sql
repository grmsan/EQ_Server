-- Bazaar targeted fixes:
-- 1) Restore Troll/Ogre scale to canonical defaults
-- 2) Keep Amondson "brothers" as one consistent race

-- 1) Troll/Ogre size defaults in Bazaar
UPDATE npc_types n
JOIN (
	SELECT DISTINCT se.npcID AS npc_id
	FROM spawn2 s
	JOIN spawnentry se ON se.spawngroupID = s.spawngroupID
	WHERE s.zone = 'bazaar'
) b ON b.npc_id = n.id
SET n.size = CASE n.race
	WHEN 9 THEN 8.0   -- Troll
	WHEN 10 THEN 9.0  -- Ogre
	ELSE n.size
END
WHERE n.race IN (9,10);

-- 2) Amondson brothers consistency
UPDATE npc_types n
JOIN (
	SELECT DISTINCT se.npcID AS npc_id
	FROM spawn2 s
	JOIN spawnentry se ON se.spawngroupID = s.spawngroupID
	WHERE s.zone = 'bazaar'
) b ON b.npc_id = n.id
SET
	n.race = 1,   -- Human
	n.gender = 0, -- Male
	n.size = 6.0
WHERE n.name LIKE '%Amondson%';

-- Verification queries
SELECT DISTINCT n.id,n.name,n.race,n.gender,n.size
FROM npc_types n
JOIN spawnentry se ON se.npcID=n.id
JOIN spawn2 s ON s.spawngroupID=se.spawngroupID
WHERE s.zone='bazaar' AND n.race IN (9,10)
ORDER BY n.race,n.id;

SELECT DISTINCT n.id,n.name,n.race,n.gender,n.size
FROM npc_types n
JOIN spawnentry se ON se.npcID=n.id
JOIN spawn2 s ON s.spawngroupID=se.spawngroupID
WHERE s.zone='bazaar' AND n.name LIKE '%Amondson%'
ORDER BY n.id;
