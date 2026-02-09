-- Batch 2: Bard, Beastlord, Berserker, Paladin Custom Synergies

-- BARDS (Classes: 128)
-- Spells: 65600, 65610, 65620
-- AA IDs: 10600, 10601, 10602

DELETE FROM aa_ability WHERE id BETWEEN 10600 AND 10602;
DELETE FROM aa_ranks WHERE id BETWEEN 10600 AND 10602;
DELETE FROM spells_new WHERE id BETWEEN 65600 AND 65620;
DELETE FROM db_str WHERE id BETWEEN 10600 AND 10602;

-- Bard Spells
INSERT INTO spells_new (id, name, cast_time, buffduration, buffdurationformula, `range`, targettype, skill, effectid1, effect_base_value1, effectid2, effect_base_value2, goodEffect) VALUES
(65600, 'Discordant Chord', 0, 0, 0, 100, 5, 51, 79, -1, 0, 0, 0),
(65610, 'Shield of Song', 0, 60, 0, 0, 1, 51, 168, 10, 0, 0, 1),
(65620, 'Bardic Synergy', 0, 120, 0, 0, 1, 51, 2, 100, 0, 0, 1); -- ATK

-- Bard db_str
INSERT INTO db_str (id, type, value) VALUES
(10600, 1, 'Discordant Chord'), (10600, 4, 'A powerful chord that deals magical damage. Scales with DEX.'),
(10601, 1, 'Shield of Song'), (10601, 4, 'Protects your group with a shield of song, mitigating damage. Scales with DEX.'),
(10602, 1, 'Bardic Synergy'), (10602, 4, 'Inspires your group, increasing ATK. Scales with DEX.');

-- Bard AA Ability
INSERT INTO aa_ability (id, name, category, classes, type, charges, grant_only, first_rank_id, enabled) VALUES
(10600, 'Discordant Chord', -1, 128, 3, 0, 0, 10600, 1),
(10601, 'Shield of Song', -1, 128, 3, 0, 0, 10601, 1),
(10602, 'Bardic Synergy', -1, 128, 3, 0, 0, 10602, 1);

-- Bard AA Ranks
INSERT INTO aa_ranks (id, upper_rank_id, aa_id, title_sid, desc_sid, cost, level_req, spell, spell_type, recast_time, expansion, prev_id, next_id) VALUES
(10600, 10600, 10600, 10600, 10600, 0, 1, 65600, 33, 45, 0, 0, 0),
(10601, 10601, 10601, 10601, 10601, 0, 1, 65610, 34, 120, 0, 0, 0),
(10602, 10602, 10602, 10602, 10602, 0, 1, 65620, 35, 60, 0, 0, 0);

-- BEASTLORD (Classes: 16384)
-- Spells: 65700, 65710, 65720
-- AA IDs: 10700, 10701, 10702

DELETE FROM aa_ability WHERE id BETWEEN 10700 AND 10702;
DELETE FROM aa_ranks WHERE id BETWEEN 10700 AND 10702;
DELETE FROM spells_new WHERE id BETWEEN 65700 AND 65720;
DELETE FROM db_str WHERE id BETWEEN 10700 AND 10702;

-- Beastlord Spells
INSERT INTO spells_new (id, name, cast_time, buffduration, buffdurationformula, `range`, targettype, skill, effectid1, effect_base_value1, effectid2, effect_base_value2, goodEffect) VALUES
(65700, 'Bestial Rage', 0, 0, 0, 15, 5, 51, 79, -1, 0, 0, 0),
(65710, 'Primal Fortitude', 0, 60, 0, 0, 1, 51, 168, 10, 0, 0, 1),
(65720, 'Bestial Synergy', 0, 120, 0, 100, 1, 51, 2, 50, 216, 50, 1); -- ATK + Accuracy

-- Beastlord db_str
INSERT INTO db_str (id, type, value) VALUES
(10700, 1, 'Bestial Rage'), (10700, 4, 'Unleashes the primal rage of the beast. Scales with STR.'),
(10701, 1, 'Primal Fortitude'), (10701, 4, 'Hardens your skin with primal energy, mitigating damage. Scales with STA.'),
(10702, 1, 'Bestial Synergy'), (10702, 4, 'Connects your soul with your pet/group, increasing ATK and Accuracy.');

-- Beastlord AA Ability
INSERT INTO aa_ability (id, name, category, classes, type, charges, grant_only, first_rank_id, enabled) VALUES
(10700, 'Bestial Rage', -1, 16384, 3, 0, 0, 10700, 1),
(10701, 'Primal Fortitude', -1, 16384, 3, 0, 0, 10701, 1),
(10702, 'Bestial Synergy', -1, 16384, 3, 0, 0, 10702, 1);

-- Beastlord AA Ranks
INSERT INTO aa_ranks (id, upper_rank_id, aa_id, title_sid, desc_sid, cost, level_req, spell, spell_type, recast_time, expansion, prev_id, next_id) VALUES
(10700, 10700, 10700, 10700, 10700, 0, 1, 65700, 36, 60, 0, 0, 0),
(10701, 10701, 10701, 10701, 10701, 0, 1, 65710, 37, 120, 0, 0, 0),
(10702, 10702, 10702, 10702, 10702, 0, 1, 65720, 38, 60, 0, 0, 0);

-- BERSERKER (Classes: 32768)
-- Spells: 65800, 65810, 65820
-- AA IDs: 10800, 10801, 10802

DELETE FROM aa_ability WHERE id BETWEEN 10800 AND 10802;
DELETE FROM aa_ranks WHERE id BETWEEN 10800 AND 10802;
DELETE FROM spells_new WHERE id BETWEEN 65800 AND 65820;
DELETE FROM db_str WHERE id BETWEEN 10800 AND 10802;

-- Berserker Spells
INSERT INTO spells_new (id, name, cast_time, buffduration, buffdurationformula, `range`, targettype, skill, effectid1, effect_base_value1, effectid2, effect_base_value2, goodEffect) VALUES
(65800, 'Decapitating Strike', 0, 0, 0, 15, 5, 51, 79, -1, 0, 0, 0),
(65810, 'Brute Force', 0, 60, 0, 0, 1, 51, 168, 10, 0, 0, 1),
(65820, 'Berserker Synergy', 0, 60, 0, 0, 1, 51, 169, 10, 0, 0, 1); -- Crit Chance

-- Berserker db_str
INSERT INTO db_str (id, type, value) VALUES
(10800, 1, 'Decapitating Strike'), (10800, 4, 'A brutal strike that deals massive damage. Scales with STR.'),
(10801, 1, 'Brute Force'), (10801, 4, 'Uses raw strength to ignore pain, mitigating damage. Scales with STA.'),
(10802, 1, 'Berserker Synergy'), (10802, 4, 'Frenzy that increases critical hit chance for the group. Scales with STR.');

-- Berserker AA Ability
INSERT INTO aa_ability (id, name, category, classes, type, charges, grant_only, first_rank_id, enabled) VALUES
(10800, 'Decapitating Strike', -1, 32768, 3, 0, 0, 10800, 1),
(10801, 'Brute Force', -1, 32768, 3, 0, 0, 10801, 1),
(10802, 'Berserker Synergy', -1, 32768, 3, 0, 0, 10802, 1);

-- Berserker AA Ranks
INSERT INTO aa_ranks (id, upper_rank_id, aa_id, title_sid, desc_sid, cost, level_req, spell, spell_type, recast_time, expansion, prev_id, next_id) VALUES
(10800, 10800, 10800, 10800, 10800, 0, 1, 65800, 39, 60, 0, 0, 0),
(10801, 10801, 10801, 10801, 10801, 0, 1, 65810, 40, 120, 0, 0, 0),
(10802, 10802, 10802, 10802, 10802, 0, 1, 65820, 41, 60, 0, 0, 0);

-- PALADIN (Classes: 4)
-- Spells: 65900, 65910, 65920
-- AA IDs: 10900, 10901, 10902

DELETE FROM aa_ability WHERE id BETWEEN 10900 AND 10902;
DELETE FROM aa_ranks WHERE id BETWEEN 10900 AND 10902;
DELETE FROM spells_new WHERE id BETWEEN 65900 AND 65920;
DELETE FROM db_str WHERE id BETWEEN 10900 AND 10902;

-- Paladin Spells
INSERT INTO spells_new (id, name, cast_time, buffduration, buffdurationformula, `range`, targettype, skill, effectid1, effect_base_value1, effectid2, effect_base_value2, goodEffect) VALUES
(65900, 'Holy Strike', 0, 0, 0, 15, 5, 51, 79, -1, 0, 0, 0),
(65910, 'Holy Aegis', 0, 60, 0, 0, 1, 51, 168, 10, 0, 0, 1),
(65920, 'Paladin Synergy', 0, 60, 0, 0, 1, 51, 125, 20, 0, 0, 1); -- Improved Heal

-- Paladin db_str
INSERT INTO db_str (id, type, value) VALUES
(10900, 1, 'Holy Strike'), (10900, 4, 'Strikes the enemy with holy light. Scales with WIS/STR.'),
(10901, 1, 'Holy Aegis'), (10901, 4, 'Protects you with divine grace, mitigating damage. Scales with STA.'),
(10902, 1, 'Paladin Synergy'), (10902, 4, 'Divine presence that increases healing effectiveness for the group. Scales with WIS.');

-- Paladin AA Ability
INSERT INTO aa_ability (id, name, category, classes, type, charges, grant_only, first_rank_id, enabled) VALUES
(10900, 'Holy Strike', -1, 4, 3, 0, 0, 10900, 1),
(10901, 'Holy Aegis', -1, 4, 3, 0, 0, 10901, 1),
(10902, 'Paladin Synergy', -1, 4, 3, 0, 0, 10902, 1);

-- Paladin AA Ranks
INSERT INTO aa_ranks (id, upper_rank_id, aa_id, title_sid, desc_sid, cost, level_req, spell, spell_type, recast_time, expansion, prev_id, next_id) VALUES
(10900, 10900, 10900, 10900, 10900, 0, 1, 65900, 42, 60, 0, 0, 0),
(10901, 10901, 10901, 10901, 10901, 0, 1, 65910, 43, 120, 0, 0, 0),
(10902, 10902, 10902, 10902, 10902, 0, 1, 65920, 44, 60, 0, 0, 0);