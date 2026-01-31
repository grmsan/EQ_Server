# Quest Database Reference

SQL schemas, common queries, and database tips for quest development.

## Table of Contents

1. [Key Tables](#key-tables)
2. [Task Tables Schema](#task-tables-schema)
3. [NPC & Spawn Tables](#npc--spawn-tables)
4. [Item Tables](#item-tables)
5. [Faction Tables](#faction-tables)
6. [Common Queries](#common-queries)
7. [Task Templates](#task-templates)

---

## Key Tables

| Table | Purpose |
|-------|---------|
| `tasks` | Task definitions (title, rewards, requirements) |
| `task_activities` | Individual objectives within tasks |
| `npc_types` | NPC definitions (name, stats, appearance) |
| `spawn2` | Where NPCs spawn in zones |
| `spawngroup` | Groups of NPCs that can spawn |
| `spawnentry` | Which NPCs in a spawn group, with chances |
| `items` | Item definitions |
| `faction_list` | Faction definitions |
| `npc_faction` | NPC faction assignments |
| `npc_faction_entries` | Faction relationships |

---

## Task Tables Schema

### tasks Table

```sql
CREATE TABLE `tasks` (
  `id` INT UNSIGNED NOT NULL,               -- Unique task ID
  `type` TINYINT NOT NULL DEFAULT 0,        -- 0=Task, 1=Shared, 2=Quest
  `duration` INT UNSIGNED DEFAULT 0,        -- Time limit (seconds), 0=none
  `duration_code` TINYINT DEFAULT 0,        -- Duration display type
  `title` VARCHAR(100) NOT NULL,            -- Task name in journal
  `description` TEXT NOT NULL,              -- Step descriptions [1,text][2,text]
  `reward_text` VARCHAR(64) DEFAULT '',     -- "You receive X" text
  `reward_id_list` VARCHAR(128) DEFAULT '', -- Item IDs (comma-separated)
  `cash_reward` INT UNSIGNED DEFAULT 0,     -- Copper amount
  `exp_reward` INT DEFAULT 0,               -- Experience points
  `reward_method` TINYINT UNSIGNED DEFAULT 0,
  `reward_points` INT DEFAULT 0,            -- Alternative currency amount
  `reward_point_type` INT DEFAULT 0,        -- Alt currency type
  `min_level` TINYINT UNSIGNED DEFAULT 0,   -- Minimum player level
  `max_level` TINYINT UNSIGNED DEFAULT 0,   -- Maximum player level (0=any)
  `level_spread` INT UNSIGNED DEFAULT 0,    -- Max level diff in group
  `min_players` INT UNSIGNED DEFAULT 0,     -- Min group size (shared)
  `max_players` INT UNSIGNED DEFAULT 0,     -- Max group size (shared)
  `repeatable` TINYINT UNSIGNED DEFAULT 1,  -- 0=One-time, 1=Repeatable
  `faction_reward` INT DEFAULT 0,           -- Faction ID for reward
  `completion_emote` VARCHAR(512) DEFAULT '',-- Yellow text on complete
  `replay_timer_group` INT UNSIGNED DEFAULT 0,
  `replay_timer_seconds` INT UNSIGNED DEFAULT 0, -- Lockout time
  `request_timer_group` INT UNSIGNED DEFAULT 0,
  `request_timer_seconds` INT UNSIGNED DEFAULT 0,
  `dz_template_id` INT UNSIGNED DEFAULT 0,  -- Dynamic zone template
  `lock_activity_id` INT DEFAULT -1,        -- Activity that locks task
  `faction_amount` INT DEFAULT 0,           -- Faction reward amount
  `enabled` SMALLINT DEFAULT 1,             -- Is task active
  PRIMARY KEY (`id`)
);
```

### task_activities Table

```sql
CREATE TABLE `task_activities` (
  `taskid` INT UNSIGNED NOT NULL,           -- Parent task ID
  `activityid` INT UNSIGNED NOT NULL,       -- Activity number (0-19)
  `req_activity_id` INT DEFAULT -1,         -- Prerequisite activity (-1=none)
  `step` INT DEFAULT 0,                     -- Display step number
  `activitytype` TINYINT UNSIGNED DEFAULT 0, -- Type of activity
  `target_name` VARCHAR(64) DEFAULT '',     -- Display name in journal
  `goalmethod` INT UNSIGNED DEFAULT 0,      -- Goal method
  `goalcount` INT DEFAULT 1,                -- Amount needed
  `description_override` VARCHAR(128) DEFAULT '', -- Custom description
  `npc_match_list` VARCHAR(200) DEFAULT '', -- NPC names (pipe-separated)
  `item_id_list` VARCHAR(200) DEFAULT '',   -- Item IDs (pipe-separated)
  `item_list` VARCHAR(128) DEFAULT '',      -- Item names
  `dz_switch_id` INT DEFAULT 0,             -- Dynamic zone switch
  `min_x` FLOAT DEFAULT 0,                  -- Explore zone min X
  `min_y` FLOAT DEFAULT 0,                  -- Explore zone min Y
  `min_z` FLOAT DEFAULT 0,                  -- Explore zone min Z
  `max_x` FLOAT DEFAULT 0,                  -- Explore zone max X
  `max_y` FLOAT DEFAULT 0,                  -- Explore zone max Y
  `max_z` FLOAT DEFAULT 0,                  -- Explore zone max Z
  `skill_list` VARCHAR(64) DEFAULT '-1',    -- Skill requirements
  `spell_list` VARCHAR(64) DEFAULT '0',     -- Spell requirements
  `zones` VARCHAR(64) DEFAULT '',           -- Valid zone IDs (semicolon-sep)
  `zone_version` INT DEFAULT -1,            -- Instance version (-1=any)
  `optional` TINYINT DEFAULT 0,             -- 0=Required, 1=Optional
  `list_group` TINYINT UNSIGNED DEFAULT 0,  -- Grouping for display
  PRIMARY KEY (`taskid`, `activityid`)
);
```

### Activity Types

| Type | Value | Auto-Update | Use Case |
|------|-------|-------------|----------|
| None | 0 | No | Placeholder |
| Deliver | 1 | Manual | Turn in item to NPC |
| Kill | 2 | Yes | Kill NPCs |
| Loot | 3 | Yes | Loot items from corpses |
| SpeakWith | 4 | Manual | Talk to NPC |
| Explore | 5 | Yes | Visit coordinates |
| TradeSkill | 6 | Yes | Craft items |
| Fish | 7 | Yes | Catch fish |
| Forage | 8 | Yes | Forage items |
| CastOn | 9 | Yes | Cast spell on target |
| SkillOn | 10 | Yes | Use skill on target |
| Touch | 11 | Manual | Click object |
| Collect | 13 | Yes | Pick up ground spawns |
| GiveCash | 100 | Manual | Give money to NPC |

---

## NPC & Spawn Tables

### npc_types Table (Key Fields)

```sql
SELECT
  id,              -- NPC Type ID (used in scripts, spawns)
  name,            -- NPC name (underscores = spaces)
  level,           -- NPC level
  class,           -- Class ID
  race,            -- Race ID
  hp,              -- Hit points
  mana,            -- Mana pool
  gender,          -- 0=male, 1=female, 2=neuter
  texture,         -- Appearance texture
  helmtexture,     -- Helm appearance
  size,            -- Model size multiplier
  loottable_id,    -- Loot table reference
  merchant_id,     -- Merchant list (0=not merchant)
  npc_faction_id,  -- Faction assignment
  adventure_template_id,
  trap_template,
  attack_speed,    -- Attack delay
  findable,        -- Shows on /find
  trackable        -- Can be tracked
FROM npc_types;
```

### Spawn System

```sql
-- spawn2: WHERE NPCs spawn
SELECT zone, x, y, z, heading, spawngroupID, respawntime, enabled
FROM spawn2 WHERE zone = 'qeynos';

-- spawngroup: Groups of potential spawns
SELECT id, name, spawn_limit, dist, max_x, min_x, max_y, min_y
FROM spawngroup;

-- spawnentry: WHAT spawns in a group
SELECT spawngroupID, npcID, chance
FROM spawnentry;
```

### Creating a Complete NPC Spawn

```sql
-- Step 1: Create the NPC
INSERT INTO npc_types (id, name, level, race, class, hp, gender, npc_faction_id)
VALUES (500001, 'Quest_Giver_Bob', 50, 1, 1, 32000, 0, 0);

-- Step 2: Create spawn group
INSERT INTO spawngroup (id, name, spawn_limit)
VALUES (500001, 'Quest_Giver_Bob', 1);

-- Step 3: Link NPC to spawn group
INSERT INTO spawnentry (spawngroupID, npcID, chance)
VALUES (500001, 500001, 100);

-- Step 4: Place in zone
INSERT INTO spawn2 (id, spawngroupID, zone, x, y, z, heading, respawntime, enabled)
VALUES (500001, 500001, 'qeynos', -100, 50, 3, 128, 640, 1);
```

---

## Item Tables

### items Table (Key Fields)

```sql
SELECT
  id,              -- Item ID
  name,            -- Item name
  lore,            -- Lore text
  idfile,          -- Icon file
  itemtype,        -- Type (weapon, armor, etc.)
  slots,           -- Equip slots (bitmask)
  nodrop,          -- 0=tradeable, 1=no-drop
  norent,          -- 0=persists, 1=temporary
  magic,           -- 0=mundane, 1=magic
  ac, hp, mana,    -- Stats
  astr, asta, aagi, adex, awis, aint, acha,  -- Stat bonuses
  classes,         -- Usable classes (bitmask)
  races,           -- Usable races (bitmask)
  reqlevel,        -- Required level
  questitemflag    -- 1=quest item
FROM items;
```

### Creating a Quest Item

```sql
-- Simple quest turn-in item
INSERT INTO items (id, name, lore, idfile, itemtype, slots, nodrop, norent, questitemflag)
VALUES (50001, 'Ancient Relic', 'A quest item', 'IT64', 0, 0, 1, 0, 1);

-- Reward item
INSERT INTO items (id, name, lore, idfile, itemtype, slots, nodrop, ac, hp, classes, races)
VALUES (50002, 'Ring of the Questor', 'Earned through valor', 'IT10', 0, 98304, 0, 5, 25, 65535, 65535);
-- slots 98304 = fingers
-- classes 65535 = all classes
-- races 65535 = all races
```

---

## Faction Tables

### faction_list Table

```sql
SELECT id, name, base FROM faction_list;
-- id: Faction ID
-- name: Faction name
-- base: Starting value
```

### NPC Faction Assignment

```sql
-- npc_faction: Assign NPC to faction template
SELECT id, name, primaryfaction, ignore_primary_assist
FROM npc_faction;

-- npc_faction_entries: How factions react to each other
SELECT npc_faction_id, faction_id, value, npc_value, temp
FROM npc_faction_entries;
```

### Common Faction Setup

```sql
-- Check existing factions
SELECT id, name FROM faction_list WHERE name LIKE '%qeynos%';

-- Create custom faction
INSERT INTO faction_list (id, name, base) VALUES (5000, 'My Custom Faction', 0);

-- Create faction template for NPC
INSERT INTO npc_faction (id, name, primaryfaction)
VALUES (5000, 'custom_faction_template', 5000);

-- Assign to NPC
UPDATE npc_types SET npc_faction_id = 5000 WHERE id = 500001;
```

---

## Common Queries

### Find NPC by Name

```sql
SELECT id, name, level, zone FROM npc_types
WHERE name LIKE '%guard%'
ORDER BY name
LIMIT 20;
```

### Find Item by Name

```sql
SELECT id, name, lore FROM items
WHERE name LIKE '%sword%'
ORDER BY name
LIMIT 20;
```

### Find Zone ID

```sql
SELECT id, short_name, long_name FROM zone
WHERE short_name LIKE '%freeport%' OR long_name LIKE '%freeport%';
```

### Find NPC Spawn Location

```sql
SELECT s2.zone, s2.x, s2.y, s2.z, nt.name
FROM spawn2 s2
JOIN spawnentry se ON s2.spawngroupID = se.spawngroupID
JOIN npc_types nt ON se.npcID = nt.id
WHERE nt.name LIKE '%Merchant%' AND s2.zone = 'qeynos';
```

### List Tasks by Level Range

```sql
SELECT id, title, min_level, max_level, type
FROM tasks
WHERE min_level <= 20 AND (max_level >= 10 OR max_level = 0)
AND enabled = 1
ORDER BY min_level;
```

### Find Task Activities

```sql
SELECT
  t.id, t.title,
  ta.activityid, ta.step, ta.activitytype, ta.target_name, ta.goalcount
FROM tasks t
JOIN task_activities ta ON t.id = ta.taskid
WHERE t.id = 5000
ORDER BY ta.step;
```

### Check Player Task Completion

```sql
SELECT
  ct.charid, ct.taskid, t.title, ct.acceptedtime, ct.was_rewarded
FROM character_tasks ct
JOIN tasks t ON ct.taskid = t.id
WHERE ct.charid = YOUR_CHAR_ID;
```

### Find Faction by Faction ID

```sql
SELECT fl.id, fl.name, fl.base
FROM faction_list fl
WHERE fl.id = 219;
```

---

## Task Templates

### Kill Quest Template

```sql
-- Simple kill X mobs quest
INSERT INTO tasks VALUES (
  YOUR_TASK_ID, 2, 0, 0,
  'Task Title Here',
  '[1,Kill X target_name in zone_name.]',
  '', '', 0, EXP_REWARD, 0, 0, 0,
  MIN_LEVEL, MAX_LEVEL, 0, 0, 0,
  1, 0, 'Completion message here!',
  0, 0, 0, 0, 0, -1, 0, 1
);

INSERT INTO task_activities VALUES (
  YOUR_TASK_ID, 0, -1, 1, 2,
  'target_name', 0, KILL_COUNT, '',
  'npc_name_pattern', '', '', 0,
  0, 0, 0, 0, 0, 0,
  '-1', '0', 'ZONE_ID',
  -1, 0, 0
);
```

### Loot Quest Template

```sql
-- Collect X items quest
INSERT INTO tasks VALUES (
  YOUR_TASK_ID, 2, 0, 0,
  'Task Title Here',
  '[1,Collect X item_name from mobs in zone_name.]',
  'Reward Name', 'REWARD_ITEM_ID', CASH_REWARD, EXP_REWARD, 0, 0, 0,
  MIN_LEVEL, MAX_LEVEL, 0, 0, 0,
  1, 0, 'Completion message!',
  0, 0, 0, 0, 0, -1, 0, 1
);

INSERT INTO task_activities VALUES (
  YOUR_TASK_ID, 0, -1, 1, 3,
  'item_name', 0, COLLECT_COUNT, '',
  '', 'ITEM_ID', '', 0,
  0, 0, 0, 0, 0, 0,
  '-1', '0', 'ZONE_ID',
  -1, 0, 0
);
```

### Explore Quest Template

```sql
-- Visit location quest
INSERT INTO tasks VALUES (
  YOUR_TASK_ID, 2, 0, 0,
  'Task Title Here',
  '[1,Find the location_name in zone_name.]',
  '', '', 0, EXP_REWARD, 0, 0, 0,
  MIN_LEVEL, MAX_LEVEL, 0, 0, 0,
  1, 0, 'You found it!',
  0, 0, 0, 0, 0, -1, 0, 1
);

INSERT INTO task_activities VALUES (
  YOUR_TASK_ID, 0, -1, 1, 5,
  'location_name', 0, 1, '',
  '', '', '', 0,
  MIN_X, MIN_Y, MIN_Z, MAX_X, MAX_Y, MAX_Z,
  '-1', '0', 'ZONE_ID',
  -1, 0, 0
);
```

### Multi-Step Quest Template

```sql
-- Multi-step: Kill -> Loot -> Deliver
INSERT INTO tasks VALUES (
  YOUR_TASK_ID, 2, 0, 0,
  'Multi-Step Quest',
  '[1,Kill the boss.][2,Loot the artifact.][3,Return to quest giver.]',
  'Reward Name', 'REWARD_ID', 10000, 500, 0, 0, 0,
  20, 30, 0, 0, 0,
  0, 0, 'Quest complete!',
  0, 0, 0, 0, 0, -1, 0, 1
);

-- Step 1: Kill boss
INSERT INTO task_activities VALUES (
  YOUR_TASK_ID, 0, -1, 1, 2,
  'Boss Name', 0, 1, '',
  'Boss_Name', '', '', 0,
  0, 0, 0, 0, 0, 0,
  '-1', '0', 'ZONE_ID', -1, 0, 0
);

-- Step 2: Loot artifact (requires step 1)
INSERT INTO task_activities VALUES (
  YOUR_TASK_ID, 1, 0, 2, 3,
  'Artifact', 0, 1, '',
  '', 'ARTIFACT_ITEM_ID', '', 0,
  0, 0, 0, 0, 0, 0,
  '-1', '0', 'ZONE_ID', -1, 0, 0
);

-- Step 3: Talk to NPC (requires step 2)
INSERT INTO task_activities VALUES (
  YOUR_TASK_ID, 2, 1, 3, 4,
  'Quest Giver', 0, 1, '',
  'Quest_Giver_Name', '', '', 0,
  0, 0, 0, 0, 0, 0,
  '-1', '0', 'ZONE_ID', -1, 0, 0
);
```

### Daily Repeatable Template

```sql
-- Daily quest with 24-hour lockout
INSERT INTO tasks VALUES (
  YOUR_TASK_ID, 2, 0, 0,
  'Daily: Task Name',
  '[1,Do the daily thing.]',
  '', '', 5000, 100, 0, 0, 0,
  10, 0, 0, 0, 0,
  1,                    -- repeatable
  0, 'See you tomorrow!',
  1, 86400,             -- replay_timer_group, 24 hours
  0, 0, 0, -1, 0, 1
);
```

### Shared Task Template

```sql
-- Group task (2-6 players)
INSERT INTO tasks VALUES (
  YOUR_TASK_ID,
  1,                    -- type = Shared
  7200,                 -- 2 hour duration
  0,
  'Group Task Name',
  '[1,Complete group objective.]',
  '', '', 0, 1000, 0, 0, 0,
  25, 40,
  5,                    -- level_spread
  2, 6,                 -- min/max players
  1, 0, 'Group task complete!',
  1, 604800,            -- 1 week lockout
  0, 0, 0, -1, 0, 1
);
```

---

## Useful ID Ranges

Reserve ID ranges to avoid conflicts:

| Range | Purpose |
|-------|---------|
| 1-99999 | Stock EQEmu content |
| 100000-199999 | Custom NPCs |
| 200000-299999 | Custom items |
| 300000-399999 | Custom tasks |
| 400000-499999 | Custom factions |
| 500000+ | Development/testing |

Check for next available ID:

```sql
SELECT MAX(id) + 1 FROM tasks;
SELECT MAX(id) + 1 FROM npc_types;
SELECT MAX(id) + 1 FROM items;
```
