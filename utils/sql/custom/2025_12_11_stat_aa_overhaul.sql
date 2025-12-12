-- Custom stat AA overhaul for Planar Power and Innate Enlightenment
-- - Make ranks cost 0
-- - Make ranks unlock every 5 levels (starting at level 5)
-- - Multiply max stat cap bonuses by 100 (e.g., +5 -> +500)

-- PLANAR POWER (aa_ability.id = 142, ranks 418..422,1001..1005,4678..4682,7547..7551)

-- Planar Power: cost 0 and level requirements every 5 levels
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 5   WHERE `id` = 418;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 10  WHERE `id` = 419;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 15  WHERE `id` = 420;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 20  WHERE `id` = 421;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 25  WHERE `id` = 422;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 30  WHERE `id` = 1001;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 35  WHERE `id` = 1002;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 40  WHERE `id` = 1003;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 45  WHERE `id` = 1004;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 50  WHERE `id` = 1005;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 55  WHERE `id` = 4678;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 60  WHERE `id` = 4679;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 65  WHERE `id` = 4680;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 70  WHERE `id` = 4681;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 75  WHERE `id` = 4682;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 80  WHERE `id` = 7547;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 85  WHERE `id` = 7548;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 90  WHERE `id` = 7549;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 95  WHERE `id` = 7550;
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 100 WHERE `id` = 7551;

-- Planar Power: raise stat caps 100x (base1 * 100)
UPDATE `aa_rank_effects` SET `base1` =  500  WHERE `rank_id` = 418;
UPDATE `aa_rank_effects` SET `base1` = 1000  WHERE `rank_id` = 419;
UPDATE `aa_rank_effects` SET `base1` = 1500  WHERE `rank_id` = 420;
UPDATE `aa_rank_effects` SET `base1` = 2000  WHERE `rank_id` = 421;
UPDATE `aa_rank_effects` SET `base1` = 2500  WHERE `rank_id` = 422;
UPDATE `aa_rank_effects` SET `base1` = 3000  WHERE `rank_id` = 1001;
UPDATE `aa_rank_effects` SET `base1` = 3500  WHERE `rank_id` = 1002;
UPDATE `aa_rank_effects` SET `base1` = 4000  WHERE `rank_id` = 1003;
UPDATE `aa_rank_effects` SET `base1` = 4500  WHERE `rank_id` = 1004;
UPDATE `aa_rank_effects` SET `base1` = 5000  WHERE `rank_id` = 1005;
UPDATE `aa_rank_effects` SET `base1` = 5500  WHERE `rank_id` = 4678;
UPDATE `aa_rank_effects` SET `base1` = 6000  WHERE `rank_id` = 4679;
UPDATE `aa_rank_effects` SET `base1` = 6500  WHERE `rank_id` = 4680;
UPDATE `aa_rank_effects` SET `base1` = 7000  WHERE `rank_id` = 4681;
UPDATE `aa_rank_effects` SET `base1` = 7500  WHERE `rank_id` = 4682;
UPDATE `aa_rank_effects` SET `base1` = 8000  WHERE `rank_id` = 7547;
UPDATE `aa_rank_effects` SET `base1` = 8500  WHERE `rank_id` = 7548;
UPDATE `aa_rank_effects` SET `base1` = 9000  WHERE `rank_id` = 7549;
UPDATE `aa_rank_effects` SET `base1` = 9500  WHERE `rank_id` = 7550;
UPDATE `aa_rank_effects` SET `base1` = 10000 WHERE `rank_id` = 7551;

-- INNATE ENLIGHTENMENT
-- Two abilities: id 144 (ranks 426..430) and id 316 (ranks 955..959)

-- Innate Enlightenment: cost 0 and level requirements every 5 levels
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 5  WHERE `id` IN (426, 955);
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 10 WHERE `id` IN (427, 956);
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 15 WHERE `id` IN (428, 957);
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 20 WHERE `id` IN (429, 958);
UPDATE `aa_ranks` SET `cost` = 0, `level_req` = 25 WHERE `id` IN (430, 959);

-- Innate Enlightenment: raise INT/WIS caps 100x (base1 * 100)
UPDATE `aa_rank_effects` SET `base1` = 1000 WHERE `rank_id` IN (426, 955);
UPDATE `aa_rank_effects` SET `base1` = 2000 WHERE `rank_id` IN (427, 956);
UPDATE `aa_rank_effects` SET `base1` = 3000 WHERE `rank_id` IN (428, 957);
UPDATE `aa_rank_effects` SET `base1` = 4000 WHERE `rank_id` IN (429, 958);
UPDATE `aa_rank_effects` SET `base1` = 5000 WHERE `rank_id` IN (430, 959);

