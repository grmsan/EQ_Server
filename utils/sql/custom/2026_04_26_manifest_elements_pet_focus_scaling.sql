START TRANSACTION;

-- THJ-style pet focus scaling expects Manifest Elements to resolve to a scaling row.
-- With petpower stuck at 0, focusPetPower is read but never changes the pet entry.
REPLACE INTO `pets` (
	`id`,
	`type`,
	`petpower`,
	`npcID`,
	`temp`,
	`petcontrol`,
	`petnaming`,
	`monsterflag`,
	`equipmentset`
)
SELECT
	`id`,
	`type`,
	-1,
	`npcID`,
	`temp`,
	`petcontrol`,
	`petnaming`,
	`monsterflag`,
	`equipmentset`
FROM `pets`
WHERE `type` = 'SumMageMultiElement'
ORDER BY CASE WHEN `petpower` = 0 THEN 0 ELSE 1 END, `petpower` DESC
LIMIT 1;

COMMIT;