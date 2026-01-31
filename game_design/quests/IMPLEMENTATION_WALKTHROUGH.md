# Quest Implementation Walkthrough

Step-by-step guide for creating quests from scratch.

## Table of Contents

1. [Before You Start](#before-you-start)
2. [Workflow Overview](#workflow-overview)
3. [Creating a Scripted Quest](#creating-a-scripted-quest)
4. [Creating a Task-Based Quest](#creating-a-task-based-quest)
5. [Creating a Hybrid Quest](#creating-a-hybrid-quest)
6. [Best Practices](#best-practices)
7. [Checklist](#checklist)

---

## Before You Start

### Prerequisites

1. **Database access** - MySQL client or GUI tool
2. **File access** - Ability to create/edit files in `/quests/`
3. **GM account** - Status 80+ for testing commands
4. **Test character** - Character you can use for testing

### Planning Questions

Before coding, answer these:

1. **What type of quest?**
   - Simple turn-in → Scripted quest
   - Kill/collect with tracking → Task system
   - Story with tracking → Hybrid

2. **Where does it take place?**
   - Single zone or multiple zones?
   - Specific NPCs needed?

3. **What are the requirements?**
   - Level range?
   - Faction requirements?
   - Prerequisites?

4. **What are the rewards?**
   - Experience?
   - Items?
   - Money?
   - Faction?

5. **Is it repeatable?**
   - One-time only?
   - Daily/weekly lockout?
   - Infinitely repeatable?

---

## Workflow Overview

```
1. Design the quest on paper
       ↓
2. Create any needed items (database)
       ↓
3. Create any needed NPCs (database)
       ↓
4. Create task definition (if using tasks)
       ↓
5. Write quest scripts (Lua)
       ↓
6. Test in-game
       ↓
7. Iterate and fix issues
       ↓
8. Final testing and polish
```

---

## Creating a Scripted Quest

Let's create a simple turn-in quest from scratch.

### Step 1: Design

**Quest: Proof of Valor**
- NPC: Warrior Trainer in Qeynos
- Objective: Bring 4 orc scalps as proof of combat prowess
- Reward: 1 gold, 100 XP, +10 Warriors of Qeynos faction

### Step 2: Check for Existing Resources

```sql
-- Find orc scalp item (or create one)
SELECT id, name FROM items WHERE name LIKE '%orc%scalp%';

-- Find the NPC
SELECT id, name, zone FROM npc_types
WHERE name LIKE '%warrior%trainer%' AND zone = 'qeynos';
```

### Step 3: Create Item (if needed)

```sql
-- If orc scalp doesn't exist:
INSERT INTO items (id, name, lore, idfile, itemtype, nodrop, norent, questitemflag)
VALUES (300001, 'Orc Scalp', 'Proof of a slain orc', 'IT63', 0, 0, 0, 1);
```

### Step 4: Set Up Loot (if needed)

```sql
-- Add to orc loot table (find orc's loottable_id first)
SELECT loottable_id FROM npc_types WHERE name LIKE 'an_orc%' LIMIT 1;

-- Add scalp to loot table
INSERT INTO lootdrop (id, name) VALUES (300001, 'Orc_Scalp_Drop');
INSERT INTO lootdrop_entries (lootdrop_id, item_id, item_charges, equip_item, chance, minlevel, maxlevel)
VALUES (300001, 300001, 1, 0, 25, 0, 0);

-- Link to loottable
INSERT INTO loottable_entries (loottable_id, lootdrop_id, multiplier, probability)
VALUES (ORC_LOOTTABLE_ID, 300001, 1, 100);
```

### Step 5: Create the Script

Create file: `quests/qeynos/Weapon_Master_Hanns.lua`

```lua
--[[
    Quest: Proof of Valor
    NPC: Weapon Master Hanns (Warrior Trainer)
    Zone: South Qeynos

    Objective: Turn in 4 orc scalps
    Rewards: 1 gold, 100 XP, faction
]]

local QUEST_ITEM = 300001  -- Orc Scalp
local ITEMS_REQUIRED = 4

function event_say(e)
    if e.message:findi("hail") then
        e.self:Say("Hail, " .. e.other:GetName() .. "! " ..
            "Are you here to train, or perhaps prove your [valor]?")

    elseif e.message:findi("valor") then
        e.self:Say("A true warrior proves themselves through combat! " ..
            "Bring me proof of your battles - [orc scalps] from the " ..
            "beasts that plague our lands.")

    elseif e.message:findi("orc scalps") then
        e.self:Say("Yes! Bring me four orc scalps and I shall reward " ..
            "your dedication to the warrior's path.")
    end
end

function event_trade(e)
    local item_lib = require("items")

    -- Check for 4 orc scalps
    if item_lib.check_turn_in(e.trade, {
        item1 = QUEST_ITEM,
        item2 = QUEST_ITEM,
        item3 = QUEST_ITEM,
        item4 = QUEST_ITEM
    }) then
        -- Success!
        e.self:Say("Excellent work, " .. e.other:GetName() .. "! " ..
            "You have proven yourself a capable warrior. " ..
            "Take this gold as payment for your service!")

        -- Rewards
        e.other:GiveCash(0, 0, 1, 0)  -- 1 gold
        e.other:AddEXP(100)
        e.other:Faction(219, 10)       -- Warriors of Qeynos
        e.other:Faction(331, -5)       -- Orcs hate you more

    else
        -- Wrong items or not enough
        e.self:Say("These are not what I asked for.")
        item_lib.return_items(e.self, e.other, e.trade)
    end
end
```

### Step 6: Test

```
#reloadqst qeynos
#goto Weapon_Master_Hanns
```

Test dialogue:
1. Hail the NPC
2. Say "valor"
3. Say "orc scalps"

Test turn-in:
```
#si 300001        -- Summon 4 scalps
#si 300001
#si 300001
#si 300001
```

Trade all 4 to NPC.

---

## Creating a Task-Based Quest

Let's create a kill quest with automatic tracking.

### Step 1: Design

**Quest: Gnoll Menace**
- Type: Solo task
- Objective: Kill 10 gnolls in Blackburrow
- Reward: 200 XP, 50 silver

### Step 2: Create Task SQL

```sql
-- Choose a unique task ID
SELECT MAX(id) + 1 FROM tasks;  -- Let's say 300001

-- Create the task
INSERT INTO tasks (
    id, type, duration, duration_code,
    title, description, reward_text, reward_id_list,
    cash_reward, exp_reward, reward_method, reward_points, reward_point_type,
    min_level, max_level, level_spread, min_players, max_players,
    repeatable, faction_reward, completion_emote,
    replay_timer_group, replay_timer_seconds,
    request_timer_group, request_timer_seconds,
    dz_template_id, lock_activity_id, faction_amount, enabled
) VALUES (
    300001, 2, 0, 0,
    'Gnoll Menace',
    '[1,The guards need you to thin the gnoll population in Blackburrow. Kill 10 gnolls.]',
    '', '',
    500, 200, 0, 0, 0,           -- 5 silver, 200 XP
    5, 15, 0, 0, 0,              -- Levels 5-15
    1, 0,                        -- Repeatable, no faction
    'You have helped control the gnoll threat!',
    0, 0, 0, 0, 0, -1, 0, 1
);

-- Create the kill activity
INSERT INTO task_activities (
    taskid, activityid, req_activity_id, step,
    activitytype, target_name, goalmethod, goalcount,
    description_override, npc_match_list, item_id_list, item_list,
    dz_switch_id, min_x, min_y, min_z, max_x, max_y, max_z,
    skill_list, spell_list, zones, zone_version, optional, list_group
) VALUES (
    300001, 0, -1, 1,
    2,                          -- Kill type
    'gnolls', 0, 10,
    '',
    'a_gnoll|gnoll_',           -- Match any NPC starting with "a_gnoll" or "gnoll_"
    '', '', 0,
    0, 0, 0, 0, 0, 0,
    '-1', '0',
    '17',                       -- Blackburrow zone ID
    -1, 0, 0
);
```

### Step 3: Create Task Giver Script

Create: `quests/qeynos/Guard_Dunix.lua`

```lua
--[[
    Task Giver: Guard Dunix
    Task: Gnoll Menace (300001)
]]

local TASK_ID = 300001

function event_say(e)
    if e.message:findi("hail") then
        e.self:Say("Well met, citizen! The gnoll threat grows daily. " ..
            "Are you willing to [help] defend Qeynos?")

    elseif e.message:findi("help") then
        -- Check task status
        if e.other:IsTaskCompleted(TASK_ID) then
            e.self:Say("You've already helped us greatly. Thank you!")
        elseif e.other:IsTaskActive(TASK_ID) then
            e.self:Say("Still hunting gnolls? Check your quest journal for progress!")
        else
            e.self:Say("Head to Blackburrow and thin their numbers. " ..
                "Report back when you've slain ten of the beasts!")
            -- Show task selector
            eq.task_selector({TASK_ID})
        end
    end
end
```

### Step 4: Test

```
#reloadtasks
#reloadqst qeynos
#goto Guard_Dunix
```

1. Hail and say "help"
2. Accept the task
3. Check quest journal (Alt+Q)
4. Go to Blackburrow
5. Kill gnolls and watch progress update
6. Complete for reward

---

## Creating a Hybrid Quest

Combine scripted dialogue with task tracking.

### Step 1: Design

**Quest: The Missing Supplies**
- Talk to Merchant → Get task
- Find Guard Captain → Learn location
- Kill bandits → Auto-tracked
- Return to Merchant → Complete

### Step 2: Create Task

```sql
INSERT INTO tasks VALUES (
    300002, 2, 0, 0,
    'The Missing Supplies',
    '[1,Speak with Guard Captain Riles about the missing shipment.][2,Eliminate the bandits who stole the supplies.][3,Return to Merchant Valia with news of your success.]',
    'Merchant''s Thanks', '300010',  -- Reward item
    1000, 300, 0, 0, 0,
    10, 25, 0, 0, 0,
    0, 0,  -- Not repeatable
    'The trade routes are safe once more!',
    0, 0, 0, 0, 0, -1, 0, 1
);

-- Activity 0: Speak with Guard Captain (manual update)
INSERT INTO task_activities VALUES (
    300002, 0, -1, 1, 4,
    'Guard Captain Riles', 0, 1, '',
    'Guard_Captain_Riles', '', '', 0,
    0, 0, 0, 0, 0, 0,
    '-1', '0', '1', -1, 0, 0  -- Qeynos
);

-- Activity 1: Kill bandits (auto-tracked)
INSERT INTO task_activities VALUES (
    300002, 1, 0, 2, 2,
    'bandits', 0, 5, '',
    'a_bandit|bandit_', '', '', 0,
    0, 0, 0, 0, 0, 0,
    '-1', '0', '14', -1, 0, 0  -- West Karana
);

-- Activity 2: Return to merchant (manual update)
INSERT INTO task_activities VALUES (
    300002, 2, 1, 3, 4,
    'Merchant Valia', 0, 1, '',
    'Merchant_Valia', '', '', 0,
    0, 0, 0, 0, 0, 0,
    '-1', '0', '1', -1, 0, 0  -- Qeynos
);
```

### Step 3: Create Scripts

**Merchant_Valia.lua**
```lua
local TASK_ID = 300002

function event_say(e)
    if e.message:findi("hail") then
        if not e.other:IsTaskActive(TASK_ID) and not e.other:IsTaskCompleted(TASK_ID) then
            e.self:Say("Oh, adventurer! My supply shipment was [stolen]! Can you help?")
        elseif e.other:IsTaskActive(TASK_ID) then
            local bandits_done = eq.get_task_activity_done_count(TASK_ID, 1)
            if bandits_done >= 5 then
                e.self:Say("You defeated the bandits! Thank you so much!")
                e.other:UpdateTaskActivity(TASK_ID, 2, 1)
            else
                e.self:Say("Please, find out what happened to my supplies!")
            end
        else
            e.self:Say("Thank you again for your help!")
        end

    elseif e.message:findi("stolen") then
        if not e.other:IsTaskActive(TASK_ID) and not e.other:IsTaskCompleted(TASK_ID) then
            e.self:Say("Yes! Please speak with [Guard Captain Riles]. He may know something!")
            e.other:AssignTask(TASK_ID)
        end
    end
end
```

**Guard_Captain_Riles.lua**
```lua
local TASK_ID = 300002

function event_say(e)
    if e.message:findi("hail") then
        if e.other:IsTaskActive(TASK_ID) then
            local spoke = eq.get_task_activity_done_count(TASK_ID, 0)
            if spoke < 1 then
                e.self:Say("Ah, you're here about Valia's supplies? " ..
                    "We tracked the [bandits] to West Karana.")
            else
                e.self:Say("Did you find those bandits yet?")
            end
        else
            e.self:Say("Move along, citizen.")
        end

    elseif e.message:findi("bandits") then
        if e.other:IsTaskActive(TASK_ID) then
            e.self:Say("They've set up camp near the lake. Be careful!")
            e.other:UpdateTaskActivity(TASK_ID, 0, 1)
        end
    end
end
```

### Step 4: Test the Full Flow

1. Talk to Merchant Valia, say "stolen"
2. Accept task
3. Go to Guard Captain Riles, say "hail" then "bandits"
4. Go to West Karana, kill 5 bandits
5. Return to Merchant Valia, say "hail"
6. Receive reward

---

## Best Practices

### Script Organization

```lua
--[[
    Quest: Quest Name Here
    NPC: NPC Name
    Zone: Zone Name

    Description: Brief description

    Objectives:
    1. First objective
    2. Second objective

    Rewards:
    - X gold/plat
    - Y XP
    - Item Name
]]

-- Constants at top
local TASK_ID = 300001
local QUEST_ITEM = 50001
local REWARD_ITEM = 50002

-- Functions below
function event_say(e)
    -- ...
end
```

### Error Handling

```lua
function event_trade(e)
    local item_lib = require("items")

    -- Always return items on failure
    local success = false

    if e.other:IsTaskActive(TASK_ID) then
        if item_lib.check_turn_in(e.trade, {item1 = QUEST_ITEM}) then
            success = true
            -- Handle success
        end
    end

    if not success then
        item_lib.return_items(e.self, e.other, e.trade)
    end
end
```

### Faction Checking

```lua
function event_say(e)
    local faction = e.other:GetFactionValue(FACTION_ID)

    if faction < -100 then
        e.self:Say("I won't speak with the likes of you!")
        return  -- Early exit
    end

    -- Normal dialogue...
end
```

### Level Checking

```lua
function event_say(e)
    local level = e.other:GetLevel()

    if level < 10 then
        e.self:Say("You're too inexperienced for this task.")
        return
    end

    -- Quest dialogue...
end
```

---

## Checklist

### Before Creating

- [ ] Quest designed on paper
- [ ] Unique task/item IDs reserved
- [ ] Required NPCs identified
- [ ] Required items identified

### Database Setup

- [ ] Task created (if using task system)
- [ ] Activities created (if using task system)
- [ ] Items created (if new items needed)
- [ ] Loot tables updated (if items drop from NPCs)
- [ ] NPCs created (if new NPCs needed)
- [ ] Spawns created (if new NPCs)

### Script Creation

- [ ] Script file named correctly
- [ ] Header comment with quest info
- [ ] Constants defined at top
- [ ] All dialogue branches implemented
- [ ] Trade handling returns items on failure
- [ ] Level/faction checks if required

### Testing

- [ ] Script loads without errors
- [ ] All dialogue options work
- [ ] Turn-ins work correctly
- [ ] Wrong items are returned
- [ ] Task progress updates correctly
- [ ] Rewards given correctly
- [ ] Quest completable end-to-end

### Polish

- [ ] Dialogue is engaging and clear
- [ ] Instructions are unambiguous
- [ ] Rewards feel appropriate
- [ ] No exploits possible
