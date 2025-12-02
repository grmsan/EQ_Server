# Testing Dynamic Item Persistence

## What Was Fixed
Dynamic items (infinite progression items) now persist correctly across:
- ✅ Zone changes
- ✅ Logout/login
- ✅ Server restarts
- ✅ Shared bank storage

## Test Plan

### Test 1: Basic Item Persistence
1. **Get a base item**: `#summonitem 1001` (Cloth Cap)
2. **Upgrade it**: `#upgrade` (should become Cloth Cap +1 with ID 101001001)
3. **Verify stats**: Check that stats are improved
4. **Zone out**: Go to a different zone
5. **Zone back**: Return to original zone
6. **Check item**: Should still be Cloth Cap +1 with improved stats

### Test 2: Logout/Login Persistence
1. **Get and upgrade an item** (as above)
2. **Camp to character select**
3. **Log back in**
4. **Check inventory**: Item should still be upgraded

### Test 3: Multiple Upgrades
1. **Start with Cloth Cap**: `#summonitem 1001`
2. **Upgrade to +1**: `#upgrade` → ID should be 101001001
3. **Upgrade to +2**: `#upgrade` → ID should be 102001001
4. **Upgrade to +3**: `#upgrade` → ID should be 103001001
5. **Zone and relog**: All upgrades should persist
6. **Verify each**: Each level should have progressively better stats

### Test 4: Shared Bank Persistence
1. **Upgrade an item**
2. **Put it in shared bank** (if accessible)
3. **Logout and login with different character**
4. **Check shared bank**: Item should be there with correct level

### Test 5: Database Verification
Run this SQL query to verify items are stored correctly:
```sql
SELECT
    character_id,
    slot_id,
    item_id,
    custom_data,
    charges
FROM inventory
WHERE character_id = <YOUR_CHAR_ID>
ORDER BY slot_id;
```

**What to look for**:
- `item_id` should be the **base item ID** (e.g., 1001 for Cloth Cap)
- `custom_data` should contain JSON like: `{"dynamic_level":1}`

Example correct data:
```
character_id: 1
slot_id: 0 (head slot)
item_id: 1001
custom_data: {"dynamic_level":3}
charges: 1
```
This represents: Cloth Cap +3 (ID 103001001) in the head slot

### Test 6: Item in Bags
1. **Put items in bags** (General inventory bags)
2. **Upgrade items while in bags**
3. **Zone/relog**
4. **Verify**: Items in bags should persist

### Test 7: Equipped Items
1. **Equip an item** (e.g., head slot)
2. **Upgrade it while equipped**
3. **Zone/relog**
4. **Verify**: Equipped upgraded items persist

## Expected Behavior

### Before Fix
- Items would disappear on zone/logout
- Database would have no entry for the item
- Error in logs: "Warning: charid [X] has an invalid item_id [101001001]"

### After Fix
- Items persist correctly
- Database stores base_id with level in custom_data
- No errors in logs
- Items load with correct dynamic ID

## Debugging

### If Items Disappear
1. **Check logs**: `logs/zone_*.log` and `logs/world.log`
2. **Look for errors** containing "invalid item_id"
3. **Check database**:
   ```sql
   SELECT * FROM inventory WHERE character_id = <ID>;
   ```
4. **Verify custom_data** has dynamic_level field

### Check Item ID Format
Dynamic item IDs follow this pattern: **LLLLIIIIII**
- First 4 digits: Level + 100 (e.g., 0101 = level 1)
- Last 6 digits: Base item ID (e.g., 001001 = item 1001)

Examples:
- `101001001` = Cloth Cap +1 (level 1)
- `102001001` = Cloth Cap +2 (level 2)
- `110042000` = Some Item +10 (level 10, base ID 42000)

## Common Commands
- `#summonitem <id>` - Get base item
- `#upgrade` - Upgrade item in cursor
- `#finditem <name>` - Find item by name (shows ID)
- `#iteminfo` - Get info about item in cursor
- `#zone <zone>` - Change zones
- `#gm 1` - Enable GM mode (if needed)

## Success Criteria
✅ Items with dynamic IDs persist through zone changes
✅ Items with dynamic IDs persist through logout/login
✅ Database stores base_id + level correctly
✅ Multiple upgrade levels work correctly
✅ Shared bank items persist
✅ Items in bags persist
✅ Equipped items persist
