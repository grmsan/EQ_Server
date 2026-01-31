# Scripted Quests (Lua/Perl)

Traditional quest system using server-side scripts. NPCs respond to player actions through event handlers.

## Table of Contents

1. [File Organization](#file-organization)
2. [Script Naming Convention](#script-naming-convention)
3. [Event Types](#event-types)
4. [Common Quest Patterns](#common-quest-patterns)
5. [Item Turn-ins](#item-turn-ins)
6. [Rewards](#rewards)
7. [Lua vs Perl](#lua-vs-perl)
8. [Global Scripts](#global-scripts)
9. [Debugging](#debugging)

---

## File Organization

Quest scripts are stored in `/quests/` organized by zone:

```
/quests/
├── global/              # Runs for ALL zones
│   ├── global_npc.lua   # All NPCs everywhere
│   ├── global_player.lua # All player events
│   └── Priest_of_Discord.lua
├── rivervale/           # Zone-specific
│   ├── Fiddy_Bobick.lua
│   └── Marshal_Anrey.lua
├── freeporte/           # Freeport East
├── freportw/            # Freeport West
└── plugins/             # Reusable Perl modules
```

## Script Naming Convention

Scripts must match the NPC's "clean name" (underscores replace spaces):

| NPC Name | Script Filename |
|----------|----------------|
| Fiddy Bobick | `Fiddy_Bobick.lua` |
| a training dummy | `a_training_dummy.lua` |
| Guard #13 | `Guard_#13.lua` |

Alternatively, use NPC Type ID: `19086.lua`

## Event Types

### Core NPC Events

```lua
function event_say(e)
    -- Triggered when player talks to NPC
    -- e.message = what player said
    -- e.other = the client who spoke
    -- e.self = the NPC
end

function event_trade(e)
    -- Triggered when player gives items to NPC
    -- e.trade = table with item1-item4, platinum, gold, silver, copper
    -- e.other = the client
    -- e.self = the NPC
end

function event_spawn(e)
    -- NPC spawned in zone
end

function event_death(e)
    -- NPC is dying
    -- e.other = killer
end

function event_death_complete(e)
    -- NPC finished dying (safe to access corpse)
    -- e.other = killer
end

function event_combat(e)
    -- NPC entered/exited combat
    -- e.joined = true/false
end

function event_aggro(e)
    -- NPC aggroed on someone
    -- e.other = target
end

function event_slay(e)
    -- NPC killed a player
    -- e.other = dead player
end

function event_timer(e)
    -- Timer triggered
    -- e.timer = timer name
end

function event_signal(e)
    -- Signal received from another NPC/script
    -- e.signal = signal ID
end

function event_waypoint_arrive(e)
    -- NPC arrived at waypoint
    -- e.wp = waypoint number
end

function event_enter(e)
    -- Player entered NPC's proximity
    -- e.other = client
end

function event_exit(e)
    -- Player left NPC's proximity
    -- e.other = client
end
```

### Player Events (global_player.lua)

```lua
function event_enter_zone(e)
    -- Player entered zone
end

function event_level_up(e)
    -- Player leveled up
end

function event_task_accepted(e)
    -- Player accepted a task
    -- e.task_id = task ID
end

function event_task_complete(e)
    -- Player completed a task
    -- e.task_id = task ID
end

function event_loot(e)
    -- Player looted an item
    -- e.item = item instance
end
```

## Common Quest Patterns

### Basic Dialogue Quest

```lua
function event_say(e)
    if e.message:findi("hail") then
        e.self:Say("Greetings, " .. e.other:GetName() .. "! I am the town guard. Have you heard about the [wolf problem]?")
    elseif e.message:findi("wolf problem") then
        e.self:Say("Wolves have been attacking travelers! Bring me proof of their demise - perhaps some [wolf pelts] - and I shall reward you.")
    elseif e.message:findi("wolf pelts") then
        e.self:Say("Yes! Bring me 4 wolf pelts and I will pay you 5 gold pieces.")
    end
end
```

### String Matching Methods

```lua
-- Case-insensitive find (preferred)
if e.message:findi("hail") then

-- Exact match
if e.message == "Hail" then

-- Lua pattern matching
if e.message:match("[Hh]ail") then
```

## Item Turn-ins

### Using the items Module (Recommended)

```lua
function event_trade(e)
    local item_lib = require("items")

    -- Check for specific items (item IDs)
    if item_lib.check_turn_in(e.trade, {item1 = 13917, item2 = 13917, item3 = 13917, item4 = 13917}) then
        -- Player turned in 4x item 13917
        e.self:Say("Excellent work! Here is your reward.")
        e.other:GiveCash(0, 5, 0, 0)  -- 5 gold
        e.other:AddEXP(100)
    else
        -- Return items if not the right ones
        item_lib.return_items(e.self, e.other, e.trade)
    end
end
```

### Trade Table Structure

```lua
-- e.trade contains:
e.trade.item1      -- ItemInst object (or nil)
e.trade.item2      -- ItemInst object (or nil)
e.trade.item3      -- ItemInst object (or nil)
e.trade.item4      -- ItemInst object (or nil)
e.trade.platinum   -- amount of platinum
e.trade.gold       -- amount of gold
e.trade.silver     -- amount of silver
e.trade.copper     -- amount of copper
e.trade.self       -- the NPC
e.trade.other      -- the client
```

### Manual Item Checking

```lua
function event_trade(e)
    local item_lib = require("items")

    -- Check individual items
    if e.trade.item1 and e.trade.item1:GetID() == 13917 then
        e.self:Say("You brought me a wolf pelt!")
        -- Don't forget to handle other items
    else
        item_lib.return_items(e.self, e.other, e.trade)
    end
end
```

### Money Turn-ins

```lua
function event_trade(e)
    local item_lib = require("items")

    -- Check for money (10 platinum)
    if item_lib.check_turn_in(e.trade, {platinum = 10}) then
        e.self:Say("Thank you for your donation!")
        e.other:Faction(292, 10)  -- Increase faction
    else
        item_lib.return_items(e.self, e.other, e.trade)
    end
end
```

## Rewards

### Experience

```lua
e.other:AddEXP(100)           -- Flat amount
e.other:AddAAPoints(1)        -- AA points
```

### Money

```lua
-- GiveCash(copper, silver, gold, platinum)
e.other:GiveCash(0, 0, 5, 0)  -- 5 gold
e.other:GiveCash(50, 0, 0, 1) -- 1 plat 50 copper
```

### Items

```lua
-- Summon item to cursor
e.other:SummonItem(12345)                    -- Item ID
e.other:SummonItem(12345, 10)                -- 10 charges
e.other:SummonItem(12345, 1, false)          -- Not attuned

-- Summon to inventory (finds empty slot)
e.other:SummonItemIntoInventory(12345)
```

### Faction

```lua
-- Faction(faction_id, amount)
e.other:Faction(292, 10)   -- +10 with faction 292
e.other:Faction(336, -5)   -- -5 with faction 336
```

### Common Faction IDs (examples)

```lua
-- Rivervale area
292  -- Merchants of Rivervale
241  -- Deeppockets
263  -- Guardians of the Vale
286  -- Mayor Gubbin
336  -- Coalition of Tradefolk Underground
```

## Lua vs Perl

Both languages are supported. **Lua is recommended** for new scripts.

### Lua Advantages
- Better performance
- Cleaner syntax
- Active development focus
- Modern language features

### Perl Still Works
- Legacy scripts still function
- Extensive plugin library in `/quests/plugins/`

### Perl Example (for reference)

```perl
sub EVENT_SAY {
    if ($text =~ /hail/i) {
        quest::say("Greetings, $name!");
    }
}

sub EVENT_ITEM {
    if (plugin::check_handin(\%itemcount, 13917 => 4)) {
        quest::say("Thank you for the pelts!");
        quest::exp(100);
        quest::givecash(0, 5, 0, 0);
    }
    plugin::return_items(\%itemcount);
}
```

## Global Scripts

### global_npc.lua

Runs for ALL NPCs in all zones. Useful for server-wide behaviors.

```lua
function event_spawn(e)
    -- Every NPC that spawns runs this
end

function event_death_complete(e)
    -- Every NPC death runs this
    -- Used for drop tables, achievements, etc.
end
```

### global_player.lua

Runs for all player events.

```lua
function event_enter_zone(e)
    -- Player entered any zone
    e.self:Message(15, "Welcome to " .. eq.get_zone_long_name())
end

function event_level_up(e)
    -- Player leveled up anywhere
end
```

## Debugging

### Logging

```lua
function event_say(e)
    eq.debug(string.format("Player %s said: %s", e.other:GetName(), e.message))
    -- Check server logs or use #logs command
end
```

### Common Issues

1. **Script not loading**: Check filename matches NPC's clean name exactly
2. **Events not firing**: Verify function names are correct (`event_say`, not `EVENT_SAY`)
3. **Items not being consumed**: Make sure `check_turn_in` returns true
4. **Items not returning**: Always call `return_items` in else branch

### Testing In-Game

```
#reloadqst           -- Reload all quest scripts
#reloadqst zonename  -- Reload specific zone
#repop               -- Repop zone to test spawn events
```

## Advanced Topics

### Timers

```lua
function event_spawn(e)
    eq.set_timer("shout", 60000)  -- 60 second timer
end

function event_timer(e)
    if e.timer == "shout" then
        e.self:Shout("I am still here!")
    end
end
```

### Signals Between NPCs

```lua
-- NPC 1
function event_death_complete(e)
    eq.signal(12345, 1)  -- Signal NPC type ID 12345 with signal 1
end

-- NPC 2 (type ID 12345)
function event_signal(e)
    if e.signal == 1 then
        e.self:Say("My friend has fallen!")
    end
end
```

### Proximity Triggers

```lua
function event_spawn(e)
    eq.set_proximity(-100, 100, -100, 100)  -- x_min, x_max, y_min, y_max
end

function event_enter(e)
    e.other:Message(15, "You sense something watching you...")
end
```

### Spawning NPCs

```lua
-- Spawn NPC by type ID at location
eq.spawn2(12345, 0, 0, e.self:GetX(), e.self:GetY(), e.self:GetZ(), 0)

-- Spawn at NPC's location
eq.spawn2(12345, 0, 0, e.self:GetX(), e.self:GetY(), e.self:GetZ(), e.self:GetHeading())
```

---

## See Also

- [TASK_SYSTEM.md](TASK_SYSTEM.md) - For UI-tracked quests
- [EXAMPLES.md](EXAMPLES.md) - More practical examples
