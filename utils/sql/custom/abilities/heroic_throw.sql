
-- ============================================================================
-- Heroic Throw - Warrior AA Ability (Full Install/Replace Script)
-- ============================================================================
-- Description: Active AA that throws the warrior's weapon at a target,
--              dealing damage as if performing a melee swing.
-- Cost: 0 AA points
-- Level Requirement: 5
-- Cooldown: 10 seconds
-- Range: 150 units
-- ============================================================================

-- Step 1: Remove old entries if they exist
DELETE FROM aa_ability WHERE id = 10000;
DELETE FROM aa_ranks WHERE id = 10000;
DELETE FROM spells_new WHERE id = 65000;
DELETE FROM db_str WHERE id IN (10000, 10001);

-- Step 2: Create the spell that the AA will cast
INSERT INTO spells_new (
    id,
    name,
    player_1,
    you_cast,
    other_casts,
    cast_on_other,
    cast_time,
    buffduration,
    buffdurationformula,
    `range`,
    targettype,
    skill,
    effectid1,
    effect_base_value1,
    effect_limit_value1,
    effectid2,
    effect_base_value2,
    effect_limit_value2,
    effectid3,
    effect_base_value3,
    effect_limit_value3,
    goodEffect,
    TargetAnim
) VALUES (
    65000,                          -- id (Custom spell ID) (below 65535 to avoid client wrap)
    'Heroic Throw',                 -- name
    'You throw your weapon!',       -- player_1 (message to player)
    'You throw your weapon!',       -- you_cast (message to caster)
    'throws their weapon!',         -- other_casts (message to others)
    'You are struck by a thrown weapon!', -- cast_on_other (message to target)
    0,                              -- cast_time (instant)
    60,                             -- buffduration (6 seconds @ 10 ticks/sec)
    0,                              -- buffdurationformula (no scaling)
    200,                            -- range (200 units)
    5,                              -- targettype (5 = single target enemy)
    51,                             -- skill (SkillThrowing) for messaging/animations
    79,                             -- effectid1 (CurrentHPOnce - direct damage, scaled in code)
    -500,                           -- effect_base_value1 (placeholder; overridden in code)
    0,                              -- effect_limit_value1
    1,                              -- effectid2 (ArmorClass debuff, magnitude set in code)
    -5,                             -- effect_base_value2 (placeholder; overridden in code)
    0,
    11,                             -- effectid3 (MovementSpeed - Snare)
    -50,                            -- 50% snare
    0,
    0,                              -- goodEffect (detrimental)
    0                               -- TargetAnim (animation)
);

-- Step 3: Add db_str entries for AA name and description
INSERT INTO db_str (id, type, value) VALUES
    (10000, 1, 'Heroic Throw'),    -- id 10000, type 1 = AA name
	(10000, 2, 'Heroic Throw'),    -- id 10000, type 2 = hotkey name
	(10000, 3, 'Heroic Throw'),    -- id 10000, type 3 = hotkey name
    (10000, 4, 'Throws your weapon at the target, dealing melee damage as if you performed a normal attack. 10 second cooldown. Usable at level 5.'); -- id 10001, type 4 = AA description

-- Step 4: Create the AA Ability
INSERT INTO aa_ability (
    id,
    name,
    category,
    classes,
    type,
    charges,
    grant_only,
    first_rank_id,
    enabled
) VALUES (
    10000,                          -- id (Custom AA ID)
    'Heroic Throw',                 -- name
    -1,                             -- category (-1 = None, standard for most AAs)
    1,                              -- classes (1 = Warrior bitmask)
    3,                              -- type (3 = Class-specific)
    0,                              -- charges (0 = unlimited)
    0,                              -- grant_only (0 = can purchase)
    10000,                          -- first_rank_id (must match aa_ranks.id below)
    1                               -- enabled
);

-- Step 5: Create the AA Rank, referencing db_str for name/desc
INSERT INTO aa_ranks (
    id,
    upper_hotkey_sid,
    lower_hotkey_sid,
    title_sid,
    desc_sid,
    cost,
    level_req,
    spell,
    spell_type,
    recast_time,
    expansion,
    prev_id,
    next_id
  ) VALUES (
      10000,                          -- id (Rank ID)
      10000,                          -- upper_hotkey_sid (db_str id for name)
      -1,                             -- lower_hotkey_sid (-1 = none)
      10000,                          -- title_sid (db_str id for name)
      10000,                          -- desc_sid (db_str id for description)
      0,                              -- cost (0 AA points - free!)
      5,                              -- level_req (Level 5 minimum)
      65000,                          -- spell (Links to spell ID above)
      20,                             -- spell_type (dedicated timer group for Heroic Throw)
      10,                             -- recast_time (10 seconds)
      0,                              -- expansion (0 = base)
      0,                              -- prev_id (no previous rank)
      0                               -- next_id (no next rank)
  );

-- ============================================================================
-- INSTALLATION INSTRUCTIONS
-- ============================================================================
-- 1. Run this SQL file against your EQEmu database:
--    mysql -u root -p eqemu < heroic_throw_full.sql
--
-- 2. Reload AA data (restart zone or use #reload aa):
--    #reload aa
--
-- 3. Grant yourself the AA using ONE of these methods:
--
--    METHOD 1 - Manual Purchase (requires AA points):
--      a. Give yourself AA points: #set aa_points aa 1
--      b. Open AA window (Alt+Y)
--      c. Find "Heroic Throw" and purchase it
--
--    METHOD 2 - Grant All AAs:
--      #grantaa [level]
--      (Grants ALL available AAs for your level, not just this one)
--
-- 4. Use the ability:
--    - Open AA window (Alt+Y)
--    - Find "Heroic Throw" and click it, OR
--    - Put it on a hotbar from the AA window
--
-- ============================================================================
-- NOTES
-- ============================================================================
-- - This AA uses SpellEffect 266 (ExtraAttackChance) set to 100% for one swing, so it should behave like an extra melee attack with your current weapon.
-- - If you want spell-style damage instead, swap effectid1/effect_base_value1 to a proc spell (WeaponProc = 85).
-- - The ability has a 150 unit range (medium distance)
-- - 10 second cooldown prevents spam
-- - No cost makes it available immediately at level 5
-- - Descriptions are stored in db_str table:
--   * ID 10000 (type 1): AA title "Heroic Throw"
--   * ID 10001 (type 4): AA description shown in tooltip
--
-- CUSTOMIZATION OPTIONS:
-- - Increase/decrease range by changing `range` field in spell
-- - Modify cooldown by changing `recast_time` in aa_ranks
-- - Add AA cost by changing `cost` in aa_ranks
-- - Change level requirement by changing `level_req` in aa_ranks
-- - Edit descriptions by updating db_str entries 10000 and 10001
-- ============================================================================
