# Quest Testing Guide

How to test quests during and after development.

## Table of Contents

1. [Testing Environment](#testing-environment)
2. [GM Commands](#gm-commands)
3. [Quick Testing Workflow](#quick-testing-workflow)
4. [Debugging Scripts](#debugging-scripts)
5. [Testing Tasks](#testing-tasks)
6. [Common Issues](#common-issues)
7. [Test Checklist](#test-checklist)

---

## Testing Environment

### Required GM Status

Most testing commands require GM status 80+:

```sql
-- Grant GM status to test account
UPDATE account SET status = 200 WHERE name = 'TestAccount';
```

### In-Game GM Mode

```
#gm on              -- Enable GM mode (no aggro, see invisible, etc.)
#gm off             -- Disable GM mode
#invul on           -- Invulnerable
#invul off          -- Vulnerable
```

### Useful Setup Commands

```
#level 50           -- Set level
#setstat all 255    -- Max all stats
#heal               -- Full heal
#mana               -- Full mana
```

---

## GM Commands

### Quest Script Commands

```
#reloadqst                  -- Reload ALL quest scripts
#reloadqst zonename         -- Reload specific zone scripts
#reloadqst global           -- Reload global scripts only

#repop                      -- Repop all NPCs in zone
#depop                      -- Depop target NPC
#spawn npc_type_id          -- Spawn specific NPC
```

### Task Commands

```
#task show                  -- Show your active tasks
#task remove task_id        -- Remove a task
#task assign task_id        -- Assign task to yourself
#task update task_id act count  -- Update activity progress
#task complete task_id      -- Force complete task

#reloadtasks                -- Reload tasks from database
#reloadtasksets             -- Reload task sets
```

### Item Commands

```
#si item_id                 -- Summon item by ID
#si item_id 10              -- Summon 10 of item
#finditem name              -- Search for item by name

#delitem item_id            -- Delete item from inventory
#delitem item_id 5          -- Delete 5 of item
```

### Faction Commands

```
#faction faction_id         -- Show faction value
#setfaction faction_id val  -- Set faction value
#faction reset              -- Reset all faction to base
```

### Bucket Commands

```
#databucket show key        -- Show bucket value
#databucket set key value   -- Set bucket value
#databucket del key         -- Delete bucket
```

---

## Quick Testing Workflow

### Testing a New Script

1. **Write the script** in `/quests/zonename/NPC_Name.lua`

2. **Reload scripts:**
   ```
   #reloadqst zonename
   ```

3. **Find/Spawn the NPC:**
   ```
   #goto npc_name
   -- or --
   #spawn npc_type_id
   ```

4. **Test interaction:**
   - Hail the NPC
   - Check dialogue paths
   - Test item turn-ins

5. **Check for errors:**
   - Look at zone logs
   - Check server console

6. **Iterate:**
   - Edit script
   - `#reloadqst zonename`
   - Test again

### Testing a New Task

1. **Insert task SQL** into database

2. **Reload tasks:**
   ```
   #reloadtasks
   ```

3. **Assign task:**
   ```
   #task assign 5000
   ```

4. **Test activities:**
   - For Kill: `#spawn npc_type_id` and kill it
   - For Explore: Go to coordinates
   - For Loot: `#si item_id` then `#task update`

5. **Check progress:**
   ```
   #task show
   ```

6. **Force complete if needed:**
   ```
   #task complete 5000
   ```

### Testing Turn-ins

1. **Get the required item:**
   ```
   #si 13917           -- Summon item
   ```

2. **Open trade with NPC**

3. **Give item**

4. **Check:**
   - Did NPC say the right thing?
   - Did you get the reward?
   - Was the item consumed?

5. **If items returned unexpectedly:**
   - Check `item_lib.check_turn_in()` parameters
   - Verify item ID is correct
   - Make sure you're in the right `if` branch

---

## Debugging Scripts

### Enable Debug Logging

Add debug statements to your script:

```lua
function event_say(e)
    eq.debug(string.format("[DEBUG] Player %s said: %s",
        e.other:GetName(), e.message))

    if e.message:findi("hail") then
        eq.debug("[DEBUG] Matched hail")
        e.self:Say("Hello!")
    end
end
```

### Check Logs

Logs location:
- Zone logs: `logs/zone/zone_*.log`
- Quest logs: `logs/quests.log` (if configured)
- Console output

### Common Debug Patterns

```lua
-- Debug trade contents
function event_trade(e)
    eq.debug("=== Trade Debug ===")
    if e.trade.item1 then
        eq.debug("Item1: " .. e.trade.item1:GetID() .. " - " .. e.trade.item1:GetName())
    else
        eq.debug("Item1: nil")
    end
    if e.trade.item2 then
        eq.debug("Item2: " .. e.trade.item2:GetID())
    end
    eq.debug("Platinum: " .. (e.trade.platinum or 0))
    eq.debug("===================")

    -- Rest of trade logic...
end

-- Debug bucket values
function event_say(e)
    local stage = e.other:GetBucket("quest_stage") or "nil"
    eq.debug("Quest stage bucket: " .. stage)
end
```

### Live Debugging

```
#logs set quests 3    -- Enable quest logging
#logs set zone 3      -- Enable zone logging
```

---

## Testing Tasks

### Activity Type Testing

#### Kill Activities

```
-- Spawn the target NPC
#spawn npc_type_id

-- Kill it (or make it killable)
#damage 99999

-- Check progress
#task show

-- Manual update if needed
#task update task_id activity_id 1
```

#### Loot Activities

```
-- Summon the item
#si item_id

-- The task should update when you have the item
-- Or manually update:
#task update task_id activity_id 1
```

#### Explore Activities

```
-- Get coordinates
#loc

-- Go to the activity zone
#zone zonename

-- Walk into the coordinate box
-- Should auto-update

-- Debug if not working:
-- Check min_x, max_x, min_y, max_y, min_z, max_z in task_activities
```

#### Deliver Activities

```
-- Get the item
#si item_id

-- Find the NPC
#goto npc_name

-- Trade the item
-- Should trigger script which updates activity
```

### Checking Task State

```sql
-- Check task in database
SELECT * FROM tasks WHERE id = 5000;

-- Check activities
SELECT * FROM task_activities WHERE taskid = 5000;

-- Check player's active tasks
SELECT * FROM character_tasks WHERE charid = YOUR_CHAR_ID;

-- Check activity progress
SELECT * FROM character_activities WHERE charid = YOUR_CHAR_ID;
```

---

## Common Issues

### Script Not Loading

**Symptoms:** NPC doesn't respond to hail

**Solutions:**
1. Check filename matches NPC's clean name exactly (case-sensitive on Linux)
2. Verify file extension is `.lua` not `.lua.txt`
3. Check for Lua syntax errors:
   ```bash
   luac -p quests/zonename/NPC_Name.lua
   ```
4. Check zone logs for load errors
5. Try `#reloadqst zonename`

### Event Not Firing

**Symptoms:** `event_say` doesn't trigger

**Solutions:**
1. Verify function name is exactly `event_say` (not `EVENT_SAY`)
2. Check NPC is targetable and interactable
3. Verify you're close enough to the NPC
4. Check if global scripts are intercepting

### Turn-in Not Working

**Symptoms:** Items returned, quest doesn't progress

**Solutions:**
1. Print the item IDs being traded:
   ```lua
   if e.trade.item1 then
       eq.debug("Got item: " .. e.trade.item1:GetID())
   end
   ```
2. Verify item IDs match exactly
3. Check `check_turn_in` is using correct format:
   ```lua
   -- Correct:
   item_lib.check_turn_in(e.trade, {item1 = 13917})

   -- Wrong:
   item_lib.check_turn_in(e.trade, {13917})
   ```
4. Make sure you're not returning items in the success branch

### Task Not Updating

**Symptoms:** Kill/loot doesn't increment progress

**Solutions:**
1. Check `npc_match_list` matches NPC name:
   ```sql
   SELECT npc_match_list FROM task_activities WHERE taskid = 5000;
   ```
2. NPC name uses `|` as separator, not comma
3. Check `zones` field includes current zone
4. Verify task is active: `#task show`
5. Check `zone_version` if using instances

### Timer Not Firing

**Symptoms:** `event_timer` never runs

**Solutions:**
1. Verify timer was set with `eq.set_timer("name", ms)`
2. Check timer name matches in `event_timer`
3. Verify NPC hasn't been depopped
4. Check for typos in timer name (case-sensitive)

---

## Test Checklist

### For Each New Quest Script

- [ ] Script file named correctly (NPC clean name + .lua)
- [ ] Script loads without errors (`#reloadqst`)
- [ ] Hail response works
- [ ] All dialogue branches work
- [ ] Item turn-in works (if applicable)
- [ ] Correct items consumed
- [ ] Incorrect items returned
- [ ] Rewards given correctly:
  - [ ] Money
  - [ ] Experience
  - [ ] Items
  - [ ] Faction
- [ ] Quest repeatable/non-repeatable as designed
- [ ] Works for different classes/races (if restricted)
- [ ] Works at different levels (if restricted)

### For Each New Task

- [ ] Task appears in task selector
- [ ] Task can be assigned
- [ ] Task shows in quest journal
- [ ] Description displays correctly
- [ ] All activities track progress
- [ ] Activity chain (req_activity_id) works
- [ ] Step numbers display correctly
- [ ] Rewards given on completion
- [ ] Completion emote shows
- [ ] Repeatable/lockout works correctly
- [ ] Level requirements enforced

### Integration Testing

- [ ] Quest works end-to-end
- [ ] Multiple players can do quest simultaneously
- [ ] Quest survives zone crashes
- [ ] Quest survives server restarts
- [ ] Bucket data persists correctly
- [ ] No item duplication exploits
- [ ] No faction exploit possibilities

---

## Test Data Templates

### Quick SQL for Test Items

```sql
-- Create a test turn-in item
INSERT INTO items (id, name, lore, nodrop) VALUES
(99901, 'Test Quest Item', 'Quest Item', 1);

-- Create a test reward item
INSERT INTO items (id, name, lore, nodrop, ac) VALUES
(99902, 'Test Reward Ring', 'Reward', 0, 5);
```

### Quick SQL for Test NPC

```sql
-- Find an unused NPC type ID
SELECT MAX(id) + 1 FROM npc_types;

-- Create test NPC
INSERT INTO npc_types (id, name, level, class, race, hp, gender) VALUES
(999001, 'Test_Quest_Giver', 50, 1, 1, 10000, 0);

-- Add spawn point (South Qeynos near bank)
INSERT INTO spawn2 (spawngroupID, zone, x, y, z, heading, enabled) VALUES
(999001, 'qeynos', -150, 50, 3, 0, 1);

INSERT INTO spawngroup (id, name) VALUES
(999001, 'Test_Quest_Giver');

INSERT INTO spawnentry (spawngroupID, npcID, chance) VALUES
(999001, 999001, 100);
```

### Quick Test Script

```lua
-- quests/qeynos/Test_Quest_Giver.lua
-- Minimal test script for verifying setup

function event_spawn(e)
    eq.debug("Test_Quest_Giver spawned")
end

function event_say(e)
    eq.debug("Player " .. e.other:GetName() .. " said: " .. e.message)

    if e.message:findi("hail") then
        e.self:Say("Hello! Say [test] to test, or [item] to test turn-in.")
    elseif e.message:findi("test") then
        e.self:Say("Test successful! You said the magic word.")
        e.other:Message(15, "Quest script is working!")
    elseif e.message:findi("item") then
        e.self:Say("Give me any item to test turn-in handling.")
    end
end

function event_trade(e)
    local item_lib = require("items")

    if e.trade.item1 then
        e.self:Say("You gave me: " .. e.trade.item1:GetName() ..
                   " (ID: " .. e.trade.item1:GetID() .. ")")
        -- Return it for testing
        item_lib.return_items(e.self, e.other, e.trade)
    else
        e.self:Say("You didn't give me anything!")
    end
end
```
