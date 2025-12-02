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
Items can level up infinitely. Each level increases stats using tiered linear scaling formulas.

**Item ID Format:**
```
Dynamic ID: 1LLLIIIIII
  1      = Prefix (indicates dynamic item, 1 billion+)
  LLL    = Level with offset 100 (101-250 = levels 1-150)
  IIIIII = Base Item ID (last 6 digits)

Example:
  Short Sword (ID 9998) at level 1
  = 1,101,009,998 (1 billion + 101*1M + 9998)

  Short Sword at level 100
  = 1,200,009,998 (1 billion + 200*1M + 9998)
```

### 2. Stat Scaling Rules (Tiered Linear System)
**Tiered Increments:**
- Every 10 levels = new tier, increment increases
- Tier 0 (1-10): base increment
- Tier 1 (11-20): base + tier_bonus
- Tier 2 (21-30): base + (tier_bonus × 2)
- etc.

**Current Scaling:**
- **ALL items gain ALL stats** when upgraded (even if base is 0)
- AC: +1/level tier 0, +1 bonus per tier
- HP: +4/level tier 0, +4 bonus per tier
- Mana: +1/level tier 0, +1 bonus per tier
- Stats (STR/STA/AGI/DEX/WIS/INT/CHA): +1/level tier 0, +1 bonus per tier
- Attack (weapons): +2/level tier 0, +2 bonus per tier
- **Weapon Damage**: +4/level tier 0, +4 bonus per tier

**Stat Capping:**
- Base stats cap at 127 (EQ client limit)
- Overflow automatically goes to Heroic stats
- Example: Formula gives +200 STR → Display as +127 STR, +73 H.STR

**Milestones:**
- **Level 1+**: All stats scale every level
- **Level 50+**: Heroic stats from milestones (+1 per 5 levels)
- **Level 50+**: HP Regen (+1 per 5 levels from level 50)
- **TODO**: Haste, Focus effects, Procs (not yet implemented)

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
Level 1: Kill mob, item upgrades
  → Short Sword +1 (damage: 8, AC: 1, all stats: +1)

Level 10: Continue progression
  → Short Sword +10 (damage: 44, AC: 55, all stats: +55, +1 heroic)

Level 50: Mid-game
  → Short Sword +50 (damage: 404, AC: 550, base stats: +127 (capped), heroics: +100+)

Level 100: Endgame
  → Short Sword +100 (damage: 2,004, AC: 2,750, base stats: +127, heroics: +500+)

**Fusion Example:**
Find Blade of Tactics (better base: 14 damage vs 4)
Fuse Short Sword +100 into Blade of Tactics
→ Result: Blade of Tactics +100 (14 base damage + 2,000 scaled = 2,014 damage!)
```

## Technical Implementation

### Architecture
- **C++ Server**: Dynamic item generation, stat scaling formulas
- **Lua Scripts**: Drop logic, upgrade triggers
- **Database**: Persistent storage for dynamic items
- **Client**: Standard EQ client (no mods needed)

### Key Files
- `zone/dynamic_item_manager.h/cpp` - Core scaling system, formulas, caching
- `zone/dynamic_item_integration.cpp` - GM command integration (#upgrade)
- `common/shareddb.h/cpp` - Two-tier cache (shared memory + per-process cache)
- `common/item_instance.cpp` - Dynamic item loading on-demand
- `quests/global/global_npc.lua` - Automatic upgrade on mob death (5% chance, 100% on named)

### Database Schema
**Dynamic items are stored directly in the `items` table:**
- Dynamic ID format: 1LLLIIIIII (1 billion + level offset + base ID)
- Full 243-column schema with all scaled stats
- INSERT copies base item, then 8 UPDATE queries apply scaled stats
- Two-tier cache prevents shared memory crashes:
  - Shared Memory: Base items only (ID < 1 billion)
  - Per-Process Cache: Dynamic items (ID >= 1 billion)
- On-demand loading: Items load from database when needed across zones

## Future Enhancements (TODO)
- ⏳ **Haste scaling** - Formula exists, needs to be applied
- ⏳ **Focus effects** - Milestones defined (100/200/500), not implemented
- ⏳ **Player fusion access** - Currently GM-only, expose to players
- ⏳ **Random stat system** - Framework exists, needs milestone triggers
- 📋 **Reforge system** - Reroll random stats for currency
- 📋 **Item quality tiers** - Normal, magic, rare, epic variants
- 📋 **Set bonuses** - Wearing multiple items from same base
- 📋 **Corruption system** - Risk/reward high-power upgrades
- 📋 **Prestige levels** - Reset item to gain permanent bonuses
- 📋 **Proc/Click effects** - Level 500+ content

## Design Goals Checklist
- ✅ Infinite progression (no level cap) - WORKING
- ✅ Automatic upgrades (5% on mob kill, 100% on named) - WORKING
- ✅ Natural gameplay feel (no GM commands needed) - WORKING
- ✅ All items gain all stats (universal scaling) - WORKING
- ✅ Weapon damage scaling - WORKING
- ✅ Stat capping with heroic overflow - WORKING
- ✅ Fusion system - GM command implemented
- ✅ Cross-zone persistence - WORKING (on-demand cache loading)
- ✅ Solo-friendly (no trading required) - WORKING
- ✅ Standard client (no mods needed) - WORKING
- ⏳ Strategic choices - TODO (fusion needs player access)
- ⏳ Haste/Focus effects - TODO (formulas exist, not applied yet)
- ⏳ Random stat rolls - TODO (future enhancement)

## See Also
- [SCALING_FORMULAS.md](SCALING_FORMULAS.md) - Detailed stat scaling math
- [FUSION_SYSTEM.md](FUSION_SYSTEM.md) - Item fusion mechanics
- [COMMANDS.md](COMMANDS.md) - Player commands reference
- [IMPLEMENTATION.md](IMPLEMENTATION.md) - Technical implementation guide
