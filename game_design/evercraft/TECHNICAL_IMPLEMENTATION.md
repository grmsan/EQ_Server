# EveCraft: Technical Implementation Guide

## Executive Summary

EveCraft integrates into the existing EQEmu tradeskill system by:

1. **Intercepting combine attempts** in `zone/tradeskills.cpp::HandleCombine()`
2. **Checking for EveCraft eligibility** (enabled flag, valid items)
3. **Computing deterministic result** using item hashing + skill lookup
4. **Caching combinations** in data buckets for determinism
5. **Generating or fetching dynamic items** with skill-scaled stats
6. **Executing the combine** using existing inventory, skill, and item systems

**No new database tables required.** Everything uses existing infrastructure.

---

## Architecture Overview

```
User initiates combine
    ↓
HandleCombine() [zone/tradeskills.cpp]
    ↓
Check: EveCraft enabled AND valid items?
    ├─ NO → Use existing recipe system (unchanged)
    └─ YES → EveCraft::ProcessCombine()
         ↓
    EveCraft::DetermineCombination()
         ├─ Hash(item1_id, item2_id, skill)
         ├─ Check cache (data_buckets)
         ├─ Lookup existing item DB match
         └─ Generate dynamic item if needed
         ↓
    EveCraft::ValidateCombination()
         ├─ Check class/race restrictions
         ├─ Check lore conflicts
         ├─ Check no-drop/attuned violations
         └─ Check currency/quest flags
         ↓
    EveCraft::ExecuteCombine()
         ├─ Compute success chance (skill-based)
         ├─ Roll for success/failure
         ├─ Handle salvage on failure
         ├─ Create result item instance
         └─ Update inventory, skill, log
```

---

## Data Storage Architecture

### 1. **Combination Cache (data_buckets table)**

**Key structure:**
```
evercraft_combo_{item1_id}_{item2_id}
```

**Value (JSON):**
```json
{
  "result_item_id": 12345,
  "result_name": "Masterwork Wolf-Skin Blade",
  "result_stats": {
    "attack": 45,
    "ac": -12,
    "hp": 50,
    "resist_cold": 5
  },
  "variant_id": 1,
  "min_skill": 50,
  "created_at": 1706880000,
  "created_by": "player_name"
}
```

**Why data_buckets?**
- Supports character-scoped and zone-scoped storage
- Built-in expiration (cache invalidation)
- JSON support via nested keys
- Persistent across zone changes

**Scope:**
- `character_id=0, zone_id=0` → Global (shared by all players)
- Or `character_id=X` → Per-character (optional, for tracking discovery)

### 2. **Dynamic Items**

Dynamic items are **created in the items table** the first time a new combination is discovered.

**Generation algorithm:**

```cpp
struct DynamicItemDef {
    uint32_t base_item1_id;       // For tracking origin
    uint32_t base_item2_id;       // For tracking origin
    uint32_t skill_tier;          // 0=trash, 1=standard, 2=enhanced, 3=mastercrafted
    std::string generated_name;   // Contextually generated
    EQ::ItemData stats;           // Generated stats
    uint16_t craft_version;       // Invalidation marker (bump when formula changes)
};
```

**Naming convention for dynamic items:**
```
{adjective}_{itemtype}_{material_descriptor}

Examples:
- "Reinforced_Helm_Dragon"
- "Masterwork_Blade_Moonlit"
- "Enhanced_Gloves_Dragonscale"
- "Crude_Dagger_Leather"
```

**Storage:** Items are stored in the standard `items` table with:
- `id`: New auto-generated ID (next available)
- `name`: Generated per formula
- `itemtype`: Inherited from dominant input
- `slots`: Inherited from dominant input
- `classes`: Intersection of both inputs' class restrictions
- Stats: Computed per skill tier

### 3. **Character Crafting History (optional, via data_buckets)**

**Key:** `evercraft_history_{character_id}`

**Value:**
```json
{
  "total_combines": 342,
  "successes": 310,
  "failures": 32,
  "discovered_recipes": [12345, 67890, ...],
  "favorite_combos": [
    { "item1": 123, "item2": 456, "times": 45 },
    { "item1": 789, "item2": 234, "times": 32 }
  ]
}
```

**Scope:** `character_id=X` (per-character tracking)

---

## Code Integration Points

### 1. **Entry Point: zone/tradeskills.cpp**

**File:** `zone/tradeskills.cpp`

**Function:** `Object::HandleCombine()`

**Current flow:**
```cpp
void Object::HandleCombine(Client* user, const NewCombine_Struct* in_combine, Object *worldo)
{
    // ... existing validation ...

    // Get recipe
    DBTradeskillRecipe_Struct spec;
    if (!content_db.GetTradeRecipe(container, c_type, some_id, user, &spec, &is_augmented)) {
        // Recipe not found → failure
        return;
    }

    // ... existing skill check, class check, etc ...

    // Execute combine
    bool success = user->TradeskillExecute(&spec);

    // ... handle results ...
}
```

**New integration:**
```cpp
void Object::HandleCombine(Client* user, const NewCombine_Struct* in_combine, Object *worldo)
{
    // ... existing validation up to GetTradeRecipe() ...

    // NEW: Check if EveCraft enabled
    if (RuleB(EveCraft, Enabled) && container && !is_augmented) {

        // Try EveCraft combine
        EQ::ItemInstance* inst1 = /* extract first item from container */;
        EQ::ItemInstance* inst2 = /* extract second item from container */;

        if (inst1 && inst2 && EveCraft::IsValidCombination(inst1, inst2)) {

            if (EveCraft::ProcessCombine(user, container, inst1, inst2)) {
                // EveCraft handled it—early return
                return;
            }
        }
    }

    // Fall through to existing recipe system
    if (!content_db.GetTradeRecipe(container, c_type, some_id, user, &spec, &is_augmented)) {
        // ... existing failure handling ...
    }

    // ... rest of existing code ...
}
```

### 2. **EveCraft Namespace: zone/evercraft.h & zone/evercraft.cpp**

**File:** `zone/evercraft.h`

```cpp
#pragma once

#include "../common/item_instance.h"
#include "../common/item_data.h"

namespace EveCraft {

    // Configuration
    struct Config {
        bool enabled;
        uint16_t skill_min;      // Minimum skill to attempt
        uint16_t skill_max;      // Maximum skill cap
        float success_curve;     // Difficulty scaling
        bool allow_no_drop;      // Allow combining no-drop items
        bool cache_results;      // Cache to data buckets
    };

    // Result structure
    struct CombineResult {
        uint32_t result_item_id;
        std::string result_name;
        bool success;
        uint8_t salvage_count;   // How many items salvaged on fail
        std::string failure_reason;
    };

    // Main entry point
    bool ProcessCombine(
        Client* user,
        EQ::ItemInstance* container,
        EQ::ItemInstance* item1,
        EQ::ItemInstance* item2
    );

    // Validation
    bool IsValidCombination(const EQ::ItemInstance* i1, const EQ::ItemInstance* i2);
    bool CheckLoreConflict(uint32_t item1_id, uint32_t item2_id);

    // Combination logic
    CombineResult DetermineCombination(uint32_t item1_id, uint32_t item2_id, uint16_t skill);
    uint32_t ComputeResultItem(uint32_t item1_id, uint32_t item2_id, uint16_t skill, uint8_t skill_tier);

    // Caching
    bool TryGetCachedResult(uint32_t item1_id, uint32_t item2_id, uint32_t& out_result_id);
    void CacheResult(uint32_t item1_id, uint32_t item2_id, uint32_t result_id, const std::string& json_data);

    // Skill utilities
    uint16_t GetSkillTier(uint16_t skill);  // 0=trash, 1=standard, 2=enhanced, 3=master
    float GetSuccessChance(uint16_t skill, uint16_t difficulty);
    bool CheckSkillIncrease(Client* user, uint16_t base_chance);

    // Item generation
    EQ::ItemData GenerateResultItem(
        const EQ::ItemData* base1,
        const EQ::ItemData* base2,
        uint8_t skill_tier,
        std::string& out_name
    );

    // Logging
    void LogCombineAttempt(
        uint32_t char_id,
        uint32_t item1_id,
        uint32_t item2_id,
        uint32_t result_id,
        bool success
    );
}
```

**File:** `zone/evercraft.cpp` (skeleton)

```cpp
#include "evercraft.h"
#include "client.h"
#include "zonedb.h"
#include "../common/data_bucket.h"
#include "../common/item_instance.h"

static const uint16_t EVERCRAFT_VERSION = 1;

namespace EveCraft {

bool ProcessCombine(
    Client* user,
    EQ::ItemInstance* container,
    EQ::ItemInstance* item1,
    EQ::ItemInstance* item2
)
{
    if (!user || !container || !item1 || !item2) {
        return false;
    }

    uint32_t item1_id = item1->GetItem()->ID;
    uint32_t item2_id = item2->GetItem()->ID;
    uint16_t skill = user->GetSkill(EQ::skills::SkillBlacksmithing); // or other tradeskill

    // Determine result
    CombineResult result = DetermineCombination(item1_id, item2_id, skill);

    if (result.failure_reason.length() > 0) {
        user->Message(Chat::Red, result.failure_reason.c_str());
        return false;
    }

    // Roll for success
    float success_chance = GetSuccessChance(skill, 100);
    bool success = zone->random.Real(0.0, 100.0) < success_chance;

    if (success) {
        // Create result item
        auto result_inst = database.CreateItem(result.result_item_id, 1);
        user->SummonItem(result.result_item_id, 1);

        // Skill increase check
        CheckSkillIncrease(user, skill * 2 / 300); // ~skill% chance

        user->Message(Chat::LightBlue, "You successfully crafted %s!", result.result_name.c_str());

        // Log
        LogCombineAttempt(user->CharacterID(), item1_id, item2_id, result.result_item_id, true);

    } else {
        // Salvage
        if (result.salvage_count > 0) {
            auto salvage_item = database.GetItem(item1_id);
            if (salvage_item) {
                user->SummonItem(item1_id, result.salvage_count);
                user->Message(Chat::Yellow, "Your combine failed. You salvaged %d of the components.", result.salvage_count);
            }
        } else {
            user->Message(Chat::Red, "Your combine failed completely.");
        }

        // Log
        LogCombineAttempt(user->CharacterID(), item1_id, item2_id, 0, false);
    }

    // Remove input items
    container->DeleteItem(item1);
    container->DeleteItem(item2);

    return true;
}

bool IsValidCombination(const EQ::ItemInstance* i1, const EQ::ItemInstance* i2)
{
    if (!i1 || !i2 || !i1->GetItem() || !i2->GetItem()) {
        return false;
    }

    auto item1 = i1->GetItem();
    auto item2 = i2->GetItem();

    // Check prohibited flags
    if (item1->QuestItemFlag || item2->QuestItemFlag) {
        return false;
    }

    // Check currency (assume slot-based or flag-based indicator)
    if (item1->ItemClass == 1 || item2->ItemClass == 1) { // Assuming 1 = container (non-combineable)
        return false;
    }

    return true;
}

CombineResult DetermineCombination(uint32_t item1_id, uint32_t item2_id, uint16_t skill)
{
    CombineResult result{};
    result.success = false;

    // Check cache first
    uint32_t cached_result = 0;
    if (TryGetCachedResult(item1_id, item2_id, cached_result)) {
        result.result_item_id = cached_result;
        result.success = true;
        return result;
    }

    // Compute result
    uint8_t skill_tier = GetSkillTier(skill);
    uint32_t computed_id = ComputeResultItem(item1_id, item2_id, skill, skill_tier);

    if (computed_id == 0) {
        result.failure_reason = "This combination is not possible.";
        return result;
    }

    // Cache result
    CacheResult(item1_id, item2_id, computed_id, "");

    result.result_item_id = computed_id;
    result.success = true;

    // Determine salvage count on failure
    result.salvage_count = skill < 100 ? 1 : (skill < 200 ? 0 : 0);

    return result;
}

uint32_t ComputeResultItem(uint32_t item1_id, uint32_t item2_id, uint16_t skill, uint8_t skill_tier)
{
    auto item1 = database.GetItem(item1_id);
    auto item2 = database.GetItem(item2_id);

    if (!item1 || !item2) {
        return 0;
    }

    // TODO: Lookup existing item in database
    // This would search for an item that matches the combination profile
    // For now, we'll generate a new one

    std::string generated_name;
    auto generated_stats = GenerateResultItem(item1, item2, skill_tier, generated_name);

    // TODO: Insert into items table (or use existing ID if found)
    // For now, return a placeholder
    return 0;
}

bool TryGetCachedResult(uint32_t item1_id, uint32_t item2_id, uint32_t& out_result_id)
{
    DataBucketKey k{};
    k.key = fmt::format("evercraft_combo_{}_{}",
        std::min(item1_id, item2_id),
        std::max(item1_id, item2_id));

    auto cached = DataBucket::GetData(k);

    if (cached.value.empty()) {
        return false;
    }

    // Parse JSON
    try {
        auto json = nlohmann::json::parse(cached.value);
        out_result_id = json["result_item_id"];
        return true;
    } catch (...) {
        return false;
    }
}

void CacheResult(uint32_t item1_id, uint32_t item2_id, uint32_t result_id, const std::string& json_data)
{
    DataBucketKey k{};
    k.key = fmt::format("evercraft_combo_{}_{}",
        std::min(item1_id, item2_id),
        std::max(item1_id, item2_id));
    k.value = json_data.empty() ?
        fmt::format(R"({{"result_item_id": {}}})", result_id) :
        json_data;

    DataBucket::SetData(k);
}

uint16_t GetSkillTier(uint16_t skill)
{
    if (skill < 50) return 0;   // Trash tier
    if (skill < 150) return 1;  // Standard tier
    if (skill < 250) return 2;  // Enhanced tier
    return 3;                    // Master tier
}

float GetSuccessChance(uint16_t skill, uint16_t difficulty)
{
    // Base formula: (skill / difficulty) capped at 99%
    float chance = (skill / (float)difficulty) * 100.0f;
    return std::min(99.0f, chance);
}

bool CheckSkillIncrease(Client* user, uint16_t base_chance)
{
    // TODO: Integrate with existing skill increase system
    // For now, simplified version
    return zone->random.Int(1, 100) <= base_chance;
}

EQ::ItemData GenerateResultItem(
    const EQ::ItemData* base1,
    const EQ::ItemData* base2,
    uint8_t skill_tier,
    std::string& out_name
)
{
    EQ::ItemData result{};

    // TODO: Implement contextual stat averaging and name generation

    return result;
}

void LogCombineAttempt(
    uint32_t char_id,
    uint32_t item1_id,
    uint32_t item2_id,
    uint32_t result_id,
    bool success
)
{
    // TODO: Log to player event log or tradeskill log
    LogTradeskills(
        "EveCraft combine | char_id={} | item1={} | item2={} | result={} | success={}",
        char_id, item1_id, item2_id, result_id, success
    );
}

} // namespace EveCraft
```

### 3. **Configuration: rules.cpp**

Add new rules:
```cpp
// In rules.cpp, Rules section for EveCraft:

{ "EveCraft:Enabled", false, "Enable EveCraft infinite crafting system" },
{ "EveCraft:SkillMin", 1, "Minimum skill to attempt EveCraft combine" },
{ "EveCraft:SkillMax", 300, "Maximum skill cap for EveCraft" },
{ "EveCraft:SuccessCurve", 2.0, "Difficulty multiplier for success chance" },
{ "EveCraft:AllowNoDrop", false, "Allow combining no-drop items" },
{ "EveCraft:AllowAttuned", false, "Allow combining attuned items" },
{ "EveCraft:CacheResults", true, "Cache combination results to data buckets" },
{ "EveCraft:Variant", 1, "EveCraft formula version (bump to invalidate cache)" },
```

### 4. **Item Lookup: Existing Database Queries**

**Existing query infrastructure:**
- `ItemRepository::FindWhere()` → Search items by name pattern
- `ItemRepository::All()` → Get all items (use with filter)

**New query for combination matching:**
```sql
SELECT i.id
FROM items i
WHERE
  i.name LIKE CONCAT('%', base1_name, '%', base2_name, '%')
  OR i.name LIKE CONCAT('%', base2_name, '%', base1_name, '%')
ORDER BY i.id DESC
LIMIT 1;
```

Alternative: Pre-define a set of "combination templates" in a simple Lua script:

```lua
-- quests/shared/evercraft_templates.lua

local function find_matching_result(item1_id, item2_id)
    local i1 = eq.get_item(item1_id)
    local i2 = eq.get_item(item2_id)

    if not i1 or not i2 then return nil end

    -- Template matching logic
    if i1:GetItemType() == ItemType.Weapon and i2:GetItemType() == ItemType.Armor then
        -- Search for existing item matching pattern
        return search_items_like("enhanced", i1:GetName(), i2:GetName())
    end

    return nil
end
```

### 5. **Inventory Management**

Uses existing inventory system:
```cpp
// In evercraft.cpp, result item creation:

EQ::ItemInstance* result_inst = new EQ::ItemInstance(result_item_def, 1);
user->PutItemInInventory(result_inst, true);
```

No changes needed to inventory code.

### 6. **Skill Increase Integration**

Piggyback on existing system:
```cpp
// In evercraft.cpp, after successful combine:

if (EveCraft::CheckSkillIncrease(user, 50 + skill)) { // 50 + current_skill% chance
    user->CheckIncreaseTradeskill(
        user->GetINT(),  // Or WIS, depending on tradeskill
        15,              // stat modifier
        2.0,             // skillup_modifier
        1,               // success_modifier
        tradeskill_id    // e.g., SkillBlacksmithing
    );
}
```

---

## Dynamic Item Generation Algorithm

### Naming

**Pattern:** `{quality}_{itemtype}_{material}`

```cpp
std::string GenerateItemName(const EQ::ItemData* base1, const EQ::ItemData* base2, uint8_t tier)
{
    static const std::vector<std::string> quality_prefixes{
        "Crude",
        "Standard",
        "Enhanced",
        "Masterwork",
        "Legendary"
    };

    // Dominant item determines type
    auto dominant = (base1->Attack > base2->Attack) ? base1 : base2;
    std::string type_name = "Item"; // Fallback

    // Material from secondary item
    std::string material = "Crafted";
    if (base2->Material > 0) {
        material = GetMaterialName(base2->Material);
    }

    return fmt::format("{}_{}_{}",
        quality_prefixes[std::min(tier, (uint8_t)4)],
        type_name,
        material
    );
}
```

### Stat Computation

```cpp
struct StatBlend {
    int32_t base_attack;
    int32_t base_ac;
    int32_t hp;
    int32_t mana;
    int8_t resistances[6];  // CR, DR, PR, MR, FR, SVCorrup
};

StatBlend ComputeStats(const EQ::ItemData* base1, const EQ::ItemData* base2, uint8_t tier)
{
    StatBlend result{};

    // Average attack/AC
    result.base_attack = (base1->Attack + base2->Attack) / 2;
    result.base_ac = (base1->AC + base2->AC) / 2;

    // Average HP/Mana
    result.hp = (base1->HP + base2->HP) / 2;
    result.mana = (base1->Mana + base2->Mana) / 2;

    // Average resistances
    for (int i = 0; i < 6; i++) {
        int8_t res = ((int32_t)base1->resistances[i] + base2->resistances[i]) / 2;
        result.resistances[i] = res;
    }

    // Apply tier multiplier
    float tier_multiplier = 1.0f + (tier * 0.25f);
    result.base_attack = (int32_t)(result.base_attack * tier_multiplier);
    result.hp = (int32_t)(result.hp * tier_multiplier);

    return result;
}
```

---

## Integration Checklist

- [ ] Add `zone/evercraft.h` and `zone/evercraft.cpp`
- [ ] Modify `zone/tradeskills.cpp::HandleCombine()` to check for EveCraft
- [ ] Add EveCraft rules to `rules.cpp`
- [ ] Update CMakeLists.txt to compile evercraft.cpp
- [ ] Implement item lookup/generation logic
- [ ] Implement stat computation and naming
- [ ] Test with GM commands: `#evercraft test item1_id item2_id skill_level`
- [ ] Add admin command to toggle EveCraft: `#evercraft enable|disable`
- [ ] Document in server_manager.py or web UI

---

## Logging and Debugging

### Tradeskill Log

All EveCraft combines logged to `logs/inf/tradeskill.log`:
```
[2025-02-06 12:34:56] EveCraft | CharID=123 | Item1=2345 | Item2=5678 | Result=99999 | Skill=150 | Success=1 | Tier=2
```

### Debug Output

Set `Logs::Detail` level to `DEBUG` for verbose output:
```
[DEBUG] EveCraft::DetermineCombination: Checking cache for (2345, 5678)...
[DEBUG] EveCraft::DetermineCombination: Cache miss, computing result...
[DEBUG] EveCraft::GenerateResultItem: Blending stats from base1 and base2...
[DEBUG] EveCraft::CacheResult: Cached result 99999 for combo (2345, 5678)
```

### GM Commands

```
#evercraft test <item1_id> <item2_id> <skill_level>
    Simulates a combine without consuming items

#evercraft enable
#evercraft disable
    Toggle EveCraft system

#evercraft cache clear
    Clear all cached combinations

#evercraft cache view <item1_id> <item2_id>
    View cached result for a specific combo

#evercraft history <char_id> [limit]
    View player's combine history
```

---

## Performance Considerations

### Caching Strategy

- **Data bucket cache** is memory-resident (read: ~1ms lookup)
- **No database hits** for cached combinations
- **Occasional DB writes** for new dynamic items (~50ms)
- **Expected throughput:** 100+ combines/second per zone

### Memory Impact

- ~10 KB per cached combination (JSON + metadata)
- 10,000 cached combos = ~100 MB per zone
- Acceptable for typical server loads

### Future Optimization

- LRU eviction policy if cache grows beyond threshold
- Separate cache table for frequently-used combos
- Compression for rarely-used result data

---

## Compatibility

### Existing Systems

- ✅ Works with existing recipe system (EveCraft is parallel)
- ✅ Existing quests unaffected
- ✅ Existing trades/sales unaffected
- ✅ Inventory system unchanged
- ✅ Skill system unchanged

### Breaking Changes

None. This is entirely opt-in via rule `EveCraft:Enabled`.

---

## Future Extensibility

### Lua Hooks (for customization)

```lua
-- In quest files, optional customization:

function evercraft_validate_combo(item1_id, item2_id, char_id)
    -- Return false to reject, true to allow
    return true
end

function evercraft_compute_name(item1_id, item2_id, tier)
    -- Return custom name, or nil for auto-generation
    return nil
end

function evercraft_compute_stats(item1_id, item2_id, tier)
    -- Return table with stat overrides, or nil for defaults
    return nil
end
```

### NPC-Driven Recipes

```lua
-- NPC teaching EveCraft recipes (future):
NPC:TeachEveCraftRecipe(item1_id, item2_id, result_name, result_item_id)
```

---

## Testing Strategy

### Unit Tests

```cpp
// tests/evercraft_test.h

class EveCraftTest : public Test::Suite {
    void TestValidCombination() { /* ... */ }
    void TestStatBlending() { /* ... */ }
    void TestNaming() { /* ... */ }
    void TestCaching() { /* ... */ }
    void TestDeterminism() { /* ... */ }
};
```

### Integration Tests

1. **Combine without recipe:** EveCraft handles it
2. **Combine with recipe:** Recipe system takes precedence
3. **Skill scaling:** Higher skill = better results
4. **Failure rates:** Low skill = failures, high skill = successes
5. **Caching:** Same inputs always produce same result
6. **Restrictions:** No-drop/quest items rejected

---

## Deployment Rollout

### Phase 1: Alpha (Internal Testing)
- Enable on test server only
- Log all combines heavily
- Monitor cache performance

### Phase 2: Beta (Controlled Rollout)
- Enable on one live zone
- Gather feedback
- Tune success rates and stat scaling

### Phase 3: Live
- Enable globally or per-zone
- Documentation for players
- Ongoing monitoring and balance adjustments
