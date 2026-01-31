# Account Progression - API Reference

Complete API documentation for implementing the account progression system.

## Table of Contents

1. [C++ Classes](#c-classes)
2. [Lua Functions](#lua-functions)
3. [Commands](#commands)
4. [Return Codes](#return-codes)
5. [Events](#events)

---

## C++ Classes

### AccountProgressionManager

**Location:** `common/account_progression.h` and `common/account_progression.cpp`

This singleton manages all account progression operations.

```cpp
class AccountProgressionManager {
public:
    static AccountProgressionManager& Instance();
    
    // ============================================
    // ACCOUNT UNLOCKS
    // ============================================
    
    /**
     * Check if account has a specific unlock
     * @param account_id The account to check
     * @param unlock_key The unlock key (e.g., "sebilis_key")
     * @return true if account has this unlock
     */
    bool HasUnlock(uint32 account_id, const std::string& unlock_key);
    
    /**
     * Grant an unlock to an account
     * @param account_id The account to grant to
     * @param unlock_key The unlock key
     * @param char_id Character who earned it (for logging)
     * @param char_name Character name (for logging)
     * @return true if granted (false if already had it)
     */
    bool GrantUnlock(uint32 account_id, const std::string& unlock_key, 
                     uint32 char_id = 0, const std::string& char_name = "");
    
    /**
     * Remove an unlock from an account (admin use)
     * @return true if removed
     */
    bool RemoveUnlock(uint32 account_id, const std::string& unlock_key);
    
    /**
     * Get all unlocks for an account
     * @return Vector of unlock keys
     */
    std::vector<std::string> GetAllUnlocks(uint32 account_id);
    
    // ============================================
    // ZONE ACCESS
    // ============================================
    
    /**
     * Check if account can enter a zone
     * @param account_id The account
     * @param zone_id The zone to check
     * @param char_level Character's current level (for bypass checks)
     * @param out_message Output: message to show if denied
     * @return true if access allowed
     */
    bool CanEnterZone(uint32 account_id, uint32 zone_id, uint8 char_level,
                      std::string& out_message);
    
    /**
     * Get the unlock key required for a zone
     * @return Empty string if zone has no lock
     */
    std::string GetZoneRequiredUnlock(uint32 zone_id);
    
    // ============================================
    // LEVEL CAPS
    // ============================================
    
    /**
     * Get the effective level cap for an account
     * @param account_id The account
     * @param base_cap The server's base level cap
     * @return The account's current level cap
     */
    uint8 GetEffectiveLevelCap(uint32 account_id, uint8 base_cap);
    
    // ============================================
    // ACCOUNT PERKS
    // ============================================
    
    /**
     * Check if account has a specific perk
     */
    bool HasPerk(uint32 account_id, const std::string& perk_type, 
                 const std::string& perk_key);
    
    /**
     * Get perk value (returns 0 if not owned)
     */
    int GetPerkValue(uint32 account_id, const std::string& perk_type,
                     const std::string& perk_key);
    
    /**
     * Grant or stack a perk
     * @param account_id Account to grant to
     * @param perk_type Type of perk ("stat", "teleport", etc.)
     * @param perk_key Specific perk ("str", "neriak", etc.)
     * @param value Value to add (for stackable) or set (for non-stackable)
     * @param source Description of where this came from (for logging)
     * @return Actual value added (may be less due to caps/diminishing)
     */
    int GrantPerk(uint32 account_id, const std::string& perk_type,
                  const std::string& perk_key, int value,
                  const std::string& source = "");
    
    /**
     * Get detailed perk info for stackable perks
     */
    struct PerkInfo {
        int current_value;
        int max_value;       // -1 if no cap
        int times_completed;
        int next_stack_value; // How much next stack would add
        bool at_cap;
    };
    PerkInfo GetPerkInfo(uint32 account_id, const std::string& perk_type,
                         const std::string& perk_key);
    
    /**
     * Get all perks of a specific type
     * @return Map of perk_key -> current_value
     */
    std::map<std::string, int> GetPerksOfType(uint32 account_id, 
                                               const std::string& perk_type);
    
    /**
     * Get all perks for an account (for #account display)
     */
    std::vector<std::tuple<std::string, std::string, int>> GetAllPerks(uint32 account_id);
    
    // ============================================
    // VETERAN BONUSES
    // ============================================
    
    /**
     * Get the cached veteran data for an account
     */
    struct VeteranData {
        uint8 highest_level;
        uint32 highest_level_char_id;
        uint32 total_aa_points;
        uint32 character_count;
        uint32 total_play_time_minutes;
    };
    VeteranData GetVeteranData(uint32 account_id);
    
    /**
     * Refresh veteran cache for an account (call on login)
     */
    void RefreshVeteranCache(uint32 account_id);
    
    /**
     * Calculate XP bonus multiplier for a character
     * @param account_id The account
     * @param char_level The character's current level
     * @return Multiplier (e.g., 0.5 = 50% bonus)
     */
    float GetVeteranXPMultiplier(uint32 account_id, uint8 char_level);
    
    /**
     * Calculate AA bonus multiplier for a character
     * @param account_id The account
     * @param char_aa_spent Character's spent AA points
     * @return Multiplier (e.g., 0.3 = 30% bonus)
     */
    float GetVeteranAAMultiplier(uint32 account_id, uint32 char_aa_spent);
    
    /**
     * Check if account has a max-level character (for inherited unlocks)
     * @param server_max_level The server's maximum level
     */
    bool HasMaxLevelCharacter(uint32 account_id, uint8 server_max_level);
    
    // ============================================
    // CONFIGURATION
    // ============================================
    
    /**
     * Load/reload configuration from database
     */
    void LoadConfiguration();
    
    /**
     * Get a veteran rule value
     */
    float GetVeteranRule(const std::string& rule_key, float default_value);
    
private:
    AccountProgressionManager() = default;
    
    // Cache structures
    std::unordered_map<uint32, std::unordered_set<std::string>> m_unlock_cache;
    std::unordered_map<uint32, VeteranData> m_veteran_cache;
    std::map<std::string, float> m_veteran_rules;
    
    // Diminishing returns calculation
    int CalculateDiminishedValue(int base_value, int times_completed,
                                  const std::string& dim_type, float dim_factor);
};
```

### Usage in Zone Code

```cpp
// In zone/client.cpp - when entering a zone
bool Client::CanEnterZone(uint32 zone_id) {
    std::string deny_message;
    if (!AccountProgressionManager::Instance().CanEnterZone(
            GetAccountID(), zone_id, GetLevel(), deny_message)) {
        Message(Chat::Red, deny_message.c_str());
        return false;
    }
    return true;
}

// In zone/exp.cpp - when calculating XP
uint64 Client::GetAdjustedXP(uint64 base_xp) {
    float veteran_mult = AccountProgressionManager::Instance()
        .GetVeteranXPMultiplier(GetAccountID(), GetLevel());
    return base_xp * (1.0f + veteran_mult);
}

// In zone/client.cpp - on login
void Client::OnZoneEntry() {
    // Refresh veteran cache
    AccountProgressionManager::Instance().RefreshVeteranCache(GetAccountID());
    
    // Apply account stat perks
    ApplyAccountPerks();
}

void Client::ApplyAccountPerks() {
    auto stat_perks = AccountProgressionManager::Instance()
        .GetPerksOfType(GetAccountID(), "stat");
    
    for (const auto& [stat_key, value] : stat_perks) {
        if (stat_key == "str") AddAccountStatBonus(EQ::stats::Str, value);
        else if (stat_key == "sta") AddAccountStatBonus(EQ::stats::Sta, value);
        // ... etc
    }
}
```

---

## Lua Functions

All Lua functions are available via the `eq` namespace.

### Unlock Functions

```lua
-- Check if player's account has an unlock
-- @param e.self or client userdata
-- @param unlock_key string
-- @return boolean
eq.has_account_unlock(client, unlock_key)

-- Grant an unlock to player's account
-- @param e.self or client userdata
-- @param unlock_key string
-- @param show_message boolean (optional, default true)
-- @return boolean (true if newly granted, false if already had)
eq.grant_account_unlock(client, unlock_key, show_message)

-- Remove an unlock (admin use)
-- @return boolean
eq.remove_account_unlock(client, unlock_key)

-- Get all unlocks as a table
-- @return table of unlock keys
eq.get_account_unlocks(client)
```

**Example:**

```lua
function event_death(e)
    -- When raid boss dies, grant unlock to everyone in raid
    if e.self:GetNPCTypeID() == 117000 then  -- Lord Nagafen
        local raid = eq.get_entity_list():GetRaidByClient(e.killer)
        if raid.valid then
            for i = 0, raid:RaidCount() - 1 do
                local member = raid:GetMember(i)
                if member.valid then
                    eq.grant_account_unlock(member, "killed_nagafen", true)
                end
            end
        end
    end
end
```

### Perk Functions

```lua
-- Check if account has a perk
-- @return boolean
eq.has_account_perk(client, perk_type, perk_key)

-- Get perk value (0 if not owned)
-- @return integer
eq.get_account_perk(client, perk_type, perk_key)

-- Grant a perk (handles stacking automatically)
-- @param client userdata
-- @param perk_type string ("stat", "teleport", etc.)
-- @param perk_key string ("str", "neriak", etc.)
-- @param value integer (optional, uses definition base_value if omitted)
-- @param source string (optional, for logging)
-- @return integer (actual value added, may be reduced by caps)
eq.grant_account_perk(client, perk_type, perk_key, value, source)

-- Get detailed perk info (for stackable perks)
-- @return table {current_value, max_value, times_completed, next_stack_value, at_cap}
eq.get_account_perk_info(client, perk_type, perk_key)

-- Get all perks of a type as table
-- @return table {perk_key = value, ...}
eq.get_account_perks_of_type(client, perk_type)
```

**Example:**

```lua
function event_task_complete(e)
    if e.task_id == 150 then  -- Wisdom trial quest
        local info = eq.get_account_perk_info(e.self, "stat", "wis")
        
        if info.at_cap then
            e.self:Message(Chat.Yellow, "You have already mastered this wisdom.")
        else
            local added = eq.grant_account_perk(e.self, "stat", "wis", 2, "Wisdom Trial")
            e.self:Message(Chat.Yellow, 
                string.format("Your wisdom grows by %d! (Total: %d/%d, Completed %d times)",
                added, info.current_value + added, info.max_value, info.times_completed + 1))
        end
    end
end
```

### Veteran Bonus Functions

```lua
-- Get veteran XP bonus multiplier
-- @return float (e.g., 0.5 = 50% bonus)
eq.get_veteran_xp_bonus(client)

-- Get veteran AA bonus multiplier  
-- @return float
eq.get_veteran_aa_bonus(client)

-- Get veteran data as table
-- @return table {highest_level, total_aa_points, character_count}
eq.get_veteran_data(client)

-- Check if account has max level character
-- @return boolean
eq.has_max_level_character(client)

-- Manually refresh veteran cache (normally automatic on login)
eq.refresh_veteran_cache(client)
```

**Example:**

```lua
function event_say(e)
    if e.message:findi("veteran status") then
        local data = eq.get_veteran_data(e.self)
        local xp_bonus = eq.get_veteran_xp_bonus(e.self)
        local aa_bonus = eq.get_veteran_aa_bonus(e.self)
        
        e.self:Message(Chat.Yellow, "=== Veteran Status ===")
        e.self:Message(Chat.White, string.format(
            "Highest Character: Level %d", data.highest_level))
        e.self:Message(Chat.White, string.format(
            "Total Account AAs: %d", data.total_aa_points))
        e.self:Message(Chat.White, string.format(
            "XP Bonus: +%.0f%%", xp_bonus * 100))
        e.self:Message(Chat.White, string.format(
            "AA Bonus: +%.0f%%", aa_bonus * 100))
    end
end
```

### Zone Access Functions

```lua
-- Check if account can enter a zone
-- @return boolean, string (allowed, deny_message)
eq.can_enter_zone(client, zone_id)

-- Get required unlock for a zone
-- @return string (unlock_key or empty)
eq.get_zone_required_unlock(zone_id)
```

### Level Cap Functions

```lua
-- Get account's effective level cap
-- @return integer
eq.get_account_level_cap(client)

-- Check if character is at their account's level cap
-- @return boolean
eq.at_account_level_cap(client)
```

---

## Commands

### Player Commands

#### #account or #progression

Shows account progression status.

```
Usage: #account [unlocks|perks|veteran|all]

#account           - Show summary
#account unlocks   - List all account unlocks
#account perks     - List all account perks with values
#account veteran   - Show veteran bonus details
#account all       - Show everything
```

**Implementation:**

```cpp
void command_account(Client *c, const Seperator *sep) {
    std::string subcommand = sep->arg[1] ? sep->arg[1] : "";
    auto& mgr = AccountProgressionManager::Instance();
    
    if (subcommand.empty() || subcommand == "all") {
        // Show summary
        c->Message(Chat::White, "=== Account Progression ===");
        
        auto unlocks = mgr.GetAllUnlocks(c->AccountID());
        c->Message(Chat::White, "Unlocks: %zu", unlocks.size());
        
        auto perks = mgr.GetAllPerks(c->AccountID());
        c->Message(Chat::White, "Perks: %zu", perks.size());
        
        auto veteran = mgr.GetVeteranData(c->AccountID());
        c->Message(Chat::White, "Highest Level: %d", veteran.highest_level);
        
        float xp_bonus = mgr.GetVeteranXPMultiplier(c->AccountID(), c->GetLevel());
        float aa_bonus = mgr.GetVeteranAAMultiplier(c->AccountID(), c->GetAAPointsSpent());
        c->Message(Chat::White, "XP Bonus: +%.0f%%, AA Bonus: +%.0f%%", 
                   xp_bonus * 100, aa_bonus * 100);
    }
    
    if (subcommand == "unlocks" || subcommand == "all") {
        auto unlocks = mgr.GetAllUnlocks(c->AccountID());
        c->Message(Chat::Yellow, "--- Account Unlocks ---");
        for (const auto& key : unlocks) {
            c->Message(Chat::White, "  - %s", key.c_str());
        }
    }
    
    // ... similar for perks and veteran
}
```

### GM Commands

#### #grantunlock

```
Usage: #grantunlock <target> <unlock_key>
       #grantunlock <account_id> <unlock_key>

Examples:
  #grantunlock Playername sebilis_key
  #grantunlock 12345 sebilis_key
```

#### #removeunlock

```
Usage: #removeunlock <target> <unlock_key>
```

#### #grantperk

```
Usage: #grantperk <target> <perk_type> <perk_key> [value]

Examples:
  #grantperk Playername stat str 2
  #grantperk Playername teleport neriak
```

#### #setlevelcap

```
Usage: #setlevelcap <target> <level>

Grants necessary unlocks to achieve the specified level cap.
```

#### #refreshveteran

```
Usage: #refreshveteran [target]

Force refresh veteran cache for target (or self).
```

---

## Return Codes

### GrantUnlock Return Values

| Return | Meaning |
|--------|---------|
| true   | Unlock newly granted |
| false  | Already had unlock |

### GrantPerk Return Values

| Return | Meaning |
|--------|---------|
| > 0    | Value successfully added |
| 0      | At cap, nothing added |
| -1     | Invalid perk type/key |
| -2     | Perk is disabled |

### CanEnterZone Return Values

| Return | out_message | Meaning |
|--------|-------------|---------|
| true   | empty | Access allowed |
| false  | filled | Access denied, message explains why |

---

## Events

The system fires these Lua events that quest scripts can handle.

### EVENT_ACCOUNT_UNLOCK

Fired when an account unlock is granted.

```lua
function event_account_unlock(e)
    -- e.unlock_key: The unlock that was granted
    -- e.self: The client who earned it
    -- e.is_new: true if newly granted, false if already had
    
    if e.unlock_key == "classic_complete" and e.is_new then
        e.self:Message(Chat.Yellow, "The path to Kunark opens before you!")
    end
end
```

### EVENT_ACCOUNT_PERK

Fired when a perk is granted or stacked.

```lua
function event_account_perk(e)
    -- e.perk_type: Type of perk
    -- e.perk_key: Specific perk
    -- e.value_added: How much was actually added
    -- e.new_total: Current total value
    -- e.times_completed: How many times this perk has been earned
end
```

### EVENT_VETERAN_LEVEL_UP

Fired when account's highest level character levels up.

```lua
function event_veteran_level_up(e)
    -- e.new_highest_level: The new highest level
    -- e.previous_highest: What it was before
    
    -- Could trigger account-wide bonuses at milestones
    if e.new_highest_level == 50 then
        eq.zone_emote(Chat.Yellow, 
            string.format("%s's account has reached level 50!", e.self:GetName()))
    end
end
```
