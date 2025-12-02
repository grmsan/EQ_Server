# Infinite Progression System - Overview

## Vision
A solo-focused EverQuest server where character power has **no ceiling**. Players can continue to progress infinitely through:
- Item level upgrades (unlimited)
- Item fusion (transfer progress to better base items)
- Dynamic stat allocation (items gain random stats as they level)
- Augment system for specialized bonuses

## Core Philosophy
**"Always something to chase"**
- Every boss kill = potential upgrade
- No gear caps or "BiS" (Best in Slot) limits
- Strategic choices matter (which items to level, when to fuse)
- Solo-friendly progression (no economy/trading required)

## System Components

### 1. Dynamic Item Levels
Items can level up infinitely. Each level increases stats and periodically adds new stats.

**Item ID Format:**
```
Dynamic ID: 5LLLLLIIIIII
  5      = Prefix (indicates dynamic item)
  LLLLL  = Level (00001 to 99999)
  IIIIII = Base Item ID

Example:
  Cloth Cap (ID 1001) at level 127
  = 500127001
```

### 2. Stat Scaling Rules
**Every Level:**
- All existing stats increase by their base increment
- AC: +1 per level
- HP: +2 per level
- Primary stats (if > 0): +1 per level
- Mana: +1 per level

**Every 5 Levels:**
- 50% chance to add a new random stat from available pool
- Stat pool includes: STR, STA, AGI, DEX, WIS, INT, CHA, Resistances

**Every 10 Levels:**
- Guaranteed milestone bonus:
  - Add heroic version of highest stat
  - OR add secondary stat (Attack, Haste, Regen)
  - OR upgrade stat tier

**Special Milestones:**
- Level 25: Unlock Haste (starts at 1%, +1% per 10 levels, caps at 100%)
- Level 50: Unlock Heroic stats
- Level 100: Add minor focus effect
- Level 200: Upgrade to major focus effect
- Level 500: Add proc
- Level 1000: Add click effect

### 3. Item Fusion
Transfer levels from one item to another (better base item).

**Mechanics:**
```
Donor Item: Cloth Cap +127 (3 AC base)
Receiver Item: Dragon Helm +0 (25 AC base)
Result: Dragon Helm +127 (25 AC base + 127 levels of scaling)
```

**Rules:**
- Donor item is destroyed
- Receiver gets donor's level
- Base stats come from receiver
- Scaling applied to receiver's base
- Random stats may carry over partially (configurable)

### 4. Loot System
**Boss Drops:**
- 100% chance to drop item upgrades
- Items drop at random level (1-5 for normal, 5-10 for named, 10-25 for raid)
- Can drop base items (for fusion purposes)
- Can drop augments (specialized bonuses)

**No Economy:**
- No player trading
- Merchant selling for currency
- Currency used to buy upgrade materials or reroll stats

## Progression Example

### Fresh Character
```
Level 1: Kill rat
  → Drop: Cloth Cap +1 (4 AC, +1 HP)

Level 10: Kill gnoll boss
  → Drop: Banded Mail +3 (15 AC, +8 STR, +6 HP)

Level 20: Manually upgrade Banded Mail
  → Banded Mail +20 (32 AC, +25 STR, +40 HP, +5 STA, +10 Mana)

Level 30: Find Plate Chestguard
  → Fuse Banded Mail +20 into Plate Chestguard
  → Result: Plate Chestguard +20 (better base + 20 levels)

Level 50: Continue upgrading
  → Plate Chestguard +50 (60 AC, +55 STR, +100 HP, +15 STA, +30 Mana, +5 Heroic STR, +3% Haste)

Level 100+: Endgame
  → Plate Chestguard +127 (hundreds of stats, multiple focus effects)
```

## Technical Implementation

### Architecture
- **C++ Server**: Dynamic item generation, stat scaling formulas
- **Lua Scripts**: Drop logic, upgrade triggers
- **Database**: Persistent storage for dynamic items
- **Client**: Standard EQ client (no mods needed)

### Key Files
- `zone/dynamic_item_manager.h/cpp` - Core scaling system
- `common/item_instance.cpp` - Item cloning/modification
- `quests/global/global_npc.lua` - Drop logic
- `lua_modules/commands/upgrade.lua` - Upgrade command
- `lua_modules/commands/fuse.lua` - Fusion command

### Database Schema
```sql
CREATE TABLE dynamic_items (
  item_id INT PRIMARY KEY,
  base_item_id INT,
  level INT,
  random_stats JSON,  -- Store random stat rolls
  created_at TIMESTAMP,
  INDEX idx_base_level (base_item_id, level)
);
```

## Future Enhancements
- Reforge system (reroll random stats)
- Item quality tiers (normal, magic, rare, epic)
- Set bonuses (wearing multiple items from same base)
- Corruption system (risk/reward high-power upgrades)
- Prestige levels (reset item to gain permanent bonuses)

## Design Goals Checklist
- ✅ Infinite progression (no level cap)
- ✅ Always something to chase (next level, better base, fusion)
- ✅ Strategic choices (which items to invest in)
- ✅ RNG excitement (random stat rolls)
- ✅ Solo-friendly (no trading required)
- ✅ Standard client (no mods needed)
- ✅ Scalable (formulas, not static data)

## See Also
- [SCALING_FORMULAS.md](SCALING_FORMULAS.md) - Detailed stat scaling math
- [FUSION_SYSTEM.md](FUSION_SYSTEM.md) - Item fusion mechanics
- [COMMANDS.md](COMMANDS.md) - Player commands reference
- [IMPLEMENTATION.md](IMPLEMENTATION.md) - Technical implementation guide
