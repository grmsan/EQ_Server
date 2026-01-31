# Task System (Database-Driven Quests)

The Task System provides UI-tracked quests that appear in the client's Quest Journal. Tasks are defined in database tables and can track progress automatically.

## Table of Contents

1. [Overview](#overview)
2. [Task Types](#task-types)
3. [Database Tables](#database-tables)
4. [Activity Types](#activity-types)
5. [Creating Tasks](#creating-tasks)
6. [Assigning Tasks](#assigning-tasks)
7. [Updating Task Progress](#updating-task-progress)
8. [Shared Tasks](#shared-tasks)
9. [Task Sets](#task-sets)
10. [Script Integration](#script-integration)
11. [Debugging](#debugging)

---

## Overview

Tasks provide:
- **Quest Journal UI** - Players see objectives in their quest log
- **Progress Tracking** - "Kill 3/10 gnolls" automatically updates
- **Multiple Objectives** - Chain activities together
- **Level Requirements** - Restrict by player level
- **Replay Timers** - Control quest repeatability
- **Rewards** - Items, experience, faction, currency

## Task Types

| Type | Value | Description |
|------|-------|-------------|
| Task | 0 | Solo quest (1 active at a time) |
| Shared | 1 | Group quest (1 active at a time) |
| Quest | 2 | Standard quest (up to 19/29 active) |

```cpp
enum class TaskType {
    Task   = 0,   // Solo task slot
    Shared = 1,   // Shared task slot
    Quest  = 2,   // Quest slots (multiple)
};
```

## Database Tables

### `tasks` Table

Main task definitions.

```sql
CREATE TABLE `tasks` (
  `id` int(11) UNSIGNED NOT NULL,
  `type` tinyint(4) NOT NULL DEFAULT 0,
  `duration` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `duration_code` tinyint(4) NOT NULL DEFAULT 0,
  `title` varchar(100) NOT NULL DEFAULT '',
  `description` text NOT NULL,
  `reward_text` varchar(64) NOT NULL DEFAULT '',
  `reward_id_list` varchar(128) NOT NULL DEFAULT '',
  `cash_reward` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `exp_reward` int(11) NOT NULL DEFAULT 0,
  `reward_method` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `reward_points` int(11) NOT NULL DEFAULT 0,
  `reward_point_type` int(11) NOT NULL DEFAULT 0,
  `min_level` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `max_level` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `level_spread` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `min_players` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `max_players` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `repeatable` tinyint(3) UNSIGNED NOT NULL DEFAULT 1,
  `faction_reward` int(11) NOT NULL DEFAULT 0,
  `completion_emote` varchar(512) NOT NULL DEFAULT '',
  `replay_timer_group` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `replay_timer_seconds` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `request_timer_group` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `request_timer_seconds` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `dz_template_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `lock_activity_id` int(11) NOT NULL DEFAULT -1,
  `faction_amount` int(11) NOT NULL DEFAULT 0,
  `enabled` smallint(6) NOT NULL DEFAULT 1,
  PRIMARY KEY (`id`)
);
```

### Key Fields

| Field | Description |
|-------|-------------|
| `id` | Unique task identifier |
| `type` | 0=Task, 1=Shared, 2=Quest |
| `duration` | Time limit in seconds (0 = no limit) |
| `title` | Task name shown in Quest Journal |
| `description` | Multi-step description with step markers |
| `reward_id_list` | Item IDs to reward (comma separated) |
| `cash_reward` | Copper amount reward |
| `exp_reward` | Experience points reward |
| `min_level` / `max_level` | Level requirements |
| `repeatable` | 0=One-time, 1=Repeatable |
| `completion_emote` | Yellow text shown on completion |

### `task_activities` Table

Individual objectives within a task.

```sql
CREATE TABLE `task_activities` (
  `taskid` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `activityid` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `req_activity_id` int(11) NOT NULL DEFAULT -1,
  `step` int(11) NOT NULL DEFAULT 0,
  `activitytype` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `target_name` varchar(64) NOT NULL DEFAULT '',
  `goalmethod` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `goalcount` int(11) NOT NULL DEFAULT 1,
  `description_override` varchar(128) NOT NULL DEFAULT '',
  `npc_match_list` varchar(200) NOT NULL DEFAULT '',
  `item_id_list` varchar(200) NOT NULL DEFAULT '',
  `item_list` varchar(128) NOT NULL DEFAULT '',
  `dz_switch_id` int(11) NOT NULL DEFAULT 0,
  `min_x` float NOT NULL DEFAULT 0,
  `min_y` float NOT NULL DEFAULT 0,
  `min_z` float NOT NULL DEFAULT 0,
  `max_x` float NOT NULL DEFAULT 0,
  `max_y` float NOT NULL DEFAULT 0,
  `max_z` float NOT NULL DEFAULT 0,
  `skill_list` varchar(64) NOT NULL DEFAULT '-1',
  `spell_list` varchar(64) NOT NULL DEFAULT '0',
  `zones` varchar(64) NOT NULL DEFAULT '',
  `zone_version` int(11) NOT NULL DEFAULT -1,
  `optional` tinyint(4) NOT NULL DEFAULT 0,
  `list_group` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`taskid`, `activityid`)
);
```

### Key Activity Fields

| Field | Description |
|-------|-------------|
| `taskid` | Parent task ID |
| `activityid` | Activity number (0-19) |
| `req_activity_id` | Previous activity that must complete first (-1 = none) |
| `step` | Display step number in UI |
| `activitytype` | Type of objective (see below) |
| `target_name` | NPC/location name shown in UI |
| `goalcount` | Number required to complete |
| `npc_match_list` | NPC names or IDs to match (pipe-separated) |
| `item_id_list` | Item IDs for collect/deliver (pipe-separated) |
| `zones` | Zone IDs where activity is valid (semicolon-separated) |
| `optional` | 0=Required, 1=Optional objective |

## Activity Types

```cpp
enum class TaskActivityType {
    Unknown    = -1,  // Hidden activity
    None       = 0,   // No specific type
    Deliver    = 1,   // Deliver item to NPC
    Kill       = 2,   // Kill NPCs
    Loot       = 3,   // Loot items from corpses
    SpeakWith  = 4,   // Talk to NPC
    Explore    = 5,   // Visit location
    TradeSkill = 6,   // Craft items
    Fish       = 7,   // Catch fish
    Forage     = 8,   // Forage items
    CastOn     = 9,   // Cast spell on target
    SkillOn    = 10,  // Use skill on target
    Touch      = 11,  // Touch/click object
    Collect    = 13,  // Pick up ground spawns
    GiveCash   = 100, // Give money to NPC
};
```

### Activity Type Details

| Type | Value | Auto-Updates? | Notes |
|------|-------|---------------|-------|
| Kill | 2 | Yes | Uses `npc_match_list` to match NPCs |
| Loot | 3 | Yes | Uses `item_id_list` for items |
| SpeakWith | 4 | Manual | Triggered by script |
| Explore | 5 | Yes | Uses min/max coordinates |
| Deliver | 1 | Manual | Script handles turn-in |

## Creating Tasks

### Example: Simple Kill Quest

```sql
-- Task definition
INSERT INTO `tasks` VALUES (
  5000,                -- id
  2,                   -- type (Quest)
  0,                   -- duration (no limit)
  0,                   -- duration_code
  'Gnoll Extermination',  -- title
  '[1,The guard wants you to kill 10 gnolls in Blackburrow.]',  -- description
  'Guard Token',       -- reward_text
  '12345',             -- reward_id_list (item ID)
  0,                   -- cash_reward
  500,                 -- exp_reward
  0,                   -- reward_method
  0,                   -- reward_points
  0,                   -- reward_point_type
  5,                   -- min_level
  15,                  -- max_level
  0,                   -- level_spread
  0,                   -- min_players
  0,                   -- max_players
  1,                   -- repeatable
  0,                   -- faction_reward
  'You have proven yourself a capable gnoll hunter!',  -- completion_emote
  0, 0, 0, 0,          -- replay/request timers
  0,                   -- dz_template_id
  -1,                  -- lock_activity_id
  0,                   -- faction_amount
  1                    -- enabled
);

-- Activity: Kill 10 gnolls
INSERT INTO `task_activities` VALUES (
  5000,                -- taskid
  0,                   -- activityid
  -1,                  -- req_activity_id (none)
  1,                   -- step
  2,                   -- activitytype (Kill)
  'gnolls',            -- target_name
  0,                   -- goalmethod
  10,                  -- goalcount
  '',                  -- description_override
  'a_gnoll|gnoll_',    -- npc_match_list
  '',                  -- item_id_list
  '',                  -- item_list
  0,                   -- dz_switch_id
  0, 0, 0, 0, 0, 0,    -- coordinates (not used for Kill)
  '-1',                -- skill_list
  '0',                 -- spell_list
  '17',                -- zones (Blackburrow zone ID)
  -1,                  -- zone_version
  0,                   -- optional
  0                    -- list_group
);
```

### Multi-Step Quest Example

```sql
-- Task with multiple steps
INSERT INTO `tasks` VALUES (
  5001, 2, 0, 0,
  'The Lost Artifact',
  '[1,Find the ancient scroll in Befallen.][2,Bring the scroll to Scholar Zendel in Freeport.][3,Scholar Zendel has translated the scroll. Find the artifact location in Oasis.]',
  'Ancient Artifact', '12346', 100000, 1000, 0, 0, 0,
  20, 30, 0, 0, 0, 0, 0,
  'You have recovered the lost artifact!',
  0, 0, 0, 0, 0, -1, 0, 1
);

-- Step 1: Loot scroll
INSERT INTO `task_activities` VALUES (
  5001, 0, -1, 1, 3,   -- Loot type
  'Ancient Scroll', 0, 1, '',
  '', '12350', '',     -- item_id_list = scroll item
  0, 0, 0, 0, 0, 0, 0,
  '-1', '0', '36',     -- Befallen
  -1, 0, 0
);

-- Step 2: Deliver to NPC (requires step 1)
INSERT INTO `task_activities` VALUES (
  5001, 1, 0, 2, 1,    -- Deliver type, req_activity_id = 0
  'Scholar Zendel', 0, 1, '',
  'Scholar_Zendel', '12350', '',  -- NPC name and item
  0, 0, 0, 0, 0, 0, 0,
  '-1', '0', '10',     -- Freeport
  -1, 0, 0
);

-- Step 3: Explore location (requires step 2)
INSERT INTO `task_activities` VALUES (
  5001, 2, 1, 3, 5,    -- Explore type, req_activity_id = 1
  'Hidden Cave', 0, 1, '',
  '', '', '',
  0,
  1200, -500, -50,     -- min coordinates
  1300, -400, 50,      -- max coordinates
  '-1', '0', '37',     -- Oasis
  -1, 0, 0
);
```

### Description Format

The description field uses step markers:
```
[1,First step text.][2,Second step text.][3,Third step text.]
```

The client displays the appropriate step based on current progress.

## Assigning Tasks

### Via Script (Lua)

```lua
function event_say(e)
    if e.message:findi("help") then
        -- Show task selector UI
        eq.task_selector({5000, 5001})  -- List of task IDs
    end
end
```

### Directly Assign

```lua
function event_say(e)
    if e.message:findi("yes") then
        e.other:AssignTask(5000)  -- Assign task 5000
    end
end
```

### Task Set Selector

```lua
-- Show all tasks in a task set
eq.task_set_selector(100)  -- Task set 100
```

## Updating Task Progress

### Automatic Updates

These activity types update automatically:
- **Kill** - When matching NPC dies
- **Loot** - When matching item is looted
- **Explore** - When player enters coordinates
- **Fish/Forage** - When skill succeeds

### Manual Updates (Script)

For activities that need script control:

```lua
function event_say(e)
    if e.message:findi("report") then
        -- Update task 5000, activity 1, by 1 count
        e.other:UpdateTaskActivity(5000, 1, 1)
    end
end
```

### Deliver Activity Pattern

```lua
function event_trade(e)
    local item_lib = require("items")

    -- Check if player has the task active
    if e.other:IsTaskActive(5001) then
        -- Check for the required item
        if item_lib.check_turn_in(e.trade, {item1 = 12350}) then
            e.self:Say("Excellent! Let me study this scroll...")
            -- Update the deliver activity
            e.other:UpdateTaskActivity(5001, 1, 1)
            return
        end
    end

    item_lib.return_items(e.self, e.other, e.trade)
end
```

## Shared Tasks

Shared Tasks are group quests that track progress for all members.

### Creating Shared Tasks

```sql
INSERT INTO `tasks` VALUES (
  6000,
  1,      -- type = 1 (Shared)
  3600,   -- duration = 1 hour
  0,
  'Group Dungeon Crawl',
  '[1,Clear the dungeon of all threats.]',
  '', '', 0, 2000, 0, 0, 0,
  30, 60, 5,    -- Level 30-60, spread 5
  2, 6,         -- 2-6 players
  0, 0, '',
  0, 0, 0, 0, 0, -1, 0, 1
);
```

### Offering Shared Tasks

```lua
function event_say(e)
    if e.message:findi("expedition") then
        -- Only leader can request shared tasks
        eq.shared_task_selector({6000, 6001})
    end
end
```

## Task Sets

Group related tasks for easier management.

### Defining Task Sets

Task sets are configured in the server and assigned via `task_set_id` logic.

```lua
-- Offer all tasks in set 100 to the player
eq.task_set_selector(100)

-- With cooldown ignore
eq.task_set_selector(100, true)
```

## Script Integration

### Task Events (global_player.lua)

```lua
function event_task_accepted(e)
    eq.debug("Player accepted task: " .. e.task_id)
end

function event_task_complete(e)
    eq.debug("Player completed task: " .. e.task_id)
    -- Award bonus based on task
    if e.task_id == 5000 then
        e.self:AddAAPoints(1)
    end
end

function event_task_fail(e)
    e.self:Message(13, "You have failed the task!")
end

function event_task_stage_complete(e)
    -- A single activity completed
    eq.debug("Task " .. e.task_id .. " stage complete")
end

function event_task_update(e)
    -- Progress incremented on any activity
end
```

### Checking Task State

```lua
-- Is task currently active?
if e.other:IsTaskActive(5000) then

-- Has task been completed before?
if e.other:IsTaskCompleted(5000) then

-- Is specific activity active?
if e.other:IsTaskActivityActive(5000, 1) then

-- Get activity progress
local done = eq.get_task_activity_done_count(5000, 0)
```

### Failing Tasks

```lua
-- Fail a task (timer expired, etc.)
e.other:FailTask(5000)
```

## Debugging

### Reload Tasks

```
#reloadtasks         -- Reload all tasks from database
#reloadtasksets      -- Reload task sets
```

### View Task State

```
#task show           -- Show your active tasks
#task assign 5000    -- Assign task 5000
#task update 5000 0 1  -- Update task 5000, activity 0, +1
```

### Common Issues

1. **Task not showing**: Check `enabled = 1` and level requirements
2. **Kill not counting**: Verify `npc_match_list` matches NPC name/ID
3. **Activity not unlocking**: Check `req_activity_id` chain
4. **Zone restriction**: Verify `zones` field has correct zone IDs

### Testing Tips

- Use `#task assign X` to quickly test tasks
- Check `task_activities` zone restrictions
- Verify NPC names match `npc_match_list` patterns
- Use `#gm on` to bypass level requirements

---

## See Also

- [SCRIPTED_QUESTS.md](SCRIPTED_QUESTS.md) - For script-based quests
- [EXAMPLES.md](EXAMPLES.md) - Practical examples
- [EQEmu Task Schema](https://docs.eqemu.io/schema/tasks/)
