# Dynamic Item Persistence Implementation

## Problem
Dynamic items (infinite progression items with IDs like 101001001) were disappearing when players logged out or changed zones. The root cause was in the database save/load logic in `common/shareddb.cpp`.

### Root Cause
1. Dynamic item IDs (e.g., 101001001 for Cloth Cap +1) don't exist in the `items` table
2. `SaveInventory()` called `GetItem(101001001)` which returned `nullptr`
3. The code checked `if (!inst->GetItem())` and returned false, silently discarding the item
4. Items were never written to the `inventory` or `sharedbank` tables

## Solution
Modified `common/shareddb.cpp` to store dynamic items using their base item ID and level separately:

### Save Logic (Lines 262-329, 349-417)
- **Detect dynamic items**: Check if `(item_id / 1000000) > 100`
- **Extract components**:
  - `base_id = item_id % 1000000` (the real item ID from items table)
  - `level = (item_id / 1000000) - 100` (the upgrade level)
- **Store in database**:
  - `item_id` column = `base_id` (links to items table)
  - `custom_data` column = JSON with `{"dynamic_level": level}`

### Load Logic (Lines 730-755, 634-651)
- **Check custom_data**: Parse JSON for `dynamic_level` field
- **Reconstruct dynamic ID**: `item_id = ((100 + level) * 1000000) + base_id`
- **Example**:
  - Database stores: `item_id=1001`, `custom_data={"dynamic_level":1}`
  - Loads as: `item_id=101001001` (Cloth Cap +1)

## Files Modified
- `common/shareddb.cpp`:
  - Added `#include "json/json.h"` and `#include <sstream>`
  - Modified `UpdateInventorySlot()` - Regular inventory save
  - Modified `UpdateSharedBankSlot()` - Shared bank save
  - Modified inventory load loop (around line 730)
  - Modified shared bank load loop (around line 634)

## Testing
1. Upgrade an item using `#upgrade` command
2. Verify dynamic ID is correct (check with `#finditem`)
3. Zone out and back in - item should persist
4. Logout and login - item should persist
5. Check database:
   ```sql
   SELECT item_id, custom_data FROM inventory WHERE character_id = <your_char_id>;
   ```
   Should show base_id in item_id and JSON in custom_data

## Database Schema
No changes required - uses existing `custom_data` TEXT column in:
- `inventory` table
- `sharedbank` table

## Example Data Flow
```
Game: Cloth Cap +1 (ID: 101001001)
  ↓ Save
Database: item_id=1001, custom_data='{"dynamic_level":1}'
  ↓ Load
Game: Cloth Cap +1 (ID: 101001001)
```

## Related Systems
- **Dynamic Item Manager**: `zone/dynamic_item_manager.h`
- **Item Scaling**: `common/item_instance.cpp`
- **Evolving Items**: `common/evolving_items.cpp`
