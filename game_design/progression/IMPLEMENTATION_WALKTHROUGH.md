# Account Progression - Implementation Walkthrough

Step-by-step guide for implementing the account progression system.

## Prerequisites

Before starting, you should be familiar with:
- EQEmu server architecture (zone, world, common)
- Database repository pattern used in this codebase
- Lua quest scripting basics
- C++ basics (classes, templates, STL)

## Implementation Order

Follow this order to build incrementally testable features:

| Phase | Feature | Estimated Time | Dependencies |
|-------|---------|----------------|--------------|
| 1 | Database tables + repository | 4-6 hours | None |
| 2 | Core manager class | 4-6 hours | Phase 1 |
| 3 | Account unlocks | 2-4 hours | Phase 2 |
| 4 | Zone access checks | 2-4 hours | Phase 3 |
| 5 | Account perks (non-stackable) | 3-4 hours | Phase 2 |
| 6 | Stackable perks + diminishing | 4-6 hours | Phase 5 |
| 7 | Veteran bonuses | 4-6 hours | Phase 2 |
| 8 | Lua bindings | 4-6 hours | Phase 3-7 |
| 9 | Commands | 2-3 hours | Phase 8 |
| 10 | Testing + polish | 4-8 hours | All |

**Total estimate:** 35-55 hours for complete implementation

---

## Phase 1: Database Tables + Repository

### Step 1.1: Create Tables

Run the SQL from [DATABASE_SCHEMA.md](DATABASE_SCHEMA.md) to create:
- `account_unlocks`
- `account_perks`
- `account_veteran_cache`
- `account_unlock_definitions`
- `account_zone_locks`
- `account_level_caps`
- `account_perk_definitions`
- `account_veteran_rules`

### Step 1.2: Create Repository Classes

Create `common/repositories/account_unlocks_repository.h`:

```cpp
#ifndef EQEMU_ACCOUNT_UNLOCKS_REPOSITORY_H
#define EQEMU_ACCOUNT_UNLOCKS_REPOSITORY_H

#include "../database.h"
#include "../strings.h"
#include <vector>

class AccountUnlocksRepository {
public:
    struct AccountUnlock {
        uint32 id;
        uint32 account_id;
        std::string unlock_key;
        std::string unlocked_at;
        uint32 unlocked_by_char_id;
        std::string unlocked_by_char_name;
    };

    static std::string TableName() { return "account_unlocks"; }

    static std::vector<AccountUnlock> GetWhere(Database& db, const std::string& where) {
        std::vector<AccountUnlock> results;

        auto query = fmt::format(
            "SELECT id, account_id, unlock_key, unlocked_at, "
            "unlocked_by_char_id, unlocked_by_char_name "
            "FROM {} WHERE {}",
            TableName(), where
        );

        auto results_set = db.QueryDatabase(query);

        for (auto row = results_set.begin(); row != results_set.end(); ++row) {
            AccountUnlock entry{};
            entry.id = Strings::ToUnsignedInt(row[0]);
            entry.account_id = Strings::ToUnsignedInt(row[1]);
            entry.unlock_key = row[2] ? row[2] : "";
            entry.unlocked_at = row[3] ? row[3] : "";
            entry.unlocked_by_char_id = Strings::ToUnsignedInt(row[4]);
            entry.unlocked_by_char_name = row[5] ? row[5] : "";
            results.push_back(entry);
        }

        return results;
    }

    static std::vector<AccountUnlock> GetByAccountId(Database& db, uint32 account_id) {
        return GetWhere(db, fmt::format("account_id = {}", account_id));
    }

    static bool HasUnlock(Database& db, uint32 account_id, const std::string& unlock_key) {
        auto query = fmt::format(
            "SELECT 1 FROM {} WHERE account_id = {} AND unlock_key = '{}' LIMIT 1",
            TableName(), account_id, Strings::Escape(unlock_key)
        );
        auto results = db.QueryDatabase(query);
        return results.RowCount() > 0;
    }

    static bool Insert(Database& db, uint32 account_id, const std::string& unlock_key,
                       uint32 char_id = 0, const std::string& char_name = "") {
        auto query = fmt::format(
            "INSERT IGNORE INTO {} (account_id, unlock_key, unlocked_by_char_id, "
            "unlocked_by_char_name) VALUES ({}, '{}', {}, '{}')",
            TableName(), account_id, Strings::Escape(unlock_key),
            char_id, Strings::Escape(char_name)
        );
        auto results = db.QueryDatabase(query);
        return results.RowsAffected() > 0;
    }

    static bool Delete(Database& db, uint32 account_id, const std::string& unlock_key) {
        auto query = fmt::format(
            "DELETE FROM {} WHERE account_id = {} AND unlock_key = '{}'",
            TableName(), account_id, Strings::Escape(unlock_key)
        );
        auto results = db.QueryDatabase(query);
        return results.RowsAffected() > 0;
    }
};

#endif
```

Create similar repositories for:
- `account_perks_repository.h`
- `account_veteran_cache_repository.h`
- `account_zone_locks_repository.h`
- `account_perk_definitions_repository.h`

### Step 1.3: Test Database Layer

```cpp
// Simple test
auto unlocks = AccountUnlocksRepository::GetByAccountId(database, 1);
LogInfo("Account 1 has {} unlocks", unlocks.size());

AccountUnlocksRepository::Insert(database, 1, "test_unlock");
assert(AccountUnlocksRepository::HasUnlock(database, 1, "test_unlock"));
AccountUnlocksRepository::Delete(database, 1, "test_unlock");
```

---

## Phase 2: Core Manager Class

### Step 2.1: Create Header

Create `common/account_progression.h`:

```cpp
#ifndef EQEMU_ACCOUNT_PROGRESSION_H
#define EQEMU_ACCOUNT_PROGRESSION_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <memory>

class Database;

class AccountProgressionManager {
public:
    // Singleton access
    static AccountProgressionManager& Instance();

    // Initialize with database reference
    void Initialize(Database* db);

    // Configuration
    void LoadConfiguration();
    void ReloadConfiguration();

    // === UNLOCKS ===
    bool HasUnlock(uint32 account_id, const std::string& unlock_key);
    bool GrantUnlock(uint32 account_id, const std::string& unlock_key,
                     uint32 char_id = 0, const std::string& char_name = "");
    bool RemoveUnlock(uint32 account_id, const std::string& unlock_key);
    std::vector<std::string> GetAllUnlocks(uint32 account_id);

    // === ZONE ACCESS ===
    bool CanEnterZone(uint32 account_id, uint32 zone_id, uint8 char_level,
                      std::string& out_message);

    // === LEVEL CAPS ===
    uint8 GetEffectiveLevelCap(uint32 account_id, uint8 base_cap);

    // === PERKS ===
    bool HasPerk(uint32 account_id, const std::string& perk_type,
                 const std::string& perk_key);
    int GetPerkValue(uint32 account_id, const std::string& perk_type,
                     const std::string& perk_key);
    int GrantPerk(uint32 account_id, const std::string& perk_type,
                  const std::string& perk_key, int value,
                  const std::string& source = "");

    struct PerkInfo {
        int current_value = 0;
        int max_value = -1;
        int times_completed = 0;
        int next_stack_value = 0;
        bool at_cap = false;
    };
    PerkInfo GetPerkInfo(uint32 account_id, const std::string& perk_type,
                         const std::string& perk_key);

    // === VETERAN ===
    struct VeteranData {
        uint8 highest_level = 1;
        uint32 highest_level_char_id = 0;
        uint32 total_aa_points = 0;
        uint32 character_count = 0;
    };
    VeteranData GetVeteranData(uint32 account_id);
    void RefreshVeteranCache(uint32 account_id);
    float GetVeteranXPMultiplier(uint32 account_id, uint8 char_level);
    float GetVeteranAAMultiplier(uint32 account_id, uint32 char_aa_spent);
    bool HasMaxLevelCharacter(uint32 account_id, uint8 server_max_level);

private:
    AccountProgressionManager() = default;
    ~AccountProgressionManager() = default;
    AccountProgressionManager(const AccountProgressionManager&) = delete;
    AccountProgressionManager& operator=(const AccountProgressionManager&) = delete;

    Database* m_database = nullptr;

    // Caches
    std::unordered_map<uint32, std::unordered_set<std::string>> m_unlock_cache;
    std::unordered_map<uint32, VeteranData> m_veteran_cache;

    // Configuration (loaded from database)
    std::map<std::string, float> m_veteran_rules;

    // Perk definition cache
    struct PerkDefinition {
        std::string perk_type;
        std::string perk_key;
        int base_value = 1;
        int max_value = -1;
        int max_stacks = -1;
        std::string diminishing_type = "none";
        float diminishing_factor = 0;
        bool stackable = false;
        bool enabled = true;
    };
    std::map<std::string, PerkDefinition> m_perk_definitions;  // key = type:perk_key

    // Zone lock cache
    struct ZoneLock {
        uint32 zone_id;
        std::string required_unlock_key;
        bool bypass_with_max_level = true;
        uint8 bypass_level = 0;
        std::string lock_message;
        bool enabled = true;
    };
    std::unordered_map<uint32, ZoneLock> m_zone_locks;

    // Helper functions
    void CacheAccountUnlocks(uint32 account_id);
    void InvalidateUnlockCache(uint32 account_id);
    int CalculateDiminishedValue(const PerkDefinition& def, int times_completed);
    PerkDefinition* GetPerkDefinition(const std::string& perk_type, const std::string& perk_key);
    float GetVeteranRule(const std::string& key, float default_value);
};

#endif
```

### Step 2.2: Create Implementation

Create `common/account_progression.cpp`:

```cpp
#include "account_progression.h"
#include "database.h"
#include "eqemu_logsys.h"
#include "repositories/account_unlocks_repository.h"
// ... other repository includes

AccountProgressionManager& AccountProgressionManager::Instance() {
    static AccountProgressionManager instance;
    return instance;
}

void AccountProgressionManager::Initialize(Database* db) {
    m_database = db;
    LoadConfiguration();
    LogInfo("AccountProgressionManager initialized");
}

void AccountProgressionManager::LoadConfiguration() {
    if (!m_database) return;

    // Load veteran rules
    m_veteran_rules.clear();
    auto rules_query = m_database->QueryDatabase(
        "SELECT rule_key, rule_value FROM account_veteran_rules"
    );
    for (auto row = rules_query.begin(); row != rules_query.end(); ++row) {
        m_veteran_rules[row[0]] = Strings::ToFloat(row[1]);
    }

    // Load perk definitions
    m_perk_definitions.clear();
    // ... load from account_perk_definitions

    // Load zone locks
    m_zone_locks.clear();
    // ... load from account_zone_locks

    LogInfo("AccountProgression config loaded: {} rules, {} perk defs, {} zone locks",
            m_veteran_rules.size(), m_perk_definitions.size(), m_zone_locks.size());
}

// ... implement all methods
```

### Step 2.3: Add to CMake

Edit `common/CMakeLists.txt`:

```cmake
SET(common_sources
    # ... existing sources ...
    account_progression.cpp
)
```

### Step 2.4: Initialize on Startup

In `zone/main.cpp` or `zone/zone.cpp`:

```cpp
#include "../common/account_progression.h"

// During zone initialization, after database is connected:
AccountProgressionManager::Instance().Initialize(&database);
```

---

## Phase 3: Account Unlocks

Implement in `account_progression.cpp`:

```cpp
bool AccountProgressionManager::HasUnlock(uint32 account_id, const std::string& unlock_key) {
    // Check cache first
    auto it = m_unlock_cache.find(account_id);
    if (it != m_unlock_cache.end()) {
        return it->second.count(unlock_key) > 0;
    }

    // Load from database and cache
    CacheAccountUnlocks(account_id);

    it = m_unlock_cache.find(account_id);
    return it != m_unlock_cache.end() && it->second.count(unlock_key) > 0;
}

bool AccountProgressionManager::GrantUnlock(uint32 account_id, const std::string& unlock_key,
                                             uint32 char_id, const std::string& char_name) {
    // Try to insert (INSERT IGNORE handles duplicates)
    bool was_new = AccountUnlocksRepository::Insert(*m_database, account_id,
                                                     unlock_key, char_id, char_name);

    if (was_new) {
        // Update cache
        m_unlock_cache[account_id].insert(unlock_key);

        LogInfo("Account {} granted unlock '{}' by char {} ({})",
                account_id, unlock_key, char_id, char_name);
    }

    return was_new;
}

void AccountProgressionManager::CacheAccountUnlocks(uint32 account_id) {
    auto unlocks = AccountUnlocksRepository::GetByAccountId(*m_database, account_id);

    auto& cache_set = m_unlock_cache[account_id];
    cache_set.clear();

    for (const auto& unlock : unlocks) {
        cache_set.insert(unlock.unlock_key);
    }
}
```

### Test Unlocks

```cpp
// In a test or GM command:
auto& mgr = AccountProgressionManager::Instance();

// Should be false
assert(!mgr.HasUnlock(1, "test_key"));

// Grant it
assert(mgr.GrantUnlock(1, "test_key", 123, "TestChar"));

// Should now be true
assert(mgr.HasUnlock(1, "test_key"));

// Duplicate should return false
assert(!mgr.GrantUnlock(1, "test_key"));

// Remove
mgr.RemoveUnlock(1, "test_key");
assert(!mgr.HasUnlock(1, "test_key"));
```

---

## Phase 4: Zone Access Checks

### Step 4.1: Implement Check

```cpp
bool AccountProgressionManager::CanEnterZone(uint32 account_id, uint32 zone_id,
                                              uint8 char_level, std::string& out_message) {
    // Find zone lock
    auto it = m_zone_locks.find(zone_id);
    if (it == m_zone_locks.end() || !it->second.enabled) {
        return true;  // No lock on this zone
    }

    const auto& lock = it->second;

    // Check if has the required unlock
    if (HasUnlock(account_id, lock.required_unlock_key)) {
        return true;
    }

    // Check max level bypass
    if (lock.bypass_with_max_level) {
        uint8 bypass_level = lock.bypass_level > 0 ? lock.bypass_level :
                             RuleI(Character, MaxLevel);  // Or however you get max level

        if (HasMaxLevelCharacter(account_id, bypass_level)) {
            return true;
        }
    }

    // Access denied
    out_message = lock.lock_message;
    return false;
}
```

### Step 4.2: Hook into Zone Entry

In `zone/client.cpp`, find where zone entry is validated:

```cpp
// In Client::Handle_OP_ZoneChange or similar
bool Client::CheckZoneAccess(uint32 zone_id) {
    std::string deny_message;

    if (!AccountProgressionManager::Instance().CanEnterZone(
            AccountID(), zone_id, GetLevel(), deny_message)) {
        Message(Chat::Red, deny_message.c_str());
        return false;
    }

    return true;
}
```

---

## Phase 5-6: Account Perks

See [API_REFERENCE.md](API_REFERENCE.md) for the full perk API.

Key implementation points:

1. **Non-stackable perks**: Simple presence check, value is always base_value
2. **Stackable perks**: Track `times_completed`, calculate actual value each time
3. **Diminishing returns**: Apply formula based on `diminishing_type`

```cpp
int AccountProgressionManager::CalculateDiminishedValue(const PerkDefinition& def,
                                                         int times_completed) {
    float value = static_cast<float>(def.base_value);

    if (def.diminishing_type == "linear") {
        value -= times_completed * def.diminishing_factor;
    } else if (def.diminishing_type == "exponential") {
        value *= std::pow(def.diminishing_factor, times_completed);
    }
    // "none" = no change

    return std::max(0, static_cast<int>(std::round(value)));
}
```

---

## Phase 7: Veteran Bonuses

```cpp
float AccountProgressionManager::GetVeteranXPMultiplier(uint32 account_id, uint8 char_level) {
    auto veteran = GetVeteranData(account_id);

    if (char_level >= veteran.highest_level) {
        return 0.0f;  // No bonus for highest or equal level
    }

    float bonus_per_level = GetVeteranRule("xp_bonus_per_level", 0.02f);
    float max_bonus = GetVeteranRule("xp_bonus_max", 1.0f);

    float bonus = (veteran.highest_level - char_level) * bonus_per_level;
    return std::min(bonus, max_bonus);
}

void AccountProgressionManager::RefreshVeteranCache(uint32 account_id) {
    // Query character_data for this account
    auto query = m_database->QueryDatabase(fmt::format(
        "SELECT MAX(level), SUM(aa_points_spent), COUNT(*) "
        "FROM character_data WHERE account_id = {} AND deleted_at IS NULL",
        account_id
    ));

    if (query.RowCount() > 0) {
        auto row = query.begin();
        VeteranData data;
        data.highest_level = Strings::ToUnsignedInt(row[0]);
        data.total_aa_points = Strings::ToUnsignedInt(row[1]);
        data.character_count = Strings::ToUnsignedInt(row[2]);

        m_veteran_cache[account_id] = data;

        // Also update database cache table
        // ...
    }
}
```

---

## Phase 8: Lua Bindings

Create bindings in `zone/lua_general.cpp`:

```cpp
int lua_has_account_unlock(lua_State* L) {
    Lua_Client* client = Lua_Client::GetUserData(L, 1);
    const char* unlock_key = luaL_checkstring(L, 2);

    if (client && client->GetClient()) {
        bool has = AccountProgressionManager::Instance()
            .HasUnlock(client->GetClient()->AccountID(), unlock_key);
        lua_pushboolean(L, has);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

int lua_grant_account_unlock(lua_State* L) {
    Lua_Client* client = Lua_Client::GetUserData(L, 1);
    const char* unlock_key = luaL_checkstring(L, 2);
    bool show_message = lua_isboolean(L, 3) ? lua_toboolean(L, 3) : true;

    if (client && client->GetClient()) {
        auto c = client->GetClient();
        bool was_new = AccountProgressionManager::Instance()
            .GrantUnlock(c->AccountID(), unlock_key, c->CharacterID(), c->GetName());

        if (was_new && show_message) {
            // Could look up unlock definition for custom message
            c->Message(Chat::Yellow, "Account unlock granted: %s", unlock_key);
        }

        lua_pushboolean(L, was_new);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Register in lua_register_general:
lua_register(L, "has_account_unlock", lua_has_account_unlock);
lua_register(L, "grant_account_unlock", lua_grant_account_unlock);
// ... etc
```

---

## Phase 9: Commands

Add to `zone/command.cpp`:

```cpp
void command_account(Client *c, const Seperator *sep) {
    std::string subcommand = sep->arg[1] ? sep->arg[1] : "summary";
    auto& mgr = AccountProgressionManager::Instance();
    uint32 account_id = c->AccountID();

    if (subcommand == "summary" || subcommand == "all") {
        c->Message(Chat::Yellow, "=== Account Progression ===");

        // Unlocks summary
        auto unlocks = mgr.GetAllUnlocks(account_id);
        c->Message(Chat::White, "Unlocks: %zu earned", unlocks.size());

        // Veteran info
        auto veteran = mgr.GetVeteranData(account_id);
        c->Message(Chat::White, "Highest Level: %d", veteran.highest_level);
        c->Message(Chat::White, "Total Account AAs: %d", veteran.total_aa_points);

        // Bonus info
        float xp_bonus = mgr.GetVeteranXPMultiplier(account_id, c->GetLevel());
        float aa_bonus = mgr.GetVeteranAAMultiplier(account_id, c->GetAAPointsSpent());
        c->Message(Chat::White, "Current XP Bonus: +%.0f%%", xp_bonus * 100);
        c->Message(Chat::White, "Current AA Bonus: +%.0f%%", aa_bonus * 100);
    }

    if (subcommand == "unlocks" || subcommand == "all") {
        auto unlocks = mgr.GetAllUnlocks(account_id);
        c->Message(Chat::Yellow, "--- Account Unlocks (%zu) ---", unlocks.size());
        for (const auto& key : unlocks) {
            c->Message(Chat::White, "  - %s", key.c_str());
        }
    }

    // ... perks, veteran details
}

// Register in command_init:
command_add("account", "Show account progression", 0, command_account);
```

---

## Phase 10: Testing

Follow the [TESTING_GUIDE.md](TESTING_GUIDE.md) to:

1. Run unit tests
2. Run Lua integration tests
3. Complete manual testing checklist
4. Test edge cases

---

## Common Pitfalls

1. **Forgetting to refresh cache** - Always call `RefreshVeteranCache` on login
2. **Case sensitivity** - Unlock keys are case-sensitive, be consistent
3. **Thread safety** - If zone is multi-threaded, add mutex to cache access
4. **Database escaping** - Always use `Strings::Escape()` for string inputs
5. **Float precision** - Use `std::round()` when converting float bonuses to int

---

## Next Steps After Implementation

1. Create content (zone locks, perks, quests that grant them)
2. Add admin tools (web panel, more GM commands)
3. Monitor performance (add logging, check cache hit rates)
4. Gather player feedback and tune values
