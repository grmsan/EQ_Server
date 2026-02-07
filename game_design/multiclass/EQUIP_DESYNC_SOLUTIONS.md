# Multiclass Equipment Desync Problem & Solutions

## Problem Statement

When a multiclass player attempts to equip an item, there's a potential desync between client and server:

1. **Player drags item** to an equipment slot
2. **Client validates locally** using `GetUsableClasses()` (hooked by DLL to return multiclass bits)
3. **Client sends `OP_SwapItem`** to server if local check passes
4. **Server validates** using `SwapItem()` with `GetClassesBits()`
5. **If server rejects**: Item bounces back to cursor = **visual desync**

The player sees the item "snap back" which is jarring UX and indicates the client thought the equip was valid but the server disagreed.

### Root Cause

The client and server must agree on what classes the player has. Currently:

- **Server**: Reads `GestaltClasses` from data_bucket via `GetClassesBits()`
- **Client DLL**: Receives class bitmask via EdgeStatLabel opcode (0x1338, key 200)

If the DLL hasn't received the bitmask, or receives it late, or the RVA whitelist is incomplete, the client may allow an equip attempt the server will reject.

---

## Current Architecture

### Server-Side: Class Validation

```cpp
// zone/client.cpp:13299
uint32 Client::GetClassesBits() const
{
    const uint32 base_bit = GetPlayerClassBit(GetClass());

    if (!RuleB(Custom, MulticlassingEnabled)) {
        return base_bit;
    }

    // Read from data_bucket "GestaltClasses"
    std::string raw = GetBucket(kGestaltClassesBucketKey);
    // ... fallback logic ...

    uint32 bits = static_cast<uint32>(Strings::ToUnsignedInt(raw, base_bit));
    return bits | base_bit;
}
```

### Server-Side: Equipment Validation

```cpp
// common/inventory_profile.cpp:353
if (race_id && effective_class_bits && !source_item->IsEquipable(race_id, effective_class_bits)) {
    fail_state = swapRaceClass;
    return false;
}

// common/item_data.cpp:170
bool EQ::ItemData::IsEquipable(uint16 race_id, uint16 class_bits) const
{
    uint32 race_bit = GetPlayerRaceBit(race_id);
    if (!(Races & race_bit)) {
        return false;
    }
    // THJServer parity: class_bits is now a bitmask directly
    if (!(Classes & class_bits)) {
        return false;
    }
    return true;
}
```

### Server-Side: Sending Class Bits to DLL

```cpp
// zone/client.cpp:13629
constexpr uint32 kClassesBitmask = 200;

const Pair pairs[] = {
    // ... other stats ...
    { kClassesBitmask, static_cast<uint64>(GetClassesBits()) }
};
// Sent via EdgeStatLabel opcode 0x1338
```

### Client-Side (DLL): Intercepting Class Checks

```cpp
// extras/eq-core-dll-main/src/eqgame.cpp:2072
// Whitelist of RVAs that need multiclass bits for equipment validation
static const std::set<DWORD> s_multiclass_rvas = {
    0x0004C472,  // Equipment validation - MUST have multiclass bits
};

int __fastcall EQCharacter_GetUsableClasses_Detour(void* This, void* edx, int a1, DWORD a2)
{
    int nativeVal = EQCharacter_GetUsableClasses_Tramp(This, edx, a1, a2);

    // If multiclass disabled or no server mask, use native
    if (!isMulticlassUsableClassesOverrideEnabled || g_serverUsableClassesMask == 0) {
        return nativeVal;
    }

    // Check if this RVA needs multiclass bits
    DWORD ret_rva = (DWORD)((uintptr_t)_ReturnAddress() - (uintptr_t)baseAddress);
    bool useMulticlass = (s_multiclass_rvas.find(ret_rva) != s_multiclass_rvas.end());

    return useMulticlass ? static_cast<int>(g_serverUsableClassesMask) : nativeVal;
}
```

---

## Solution Options

### Option A: Modify Item Classes in Serialization (Server-Side)

**Concept**: When serializing items to send to the client, compute the **intersection** of the item's classes and the player's owned classes. This tells the client exactly what the player can equip - items outside their classes show as unequippable.

**How it makes server authoritative**:
- Server computes `effective_classes = item->Classes & player->GetClassesBits()`
- If intersection is 0 → client sees "no valid classes" → blocks equip attempt
- If intersection > 0 → client sees "equippable" → allows equip (server already validated)
- Server's `SwapItem()` validation is unchanged - it's the source of truth

**Example - Player CAN equip**:
```
Item: WAR/ROG only (Classes = 0x0009)
Player: WAR/CLR (GetClassesBits() = 0x0003)
Intersection: 0x0009 & 0x0003 = 0x0001 (WAR bit)
Client sees: "Equippable by Warrior" ✓
```

**Example - Player CANNOT equip**:
```
Item: ROG only (Classes = 0x0008)
Player: WAR/CLR (GetClassesBits() = 0x0003)
Intersection: 0x0008 & 0x0003 = 0x0000 (no overlap)
Client sees: "Cannot equip - no valid classes" ✗
```

**Pros**:
- Server-side change only - no DLL modifications needed
- Client blocks invalid equips BEFORE sending to server (no desync)
- Tooltips show only the classes the player actually has that can use the item
- Works even if DLL hooks fail or aren't installed

**Cons**:
- Need to pass player's class bits into serialization (currently per-item, not per-player)
- Requires architecture change to thread player context into `SerializeItem()`

**Implementation Challenge**:

The problem is `SerializeItem()` in `common/patches/rof2.cpp` doesn't know which player it's serializing for:

```cpp
// Current signature - no player context:
void SerializeItem(EQ::OutBuffer& ob, const EQ::ItemInstance *inst, int16 slot_id, uint8 depth, ItemPacketType packet_type)
{
    const EQ::ItemData *item = inst->GetItem();
    // ...
    ibs.Classes = item->Classes;  // No way to know player's classes here!
}
```

**Solution 1: Store player classes in ItemInstance custom data**

```cpp
// zone/client.cpp - Before sending any item, stamp it with player's classes
void Client::PrepareItemForSend(EQ::ItemInstance* inst)
{
    if (RuleB(Custom, MulticlassingEnabled)) {
        uint32 player_classes = GetClassesBits();
        inst->SetCustomData("__player_classes", std::to_string(player_classes));
    }
}

// common/patches/rof2.cpp - SerializeItem reads the stamp
void SerializeItem(EQ::OutBuffer& ob, const EQ::ItemInstance *inst, int16 slot_id, uint8 depth, ItemPacketType packet_type)
{
    const EQ::ItemData *item = inst->GetItem();

    // Compute effective classes for this player
    uint32 effective_classes = item->Classes;
    std::string player_classes_str = inst->GetCustomData("__player_classes");
    if (!player_classes_str.empty()) {
        uint32 player_classes = std::stoul(player_classes_str);
        effective_classes = item->Classes & player_classes;  // Intersection
        // If no overlap, item is unequippable for this player
    }

    // ... later ...
    ibs.Classes = effective_classes;
}
```

**Solution 2: Add player context parameter to SerializeItem**

```cpp
// common/patches/rof2.cpp - Add optional player_classes parameter
void SerializeItem(EQ::OutBuffer& ob, const EQ::ItemInstance *inst, int16 slot_id,
                   uint8 depth, ItemPacketType packet_type, uint32 player_classes = 0xFFFF)
{
    const EQ::ItemData *item = inst->GetItem();

    // Compute intersection - default 0xFFFF means "show all" (for merchants, etc.)
    uint32 effective_classes = item->Classes & player_classes;

    // ... later ...
    ibs.Classes = effective_classes;
}

// All call sites that send to a specific player pass their classes:
SerializeItem(ob, inst, slot_id, 0, ItemPacketCharInventory, client->GetClassesBits());
```

**Solution 3: Use m_scaledItem to override Classes (leverages existing dynamic item system)**

```cpp
// zone/client.cpp - Create a player-specific view of the item
void Client::SendItemPacketWithClassFilter(int16 slot_id, const EQ::ItemInstance* inst, ItemPacketType packet_type)
{
    if (RuleB(Custom, MulticlassingEnabled)) {
        // Clone the item data with filtered classes
        EQ::ItemInstance* filtered = inst->Clone();

        // If using scaled item system, modify the scaled copy
        if (filtered->m_scaledItem) {
            filtered->m_scaledItem->Classes &= GetClassesBits();
        } else {
            // Create a scaled item just for class filtering
            filtered->m_scaledItem = new EQ::ItemData(*filtered->GetUnscaledItem());
            filtered->m_scaledItem->Classes &= GetClassesBits();
        }

        // Send the filtered version
        SendItemPacket(slot_id, filtered, packet_type);
        safe_delete(filtered);
        return;
    }

    SendItemPacket(slot_id, inst, packet_type);
}

---

### Option B: Block Equip Earlier in DLL (Client-Side)

**Concept**: Add a pre-equip hook in the DLL that validates the item's classes against `g_serverUsableClassesMask` BEFORE allowing the drag-to-slot operation. If the item can't be equipped, block the action client-side with a message.

**Pros**:
- Clean UX - item never moves if invalid
- Server doesn't receive invalid requests
- Can show custom "You cannot equip this item" message

**Cons**:
- Requires DLL modification and rebuild
- Need to find the right hook point for drag operations
- More complex than server-side fix

**Implementation**:

```cpp
// extras/eq-core-dll-main/src/eqgame.cpp

// New: Pre-equip validation hook
// Hook the function that handles item drag-to-slot
// RVA TBD - need to find via debugging

static bool ValidateItemEquipable(uint32_t itemClasses)
{
    // If multiclass is disabled, let native behavior handle it
    if (!isMulticlassUsableClassesOverrideEnabled || g_serverUsableClassesMask == 0) {
        return true;  // Let native check happen
    }

    // Check if item's class restriction overlaps with player's classes
    return (itemClasses & g_serverUsableClassesMask) != 0;
}

// Hook into item equip initiation
DETOUR_TRAMPOLINE_EMPTY(bool __fastcall TryEquipItem_Tramp(void*, void*, int slot, void* item));
bool __fastcall TryEquipItem_Detour(void* This, void* edx, int slot, void* item)
{
    // Extract item's Classes field from the item struct
    // Offset TBD - need to find via debugging
    uint32_t itemClasses = *(uint32_t*)((char*)item + ITEM_CLASSES_OFFSET);

    if (!ValidateItemEquipable(itemClasses)) {
        // Show message to player
        // WriteChatf("Your classes cannot equip this item.");
        if (isDebugLoggingEnabled) {
            LogDebug("EQUIP_BLOCKED: itemClasses=0x%04X playerClasses=0x%04X",
                itemClasses, g_serverUsableClassesMask);
        }
        return false;  // Block the equip attempt
    }

    return TryEquipItem_Tramp(This, edx, slot, item);
}
```

**Finding the hook point**:
```cpp
// Enable verbose logging to discover the RVA for equip initiation
// In _options.h or via debug command:
static bool s_log_equip_attempts = true;

// Add to existing hook to trace call stack when equip happens
// This will help identify the right function to hook
```

---

### Option C: Ensure EdgeStatLabel Timing (Server-Side)

**Concept**: Send the EdgeStatLabel packet more aggressively to ensure the DLL always has current class bits before any equip attempt can occur.

**Pros**:
- Minimal code changes
- Works with existing DLL architecture
- No item serialization changes

**Cons**:
- May not cover all edge cases (race conditions)
- Adds network overhead
- Doesn't fix the fundamental timing issue

**Implementation**:

```cpp
// zone/client.cpp - Add calls to SendEdgeStats() in more places

// 1. When inventory window is opened
void Client::Handle_OP_OpenInventory(const EQApplicationPacket *app)
{
    SendEdgeStats();  // Ensure DLL has current classes before viewing inventory
    // ... existing logic ...
}

// 2. When any item is picked up to cursor
void Client::Handle_OP_ClickObject(const EQApplicationPacket *app)
{
    SendEdgeStats();  // Refresh before potential equip
    // ... existing logic ...
}

// 3. Before sending any item packet
void Client::SendItemPacket(int16 slot_id, const EQ::ItemInstance* inst, ItemPacketType packet_type)
{
    // Ensure DLL has multiclass bits before item data arrives
    if (RuleB(Custom, MulticlassingEnabled)) {
        SendEdgeStats();
    }
    // ... existing send logic ...
}
```

**Current SendEdgeStats call sites** (for reference):
```cpp
// zone/client_packet.cpp:559 - On zone entry
SendEdgeStats();

// zone/client_packet.cpp:1293 - After zone load complete
SendEdgeStats();

// zone/client_process.cpp:126 - Periodic refresh after connect
SendEdgeStats();

// zone/client.cpp:13371, 13460, 13468, 13508 - Various stat updates
SendEdgeStats();
```

**Enhanced timing approach**:
```cpp
// zone/client.h - Add tracking
class Client {
    // ...
    Timer m_edge_stats_timer;  // Periodic refresh timer
    bool m_edge_stats_dirty{true};  // Flag when classes change
};

// zone/client.cpp
void Client::Process()
{
    // ... existing process logic ...

    // Periodic EdgeStats refresh for multiclass players
    if (RuleB(Custom, MulticlassingEnabled) && m_edge_stats_timer.Check()) {
        if (m_edge_stats_dirty || GetClassesBits() != GetPlayerClassBit(GetClass())) {
            SendEdgeStats();
            m_edge_stats_dirty = false;
        }
    }
}

void Client::SetClassesBits(uint32 classes_bits)
{
    // ... existing logic ...
    m_edge_stats_dirty = true;  // Mark for refresh
    SendEdgeStats();  // Immediate send when classes change
}
```

---

## Recommendation

**Option B (DLL pre-equip hook)** is likely the cleanest solution because:

1. **No serialization changes** - Item data stays accurate for tooltips, trading, etc.
2. **DLL already has the data** - `g_serverUsableClassesMask` contains player's classes
3. **Single hook point** - Find the equip validation and check `item->Classes & g_serverUsableClassesMask`
4. **Immediate feedback** - Can show "You cannot equip this item" message

**However**, if the DLL approach is failing (which your symptoms suggest), we need to diagnose WHY first:

### Diagnostic Steps

1. **Check if EdgeStatLabel is being received**:
   ```
   Look in <RoF2 client>/dinput8_debug.log for:
   EDGE_STAT multiclass classes_bitmask=0xNNNN
   ```

2. **Check if GetUsableClasses is being called**:
   ```
   Look for: [USABLE_CLASSES] RVA=0x... logs
   ```

3. **Run #mcdiag in-game**:
   ```
   Should show server's GetClassesBits() value
   ```

If the DLL isn't receiving classes, the EdgeStatLabel sending needs to be fixed first.

If the DLL IS receiving classes but equips still work, then:
- The RVA whitelist (0x0004C472) may be wrong for your client version
- Or there's a different code path for equip validation

### Quick Test for Option A

To test Option A without architecture changes, you can temporarily hack it:

```cpp
// common/patches/rof2.cpp - Around line 6606
// TEMPORARY TEST - hardcode intersection for testing
// This won't work for different players but proves the concept

// Original:
// ibs.Classes = item->Classes;

// Test hack - assume single-class Warrior (bit 0x0001):
// Items that don't include Warrior will show as unequippable
ibs.Classes = item->Classes & 0x0001;
```

If this makes non-Warrior items unequippable on the client, Option A is viable and we just need to properly thread player context through.

---

## Testing Checklist

After implementing any option:

- [ ] Multiclass player can equip items for all owned classes
- [ ] Multiclass player CANNOT equip items for classes they don't have (server rejects)
- [ ] Single-class player equipment works normally
- [ ] Item tooltips show correct class restrictions
- [ ] No "bounce back to cursor" desync on valid equips
- [ ] EdgeStatLabel is received by DLL (check dinput8_debug.log)
- [ ] `#mcdiag` command shows correct class bits on server and client
