# Procedural Loot & Item Upgrade System

## Overview
This system allows for the dynamic creation of upgraded items based on existing items in the game. Currently, it is configured so that when a player kills any NPC, there is a 100% chance to receive a clone of their current chest armor with upgraded stats (+5 STR, +50 HP).

This system works by using **Lua** to define the logic and **C++ Server Code** to apply the stats so the client can see them natively without needing a custom DLL.

## 1. Lua Implementation
**File:** `quests/global/global_npc.lua`

The logic is hooked into the `event_death_complete` event, which triggers whenever an NPC dies.

### Logic Flow:
1.  **Check Player:** Ensure the killer is a client (player).
2.  **Get Item:** Retrieve the item in the Chest slot (`e.other:GetItemAt(17)`).
3.  **Clone Item:** Create a new instance of the item using `e.other:GetItemAt(17):Clone()`.
4.  **Apply Upgrades:** Use `SetCustomData` to attach new stat values.
    ```lua
    -- Example Lua Code
    local new_item = item:Clone()
    new_item:SetCustomData("STR", 5)  -- Adds 5 Strength
    new_item:SetCustomData("HP", 50)  -- Adds 50 Hit Points
    ```
5.  **Give to Player:** Push the new item to the player's cursor using `e.other:PushItemOnCursor(new_item)`.

## 2. C++ Server Implementation
**Files:**
- `common/item_instance.h`
- `common/item_instance.cpp`

The standard EQEmu server stores `CustomData` as simple text strings. By default, these do not affect item stats. We modified the server code to parse specific keys from `CustomData` and apply them to the item's effective stats.

### Key Function: `ApplyCustomStats()`
We added a new function `void EQ::ItemInstance::ApplyCustomStats()` to `common/item_instance.cpp`.

**How it works:**
1.  It checks if `m_custom_data` contains any entries.
2.  It ensures `m_scaledItem` (a temporary copy of the item used for scaling/modifying stats) exists.
3.  It iterates through the custom data keys and adds the values to the `m_scaledItem` stats.

### Supported Stats
The following keys are currently supported in `ApplyCustomStats`. Use these exact string keys in Lua's `SetCustomData`:

| Key | Stat Modified |
| :--- | :--- |
| `STR` | Strength |
| `STA` | Stamina |
| `DEX` | Dexterity |
| `AGI` | Agility |
| `INT` | Intelligence |
| `WIS` | Wisdom |
| `CHA` | Charisma |
| `HP` | Hit Points |
| `MANA` | Mana |
| `AC` | Armor Class |

### Code Triggers
The `ApplyCustomStats()` function is automatically called whenever:
1.  `SetCustomData` is called (so Lua updates happen immediately).
2.  `Initialize` is called (so stats persist after server restarts/zoning).

## 3. How to Extend

### Changing Drop Rates or Logic
Modify `quests/global/global_npc.lua`. You can add random chance, level checks, or specific zone restrictions there.

### Adding New Stats
To support new stats (e.g., Resistances, Attack, Haste):
1.  Open `common/item_instance.cpp`.
2.  Find `void EQ::ItemInstance::ApplyCustomStats()`.
3.  Add a new `else if` block for the stat.
    ```cpp
    else if (key == "FR") m_scaledItem->FR += iVal; // Fire Resist
    ```
4.  **Recompile** the server (Zone and World).

## 4. Client Compatibility
Because the server modifies the packet data sent to the client (via `m_scaledItem`), the standard EQ client (and standard DLLs) will see the updated stats automatically. **No custom DLL or client-side patch is required.**
