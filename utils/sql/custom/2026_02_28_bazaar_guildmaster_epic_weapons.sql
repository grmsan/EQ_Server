-- Assign class-appropriate epic weapon models to Bazaar guildmasters (IDs 990120..990135)
-- Uses npc_types.d_melee_texture1/2 display models (idfile numeric portion).

UPDATE npc_types
SET
	d_melee_texture1 = CASE id
		WHEN 990120 THEN 146   -- Warrior: Jagged Blade of War
		WHEN 990121 THEN 156   -- Cleric: Water Sprinkler of Nem Ankh
		WHEN 990122 THEN 160   -- Paladin: Fiery Defender
		WHEN 990123 THEN 149   -- Ranger: Earthcaller / Swiftwind
		WHEN 990124 THEN 145   -- Shadow Knight: Innoruuk's Curse
		WHEN 990125 THEN 150   -- Druid: Nature Walkers Scimitar
		WHEN 990126 THEN 159   -- Monk: Celestial Fists
		WHEN 990127 THEN 148   -- Bard: Singing Short Sword
		WHEN 990128 THEN 140   -- Rogue: Ragebringer
		WHEN 990129 THEN 154   -- Shaman: Spear of Fate
		WHEN 990130 THEN 153   -- Necromancer: Scythe of the Shadowed Soul
		WHEN 990131 THEN 155   -- Wizard: Staff of the Four
		WHEN 990132 THEN 151   -- Magician: Orb of Mastery
		WHEN 990133 THEN 157   -- Enchanter: Staff of the Serpent
		WHEN 990134 THEN 10029 -- Beastlord: Claw of the Savage Spirit (Primary)
		WHEN 990135 THEN 10727 -- Berserker: Kerasian Axe of Ire
		ELSE d_melee_texture1
	END,
	d_melee_texture2 = CASE id
		WHEN 990123 THEN 149   -- Ranger offhand
		WHEN 990126 THEN 159   -- Monk offhand
		WHEN 990128 THEN 140   -- Rogue offhand
		WHEN 990134 THEN 10015 -- Beastlord: Claw of the Savage Spirit (Offhand)
		ELSE 0
	END
WHERE id BETWEEN 990120 AND 990135;

-- Verification query
SELECT id, name, class, d_melee_texture1, d_melee_texture2
FROM npc_types
WHERE id BETWEEN 990120 AND 990135
ORDER BY id;
