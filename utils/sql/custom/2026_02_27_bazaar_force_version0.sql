-- Force Bazaar routing to classic/old Bazaar (version 0) for this server.
-- Safe to run multiple times.

START TRANSACTION;

-- Keep classic Bazaar valid for all expansions.
UPDATE zone
SET min_expansion = 0,
	max_expansion = 99
WHERE short_name = 'bazaar'
  AND version = 0;

-- Disable DoN Bazaar version from content routing.
UPDATE zone
SET min_expansion = 120,
	max_expansion = 127
WHERE short_name = 'bazaar'
  AND version = 1;

-- Route the global static Bazaar instance to version 0.
UPDATE instance_list
SET version = 0,
	notes = 'Classic Bazaar (forced v0)'
WHERE zone = 151
  AND is_global = 1
  AND never_expires = 1;

COMMIT;

