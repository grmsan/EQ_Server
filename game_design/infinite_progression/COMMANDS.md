# Player Commands Reference

## ⚠️ IMPORTANT: Natural Progression System

**Players DON'T use commands!** Items upgrade automatically when killing mobs.

**How It Works:**
- Kill any mob: **5% chance** to upgrade a random equipped item
- Kill named/rare mob: **100% chance** to upgrade 1-3 random equipped items
- No player input needed - system handles everything

**Message Example:**
```
[You have slain a fire goblin!]
Your Short Sword +10 has been upgraded!
```

---

## GM Commands (Admin Only)

### #upgrade
Manually upgrade an item in a specific equipment slot.

**Syntax:**
```
#upgrade <slot_number> [levels]
```

**Parameters:**
- `slot_number`: Equipment slot (0-21, see slot reference below)
- `levels` (optional): Number of levels to add (default: 1)

**Slot Reference:**
```
0  = Charm        11 = Range
1  = Left Ear     12 = Hands
2  = Head         13 = Primary
3  = Face         14 = Secondary
4  = Right Ear    15 = Left Finger
5  = Neck         16 = Right Finger
6  = Shoulders    17 = Chest
7  = Arms         18 = Legs
8  = Back         19 = Feet
9  = Left Wrist   20 = Waist
10 = Right Wrist  21 = Ammo
```

**Examples:**
```
#upgrade 17          -- Upgrade chest item by 1 level
#upgrade 13 10       -- Upgrade primary weapon by 10 levels
#upgrade 2 100       -- Upgrade head item by 100 levels (testing)
```

**Implementation:**
- Called internally by Lua event system
- Can bypass GM status check via `SendGMCommand(cmd, true)`
- Used by auto-upgrade system

---

## TODO: Player-Accessible Commands

These commands exist in code but need player-safe implementations:

###  #fuse (TODO)
###  #fuse (TODO)
Transfer levels from cursor item (donor) to a worn/inventory item (receiver).

**Status:** GM command exists, needs player-safe version

**Proposed Syntax:**
```
#fuse [slot]
#fuse confirm
```

**Examples:**
```
#fuse chest       -- Fuse cursor item into chest slot
#fuse 17          -- Fuse into slot 17 (chest)
#fuse confirm     -- Execute fusion after preview
```

**TODO:**
- Add cost system (platinum, materials)
- Add confirmation prompts
- Prevent exploits (same item fusion)

---

### #iteminfo (TODO)
Display detailed information about an item's level and stats.

**Status:** Framework exists, needs implementation

**Proposed Syntax:**
```
#iteminfo [target]
```

**Examples:**
```
#iteminfo cursor    -- Info about cursor item
#iteminfo chest     -- Info about chest slot item
```

---

## Future Commands (Not Implemented)

### #reforge (Planned)
Reroll random stats on an item for a cost.

**Concept:**
```
#reforge          -- Reroll all random stats
#reforge 2        -- Keep 2 best stats, reroll the rest
```

### #extract (Planned)
Extract levels from an item into a consumable essence.

**Concept:**
```
#extract 10       -- Extract 10 levels into essence
```

### #itemstats (Planned)
Show current equipment stats summary.

**Concept:**
```
#itemstats        -- Display total stats from all equipment
```

---

## How Automatic Upgrades Work (CURRENT SYSTEM)

**File:** `quests/global/global_npc.lua`

**Trigger:** NPC death event

**Logic:**
1. Check if killer is a player
2. Determine if named/rare mob (100% chance) or regular (5% chance)
3. Roll for upgrade trigger
4. Pick 1-3 random equipped slots (more for named mobs)
5. Call `#upgrade <slot>` internally for each slot
6. Player sees message: "Your [item] has been upgraded!"

**Player Experience:**
- Natural progression through gameplay
- No commands to remember
- Clear feedback when items upgrade
- Strategic: equip items you want to upgrade

---

## GM Debug Commands

### #itemdebug (TODO)
Toggle debug output for item system.

**Proposed:**
```
#itemdebug on     -- Enable debug logging
#itemdebug off    -- Disable debug logging
```

### #setitemlevel (TODO)
Force set an item's level (testing only).

**Proposed:**
```
#setitemlevel 100     -- Set cursor item to level 100
#setitemlevel 0       -- Reset item to base level
```

---

## Quick Reference Table

| Command | Status | Purpose |
|---------|--------|---------|
| (Auto-upgrade) | ✅ WORKING | Items upgrade when killing mobs |
| `#upgrade <slot>` | ✅ GM-only | Manually upgrade equipped item |
| `#fuse` | ⏳ GM-only | Fuse items (needs player version) |
| `#iteminfo` | ⏳ TODO | View item details |
| `#itemstats` | 📋 Planned | View equipment summary |
| `#reforge` | 📋 Planned | Reroll stats |
| `#extract` | 📋 Planned | Extract levels to essence |

**Legend:**
- ✅ = Working now
- ⏳ = Partially implemented
- 📋 = Planned for future

## See Also
- [OVERVIEW.md](OVERVIEW.md) - System overview
- [SCALING_FORMULAS.md](SCALING_FORMULAS.md) - How stats scale
- [FUSION_SYSTEM.md](FUSION_SYSTEM.md) - Fusion mechanics
- [QUICK_START.md](QUICK_START.md) - Getting started

**Last Updated:** December 2, 2025
**System Status:** Auto-upgrades working, manual commands GM-only
