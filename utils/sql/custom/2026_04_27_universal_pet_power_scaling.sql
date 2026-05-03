-- =============================================================================
-- Universal Pet Power Scaling
-- 2026-04-27
-- =============================================================================
--
-- PURPOSE
-- -------
-- All summoned pets now respond to pet focus items.  Previously only pet types
-- with an explicit petpower=-1 sentinel row in the pets table (Manifest Elements)
-- received dynamic stat scaling from focus items.  This migration converts every
-- other pet type that has exactly one pets table entry (petpower=0) to use the
-- dynamic-scaling sentinel (petpower=-1), enabling the same formula for all of them.
--
-- HOW THE SYSTEM WORKS
-- --------------------
-- 1. When a pet is summoned, GetPoweredPetEntry() queries:
--      SELECT ... WHERE type='X' AND petpower <= act_power ORDER BY petpower DESC
--    The highest matching petpower row wins.
--
-- 2. If the winning row has petpower = -1 AND act_power > 0, MakePoweredPet() in
--    zone/pets.cpp applies proportional dynamic scaling to the base NPC:
--
--      scale_power  = act_power / 100.0
--      level_bonus  = min(Pets.PetPowerLevelCap, act_power * Pets.PetPowerLevelScale)
--      max_hp      *= (1 + scale_power)
--      AC          *= (1 + scale_power)
--      level       += level_bonus
--      min_dmg     *= (1 + scale_power / 2)
--      max_dmg     *= (1 + scale_power / 2)
--      size         = min(size*3, size * (1 + scale_power / 2))
--
--    Default rules (ruletypes.h):
--      Pets.PetPowerLevelScale = 0.60  (0.6 levels per point of pet power)
--      Pets.PetPowerLevelCap   = 15    (max +15 levels from focus)
--
--    At petpower=35 (endgame focus): +15 levels, +35% HP/AC, +17.5% damage.
--    At petpower=20: +12 levels, +20% HP/AC, +10% damage.
--
-- 3. Pet types with MULTIPLE hand-tuned rows (e.g. SumEarthR16 at 0/15/20/25/30/35/40/45)
--    are NOT affected by this migration.  The explicit rows always win via ORDER BY.
--
-- MANIFEST ELEMENTS SPECIAL CASE (SumMageMultiElement)
-- -------------------------------------------------------
-- Manifest Elements uses a hybrid approach:
--   petpower=-1  -> dynamic scaling for any focus power below 35
--   petpower=35  -> explicit hand-crafted NPC 1120001598 (level 65) for best-in-slot focus
-- This gives smooth progression at intermediate power levels while providing a
-- purpose-built "top tier" NPC for endgame gear.
--
-- The petpower=-1 base row and petpower=35 NPC were set up by the three scripts:
--   2026_04_26_manifest_elements_pet_focus_scaling.sql
--   2026_04_26_manifest_elements_petpower_35_tier.sql
--   2026_04_26_minion_of_eternity_petpower_35.sql
-- This script is idempotent — re-running it when those rows already exist is safe.
--
-- ADDING NEW TIERS IN THE FUTURE
-- --------------------------------
-- To add an explicit tier for any pet type (e.g. SumEarthR16 at petpower=50):
--   1. Create a new npc_types row (copy + tune from the existing top-tier NPC).
--   2. INSERT a pets row: type='SumEarthR16', petpower=50, npcID=<new_id>.
-- Single-tier pets already benefit from smooth dynamic scaling and rarely need
-- explicit tiers unless hand-tuned stats are required.
--
-- =============================================================================

START TRANSACTION;

-- -------------------------------------------------------
-- PART 1 — Manifest Elements sentinel (idempotent)
-- -------------------------------------------------------
-- Ensure SumMageMultiElement has a petpower=-1 base row.
-- If petpower=0 still exists (fresh DB before the Apr-26 scripts), convert it.
-- If petpower=-1 already exists, ON DUPLICATE KEY UPDATE is a no-op.

INSERT INTO `pets` (`type`, `petpower`, `npcID`, `temp`, `petcontrol`, `petnaming`, `monsterflag`, `equipmentset`)
SELECT `type`, -1, `npcID`, `temp`, `petcontrol`, `petnaming`, `monsterflag`, `equipmentset`
FROM `pets`
WHERE `type` = 'SumMageMultiElement'
ORDER BY `petpower` DESC   -- grab the highest existing row as the base template
LIMIT 1
ON DUPLICATE KEY UPDATE `npcID` = VALUES(`npcID`);

-- Remove the petpower=0 row for Manifest Elements if it still exists.
-- (After converting to -1 the 0 row is redundant; the -1 row handles no-focus summons.)
DELETE FROM `pets` WHERE `type` = 'SumMageMultiElement' AND `petpower` = 0;

-- -------------------------------------------------------
-- PART 2 — Bulk conversion: single-tier pets → petpower=-1
-- -------------------------------------------------------
-- Any pet type with exactly ONE pets table entry at petpower=0 is treated as a
-- "base-only" pet that was never given focus tiers.  Changing it to petpower=-1
-- enables dynamic scaling for those pets when the summoner has a focus item,
-- with no other behavioral change when no focus item is equipped (act_power=0
-- exits the scaling block before applying any bonus).

UPDATE `pets` p
JOIN (
    SELECT `type`
    FROM `pets`
    GROUP BY `type`
    HAVING COUNT(*) = 1 AND MAX(`petpower`) = 0
) single_tier USING (`type`)
SET p.`petpower` = -1;

-- -------------------------------------------------------
-- PART 3 — Spot overrides (optional, document exceptions here)
-- -------------------------------------------------------
-- If any pet type converted by Part 2 should remain at petpower=0 (no focus
-- scaling), add an UPDATE here to revert it:
--
--   UPDATE `pets` SET `petpower` = 0 WHERE `type` = 'SomePetType' AND `petpower` = -1;
--
-- Currently there are no known exceptions; all converted types benefit from scaling.

COMMIT;
