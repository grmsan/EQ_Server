# Player Commands Reference

## Item Progression Commands

### #upgrade
Increase the level of the item on your cursor.

**Syntax:**
```
#upgrade [levels]
```

**Parameters:**
- `levels` (optional): Number of levels to add (default: 1)

**Requirements:**
- Item must be on cursor
- Item must be eligible for upgrade

**Examples:**
```
#upgrade          -- Add 1 level to cursor item
#upgrade 5        -- Add 5 levels to cursor item
#upgrade 100      -- Add 100 levels (if you have resources)
```

**Output:**
```
[Upgrade] Cloth Cap +10 upgraded to Cloth Cap +11
[Stats] +1 AC, +2 HP, +1 STR
```

---

### #fuse
Transfer levels from cursor item (donor) to a worn/inventory item (receiver).

**Syntax:**
```
#fuse [slot]
#fuse confirm
```

**Parameters:**
- `slot` (optional): Equipment slot number or name (e.g., "chest", "17")
- `confirm`: Confirms fusion after preview

**Requirements:**
- Donor item on cursor
- Receiver item worn or in inventory
- Items must be same equipment slot
- Donor level > 0

**Examples:**
```
#fuse             -- Preview fusion with equipped item
#fuse chest       -- Fuse into chest slot
#fuse 17          -- Fuse into slot 17 (chest)
#fuse confirm     -- Execute fusion after preview
```

**Output:**
```
=== FUSION PREVIEW ===
Donor: Cloth Cap +127 (will be destroyed)
Receiver: Dragon Helm +0
Result: Dragon Helm +127

Type #fuse confirm to proceed
```

---

### #iteminfo
Display detailed information about an item's level and stats.

**Syntax:**
```
#iteminfo [target]
```

**Parameters:**
- `target` (optional): "cursor", slot number, or nothing for worn items

**Examples:**
```
#iteminfo           -- Info about targeted item
#iteminfo cursor    -- Info about cursor item
#iteminfo chest     -- Info about chest slot item
```

**Output:**
```
=== ITEM INFO ===
Item: Dragon Helm +127
Base Item: Dragon Helm (ID: 8403)
Level: 127
Dynamic ID: 500127403

Base Stats:
  AC: 25 → 152 (+127)
  STR: 5 → 132 (+127)
  HP: 0 → 254 (+254)

Random Stats (acquired through leveling):
  WIS: +65 (added at level 15)
  STA: +45 (added at level 20)
  Fire Resist: +12 (added at level 30)

Milestone Bonuses:
  Haste: 12% (unlocked at level 25)
  Heroic STR: +7 (unlocked at level 50)
  Focus: Improved Damage III (unlocked at level 200)
```

---

### #reforge (Future)
Reroll random stats on an item.

**Syntax:**
```
#reforge [keep_count]
```

**Parameters:**
- `keep_count` (optional): Number of stats to keep (default: 0)

**Examples:**
```
#reforge          -- Reroll all random stats
#reforge 2        -- Keep 2 best stats, reroll the rest
```

---

### #extract (Future)
Extract levels from an item into a consumable essence.

**Syntax:**
```
#extract [levels]
```

**Parameters:**
- `levels`: Number of levels to extract

**Examples:**
```
#extract 10       -- Extract 10 levels into essence
```

**Result:**
```
Cloth Cap +127 → Cloth Cap +117
Created: Level Essence x10 (consumable)
```

---

## Information Commands

### #itemstats
Show current equipment stats summary.

**Syntax:**
```
#itemstats
```

**Output:**
```
=== EQUIPMENT STATS ===
Total Item Levels: 847
Average Item Level: 48.1

Slot Breakdown:
  Head:  Dragon Helm +127
  Chest: Plate Chestguard +98
  Arms:  Cloth Sleeves +45
  ...

Total Stats:
  AC: 1,247
  HP: 3,892
  Mana: 1,456
  STR: 342
  Haste: 35%
```

---

### #progression
Show character progression statistics.

**Syntax:**
```
#progression
```

**Output:**
```
=== PROGRESSION STATS ===
Total Upgrades: 1,247
Total Fusions: 23
Highest Item Level: 127 (Dragon Helm)
Total Boss Kills: 456

Recent Milestones:
  Level 100 item achieved: Dragon Helm
  Level 50 Heroic stats unlocked
  First fusion: Cloth Cap → Banded Helm
```

---

## Admin/Debug Commands

### #createscaled (GM)
Create a scaled item at specified level.

**Syntax:**
```
#createscaled <item_id> <level>
```

**Parameters:**
- `item_id`: Base item ID
- `level`: Item level to create

**Examples:**
```
#createscaled 1001 100    -- Create Cloth Cap +100
#createscaled 8403 500    -- Create Dragon Helm +500
```

---

### #itemdebug (GM)
Toggle debug output for item system.

**Syntax:**
```
#itemdebug [on|off]
```

**Output:**
```
[DEBUG] Item scaling enabled
[DEBUG] Cloth Cap +127:
  - Base AC: 3
  - Level bonus: +127
  - Random stats: 4 (WIS, STA, CHA, FR)
  - Milestones: Haste(25), Heroic(50), Focus(100)
```

---

### #setitemlevel (GM)
Force set an item's level.

**Syntax:**
```
#setitemlevel <level>
```

**Parameters:**
- `level`: New level for cursor item

**Examples:**
```
#setitemlevel 100     -- Set cursor item to level 100
#setitemlevel 0       -- Reset item to base level
```

---

## Quick Reference Table

| Command | Purpose | Syntax |
|---------|---------|--------|
| `#upgrade` | Level up item | `#upgrade [levels]` |
| `#fuse` | Fuse items | `#fuse [slot]` |
| `#iteminfo` | View item details | `#iteminfo [target]` |
| `#itemstats` | View equipment summary | `#itemstats` |
| `#progression` | View character progression | `#progression` |
| `#createscaled` (GM) | Create scaled item | `#createscaled <id> <lvl>` |

## See Also
- [OVERVIEW.md](OVERVIEW.md) - System overview
- [SCALING_FORMULAS.md](SCALING_FORMULAS.md) - How stats scale
- [FUSION_SYSTEM.md](FUSION_SYSTEM.md) - Fusion mechanics
- [IMPLEMENTATION.md](IMPLEMENTATION.md) - For developers
