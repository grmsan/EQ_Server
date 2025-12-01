-- Cleanup previous attempts
DELETE FROM items WHERE id IN (200000, 200001);
DELETE FROM lootdrop_entries WHERE lootdrop_id = 200000;
DELETE FROM lootdrop WHERE id = 200000;
DELETE FROM loottable_entries WHERE loottable_id = 200000;
DELETE FROM loottable WHERE id = 200000;
DELETE FROM spawnentry WHERE spawngroupID IN (200000, 200001);
DELETE FROM spawngroup WHERE id IN (200000, 200001);
DELETE FROM spawn2 WHERE id IN (200000, 200001);
DELETE FROM npc_types WHERE id IN (2000100, 2000101);

-- Items
-- 200000: Corrupted Wolf Pelt
INSERT INTO items (id, Name, lore, itemtype, icon, nodrop, tradeskills)
VALUES (200000, 'Corrupted Wolf Pelt', 'A pelt from a diseased wolf.', 10, 500, 0, 0);

-- 200001: Bow of the Green Warden
INSERT INTO items (id, Name, lore, itemtype, icon, damage, delay, `range`, classes, races, material, color)
VALUES (200001, 'Bow of the Green Warden', 'A finely crafted bow given to defenders of the woods.', 5, 501, 15, 35, 100, 256, 65535, 1, 0);

-- Loot
-- Lootdrop for Pelt
INSERT INTO lootdrop (id, name) VALUES (200000, 'Corrupted Wolf Pelt Drop');
INSERT INTO lootdrop_entries (lootdrop_id, item_id, chance) VALUES (200000, 200000, 100);

-- Loottable for Wolf
INSERT INTO loottable (id, name, mincash, maxcash) VALUES (200000, 'Corrupted Wolf Loot', 0, 0);
INSERT INTO loottable_entries (loottable_id, lootdrop_id, multiplier, probability) VALUES (200000, 200000, 1, 100);

-- NPCs
-- Ranger Alaric
INSERT INTO npc_types (id, name, level, race, class, hp, mana, loottable_id, texture, helmtexture, gender)
VALUES (2000100, 'Ranger_Alaric', 50, 4, 4, 5000, 1000, 0, 1, 0, 0);

-- A Corrupted Wolf
INSERT INTO npc_types (id, name, level, race, class, hp, mana, loottable_id, texture, gender, bodytype)
VALUES (2000101, 'a_corrupted_wolf', 6, 42, 1, 100, 0, 200000, 0, 0, 1);

-- Spawns
-- Spawngroup for Alaric
INSERT INTO spawngroup (id, name, min_x, max_x, min_y, max_y, dist) VALUES (200000, 'Ranger Alaric', 0, 0, 0, 0, 0);
INSERT INTO spawnentry (spawngroupID, npcID, chance) VALUES (200000, 2000100, 100);

-- Spawngroup for Wolf
INSERT INTO spawngroup (id, name, min_x, max_x, min_y, max_y, dist) VALUES (200001, 'Corrupted Wolf', 0, 0, 0, 0, 0);
INSERT INTO spawnentry (spawngroupID, npcID, chance) VALUES (200001, 2000101, 100);

-- Spawn2 (Placing them in ecommons)
-- Alaric near the tunnel (approx coords)
INSERT INTO spawn2 (id, spawngroupID, zone, x, y, z, heading, respawntime)
VALUES (200000, 200000, 'ecommons', -130, -1500, 3, 0, 0);

-- Wolf wandering nearby
INSERT INTO spawn2 (id, spawngroupID, zone, x, y, z, heading, respawntime, variance)
VALUES (200001, 200001, 'ecommons', -200, -1400, 3, 0, 60, 30);
