-- Batch 3: Shadowknight, Ranger, Monk, Cleric Custom Synergies

-- SHADOWKNIGHT (Classes: 16)
-- Spells: 66000, 66010, 66020
-- AA IDs: 11000, 11001, 11002

DELETE FROM aa_ability WHERE id BETWEEN 11000 AND 11002;
DELETE FROM aa_ranks WHERE id BETWEEN 11000 AND 11002;
DELETE FROM spells_new WHERE id BETWEEN 66000 AND 66020;
DELETE FROM db_str WHERE id BETWEEN 11000 AND 11002;

-- Shadowknight Spells
INSERT INTO spells_new (id, name, cast_time, buffduration, buffdurationformula, `range`, targettype, skill, effectid1, effect_base_value1, effectid2, effect_base_value2, goodEffect) VALUES
(66000, 'Unholy Strike', 0, 0, 0, 15, 5, 51, 79, -1, 0, 0, 0),
(66010, 'Unholy Aegis', 0, 60, 0, 0, 1, 51, 168, 10, 0, 0, 1),
(66020, 'Shadow Synergy', 0, 60, 0, 0, 1, 51, 178, 10, 0, 0, 1); -- Melee Lifetap

-- Shadowknight db_str
INSERT INTO db_str (id, type, value) VALUES
(11000, 1, 'Unholy Strike'), (11000, 4, 'Strikes the enemy with unholy energy. Scales with INT/STR.'),
(11001, 1, 'Unholy Aegis'), (11001, 4, 'Protects you with dark power, mitigating damage. Scales with STA.'),
(11002, 1, 'Shadow Synergy'), (11002, 4, 'Unholy presence that grants melee lifetap to the group. Scales with INT.');

-- Shadowknight AA Ability
INSERT INTO aa_ability (id, name, category, classes, type, charges, grant_only, first_rank_id, enabled) VALUES
(11000, 'Unholy Strike', -1, 16, 3, 0, 0, 11000, 1),
(11001, 'Unholy Aegis', -1, 16, 3, 0, 0, 11001, 1),
(11002, 'Shadow Synergy', -1, 16, 3, 0, 0, 11002, 1);

-- Shadowknight AA Ranks
INSERT INTO aa_ranks (id, upper_rank_id, aa_id, title_sid, desc_sid, cost, level_req, spell, spell_type, recast_time, expansion, prev_id, next_id) VALUES
(11000, 11000, 11000, 11000, 11000, 0, 1, 66000, 45, 60, 0, 0, 0),
(11001, 11001, 11001, 11001, 11001, 0, 1, 66010, 46, 120, 0, 0, 0),
(11002, 11002, 11002, 11002, 11002, 0, 1, 66020, 47, 60, 0, 0, 0);

-- RANGER (Classes: 8)
-- Spells: 66100, 66110, 66120
-- AA IDs: 11100, 11101, 11102

DELETE FROM aa_ability WHERE id BETWEEN 11100 AND 11102;
DELETE FROM aa_ranks WHERE id BETWEEN 11100 AND 11102;
DELETE FROM spells_new WHERE id BETWEEN 66100 AND 66120;
DELETE FROM db_str WHERE id BETWEEN 11100 AND 11102;

-- Ranger Spells
INSERT INTO spells_new (id, name, cast_time, buffduration, buffdurationformula, `range`, targettype, skill, effectid1, effect_base_value1, effectid2, effect_base_value2, goodEffect) VALUES
(66100, 'Predator\'s Strike', 0, 0, 0, 200, 5, 51, 79, -1, 0, 0, 0),
(66110, 'Nature\'s Guard', 0, 60, 0, 0, 1, 51, 168, 10, 0, 0, 1),
(66120, 'Ranger Synergy', 0, 120, 0, 0, 1, 51, 216, 50, 0, 0, 1); -- Accuracy

-- Ranger db_str
INSERT INTO db_str (id, type, value) VALUES
(11100, 1, 'Predator\'s Strike'), (11100, 4, 'A deadly ranged or melee strike. Scales with DEX.'),
(11101, 1, 'Nature\'s Guard'), (11101, 4, 'Calls upon nature to protect you, mitigating damage. Scales with STA.'),
(11102, 1, 'Ranger Synergy'), (11102, 4, 'Sharpens the senses of your group, increasing accuracy. Scales with DEX.');

-- Ranger AA Ability
INSERT INTO aa_ability (id, name, category, classes, type, charges, grant_only, first_rank_id, enabled) VALUES
(11100, 'Predator\'s Strike', -1, 8, 3, 0, 0, 11100, 1),
(11101, 'Nature\'s Guard', -1, 8, 3, 0, 0, 11101, 1),
(11102, 'Ranger Synergy', -1, 8, 3, 0, 0, 11102, 1);

-- Ranger AA Ranks
INSERT INTO aa_ranks (id, upper_rank_id, aa_id, title_sid, desc_sid, cost, level_req, spell, spell_type, recast_time, expansion, prev_id, next_id) VALUES
(11100, 11100, 11100, 11100, 11100, 0, 1, 66100, 48, 60, 0, 0, 0),
(11101, 11101, 11101, 11101, 11101, 0, 1, 66110, 49, 120, 0, 0, 0),
(11102, 11102, 11102, 11102, 11102, 0, 1, 66120, 50, 60, 0, 0, 0);

-- MONK (Classes: 64)
-- Spells: 66200, 66210, 66220
-- AA IDs: 11200, 11201, 11202

DELETE FROM aa_ability WHERE id BETWEEN 11200 AND 11202;
DELETE FROM aa_ranks WHERE id BETWEEN 11200 AND 11202;
DELETE FROM spells_new WHERE id BETWEEN 66200 AND 66220;
DELETE FROM db_str WHERE id BETWEEN 11200 AND 11202;

-- Monk Spells
INSERT INTO spells_new (id, name, cast_time, buffduration, buffdurationformula, `range`, targettype, skill, effectid1, effect_base_value1, effectid2, effect_base_value2, goodEffect) VALUES
(66200, 'Flying Kick', 0, 0, 0, 15, 5, 51, 79, -1, 0, 0, 0),
(66210, 'Iron Skin', 0, 60, 0, 0, 1, 51, 168, 10, 0, 0, 1),
(66220, 'Focused Synergy', 0, 60, 0, 0, 1, 51, 172, 10, 0, 0, 1); -- Avoidance

-- Monk db_str
INSERT INTO db_str (id, type, value) VALUES
(11200, 1, 'Flying Kick'), (11200, 4, 'A powerful flying kick that deals massive damage. Scales with STR.'),
(11201, 1, 'Iron Skin'), (11201, 4, 'Focuses your internal chi to harden your skin, mitigating damage. Scales with STA.'),
(11202, 1, 'Focused Synergy'), (11202, 4, 'Perfected focus that increases avoidance for the group. Scales with DEX.');

-- Monk AA Ability
INSERT INTO aa_ability (id, name, category, classes, type, charges, grant_only, first_rank_id, enabled) VALUES
(11200, 'Flying Kick', -1, 64, 3, 0, 0, 11200, 1),
(11201, 'Iron Skin', -1, 64, 3, 0, 0, 11201, 1),
(11202, 'Focused Synergy', -1, 64, 3, 0, 0, 11202, 1);

-- Monk AA Ranks
INSERT INTO aa_ranks (id, upper_rank_id, aa_id, title_sid, desc_sid, cost, level_req, spell, spell_type, recast_time, expansion, prev_id, next_id) VALUES
(11200, 11200, 11200, 11200, 11200, 0, 1, 66200, 51, 45, 0, 0, 0),
(11201, 11201, 11201, 11201, 11201, 0, 1, 66210, 52, 120, 0, 0, 0),
(11202, 11202, 11202, 11202, 11202, 0, 1, 66220, 53, 60, 0, 0, 0);

-- CLERIC (Classes: 2)
-- Spells: 66300, 66310, 66320
-- AA IDs: 11300, 11301, 11302

DELETE FROM aa_ability WHERE id BETWEEN 11300 AND 11302;
DELETE FROM aa_ranks WHERE id BETWEEN 11300 AND 11302;
DELETE FROM spells_new WHERE id BETWEEN 66300 AND 66320;
DELETE FROM db_str WHERE id BETWEEN 11300 AND 11302;

-- Cleric Spells
INSERT INTO spells_new (id, name, cast_time, buffduration, buffdurationformula, `range`, targettype, skill, effectid1, effect_base_value1, effectid2, effect_base_value2, goodEffect) VALUES
(66300, 'Celestial Strike', 0, 0, 0, 200, 5, 51, 79, -1, 0, 0, 0),
(66310, 'Divine Guard', 0, 60, 0, 0, 1, 51, 168, 10, 0, 0, 1),
(66320, 'Celestial Synergy', 0, 60, 0, 0, 1, 51, 125, 50, 0, 0, 1); -- Improved Heal

-- Cleric db_str
INSERT INTO db_str (id, type, value) VALUES
(11300, 1, 'Celestial Strike'), (11300, 4, 'Strikes the target with divine energy. Scales with WIS.'),
(11301, 1, 'Divine Guard'), (11301, 4, 'Protects you with divine power, mitigating damage. Scales with WIS/STA.'),
(11302, 1, 'Celestial Synergy'), (11302, 4, 'Divine light that massively increases healing effectiveness. Scales with WIS.');

-- Cleric AA Ability
INSERT INTO aa_ability (id, name, category, classes, type, charges, grant_only, first_rank_id, enabled) VALUES
(11300, 'Celestial Strike', -1, 2, 3, 0, 0, 11300, 1),
(11301, 'Divine Guard', -1, 2, 3, 0, 0, 11301, 1),
(11302, 'Celestial Synergy', -1, 2, 3, 0, 0, 11302, 1);

-- Cleric AA Ranks
INSERT INTO aa_ranks (id, upper_rank_id, aa_id, title_sid, desc_sid, cost, level_req, spell, spell_type, recast_time, expansion, prev_id, next_id) VALUES
(11300, 11300, 11300, 11300, 11300, 0, 1, 66300, 54, 90, 0, 0, 0),
(11301, 11301, 11301, 11301, 11301, 0, 1, 66310, 55, 120, 0, 0, 0),
(11302, 11302, 11302, 11302, 11302, 0, 1, 66320, 56, 60, 0, 0, 0);