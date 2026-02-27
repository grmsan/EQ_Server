-- ============================================================================
-- Colossal Smash - Warrior AA Ability
-- ============================================================================
-- Guaranteed crippling-style strike that uses equipped weapons (primary + offhand).
-- Damage is scaled in code via the special attack scaler (spell_id 65010) and
-- applies a crippling-style damage multiplier. No proc/debuff riders here.
-- Cooldown starts at 5m and is reduced by 30s each rank (9 ranks -> 60s final).
-- Level 10 unlock, new rank roughly every 5 levels to ~50.
-- ============================================================================

-- Cleanup prior definitions (if any)
DELETE FROM aa_rank_effects WHERE rank_id BETWEEN 12000 AND 12008;
DELETE FROM aa_ranks        WHERE id      BETWEEN 12000 AND 12008;
DELETE FROM aa_ability      WHERE id = 12000;
DELETE FROM spells_new      WHERE id = 65010;
DELETE FROM db_str          WHERE id IN (110010, 110011, 10010, 10011);

-- Spell definition (used by all ranks; damage is overridden/scaled in code)
INSERT INTO spells_new (
    id, name, player_1, you_cast, other_casts, cast_on_other,
    cast_time, buffdurationformula, buffduration, `range`, targettype, skill,
    effectid1, effect_base_value1, effect_limit_value1,
    effectid2, effect_base_value2, effect_limit_value2,
    goodEffect, ResistDiff, TargetAnim
) VALUES (
    65010,
    'Colossal Smash',
    'You unleash a colossal smash!',
    'You unleash a colossal smash!',
    'unleashes a colossal smash!',
    'are smashed by a colossal blow!',
    0,
    0,
    60,        -- 6 seconds duration
    50,        -- short melee-ranged activation
    5,         -- single target enemy
    51,        -- throwing skill for visuals; damage is melee-like in code
    79,        -- CurrentHPOnce (damage), base overridden in code via scaler
    -500,      -- placeholder, real damage is computed in zone/spell_effects.cpp
    0,
    1,         -- ArmorClass (debuff magnitude set in code)
    -10,       -- base value (scaled in code)
    0,
    0,         -- detrimental
    -150,
    45         -- throw animation; change if you want different visuals
);

-- db_str entries for name/description
INSERT INTO db_str (id, type, value) VALUES
    (10010, 1, 'Colossal Smash'),
    (10011, 4, 'A guaranteed crippling strike using your equipped weapons. Damage scales with strength and both weapons. Cooldown decreases with each rank.');

-- AA ability base
INSERT INTO aa_ability (
    id, name, category, classes, type, charges, grant_only, first_rank_id, enabled
) VALUES (
    12000, 'Colossal Smash', -1, 1, 3, 0, 0, 12000, 1
);

-- Ranks: 9 ranks, cooldown 300s dropping by 30s each rank, level 10..50
INSERT INTO aa_ranks (
    id, upper_hotkey_sid, lower_hotkey_sid, title_sid, desc_sid,
    cost, level_req, spell, spell_type, recast_time, expansion,
    prev_id, next_id
) VALUES
    (12000, 10010, -1, 10010, 10011, 1, 10, 65010, 21, 300, 0, 0, 12001),
    (12001, 10010, -1, 10010, 10011, 1, 15, 65010, 21, 270, 0, 12000, 12002),
    (12002, 10010, -1, 10010, 10011, 1, 20, 65010, 21, 240, 0, 12001, 12003),
    (12003, 10010, -1, 10010, 10011, 1, 25, 65010, 21, 210, 0, 12002, 12004),
    (12004, 10010, -1, 10010, 10011, 1, 30, 65010, 21, 180, 0, 12003, 12005),
    (12005, 10010, -1, 10010, 10011, 1, 35, 65010, 21, 150, 0, 12004, 12006),
    (12006, 10010, -1, 10010, 10011, 1, 40, 65010, 21, 120, 0, 12005, 12007),
    (12007, 10010, -1, 10010, 10011, 1, 45, 65010, 21, 90,  0, 12006, 12008),
    (12008, 10010, -1, 10010, 10011, 1, 50, 65010, 21, 60,  0, 12007, 0);

-- Notes:
-- - Damage is calculated in code (zone/spell_effects.cpp special attack scaler).
-- - If you change the spell_id here, update the scaler registry accordingly.
-- - Recast times are in seconds.
-- - Costs are placeholder (1 per rank); adjust to your economy.
