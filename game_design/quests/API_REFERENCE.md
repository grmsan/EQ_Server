# Quest API Reference

Complete API documentation for quest scripting in Lua.

## Table of Contents

1. [Client Methods](#client-methods)
2. [NPC Methods](#npc-methods)
3. [Global eq Functions](#global-eq-functions)
4. [Trade Handling](#trade-handling)
5. [Task Functions](#task-functions)
6. [Inventory Functions](#inventory-functions)
7. [Faction Functions](#faction-functions)
8. [Data Buckets](#data-buckets)
9. [Spawning & NPCs](#spawning--npcs)
10. [Timers & Signals](#timers--signals)
11. [Zone Functions](#zone-functions)

---

## Client Methods

Called on `e.other` (the player) in most quest events.

### Identity & Info

```lua
e.other:GetName()           -- Returns character name (string)
e.other:GetCleanName()      -- Returns name without numbers/symbols
e.other:GetID()             -- Returns entity ID (int)
e.other:CharacterID()       -- Returns character_id from DB (int)
e.other:AccountID()         -- Returns account_id (int)
e.other:AccountName()       -- Returns account name (string)
e.other:GetClass()          -- Returns class ID (int, see classes.h)
e.other:GetClassName()      -- Returns class name (string)
e.other:GetRace()           -- Returns race ID (int)
e.other:GetRaceName()       -- Returns race name (string)
e.other:GetLevel()          -- Returns level (int)
e.other:GetGender()         -- Returns gender (0=male, 1=female, 2=neuter)
e.other:GetDeity()          -- Returns deity ID (int)
e.other:GetGM()             -- Returns GM flag (bool)
```

### Stats

```lua
e.other:GetSTR()            -- Returns current strength
e.other:GetSTA()            -- Returns current stamina
e.other:GetAGI()            -- Returns current agility
e.other:GetDEX()            -- Returns current dexterity
e.other:GetWIS()            -- Returns current wisdom
e.other:GetINT()            -- Returns current intelligence
e.other:GetCHA()            -- Returns current charisma
e.other:GetMaxHP()          -- Returns max HP
e.other:GetHP()             -- Returns current HP
e.other:GetMaxMana()        -- Returns max mana
e.other:GetMana()           -- Returns current mana
e.other:GetAC()             -- Returns armor class
e.other:GetATK()            -- Returns attack rating
```

### Position

```lua
e.other:GetX()              -- X coordinate (float)
e.other:GetY()              -- Y coordinate (float)
e.other:GetZ()              -- Z coordinate (float)
e.other:GetHeading()        -- Heading direction (float)
e.other:GetZoneID()         -- Current zone ID (int)
```

### Rewards

```lua
-- Give currency (copper, silver, gold, platinum)
e.other:GiveCash(copper, silver, gold, platinum)
e.other:GiveCash(0, 0, 5, 0)    -- 5 gold

-- Take currency
e.other:TakeCash(amount)        -- Take from inventory (copper)

-- Give experience
e.other:AddEXP(amount)          -- Add flat XP
e.other:AddEXP(amount, mode)    -- mode: 0=normal, 1=group, 2=raid

-- Give AA
e.other:AddAAPoints(points)     -- Add AA points

-- Give items
e.other:SummonItem(item_id)                    -- To cursor
e.other:SummonItem(item_id, charges)           -- With charges
e.other:SummonItem(item_id, charges, attune)   -- attune: true/false
e.other:SummonItemIntoInventory(item_id)       -- To first open slot
```

### Inventory Checks

```lua
e.other:HasItem(item_id)                -- Has item anywhere (bool)
e.other:CountItem(item_id)              -- Count of item (int)
e.other:GetItemInSlot(slot_id)          -- ItemInst at slot (or nil)
e.other:DeleteItemInInventory(item_id, count, update)  -- Remove items
```

### Skills & Spells

```lua
e.other:GetSkill(skill_id)              -- Get skill level
e.other:SetSkill(skill_id, value)       -- Set skill level
e.other:MaxSkill(skill_id)              -- Get max trainable skill
e.other:HasSpellScribed(spell_id)       -- Has spell in book (bool)
e.other:ScribeSpell(spell_id, slot)     -- Scribe spell to book
e.other:MemSpell(spell_id, slot)        -- Memorize spell
e.other:CastSpell(spell_id, target_id)  -- Cast spell
```

### Communication

```lua
e.other:Message(type, "text")           -- Send message to player
e.other:Message(Chat.White, "Hello!")   -- Using Chat constants
e.other:Message(15, "Yellow text")      -- 15 = yellow
e.other:Emote("smiles warmly.")         -- Player emotes

-- Message types commonly used:
-- Chat.White = 0, Chat.Red = 13, Chat.Yellow = 15
-- Chat.Blue = 4, Chat.Green = 2
```

### Movement

```lua
e.other:MovePC(zone_id, x, y, z, heading)  -- Zone player
e.other:MovePC(zone_id, x, y, z)           -- Use default heading
e.other:MovePCInstance(zone_id, instance_id, x, y, z, heading)
```

### Groups & Raids

```lua
e.other:GetGroup()           -- Returns Group object or nil
e.other:GetRaid()            -- Returns Raid object or nil
e.other:IsGrouped()          -- Is in a group (bool)
e.other:IsRaidGrouped()      -- Is in a raid (bool)
```

---

## NPC Methods

Called on `e.self` (the NPC) in NPC events.

### Communication

```lua
e.self:Say("Hello, adventurer!")        -- NPC speaks
e.self:Shout("Hear me!")                -- NPC shouts
e.self:Emote("bows respectfully.")      -- NPC emotes
e.self:Quest("[Tell] me more.")         -- Linkable text
```

### Identity

```lua
e.self:GetName()             -- NPC name
e.self:GetCleanName()        -- Clean name
e.self:GetID()               -- Entity ID
e.self:GetNPCTypeID()        -- NPC Type ID from database
e.self:GetLevel()            -- NPC level
```

### Position & Movement

```lua
e.self:GetX()                -- X coordinate
e.self:GetY()                -- Y coordinate
e.self:GetZ()                -- Z coordinate
e.self:GetHeading()          -- Current heading
e.self:MoveTo(x, y, z, h, save)  -- Move NPC to location
e.self:FaceTarget(target)    -- Turn to face target
e.self:Depop()               -- Remove NPC from zone
e.self:Depop(start_spawn_timer)  -- Depop and optionally restart spawn
```

### Combat

```lua
e.self:GetTarget()           -- Current target (Mob or nil)
e.self:SetTarget(mob)        -- Set target
e.self:AddToHateList(mob, hate, damage)
e.self:RemoveFromHateList(mob)
e.self:Attack(target)        -- Start attacking
e.self:IsEngaged()           -- In combat (bool)
```

### Stats & HP

```lua
e.self:GetHP()               -- Current HP
e.self:GetMaxHP()            -- Max HP
e.self:SetHP(amount)         -- Set HP
e.self:Heal()                -- Heal to full
e.self:Kill()                -- Kill NPC
```

---

## Global eq Functions

Utility functions available via the `eq` namespace.

### Logging & Debug

```lua
eq.debug("Debug message")           -- Write to debug log
eq.log(level, "Message")            -- Log at specific level
eq.zone_emote(type, "Text")         -- Zone-wide emote
eq.world_emote(type, "Text")        -- World-wide emote
```

### Zone Info

```lua
eq.get_zone_id()                    -- Current zone ID
eq.get_zone_short_name()            -- Zone short name
eq.get_zone_long_name()             -- Zone long name
eq.get_zone_instance_id()           -- Instance ID (0 if none)
eq.get_zone_instance_version()      -- Instance version
```

### Entity List

```lua
eq.get_entity_list()                -- Returns EntityList object
local el = eq.get_entity_list()
el:GetClientByName("PlayerName")    -- Find client
el:GetNPCByNPCTypeID(npc_type_id)   -- Find NPC
el:GetMobByID(entity_id)            -- Find any mob
```

### Time

```lua
eq.clock()                          -- Game time (table with hour, minute)
eq.seconds()                        -- Seconds since server start
eq.real_time()                      -- Real world timestamp
```

### Random

```lua
eq.ChooseRandom(1, 2, 3, 4)         -- Pick random from list
eq.ChooseRandom("a", "b", "c")      -- Works with any type
math.random(1, 100)                 -- Lua standard random
```

---

## Trade Handling

### The items Module

```lua
local item_lib = require("items")

-- Check if specific items were turned in
if item_lib.check_turn_in(e.trade, {item1 = 13917}) then
    -- Player gave item 13917
end

-- Check multiple items
if item_lib.check_turn_in(e.trade, {
    item1 = 13917,
    item2 = 13917,
    item3 = 13918,
    item4 = 13919
}) then
    -- Player gave all 4 items
end

-- Check money
if item_lib.check_turn_in(e.trade, {platinum = 10}) then
    -- Player gave 10 platinum
end

-- Return items not accepted
item_lib.return_items(e.self, e.other, e.trade)
```

### Trade Table Structure

```lua
e.trade = {
    item1 = ItemInst or nil,   -- First item slot
    item2 = ItemInst or nil,   -- Second item slot
    item3 = ItemInst or nil,   -- Third item slot
    item4 = ItemInst or nil,   -- Fourth item slot
    platinum = 0,              -- Platinum given
    gold = 0,                  -- Gold given
    silver = 0,                -- Silver given
    copper = 0,                -- Copper given
    self = NPC,                -- The NPC
    other = Client             -- The player
}
```

### ItemInst Methods

```lua
local item = e.trade.item1
if item then
    item:GetID()               -- Item ID
    item:GetName()             -- Item name
    item:GetCharges()          -- Remaining charges
    item:IsStackable()         -- Can stack (bool)
    item:GetStackSize()        -- Stack count
end
```

---

## Task Functions

### On Client

```lua
-- Task assignment
e.other:AssignTask(task_id)             -- Assign task directly
e.other:AssignTask(task_id, npc_id)     -- Assign with NPC

-- Task state checks
e.other:IsTaskActive(task_id)           -- Task in progress (bool)
e.other:IsTaskCompleted(task_id)        -- Ever completed (bool)
e.other:IsTaskActivityActive(task_id, activity_id)  -- Activity in progress

-- Task progress
e.other:UpdateTaskActivity(task_id, activity_id, count)  -- Add progress
e.other:FailTask(task_id)               -- Fail the task
e.other:RemoveTask(task_id)             -- Remove without failing
```

### Global Task Functions

```lua
-- Show task selection UI
eq.task_selector({5000, 5001, 5002})    -- List of task IDs

-- Show all tasks in a set
eq.task_set_selector(100)               -- Task set ID

-- Get task progress
eq.get_task_activity_done_count(task_id, activity_id)  -- Done count (int)

-- Shared tasks
eq.shared_task_selector({6000, 6001})   -- For group leader
```

---

## Inventory Functions

### Checking Items

```lua
-- Does player have item?
if e.other:HasItem(13917) then
    -- Has it somewhere in inventory
end

-- How many?
local count = e.other:CountItem(13917)

-- Check specific slot
local item = e.other:GetItemInSlot(1)  -- Slot 1
if item then
    local id = item:GetID()
end
```

### Slot Constants

```lua
-- Main inventory slots
local SLOT_CHARM = 0
local SLOT_EAR1 = 1
local SLOT_HEAD = 2
local SLOT_FACE = 3
local SLOT_EAR2 = 4
local SLOT_NECK = 5
local SLOT_SHOULDER = 6
local SLOT_ARMS = 7
local SLOT_BACK = 8
local SLOT_WRIST1 = 9
local SLOT_WRIST2 = 10
local SLOT_RANGE = 11
local SLOT_HANDS = 12
local SLOT_PRIMARY = 13
local SLOT_SECONDARY = 14
local SLOT_RING1 = 15
local SLOT_RING2 = 16
local SLOT_CHEST = 17
local SLOT_LEGS = 18
local SLOT_FEET = 19
local SLOT_WAIST = 20
local SLOT_POWERSOURCE = 21
local SLOT_AMMO = 22

-- General inventory starts at 23
```

### Removing Items

```lua
-- Remove items from inventory
e.other:DeleteItemInInventory(item_id, count, update_client)
e.other:DeleteItemInInventory(13917, 1, true)  -- Remove 1, update client
```

---

## Faction Functions

### Checking Faction

```lua
local faction = e.other:GetFactionValue(faction_id)

-- Faction thresholds:
-- 1100+ = Ally
-- 750-1099 = Warmly
-- 500-749 = Kindly
-- 100-499 = Amiably
-- 0-99 = Indifferently
-- -100 to -1 = Apprehensively
-- -500 to -101 = Dubiously
-- -750 to -501 = Threateningly
-- Below -750 = Scowls (KOS)

if faction >= 100 then
    -- Amiable or better
end
```

### Modifying Faction

```lua
-- Adjust faction
e.other:Faction(faction_id, amount)
e.other:Faction(219, 10)     -- +10 with faction 219
e.other:Faction(336, -5)     -- -5 with faction 336

-- Set faction directly (use sparingly)
e.other:SetFactionValue(faction_id, value)
```

### Common Faction IDs

```lua
-- Check your database's `faction_list` table for server-specific IDs
-- Examples from classic EQ:
-- 219 = Guards of Qeynos
-- 262 = Antonican Bards
-- 330 = Merchants of Qeynos
-- 331 = Sabertooths of Blackburrow
-- 336 = Coalition of Tradefolk
```

---

## Data Buckets

Persistent key-value storage for quest progress.

### Character Buckets

```lua
-- Get value (returns string or nil)
local value = e.other:GetBucket("quest_stage")

-- Set value
e.other:SetBucket("quest_stage", "2")

-- Delete bucket
e.other:DeleteBucket("quest_stage")

-- Set with expiration (seconds)
e.other:SetBucket("temp_flag", "1", 3600)  -- Expires in 1 hour
```

### NPC Buckets

```lua
-- NPC-specific storage
e.self:GetBucket("npc_data")
e.self:SetBucket("npc_data", "value")
e.self:DeleteBucket("npc_data")
```

### Global Buckets (via eq)

```lua
-- Server-wide storage
eq.get_data("global_event_stage")
eq.set_data("global_event_stage", "active")
eq.delete_data("global_event_stage")
```

### Best Practices

```lua
-- Use descriptive keys
e.other:SetBucket("lost_heirloom_quest_stage", "1")

-- Parse numeric values
local stage = tonumber(e.other:GetBucket("quest_stage")) or 0

-- Check for nil
local value = e.other:GetBucket("key")
if value then
    -- Key exists
else
    -- Key doesn't exist
end
```

---

## Spawning & NPCs

### Spawn NPCs

```lua
-- Spawn by NPC Type ID
eq.spawn2(npc_type_id, grid, unused, x, y, z, heading)
eq.spawn2(12345, 0, 0, 100, 200, 10, 64)

-- Spawn from spawn group
eq.spawn_from_spawn_group(spawn_group_id, x, y, z, heading)

-- Spawn condition
eq.spawn_condition(zone, instance_id, condition_id, value)
```

### Unique Spawns

```lua
-- Only spawn if not already up
local list = eq.get_entity_list()
local existing = list:GetNPCByNPCTypeID(12345)
if not existing then
    eq.spawn2(12345, 0, 0, 100, 200, 10, 64)
end
```

### Depop NPCs

```lua
e.self:Depop()              -- Remove immediately
e.self:Depop(true)          -- Remove and restart spawn timer
```

---

## Timers & Signals

### Timers

```lua
-- Set a named timer (milliseconds)
eq.set_timer("timer_name", 5000)   -- 5 seconds

-- Stop a timer
eq.stop_timer("timer_name")

-- Stop all timers
eq.stop_all_timers()

-- Handle timer event
function event_timer(e)
    if e.timer == "timer_name" then
        e.self:Say("Timer fired!")
        eq.stop_timer("timer_name")  -- Stop repeating
    end
end
```

### Timer Patterns

```lua
-- One-shot timer
function event_spawn(e)
    eq.set_timer("despawn", 60000)  -- 1 minute
end

function event_timer(e)
    if e.timer == "despawn" then
        e.self:Depop()
    end
end

-- Repeating timer (don't stop it)
function event_spawn(e)
    eq.set_timer("patrol_shout", 30000)  -- Every 30 seconds
end

function event_timer(e)
    if e.timer == "patrol_shout" then
        e.self:Shout("All clear on the western wall!")
        -- Timer continues automatically
    end
end
```

### Signals

```lua
-- Send signal to NPC type
eq.signal(npc_type_id, signal_id)
eq.signal(12345, 1)  -- Signal all NPCs of type 12345

-- Send signal to specific entity
eq.signal(npc_type_id, signal_id, wait_ms)
eq.signal(12345, 1, 5000)  -- Signal after 5 seconds

-- Handle signal
function event_signal(e)
    if e.signal == 1 then
        e.self:Say("I received signal 1!")
    elseif e.signal == 2 then
        e.self:Shout("Alert! Alert!")
    end
end
```

---

## Zone Functions

### Zone Players

```lua
-- Move player to zone
e.other:MovePC(zone_id, x, y, z, heading)
e.other:MovePC(1, 0, 0, 0, 0)  -- South Qeynos, safe coords

-- Move to instance
e.other:MovePCInstance(zone_id, instance_id, x, y, z, heading)
```

### Zone-wide Messages

```lua
-- Emote to entire zone
eq.zone_emote(type, "A great roar echoes through the zone!")

-- Message to entire zone
local list = eq.get_entity_list()
local clients = list:GetClientList()
for client in clients.entries do
    client:Message(15, "Zone message!")
end
```

### Zone Info

```lua
eq.get_zone_id()                -- Current zone ID
eq.get_zone_short_name()        -- "qeynos", "gfaydark", etc.
eq.get_zone_long_name()         -- "South Qeynos", etc.
eq.get_zone_instance_id()       -- Instance ID (0 if static)
```

---

## Quick Reference

### Message Types (Chat Colors)

| Value | Name | Color |
|-------|------|-------|
| 0 | White | White |
| 2 | Green | Green |
| 4 | Blue | Blue |
| 5 | Light Blue | Light Blue |
| 13 | Red | Red |
| 15 | Yellow | Yellow |
| 18 | Cyan | Cyan |

### Event Function Names

| Event | Function | Fires When |
|-------|----------|------------|
| NPC spawn | `event_spawn(e)` | NPC enters world |
| Player speaks | `event_say(e)` | Player talks to NPC |
| Item trade | `event_trade(e)` | Player gives items |
| NPC death | `event_death(e)` | NPC starts dying |
| Death complete | `event_death_complete(e)` | NPC finished dying |
| Timer | `event_timer(e)` | Timer fires |
| Signal | `event_signal(e)` | Signal received |
| Combat | `event_combat(e)` | Combat state changes |
| Enter area | `event_enter(e)` | Player enters proximity |
| Exit area | `event_exit(e)` | Player exits proximity |
| Waypoint | `event_waypoint_arrive(e)` | NPC reaches waypoint |

### Event Object Properties

| Event | Key Properties |
|-------|---------------|
| `event_say` | `e.self` (NPC), `e.other` (client), `e.message` (text) |
| `event_trade` | `e.self`, `e.other`, `e.trade` (trade table) |
| `event_death` | `e.self`, `e.other` (killer), `e.killer_id`, `e.damage` |
| `event_timer` | `e.self`, `e.timer` (timer name) |
| `event_signal` | `e.self`, `e.signal` (signal ID) |
| `event_combat` | `e.self`, `e.joined` (bool) |
