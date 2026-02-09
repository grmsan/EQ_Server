-- Batch 1: Magician, Rogue, Wizard, Shaman Custom Synergies

-- MAGICIANS (Classes: 4096)
-- Spells: 65200 (Offensive), 65210 (Defensive), 65220 (Utility)
-- AA IDs: 10200, 10201, 10202

DELETE FROM aa_ability WHERE id BETWEEN 10200 AND 10202;
DELETE FROM aa_ranks WHERE id BETWEEN 10200 AND 10202;
DELETE FROM spells_new WHERE id BETWEEN 65200 AND 65220;
DELETE FROM db_str WHERE id BETWEEN 10200 AND 10202;

-- Magician Spells
INSERT INTO spells_new (id, name, cast_time, buffduration, buffdurationformula, `range`, targettype, skill, effectid1, effect_base_value1, effectid2, effect_base_value2, goodEffect) VALUES
(65200, 'Elemental Burst', 0, 0, 0, 200, 5, 51, 79, -1, 0, 0, 0), -- Offensive: Fire Nuke
(65210, 'Elemental Shield', 0, 60, 0, 0, 1, 51, 168, 5, 0, 0, 1), -- Defensive: Mitigation
(65220, 'Planar Synergy', 0, 120, 0, 100, 1, 51, 2, 50, 216, 50, 1); -- Utility: Pet/Group ATK+Accuracy

-- Magician db_str
INSERT INTO db_str (id, type, value) VALUES
(10200, 1, 'Elemental Burst'), (10200, 4, 'Unleashes a massive burst of elemental energy. Scales with INT.'),
(10201, 1, 'Elemental Shield'), (10201, 4, 'Wraps you in a shield of elements, mitigating damage. Scales with INT.'),
(10202, 1, 'Planar Synergy'), (10202, 4, 'Empowers your pet and group with planar energy. Increases ATK and Accuracy.');

-- Magician AA Ability
INSERT INTO aa_ability (id, name, category, classes, type, charges, grant_only, first_rank_id, enabled) VALUES
(10200, 'Elemental Burst', -1, 4096, 3, 0, 0, 10200, 1),
(10201, 'Elemental Shield', -1, 4096, 3, 0, 0, 10201, 1),
(10202, 'Planar Synergy', -1, 4096, 3, 0, 0, 10202, 1);

-- Magician AA Ranks (Cooldowns: 60s, 120s, 60s)
INSERT INTO aa_ranks (id, upper_rank_id, aa_id, title_sid, desc_sid, cost, level_req, spell, spell_type, recast_time, expansion, prev_id, next_id) VALUES
(10200, 10200, 10200, 10200, 10200, 0, 1, 65200, 21, 60, 0, 0, 0),
(10201, 10201, 10201, 10201, 10201, 0, 1, 65210, 22, 120, 0, 0, 0),
(10202, 10202, 10202, 10202, 10202, 0, 1, 65220, 23, 60, 0, 0, 0);

-- ROGUES (Classes: 256)
-- Spells: 65300 (Offensive), 65310 (Defensive), 65320 (Utility)
-- AA IDs: 10300, 10301, 10302

DELETE FROM aa_ability WHERE id BETWEEN 10300 AND 10302;
DELETE FROM aa_ranks WHERE id BETWEEN 10300 AND 10302;
DELETE FROM spells_new WHERE id BETWEEN 65300 AND 65320;
DELETE FROM db_str WHERE id BETWEEN 10300 AND 10302;

-- Rogue Spells
INSERT INTO spells_new (id, name, cast_time, buffduration, buffdurationformula, `range`, targettype, skill, effectid1, effect_base_value1, effectid2, effect_base_value2, goodEffect) VALUES
(65300, 'Kidney Strike', 0, 0, 0, 15, 5, 51, 79, -1, 0, 0, 0),
(65310, 'Smoke Screen', 0, 60, 0, 0, 1, 51, 172, 10, 0, 0, 1), -- Avoidance
(65320, 'Armor Shred', 0, 60, 0, 15, 5, 51, 1, -50, 0, 0, 0); -- AC Debuff

-- Rogue db_str
INSERT INTO db_str (id, type, value) VALUES
(10300, 1, 'Kidney Strike'), (10300, 4, 'A precision strike that deals heavy damage. Scales with DEX.'),
(10301, 1, 'Smoke Screen'), (10301, 4, 'Disappear into a cloud of smoke, increasing avoidance. Scales with DEX.'),
(10302, 1, 'Armor Shred'), (10302, 4, 'Shreds the targets armor, reducing their AC. Scales with DEX.');

-- Rogue AA Ability
INSERT INTO aa_ability (id, name, category, classes, type, charges, grant_only, first_rank_id, enabled) VALUES
(10300, 'Kidney Strike', -1, 256, 3, 0, 0, 10300, 1),
(10301, 'Smoke Screen', -1, 256, 3, 0, 0, 10301, 1),
(10302, 'Armor Shred', -1, 256, 3, 0, 0, 10302, 1);

-- Rogue AA Ranks
INSERT INTO aa_ranks (id, upper_rank_id, aa_id, title_sid, desc_sid, cost, level_req, spell, spell_type, recast_time, expansion, prev_id, next_id) VALUES
(10300, 10300, 10300, 10300, 10300, 0, 1, 65300, 24, 60, 0, 0, 0),
(10301, 10301, 10301, 10301, 10301, 0, 1, 65310, 25, 90, 0, 0, 0),
(10302, 10302, 10302, 10302, 10302, 0, 1, 65320, 26, 60, 0, 0, 0);

-- WIZARDS (Classes: 2048)
-- Spells: 65400, 65410, 65420
-- AA IDs: 10400, 10401, 10402

DELETE FROM aa_ability WHERE id BETWEEN 10400 AND 10402;
DELETE FROM aa_ranks WHERE id BETWEEN 10400 AND 10402;
DELETE FROM spells_new WHERE id BETWEEN 65400 AND 65420;
DELETE FROM db_str WHERE id BETWEEN 10400 AND 10402;

-- Wizard Spells
INSERT INTO spells_new (id, name, cast_time, buffduration, buffdurationformula, `range`, targettype, skill, effectid1, effect_base_value1, effectid2, effect_base_value2, goodEffect) VALUES
(65400, 'Mana Burn (Static)', 0, 0, 0, 200, 5, 51, 79, -1, 0, 0, 0),
(65410, 'Arcane Barrier', 0, 60, 0, 0, 1, 51, 168, 10, 0, 0, 1), -- Mitigation
(65420, 'Spell Synergy', 0, 120, 0, 0, 1, 51, 124, 50, 0, 0, 1); -- Improved Damage

-- Wizard db_str
INSERT INTO db_str (id, type, value) VALUES
(10400, 1, 'Mana Burn (Static)'), (10400, 4, 'Sacrifices mana for a devastating burst. Scales with INT.'),
(10401, 1, 'Arcane Barrier'), (10401, 4, 'Protects you with arcane power, mitigating damage. Scales with INT.'),
(10402, 1, 'Spell Synergy'), (10402, 4, 'Focuses your mind, increasing spell damage. Scales with INT.');

-- Wizard AA Ability
INSERT INTO aa_ability (id, name, category, classes, type, charges, grant_only, first_rank_id, enabled) VALUES
(10400, 'Mana Burn (Static)', -1, 2048, 3, 0, 0, 10400, 1),
(10401, 'Arcane Barrier', -1, 2048, 3, 0, 0, 10401, 1),
(10402, 'Spell Synergy', -1, 2048, 3, 0, 0, 10402, 1);

-- Wizard AA Ranks
INSERT INTO aa_ranks (id, upper_rank_id, aa_id, title_sid, desc_sid, cost, level_req, spell, spell_type, recast_time, expansion, prev_id, next_id) VALUES
(10400, 10400, 10400, 10400, 10400, 0, 1, 65400, 27, 90, 0, 0, 0),
(10401, 10401, 10401, 10401, 10401, 0, 1, 65410, 28, 120, 0, 0, 0),
(10402, 10402, 10402, 10402, 10402, 0, 1, 65420, 29, 60, 0, 0, 0);

-- SHAMAN (Classes: 512)
-- Spells: 65500, 65510, 65520
-- AA IDs: 10500, 10501, 10502

DELETE FROM aa_ability WHERE id BETWEEN 10500 AND 10502;
DELETE FROM aa_ranks WHERE id BETWEEN 10500 AND 10502;
DELETE FROM spells_new WHERE id BETWEEN 65500 AND 65520;
DELETE FROM db_str WHERE id BETWEEN 10500 AND 10502;

-- Shaman Spells
INSERT INTO spells_new (id, name, cast_time, buffduration, buffdurationformula, `range`, targettype, skill, effectid1, effect_base_value1, effectid2, effect_base_value2, goodEffect) VALUES
(65500, 'Spirit Strike', 0, 0, 0, 200, 5, 51, 79, -1, 0, 0, 0),
(65510, 'Ancestral Guard', 0, 60, 0, 0, 1, 51, 168, 10, 0, 0, 1),
(65520, 'Spirit Synergy', 0, 120, 0, 0, 1, 51, 159, 20, 0, 0, 1); -- All Stats

-- Shaman db_str
INSERT INTO db_str (id, type, value) VALUES
(10500, 1, 'Spirit Strike'), (10500, 4, 'Strikes the target with pure spirit energy. Scales with WIS.'),
(10501, 1, 'Ancestral Guard'), (10501, 4, 'Summons the protection of ancestors, mitigating damage. Scales with WIS.'),
(10502, 1, 'Spirit Synergy'), (10502, 4, 'Empowers your group with spiritual energy, increasing all stats. Scales with WIS.');

-- Shaman AA Ability
INSERT INTO aa_ability (id, name, category, classes, type, charges, grant_only, first_rank_id, enabled) VALUES
(10500, 'Spirit Strike', -1, 512, 3, 0, 0, 10500, 1),
(10501, 'Ancestral Guard', -1, 512, 3, 0, 0, 10501, 1),
(10502, 'Spirit Synergy', -1, 512, 3, 0, 0, 10502, 1);

-- Shaman AA Ranks
INSERT INTO aa_ranks (id, upper_rank_id, aa_id, title_sid, desc_sid, cost, level_req, spell, spell_type, recast_time, expansion, prev_id, next_id) VALUES
(10500, 10500, 10500, 10500, 10500, 0, 1, 65500, 30, 60, 0, 0, 0),
(10501, 10501, 10501, 10501, 10501, 0, 1, 65510, 31, 120, 0, 0, 0),
(10502, 10502, 10502, 10502, 10502, 0, 1, 65520, 32, 60, 0, 0, 0);