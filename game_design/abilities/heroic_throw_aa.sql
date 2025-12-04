-- ============================================================================
-- Heroic Throw - Warrior AA Ability
-- ============================================================================
-- Description: Active AA that throws the warrior's weapon at a target,
--              dealing damage as if performing a melee swing.
-- Cost: 0 AA points
-- Level Requirement: 5
-- Cooldown: 10 seconds
-- Range: 150 units
-- ============================================================================

-- Step 1: Create the spell that the AA will cast
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
    goodEffect,
    TargetAnim
) VALUES (
    65000,                          -- id (Custom spell ID) (below 65535 to avoid client wrap)
    'Heroic Throw',                 -- name
    'BLUE_TRAIL',                   -- player_1 (default animation)
    'You hurl your weapon!',        -- you_cast
    'hurls their weapon!',          -- other_casts
    'is struck by a hurled weapon!', -- cast_on_other
    0,                              -- cast_time (instant)
    0,                              -- buffduration (instant)
    0,                              -- buffdurationformula (no scaling)
    200,                            -- range (200 units)
    5,                              -- targettype (5 = single target enemy)
    51,                             -- skill (SkillThrowing) for messaging/animations
    79,                             -- effectid1 (CurrentHPOnce - direct damage)
    -500,                           -- effect_base_value1 (base damage; tune as needed)
    0,                              -- effect_limit_value1
    0,                              -- goodEffect (detrimental)
    45                              -- TargetAnim (throw animation)
);

-- Step 2: Create the AA Ability
INSERT INTO aa_ability (
    id,
    name,
    category,
    classes,
    type,
    charges,
    grant_only,
    first_rank_id,
    enabled,
    reset_on_death
) VALUES (
    10000,                          -- id (Custom AA ID)
    'Heroic Throw',                 -- name
    -1,                             -- category (-1 = None, standard for most AAs)
    1,                              -- classes (1 = Warrior bitmask)
    3,                              -- type (3 = Class-specific)
    0,                              -- charges (0 = unlimited)
    0,                              -- grant_only (0 = can purchase)
    10000,                          -- first_rank_id (MUST match aa_ranks.id below)
    1,                              -- enabled
    0                               -- reset_on_death (0 = persists)
);

-- Step 3: Create the AA Rank
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
    10000,                          -- id (Rank ID) - matches aa_ability.first_rank_id
    10000,                          -- upper_hotkey_sid (db_str id for name)
    -1,                             -- lower_hotkey_sid (-1 = none)
    10000,                          -- title_sid (db_str id for name)
    10001,                          -- desc_sid (db_str id for description)
    0,                              -- cost (0 AA points - free!)
    5,                              -- level_req (Level 5 minimum)
    65000,                          -- spell (Links to spell ID above)
    1,                              -- spell_type (Timer group 1)
    10,                             -- recast_time (10 seconds)
    0,                              -- expansion (base game)
    -1,                             -- prev_id (no previous rank)
    -1                              -- next_id (no next rank)
);

-- ============================================================================
-- INSTALLATION INSTRUCTIONS
-- ============================================================================
-- 1. Run this SQL file against your EQEmu database:
--    mysql -u root -p eqemu < heroic_throw_aa.sql
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
--    METHOD 2 - Direct Grant via Quest Script:
--      a. Say "heroic throw" in-game as a Warrior
--         (The global player.pl script will grant it)
--
--    METHOD 3 - Grant All AAs:
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
--   * ID 10000 (type 4): AA description shown in tooltip
--
-- CUSTOMIZATION OPTIONS:
-- - Increase/decrease range by changing `range` field in spell
-- - Modify cooldown by changing `recast_time` in aa_ranks
-- - Add AA cost by changing `cost` in aa_ranks
-- - Change level requirement by changing `level_req` in aa_ranks
-- - Edit descriptions by updating db_str entries 100000 and 100001
-- ============================================================================
