-- ============================================================================
-- Step 11: Augment Infusion Pool + Infusion Catalysts
-- ============================================================================
-- Creates the Infusion Pool container and 5 tiers of Infusion Catalyst.
-- Adds vendor entries to the Augment Forgemaster (merchant_id 181203).
-- Infusion Catalysts are priced in Common Essence (alt_currency_cost).
--
-- Item IDs:
--   200520 = Infusion Catalyst I   (250 CE)
--   200521 = Infusion Catalyst II  (500 CE)
--   200522 = Infusion Catalyst III (1000 CE)
--   200523 = Infusion Catalyst IV  (2000 CE)
--   200524 = Infusion Catalyst V   (4000 CE)
--   200530 = Infusion Pool         (2-slot container, 1 CE)
-- ============================================================================

-- Infusion Catalyst I
REPLACE INTO items (id, Name, lore, idfile, itemclass, itemtype, icon,
    weight, norent, nodrop, magic, stacksize, stackable, loregroup)
VALUES (200520, 'Infusion Catalyst I', 'A shimmering crystal that can enhance an augment (Tier I)',
    'IT63', 0, 0, 2661, 0, 1, 0, 1, 20, 1, 0);

-- Infusion Catalyst II
REPLACE INTO items (id, Name, lore, idfile, itemclass, itemtype, icon,
    weight, norent, nodrop, magic, stacksize, stackable, loregroup)
VALUES (200521, 'Infusion Catalyst II', 'A pulsing crystal that can further enhance an augment (Tier II)',
    'IT63', 0, 0, 2662, 0, 1, 0, 1, 20, 1, 0);

-- Infusion Catalyst III
REPLACE INTO items (id, Name, lore, idfile, itemclass, itemtype, icon,
    weight, norent, nodrop, magic, stacksize, stackable, loregroup)
VALUES (200522, 'Infusion Catalyst III', 'A radiant crystal charged with arcane power (Tier III)',
    'IT63', 0, 0, 2663, 0, 1, 0, 1, 20, 1, 0);

-- Infusion Catalyst IV
REPLACE INTO items (id, Name, lore, idfile, itemclass, itemtype, icon,
    weight, norent, nodrop, magic, stacksize, stackable, loregroup)
VALUES (200523, 'Infusion Catalyst IV', 'A brilliant crystal surging with power (Tier IV)',
    'IT63', 0, 0, 2664, 0, 1, 0, 1, 20, 1, 0);

-- Infusion Catalyst V
REPLACE INTO items (id, Name, lore, idfile, itemclass, itemtype, icon,
    weight, norent, nodrop, magic, stacksize, stackable, loregroup)
VALUES (200524, 'Infusion Catalyst V', 'A transcendent crystal of ultimate potency (Tier V)',
    'IT63', 0, 0, 2665, 0, 1, 0, 1, 20, 1, 0);

-- Infusion Pool container (2 slots)
REPLACE INTO items (id, Name, lore, idfile, itemclass, itemtype, icon,
    weight, norent, nodrop, magic, bagslots, bagtype, bagsize, bagwr, stacksize)
VALUES (200530, 'Infusion Pool', 'A mystical basin for empowering augments',
    'IT63', 1, 0, 2670, 5, 1, 1, 1, 2, 0, 10, 0, 0);

-- ============================================================================
-- Vendor entries: add to Augment Forgemaster (merchant_id 181203)
-- Existing slots: 0=Lesser Merge Cat, 1=Merge Cat, 2=Augment Forge
-- New slots start at 3
-- ============================================================================

REPLACE INTO merchantlist (merchantid, slot, item, faction_required, level_required, alt_currency_cost, classes_required, probability)
VALUES
(181203, 3, 200530, -100, 0, 1, 65535, 100),      -- Infusion Pool: 1 CE
(181203, 4, 200520, -100, 0, 250, 65535, 100),     -- Infusion Catalyst I: 250 CE
(181203, 5, 200521, -100, 0, 500, 65535, 100),     -- Infusion Catalyst II: 500 CE
(181203, 6, 200522, -100, 0, 1000, 65535, 100),    -- Infusion Catalyst III: 1000 CE
(181203, 7, 200523, -100, 0, 2000, 65535, 100),    -- Infusion Catalyst IV: 2000 CE
(181203, 8, 200524, -100, 0, 4000, 65535, 100);    -- Infusion Catalyst V: 4000 CE
