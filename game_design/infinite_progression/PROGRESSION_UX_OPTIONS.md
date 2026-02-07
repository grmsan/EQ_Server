# Infinite Progression UX Design

> **Status:** SELECTED DESIGN - Hybrid XP + Dual Essence System
> **Purpose:** Player-driven item progression with visible progress, meaningful choices, and tunable scaling.

---

## Selected Design: Hybrid XP + Dual Essence

### Overview
Items level up through three complementary paths:
1. **Passive XP** - Items gain XP from combat (always progressing)
2. **Common Essence** - Bulk currency from salvaging items (grind path)
3. **Rare Essence** - Valuable currency from named/raids (shortcut path)

### Core Currencies

| Currency | Source | Purpose |
|----------|--------|---------|
| **Item XP** | Combat kills | Passive progression, always ticking |
| **Common Essence** | Salvage any item | Bulk upgrades, always useful |
| **Rare Essence** | Named mobs, raids, rare drops | Skip the grind, valuable |

---

## Training Slot (Power Source)

### The Problem
You find a great base item (Dragon Helm +1) but all your gear is +50. You want to level it up, but equipping it means losing stats. How do you progress the new item without gimping yourself?

### The Solution: Training Slot
The **Power Source slot** (slot 22) acts as a "training slot" where ANY equipment can be placed:
- Item in training slot gains XP from kills (same as equipped items)
- Item provides **NO stats** while in training slot
- Limited to **1 item at a time** (forces choice)
- Once leveled up, move it to actual equipment slot

### How It Works
```
[Scenario]
  Current: Banded Helm +50 equipped (great stats)
  Found:   Dragon Helm +1 (better base, no levels)

[Solution]
  1. Put Dragon Helm +1 in Power Source slot
  2. Keep Banded Helm +50 equipped (keep your stats)
  3. Kill mobs → Dragon Helm gains XP in training slot
  4. Once Dragon Helm reaches +50, swap it in

[Result]
  Dragon Helm +50 now equipped (better than Banded +50)
  Banded Helm +50 can be salvaged or saved for fusion
```

### Training Slot Rules
```lua
TRAINING_SLOT = 22  -- Power Source slot (RoF2)

-- Training slot XP modifier (optional penalty for "free" leveling)
TRAINING_SLOT_XP_MULTIPLIER = 1.0  -- 100% XP (no penalty by default)
-- Alternative: 0.5 = 50% XP (makes equipping faster, training slower)

-- What can go in training slot
TRAINING_SLOT_ALLOWED = {
    "armor",      -- All armor pieces
    "weapons",    -- All weapon types
    "jewelry",    -- Rings, earrings, neck, etc.
    "misc"        -- Charms, range items, etc.
}

-- NOT allowed (if we want restrictions)
TRAINING_SLOT_BLOCKED = {
    -- Could block certain items if needed
}
```

### UX Messages
```
[Place item in training slot]
You place Dragon Helm +1 in your training slot.
This item will gain XP but provide no stats.

[Kill mob]
You have slain a fire giant!
  Your Banded Helm +50 gains 70 XP (equipped)
  Your Dragon Helm +1 gains 70 XP (training)

[Item ready]
Your Dragon Helm has reached +50!
Consider moving it from training to equipment.
```

### Strategic Choices
The training slot creates meaningful decisions:

1. **Level one item passively** - Put new drop in training, keep farming
2. **Equip weaker item for faster XP** - If you can handle the stat loss
3. **Use essence to skip training** - Spend essence to level the item directly
4. **Fusion shortcut** - Fuse old +50 into new base (costs catalyst)

---

## Tunable Formulas

All values driven by formulas with adjustable constants in `item_scaling.json`:

### XP Required Per Level
```lua
-- Formula: XP_to_next = BASE_XP * (1 + level * XP_LEVEL_SCALE) ^ XP_EXPONENT
-- Knobs: BASE_XP, XP_LEVEL_SCALE, XP_EXPONENT

XP_BASE = 100           -- Starting XP requirement
XP_LEVEL_SCALE = 0.02   -- How much each level adds (2% per level)
XP_EXPONENT = 1.5       -- Curve steepness (1.0 = linear, 2.0 = quadratic)

function xp_to_next_level(item_level)
    return math.floor(XP_BASE * (1 + item_level * XP_LEVEL_SCALE) ^ XP_EXPONENT)
end

-- Examples with default values:
-- Level 1→2:    100 XP
-- Level 10→11:  133 XP
-- Level 50→51:  346 XP
-- Level 100→101: 693 XP
-- Level 200→201: 1,744 XP
-- Level 500→501: 7,416 XP
```

### XP Gained Per Kill
```lua
-- Formula: XP_gained = MOB_LEVEL * XP_PER_MOB_LEVEL * CON_MULTIPLIER
-- Knobs: XP_PER_MOB_LEVEL, con multipliers

XP_PER_MOB_LEVEL = 1.0  -- Base XP per mob level

CON_MULTIPLIERS = {
    gray = 0.1,
    green = 0.25,
    light_blue = 0.5,
    blue = 0.75,
    white = 1.0,
    yellow = 1.25,
    red = 1.5
}

NAMED_XP_MULTIPLIER = 5.0   -- Named mobs give 5x XP
RAID_XP_MULTIPLIER = 20.0   -- Raid bosses give 20x XP

function xp_from_kill(mob_level, con_color, is_named, is_raid)
    local base = mob_level * XP_PER_MOB_LEVEL * CON_MULTIPLIERS[con_color]
    if is_raid then return base * RAID_XP_MULTIPLIER end
    if is_named then return base * NAMED_XP_MULTIPLIER end
    return base
end

-- Examples (level 70 mob):
-- Gray trash:  70 * 1.0 * 0.1 = 7 XP
-- White trash: 70 * 1.0 * 1.0 = 70 XP
-- Red named:   70 * 1.0 * 1.5 * 5 = 525 XP
-- Raid boss:   70 * 1.0 * 1.5 * 20 = 2,100 XP
```

### Common Essence Cost Per Level
```lua
-- Formula: cost = COMMON_BASE * (1 + level * COMMON_SCALE) ^ COMMON_EXPONENT
-- Knobs: COMMON_BASE, COMMON_SCALE, COMMON_EXPONENT

COMMON_BASE = 10            -- Base cost at level 1
COMMON_SCALE = 0.05         -- 5% increase per level
COMMON_EXPONENT = 2.0       -- Quadratic scaling

function common_essence_cost(item_level)
    return math.floor(COMMON_BASE * (1 + item_level * COMMON_SCALE) ^ COMMON_EXPONENT)
end

-- Examples:
-- Level 1→2:    10 Common
-- Level 10→11:  23 Common
-- Level 50→51:  156 Common
-- Level 100→101: 506 Common
-- Level 200→201: 1,806 Common
-- Level 500→501: 10,506 Common
```

### Rare Essence Cost Per Level
```lua
-- Formula: cost = max(1, floor(level / RARE_LEVEL_DIVISOR))
-- Knobs: RARE_LEVEL_DIVISOR

RARE_LEVEL_DIVISOR = 100    -- 1 Rare per 100 levels

function rare_essence_cost(item_level)
    return math.max(1, math.floor(item_level / RARE_LEVEL_DIVISOR))
end

-- Examples:
-- Level 1→2:    1 Rare
-- Level 50→51:  1 Rare
-- Level 100→101: 1 Rare
-- Level 150→151: 2 Rare
-- Level 200→201: 2 Rare
-- Level 500→501: 5 Rare
```

### Item Salvage Values
```lua
-- Formula: essence = BASE_SALVAGE + (item_level * SALVAGE_PER_LEVEL) + QUALITY_BONUS
-- Knobs: BASE_SALVAGE, SALVAGE_PER_LEVEL, quality bonuses

BASE_SALVAGE = 1            -- Minimum essence from any item
SALVAGE_PER_LEVEL = 2       -- +2 Common per item level

QUALITY_SALVAGE_BONUS = {
    -- Based on original item quality/value
    trash = 0,              -- Rusty weapons, cloth
    common = 5,             -- Standard drops
    uncommon = 15,          -- Named drops
    rare = 50,              -- Rare/quest items
    epic = 200,             -- Epic items
    raid = 500              -- Raid drops
}

-- Rare essence from salvage (only high-value items)
RARE_FROM_SALVAGE_THRESHOLD = 100   -- Item must give 100+ Common to yield Rare
RARE_SALVAGE_RATIO = 100            -- 100 Common value = 1 Rare

function salvage_item(item)
    local common = BASE_SALVAGE
                 + (item.level * SALVAGE_PER_LEVEL)
                 + QUALITY_SALVAGE_BONUS[item.quality]

    local rare = 0
    if common >= RARE_FROM_SALVAGE_THRESHOLD then
        rare = math.floor(common / RARE_SALVAGE_RATIO)
    end

    return common, rare
end

-- Examples:
-- Rusty Sword +0:         1 Common, 0 Rare
-- Banded Helm +10:        1 + 20 + 5 = 26 Common, 0 Rare
-- Dragon Helm +50 (rare): 1 + 100 + 50 = 151 Common, 1 Rare
-- Raid Drop +0:           1 + 0 + 500 = 501 Common, 5 Rare
-- Raid Drop +100:         1 + 200 + 500 = 701 Common, 7 Rare
```

---

## Player Actions

### 1. Passive XP (Automatic)
Every kill adds XP to ALL equipped items. No player action needed.

```
[Kill Message]
You have slain a fire giant!
  Your Dragon Helm gains 70 XP (234/346 to +51)
  Your Plate Chest gains 70 XP (89/693 to +101)
  ...
```

### 2. Salvage Items (NPC: "The Recycler")
Trade unwanted items for essence.

```
[Hail The Recycler]
"Bring me your unwanted arms and armor. I'll extract their essence for you."

[Trade Window]
  Give: Rusty Sword, Banded Helm +10, Dragon Helm +50
  Receive: 178 Common Essence, 1 Rare Essence

[Confirm] → Items destroyed, essence added to currency
```

### 3. Upgrade Item (NPC: "The Infuser")
Spend essence to instantly level an item.

```
[Hail The Infuser]
"I can infuse your equipment with essence to make it stronger."

[Trade Window - shows upgrade options]
  Item: Dragon Helm +50
  Next Level Cost:
    - 156 Common Essence (you have: 1,234)
    - OR 1 Rare Essence (you have: 7)

  [Upgrade with Common] [Upgrade with Rare]

[Result]
  Your Dragon Helm is now +51!
```

### 4. Bulk Upgrade (NPC: "The Infuser" - Multi-level)
Spend essence to gain multiple levels at once.

```
[Multi-Upgrade Dialog]
  Item: Dragon Helm +50
  Upgrade to level: [____] (enter target level)

  Cost to reach +60:
    - 2,145 Common Essence
    - OR 10 Rare Essence

  [Confirm]
```

### 5. Bulk Salvage Quest (NPC: "The Collector")
Turn in X items for bonus rewards (solves the "50 raid drops" problem).

```
[Hail The Collector]
"I'm always looking for equipment. Bring me items and I'll reward you handsomely."

[Quest: Raid Salvage Run]
  Turn in: 25 items of any quality
  Reward:
    - 500 Common Essence (bonus beyond individual salvage)
    - 3 Rare Essence
    - 10,000 Item XP (applied to lowest-level equipped item)

[Quest: Mass Recycling]
  Turn in: 100 items of any quality
  Reward:
    - 2,500 Common Essence
    - 15 Rare Essence
    - 50,000 Item XP (split across all equipped items)
```

### 6. Direct Item XP Infusion (NPC: "The Trainer")
Convert essence directly to Item XP for a specific item.

```
[Hail The Trainer]
"I can help your equipment gain experience directly."

[Trade Window]
  Item: Dragon Helm +50
  Current XP: 234/346

  Options:
    - 10 Common → +50 XP
    - 50 Common → +275 XP (10% bonus)
    - 100 Common → +600 XP (20% bonus)

  [Apply 100 Common]

[Result]
  Your Dragon Helm gains 600 XP!
  Your Dragon Helm is now +51! (254/506 to +52)
```

---

## Fusion System (Detailed Implementation)

Fusion transfers levels from a **donor item** to a **receiver item**. The donor is destroyed, and the receiver becomes a new item with the receiver's base stats + donor's levels.

### Why Use Fusion?

| Scenario | Without Fusion | With Fusion |
|----------|---------------|-------------|
| Found Dragon Helm +0, have Banded Helm +50 | Train Dragon Helm from scratch (slow) | Fuse Banded +50 → Dragon = Dragon +50 (instant) |
| Want to upgrade base item | Lose all progress | Keep all progress on better base |

### Fusion Methods

#### Method 1: Direct Fusion (Item → Item)
Transfer levels directly from one item to another.

```
[Requirements]
- Donor item (on cursor) - will be DESTROYED
- Receiver item (equipped or in inventory)
- Catalyst: Rare Essence (cost scales with donor level)
- Same equipment slot (helm → helm, weapon → weapon)

[Process]
1. Player puts donor item on cursor
2. Player talks to "The Fuser" NPC
3. NPC shows preview: "Fuse Banded Helm +50 into Dragon Helm +0?"
4. NPC shows cost: "Catalyst cost: 1 Rare Essence"
5. Player confirms
6. Donor destroyed, receiver becomes Dragon Helm +50
```

#### Method 2: Extract + Infuse (Item → Essence → Item)
Convert levels to essence, then spend essence to level a different item.

```
[Extraction - at "The Extractor" NPC]
1. Player trades leveled item
2. NPC returns: Base item +0 AND essence based on levels
   - Banded Helm +50 → Banded Helm +0 + 250 Common + 2 Rare
3. Player keeps base item (can re-level or sell)

[Infusion - at "The Infuser" NPC]
1. Player spends essence to level target item
2. Dragon Helm +0 + 506 Common = Dragon Helm +50
   (or 1 Rare = Dragon Helm +1, need 50 Rare for +50)
```

### Catalyst Cost Formula
```lua
-- Direct fusion requires Rare Essence as "catalyst"
-- This prevents trivial fusion abuse

FUSION_CATALYST_DIVISOR = 50  -- 1 Rare per 50 levels

function fusion_catalyst_cost(donor_level)
    return math.max(1, math.floor(donor_level / FUSION_CATALYST_DIVISOR))
end

-- Examples:
-- Fuse +25 item:  1 Rare catalyst
-- Fuse +50 item:  1 Rare catalyst
-- Fuse +100 item: 2 Rare catalyst
-- Fuse +250 item: 5 Rare catalyst
-- Fuse +500 item: 10 Rare catalyst
```

### Extraction Values
```lua
-- Extraction is MORE efficient than salvage for leveled items
-- Salvage: BASE + (level * 2) + quality_bonus
-- Extract: (level * 5) Common + (level / 25) Rare

EXTRACTION_COMMON_PER_LEVEL = 5
EXTRACTION_RARE_PER_LEVELS = 25

function extract_item(item)
    local common = item.level * EXTRACTION_COMMON_PER_LEVEL
    local rare = math.floor(item.level / EXTRACTION_RARE_PER_LEVELS)
    return common, rare
end

-- Examples:
-- Extract +25 item:  125 Common, 1 Rare
-- Extract +50 item:  250 Common, 2 Rare
-- Extract +100 item: 500 Common, 4 Rare
-- Extract +200 item: 1000 Common, 8 Rare
```

### Implementation: NPC Lua Scripts

#### The Fuser NPC (Direct Fusion)
```lua
-- quests/templates/the_fuser.lua

function event_say(e)
    if e.message:findi("hail") then
        e.self:Say("I can [fuse] the power of one item into another. "
            .. "The donor item will be destroyed, but its levels transfer to the receiver.")
    elseif e.message:findi("fuse") then
        -- Check if player has item on cursor
        local cursor_item = e.other:GetInventory():GetItem(33)  -- Cursor slot
        if not cursor_item then
            e.self:Say("Place the DONOR item on your cursor, then say 'fuse' again.")
            return
        end

        local donor_level = get_item_level(cursor_item:GetID())
        if donor_level == 0 then
            e.self:Say("That item has no levels to transfer.")
            return
        end

        -- Store pending fusion state
        e.other:SetEntityVariable("fusion_donor_id", tostring(cursor_item:GetID()))
        e.other:SetEntityVariable("fusion_donor_level", tostring(donor_level))

        local catalyst_cost = math.max(1, math.floor(donor_level / 50))

        e.self:Say(string.format(
            "Your %s (+%d) can be fused. Catalyst cost: %d Rare Essence. "
            .. "Now target the RECEIVER item and say 'confirm [slot]' (e.g., 'confirm head').",
            cursor_item:GetItem():Name(), donor_level, catalyst_cost
        ))

    elseif e.message:findi("confirm") then
        local donor_level = tonumber(e.other:GetEntityVariable("fusion_donor_level") or "0")
        if donor_level == 0 then
            e.self:Say("You haven't started a fusion. Say 'fuse' with a donor item on cursor.")
            return
        end

        -- Parse slot from message: "confirm head" -> "head"
        local slot_name = e.message:match("confirm%s+(%w+)")
        local slot_id = slot_name_to_id(slot_name)

        local receiver = e.other:GetInventory():GetItem(slot_id)
        if not receiver then
            e.self:Say("No item in that slot.")
            return
        end

        -- Check catalyst cost
        local catalyst_cost = math.max(1, math.floor(donor_level / 50))
        local player_rare = e.other:GetAlternateCurrency(RARE_ESSENCE_ID)

        if player_rare < catalyst_cost then
            e.self:Say(string.format("You need %d Rare Essence. You have %d.",
                catalyst_cost, player_rare))
            return
        end

        -- Perform fusion
        local receiver_base_id = get_base_item_id(receiver:GetID())
        local fused_item = eq.CreateDynamicInstance(receiver_base_id, donor_level)

        if fused_item then
            -- Deduct catalyst
            e.other:AddAlternateCurrency(RARE_ESSENCE_ID, -catalyst_cost)

            -- Remove donor from cursor
            e.other:DeleteItemInInventory(33, 0, true)

            -- Replace receiver with fused item
            e.other:DeleteItemInInventory(slot_id, 0, true)
            e.other:SummonItem(fused_item:GetID(), 1, false, slot_id)

            e.self:Say(string.format("Fusion complete! Your %s is now +%d!",
                fused_item:GetItem():Name(), donor_level))
        else
            e.self:Say("Fusion failed! Please contact a GM.")
        end

        -- Clear state
        e.other:SetEntityVariable("fusion_donor_id", "")
        e.other:SetEntityVariable("fusion_donor_level", "")
    end
end
```

#### The Extractor NPC (Item → Essence)
```lua
-- quests/templates/the_extractor.lua

function event_trade(e)
    local item_lib = require("items")

    for i = 1, 4 do
        local item = e.trade["item" .. i]
        if item and item.valid then
            local item_level = get_item_level(item.item_id)

            if item_level > 0 then
                -- Extraction: levels → essence
                local common_gained = item_level * 5
                local rare_gained = math.floor(item_level / 25)

                -- Give essence
                e.other:AddAlternateCurrency(COMMON_ESSENCE_ID, common_gained)
                if rare_gained > 0 then
                    e.other:AddAlternateCurrency(RARE_ESSENCE_ID, rare_gained)
                end

                -- Give back base item (level 0)
                local base_id = get_base_item_id(item.item_id)
                e.other:SummonItem(base_id)

                e.self:Say(string.format(
                    "Extracted %d levels from %s. You receive: %d Common Essence%s. "
                    .. "Your base item has been returned.",
                    item_level, item.item_name, common_gained,
                    rare_gained > 0 and (", " .. rare_gained .. " Rare Essence") or ""
                ))
            else
                -- No levels, just salvage
                e.self:Say("That item has no levels to extract. Try the Recycler for salvage.")
                e.other:SummonItem(item.item_id)  -- Return item
            end
        end
    end
end
```

### C++ API (Already Exists)

The core fusion logic already exists in `zone/dynamic_item_manager.cpp`:

```cpp
// FuseItems - transfer levels from donor to receiver
EQ::ItemInstance* DynamicItemManager::FuseItems(
    EQ::ItemInstance* donor,
    EQ::ItemInstance* receiver
) {
    int donor_level = GetItemLevel(donor->GetID());
    uint32 receiver_base_id = GetBaseItemID(receiver->GetID());

    // Create new item: receiver's base + donor's level
    return CreateDynamicInstance(receiver_base_id, donor_level);
}

// FuseWithCharge - use a stored level value instead of donor item
EQ::ItemInstance* DynamicItemManager::FuseWithCharge(
    int donorLevel,
    EQ::ItemInstance* receiver
) {
    uint32 receiver_base_id = GetBaseItemID(receiver->GetID());
    return CreateDynamicInstance(receiver_base_id, donorLevel);
}
```

**To expose to Lua**, we need to add bindings in `zone/lua_mod.cpp`:
```cpp
// Add to lua_register_general or similar
luabind::def("CreateDynamicInstance",
    [](uint32 base_id, int level) {
        return EQ::DynamicItemManager::Get().CreateDynamicInstance(base_id, level);
    }),
luabind::def("GetItemLevel",
    [](uint32 item_id) {
        return EQ::DynamicItemManager::Get().GetItemLevel(item_id);
    }),
luabind::def("GetBaseItemID",
    [](uint32 item_id) {
        return EQ::DynamicItemManager::Get().GetBaseItemID(item_id);
    }),
```

### Slot Compatibility Rules

```lua
-- Fusion requires same slot type
SLOT_GROUPS = {
    head = {"head"},
    chest = {"chest"},
    arms = {"arms"},
    hands = {"hands"},
    legs = {"legs"},
    feet = {"feet"},
    waist = {"waist"},
    back = {"back"},
    neck = {"neck"},
    face = {"face"},
    ears = {"ear1", "ear2"},
    wrists = {"wrist1", "wrist2"},
    fingers = {"finger1", "finger2"},
    shoulders = {"shoulder"},
    range = {"range"},
    primary = {"primary"},
    secondary = {"secondary", "primary"},  -- Can fuse 1h into either
    charm = {"charm"},
    ammo = {"ammo"}
}

function can_fuse(donor_slots, receiver_slots)
    -- Find slot group for each item
    for group_name, slots in pairs(SLOT_GROUPS) do
        local donor_match = false
        local receiver_match = false
        for _, slot in ipairs(slots) do
            if donor_slots:find(slot) then donor_match = true end
            if receiver_slots:find(slot) then receiver_match = true end
        end
        if donor_match and receiver_match then
            return true
        end
    end
    return false
end
```

### Full UX Flow Example

```
[Player has]
  Equipped: Banded Helm +50 (head slot)
  Found: Dragon Helm +0 (raid drop, better base stats)
  Currency: 15 Rare Essence, 2000 Common Essence

[Option 1: Direct Fusion - Instant but costs catalyst]
  1. Put Banded Helm +50 on cursor
  2. Hail The Fuser: "fuse"
  3. NPC: "Your Banded Helm (+50) can be fused. Cost: 1 Rare Essence."
  4. Equip Dragon Helm +0 in head slot
  5. Say: "confirm head"
  6. Result: Dragon Helm +50, -1 Rare Essence, Banded Helm destroyed

[Option 2: Extract + Infuse - Slower but keeps base item]
  1. Trade Banded Helm +50 to The Extractor
  2. Receive: Banded Helm +0 (returned) + 250 Common + 2 Rare
  3. Visit The Infuser with Dragon Helm +0
  4. Spend 506 Common (or 1 Rare for +1 at a time) to reach +50
  5. Result: Dragon Helm +50, Banded Helm +0 saved for later

[Option 3: Training Slot - Free but slow]
  1. Put Dragon Helm +0 in training slot
  2. Keep Banded Helm +50 equipped
  3. Kill mobs, Dragon Helm gains XP
  4. Wait until Dragon Helm reaches +50
  5. Swap items
  6. Result: Dragon Helm +50, Banded Helm +50 can be salvaged/extracted
```


---

## UX Flow: Post-Raid Example

Player completes a raid, gets 50 drops:
- 2 items are upgrades (equip them)
- 3 items are good bases for fusion later (bank them)
- 45 items are "junk"

**Option A: Quick Salvage**
```
Visit The Recycler, trade all 45 items
  → Receive ~5,000 Common Essence, ~20 Rare Essence
  → Use essence to upgrade equipped items
```

**Option B: Bulk Quest**
```
Visit The Collector, turn in 25 items for quest
  → Receive bonus essence + Item XP
Turn in remaining 20 to Recycler
  → Get standard salvage value
```

**Option C: Mix of Actions**
```
Salvage 40 trash items → 4,000 Common, 15 Rare
Extract 5 leveled items you don't need → 500 Common, 5 Rare
Use 20 Rare to upgrade your main weapon 10 levels
Bank the Common for later
```

---

## Config File Structure

All tuning values in `item_scaling.json`:

```json
{
  "ItemProgression": {
    "XP": {
      "BaseXP": 100,
      "LevelScale": 0.02,
      "Exponent": 1.5,
      "PerMobLevel": 1.0,
      "NamedMultiplier": 5.0,
      "RaidMultiplier": 20.0,
      "ConMultipliers": {
        "gray": 0.1,
        "green": 0.25,
        "light_blue": 0.5,
        "blue": 0.75,
        "white": 1.0,
        "yellow": 1.25,
        "red": 1.5
      }
    },
    "TrainingSlot": {
      "Enabled": true,
      "SlotID": 22,
      "XPMultiplier": 1.0,
      "AllowAllEquipment": true
    },
    "CommonEssence": {
      "BaseCost": 10,
      "LevelScale": 0.05,
      "Exponent": 2.0
    },
    "RareEssence": {
      "LevelDivisor": 100
    },
    "Salvage": {
      "BaseValue": 1,
      "PerLevel": 2,
      "QualityBonus": {
        "trash": 0,
        "common": 5,
        "uncommon": 15,
        "rare": 50,
        "epic": 200,
        "raid": 500
      },
      "RareThreshold": 100,
      "RareRatio": 100
    },
    "Extraction": {
      "CommonPerLevel": 5,
      "RarePerLevels": 25
    },
    "Fusion": {
      "CatalystDivisor": 50
    },
    "Quests": {
      "BulkSalvageCount": 25,
      "BulkSalvageBonusCommon": 500,
      "BulkSalvageBonusRare": 3,
      "BulkSalvageBonusXP": 10000
    }
  }
}
```

---

## Implementation Phases

### Phase 1: Core Currency & XP System
- [ ] Add Common/Rare Essence as alternate currencies
- [ ] Implement XP tracking on items (stored in item_instance or data_bucket)
- [ ] XP gain on mob death (modify global_npc.lua)
- [ ] **Training Slot support** - Power Source slot accepts any equipment
- [ ] Training Slot XP gain (no stats, just XP)
- [ ] Basic upgrade NPC (Lua quest script)

### Phase 2: Salvage System
- [ ] Salvage NPC with trade window
- [ ] Item quality detection for bonus essence
- [ ] Salvage value formulas

### Phase 3: Advanced Features
- [ ] Bulk upgrade (multi-level)
- [ ] Extraction system
- [ ] Bulk salvage quests
- [ ] Direct XP infusion

### Phase 4: Fusion Integration
- [ ] Fusion catalyst cost
- [ ] Player-accessible fusion NPC
- [ ] Fusion preview system

---

## Legacy Options (Archived)

The following options were considered but not selected:

| Option | Concept | Why Not Selected |
|--------|---------|------------------|
| **A: Pure XP** | Items gain XP, auto-level | Too passive, no player agency |
| **B: Tiered Essence** | 4 tiers of essence drops | Too complex, inventory clutter |
| **C: Kill Counts** | Items track kills, evolve | Feels grindy, hard to catch up new items |
| **D: Sacrifice** | Feed items to other items | Confusing math, wasteful feeling |
| **F: Risk/Reward** | Upgrades can fail at high levels | Too punishing for solo server |

**Selected: Modified Option E** - Hybrid XP + Dual Essence with Salvage
- Combines passive progress (XP) with player agency (essence spending)
- Two currencies (Common/Rare) instead of four tiers
- Salvage system gives value to all drops
- Formula-driven for easy tuning
