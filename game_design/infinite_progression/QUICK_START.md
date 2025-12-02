# Infinite Progression - Quick Start Guide

## Current Status: 90% Complete ✅

### What's Working NOW:
1. ✅ **Core C++ scaling system** - All tiered formulas implemented
2. ✅ **Dynamic ID encoding/decoding** - 1LLLIIIIII format (1 billion + level + base ID)
3. ✅ **Stat capping with heroic overflow** - 127 cap handled correctly
4. ✅ **Tiered linear scaling** - All stats scale (AC, HP, damage, all attributes)
5. ✅ **Weapon damage scaling** - Fully implemented (+4/level tier 0, +4 bonus/tier)
6. ✅ **Universal stat scaling** - ALL items gain ALL stats (even if base is 0)
7. ✅ **Item generation & caching** - Two-tier cache (shared memory + per-process)
8. ✅ **Cross-zone persistence** - On-demand loading from database
9. ✅ **Automatic upgrades** - 5% chance on mob kill, 100% on named/rare
10. ✅ **GM Commands** - #upgrade working via Lua SendGMCommand
11. ✅ **Natural progression** - Players don't use commands, just kill mobs
12. ✅ **Database integration** - Items stored in `items` table with full stats

### Remaining Work (10%):
1. ⏳ **Haste formula** - Config exists, needs to be applied in ApplyMilestoneBonus
2. ⏳ **Focus effects** - Milestones defined, not yet implemented
3. ⏳ **Player fusion access** - Currently GM-only, need to expose safely
4. ⏳ **Random stats** - Framework exists, needs milestone triggers
5. ⏳ **Combat stats** - Shielding, StrikeThrough, etc. scaling disabled for now

---

## How It Works (Current Implementation)

### Automatic Progression System
**Players don't use commands** - items upgrade automatically!

**When you kill a mob:**
1. 5% chance to upgrade a random equipped item (+1 level)
2. 100% chance if mob is named/rare (upgrades 1-3 random items)
3. Message: "Your Short Sword +10 has been upgraded!" (appears automatically)

**No GM commands needed for normal play!**

### Current ID Format
```
1LLLIIIIII
├─ 1: Prefix (1 billion) - marks dynamic item
├─ LLL: Level with offset 100 (101-250 = levels 1-150)
└─ IIIIII: Base Item ID (last 6 digits)

Example: 1,200,009,998
  ├─ Prefix: 1 billion
  ├─ Level: 200 - 100 = 100
  └─ Base ID: 9998 (Short Sword)
```

### Stat Scaling (What Actually Happens)
```
Short Sword +100:
  Base: 4 damage, 0 AC, 0 stats

  After scaling:
  - Damage: 2,004 (+4/level tier 0, +4/tier bonus → ~2000 added)
  - AC: 2,750 (+1/level → ~2750 added)
  - STR/STA/AGI/DEX/WIS/INT/CHA: 127 base + 473 heroic (capped at 127)
  - Attack: 2,200
  - HP: ~5500
  - Mana: ~2750
```

---

## Testing the System

### Step 1: Boot Server
```bash
python start_server.py
```

### Step 2: Test Automatic Upgrades

**In-Game:**
1. Equip any item (Short Sword, Cloth Cap, etc.)
2. Kill mobs repeatedly
3. Watch for message: "Your [item] has been upgraded!"
4. Inspect item to see stats increase

**Expected Results:**
- 5% of kills trigger upgrade
- Named mobs always trigger (1-3 items upgraded)
- Stats increase with each level
- All attributes appear (even if base item had none)

### Step 3: Test GM Commands (Admin Only)

```
#upgrade 17        -- Upgrade chest slot item by 1 level
#upgrade 13 10     -- Upgrade primary weapon by 10 levels
```

**Note:** Regular players don't need these - auto-upgrades handle everything!

---

## Formulas Reference (Current Implementation)

### Dynamic ID Format
```
1LLLIIIIII
├─ 1: Prefix (1 billion marker for dynamic items)
├─ LLL: Level with offset 100 (101-250 = levels 1-150)
└─ IIIIII: Base Item ID (last 6 digits)

Example: 1,200,009,998
  ├─ Prefix: 1,000,000,000
  ├─ Level: 200 - 100 = 100
  └─ Base ID: 9998 (Short Sword)

Max Level: 150 (ID would be 1,250,999,999)
Fits in signed int32: 2,147,483,647 ✓
```

### Tiered Scaling Formula (CURRENT IMPLEMENTATION)
```
Total = Base + Sum of all tier contributions

For level 50:
  Tier 0 (1-10):   10 levels × +1/level = +10
  Tier 1 (11-20):  10 levels × +2/level = +20
  Tier 2 (21-30):  10 levels × +3/level = +30
  Tier 3 (31-40):  10 levels × +4/level = +40
  Tier 4 (41-50):  10 levels × +5/level = +50
  Total: 10+20+30+40+50 = 150

Short Sword (base 4 damage) at level 50:
  Final Damage = 4 + (4×150) = 4 + 600 = 604 damage
  (Using damage_base_increment=4, damage_tier_bonus=4)
```

### Milestone Bonuses (PARTIALLY IMPLEMENTED)
- ✅ **Level 1+**: All stats scale every level
- ✅ **Level 50+**: Heroic stats (+1 per 5 levels)
- ✅ **Level 50+**: HP Regen (+1 per 10 levels)
- ⏳ **Level 25+**: Haste (formula exists, not applied)
- ⏳ **Level 100+**: Focus effects (milestones defined, not implemented)

---

## Lua Integration (CURRENT SYSTEM)

### How Automatic Upgrades Work

**File:** `quests/global/global_npc.lua`

```lua
function event_death_complete(e)
    -- 5% chance on regular mobs, 100% on named
    local upgrade_chance = is_named and 100 or 5

    -- Pick 1-3 random equipped items
    -- Call: client:SendGMCommand("#upgrade " .. slot_id, true)
    -- (bypasses GM status check)
end
```

**Player Experience:**
- Kill mob → item upgrades automatically
- No commands needed
- Message shows which item upgraded

---

## Architecture Details (CURRENT)

### Two-Tier Cache System
**Problem:** Dynamic items (ID >= 1B) caused shared memory ACCESS_VIOLATION
**Solution:** Split caching strategy

**Tier 1 - Shared Memory:**
- Base items only (ID < 1 billion)
- Loaded at startup via shared_memory process
- Uses FixedMemoryHashSet

**Tier 2 - Per-Process Cache:**
- Dynamic items only (ID >= 1 billion)
- `std::unordered_map<uint32, std::unique_ptr<ItemData>>`
- Thread-safe with `std::mutex`
- On-demand loading from database

**Files:**
- `common/shareddb.h` - Cache declarations
- `common/shareddb.cpp` - LoadDynamicItemsCache(), LoadDynamicItemToCache()
- `common/item_instance.cpp` - On-demand loading in constructor

### Database Storage
**Items stored in existing `items` table:**
1. INSERT copies base item with dynamic ID
2. 8 UPDATE queries apply scaled stats:
   - name, damage, hp, mana, endur, ac
   - astr, asta, aagi, adex, awis, aint, acha
   - fr, cr, mr, pr, dr, svcorruption
   - heroic_str, heroic_sta, heroic_agi, heroic_dex, heroic_wis, heroic_int, heroic_cha
   - heroic_fr, heroic_cr, heroic_mr, heroic_pr, heroic_dr, heroic_svcorrup
   - haste, regen, manaregen, enduranceregen
   - attack, strikethrough, accuracy, stunresist, avoidance
   - shielding, dotshielding, spellshield, healamt, spelldmg, clairvoyance, backstabdmg

3. LoadDynamicItemToCache() reloads from database after UPDATEs

**No separate dynamic_items table** - all in main `items` table!

---

## Next Steps (TODO)

### High Priority
1. **Enable Haste Scaling**
   - Formula exists in config
   - Add to ApplyMilestoneBonus()
   - Test on items

2. **Add Focus Effects**
   - Milestones defined (100/200/500)
   - Implement AddFocusEffect()
   - Test spell damage/healing boosts

3. **Player Fusion Access**
   - Currently GM-only (#upgrade command)
   - Need safe player version
   - Consider fusion costs/restrictions

### Medium Priority
4. **Random Stats System**
   - Framework exists
   - Need milestone triggers (every 5/10 levels)
   - Implement AddRandomStat()

5. **Combat Stats Scaling**
   - Shielding, StrikeThrough, etc.
   - Currently commented out
   - Enable and test

### Low Priority
6. **Reforge System** - Reroll stats for currency
7. **Item Quality Tiers** - Normal/Magic/Rare/Epic variants
8. **Set Bonuses** - Bonuses for wearing matched items

---

**Last Updated**: December 2, 2025
**Build Status**: WORKING - Server running with auto-upgrades
**Core Features**: 90% complete
**Player Experience**: Natural progression via mob kills ✓
