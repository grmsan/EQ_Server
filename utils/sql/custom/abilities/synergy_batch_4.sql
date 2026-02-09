-- Batch 4: Enchanter, Druid, Necromancer Custom Synergies

-- ENCHANTER (Classes: 8192)
-- Spells: 65100, 65110, 65120
-- AA IDs: 10100, 10101, 10102

DELETE FROM aa_ability WHERE id BETWEEN 10100 AND 10102;
DELETE FROM aa_ranks WHERE id BETWEEN 10100 AND 10102;
DELETE FROM spells_new WHERE id BETWEEN 65100 AND 65120;
DELETE FROM db_str WHERE id BETWEEN 10100 AND 10102;

-- Enchanter Spells
INSERT INTO spells_new (id, name, cast_time, buffduration, buffdurationformula, `range`, targettype, skill, effectid1, effect_base_value1, effectid2, effect_base_value2, goodEffect) VALUES
(65100, 'Echoes of Power', 0, 120, 0, 0, 1, 51, 124, 25, 0, 0, 1), -- Improved Damage
(65110, 'Mind Shield', 0, 60, 0, 0, 1, 51, 168, 10, 0, 0, 1), -- Mitigation
(65120, 'Mental Synergy', 0, 120, 0, 0, 1, 51, 159, 15, 0, 0, 1); -- All Stats

-- Enchanter db_str
INSERT INTO db_str (id, type, value) VALUES
(10100, 1, 'Echoes of Power'), (10100, 4, 'Infuses your offensive spells with echoing power, increasing damage. Scales with INT.'),
(10101, 1, 'Mind Shield'), (10101, 4, 'Protects your mind with a shield of pure intelligence, mitigating damage. Scales with INT.'),
(10102, 1, 'Mental Synergy'), (10102, 4, 'Harmonizes the groups minds, increasing all stats. Scales with INT.');

-- Enchanter AA Ability
INSERT INTO aa_ability (id, name, category, classes, type, charges, grant_only, first_rank_id, enabled) VALUES
(10100, 'Echoes of Power', -1, 8192, 3, 0, 0, 10100, 1),
(10101, 'Mind Shield', -1, 8192, 3, 0, 0, 10101, 1),
(10102, 'Mental Synergy', -1, 8192, 3, 0, 0, 10102, 1);

-- Enchanter AA Ranks
INSERT INTO aa_ranks (id, upper_rank_id, aa_id, title_sid, desc_sid, cost, level_req, spell, spell_type, recast_time, expansion, prev_id, next_id) VALUES
(10100, 10100, 10100, 10100, 10100, 0, 1, 65100, 57, 60, 0, 0, 0),
(10101, 10101, 10101, 10101, 10101, 0, 1, 65110, 58, 120, 0, 0, 0),
(10102, 10102, 10102, 10102, 10102, 0, 1, 65120, 59, 60, 0, 0, 0);

-- DRUID (Classes: 32)
-- Spells: 66400, 66410, 66420
-- AA IDs: 11400, 11401, 11402

DELETE FROM aa_ability WHERE id BETWEEN 11400 AND 11402;
DELETE FROM aa_ranks WHERE id BETWEEN 11400 AND 11402;
DELETE FROM spells_new WHERE id BETWEEN 66400 AND 66420;
DELETE FROM db_str WHERE id BETWEEN 11400 AND 11402;

-- Druid Spells
INSERT INTO spells_new (id, name, cast_time, buffduration, buffdurationformula, `range`, targettype, skill, effectid1, effect_base_value1, effectid2, effect_base_value2, goodEffect) VALUES
(66400, 'Nature\'s Wrath', 0, 0, 0, 200, 5, 51, 79, -1, 0, 0, 0),
(66410, 'Barkskin (Active)', 0, 60, 0, 0, 1, 51, 168, 5, 59, 10, 1), -- Mitigation + Damage Shield
(66420, 'Nature\'s Synergy', 0, 120, 0, 0, 1, 51, 170, 10, 0, 0, 1); -- Spell Crit

-- Druid db_str
INSERT INTO db_str (id, type, value) VALUES
(11400, 1, 'Nature\'s Wrath'), (11400, 4, 'Strikes the target with the raw power of nature. Scales with WIS.'),
(11401, 1, 'Barkskin (Active)'), (11401, 4, 'Wraps you in a skin of ironbark, mitigating damage and reflecting it. Scales with WIS.'),
(11402, 1, 'Nature\'s Synergy'), (11402, 4, 'Connects the group with nature, increasing spell critical chance. Scales with WIS.');

-- Druid AA Ability
INSERT INTO aa_ability (id, name, category, classes, type, charges, grant_only, first_rank_id, enabled) VALUES
(11400, 'Nature\'s Wrath', -1, 32, 3, 0, 0, 11400, 1),
(11401, 'Barkskin (Active)', -1, 32, 3, 0, 0, 11401, 1),
(11402, 'Nature\'s Synergy', -1, 32, 3, 0, 0, 11402, 1);

-- Druid AA Ranks
INSERT INTO aa_ranks (id, upper_rank_id, aa_id, title_sid, desc_sid, cost, level_req, spell, spell_type, recast_time, expansion, prev_id, next_id) VALUES
(11400, 11400, 11400, 11400, 11400, 0, 1, 66400, 60, 60, 0, 0, 0),
(11401, 11401, 11401, 11401, 11401, 0, 1, 66410, 61, 120, 0, 0, 0),
(11402, 11402, 11402, 11402, 11402, 0, 1, 66420, 62, 60, 0, 0, 0);

-- NECROMANCER (Classes: 1024)
-- Spells: 66500, 66510, 66520
-- AA IDs: 11500, 11501, 11502

DELETE FROM aa_ability WHERE id BETWEEN 11500 AND 11502;
DELETE FROM aa_ranks WHERE id BETWEEN 11500 AND 11502;
DELETE FROM spells_new WHERE id BETWEEN 66500 AND 66520;
DELETE FROM db_str WHERE id BETWEEN 11500 AND 11502;

-- Necromancer Spells
INSERT INTO spells_new (id, name, cast_time, buffduration, buffdurationformula, `range`, targettype, skill, effectid1, effect_base_value1, effectid2, effect_base_value2, goodEffect) VALUES
(66500, 'Soul Strike', 0, 0, 0, 200, 5, 51, 79, -1, 0, 0, 0),
(66510, 'Death\'s Guard', 0, 60, 0, 0, 1, 51, 168, 10, 0, 0, 1),
(66520, 'Soul Synergy', 0, 120, 0, 0, 1, 51, 124, 20, 178, 5, 1); -- Improved Damage + Lifetap

-- Necromancer db_str
INSERT INTO db_str (id, type, value) VALUES
(11500, 1, 'Soul Strike'), (11500, 4, 'Strikes the target\'s soul, dealing massive damage. Scales with INT.'),
(11501, 1, 'Death\'s Guard'), (11501, 4, 'Shields you with the power of death, mitigating damage. Scales with INT.'),
(11502, 1, 'Soul Synergy'), (11502, 4, 'Empowers the group with necrotic energy, increasing spell damage and lifetaps. Scales with INT.');

-- Necromancer AA Ability
INSERT INTO aa_ability (id, name, category, classes, type, charges, grant_only, first_rank_id, enabled) VALUES
(11500, 'Soul Strike', -1, 1024, 3, 0, 0, 11500, 1),
(11501, 'Death\'s Guard', -1, 1024, 3, 0, 0, 11501, 1),
(11502, 'Soul Synergy', -1, 1024, 3, 0, 0, 11502, 1);

-- Necromancer AA Ranks
INSERT INTO aa_ranks (id, upper_rank_id, aa_id, title_sid, desc_sid, cost, level_req, spell, spell_type, recast_time, expansion, prev_id, next_id) VALUES
(11500, 11500, 11500, 11500, 11500, 0, 1, 66500, 63, 60, 0, 0, 0),
(11501, 11501, 11501, 11501, 11501, 0, 1, 66510, 64, 120, 0, 0, 0),
(11502, 11502, 11502, 11502, 11502, 0, 1, 66520, 65, 60, 0, 0, 0);