-- Bazaar and Back visibility / client string alignment
-- Safe with existing Origin AA (id 331 / rank 1000)

START TRANSACTION;

UPDATE aa_ability
SET grant_only = 1,
    auto_grant_enabled = 1
WHERE id = 32001;

INSERT INTO db_str (id, type, value)
VALUES
    (62001, 6, 'Creates a portal to the Bazaar. Use again in Bazaar to return to your saved location.')
ON DUPLICATE KEY UPDATE value = VALUES(value);

COMMIT;

