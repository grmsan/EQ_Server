START TRANSACTION;

-- Raise Minion of Eternity so endgame pet focus items select the 35-power pet tiers.
-- This lifts permanent pet lines like Rathe's Son and Child of Bertoxxulous to their
-- existing level-65 focused rows, while dynamic petpower rows continue to scale through code.
UPDATE `spells_new`
SET `effect_base_value1` = 35
WHERE `id` = 4405
	AND `effectid1` = 167;

COMMIT;