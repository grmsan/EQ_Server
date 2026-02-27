# Alternate Advancement (AA) Development Guide

## Overview

This guide is for this repository's current AA implementation and data layout.

AA flow in this codebase:

1. `aa_ability` defines the AA
2. `aa_ranks` defines each rank and click behavior
3. `aa_rank_effects` defines passive effects per rank
4. `aa_rank_prereqs` defines unlock requirements
5. `zone/aa.cpp` sends/purchases/activates AAs
6. `zone/bonuses.cpp` applies passive rank effects

## Database Tables (Current Schema)

### `aa_ability`

Core fields you will use most:

- `id`
- `name`
- `category`
- `classes`
- `type`
- `charges`
- `grant_only`
- `first_rank_id`
- `enabled`

Also available for gating:

- `races`
- `deities`
- `status`
- `reset_on_death`
- `auto_grant_enabled`

### `aa_ranks`

Core fields:

- `id`
- `upper_hotkey_sid`
- `lower_hotkey_sid`
- `title_sid`
- `desc_sid`
- `cost`
- `level_req`
- `spell` (`-1` for passive ranks)
- `spell_type` (shared timer group)
- `recast_time` (seconds)
- `expansion`
- `prev_id`
- `next_id`

### `aa_rank_effects`

- `rank_id`
- `slot`
- `effect_id`
- `base1`
- `base2`

### `aa_rank_prereqs`

- `rank_id` (the rank being unlocked)
- `aa_id` (required AA ability id)
- `points` (required points in that AA)

## AA Class Bitmask (Important)

In this branch, AA class checks use `1 << class_id` in `zone/aa.cpp` (THJ-style path retained).

Common values:

- Warrior: `2`
- Cleric: `4`
- Paladin: `8`
- Ranger: `16`
- ...
- All 16 player classes: `131070`

Do not reuse item/class bitmask assumptions (`1,2,4,...65535`) for AAs here.

## Creating a Passive AA (Example)

Example: 1-rank passive +10 STR.

```sql
-- Strings (title/desc used by AA window)
INSERT INTO db_str (id, type, value) VALUES
  (62050, 1, 'Power Training'),
  (62050, 2, 'Power Training'),
  (62050, 4, 'Passive training that increases Strength by 10.')
ON DUPLICATE KEY UPDATE value = VALUES(value);

-- Ability
INSERT INTO aa_ability (
  id, name, category, classes, type, charges, grant_only, first_rank_id, enabled
) VALUES (
  32050, 'Power Training', 5, 131070, 4, 0, 0, 52050, 1
)
ON DUPLICATE KEY UPDATE
  name = VALUES(name),
  category = VALUES(category),
  classes = VALUES(classes),
  type = VALUES(type),
  charges = VALUES(charges),
  grant_only = VALUES(grant_only),
  first_rank_id = VALUES(first_rank_id),
  enabled = VALUES(enabled);

-- Rank
INSERT INTO aa_ranks (
  id, upper_hotkey_sid, lower_hotkey_sid, title_sid, desc_sid,
  cost, level_req, spell, spell_type, recast_time, expansion, prev_id, next_id
) VALUES (
  52050, 62050, -1, 62050, 62050,
  1, 1, -1, 0, 0, 0, -1, -1
)
ON DUPLICATE KEY UPDATE
  upper_hotkey_sid = VALUES(upper_hotkey_sid),
  lower_hotkey_sid = VALUES(lower_hotkey_sid),
  title_sid = VALUES(title_sid),
  desc_sid = VALUES(desc_sid),
  cost = VALUES(cost),
  level_req = VALUES(level_req),
  spell = VALUES(spell),
  spell_type = VALUES(spell_type),
  recast_time = VALUES(recast_time),
  expansion = VALUES(expansion),
  prev_id = VALUES(prev_id),
  next_id = VALUES(next_id);

-- Passive effect (+10 STR)
INSERT INTO aa_rank_effects (rank_id, slot, effect_id, base1, base2) VALUES
  (52050, 1, 4, 10, 0)
ON DUPLICATE KEY UPDATE
  effect_id = VALUES(effect_id),
  base1 = VALUES(base1),
  base2 = VALUES(base2);
```

## Creating an Active AA (Example)

Example: 1-rank click AA that casts a spell with a 60-second cooldown.

```sql
-- Active spell the AA will cast
INSERT INTO spells_new (
  id, name, player_1, cast_time, recast_time, buffduration, buffdurationformula,
  `range`, targettype, skill, effectid1, effect_base_value1, goodEffect
) VALUES (
  65060, 'Burst Discipline', 'You unleash a burst of force.',
  0, 0, 0, 0,
  200, 5, 51, 79, -500, 0
)
ON DUPLICATE KEY UPDATE
  name = VALUES(name),
  player_1 = VALUES(player_1),
  cast_time = VALUES(cast_time),
  recast_time = VALUES(recast_time),
  buffduration = VALUES(buffduration),
  buffdurationformula = VALUES(buffdurationformula),
  `range` = VALUES(`range`),
  targettype = VALUES(targettype),
  skill = VALUES(skill),
  effectid1 = VALUES(effectid1),
  effect_base_value1 = VALUES(effect_base_value1),
  goodEffect = VALUES(goodEffect);

INSERT INTO db_str (id, type, value) VALUES
  (62060, 1, 'Burst Discipline'),
  (62060, 2, 'Burst Discipline'),
  (62060, 4, 'Click to cast Burst Discipline. Recast: 60 seconds.')
ON DUPLICATE KEY UPDATE value = VALUES(value);

INSERT INTO aa_ability (
  id, name, category, classes, type, charges, grant_only, first_rank_id, enabled
) VALUES (
  32060, 'Burst Discipline', 5, 131070, 4, 0, 0, 52060, 1
)
ON DUPLICATE KEY UPDATE
  name = VALUES(name),
  category = VALUES(category),
  classes = VALUES(classes),
  type = VALUES(type),
  charges = VALUES(charges),
  grant_only = VALUES(grant_only),
  first_rank_id = VALUES(first_rank_id),
  enabled = VALUES(enabled);

INSERT INTO aa_ranks (
  id, upper_hotkey_sid, lower_hotkey_sid, title_sid, desc_sid,
  cost, level_req, spell, spell_type, recast_time, expansion, prev_id, next_id
) VALUES (
  52060, 62060, -1, 62060, 62060,
  1, 1, 65060, 250, 60, 0, -1, -1
)
ON DUPLICATE KEY UPDATE
  upper_hotkey_sid = VALUES(upper_hotkey_sid),
  lower_hotkey_sid = VALUES(lower_hotkey_sid),
  title_sid = VALUES(title_sid),
  desc_sid = VALUES(desc_sid),
  cost = VALUES(cost),
  level_req = VALUES(level_req),
  spell = VALUES(spell),
  spell_type = VALUES(spell_type),
  recast_time = VALUES(recast_time),
  expansion = VALUES(expansion),
  prev_id = VALUES(prev_id),
  next_id = VALUES(next_id);
```

## Prerequisite Example

```sql
-- Rank 52060 requires 3 points in AA ability 32050
INSERT INTO aa_rank_prereqs (rank_id, aa_id, points) VALUES
  (52060, 32050, 3)
ON DUPLICATE KEY UPDATE
  aa_id = VALUES(aa_id),
  points = VALUES(points);
```

## Testing Workflow

1. Reload AA data: `#reload aa_data`
2. Give yourself AA points: `#set aa_points aa 20`
3. Purchase from AA window and verify behavior.
4. Validate stats with `#showstats` (for passive AAs).
5. Validate recast/timer behavior for active AAs.

If you changed spell text/names or AA text SIDs:

1. Export `spells_us.txt` and `dbstr_us.txt` from `server_manager.py`
2. Fully restart client

## Command Semantics to Avoid Confusion

- `#grantaa` in this codebase grants all AAs up to a level on the target.
- It is not "grant AA by id".

For per-AA grant testing, use purchase flow (`#set aa_points`) or direct SQL updates to `character_alternate_abilities`.

## Common Issues

### AA does not appear in window

- `aa_ability.enabled` is `1`
- `aa_ability.first_rank_id` points to existing `aa_ranks.id`
- `aa_ranks.title_sid`/`desc_sid` have valid `db_str` rows
- `classes` mask uses this branch's AA bitmask model (`1 << class_id`)
- `#reload aa_data` ran successfully
- Client was fully restarted after `dbstr_us.txt` update

### AA appears but activation does nothing

- `aa_ranks.spell` exists in `spells_new`
- `targettype` and range are valid for intended use
- `recast_time`/`spell_type` are not blocking use
- Any custom handling in `zone/aa.cpp` still matches your IDs

### Passive effect not applying

- `aa_rank_effects.rank_id` matches purchased rank
- `effect_id` exists and is handled in `ApplyAABonuses()`
- No conflicting overwrite/stack behavior is removing the bonus

