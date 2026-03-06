-- ============================================================
-- Step 4: Essence Currency + Salvage System
-- Run this after Steps 1-3 migrations and generate_tiered_items.py
-- ============================================================

-- ============================================================
-- 1. Currency Token Items (used for icons in the Alternate Currency tab)
--    These items exist only so the client has an icon to display.
--    Players never hold them directly.
-- ============================================================

-- Common Essence token (icon 3000 = small blue gem)
INSERT INTO `items` (`id`, `Name`, `itemclass`, `weight`, `norent`, `nodrop`, `magic`, `icon`, `lore`, `idfile`)
VALUES (200010, 'Common Essence', 0, 0, 1, 1, 1, 3000, 'Common Essence', 'IT63')
ON DUPLICATE KEY UPDATE `Name` = 'Common Essence', `icon` = 3000;

-- Rare Essence token (icon 3020 = small red gem)
INSERT INTO `items` (`id`, `Name`, `itemclass`, `weight`, `norent`, `nodrop`, `magic`, `icon`, `lore`, `idfile`)
VALUES (200011, 'Rare Essence', 0, 0, 1, 1, 1, 3020, 'Rare Essence', 'IT63')
ON DUPLICATE KEY UPDATE `Name` = 'Rare Essence', `icon` = 3020;

-- ============================================================
-- 2. Alternate Currency Registration
--    ID 100 = Common Essence, ID 101 = Rare Essence
--    These IDs match RuleI(ItemProgression, CommonEssenceCurrencyID/RareEssenceCurrencyID)
-- ============================================================

INSERT INTO `alternate_currency` (`id`, `item_id`) VALUES (100, 200010)
ON DUPLICATE KEY UPDATE `item_id` = 200010;

INSERT INTO `alternate_currency` (`id`, `item_id`) VALUES (101, 200011)
ON DUPLICATE KEY UPDATE `item_id` = 200011;

-- ============================================================
-- 3. Salvage Satchel — 20-slot tradeskill container (ALL/ALL, NODROP, NORENT=0 so it persists)
--    ID matches RuleI(ItemProgression, SalvageSatchelItemID) = 200020
--    itemclass = 1 (bag), bagtype = 10 (BagTypeToolBox — shows Combine button),
--    bagslots = 20, bagsize = 10 (GIANT)
--    classes = 65535 (ALL), races = 65535 (ALL), slots = 0 (no equip slot — bag only)
--    When the player clicks Combine, HandleCombine intercepts by item ID and runs
--    the salvage logic instead of a tradeskill recipe check.
-- ============================================================

INSERT INTO `items` (
    `id`, `Name`, `itemclass`, `weight`, `norent`, `nodrop`, `magic`,
    `icon`, `lore`, `idfile`, `bagtype`, `bagslots`, `bagsize`, `bagwr`,
    `classes`, `races`, `slots`
) VALUES (
    200020, 'Salvage Satchel', 1, 0, 0, 1, 1,
    677, 'Salvage Satchel', 'IT63', 10, 20, 10, 100,
    65535, 65535, 0
)
ON DUPLICATE KEY UPDATE
    `Name` = 'Salvage Satchel',
    `itemclass` = 1,
    `bagtype` = 10,
    `bagslots` = 20,
    `bagsize` = 10,
    `bagwr` = 100;

-- ============================================================
-- 4. Rule Values (optional — inserts defaults if not already present)
--    These match the RULE_INT/RULE_REAL defaults in ruletypes.h.
--    Only needed if you want to override defaults without recompiling.
-- ============================================================

-- INSERT IGNORE INTO `rule_values` VALUES
--     (1, 'ItemProgression:CommonEssenceCurrencyID', '100', 'Alternate currency ID for Common Essence'),
--     (1, 'ItemProgression:RareEssenceCurrencyID', '101', 'Alternate currency ID for Rare Essence'),
--     (1, 'ItemProgression:SalvageSatchelItemID', '200020', 'Item ID of the Salvage Satchel container'),
--     (1, 'ItemProgression:SalvageRareChanceNormal', '0.02', 'Chance of Rare Essence from normal items'),
--     (1, 'ItemProgression:SalvageRareChanceNamed', '0.10', 'Chance of Rare Essence from named/raid items'),
--     (1, 'ItemProgression:SalvageRareAmountMin', '1', 'Min Rare Essence on proc'),
--     (1, 'ItemProgression:SalvageRareAmountMax', '3', 'Max Rare Essence on proc');

-- ============================================================
-- Done! After running this:
--   1. Restart shared_memory (to reload items)
--   2. Restart zone (to reload alternate currencies and rules)
--   3. #summonitem 200020 — get a Salvage Satchel
--   4. Place magic items inside, then click "Combine" (or #salvage for GM testing)
--   5. Check Essence balance in the Alternate Currency tab on your character sheet
-- ============================================================
