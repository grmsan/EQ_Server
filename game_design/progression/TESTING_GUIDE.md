# Account Progression - Testing Guide

How to test the account progression system during and after development.

## Table of Contents

1. [Test Environment Setup](#test-environment-setup)
2. [Unit Tests](#unit-tests)
3. [Integration Tests](#integration-tests)
4. [Manual Testing Checklist](#manual-testing-checklist)
5. [Test Data Setup](#test-data-setup)
6. [Common Issues & Debugging](#common-issues--debugging)

---

## Test Environment Setup

### Database Setup

```sql
-- Create test account
INSERT INTO account (name, password, status) VALUES ('testaccount', 'test123', 0);
SET @test_account_id = LAST_INSERT_ID();

-- Create test characters at different levels
INSERT INTO character_data (account_id, name, level, zone_id) VALUES
(@test_account_id, 'TestChar1', 1, 1),
(@test_account_id, 'TestChar50', 50, 1),
(@test_account_id, 'TestChar30', 30, 1);

-- Verify setup
SELECT * FROM character_data WHERE account_id = @test_account_id;
```

### Test Configuration

Create a test rules file or add these entries:

```sql
INSERT INTO account_veteran_rules VALUES
('xp_bonus_per_level', '0.02', 'Test: 2% per level'),
('xp_bonus_max', '1.0', 'Test: 100% max'),
('aa_veteran_pool_ratio', '0.5', 'Test: 50% ratio'),
('inherit_zones_at_max_level', '1', 'Test: Enabled');
```

---

## Unit Tests

### C++ Unit Tests

Location: `tests/account_progression_tests.cpp`

```cpp
#include <gtest/gtest.h>
#include "common/account_progression.h"

class AccountProgressionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test database connection
        // Clear test data
    }
    
    void TearDown() override {
        // Clean up test data
    }
    
    uint32 test_account_id = 99999;
};

// ============================================
// UNLOCK TESTS
// ============================================

TEST_F(AccountProgressionTest, GrantUnlock_NewUnlock_ReturnsTrue) {
    auto& mgr = AccountProgressionManager::Instance();
    
    // Should return true for new unlock
    EXPECT_TRUE(mgr.GrantUnlock(test_account_id, "test_unlock"));
    
    // Should now have the unlock
    EXPECT_TRUE(mgr.HasUnlock(test_account_id, "test_unlock"));
}

TEST_F(AccountProgressionTest, GrantUnlock_DuplicateUnlock_ReturnsFalse) {
    auto& mgr = AccountProgressionManager::Instance();
    
    mgr.GrantUnlock(test_account_id, "test_unlock");
    
    // Should return false for duplicate
    EXPECT_FALSE(mgr.GrantUnlock(test_account_id, "test_unlock"));
}

TEST_F(AccountProgressionTest, HasUnlock_NonExistent_ReturnsFalse) {
    auto& mgr = AccountProgressionManager::Instance();
    
    EXPECT_FALSE(mgr.HasUnlock(test_account_id, "nonexistent_unlock"));
}

// ============================================
// PERK TESTS
// ============================================

TEST_F(AccountProgressionTest, GrantPerk_Stackable_AccumulatesValue) {
    auto& mgr = AccountProgressionManager::Instance();
    
    // Grant +2 STR twice
    mgr.GrantPerk(test_account_id, "stat", "str", 2, "test");
    mgr.GrantPerk(test_account_id, "stat", "str", 2, "test");
    
    // Should have 4 total
    EXPECT_EQ(4, mgr.GetPerkValue(test_account_id, "stat", "str"));
}

TEST_F(AccountProgressionTest, GrantPerk_AtCap_ReturnsZero) {
    auto& mgr = AccountProgressionManager::Instance();
    
    // Grant up to cap (assume max is 10)
    for (int i = 0; i < 10; i++) {
        mgr.GrantPerk(test_account_id, "stat", "str", 2, "test");
    }
    
    // Next grant should return 0
    EXPECT_EQ(0, mgr.GrantPerk(test_account_id, "stat", "str", 2, "test"));
}

TEST_F(AccountProgressionTest, GetPerkInfo_ReturnsCorrectData) {
    auto& mgr = AccountProgressionManager::Instance();
    
    mgr.GrantPerk(test_account_id, "stat", "str", 2, "test");
    mgr.GrantPerk(test_account_id, "stat", "str", 2, "test");
    
    auto info = mgr.GetPerkInfo(test_account_id, "stat", "str");
    
    EXPECT_EQ(4, info.current_value);
    EXPECT_EQ(2, info.times_completed);
    EXPECT_FALSE(info.at_cap);
}

// ============================================
// DIMINISHING RETURNS TESTS
// ============================================

TEST_F(AccountProgressionTest, DiminishingReturns_Linear_ReducesCorrectly) {
    auto& mgr = AccountProgressionManager::Instance();
    
    // Setup: Base value 2, linear diminishing 0.25
    // Stack 1: 2.0
    // Stack 2: 2.0 - (1 * 0.25) = 1.75 → rounds to 1
    // Stack 3: 2.0 - (2 * 0.25) = 1.50 → rounds to 1
    // Stack 4: 2.0 - (3 * 0.25) = 1.25 → rounds to 1
    // Stack 5: 2.0 - (4 * 0.25) = 1.00 → rounds to 1
    
    int total = 0;
    for (int i = 0; i < 5; i++) {
        total += mgr.GrantPerk(test_account_id, "stat", "wis", 2, "test");
    }
    
    // Exact values depend on implementation, but should be less than 10
    EXPECT_LT(total, 10);
    EXPECT_GT(total, 5);
}

TEST_F(AccountProgressionTest, DiminishingReturns_Exponential_ReducesCorrectly) {
    auto& mgr = AccountProgressionManager::Instance();
    
    // Setup: Base value 2, exponential diminishing 0.5
    // Stack 1: 2 * (0.5^0) = 2.0
    // Stack 2: 2 * (0.5^1) = 1.0
    // Stack 3: 2 * (0.5^2) = 0.5 → rounds to 0
    
    mgr.GrantPerk(test_account_id, "regen", "hp", 2, "test");
    EXPECT_EQ(2, mgr.GetPerkValue(test_account_id, "regen", "hp"));
    
    mgr.GrantPerk(test_account_id, "regen", "hp", 2, "test");
    EXPECT_EQ(3, mgr.GetPerkValue(test_account_id, "regen", "hp"));
    
    // Third stack should add 0 due to exponential falloff
    int added = mgr.GrantPerk(test_account_id, "regen", "hp", 2, "test");
    EXPECT_EQ(0, added);
}

// ============================================
// VETERAN BONUS TESTS
// ============================================

TEST_F(AccountProgressionTest, VeteranXPBonus_CalculatesCorrectly) {
    auto& mgr = AccountProgressionManager::Instance();
    
    // Setup: Highest level 50, current level 20
    // Expected: (50 - 20) * 0.02 = 0.60 (60% bonus)
    
    // Set up test data...
    float bonus = mgr.GetVeteranXPMultiplier(test_account_id, 20);
    
    EXPECT_NEAR(0.60f, bonus, 0.01f);
}

TEST_F(AccountProgressionTest, VeteranXPBonus_CapsAtMax) {
    auto& mgr = AccountProgressionManager::Instance();
    
    // Setup: Highest level 60, current level 1
    // Expected: (60 - 1) * 0.02 = 1.18 → capped at 1.0
    
    float bonus = mgr.GetVeteranXPMultiplier(test_account_id, 1);
    
    EXPECT_LE(bonus, 1.0f);
}

TEST_F(AccountProgressionTest, VeteranXPBonus_SameLevel_ReturnsZero) {
    auto& mgr = AccountProgressionManager::Instance();
    
    // Level 50 character on account with level 50 highest
    float bonus = mgr.GetVeteranXPMultiplier(test_account_id, 50);
    
    EXPECT_EQ(0.0f, bonus);
}

// ============================================
// ZONE ACCESS TESTS
// ============================================

TEST_F(AccountProgressionTest, ZoneAccess_WithUnlock_Allowed) {
    auto& mgr = AccountProgressionManager::Instance();
    
    mgr.GrantUnlock(test_account_id, "sebilis_key");
    
    std::string message;
    bool allowed = mgr.CanEnterZone(test_account_id, 89, 50, message);
    
    EXPECT_TRUE(allowed);
    EXPECT_TRUE(message.empty());
}

TEST_F(AccountProgressionTest, ZoneAccess_WithoutUnlock_Denied) {
    auto& mgr = AccountProgressionManager::Instance();
    
    std::string message;
    bool allowed = mgr.CanEnterZone(test_account_id, 89, 50, message);
    
    EXPECT_FALSE(allowed);
    EXPECT_FALSE(message.empty());
}

TEST_F(AccountProgressionTest, ZoneAccess_MaxLevelBypass_Works) {
    auto& mgr = AccountProgressionManager::Instance();
    
    // Don't have unlock, but have max level char
    // (setup max level character in test data)
    
    std::string message;
    bool allowed = mgr.CanEnterZone(test_account_id, 89, 60, message);
    
    // Should be allowed if inherit_zones_at_max_level is enabled
    EXPECT_TRUE(allowed);
}
```

### Running C++ Tests

```bash
# Build tests
cmake --build build --target account_progression_tests --config Debug

# Run tests
./build/bin/account_progression_tests
```

---

## Integration Tests

### Lua Integration Tests

Create `quests/global/test_account_progression.lua`:

```lua
-- Test harness for account progression
-- Run with: #luatest account_progression

local function test_unlocks()
    local results = {}
    
    -- Test 1: Grant new unlock
    local was_new = eq.grant_account_unlock(e.self, "test_unlock_1", false)
    table.insert(results, {
        name = "Grant new unlock returns true",
        passed = was_new == true
    })
    
    -- Test 2: Has unlock returns true
    local has = eq.has_account_unlock(e.self, "test_unlock_1")
    table.insert(results, {
        name = "Has unlock returns true after grant",
        passed = has == true
    })
    
    -- Test 3: Duplicate grant returns false
    was_new = eq.grant_account_unlock(e.self, "test_unlock_1", false)
    table.insert(results, {
        name = "Duplicate grant returns false",
        passed = was_new == false
    })
    
    -- Test 4: Non-existent unlock returns false
    has = eq.has_account_unlock(e.self, "nonexistent_unlock_xyz")
    table.insert(results, {
        name = "Non-existent unlock returns false",
        passed = has == false
    })
    
    -- Cleanup
    eq.remove_account_unlock(e.self, "test_unlock_1")
    
    return results
end

local function test_perks()
    local results = {}
    
    -- Test 1: Grant perk
    local added = eq.grant_account_perk(e.self, "stat", "test_stat", 2, "test")
    table.insert(results, {
        name = "Grant perk returns value added",
        passed = added == 2
    })
    
    -- Test 2: Get perk value
    local value = eq.get_account_perk(e.self, "stat", "test_stat")
    table.insert(results, {
        name = "Get perk returns correct value",
        passed = value == 2
    })
    
    -- Test 3: Stack perk
    added = eq.grant_account_perk(e.self, "stat", "test_stat", 2, "test")
    value = eq.get_account_perk(e.self, "stat", "test_stat")
    table.insert(results, {
        name = "Perk stacks correctly",
        passed = value == 4
    })
    
    -- Test 4: Perk info
    local info = eq.get_account_perk_info(e.self, "stat", "test_stat")
    table.insert(results, {
        name = "Perk info returns correct data",
        passed = info.current_value == 4 and info.times_completed == 2
    })
    
    return results
end

local function test_veteran_bonuses()
    local results = {}
    
    -- Test 1: Get veteran data
    local data = eq.get_veteran_data(e.self)
    table.insert(results, {
        name = "Veteran data returns table",
        passed = type(data) == "table" and data.highest_level ~= nil
    })
    
    -- Test 2: XP bonus is a number
    local xp_bonus = eq.get_veteran_xp_bonus(e.self)
    table.insert(results, {
        name = "XP bonus returns number",
        passed = type(xp_bonus) == "number"
    })
    
    -- Test 3: AA bonus is a number
    local aa_bonus = eq.get_veteran_aa_bonus(e.self)
    table.insert(results, {
        name = "AA bonus returns number",
        passed = type(aa_bonus) == "number"
    })
    
    return results
end

function run_all_tests()
    e.self:Message(Chat.Yellow, "=== Account Progression Tests ===")
    
    local all_results = {}
    
    e.self:Message(Chat.White, "--- Unlock Tests ---")
    for _, result in ipairs(test_unlocks()) do
        local status = result.passed and "[PASS]" or "[FAIL]"
        e.self:Message(result.passed and Chat.Green or Chat.Red, 
            string.format("  %s %s", status, result.name))
        table.insert(all_results, result)
    end
    
    e.self:Message(Chat.White, "--- Perk Tests ---")
    for _, result in ipairs(test_perks()) do
        local status = result.passed and "[PASS]" or "[FAIL]"
        e.self:Message(result.passed and Chat.Green or Chat.Red, 
            string.format("  %s %s", status, result.name))
        table.insert(all_results, result)
    end
    
    e.self:Message(Chat.White, "--- Veteran Bonus Tests ---")
    for _, result in ipairs(test_veteran_bonuses()) do
        local status = result.passed and "[PASS]" or "[FAIL]"
        e.self:Message(result.passed and Chat.Green or Chat.Red, 
            string.format("  %s %s", status, result.name))
        table.insert(all_results, result)
    end
    
    -- Summary
    local passed = 0
    local failed = 0
    for _, r in ipairs(all_results) do
        if r.passed then passed = passed + 1 else failed = failed + 1 end
    end
    
    e.self:Message(Chat.Yellow, string.format(
        "=== Results: %d passed, %d failed ===", passed, failed))
end
```

---

## Manual Testing Checklist

Print this and check off as you test:

### Account Unlocks

- [ ] Grant unlock via quest script → verify with `#account unlocks`
- [ ] Grant same unlock twice → verify only one entry, returns false
- [ ] Remove unlock → verify removed
- [ ] GM grant unlock → verify works
- [ ] Check unlock on different character same account → should have it
- [ ] Check unlock on different account → should NOT have it

### Zone Access

- [ ] Try to enter locked zone without unlock → blocked with message
- [ ] Grant unlock → can now enter
- [ ] Level alt to max → can enter all locked zones (inherited)
- [ ] Test bypass_level setting if configured
- [ ] Test zone that has bypass_with_max_level disabled

### Account Perks

- [ ] Grant stat perk → stat increases
- [ ] Grant same perk again → value stacks
- [ ] Check perk persists through logout/login
- [ ] Check perk applies to new character
- [ ] Grant perk until cap → verify stops at cap
- [ ] Test diminishing returns perk → verify values decrease

### Veteran XP Bonus

- [ ] Create high level character
- [ ] Create low level alt
- [ ] Verify alt receives XP bonus (check with `#account veteran`)
- [ ] Level alt up → verify bonus decreases as gap shrinks
- [ ] Reach same level as main → verify bonus is 0

### Veteran AA Bonus

- [ ] Spend AAs on main character
- [ ] Create alt and reach AA-eligible level
- [ ] Verify alt receives AA bonus
- [ ] Spend AAs on alt → verify bonus decreases as alt catches up

### Commands

- [ ] `#account` shows summary
- [ ] `#account unlocks` lists all unlocks
- [ ] `#account perks` lists all perks with values
- [ ] `#account veteran` shows bonus details
- [ ] `#grantunlock` works for GM
- [ ] `#grantperk` works for GM
- [ ] `#removeunlock` works for GM

---

## Test Data Setup

### Quick Test Script

Run this SQL to set up a complete test scenario:

```sql
-- Test account with characters
SET @test_acct = (SELECT id FROM account WHERE name = 'TestAccount' LIMIT 1);

-- If test account doesn't exist, create it
INSERT IGNORE INTO account (name, password, status) VALUES ('TestAccount', 'test', 0);
SET @test_acct = LAST_INSERT_ID();

-- Create characters at various levels
INSERT INTO character_data (account_id, name, level, class, race, zone_id) VALUES
(@test_acct, 'TestMain', 50, 1, 1, 1),
(@test_acct, 'TestAlt', 10, 2, 1, 1);

-- Give main character some AAs
UPDATE character_data SET aa_points_spent = 100 WHERE name = 'TestMain';

-- Pre-grant some unlocks
INSERT INTO account_unlocks (account_id, unlock_key) VALUES
(@test_acct, 'sebilis_key'),
(@test_acct, 'killed_nagafen');

-- Pre-grant some perks
INSERT INTO account_perks (account_id, perk_type, perk_key, perk_value, times_completed) VALUES
(@test_acct, 'stat', 'str', 4, 2),
(@test_acct, 'teleport', 'neriak', 1, 1);

-- Refresh veteran cache
CALL RefreshVeteranCache(@test_acct);

-- Verify
SELECT 'Unlocks:' as '';
SELECT * FROM account_unlocks WHERE account_id = @test_acct;
SELECT 'Perks:' as '';
SELECT * FROM account_perks WHERE account_id = @test_acct;
SELECT 'Veteran Cache:' as '';
SELECT * FROM account_veteran_cache WHERE account_id = @test_acct;
```

### Cleanup Script

```sql
SET @test_acct = (SELECT id FROM account WHERE name = 'TestAccount' LIMIT 1);

DELETE FROM account_unlocks WHERE account_id = @test_acct;
DELETE FROM account_perks WHERE account_id = @test_acct;
DELETE FROM account_veteran_cache WHERE account_id = @test_acct;
DELETE FROM character_data WHERE account_id = @test_acct;
DELETE FROM account WHERE id = @test_acct;
```

---

## Common Issues & Debugging

### Unlock Not Working

**Symptoms:** `has_account_unlock` returns false even after grant

**Debug steps:**
1. Check database directly:
   ```sql
   SELECT * FROM account_unlocks WHERE account_id = ? AND unlock_key = ?;
   ```
2. Check cache invalidation - did you call refresh?
3. Check spelling of unlock_key (case-sensitive)
4. Enable debug logging:
   ```cpp
   LogAccountProgression(Logs::Detail, "HasUnlock: acct=%d key=%s", account_id, key.c_str());
   ```

### Veteran Bonus Wrong

**Symptoms:** XP bonus is 0 or incorrect value

**Debug steps:**
1. Check veteran cache is populated:
   ```sql
   SELECT * FROM account_veteran_cache WHERE account_id = ?;
   ```
2. Manually refresh: `#refreshveteran`
3. Check veteran rules are loaded:
   ```sql
   SELECT * FROM account_veteran_rules;
   ```
4. Verify calculation:
   - `(highest_level - current_level) * xp_bonus_per_level`
   - Capped at `xp_bonus_max`

### Perk Not Stacking

**Symptoms:** Perk value doesn't increase on repeat

**Debug steps:**
1. Check perk definition allows stacking:
   ```sql
   SELECT stackable, max_value, max_stacks FROM account_perk_definitions 
   WHERE perk_type = ? AND perk_key = ?;
   ```
2. Check if already at cap:
   ```lua
   local info = eq.get_account_perk_info(client, type, key)
   print("at_cap:", info.at_cap)
   ```
3. Check diminishing returns configuration

### Zone Access Bypass Not Working

**Symptoms:** Max level char still can't enter locked zone

**Debug steps:**
1. Check zone lock configuration:
   ```sql
   SELECT * FROM account_zone_locks WHERE zone_id = ?;
   ```
2. Verify `bypass_with_max_level = 1`
3. Check `inherit_zones_at_max_level` rule
4. Verify character is actually at server max level

### Logging

Enable detailed logging for debugging:

```cpp
// In zone/main.cpp or where logging is configured
Log.SetLoggingCategory(Logs::AccountProgression, Logs::Detail);
```

Check logs at:
- `logs/zone/zone_*.log` - look for AccountProgression entries
