-- Echoes of Power: Enchanter DPS AA
-- Effect: Improved Damage (124)
-- Base value calculated in C++

SET @spell_id = 65100;

DELETE FROM spells_new WHERE id = @spell_id;
INSERT INTO spells_new (id, name, cast_time, recovery_time, recast_time, buffduration, buffdurationformula, targettype, can_resist, effectid1, effect_base_value1, classes14, minlevel)
VALUES (
    @spell_id,
    'Echoes of Power',
    0,      -- Instant
    0,      -- Recovery
    45000,  -- 45s cooldown
    5,      -- 5 ticks (30s)
    0,      -- Formula 0
    1,      -- Self
    0,      -- No resist
    124,    -- Improved Damage (Spell Power)
    1,      -- Base 1 (scaled by C++)
    14,     -- Enchanter
    5       -- Min Level
);

-- Register AA
DELETE FROM aa_actions WHERE spell_id = @spell_id;
INSERT INTO aa_actions (aa_id, spell_id, name, cost, category)
VALUES (65100, @spell_id, 'Echoes of Power', 1, 1); -- Category 1: DPS
