# Quest Examples

Practical examples combining scripted quests and the task system.

## Table of Contents

1. [Simple Turn-in Quest](#simple-turn-in-quest)
2. [Kill Quest with Tracking](#kill-quest-with-tracking)
3. [Multi-Item Collection](#multi-item-collection)
4. [Exploration Task](#exploration-task)
5. [NPC Dialogue Chain](#npc-dialogue-chain)
6. [Task-Triggered Script Rewards](#task-triggered-script-rewards)
7. [Daily Repeatable Task](#daily-repeatable-task)
8. [Faction-Gated Quest](#faction-gated-quest)
9. [Group Shared Task](#group-shared-task)
10. [Scripted + Task Hybrid](#scripted--task-hybrid)

---

## Simple Turn-in Quest

Classic EverQuest style: bring items, get reward.

### Script: `quests/qeynos/Guard_Riles.lua`

```lua
--[[
  Quest: Wolf Bounty
  NPC: Guard Riles in Qeynos
  Objective: Turn in 4 gnoll fangs for a reward
  Reward: 5 gold, 50 exp, faction
]]

function event_say(e)
    if e.message:findi("hail") then
        e.self:Say("Well met, citizen! I am Guard Riles, defender of Qeynos. " ..
            "The [gnoll] threat grows daily. Will you help?")
    elseif e.message:findi("gnoll") then
        e.self:Say("Those foul creatures plague our lands! Bring me [gnoll fangs] " ..
            "as proof of their demise, and I shall reward you handsomely.")
    elseif e.message:findi("gnoll fangs") then
        e.self:Say("Yes! Four gnoll fangs and I'll pay you 5 gold pieces. " ..
            "The Antonican Guard remembers those who help!")
    end
end

function event_trade(e)
    local item_lib = require("items")

    -- Gnoll Fang item ID: 13915
    if item_lib.check_turn_in(e.trade, {
        item1 = 13915,
        item2 = 13915,
        item3 = 13915,
        item4 = 13915
    }) then
        e.self:Say("Excellent work, " .. e.other:GetName() .. "! " ..
            "Four less gnolls to worry about. Here is your reward!")

        -- Rewards
        e.other:GiveCash(0, 0, 5, 0)  -- 5 gold
        e.other:AddEXP(50)
        e.other:Faction(219, 5)       -- Guards of Qeynos
        e.other:Faction(223, 1)       -- Merchants of Qeynos
        e.other:Faction(331, -5)      -- Sabertooths of Blackburrow
    else
        item_lib.return_items(e.self, e.other, e.trade)
    end
end
```

---

## Kill Quest with Tracking

Task system quest with automatic kill tracking.

### SQL: Task Definition

```sql
-- Task: Beetle Extermination
INSERT INTO `tasks` VALUES (
    7001, 2, 0, 0,
    'Beetle Extermination',
    '[1,The Qeynos Guard needs you to kill 10 fire beetles in Qeynos Hills.]',
    '', '',
    10000,      -- 1 plat cash reward
    200,        -- exp reward
    0, 0, 0,
    1, 10,      -- levels 1-10
    0, 0, 0,
    1,          -- repeatable
    219, '',    -- faction reward: Guards of Qeynos
    0, 0, 0, 0, 0, -1,
    5,          -- faction amount
    1
);

-- Activity: Kill 10 fire beetles
INSERT INTO `task_activities` VALUES (
    7001, 0, -1, 1,
    2,              -- Kill type
    'fire beetles',
    0, 10,          -- goal: 10
    '',
    'a_fire_beetle|fire_beetle',  -- NPC name matches
    '', '', 0,
    0, 0, 0, 0, 0, 0,
    '-1', '0',
    '4',            -- Qeynos Hills zone ID
    -1, 0, 0
);
```

### Script: Task Giver `quests/qeynos/Captain_Tillin.lua`

```lua
function event_say(e)
    if e.message:findi("hail") then
        e.self:Say("Greetings! I am Captain Tillin. Are you looking for [work]?")
    elseif e.message:findi("work") then
        if e.other:IsTaskCompleted(7001) then
            e.self:Say("You've already proven yourself against the beetles. Speak to the other officers for more tasks.")
        elseif e.other:IsTaskActive(7001) then
            e.self:Say("Still hunting beetles? Check your quest journal for progress!")
        else
            e.self:Say("Fire beetles have been burning crops in the hills. Will you help eliminate them?")
            eq.task_selector({7001})  -- Show task selector
        end
    end
end
```

---

## Multi-Item Collection

Collect multiple different items.

### SQL

```sql
-- Task: Alchemist's Request
INSERT INTO `tasks` VALUES (
    7002, 2, 0, 0,
    'Alchemist''s Supplies',
    '[1,Collect spider silk from spiders.][2,Collect bone chips from skeletons.][3,Collect bat wings from bats.][4,Return to Alchemist Mensah.]',
    'Potion of Speed', '12500',
    0, 100, 0, 0, 0,
    5, 20, 0, 0, 0,
    1, 0, '',
    0, 0, 0, 0, 0, -1, 0, 1
);

-- Activity 0: Loot 5 spider silk
INSERT INTO `task_activities` VALUES (
    7002, 0, -1, 1, 3,
    'spider silk', 0, 5, '',
    '', '10030', '',    -- Spider Silk Rope item ID
    0, 0, 0, 0, 0, 0, 0,
    '-1', '0', '',      -- Any zone
    -1, 0, 0
);

-- Activity 1: Loot 10 bone chips
INSERT INTO `task_activities` VALUES (
    7002, 1, -1, 2, 3,
    'bone chips', 0, 10, '',
    '', '13073', '',    -- Bone Chips item ID
    0, 0, 0, 0, 0, 0, 0,
    '-1', '0', '',
    -1, 0, 0
);

-- Activity 2: Loot 4 bat wings
INSERT INTO `task_activities` VALUES (
    7002, 2, -1, 3, 3,
    'bat wings', 0, 4, '',
    '', '13068', '',    -- Bat Wing item ID
    0, 0, 0, 0, 0, 0, 0,
    '-1', '0', '',
    -1, 0, 0
);

-- Activity 3: Speak to Alchemist (requires all others)
-- req_activity_id = 2 means this unlocks after activity 2
-- But we want ALL activities, so use step logic or script
INSERT INTO `task_activities` VALUES (
    7002, 3, 2, 4, 4,   -- SpeakWith, requires act 2
    'Alchemist Mensah', 0, 1, '',
    'Alchemist_Mensah', '', '',
    0, 0, 0, 0, 0, 0, 0,
    '-1', '0', '2',     -- Qeynos
    -1, 0, 0
);
```

### Script: Turn-in Handler

```lua
function event_say(e)
    if e.message:findi("hail") then
        if e.other:IsTaskActive(7002) then
            -- Check if collection activities are done
            local silk = eq.get_task_activity_done_count(7002, 0) >= 5
            local bones = eq.get_task_activity_done_count(7002, 1) >= 10
            local wings = eq.get_task_activity_done_count(7002, 2) >= 4

            if silk and bones and wings then
                e.self:Say("Wonderful! You have everything I need!")
                e.other:UpdateTaskActivity(7002, 3, 1)  -- Complete speak activity
            else
                e.self:Say("Still gathering supplies? Check your task journal!")
            end
        else
            e.self:Say("Hello! I'm researching new potions. Do you want to [help]?")
        end
    elseif e.message:findi("help") then
        eq.task_selector({7002})
    end
end
```

---

## Exploration Task

Visit specific locations.

### SQL

```sql
-- Task: Survey the Frontier
INSERT INTO `tasks` VALUES (
    7003, 2, 0, 0,
    'Survey the Frontier',
    '[1,Visit the gnoll camp in Blackburrow.][2,Scout the orc outpost in Highpass.][3,Report to Scout Leader.]',
    'Scout''s Cloak', '12600',
    50000, 500, 0, 0, 0,
    10, 25, 0, 0, 0,
    1, 0, '',
    0, 0, 0, 0, 0, -1, 0, 1
);

-- Activity 0: Explore gnoll camp
INSERT INTO `task_activities` VALUES (
    7003, 0, -1, 1, 5,      -- Explore type
    'Gnoll Camp', 0, 1, '',
    '', '', '', 0,
    -100, -200, -50,        -- min x, y, z
    100, 0, 50,             -- max x, y, z
    '-1', '0', '17',        -- Blackburrow
    -1, 0, 0
);

-- Activity 1: Explore orc outpost
INSERT INTO `task_activities` VALUES (
    7003, 1, 0, 2, 5,       -- Requires activity 0
    'Orc Outpost', 0, 1, '',
    '', '', '', 0,
    500, 100, 0,
    700, 300, 100,
    '-1', '0', '5',         -- Highpass
    -1, 0, 0
);

-- Activity 2: Speak with Scout Leader
INSERT INTO `task_activities` VALUES (
    7003, 2, 1, 3, 4,
    'Scout Leader', 0, 1, '',
    'Scout_Leader', '', '',
    0, 0, 0, 0, 0, 0, 0,
    '-1', '0', '1',         -- Qeynos
    -1, 0, 0
);
```

---

## NPC Dialogue Chain

Pure script-based dialogue quest with no task tracking.

### Script: `quests/freporte/Tolon_Nurbyte.lua`

```lua
--[[
  Quest: The Lost Heirloom
  Multi-NPC dialogue chain
]]

local HEIRLOOM_ITEM = 15000
local QUEST_FLAG = "lost_heirloom_stage"

function event_say(e)
    local stage = e.other:GetBucket(QUEST_FLAG) or "0"

    if e.message:findi("hail") then
        if stage == "0" then
            e.self:Say("Please... you must help me! My family's [heirloom] has been stolen!")
        elseif stage == "1" then
            e.self:Say("Did you find Marcus? What did he say?")
        elseif stage == "2" then
            e.self:Say("The heirloom! Do you have it?")
        elseif stage == "3" then
            e.self:Say("Thank you again for returning our heirloom!")
        end

    elseif e.message:findi("heirloom") and stage == "0" then
        e.self:Say("A golden locket, passed down for generations! " ..
            "I believe a man named [Marcus] in West Freeport stole it.")

    elseif e.message:findi("marcus") and stage == "0" then
        e.self:Say("Yes, Marcus the Fence. Find him and recover my locket! " ..
            "I'll reward you handsomely!")
        e.other:SetBucket(QUEST_FLAG, "1")
        e.other:Message(15, "Quest Updated: Speak to Marcus in West Freeport")
    end
end

function event_trade(e)
    local item_lib = require("items")
    local stage = e.other:GetBucket(QUEST_FLAG)

    if stage == "2" and item_lib.check_turn_in(e.trade, {item1 = HEIRLOOM_ITEM}) then
        e.self:Say("The locket! You found it! Here, take this reward!")
        e.other:GiveCash(0, 0, 0, 5)  -- 5 plat
        e.other:AddEXP(500)
        e.other:SummonItem(15001)     -- Reward ring
        e.other:SetBucket(QUEST_FLAG, "3")
        e.other:DeleteBucket(QUEST_FLAG)  -- Clean up
    else
        item_lib.return_items(e.self, e.other, e.trade)
    end
end
```

### Script: `quests/freportw/Marcus.lua`

```lua
function event_say(e)
    local stage = e.other:GetBucket("lost_heirloom_stage") or "0"

    if e.message:findi("hail") then
        if stage == "1" then
            e.self:Say("What do you want? I don't know anything about any [locket]...")
        else
            e.self:Say("Get lost, I'm busy.")
        end

    elseif e.message:findi("locket") and stage == "1" then
        e.self:Say("Fine! Fine! I sold it to a collector in Highpass. " ..
            "Talk to Gregor. Now leave me alone!")
        e.other:SetBucket("lost_heirloom_stage", "2")
        e.other:Message(15, "Quest Updated: Find Gregor in Highpass")
    end
end
```

---

## Task-Triggered Script Rewards

Use task events to give scripted rewards.

### global_player.lua

```lua
function event_task_complete(e)
    -- Epic task completion
    if e.task_id == 8000 then
        e.self:Message(15, "You have completed an epic achievement!")
        e.self:AddAAPoints(5)
        e.self:SetGM(false)  -- Remove GM flag if testing

        -- Grant title
        e.self:SetAATitle("Gnoll Slayer")
    end

    -- Daily task bonus
    if e.task_id >= 9000 and e.task_id <= 9099 then
        -- Daily tasks give bonus currency
        local points = e.self:GetBucket("daily_points") or "0"
        e.self:SetBucket("daily_points", tostring(tonumber(points) + 10))
        e.self:Message(15, "You earned 10 daily points!")
    end
end
```

---

## Daily Repeatable Task

Task with 24-hour lockout.

### SQL

```sql
INSERT INTO `tasks` VALUES (
    9001, 2, 0, 0,
    'Daily: Orc Patrol',
    '[1,Kill 5 orcs in Commonlands today.]',
    '', '',
    25000, 100, 0, 0, 0,
    10, 50, 0, 0, 0,
    1,                      -- repeatable
    0, '',
    1,                      -- replay_timer_group
    86400,                  -- 24 hours in seconds
    0, 0, 0, -1, 0, 1
);

INSERT INTO `task_activities` VALUES (
    9001, 0, -1, 1, 2,
    'orcs', 0, 5, '',
    'orc|an_orc', '', '',
    0, 0, 0, 0, 0, 0, 0,
    '-1', '0', '21',        -- Commonlands
    -1, 0, 0
);
```

---

## Faction-Gated Quest

Quest only available at certain faction.

### Script

```lua
local REQUIRED_FACTION = 219  -- Guards of Qeynos
local REQUIRED_STANDING = 1   -- Amiable or better

function event_say(e)
    if e.message:findi("hail") then
        local faction_value = e.other:GetFactionValue(REQUIRED_FACTION)

        -- Faction thresholds: Ally=1100+, Warmly=750-1099, Kindly=500-749,
        -- Amiably=100-499, Indifferent=0-99, Apprehensive=-100 to -1, etc.

        if faction_value >= 100 then  -- Amiable+
            e.self:Say("Ah, a friend of Qeynos! I have a special [mission] for trusted allies.")
        else
            e.self:Say("I don't know you well enough to share sensitive information.")
        end

    elseif e.message:findi("mission") then
        local faction_value = e.other:GetFactionValue(REQUIRED_FACTION)
        if faction_value >= 100 then
            eq.task_selector({8001})
        end
    end
end
```

---

## Group Shared Task

### SQL

```sql
-- Shared Task: Dungeon Delve
INSERT INTO `tasks` VALUES (
    6001,
    1,              -- Shared task type
    7200,           -- 2 hour duration
    0,
    'Dungeon Delve: Befallen',
    '[1,Clear the first level of undead.][2,Defeat the Befallen Guardian.][3,Escape the dungeon.]',
    'Delver''s Ring', '16001',
    100000, 2000, 0, 0, 0,
    15, 30,
    5,              -- level_spread (max 5 levels apart)
    2, 6,           -- 2-6 players
    1, 0, '',
    1, 604800,      -- 1 week replay timer
    0, 0, 0, -1, 0, 1
);

-- Kill activity
INSERT INTO `task_activities` VALUES (
    6001, 0, -1, 1, 2,
    'undead', 0, 20, '',
    'a_skeleton|a_zombie|a_ghoul', '', '',
    0, 0, 0, 0, 0, 0, 0,
    '-1', '0', '36',
    -1, 0, 0
);

-- Named kill
INSERT INTO `task_activities` VALUES (
    6001, 1, 0, 2, 2,
    'Befallen Guardian', 0, 1, '',
    'The_Befallen_Guardian', '', '',
    0, 0, 0, 0, 0, 0, 0,
    '-1', '0', '36',
    -1, 0, 0
);

-- Explore exit
INSERT INTO `task_activities` VALUES (
    6001, 2, 1, 3, 5,
    'Dungeon Exit', 0, 1, '',
    '', '', '', 0,
    0, 0, 0, 50, 50, 50,
    '-1', '0', '36',
    -1, 0, 0
);
```

### Script: Shared Task Giver

```lua
function event_say(e)
    if e.message:findi("hail") then
        e.self:Say("Greetings, adventurer! Are you looking to lead an [expedition]?")
    elseif e.message:findi("expedition") then
        -- Check if they're group leader
        local group = e.other:GetGroup()
        if group then
            if group:GetLeader():GetID() == e.other:GetID() then
                eq.shared_task_selector({6001})
            else
                e.self:Say("Only your group leader can request an expedition.")
            end
        else
            e.self:Say("You'll need a group to attempt this dungeon.")
        end
    end
end
```

---

## Scripted + Task Hybrid

Use script for dialogue, task for tracking.

### SQL: Task for tracking only

```sql
INSERT INTO `tasks` VALUES (
    7010, 2, 0, 0,
    'The Merchant''s Dilemma',
    '[1,Investigate the missing shipment.][2,Confront the bandits.][3,Return to Merchant Valen.]',
    'Merchant''s Favor', '',
    50000, 300, 0, 0, 0,
    10, 20, 0, 0, 0,
    0, 0, '',
    0, 0, 0, 0, 0, -1, 0, 1
);

-- Script-controlled activities (SpeakWith type, manually updated)
INSERT INTO `task_activities` VALUES (7010, 0, -1, 1, 4, 'Guard Captain', 0, 1, '', 'Guard_Captain', '', '', 0, 0,0,0,0,0,0, '-1', '0', '1', -1, 0, 0);
INSERT INTO `task_activities` VALUES (7010, 1, 0, 2, 2, 'bandits', 0, 5, '', 'bandit', '', '', 0, 0,0,0,0,0,0, '-1', '0', '2', -1, 0, 0);
INSERT INTO `task_activities` VALUES (7010, 2, 1, 3, 4, 'Merchant Valen', 0, 1, '', 'Merchant_Valen', '', '', 0, 0,0,0,0,0,0, '-1', '0', '1', -1, 0, 0);
```

### Script: Merchant_Valen.lua

```lua
function event_say(e)
    if e.message:findi("hail") then
        if not e.other:IsTaskActive(7010) and not e.other:IsTaskCompleted(7010) then
            e.self:Say("Please, I need [help]! My shipment was stolen!")
        elseif e.other:IsTaskActive(7010) then
            local activity0 = eq.get_task_activity_done_count(7010, 0)
            local activity1 = eq.get_task_activity_done_count(7010, 1)

            if activity1 >= 5 then
                e.self:Say("You defeated the bandits! Thank you! Here's your reward!")
                e.other:UpdateTaskActivity(7010, 2, 1)  -- Complete final activity
            elseif activity0 >= 1 then
                e.self:Say("Did you find the bandits? The Guard Captain said they're in West Karana!")
            else
                e.self:Say("Please, speak to the Guard Captain about my shipment!")
            end
        else
            e.self:Say("Thank you again for your help, friend!")
        end

    elseif e.message:findi("help") then
        if not e.other:IsTaskActive(7010) and not e.other:IsTaskCompleted(7010) then
            e.self:Say("Please, go speak with the [Guard Captain]. He can help track down the thieves!")
            e.other:AssignTask(7010)
        end
    end
end
```

### Script: Guard_Captain.lua

```lua
function event_say(e)
    if e.message:findi("hail") then
        if e.other:IsTaskActive(7010) then
            local activity0 = eq.get_task_activity_done_count(7010, 0)
            if activity0 < 1 then
                e.self:Say("Ah, you're here about Valen's shipment? We tracked the [bandits] to West Karana.")
            else
                e.self:Say("Any luck with those bandits?")
            end
        else
            e.self:Say("Move along, citizen. Official business.")
        end

    elseif e.message:findi("bandits") then
        if e.other:IsTaskActive(7010) then
            e.self:Say("They've set up camp near the lake. Be careful, they're armed!")
            e.other:UpdateTaskActivity(7010, 0, 1)  -- Complete speak activity
        end
    end
end
```

---

## Best Practices Summary

1. **Use Task System for**:
   - Multi-step quests players need to track
   - Kill/collect objectives with progress bars
   - Repeatable content with lockouts
   - Group content

2. **Use Scripts for**:
   - Rich NPC dialogue
   - Complex conditional logic
   - One-time item exchanges
   - Faction management

3. **Combine both for**:
   - Story-driven content with tracking
   - Tutorial quests
   - Achievement-style content

4. **Always test**:
   - `#task assign X` to quickly test
   - `#reloadqst` after script changes
   - `#reloadtasks` after SQL changes
