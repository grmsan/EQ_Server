START TRANSACTION;

-- ---------------------------------------------------------------------------
-- Weapon Augment Expansion
-- ---------------------------------------------------------------------------
-- 1H weapons (slash/pierce/blunt/hand-to-hand) -> ensure at least 1 type-4 slot.
-- 2H weapons + bows -> ensure at least 2 type-4 slots.
-- Uses first available empty aug slot(s) without overwriting existing aug types.

-- 1H target: >= 1 type-4 slot
UPDATE `items`
SET `augslot1type` = 4, `augslot1visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` IN (0, 2, 3, 45)
  AND IFNULL(`augslot1type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 4) + (IFNULL(`augslot2type`, 0) = 4) + (IFNULL(`augslot3type`, 0) = 4) + (IFNULL(`augslot4type`, 0) = 4) + (IFNULL(`augslot5type`, 0) = 4) + (IFNULL(`augslot6type`, 0) = 4)) < 1;

UPDATE `items`
SET `augslot2type` = 4, `augslot2visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` IN (0, 2, 3, 45)
  AND IFNULL(`augslot2type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 4) + (IFNULL(`augslot2type`, 0) = 4) + (IFNULL(`augslot3type`, 0) = 4) + (IFNULL(`augslot4type`, 0) = 4) + (IFNULL(`augslot5type`, 0) = 4) + (IFNULL(`augslot6type`, 0) = 4)) < 1;

UPDATE `items`
SET `augslot3type` = 4, `augslot3visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` IN (0, 2, 3, 45)
  AND IFNULL(`augslot3type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 4) + (IFNULL(`augslot2type`, 0) = 4) + (IFNULL(`augslot3type`, 0) = 4) + (IFNULL(`augslot4type`, 0) = 4) + (IFNULL(`augslot5type`, 0) = 4) + (IFNULL(`augslot6type`, 0) = 4)) < 1;

UPDATE `items`
SET `augslot4type` = 4, `augslot4visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` IN (0, 2, 3, 45)
  AND IFNULL(`augslot4type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 4) + (IFNULL(`augslot2type`, 0) = 4) + (IFNULL(`augslot3type`, 0) = 4) + (IFNULL(`augslot4type`, 0) = 4) + (IFNULL(`augslot5type`, 0) = 4) + (IFNULL(`augslot6type`, 0) = 4)) < 1;

UPDATE `items`
SET `augslot5type` = 4, `augslot5visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` IN (0, 2, 3, 45)
  AND IFNULL(`augslot5type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 4) + (IFNULL(`augslot2type`, 0) = 4) + (IFNULL(`augslot3type`, 0) = 4) + (IFNULL(`augslot4type`, 0) = 4) + (IFNULL(`augslot5type`, 0) = 4) + (IFNULL(`augslot6type`, 0) = 4)) < 1;

UPDATE `items`
SET `augslot6type` = 4, `augslot6visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` IN (0, 2, 3, 45)
  AND IFNULL(`augslot6type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 4) + (IFNULL(`augslot2type`, 0) = 4) + (IFNULL(`augslot3type`, 0) = 4) + (IFNULL(`augslot4type`, 0) = 4) + (IFNULL(`augslot5type`, 0) = 4) + (IFNULL(`augslot6type`, 0) = 4)) < 1;

-- 2H + Bow target: >= 2 type-4 slots
UPDATE `items`
SET `augslot1type` = 4, `augslot1visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` IN (1, 4, 5, 35)
  AND IFNULL(`augslot1type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 4) + (IFNULL(`augslot2type`, 0) = 4) + (IFNULL(`augslot3type`, 0) = 4) + (IFNULL(`augslot4type`, 0) = 4) + (IFNULL(`augslot5type`, 0) = 4) + (IFNULL(`augslot6type`, 0) = 4)) < 2;

UPDATE `items`
SET `augslot2type` = 4, `augslot2visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` IN (1, 4, 5, 35)
  AND IFNULL(`augslot2type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 4) + (IFNULL(`augslot2type`, 0) = 4) + (IFNULL(`augslot3type`, 0) = 4) + (IFNULL(`augslot4type`, 0) = 4) + (IFNULL(`augslot5type`, 0) = 4) + (IFNULL(`augslot6type`, 0) = 4)) < 2;

UPDATE `items`
SET `augslot3type` = 4, `augslot3visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` IN (1, 4, 5, 35)
  AND IFNULL(`augslot3type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 4) + (IFNULL(`augslot2type`, 0) = 4) + (IFNULL(`augslot3type`, 0) = 4) + (IFNULL(`augslot4type`, 0) = 4) + (IFNULL(`augslot5type`, 0) = 4) + (IFNULL(`augslot6type`, 0) = 4)) < 2;

UPDATE `items`
SET `augslot4type` = 4, `augslot4visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` IN (1, 4, 5, 35)
  AND IFNULL(`augslot4type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 4) + (IFNULL(`augslot2type`, 0) = 4) + (IFNULL(`augslot3type`, 0) = 4) + (IFNULL(`augslot4type`, 0) = 4) + (IFNULL(`augslot5type`, 0) = 4) + (IFNULL(`augslot6type`, 0) = 4)) < 2;

UPDATE `items`
SET `augslot5type` = 4, `augslot5visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` IN (1, 4, 5, 35)
  AND IFNULL(`augslot5type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 4) + (IFNULL(`augslot2type`, 0) = 4) + (IFNULL(`augslot3type`, 0) = 4) + (IFNULL(`augslot4type`, 0) = 4) + (IFNULL(`augslot5type`, 0) = 4) + (IFNULL(`augslot6type`, 0) = 4)) < 2;

UPDATE `items`
SET `augslot6type` = 4, `augslot6visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` IN (1, 4, 5, 35)
  AND IFNULL(`augslot6type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 4) + (IFNULL(`augslot2type`, 0) = 4) + (IFNULL(`augslot3type`, 0) = 4) + (IFNULL(`augslot4type`, 0) = 4) + (IFNULL(`augslot5type`, 0) = 4) + (IFNULL(`augslot6type`, 0) = 4)) < 2;

-- ---------------------------------------------------------------------------
-- General Gear Expansion
-- ---------------------------------------------------------------------------
-- Give stat-bearing equippable items at least one type-7 slot where possible.
-- This is intentionally broad to support customization hunts.

UPDATE `items`
SET `augslot1type` = 7, `augslot1visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` <> 54
  AND `slots` > 0
  AND `classes` > 0
  AND `races` > 0
  AND IFNULL(`augslot1type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 7) + (IFNULL(`augslot2type`, 0) = 7) + (IFNULL(`augslot3type`, 0) = 7) + (IFNULL(`augslot4type`, 0) = 7) + (IFNULL(`augslot5type`, 0) = 7) + (IFNULL(`augslot6type`, 0) = 7)) < 1
  AND (`ac` > 0 OR `hp` > 0 OR `mana` > 0 OR `endur` > 0 OR `damage` > 0 OR `attack` > 0 OR `haste` > 0 OR `aagi` > 0 OR `acha` > 0 OR `adex` > 0 OR `aint` > 0 OR `asta` > 0 OR `astr` > 0 OR `awis` > 0 OR `heroic_str` > 0 OR `heroic_int` > 0 OR `heroic_wis` > 0 OR `heroic_agi` > 0 OR `heroic_dex` > 0 OR `heroic_sta` > 0 OR `heroic_cha` > 0 OR `healamt` > 0 OR `spelldmg` > 0 OR `clairvoyance` > 0);

UPDATE `items`
SET `augslot2type` = 7, `augslot2visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` <> 54
  AND `slots` > 0
  AND `classes` > 0
  AND `races` > 0
  AND IFNULL(`augslot2type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 7) + (IFNULL(`augslot2type`, 0) = 7) + (IFNULL(`augslot3type`, 0) = 7) + (IFNULL(`augslot4type`, 0) = 7) + (IFNULL(`augslot5type`, 0) = 7) + (IFNULL(`augslot6type`, 0) = 7)) < 1
  AND (`ac` > 0 OR `hp` > 0 OR `mana` > 0 OR `endur` > 0 OR `damage` > 0 OR `attack` > 0 OR `haste` > 0 OR `aagi` > 0 OR `acha` > 0 OR `adex` > 0 OR `aint` > 0 OR `asta` > 0 OR `astr` > 0 OR `awis` > 0 OR `heroic_str` > 0 OR `heroic_int` > 0 OR `heroic_wis` > 0 OR `heroic_agi` > 0 OR `heroic_dex` > 0 OR `heroic_sta` > 0 OR `heroic_cha` > 0 OR `healamt` > 0 OR `spelldmg` > 0 OR `clairvoyance` > 0);

UPDATE `items`
SET `augslot3type` = 7, `augslot3visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` <> 54
  AND `slots` > 0
  AND `classes` > 0
  AND `races` > 0
  AND IFNULL(`augslot3type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 7) + (IFNULL(`augslot2type`, 0) = 7) + (IFNULL(`augslot3type`, 0) = 7) + (IFNULL(`augslot4type`, 0) = 7) + (IFNULL(`augslot5type`, 0) = 7) + (IFNULL(`augslot6type`, 0) = 7)) < 1
  AND (`ac` > 0 OR `hp` > 0 OR `mana` > 0 OR `endur` > 0 OR `damage` > 0 OR `attack` > 0 OR `haste` > 0 OR `aagi` > 0 OR `acha` > 0 OR `adex` > 0 OR `aint` > 0 OR `asta` > 0 OR `astr` > 0 OR `awis` > 0 OR `heroic_str` > 0 OR `heroic_int` > 0 OR `heroic_wis` > 0 OR `heroic_agi` > 0 OR `heroic_dex` > 0 OR `heroic_sta` > 0 OR `heroic_cha` > 0 OR `healamt` > 0 OR `spelldmg` > 0 OR `clairvoyance` > 0);

UPDATE `items`
SET `augslot4type` = 7, `augslot4visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` <> 54
  AND `slots` > 0
  AND `classes` > 0
  AND `races` > 0
  AND IFNULL(`augslot4type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 7) + (IFNULL(`augslot2type`, 0) = 7) + (IFNULL(`augslot3type`, 0) = 7) + (IFNULL(`augslot4type`, 0) = 7) + (IFNULL(`augslot5type`, 0) = 7) + (IFNULL(`augslot6type`, 0) = 7)) < 1
  AND (`ac` > 0 OR `hp` > 0 OR `mana` > 0 OR `endur` > 0 OR `damage` > 0 OR `attack` > 0 OR `haste` > 0 OR `aagi` > 0 OR `acha` > 0 OR `adex` > 0 OR `aint` > 0 OR `asta` > 0 OR `astr` > 0 OR `awis` > 0 OR `heroic_str` > 0 OR `heroic_int` > 0 OR `heroic_wis` > 0 OR `heroic_agi` > 0 OR `heroic_dex` > 0 OR `heroic_sta` > 0 OR `heroic_cha` > 0 OR `healamt` > 0 OR `spelldmg` > 0 OR `clairvoyance` > 0);

UPDATE `items`
SET `augslot5type` = 7, `augslot5visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` <> 54
  AND `slots` > 0
  AND `classes` > 0
  AND `races` > 0
  AND IFNULL(`augslot5type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 7) + (IFNULL(`augslot2type`, 0) = 7) + (IFNULL(`augslot3type`, 0) = 7) + (IFNULL(`augslot4type`, 0) = 7) + (IFNULL(`augslot5type`, 0) = 7) + (IFNULL(`augslot6type`, 0) = 7)) < 1
  AND (`ac` > 0 OR `hp` > 0 OR `mana` > 0 OR `endur` > 0 OR `damage` > 0 OR `attack` > 0 OR `haste` > 0 OR `aagi` > 0 OR `acha` > 0 OR `adex` > 0 OR `aint` > 0 OR `asta` > 0 OR `astr` > 0 OR `awis` > 0 OR `heroic_str` > 0 OR `heroic_int` > 0 OR `heroic_wis` > 0 OR `heroic_agi` > 0 OR `heroic_dex` > 0 OR `heroic_sta` > 0 OR `heroic_cha` > 0 OR `healamt` > 0 OR `spelldmg` > 0 OR `clairvoyance` > 0);

UPDATE `items`
SET `augslot6type` = 7, `augslot6visible` = 1
WHERE `itemclass` = 0
  AND `itemtype` <> 54
  AND `slots` > 0
  AND `classes` > 0
  AND `races` > 0
  AND IFNULL(`augslot6type`, 0) = 0
  AND ((IFNULL(`augslot1type`, 0) = 7) + (IFNULL(`augslot2type`, 0) = 7) + (IFNULL(`augslot3type`, 0) = 7) + (IFNULL(`augslot4type`, 0) = 7) + (IFNULL(`augslot5type`, 0) = 7) + (IFNULL(`augslot6type`, 0) = 7)) < 1
  AND (`ac` > 0 OR `hp` > 0 OR `mana` > 0 OR `endur` > 0 OR `damage` > 0 OR `attack` > 0 OR `haste` > 0 OR `aagi` > 0 OR `acha` > 0 OR `adex` > 0 OR `aint` > 0 OR `asta` > 0 OR `astr` > 0 OR `awis` > 0 OR `heroic_str` > 0 OR `heroic_int` > 0 OR `heroic_wis` > 0 OR `heroic_agi` > 0 OR `heroic_dex` > 0 OR `heroic_sta` > 0 OR `heroic_cha` > 0 OR `healamt` > 0 OR `spelldmg` > 0 OR `clairvoyance` > 0);

COMMIT;
