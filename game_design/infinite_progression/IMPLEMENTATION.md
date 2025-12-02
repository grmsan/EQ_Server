# Technical Implementation Guide

## Architecture Overview

### Component Structure
```
zone/
  dynamic_item_manager.h/cpp   -- Core scaling logic
  commands/
    cmd_upgrade.cpp            -- #upgrade command
    cmd_fuse.cpp               -- #fuse command
    cmd_iteminfo.cpp           -- #iteminfo command

common/
  item_instance.cpp            -- Item cloning/modification
  item_data.h                  -- Item stat structures

database/
  schema/
    dynamic_items.sql          -- Persistent storage

lua_modules/
  commands/
    upgrade.lua                -- Lua wrapper for #upgrade
    fuse.lua                   -- Lua wrapper for #fuse
```

## Core Classes

### DynamicItemManager
**File:** `zone/dynamic_item_manager.h`

```cpp
class DynamicItemManager {
public:
  // Singleton access
  static DynamicItemManager& Get();

  // Item generation
  EQ::ItemData* GenerateScaledItem(uint32 base_item_id, int level);
  EQ::ItemInstance* CreateDynamicInstance(uint32 base_item_id, int level);

  // Level management
  int GetItemLevel(uint32 dynamic_item_id);
  uint32 GetBaseItemID(uint32 dynamic_item_id);
  uint32 GenerateDynamicID(uint32 base_item_id, int level);

  // Fusion
  EQ::ItemInstance* FuseItems(
    EQ::ItemInstance* donor,
    EQ::ItemInstance* receiver
  );

  // Stat scaling
  void ApplyLevelScaling(EQ::ItemData* item, int level);
  void ApplyMilestoneBonus(EQ::ItemData* item, int level);
  void AddRandomStat(EQ::ItemData* item, int level);

  // Caching
  void CacheItem(uint32 item_id, EQ::ItemData* item);
  EQ::ItemData* GetCachedItem(uint32 item_id);
  void ClearCache();

private:
  std::map<uint32, EQ::ItemData*> item_cache_;
  std::map<uint32, std::vector<RandomStat>> random_stats_;
};
```

## Dynamic Item ID Format

### Bit Layout
```
32-bit Item ID:
[P][LLLLL][IIIIII]
 │  │      └── Base Item ID (up to 999,999)
 │  └────────── Level (up to 99,999)
 └───────────── Prefix (5 = dynamic item)

Examples:
  500001001 = Cloth Cap +1      (prefix:5, level:1, base:1001)
  500127001 = Cloth Cap +127    (prefix:5, level:127, base:1001)
  509999001 = Cloth Cap +9999   (prefix:5, level:9999, base:1001)
```

### Encoding/Decoding
```cpp
uint32 DynamicItemManager::GenerateDynamicID(uint32 base_id, int level) {
  if (level == 0) return base_id;  // Base item, no dynamic ID

  uint32 prefix = 5;  // Dynamic item marker
  uint32 encoded_level = std::min(level, 99999);
  uint32 encoded_base = base_id % 1000000;  // Limit to 6 digits

  return (prefix * 100000000) + (encoded_level * 1000) + encoded_base;
}

int DynamicItemManager::GetItemLevel(uint32 item_id) {
  if (item_id < 500000000) return 0;  // Not a dynamic item

  return (item_id / 1000) % 100000;
}

uint32 DynamicItemManager::GetBaseItemID(uint32 item_id) {
  if (item_id < 500000000) return item_id;  // Already base

  return item_id % 1000;
}
```

## Stat Scaling Implementation

### Core Scaling Function
```cpp
void DynamicItemManager::ApplyLevelScaling(EQ::ItemData* item, int level) {
  if (!item || level <= 0) return;

  // Primary stats - scale existing
  item->AC += level * 1;
  item->HP += level * 2;
  item->Mana += level * 1;

  // Attribute stats - scale if exist
  if (item->AStr > 0) item->AStr += level;
  if (item->ASta > 0) item->ASta += level;
  if (item->AAgi > 0) item->AAgi += level;
  if (item->ADex > 0) item->ADex += level;
  if (item->AWis > 0) item->AWis += level;
  if (item->AInt > 0) item->AInt += level;
  if (item->ACha > 0) item->ACha += level;

  // Milestone bonuses
  ApplyMilestoneBonus(item, level);
}
```

### Milestone System
```cpp
void DynamicItemManager::ApplyMilestoneBonus(EQ::ItemData* item, int level) {
  // Every 5 levels: random stat
  if (level % 5 == 0) {
    if (rand() % 100 < 50) {  // 50% chance
      AddRandomStat(item, level);
    }
  }

  // Every 10 levels: guaranteed bonus
  if (level % 10 == 0) {
    AddRandomStat(item, level);  // Always add
  }

  // Level 25: Haste unlock
  if (level >= 25) {
    item->Haste = std::min((level - 25) / 10, 100);
  }

  // Level 50: Heroics unlock
  if (level >= 50) {
    item->HeroicStr = (level - 50) / 10;
    item->HeroicSta = (level - 50) / 10;
    item->HeroicAgi = (level - 50) / 10;
    // ... etc
  }

  // Level 100: Focus effect
  if (level >= 100 && level < 200) {
    AddFocusEffect(item, FOCUS_MINOR);
  } else if (level >= 200 && level < 500) {
    AddFocusEffect(item, FOCUS_MAJOR);
  } else if (level >= 500) {
    AddFocusEffect(item, FOCUS_EPIC);
  }
}
```

### Random Stat System
```cpp
struct RandomStat {
  std::string type;  // "STR", "WIS", "FR", etc.
  int value;
  int added_at_level;
};

void DynamicItemManager::AddRandomStat(EQ::ItemData* item, int level) {
  // Get stats this item doesn't have yet
  std::vector<std::string> available_stats;

  if (item->AStr == 0) available_stats.push_back("STR");
  if (item->ASta == 0) available_stats.push_back("STA");
  if (item->AWis == 0) available_stats.push_back("WIS");
  // ... etc

  if (available_stats.empty()) return;  // Item has all stats

  // Pick random stat
  std::string stat = available_stats[rand() % available_stats.size()];
  int value = level / 5;  // Scale to level

  // Apply stat
  if (stat == "STR") item->AStr = value;
  else if (stat == "STA") item->ASta = value;
  // ... etc

  // Track for persistence
  random_stats_[item->ID].push_back({stat, value, level});
}
```

## Database Integration

### Schema
```sql
CREATE TABLE IF NOT EXISTS dynamic_items (
  item_id INT UNSIGNED PRIMARY KEY,
  base_item_id INT UNSIGNED NOT NULL,
  level INT UNSIGNED NOT NULL,
  random_stats JSON DEFAULT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_base_level (base_item_id, level),
  INDEX idx_created (created_at)
);

-- Example row:
-- item_id: 500127001
-- base_item_id: 1001
-- level: 127
-- random_stats: {"STR":{"value":25,"added_at":5},"WIS":{"value":13,"added_at":15}}
```

### Persistence
```cpp
void DynamicItemManager::SaveItem(uint32 item_id) {
  int level = GetItemLevel(item_id);
  uint32 base_id = GetBaseItemID(item_id);

  // Serialize random stats to JSON
  std::string random_stats_json = SerializeRandomStats(item_id);

  database.Query(
    "INSERT INTO dynamic_items (item_id, base_item_id, level, random_stats) "
    "VALUES (%u, %u, %d, '%s') "
    "ON DUPLICATE KEY UPDATE level = %d, random_stats = '%s'",
    item_id, base_id, level, random_stats_json.c_str(),
    level, random_stats_json.c_str()
  );
}

void DynamicItemManager::LoadItem(uint32 item_id) {
  auto results = database.QueryDatabase(
    "SELECT base_item_id, level, random_stats FROM dynamic_items WHERE item_id = %u",
    item_id
  );

  if (results.Success() && results.RowCount() > 0) {
    auto row = results.begin();
    uint32 base_id = atoi(row[0]);
    int level = atoi(row[1]);
    std::string random_stats_json = row[2];

    // Generate item
    EQ::ItemData* item = GenerateScaledItem(base_id, level);

    // Apply random stats
    DeserializeRandomStats(item, random_stats_json);

    CacheItem(item_id, item);
  }
}
```

## Command Implementation

### Upgrade Command
```cpp
// zone/commands/cmd_upgrade.cpp
void command_upgrade(Client* c, const Seperator* sep) {
  EQ::ItemInstance* inst = c->GetInv().GetItem(EQ::invslot::slotCursor);
  if (!inst) {
    c->Message(Chat::Red, "No item on cursor.");
    return;
  }

  int levels_to_add = sep->IsNumber(1) ? atoi(sep->arg[1]) : 1;

  uint32 current_id = inst->GetID();
  int current_level = DynamicItemManager::Get().GetItemLevel(current_id);
  uint32 base_id = DynamicItemManager::Get().GetBaseItemID(current_id);

  int new_level = current_level + levels_to_add;

  // Create upgraded item
  EQ::ItemInstance* upgraded = DynamicItemManager::Get().CreateDynamicInstance(
    base_id, new_level
  );

  // Replace cursor item
  c->DeleteItemInInventory(EQ::invslot::slotCursor);
  c->PushItemOnCursor(*upgraded);

  c->Message(Chat::Yellow, "Upgraded %s to level %d (+%d)",
    inst->GetItem()->Name, new_level, levels_to_add);

  delete upgraded;
}
```

### Fuse Command
```cpp
// zone/commands/cmd_fuse.cpp
void command_fuse(Client* c, const Seperator* sep) {
  EQ::ItemInstance* donor = c->GetInv().GetItem(EQ::invslot::slotCursor);
  if (!donor) {
    c->Message(Chat::Red, "No donor item on cursor.");
    return;
  }

  // Get receiver (chest slot for example)
  int16 receiver_slot = EQ::invslot::slotChest;
  EQ::ItemInstance* receiver = c->GetInv().GetItem(receiver_slot);
  if (!receiver) {
    c->Message(Chat::Red, "No receiver item in chest slot.");
    return;
  }

  // Perform fusion
  EQ::ItemInstance* fused = DynamicItemManager::Get().FuseItems(donor, receiver);
  if (!fused) {
    c->Message(Chat::Red, "Fusion failed!");
    return;
  }

  // Replace items
  c->DeleteItemInInventory(EQ::invslot::slotCursor);
  c->DeleteItemInInventory(receiver_slot);
  c->PutItemInInventory(receiver_slot, *fused);

  c->Message(Chat::Yellow, "Fused into %s!", fused->GetItem()->Name);

  delete fused;
}
```

## Loading at Server Start

### World Server Initialization
```cpp
// zone/main.cpp
void ZoneInit() {
  // ... existing init ...

  // Load dynamic items from database
  DynamicItemManager::Get().LoadAllCachedItems();

  LogInfo("Loaded [{}] dynamic items from database",
    DynamicItemManager::Get().GetCacheSize());
}
```

### Shared Memory Integration
Dynamic items are NOT in shared memory (too many variations). They're:
1. Generated on-demand from formulas
2. Cached in zone memory for performance
3. Persisted to database for server restarts

## Performance Considerations

### Caching Strategy
```cpp
// Keep last 1000 accessed items in memory
const int MAX_CACHE_SIZE = 1000;

void DynamicItemManager::CacheItem(uint32 item_id, EQ::ItemData* item) {
  if (item_cache_.size() >= MAX_CACHE_SIZE) {
    // Evict oldest (simple LRU)
    item_cache_.erase(item_cache_.begin());
  }

  item_cache_[item_id] = item;
}
```

### Lazy Loading
Items are only generated when:
- Player equips/views the item
- Item is involved in a transaction
- Explicit command triggers it

## Testing

### Unit Tests
```cpp
// tests/dynamic_item_tests.cpp
TEST(DynamicItemManager, GenerateID) {
  auto& mgr = DynamicItemManager::Get();

  EXPECT_EQ(mgr.GenerateDynamicID(1001, 0), 1001);
  EXPECT_EQ(mgr.GenerateDynamicID(1001, 1), 500001001);
  EXPECT_EQ(mgr.GenerateDynamicID(1001, 127), 500127001);
  EXPECT_EQ(mgr.GetItemLevel(500127001), 127);
  EXPECT_EQ(mgr.GetBaseItemID(500127001), 1001);
}

TEST(DynamicItemManager, Scaling) {
  auto& mgr = DynamicItemManager::Get();
  auto* item = mgr.GenerateScaledItem(1001, 100);

  EXPECT_EQ(item->AC, 103);  // Base 3 + 100 levels
  EXPECT_EQ(item->HP, 200);  // 0 + (100 * 2)

  delete item;
}
```

## See Also
- [OVERVIEW.md](OVERVIEW.md) - System design overview
- [SCALING_FORMULAS.md](SCALING_FORMULAS.md) - Stat formulas
- [FUSION_SYSTEM.md](FUSION_SYSTEM.md) - Fusion mechanics
- [COMMANDS.md](COMMANDS.md) - Player command reference
