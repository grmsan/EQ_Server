# Account-Based Progression System

## Overview

Account-based progression allows achievements, unlocks, and rewards to persist across all characters on a player's account. This reduces repetitive content for alts while preserving meaningful progression milestones.

## Design Goals

1. **Reduce Alt Tedium** - One-time unlocks shouldn't be repeated endlessly
2. **Preserve Achievement Value** - First completion should still feel rewarding
3. **Enable Content Gating** - Control access to zones/content based on account progress
4. **Account-Wide Rewards** - Perks that benefit all characters
5. **Flexible Configuration** - Easy to add new unlocks without code changes

---

## Core Concepts

### 1. Account Unlocks

One-time achievements that permanently unlock content for all characters.

| Category | Example | Trigger | Effect |
|----------|---------|---------|--------|
| Zone Keys | Sebilis Key | Complete key quest | All chars can enter Sebilis |
| Level Caps | Classic Cap | Kill Nagafen + Vox | Raise level cap from 50 to 60 |
| Zone Access | Lower Guk | Complete Upper Guk quest | All chars can enter Lower Guk |
| Features | Bazaar Access | Reach level 10 on any char | All chars can use Bazaar |

### 2. Content Gates

Restrictions that apply until account unlocks are earned.

| Gate Type | Description |
|-----------|-------------|
| Level Cap | Maximum level until condition met |
| Zone Lock | Cannot enter zone until unlocked |
| Expansion Lock | Entire expansion content gated |
| Feature Lock | Specific features disabled |

### 3. Account Perks

Permanent bonuses earned through achievements.

| Perk Type | Example | Source |
|-----------|---------|--------|
| Teleport AA | Gate to Neriak | Neriak quest chain |
| Stat Bonus | +2 STR | Ogre heritage quest |
| Faction Fix | Neriak faction reset | Neriak quest chain |
| Cosmetic | Special title | Achievement |
| Convenience | Expanded bank slots | Progression milestone |

### 4. Inherited Progress (Persistent Unlocks)

Once an account reaches certain milestones, ALL new characters inherit those benefits immediately.

| Milestone | Inherited Benefit |
|-----------|-------------------|
| Max level character exists | All zones unlocked for new chars |
| Character reached Kunark | New chars can access Kunark zones |
| Account has Seb key | New chars can enter Sebilis at level 1 |

**Philosophy**: If you've "beaten" content on your main, your alts shouldn't be gated.

### 5. Veteran Bonuses

Dynamic bonuses based on account-wide progress that help alts catch up.

#### XP Bonus (Highest Level Character)

New/lower characters gain bonus XP based on your highest level character:

```
XP Bonus = (HighestCharLevel - CurrentCharLevel) * BonusPerLevel
Example: Main is 60, alt is 20 → (60-20) * 2% = 80% bonus XP
```

| Level Difference | XP Bonus |
|------------------|----------|
| 1-10 levels behind | +10-20% |
| 11-30 levels behind | +22-60% |
| 31-50 levels behind | +62-100% |
| 50+ levels behind | +100% (capped) |

#### AA Bonus (Veteran AA System)

Bonus AA gain based on total AAs across OTHER characters on account:

```
Total Account AAs (excluding current char) = 2500
Veteran Bonus = TotalAAs * 0.5 (50% of total as bonus pool)
AA Gain Rate = Base + (VeteranBonus / SomeScalar)
```

**Example Implementation**:
- Account has 3 characters with 1000, 800, and 700 AAs
- New 4th character created
- Veteran pool = (1000 + 800 + 700) * 0.5 = 1250 bonus AAs worth of accelerated gain
- Could manifest as: +50% AA gain rate until "caught up"

### 6. Stackable Perks with Tracking

Some perks can be earned multiple times with diminishing returns or caps.

#### Perk Tracking Structure

```sql
CREATE TABLE `account_perk_progress` (
    `account_id` INT UNSIGNED NOT NULL,
    `perk_key` VARCHAR(64) NOT NULL,
    `times_completed` INT UNSIGNED DEFAULT 0,
    `current_value` INT NOT NULL DEFAULT 0,
    `max_value` INT NOT NULL DEFAULT 0,
    `last_completed` DATETIME,
    PRIMARY KEY (`account_id`, `perk_key`)
);
```

#### Stacking Examples

| Perk | Per Completion | Max Stacks | Max Value | Diminishing? |
|------|----------------|------------|-----------|--------------|
| Ogre STR | +2 STR | 5 | +10 STR | No |
| Troll Regen | +1 HP/tick | 10 | +5 HP/tick | Yes (after 5) |
| Scholar's Wisdom | +1 INT | 20 | +10 INT | Yes (halves each time) |

#### Diminishing Returns Formula

```lua
-- Linear diminishing (reduces by fixed amount each time)
local function calc_diminishing_linear(base_value, times_completed, reduction_per_stack)
    local value = base_value - (times_completed * reduction_per_stack)
    return math.max(value, 0)  -- Never negative
end

-- Exponential diminishing (halves each time)
local function calc_diminishing_exponential(base_value, times_completed)
    return base_value / (2 ^ times_completed)
end

-- Example: +2 STR base, halving each time
-- 1st: +2, 2nd: +1, 3rd: +0.5, 4th: +0.25...
-- With cap at 5 completions: total = +3.875 STR (rounded to +4)
```

#### Lua API for Stackable Perks

```lua
-- Grant stackable perk with tracking
-- Returns: actual_gained, new_total, times_completed, hit_cap
local gained, total, times, capped = eq.grant_stackable_perk(
    e.other,
    "stat",           -- perk_type
    "str",            -- perk_key
    2,                -- base_value per completion
    10,               -- max_value cap
    "linear",         -- diminishing type: "none", "linear", "exponential"
    0.2               -- diminishing factor (loses 0.2 per stack for linear)
)

if capped then
    e.self:Say("You have mastered this trial. There is nothing more to gain.")
else
    e.other:Message(15, string.format(
        "[Account Perk] +%d Strength (Total: +%d, %d completions)",
        gained, total, times
    ))
end

-- Check current perk status
local info = eq.get_stackable_perk_info(e.other, "stat", "str")
-- info.times_completed, info.current_value, info.max_value, info.at_cap
```

---

## Data Storage

### Option A: Account Data Buckets (Recommended)

Extend the existing `data_buckets` system to support account scope.

```sql
-- Add account_id column to data_buckets
ALTER TABLE `data_buckets` ADD COLUMN `account_id` INT UNSIGNED DEFAULT 0;

-- Account unlock example
INSERT INTO data_buckets (account_id, `key`, `value`)
VALUES (12345, 'unlock:sebilis_key', '1');

-- Account perk example
INSERT INTO data_buckets (account_id, `key`, `value`)
VALUES (12345, 'perk:neriak_gate', '1');
```

### Option B: Dedicated Tables

New tables specifically for account progression.

```sql
CREATE TABLE `account_unlocks` (
    `id` INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    `account_id` INT UNSIGNED NOT NULL,
    `unlock_key` VARCHAR(64) NOT NULL,
    `unlocked_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
    `unlocked_by_char_id` INT UNSIGNED DEFAULT 0,
    UNIQUE KEY `account_unlock` (`account_id`, `unlock_key`),
    KEY `account_id` (`account_id`)
);

CREATE TABLE `account_perks` (
    `id` INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    `account_id` INT UNSIGNED NOT NULL,
    `perk_type` VARCHAR(32) NOT NULL,  -- 'stat', 'aa', 'teleport', 'faction'
    `perk_key` VARCHAR(64) NOT NULL,
    `perk_value` INT NOT NULL DEFAULT 0,
    `source_description` VARCHAR(128),
    UNIQUE KEY `account_perk` (`account_id`, `perk_type`, `perk_key`),
    KEY `account_id` (`account_id`)
);

CREATE TABLE `account_progress_gates` (
    `id` INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    `gate_key` VARCHAR(64) NOT NULL,
    `gate_type` ENUM('level_cap', 'zone_lock', 'expansion', 'feature') NOT NULL,
    `description` VARCHAR(256),
    `unlock_requirement` VARCHAR(256),  -- JSON or simple format
    `default_locked` TINYINT(1) DEFAULT 1,
    UNIQUE KEY `gate_key` (`gate_key`)
);
```

### Option C: Hybrid Approach

- Use `data_buckets` for simple flags
- Use dedicated tables for complex perks with metadata

---

## Implementation Components

### 1. Account Bucket API

Extend existing data_bucket system:

```cpp
// In Client or new AccountManager class
class AccountManager {
public:
    // Basic unlocks
    static bool HasAccountUnlock(uint32 account_id, const std::string& unlock_key);
    static void GrantAccountUnlock(uint32 account_id, const std::string& unlock_key, uint32 char_id = 0);

    // Perks
    static bool HasAccountPerk(uint32 account_id, const std::string& perk_key);
    static void GrantAccountPerk(uint32 account_id, const std::string& perk_type,
                                  const std::string& perk_key, int value);
    static std::vector<AccountPerk> GetAccountPerks(uint32 account_id);

    // Stackable perks
    static StackablePerkResult GrantStackablePerk(uint32 account_id, const std::string& perk_type,
                                                   const std::string& perk_key, int base_value,
                                                   int max_value, DiminishingType dim_type, float dim_factor);
    static StackablePerkInfo GetStackablePerkInfo(uint32 account_id, const std::string& perk_type,
                                                   const std::string& perk_key);

    // Veteran bonuses
    static uint8 GetHighestCharacterLevel(uint32 account_id);
    static uint32 GetTotalAccountAAs(uint32 account_id, uint32 exclude_char_id = 0);
    static float GetVeteranXPBonus(uint32 account_id, uint8 current_char_level);
    static float GetVeteranAABonus(uint32 account_id, uint32 current_char_id);

    // Inherited progress
    static bool HasMaxLevelCharacter(uint32 account_id);
    static uint8 GetHighestExpansionUnlocked(uint32 account_id);
};
```

### 2. Lua/Quest API

```lua
-- Check if account has unlock
if eq.has_account_unlock(e.other, "sebilis_key") then
    e.self:Say("I sense you've proven yourself before. Enter freely.")
end

-- Grant account unlock
eq.grant_account_unlock(e.other, "sebilis_key")
e.other:Message(15, "Account Unlock: Sebilis Key - All characters can now enter Sebilis!")

-- Check account perk
local str_bonus = eq.get_account_perk(e.other, "stat", "strength") or 0

-- Grant account perk
eq.grant_account_perk(e.other, "stat", "strength", 2)
eq.grant_account_perk(e.other, "teleport", "neriak", 1)
eq.grant_account_perk(e.other, "aa", "neriak_gate", 1)
```

### 3. Zone Entry Checks

```cpp
// In zone/client.cpp or similar
bool Client::CanEnterZone(uint32 zone_id) {
    // Check account-level zone locks
    if (IsZoneLocked(zone_id)) {
        std::string unlock_key = GetZoneUnlockKey(zone_id);
        if (!AccountManager::HasAccountUnlock(AccountID(), unlock_key)) {
            return false;
        }
    }
    return true;
}
```

### 4. Level Cap System

```cpp
uint8 Client::GetAccountLevelCap() {
    uint8 cap = RuleI(Character, BaseMaxLevel);  // e.g., 50

    // Check account unlocks that raise cap
    if (AccountManager::HasAccountUnlock(AccountID(), "killed_nagafen_vox")) {
        cap = std::max(cap, (uint8)60);
    }
    if (AccountManager::HasAccountUnlock(AccountID(), "killed_kunark_bosses")) {
        cap = std::max(cap, (uint8)60);
    }
    // etc.

    return cap;
}
```

### 5. Character Login/Creation Perk Application

```cpp
void Client::ApplyAccountPerks() {
    auto perks = AccountManager::GetAccountPerks(AccountID());

    for (const auto& perk : perks) {
        if (perk.type == "stat") {
            // Apply stat bonus
            ApplyStatBonus(perk.key, perk.value);
        }
        else if (perk.type == "aa") {
            // Grant AA if not already known
            GrantAAIfNotKnown(perk.key, perk.value);
        }
        else if (perk.type == "item") {
            // Grant item key to inventory (like Seb key)
            GrantKeyItem(perk.key);
        }
    }
}
```

### 6. Veteran XP Bonus System

```cpp
// In zone/exp.cpp or client experience methods
float Client::GetVeteranXPMultiplier() {
    uint8 highest_level = AccountManager::GetHighestCharacterLevel(AccountID());
    uint8 my_level = GetLevel();

    if (highest_level <= my_level) {
        return 1.0f;  // No bonus if this is the highest char
    }

    int level_diff = highest_level - my_level;

    // 2% bonus per level difference, capped at 100%
    float bonus = std::min(level_diff * 0.02f, 1.0f);

    return 1.0f + bonus;
}

// Apply in experience calculation
void Client::AddEXP(uint64 add_exp, ...) {
    // ... existing code ...

    float veteran_mult = GetVeteranXPMultiplier();
    add_exp = static_cast<uint64>(add_exp * veteran_mult);

    // ... rest of exp calculation ...
}
```

### 7. Veteran AA Bonus System

```cpp
// In zone/aa.cpp or AA experience methods
float Client::GetVeteranAAMultiplier() {
    uint32 total_other_aas = AccountManager::GetTotalAccountAAs(AccountID(), CharacterID());

    if (total_other_aas == 0) {
        return 1.0f;  // No bonus for first character
    }

    // 50% of total other AAs as a "veteran pool"
    // Translates to bonus AA gain rate
    // Example: 2000 AAs on other chars = 1000 veteran pool
    // Could give +50% AA rate until you've earned 1000 AAs yourself

    uint32 my_aas = GetSpentAA();  // How many AAs this char has earned
    uint32 veteran_pool = total_other_aas / 2;

    if (my_aas >= veteran_pool) {
        return 1.0f;  // Caught up, no more bonus
    }

    // Scale bonus based on how far behind
    // More behind = bigger bonus
    float catchup_ratio = 1.0f - ((float)my_aas / (float)veteran_pool);
    float bonus = catchup_ratio * 0.5f;  // Max +50% AA rate

    return 1.0f + bonus;
}
```

### 8. Inherited Progress / Zone Access

```cpp
// Zone entry with inherited progress
bool Client::CanEnterZone(uint32 zone_id) {
    // If account has a max-level character, all zones are unlocked
    if (AccountManager::HasMaxLevelCharacter(AccountID())) {
        return true;
    }

    // Check expansion-based unlocks
    uint8 zone_expansion = GetZoneExpansion(zone_id);
    uint8 account_expansion = AccountManager::GetHighestExpansionUnlocked(AccountID());

    if (zone_expansion <= account_expansion) {
        return true;  // Account has unlocked this expansion
    }

    // Check specific zone keys
    if (IsZoneLocked(zone_id)) {
        std::string unlock_key = GetZoneUnlockKey(zone_id);
        if (!AccountManager::HasAccountUnlock(AccountID(), unlock_key)) {
            return false;
        }
    }

    return true;
}
```

---

## Example Configurations

### Classic Era Progression

```lua
-- Gate definitions (could be in database or config)
local GATES = {
    classic = {
        level_cap = 50,
        zones = {"soldungb", "permafrost", "nagafen", "vox"},
        unlock = "classic_complete"
    },
    kunark = {
        level_cap = 60,
        zones = {"sebilis", "chardok", "veeshan"},
        requires = "classic_complete",
        unlock = "kunark_complete"
    }
}
```

### Zone Key Unlocks

```sql
-- Account unlock definitions
INSERT INTO account_progress_gates VALUES
(1, 'sebilis_key', 'zone_lock', 'Key to Sebilis', 'Complete Trakanon Idol quest', 1),
(2, 'veeshan_key', 'zone_lock', 'Veeshan Peak Key', 'Complete medallion quest', 1),
(3, 'classic_complete', 'expansion', 'Classic Content Complete', 'Kill Nagafen AND Lady Vox', 1),
(4, 'lower_guk_access', 'zone_lock', 'Lower Guk Access', 'Complete Upper Guk intro quest', 1);
```

### Account Perk Rewards

```sql
-- Perk definitions (what's available)
-- Actual grants stored in account_perks table

-- Example: Neriak quest chain rewards
-- Quest completion grants:
--   eq.grant_account_perk(client, "teleport", "neriak", 1)
--   eq.grant_account_perk(client, "faction_fix", "neriak", 1)

-- Example: Oggok heritage quest
-- Quest completion grants:
--   eq.grant_account_perk(client, "stat", "str", 2)
```

---

## Quest Integration Examples

### Sebilis Key Quest (Account Unlock)

```lua
-- In quests/trakanon/Trakanon_Idol_Giver.lua
function event_trade(e)
    local item_lib = require("items")

    if item_lib.check_turn_in(e.trade, {item1 = IDOL_ITEM_ID}) then
        -- Check if account already has unlock
        if eq.has_account_unlock(e.other, "sebilis_key") then
            e.self:Say("Your legacy already grants you access to Sebilis.")
            e.other:SummonItem(SEBILIS_KEY_ITEM)  -- Still give the key item
        else
            e.self:Say("You have proven yourself worthy! The doors of Sebilis shall open for you and all who share your bloodline!")
            eq.grant_account_unlock(e.other, "sebilis_key")
            e.other:SummonItem(SEBILIS_KEY_ITEM)
            e.other:Message(15, "[Account Unlock] Sebilis Key - All your characters can now enter Sebilis!")
        end
    else
        item_lib.return_items(e.self, e.other, e.trade)
    end
end
```

### Nagafen/Vox Kill (Level Cap Unlock)

```lua
-- In quests/soldungb/Lord_Nagafen.lua
function event_death_complete(e)
    local killer = e.other
    if not killer or not killer:IsClient() then return end

    killer = killer:CastToClient()

    -- Grant individual kill flag
    eq.grant_account_unlock(killer, "killed_nagafen")

    -- Check if both dragons killed
    if eq.has_account_unlock(killer, "killed_nagafen") and
       eq.has_account_unlock(killer, "killed_vox") then
        eq.grant_account_unlock(killer, "classic_complete")
        killer:Message(15, "[Account Unlock] Classic Complete - Level cap raised to 60!")
        killer:Message(15, "[Account Unlock] Kunark zones are now accessible!")
    else
        killer:Message(15, "[Progress] Nagafen slain! Defeat Lady Vox to complete Classic content.")
    end
end
```

### Heritage Quest (Account Perk)

```lua
-- In quests/neriak/Grandmaster_Quest_Giver.lua
function event_trade(e)
    local item_lib = require("items")

    if item_lib.check_turn_in(e.trade, {item1 = FINAL_QUEST_ITEM}) then
        e.self:Say("You have proven your devotion to Neriak! The Dark Prince blesses your bloodline!")

        -- Grant account perks
        eq.grant_account_perk(e.other, "teleport", "neriak", 1)
        eq.grant_account_perk(e.other, "faction_fix", "neriak", 1)

        -- Also grant an AA
        eq.grant_account_perk(e.other, "aa", "gate_neriak", 1)

        e.other:Message(15, "[Account Perk] Neriak Gate - All characters gain an AA to teleport to Neriak!")
        e.other:Message(15, "[Account Perk] Dark Blessing - All characters can reset Neriak faction!")
    else
        item_lib.return_items(e.self, e.other, e.trade)
    end
end
```

### Stat Bonus Quest (Simple)

```lua
-- In quests/oggok/Ogre_Chieftain.lua
function event_trade(e)
    local item_lib = require("items")

    if item_lib.check_turn_in(e.trade, {item1 = OGRE_HERITAGE_TOKEN}) then
        e.self:Say("You strong like ogre! Ogre ancestors bless your family!")

        -- Grant +2 STR to all characters
        eq.grant_account_perk(e.other, "stat", "str", 2)

        e.other:Message(15, "[Account Perk] Ogre Blessing - All characters gain +2 Strength!")

        -- Apply immediately to current character
        e.other:SetStats()  -- Recalculate stats
    else
        item_lib.return_items(e.self, e.other, e.trade)
    end
end
```

### Stackable Stat Bonus Quest (Repeatable with Diminishing Returns)

```lua
-- In quests/gukbottom/Froglok_Elder.lua
-- This quest can be done multiple times for stacking bonuses
function event_trade(e)
    local item_lib = require("items")

    if item_lib.check_turn_in(e.trade, {item1 = FROGLOK_RELIC}) then
        -- Grant stackable perk: +2 WIS base, max +10 WIS, linear diminishing
        local gained, total, times, capped = eq.grant_stackable_perk(
            e.other,
            "stat",       -- perk_type
            "wis",        -- perk_key
            2,            -- base_value per completion
            10,           -- max_value cap
            "linear",     -- diminishing: "none", "linear", "exponential"
            0.25          -- loses 0.25 per stack (so: +2, +1.75, +1.5, +1.25, +1, +0.75...)
        )

        if capped then
            e.self:Say("The spirits have blessed you fully. You have reached enlightenment!")
            e.other:Message(15, "[Account Perk] Froglok Wisdom - MAXED at +" .. total .. " Wisdom!")
        else
            e.self:Say("The spirits bless your mind! Return with more relics for greater wisdom.")
            e.other:Message(15, string.format(
                "[Account Perk] Froglok Wisdom - +%.1f Wisdom (Total: +%d, %d completions)",
                gained, total, times
            ))
        end

        e.other:SetStats()  -- Recalculate stats
    else
        item_lib.return_items(e.self, e.other, e.trade)
    end
end
```

### Stackable Perk Progress Check

```lua
-- Let players check their progress on a stackable perk
function event_say(e)
    if e.message:findi("progress") then
        local info = eq.get_stackable_perk_info(e.other, "stat", "wis")

        if info.times_completed == 0 then
            e.self:Say("You have not yet begun the trials of wisdom.")
        elseif info.at_cap then
            e.self:Say(string.format(
                "You have completed the trials %d times and reached maximum wisdom (+%d)!",
                info.times_completed, info.current_value
            ))
        else
            local next_gain = eq.calc_next_stackable_gain(e.other, "stat", "wis", 2, "linear", 0.25)
            e.self:Say(string.format(
                "You have completed %d trials for +%d Wisdom. The next trial grants +%.1f more.",
                info.times_completed, info.current_value, next_gain
            ))
        end
    end
end
```

---

## UI/Client Considerations

### Displaying Account Progress

Options for showing players their account unlocks:

1. **Custom Window** - Use popup windows to show account status
2. **#account Command** - Show unlocks/perks via command
3. **Login Message** - Show relevant unlocks at character login
4. **DLL Enhancement** - Custom UI panel (advanced)

### Example: #account Command

```lua
-- In quests/global/global_player.lua or command handler
function handle_account_command(client)
    client:Message(15, "=== Account Progress ===")

    -- Show unlocks
    client:Message(15, "Unlocks:")
    if eq.has_account_unlock(client, "classic_complete") then
        client:Message(15, "  [X] Classic Complete (Level cap: 60)")
    else
        client:Message(15, "  [ ] Classic Complete (Kill Nagafen + Vox)")
    end

    -- Show zone keys
    client:Message(15, "Zone Keys:")
    local keys = {"sebilis_key", "veeshan_key", "charasis_key"}
    for _, key in ipairs(keys) do
        local status = eq.has_account_unlock(client, key) and "[X]" or "[ ]"
        client:Message(15, string.format("  %s %s", status, key))
    end

    -- Show perks
    client:Message(15, "Perks:")
    local str_bonus = eq.get_account_perk(client, "stat", "str") or 0
    if str_bonus > 0 then
        client:Message(15, string.format("  +%d Strength", str_bonus))
    end

    -- Show veteran bonuses
    local xp_bonus = eq.get_veteran_xp_bonus(client)
    local aa_bonus = eq.get_veteran_aa_bonus(client)
    client:Message(15, "Veteran Bonuses:")
    client:Message(15, string.format("  XP Bonus: +%.0f%%", (xp_bonus - 1) * 100))
    client:Message(15, string.format("  AA Bonus: +%.0f%%", (aa_bonus - 1) * 100))
end
```

---

## Implementation Phases

### Phase 1: Foundation
- [ ] Add account scope to data_buckets (or create new tables)
- [ ] Create AccountManager class with basic API
- [ ] Add Lua bindings for account unlocks/perks

### Phase 2: Zone Gating
- [ ] Implement zone entry checks
- [ ] Add zone unlock configuration
- [ ] Create key quest conversions (Seb, VP, etc.)
- [ ] Add inherited progress (max level = all zones)

### Phase 3: Level Cap System
- [ ] Implement dynamic level cap based on account progress
- [ ] Add classic->kunark->velious progression gates
- [ ] Handle edge cases (mentoring, shrouds, etc.)

### Phase 4: Account Perks
- [ ] Implement stat bonus system
- [ ] Add AA grant system
- [ ] Create teleport/utility perks
- [ ] Apply perks at login
- [ ] Implement stackable perks with tracking

### Phase 5: Veteran Bonuses
- [ ] Implement XP bonus based on highest level character
- [ ] Implement AA bonus based on total account AAs
- [ ] Add veteran bonus display to UI/commands

### Phase 6: Polish
- [ ] Add #account command or UI
- [ ] Create achievement-style tracking
- [ ] Add legacy unlock system for existing accounts

---

## Design Decisions

### Resolved Questions

1. **Inherited Progress**: ✅ Yes - max level characters unlock all zones for alts
2. **Perk Stacking**: ✅ Configurable per perk - track completion count, support caps and diminishing returns
3. **Veteran Bonuses**: ✅ Yes - XP bonus from highest level, AA bonus from total account AAs

### Open Questions

1. **Retroactive Unlocks**: Should existing characters who've done content get credit?
2. **Transfer/Merge**: How do account unlocks work with character transfers?
3. **Group Credit**: Should group members get unlock credit, or only the quest completer?
4. **Alt Detection**: Should very old accounts get grandfather unlocks?
5. **Diminishing Returns Tuning**: What's the right falloff rate for stackable perks?

- [Quest Systems](../quests/README.md) - How to implement unlock quests
- [Multiclass System](../multiclass/README.md) - Another account-level feature
- Data Buckets - Existing key/value storage system

---

## References

- WoW Account-Wide achievements/mounts
- FFXIV Cross-character unlocks
- GW2 Account-wide mastery system
- EQ2 Account features
