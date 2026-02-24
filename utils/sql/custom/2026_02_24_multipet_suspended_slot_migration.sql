START TRANSACTION;

CREATE TEMPORARY TABLE legacy_pet_char_ids (
	char_id INT PRIMARY KEY
)
SELECT
	char_id
FROM character_pet_info
GROUP BY char_id
HAVING
	MAX(pet) <= 1
	AND SUM(CASE WHEN pet = 100 THEN 1 ELSE 0 END) = 0;

UPDATE character_pet_info
SET pet = 100
WHERE
	pet = 1
	AND char_id IN (SELECT char_id FROM legacy_pet_char_ids);

UPDATE character_pet_buffs
SET pet = 100
WHERE
	pet = 1
	AND char_id IN (SELECT char_id FROM legacy_pet_char_ids);

UPDATE character_pet_inventory
SET pet = 100
WHERE
	pet = 1
	AND char_id IN (SELECT char_id FROM legacy_pet_char_ids);

DROP TEMPORARY TABLE legacy_pet_char_ids;

COMMIT;
