-- Ignore Pain: Warrior Defensive AA
-- Effect: Melee Mitigation (168)
-- Base value of 1 is just a placeholder, real value calculated in C++

SET @spell_id = 65020;

DELETE FROM spells_new WHERE id = @spell_id;
INSERT INTO spells_new (id, name, cast_time, recovery_time, recast_time, buffduration, buffdurationformula, targettype, can_resist, effectid1, effect_base_value1, classes1, minlevel)
VALUES (
    @spell_id,
    'Ignore Pain',
    0,      -- Instant
    0,      -- Recovery
    60000,  -- 60s cooldown
    3,      -- 3 ticks (18s)
    0,      -- Formula 0: fixed duration
    1,      -- Self
    0,      -- No resist
    168,    -- Melee Mitigation
    1,      -- Base 1 (scaled by C++)
    1,      -- Warrior
    5       -- Min Level
);

-- Register AA
DELETE FROM aa_actions WHERE spell_id = @spell_id;
INSERT INTO aa_actions (aa_id, spell_id, name, cost, category)
VALUES (65020, @spell_id, 'Ignore Pain', 1, 3); -- Category 3: Defensive
