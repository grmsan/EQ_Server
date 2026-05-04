-- ============================================================
-- Classic EQ Server Setup Script
-- Run this once to configure the server for classic-only content
-- with Bazaar as the hub zone.
-- NOTE: run_sql.py may not auto-commit multi-statement scripts.
-- Use utils/sql/apply_classic_setup.py or run statements manually.
-- ============================================================

-- -------------------------------------------------------
-- 1. Set server to Classic expansion (blocks zones > exp 0)
--    Change back to -1 to restore all expansions
-- -------------------------------------------------------
UPDATE rule_values SET rule_value = '0'
WHERE rule_name = 'Expansion:CurrentExpansion';

-- -------------------------------------------------------
-- 2. Allow Bazaar regardless of expansion setting
--    (Bazaar is Luclin expansion=3 but serves as our hub)
-- -------------------------------------------------------
UPDATE zone SET bypass_expansion_check = 1 WHERE short_name = 'bazaar';

-- -------------------------------------------------------
-- 3. New characters start in the Bazaar (zone_id 151)
-- -------------------------------------------------------
UPDATE rule_values SET rule_value = '151'
WHERE rule_name = 'World:SoFStartZoneID';

UPDATE rule_values SET rule_value = '151'
WHERE rule_name = 'World:TitaniumStartZoneID';

-- -------------------------------------------------------
-- 4. Disable tutorial so players don't go to Gloomingdeep
-- -------------------------------------------------------
UPDATE rule_values SET rule_value = 'false'
WHERE rule_name = 'World:EnableTutorialButton';

-- -------------------------------------------------------
-- 5. Bazaar Greeter NPC (id 990200) — Human female quest guide
-- -------------------------------------------------------
INSERT IGNORE INTO npc_types (
    id, name, lastname, level, race, class, bodytype, hp, mana, gender,
    texture, helmtexture, size, runspeed, findable, isquest,
    STR, STA, DEX, AGI, _INT, WIS, CHA,
    AC, hp_regen_rate, mana_regen_rate,
    mindmg, maxdmg, attack_count, aggroradius, assistradius
) VALUES (
    990200, 'Bazaar_Greeter', 'Quest Guide', 70, 1, 1, 1, 50000, 0, 1,
    0, 0, 6, 1.25, 1, 1,
    100, 100, 100, 100, 100, 100, 100,
    200, 500, 200,
    0, 0, 0, 0, 0
);

-- -------------------------------------------------------
-- 6. Emissary of the Guilds NPC (id 990201) — High Elf class trainer
-- -------------------------------------------------------
INSERT IGNORE INTO npc_types (
    id, name, lastname, level, race, class, bodytype, hp, mana, gender,
    texture, helmtexture, size, runspeed, findable, isquest,
    STR, STA, DEX, AGI, _INT, WIS, CHA,
    AC, hp_regen_rate, mana_regen_rate,
    mindmg, maxdmg, attack_count, aggroradius, assistradius
) VALUES (
    990201, 'Emissary_of_the_Guilds', 'Class Trainer', 70, 5, 14, 1, 50000, 50000, 0,
    0, 0, 6, 1.25, 1, 1,
    100, 100, 100, 100, 200, 200, 200,
    200, 500, 500,
    0, 0, 0, 0, 0
);

-- -------------------------------------------------------
-- 7. Spawn groups
-- -------------------------------------------------------
INSERT IGNORE INTO spawngroup (id, name, spawn_limit, dist, max_x, min_x, max_y, min_y, delay, mindelay, despawn, despawn_timer, wp_spawns)
VALUES (990200, 'bazaar_greeter_sg',  1, 0, 0, 0, 0, 0, 0, 15000, 0, 100, 0);

INSERT IGNORE INTO spawngroup (id, name, spawn_limit, dist, max_x, min_x, max_y, min_y, delay, mindelay, despawn, despawn_timer, wp_spawns)
VALUES (990201, 'emissary_guilds_sg', 1, 0, 0, 0, 0, 0, 0, 15000, 0, 100, 0);

-- -------------------------------------------------------
-- 8. Spawn entries
-- -------------------------------------------------------
INSERT IGNORE INTO spawnentry (spawngroupID, npcID, chance) VALUES (990200, 990200, 100);
INSERT IGNORE INTO spawnentry (spawngroupID, npcID, chance) VALUES (990201, 990201, 100);

-- -------------------------------------------------------
-- 9. Spawn placements in Bazaar (near Tearel at x=20, y=-15)
-- -------------------------------------------------------
INSERT IGNORE INTO spawn2 (spawngroupID, zone, version, x, y, z, heading, respawntime, variance, pathgrid, path_when_zone_idle, _condition, cond_value, animation, min_expansion, max_expansion)
VALUES (990200, 'bazaar', 0,  65, -15, 0.72, 256, 1800, 0, 0, 0, 0, 1, 0, -1, -1);

INSERT IGNORE INTO spawn2 (spawngroupID, zone, version, x, y, z, heading, respawntime, variance, pathgrid, path_when_zone_idle, _condition, cond_value, animation, min_expansion, max_expansion)
VALUES (990201, 'bazaar', 0, 115, -15, 0.72, 256, 1800, 0, 0, 0, 0, 1, 0, -1, -1);

-- -------------------------------------------------------
-- 10. Pet class bags for summoner classes
--     Given by the Bazaar Greeter when the player has a
--     pet-summoning class at the end of the intro quest.
-- -------------------------------------------------------
INSERT INTO items (id, Name, lore, bagtype, bagslots, bagsize, bagwr, classes, races, nodrop, norent, weight, size, itemclass, icon, slots, magic)
VALUES
    (960001, "Familiar's Satchel",   'A satchel for organizing gear for your elemental familiar.',   1, 8, 4, 0, 65535, 65535, 1, 1, 10, 1, 1, 557, 0, 0),
    (960002, 'Necromantic Bag',      'A dark bag used to carry equipment for your undead servant.',  1, 8, 4, 0, 65535, 65535, 1, 1, 10, 1, 1, 557, 0, 0),
    (960003, "Warder's Pack",        'A rugged pack for storing equipment destined for your warder.',1, 8, 4, 0, 65535, 65535, 1, 1, 10, 1, 1, 557, 0, 0)
ON DUPLICATE KEY UPDATE Name=VALUES(Name), lore=VALUES(lore);

