-- ============================================================================
-- Bazaar Spawn2 Staging (Old Bazaar migration helper)
-- Date: 2026-02-27
--
-- Purpose:
-- - Move all bazaar spawn points (all versions) into a reachable staging grid
--   around a safe anchor in old Bazaar.
-- - Keeps data persistent in `spawn2` so a zone repop uses the new staged spots.
--
-- Anchor used (old Bazaar):
--   X = -824.32, Y = 1.64, Z = 3.44
--
-- Workflow:
-- 1) Run this SQL
-- 2) #repop in bazaar
-- 3) For each NPC: move where desired, then use #spawnfix (or #advnpcspawn movespawn)
-- 4) Final #repop to verify persistence
--
-- Rollback:
--   UPDATE spawn2 s
--   JOIN thj_backup_spawn2_bazaar_20260227 b ON b.id = s.id
--   SET s.x = b.x, s.y = b.y, s.z = b.z, s.heading = b.heading
--   WHERE s.zone = 'bazaar';
-- ============================================================================

START TRANSACTION;

SET @center_x := -824.32;
SET @center_y := 1.64;
SET @center_z := 3.44;
SET @heading  := 256.00;
SET @spacing  := 12.0; -- distance between staged spawns
SET @cols     := 20;   -- grid width
SET @rows     := 20;   -- grid height cycle

CREATE TABLE IF NOT EXISTS thj_backup_spawn2_bazaar_20260227 LIKE spawn2;

INSERT IGNORE INTO thj_backup_spawn2_bazaar_20260227
SELECT *
FROM spawn2
WHERE zone = 'bazaar';

UPDATE spawn2
SET
	x = @center_x + (((id % @cols) - FLOOR(@cols / 2)) * @spacing),
	y = @center_y + ((((id DIV @cols) % @rows) - FLOOR(@rows / 2)) * @spacing),
	z = @center_z,
	heading = @heading
WHERE zone = 'bazaar'
;

COMMIT;
