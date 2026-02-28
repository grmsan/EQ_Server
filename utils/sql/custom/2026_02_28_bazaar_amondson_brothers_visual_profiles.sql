-- Bazaar Amondson brothers visual differentiation pass
-- Goal:
-- - Keep all Amondson brothers same race/gender baseline (Human male)
-- - Give each brother a distinct look/gear profile
-- Donor inspiration:
-- - Aldo   -> Aid_Eino (id 202122)
-- - Wendal -> Solomen (id 80023)
-- - Galic  -> Guardsman_Dales (id 150195)
-- - Hiklo  -> Muvinn_Vandolan (id 466053)

UPDATE npc_types
SET
	race = 1,
	gender = 0,
	size = 6.0,
	texture = CASE
		WHEN id IN (151005,151205) THEN 23
		WHEN id IN (151006,151206) THEN 16
		WHEN id IN (151007,151207) THEN 3
		WHEN id IN (151008,151208) THEN 19
		ELSE texture
	END,
	helmtexture = 0,
	armtexture = CASE
		WHEN id IN (151005,151205) THEN 23
		WHEN id IN (151006,151206) THEN 16
		WHEN id IN (151007,151207) THEN 3
		WHEN id IN (151008,151208) THEN 19
		ELSE armtexture
	END,
	bracertexture = CASE
		WHEN id IN (151005,151205) THEN 23
		WHEN id IN (151006,151206) THEN 16
		WHEN id IN (151007,151207) THEN 3
		WHEN id IN (151008,151208) THEN 19
		ELSE bracertexture
	END,
	handtexture = CASE
		WHEN id IN (151005,151205) THEN 23
		WHEN id IN (151006,151206) THEN 16
		WHEN id IN (151007,151207) THEN 3
		WHEN id IN (151008,151208) THEN 19
		ELSE handtexture
	END,
	legtexture = CASE
		WHEN id IN (151005,151205) THEN 23
		WHEN id IN (151006,151206) THEN 16
		WHEN id IN (151007,151207) THEN 3
		WHEN id IN (151008,151208) THEN 19
		ELSE legtexture
	END,
	feettexture = CASE
		WHEN id IN (151005,151205) THEN 23
		WHEN id IN (151006,151206) THEN 16
		WHEN id IN (151007,151207) THEN 3
		WHEN id IN (151008,151208) THEN 19
		ELSE feettexture
	END,
	armortint_id = 0,
	armortint_red = CASE
		WHEN id IN (151005,151205) THEN 120
		WHEN id IN (151006,151206) THEN 100
		WHEN id IN (151007,151207) THEN 140
		WHEN id IN (151008,151208) THEN 90
		ELSE armortint_red
	END,
	armortint_green = CASE
		WHEN id IN (151005,151205) THEN 70
		WHEN id IN (151006,151206) THEN 100
		WHEN id IN (151007,151207) THEN 60
		WHEN id IN (151008,151208) THEN 70
		ELSE armortint_green
	END,
	armortint_blue = CASE
		WHEN id IN (151005,151205) THEN 30
		WHEN id IN (151006,151206) THEN 100
		WHEN id IN (151007,151207) THEN 60
		WHEN id IN (151008,151208) THEN 140
		ELSE armortint_blue
	END,
	d_melee_texture1 = CASE
		WHEN id IN (151005,151205) THEN 159
		WHEN id IN (151006,151206) THEN 8
		WHEN id IN (151007,151207) THEN 10000
		WHEN id IN (151008,151208) THEN 301
		ELSE d_melee_texture1
	END,
	d_melee_texture2 = CASE
		WHEN id IN (151005,151205) THEN 159
		WHEN id IN (151006,151206) THEN 0
		WHEN id IN (151007,151207) THEN 11018
		WHEN id IN (151008,151208) THEN 10665
		ELSE d_melee_texture2
	END,
	face = CASE
		WHEN id IN (151005,151205) THEN 1
		WHEN id IN (151006,151206) THEN 1
		WHEN id IN (151007,151207) THEN 2
		WHEN id IN (151008,151208) THEN 0
		ELSE face
	END,
	luclin_hairstyle = 0,
	luclin_haircolor = 0,
	luclin_eyecolor = 0,
	luclin_eyecolor2 = 0,
	luclin_beard = 255,
	luclin_beardcolor = 0,
	drakkin_heritage = 0,
	drakkin_tattoo = 0,
	drakkin_details = 0
WHERE id IN (151005,151006,151007,151008,151205,151206,151207,151208);

-- Verification
SELECT id,name,race,gender,size,texture,helmtexture,armtexture,bracertexture,handtexture,legtexture,feettexture,
	   armortint_red,armortint_green,armortint_blue,d_melee_texture1,d_melee_texture2,face
FROM npc_types
WHERE id IN (151005,151006,151007,151008,151205,151206,151207,151208)
ORDER BY id;
