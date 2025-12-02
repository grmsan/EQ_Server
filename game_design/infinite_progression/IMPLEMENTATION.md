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
1LLLIIIIII
│ │  └────── Base Item ID (last 6 digits)
│ └──────── Level with offset of 100 (101-250 = levels 1-150)
└────────── 1 billion prefix (marks dynamic items)

Examples:
  1,101,009,998 = Short Sword +1    (1B + 101 + 009998)
  1,200,009,998 = Short Sword +100  (1B + 200 + 009998)
  1,250,009,998 = Short Sword +150  (1B + 250 + 009998 - max level)

Max Level: 150 (offset 250 in ID)
Fits in signed int32: 1,250,999,999 < 2,147,483,647
```

### Encoding/Decoding
```cpp
uint32 DynamicItemManager::GenerateDynamicID(uint32 base_id, int level) {
  if (level == 0) return base_id;  // Base item, no dynamic ID

  // Format: 1LLLIIIIII (1 billion + level with offset + base ID)
  uint32 level_with_offset = 100 + level;  // 101-250 for levels 1-150
  uint32 encoded_base = base_id % 1000000;  // Last 6 digits

  return 1000000000 + (level_with_offset * 1000000) + encoded_base;
}

int DynamicItemManager::GetItemLevel(uint32 item_id) {
  if (item_id < 1000000000) return 0;  // Not a dynamic item

  uint32 level_with_offset = (item_id / 1000000) % 1000;
  return level_with_offset - 100;  // Remove offset
}

uint32 DynamicItemManager::GetBaseItemID(uint32 item_id) {
  if (item_id < 1000000000) return item_id;  // Already base

  return item_id % 1000000;  // Last 6 digits
}
```

## Stat Scaling Implementation

### Core Scaling Function
```cpp
void DynamicItemManager::ApplyLevelScaling(EQ::ItemData* item, int level) {
  if (!item || level <= 0) return;

  // ✅ IMPLEMENTED: Primary stats - scale with tiered formulas
  item->AC = CalculateTieredStat(item->AC, level,
                                  m_config.ac_base_increment,
                                  m_config.ac_tier_bonus);
  item->HP = CalculateTieredStat(item->HP, level,
                                  m_config.hp_base_increment,
                                  m_config.hp_tier_bonus);
  item->Mana = CalculateTieredStat(item->Mana, level,
                                    m_config.mana_base_increment,
                                    m_config.mana_tier_bonus);

  // ✅ IMPLEMENTED: Universal stat scaling - ALL items gain ALL stats
  // NOTE: Removed "if (item->AStr > 0)" checks - items with 0 base STR now gain STR
  int raw_str = CalculateTieredStat(item->AStr, level,
                                     m_config.stat_base_increment,
                                     m_config.stat_tier_bonus);
  ApplyStatCap(item->AStr, item->HeroicStr, raw_str);

  // Same for all other stats (STA, AGI, DEX, WIS, INT, CHA)
  // Each can start from 0 and still scale up

  // ✅ IMPLEMENTED: Weapon damage scaling (added damage_base_increment config)
  if (item->Damage > 0) {
    item->Damage = CalculateTieredStat(item->Damage, level,
                                        m_config.damage_base_increment,  // +4
                                        m_config.damage_tier_bonus);      // +4
    item->Attack = CalculateTieredStat(item->Attack, level,
                                        m_config.attack_base_increment,
                                        m_config.attack_tier_bonus);
  }

  // ✅ IMPLEMENTED: Milestone bonuses
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

## Two-Tier Cache Architecture

### ⚠️ Critical: Why We Need Two Tiers

**Problem:** Dynamic items (ID >= 1 billion) cannot go in shared memory
- Shared memory uses `FixedMemoryHashSet` with pre-allocated offset array
- Would need `max_item_id + 1` entries in offset array
- With max ID 1,250,999,999 → would need **4.8GB+ for offset array alone**
- Result: `ACCESS_VIOLATION` crash on `shared_memory` startup

**Solution:** Split caching strategy

### Tier 1: Shared Memory (Base Items Only)
**File:** `common/shareddb.cpp` lines 948-964

```cpp
bool SharedDatabase::GetItemsCount(int32& item_count, uint32& max_id) {
  // CRITICAL: Filter out dynamic items (ID >= 1 billion)
  std::string query = "SELECT MAX(id), COUNT(*) FROM items WHERE id < 1000000000";

  auto results = QueryDatabase(query);
  if (results.Success() && results.RowCount() == 1) {
    auto row = results.begin();
    max_id = static_cast<uint32>(atoul(row[0]));  // ~200,000 for base items
    item_count = atoi(row[1]);
    return true;
  }
  return false;
}
```

**What Goes Here:**
- All base items (ID < 1 billion)
- Loaded at `shared_memory` startup
- Available to all processes via OS shared memory
- Fast lookup: `FixedMemoryHashSet` with hash table

### Tier 2: Per-Process Cache (Dynamic Items)
**File:** `common/shareddb.h` lines 200-214

```cpp
class SharedDatabase : public Database {
public:
  // Two-tier cache
  const EQ::ItemData* GetItem(uint32 item_id);  // Checks both tiers

  // Tier 2: Dynamic items (ID >= 1 billion)
  void LoadDynamicItemsCache();              // Load all at startup (optional)
  void LoadDynamicItemToCache(uint32 id);    // On-demand loading
  void AddDynamicItemToCache(EQ::ItemData*); // Direct insertion
  void ClearDynamicItemsCache();             // Cleanup

private:
  // Per-process storage
  std::unordered_map<uint32, EQ::ItemData> dynamic_items_cache_;
  std::mutex dynamic_items_mutex_;  // Thread-safe access
};
```

**What Goes Here:**
- Dynamic items only (ID >= 1 billion)
- Per-process memory (not shared)
- On-demand loading from database
- Thread-safe with mutex

### On-Demand Loading Flow

**Scenario:** Player zones with Short Sword +100 (ID: 1,200,009,998)

1. **ItemInstance Constructor** (`common/item_instance.cpp` lines 91-98):
   ```cpp
   m_item = db->GetItem(item_id);
   if (!m_item && item_id >= 1000000000U) {
       db->LoadDynamicItemToCache(item_id);  // Load from database
       m_item = db->GetItem(item_id);         // Retry
   }
   ```

2. **LoadDynamicItemToCache** (`common/shareddb.cpp` lines 2248-2547):
   ```cpp
   void SharedDatabase::LoadDynamicItemToCache(uint32 item_id) {
     // Check if already cached (early return)
     {
       std::lock_guard<std::mutex> lock(dynamic_items_mutex_);
       if (dynamic_items_cache_.find(item_id) != dynamic_items_cache_.end()) {
         return;  // Already loaded
       }
     }

     // Query database for this specific item
     auto item = ItemsRepository::FindOne(*this, item_id);
     if (item.id == 0) {
       Log(Logs::General, Logs::Error, "Dynamic item [%u] not found in database", item_id);
       return;
     }

     // Convert all 243 fields from repository to ItemData
     EQ::ItemData item_data;
     item_data.ID = item.id;
     item_data.Name = strcpy(new char[64], item.name.c_str());
     item_data.AC = item.ac;
     item_data.HP = item.hp;
     // ... (241 more fields)

     // Add to cache (thread-safe)
     {
       std::lock_guard<std::mutex> lock(dynamic_items_mutex_);
       dynamic_items_cache_[item_id] = item_data;
     }

     Log(Logs::General, Logs::Status,
         "Loaded dynamic item [%u] (%s) from database into cache",
         item_id, item_data.Name);
   }
   ```

3. **GetItem Checks Both Tiers** (`common/shareddb.cpp` lines 977-990):
   ```cpp
   const EQ::ItemData* SharedDatabase::GetItem(uint32 item_id) {
     // Tier 1: Check shared memory first (base items)
     if (item_id < 1000000000) {
       return items_hash->Get(item_id);  // Fast hash lookup
     }

     // Tier 2: Check per-process cache (dynamic items)
     std::lock_guard<std::mutex> lock(dynamic_items_mutex_);
     auto it = dynamic_items_cache_.find(item_id);
     if (it != dynamic_items_cache_.end()) {
       return &it->second;
     }

     return nullptr;  // Not found in either tier
   }
   ```

### Database Persistence

**Critical:** After creating/updating dynamic items, must reload from database:

```cpp
// zone/dynamic_item_manager.cpp lines 481-632
void DynamicItemManager::InsertItemIntoDatabase(uint32 item_id, EQ::ItemData* item) {
  // 1. INSERT base row
  database.Query("INSERT INTO items (id, name, ...) VALUES (%u, '%s', ...)",
                 item_id, item->Name, ...);

  // 2. UPDATE scaled stats (8 separate queries to avoid query length limits)
  database.Query("UPDATE items SET ac=%d, hp=%d, ... WHERE id=%u", ...);
  // ... 7 more UPDATE queries for different stat groups ...

  // 3. ⚠️ CRITICAL: Reload from database to sync cache
  database.LoadDynamicItemToCache(item_id);
  //         ^^^ Without this, cache has INSERT values (base stats)
  //             but database has UPDATE values (scaled stats)
}
```

## Database Integration

### Schema - Uses Existing `items` Table
**No separate dynamic_items table!**

```sql
-- Dynamic items stored in standard items table
-- Identified by ID >= 1,000,000,000

SELECT id, name, ac, hp, damage FROM items WHERE id >= 1000000000 LIMIT 5;

-- Results:
-- 1,200,009,998 | Short Sword +100    | 550  | 400  | 404
-- 1,150,001,001 | Cloth Cap +50       | 153  | 200  | 0
-- 1,227,008,403 | Dragon Helm +127    | 1502 | 508  | 0
```

**Why No Separate Table:**
- Simpler schema (no foreign keys, no joins)
- Existing item code works unchanged
- Client can reference items normally
- Inventory system transparent

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

### Shared Memory Process
```cpp
// shared_memory/main.cpp
void LoadItems() {
  // ✅ IMPLEMENTED: Only load base items (ID < 1 billion)
  int32 item_count = 0;
  uint32 max_id = 0;

  if (!database.GetItemsCount(item_count, max_id)) {
    LogError("Failed to get item count");
    return;
  }

  LogInfo("Loading [{}] base items (max ID: {})", item_count, max_id);
  //       ^^^ Should be ~200,000 items with max_id ~200,000
  //           NOT 1,250,999,999 which would crash!

  // Allocate hash table sized for base items only
  items_hash = new FixedMemoryHashSet<EQ::ItemData>(max_id);

  // Load all base items into shared memory
  database.LoadItems(items_hash);
}
```

### Zone Server Initialization
```cpp
// zone/main.cpp
void ZoneInit() {
  // Attach to shared memory (base items already loaded)
  if (!database.AttachToSharedMemory()) {
    LogError("Failed to attach to shared memory");
    return;
  }

  // ⏳ OPTIONAL: Pre-load dynamic items
  // database.LoadDynamicItemsCache();  // Load all dynamic items at startup
  //          ^^^ Usually not needed - on-demand loading is sufficient

  LogInfo("Zone initialized - using on-demand dynamic item loading");
}
```

### On-Demand Loading Strategy
Dynamic items are **NOT** loaded at startup. Instead:
1. Player zones in with equipped items
2. ItemInstance constructor checks cache
3. If not found, `LoadDynamicItemToCache(item_id)` queries database
4. Item added to per-process cache
5. Subsequent lookups are instant (cache hit)

**Advantages:**
- Fast startup (no need to load millions of dynamic items)
- Low memory usage (only cache items actually in use)
- Scales naturally (only active items consume memory)

## Performance Considerations

### Caching Strategy
```cpp
// ✅ IMPLEMENTED: Two-tier caching, no eviction needed

// Tier 1: Shared memory (FixedMemoryHashSet)
// - All base items (ID < 1B)
// - Never evicted (static data)
// - Shared across all zone processes

// Tier 2: Per-process unordered_map
// - Dynamic items only (ID >= 1B)
// - Grows as items are accessed
// - Thread-safe with mutex
// - Could add LRU eviction if memory becomes concern:

const int MAX_CACHE_SIZE = 10000;  // Optional limit

void CheckCacheSize() {
  if (dynamic_items_cache_.size() > MAX_CACHE_SIZE) {
    // Clear oldest entries (would need access tracking)
    // For now: unlimited cache size (typically < 1000 items per zone)
  }
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
