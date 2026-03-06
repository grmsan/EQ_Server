-- ============================================================================
-- Step 5: Consume Item / Consume Essence AAs
-- Infinite Item Progression — AA-based tier-up via item consumption or Essence
-- ============================================================================
--
-- Creates two custom AAs:
--   1. Consume Item   (ability 32100, rank 50100) — Put matching item on cursor, activate AA
--   2. Consume Essence (ability 32101, rank 50101) — Spends Common Essence from Alt Currency
--
-- AUTO-GRANT SETUP:
--   These AAs are set to auto_grant_enabled = 1. For auto-grant to work:
--     1. Set rule: Expansion:AutoGrantAAExpansion >= 0
--        (In-game: #rules set Expansion:AutoGrantAAExpansion 0)
--     2. Players receive the AAs automatically on login
--
--   If auto-grant is disabled (-1), run the manual grant SQL at the bottom.
--
-- CLIENT STRING IDS:
--   The RoF2 client needs entries in eqstr_us.txt (in your EQ client directory)
--   for the AA names to display properly. Add these lines to the END of the file:
--
--     900100^0^Consume Item^
--     900101^0^Place a matching item on your cursor and activate this ability to feed experience to your Power Source item. Higher-tier items grant more XP.^
--     900102^0^Consume Essence^
--     900103^0^Activate this ability to spend Common Essence and add experience to your Power Source item. Only the needed amount is consumed.^
--     900104^0^Consume^
--     900105^0^Item^
--     900106^0^Consume^
--     900107^0^Essence^
--
--   SID mapping: title_sid=900100/900102, desc_sid=900101/900103,
--                upper_hotkey_sid=900104/900106, lower_hotkey_sid=900105/900107
--
-- ============================================================================

-- ---- AA Abilities ----
INSERT INTO aa_ability (
    id, name, category, classes, races, drakkin_heritage, deities,
    status, type, charges, grant_only, first_rank_id, enabled,
    reset_on_death, auto_grant_enabled
) VALUES
(32100, 'Consume Item', 5, 65535, 65535, 127, 131071, 0, 1, 0, 0, 50100, 1, 0, 1),
(32101, 'Consume Essence', 5, 65535, 65535, 127, 131071, 0, 1, 0, 0, 50101, 1, 0, 1)
ON DUPLICATE KEY UPDATE
    name = VALUES(name),
    category = VALUES(category),
    classes = VALUES(classes),
    first_rank_id = VALUES(first_rank_id),
    enabled = VALUES(enabled),
    auto_grant_enabled = VALUES(auto_grant_enabled);

-- ---- AA Ranks ----
-- spell = -1 (intercepted before spell check in C++)
-- recast_time = 0 (no server-side cooldown, handled by intercept)
-- expansion = 0 (Classic, always eligible for auto-grant when rule >= 0)
-- cost = 0 (free, auto-granted)
INSERT INTO aa_ranks (
    id, upper_hotkey_sid, lower_hotkey_sid, title_sid, desc_sid,
    cost, level_req, spell, spell_type, recast_time, expansion,
    prev_id, next_id
) VALUES
(50100, 900104, 900105, 900100, 900101, 0, 1, -1, 0, 0, 0, -1, -1),
(50101, 900106, 900107, 900102, 900103, 0, 1, -1, 0, 0, 0, -1, -1)
ON DUPLICATE KEY UPDATE
    title_sid = VALUES(title_sid),
    desc_sid = VALUES(desc_sid),
    cost = VALUES(cost),
    level_req = VALUES(level_req),
    spell = VALUES(spell);

-- ============================================================================
-- MANUAL GRANT (run if auto-grant is disabled)
-- Grants both AAs to ALL existing characters.
-- New characters will need this run again, or enable auto-grant via the rule.
-- ============================================================================
-- INSERT INTO character_alternate_abilities (id, aa_id, aa_value, charges)
-- SELECT c.id, 50100, 1, 0 FROM character_data c
-- ON DUPLICATE KEY UPDATE aa_value = 1;
--
-- INSERT INTO character_alternate_abilities (id, aa_id, aa_value, charges)
-- SELECT c.id, 50101, 1, 0 FROM character_data c
-- ON DUPLICATE KEY UPDATE aa_value = 1;

-- ============================================================================
-- VERIFICATION
-- ============================================================================
-- After running this migration and restarting shared_memory + zone:
--   1. Check AA window for "Consume Item" and "Consume Essence"
--   2. If AAs don't appear: #grant_aa all  (grants all eligible AAs)
--   3. Test Consume Item: equip Power Source, put matching item on cursor, use AA
--   4. Test Consume Essence: equip Power Source, have Essence balance, use AA
--   5. #powerslot should show "Essence to next tier:" line
