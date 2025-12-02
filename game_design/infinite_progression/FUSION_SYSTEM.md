# Item Fusion System

## Concept
Transfer item levels from one item to another, allowing players to upgrade their base item while keeping progression.

## Use Case
```
Early Game: Find Cloth Cap, upgrade to level 127
Mid Game:   Find Dragon Helm (better base stats)
Solution:   Fuse Cloth Cap +127 into Dragon Helm
Result:     Dragon Helm +127 (better base + all your progress)
```

## Fusion Mechanics

### Basic Fusion
**Command:** `#fuse`

**Requirements:**
- **Cursor Item:** Donor (will be destroyed)
- **Target Item:** Receiver (must be worn or in inventory)

**Process:**
1. Extract level from donor item
2. Get base item ID from receiver
3. Generate new item: receiver base + donor level
4. Destroy donor
5. Replace receiver with fused result

### Formula
```cpp
ItemInstance* FuseItems(ItemInstance* donor, ItemInstance* receiver) {
  int donor_level = GetItemLevel(donor->GetID());
  uint32 receiver_base_id = GetBaseItemID(receiver->GetID());

  // Create scaled item using receiver's base + donor's level
  ItemInstance* fused = CreateDynamicItem(receiver_base_id, donor_level);

  return fused;
}
```

### Example Fusion
```
Donor:    Cloth Cap +127 (ID: 500127001)
          Base: 3 AC
          Level: 127
          Scaled: 130 AC, +254 HP, +127 STR, +65 WIS, +10% Haste, etc.

Receiver: Dragon Helm +0 (ID: 8403)
          Base: 25 AC, +5 STR, +10 STA
          Level: 0

Result:   Dragon Helm +127 (ID: 500127403)
          Base: 25 AC, +5 STR, +10 STA (from receiver)
          Level: 127 (from donor)
          Scaled: 152 AC, +259 STR, +137 STA, +254 HP, +65 WIS, +10% Haste
                  ^^^ Much better than Cloth Cap because Dragon Helm has superior base stats
```

## Advanced Fusion Options

### Partial Fusion (Future Enhancement)
Transfer only a percentage of levels:
```cpp
#fuse 50  // Transfer 50% of donor levels

Cloth Cap +100 → Dragon Helm +0
Result: Dragon Helm +50 (only half the levels transferred)
Cloth Cap becomes Cloth Cap +50 (keeps other half)
```

### Stat Inheritance (Configurable)
Option to carry over some random stats from donor:
```cpp
bool inherit_random_stats = true;  // Config option

if (inherit_random_stats) {
  // 25% chance each random stat from donor carries over
  for (auto& stat : donor->GetRandomStats()) {
    if (rand() < 0.25) {
      fused->AddBonusStat(stat.type, stat.value * 0.5);  // 50% value
    }
  }
}
```

Example:
```
Cloth Cap +127 had random stats: +65 WIS, +32 CHA, +15 Fire Resist
Dragon Helm +127 rolls to inherit:
  - WIS: Success! Adds +32 WIS bonus (50% of 65)
  - CHA: Failed
  - Fire Resist: Success! Adds +7 Fire Resist (50% of 15)

Final: Dragon Helm +127 with extra +32 WIS, +7 FR beyond normal scaling
```

## Fusion Costs (Future)

### Resource Requirements
```lua
-- In upgrade command
local fusion_cost = {
  platinum = donor_level * 100,        -- 100pp per level
  essence = donor_level * 10,          -- Special currency
  materials = {
    { item_id = 123456, count = 5 }    -- Fusion Crystal x5
  }
}
```

### Risk/Reward System
Add chance of failure or level loss:
```lua
local success_chance = 0.95  -- 95% success rate
local failure_penalty = 0.1  -- Lose 10% of levels on failure

if math.random() > success_chance then
  -- Fusion failed!
  result_level = donor_level * (1 - failure_penalty)
  -- Both items destroyed, get: Dragon Helm +(donor_level * 0.9)
end
```

## Strategic Considerations

### When to Fuse
**Pros:**
- Get better base stats immediately
- Continue using a superior item type
- Free up inventory from old item

**Cons:**
- Lose the donor item permanently
- May want to keep donor for different slot
- Receiver must be worth the investment

### Fusion Planning
```
Early Game (Levels 1-30):
  - Don't fuse yet, any item works
  - Focus on leveling what you have

Mid Game (Levels 30-100):
  - Start planning fusion targets
  - Look for good base items (Banded, Chain, Plate)
  - Fuse when you find significantly better base

Late Game (100+):
  - Fuse into raid-quality base items
  - Dragon scale, planar, elemental gear
  - Maximize base stats before continuing to level

End Game (500+):
  - Only fuse into "perfect" base items
  - Every base stat point matters
  - Chase rare/epic base items for fusion
```

## Fusion Chains

Players can create fusion chains by fusing repeatedly:
```
Cloth Cap +50 → Banded Helm +0 = Banded Helm +50
Later...
Banded Helm +100 → Plate Helm +0 = Plate Helm +100
Later...
Plate Helm +200 → Dragon Helm +0 = Dragon Helm +200
Later...
Dragon Helm +500 → Planar Helm +0 = Planar Helm +500
```

Each fusion keeps all the levels but gives better base stats!

## Fusion Limits

### Same-Base Restriction
Cannot fuse item into itself:
```cpp
if (donor_base_id == receiver_base_id) {
  client->Message(13, "Cannot fuse an item into the same item type!");
  return false;
}

// Blocks: Cloth Cap +100 → Cloth Cap +50
```

### Slot Compatibility
Only fuse items of same equipment slot:
```cpp
if (donor->GetItem()->Slots != receiver->GetItem()->Slots) {
  client->Message(13, "Items must be for the same equipment slot!");
  return false;
}

// Blocks: Cloth Cap (head) → Plate Chestguard (chest)
```

### Level Limits (Optional)
Prevent fusion if receiver is higher level:
```cpp
if (receiver_level > donor_level) {
  client->Message(13, "Receiver is already higher level than donor!");
  return false;
}

// Blocks: Cloth Cap +50 → Dragon Helm +100
// (Already better, don't downgrade!)
```

## UI/UX

### Command Syntax
```
#fuse                    -- Fuse cursor item into target
#fuse <slot>             -- Fuse cursor into specific worn slot
#fuse <percentage>       -- Partial fusion (future)
#fuseinfo                -- Preview fusion result before committing
```

### Confirmation Prompt
```lua
-- Show preview before fusion
local donor_level = get_item_level(cursor_item)
local receiver_base = get_base_item(worn_item)

client:Message(15, "=== FUSION PREVIEW ===")
client:Message(15, "Donor: " .. cursor_item:GetItem():Name .. " +" .. donor_level)
client:Message(15, "Receiver: " .. worn_item:GetItem():Name)
client:Message(15, "Result: " .. receiver_base:Name .. " +" .. donor_level)
client:Message(15, "")
client:Message(15, "Type #fuse confirm to proceed (donor will be destroyed!)")
```

## Database Schema

### Fusion Log (Future Analytics)
```sql
CREATE TABLE fusion_log (
  id INT AUTO_INCREMENT PRIMARY KEY,
  character_id INT,
  donor_item_id INT,
  donor_level INT,
  receiver_item_id INT,
  result_item_id INT,
  fused_at TIMESTAMP,
  INDEX idx_character (character_id),
  INDEX idx_fused_at (fused_at)
);
```

Track fusion patterns to analyze:
- Most fused base items (popular targets)
- Average fusion level
- Common fusion chains

## See Also
- [OVERVIEW.md](OVERVIEW.md) - System overview
- [SCALING_FORMULAS.md](SCALING_FORMULAS.md) - How stats scale
- [COMMANDS.md](COMMANDS.md) - All player commands
- [IMPLEMENTATION.md](IMPLEMENTATION.md) - Technical details
