-- ============================================================
-- Faded Runestones: Discoverable Dungeon Waypoints
-- ============================================================
-- Adds:
--   1. A new "Dungeons" waypoint category (id 7)
--   2. 15 dungeon waypoints (ids 21-35) — NOT default-unlocked
--   3. One "a_faded_runestone" NPC template (id 999300)
--      Race 127 (invisible), untargetable, non-aggro
--      Place via GM commands: #npcspawn create 999300
--      Reposition via: #npcedit setloc
--
-- The runestone's global quest script (quests/global/a_faded_runestone.pl)
-- handles proximity-triggered attunement in any zone.
-- ============================================================

-- ----- Category -----
INSERT INTO `thj_waypoints_categories` (`id`, `name`) VALUES (7, 'Dungeons')
ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);

-- ----- Dungeon Waypoints -----
-- Coordinates are zone safe points (near entrance). Tearel teleports here.
-- None are in thj_waypoints_default, so they start LOCKED.

INSERT INTO `thj_waypoints` (`id`, `shortname`, `long_name`, `category`, `x`, `y`, `z`, `heading`) VALUES
-- Antonica dungeons
(21, 'befallen',    'Befallen',                  7,  35.0,    -82.0,     3.0,    0),
(22, 'najena',      'Najena',                    7,  858.0,   -76.0,     4.0,    0),
(23, 'permafrost',  'Permafrost Caverns',        7,  61.0,    -121.0,    2.0,    0),
(24, 'soldunga',    'Solusek\'s Eye',            7,  -486.0,  -476.0,    73.0,   0),
(25, 'soldungb',    'Nagafen\'s Lair',           7,  -263.0,  -424.0,    -108.0, 0),
(26, 'gukbottom',   'The Ruins of Old Guk',      7,  -217.0,  1197.0,    -78.0,  0),
(27, 'unrest',      'The Estate of Unrest',      7,  52.0,    -38.0,     3.0,    0),
(28, 'hole',        'The Hole',                  7,  -1050.0, 640.0,     -80.0,  0),
-- Faydwer dungeons
(29, 'crushbone',   'Crushbone',                 7,  158.0,   -644.0,    4.0,    0),
(30, 'mistmoore',   'Castle Mistmoore',          7,  120.0,   -330.0,    -178.0, 0),
(31, 'kedge',       'Kedge Keep',                7,  14.0,    100.0,     302.0,  0),
-- Kunark dungeons
(32, 'sebilis',     'The Ruins of Sebilis',      7,  0.0,     250.0,     44.0,   0),
(33, 'chardok',     'Chardok',                   7,  859.0,   119.0,     106.0,  0),
(34, 'karnor',      'Karnor\'s Castle',          7,  302.0,   18.0,      6.0,    0),
-- Velious dungeons
(35, 'kael',        'Kael Drakkel',              7,  -633.0,  -47.0,     128.0,  0)
ON DUPLICATE KEY UPDATE
    `long_name` = VALUES(`long_name`),
    `category`  = VALUES(`category`),
    `x` = VALUES(`x`), `y` = VALUES(`y`), `z` = VALUES(`z`),
    `heading` = VALUES(`heading`);

-- ----- Faded Runestone NPC Template -----
-- Race 127 = invisible man, bodytype 11 = untargetable intractable object
-- Class 1 (warrior), level 1, no aggro, no loot, not attackable
-- Place one per dungeon zone via: #npcspawn create 999300
-- Then reposition with: #npcedit setloc  (or just move and save)
--
-- Zones that need a runestone spawned:
--   befallen, najena, permafrost, soldunga, soldungb,
--   gukbottom, unrest, hole, crushbone, mistmoore,
--   kedge, sebilis, chardok, karnor, kael

INSERT INTO `npc_types` (
    `id`, `name`, `lastname`, `level`, `race`, `class`, `bodytype`,
    `hp`, `gender`, `texture`, `helmtexture`, `size`,
    `hp_regen_rate`, `hp_regen_per_second`,
    `attack_speed`, `npc_aggro`, `maxlevel`,
    `trackable`, `findable`, `see_invis`, `see_invis_undead`,
    `qglobal`, `npc_spells_id`, `adventure_template_id`
) VALUES (
    999300, 'a_faded_runestone', 'Remnant of the Combine', 1, 127, 1, 11,
    100000, 2, 0, 0, 0,
    100000, 100000,
    0, 0, 1,
    0, 0, 0, 0,
    0, 0, 0
) ON DUPLICATE KEY UPDATE
    `name`     = VALUES(`name`),
    `lastname` = VALUES(`lastname`),
    `race`     = VALUES(`race`),
    `bodytype` = VALUES(`bodytype`);
