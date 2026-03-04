# EQEmu Implementation Feasibility Analysis

**Version:** 1.0
**Date:** March 2, 2026
**Purpose:** Technical feasibility assessment of the Infinite Item Progression system for server-side EQEmu implementation

---

## Executive Summary

**Overall Feasibility: 85% (Highly Achievable with Minor Compromises)**

The proposed Infinite Item Progression system is **largely implementable** using server-side EQEmu code alone. Most core mechanics leverage existing EQEmu capabilities: custom item instances, data buckets, packet manipulation, and Perl/Lua scripting. However, several features require workarounds or simplified implementations due to client limitations.

**Critical Dependency:** You already have the `eq-core-dll-main` (dinput8.dll) integration available. This is a **massive** advantage. Several "hard" problems (UI buttons, visual effects, item name colors) become trivial with client-side support.

---

## System-by-System Feasibility

### ✅ **1. Item Tier System (Base → Enchanted → Legendary → Mythic)**

**Feasibility: 100% — Fully Doable**

**Implementation:**
- Use ItemInstance `custom_data` to store tier level (0-3)
- Server calculates scaled stats on-the-fly when sending item packets
- Augment slot count is dynamically set based on tier
- Database tracks base item stats; server multiplies on-demand

**EQEmu Support:**
- `ItemInst::SetCustomData()` and `GetCustomData()` — store tier info
- Packet manipulation in `Client::SendItemPacket()` to apply stat multipliers
- Repository layer to query base item stats
- All stat calculations happen server-side before packets are sent

**No Client Modification Needed:** The client receives a properly formed item packet with the scaled stats already applied.

**Caveats:**
- Item stat changes require re-sending the item packet (delete + limbo + re-add pattern you're already using)
- No performance concerns — calculations are trivial

---

### ✅ **2. Power Slot Progression Tracking**

**Feasibility: 100% — Fully Doable**

**Implementation:**
- Repurpose the Power Source equipment slot (slot ID 9999 in RoF2+)
- Track item XP in `character_data_buckets` (key: `power_slot_item_xp`)
- Track equipped Power Slot item ID in bucket (key: `power_slot_item_id`)
- On mob death, award XP based on con color multipliers (server-side calculation)
- Fire tier-up event when XP threshold is met OR when player pays iLevel-scaled Essence cost

**EQEmu Support:**
- `Client::GetPowerSourceSlot()` — access the slot
- Data buckets for persistent XP tracking
- `EVENT_SLAY` in quest scripts to hook mob kills
- Con color calculation already exists: `Mob::GetLevelCon()`
- Timer checks to prevent XP exploits

**No Client Modification Needed:** Power source slot already exists in modern clients.

**Caveats:**
- Must validate item is still in the slot on each XP gain (prevent swapping exploits)
- **XP progress display:** Append item XP progress to the existing “You gain experience”
  message shown on each kill — e.g., “You gain experience! (1.2% item XP)”. This requires a
  small hook in the XP message path. Milestone messages at each 10% ("Your Hategiver has
  reached 50% toward Legendary!") provide pacing feedback.
- **Tier-up ceremony:** On tier transition, play the existing “ding” level-up sound
  (`OP_LevelUpdate` or equivalent), display a distinctive server message, and optionally
  announce to the zone/server. No DLL needed for basic ceremony.

---

### ⚠️ **3. Ghost Item Projection**

**Feasibility: 75% — Doable with Workarounds**

**Your Design:** Server spawns a "ghost duplicate" in the native equipment slot when Power Slot item has an empty native slot. Picking up the ghost unequips the Power Slot item.

**Server-Side Implementation:**
- Detect when item is equipped in Power Slot
- Check if native slot is empty (e.g., Primary hand for 1H weapon)
- Create a server-side "ghost" ItemInstance with:
  - Flag: `item->SetInstNoDrop(true)` — make it untradeable
  - Custom data: `ghost_item = true`
  - Same stats as Power Slot item
- Send ghost to client via normal equip packet
- Hook `Client::SwapItem()` — if ghost is being moved/unequipped, also unequip Power Slot item

**EQEmu Support:**
- Full control over item creation and equipping
- Hooks exist for equipment changes
- Can intercept drag/drop operations

**Challenges:**
1. **Client-side validation:** The client validates 2H vs dual-wield rules locally. If you spawn a ghost 2H weapon in Primary, the client *should* prevent equipping a shield in Secondary, but this isn't guaranteed without testing.
2. **Visual sync:** The ghost must look identical to the progressable item. Server can copy all visual fields (icon, model, etc.), but any client-side caching might cause issues.
3. **Ghost cleanup:** If the power slot item tiers up (stats change), you must delete the ghost, update it, and re-equip it.

**Recommendation:**
Start with a simpler approach: **just grant the stats without creating a ghost item**. Use server-side stat packets (`SendStats()`) to add the bonus stats directly. The slot appears empty to the client, but the player receives the stats. This avoids all ghost-related edge cases.

If you want the visual ghost for UX reasons, implement it as a Phase 2 feature once the core system works.

---

### ✅ **4. Stat Scaling Formulas**

**Feasibility: 100% — Fully Doable**

**Implementation:**
- Fetch base item stats from database
- Apply multipliers based on tier:
  - `enchanted_stat = base * 2.0`
  - `legendary_combat_stat = base * 2.0 * 1.3` (DMG, AC, HP, Mana, End)
  - `legendary_attr = base * 2.0` (STR, STA, etc. — no increase)
  - `legendary_heroic = base * 1.0`
  - `mythic_stat = legendary_stat` (no change)
- Override item packet fields before sending to client
- Handle special cases (haste, spell power, heal power, attack)

**EQEmu Support:**
- Full control over item stat packets
- Can calculate and apply any formula server-side
- No database schema changes needed (base stats already exist)

**No Client Modification Needed:** Client receives final stats in the item packet.

**Caveats:**
- Ensure heroic stats don't exceed client display limits (shouldn't be an issue)
- Spell Power and Heal Power might need to map to existing `spelldmg` or `healamt` fields depending on client version
- **Manual overrides:** Some items will need hand-tuned stat values that bypass the formula.
  See the parent design doc §12 (Manual Override & Regeneration System) for the
  `item_scaling_overrides` table schema, admin commands (`#override`, `#regenerate`), and
  the `manage_overrides.py` CLI tool. The override lookup adds one DB query per item
  (cacheable) and zero client changes.

---

### ⚠️ **5. Feeding System (Duplicate Items)**

**Feasibility: 90% — Doable with Minor UX Limits**

**Your Design:** Right-click an item in inventory → context menu "Feed to Power Slot" → if item matches Power Slot item, grant 20% XP and destroy.

**Server-Side Implementation:**
- Create a custom `#feed [slot]` command
- Or: Hook quest script on item click (`EVENT_ITEM_CLICK_CAST`)
- Validate item matches Power Slot item (exact ID match)
- Calculate XP grant (20% of current tier requirement)
- Delete fed item
- Award XP to Power Slot tracker (data bucket)

**EQEmu Support:**
- Custom commands trivial to add
- Quest hooks for item interactions exist
- Item deletion and XP tracking both supported

**Challenges:**
- **No right-click context menu modification** without DLL. You'll need to use either:
  - Command: `#feed bag1 slot5`
  - Or: Make a "Feeding Stone" consumable item that, when clicked, prompts "Target an inventory slot to feed"
  - Or: DLL adds the context menu option

**Recommendation:**
Use a command or clickable "Feeding Stone" for Phase 1. Add DLL context menu in Phase 2 for polish.

---

### ⚠️ **6. Salvage Satchel with UI Button**

**Feasibility: 85% — One-Click Salvage Doable Server-Side**

**Your Design:** 20-slot bag where dragging items in and clicking “Salvage All” converts
contents to Essence. **No catalyst required** — salvaging is always a single click.

**Server-Side Implementation:**
- Create a special container item (20 slots) flagged as the Salvage Satchel
- Store Essence as alternate currency (see §8 — EQ’s built-in alt currency system)
- Hook the container’s “combine” action or a custom activation opcode to trigger salvage:
  - Iterates through all 20 slots
  - Rejects non-magic items (magic gate — see ITEM_LEVEL_SYSTEM.md)
  - Calculates Essence yield per magic item: `max(1, iLevel - 100) × tier_bonus × era_mult`
  - Deletes salvaged items and grants Essence
  - Sends feedback message listing total Essence gained and any rejected items
- Alternative trigger: `#salvage` command that operates on the Satchel contents

**EQEmu Support:**
- Creating custom bags: fully supported
- Item iteration in containers: `Client::GetInv()->GetItem(slot)`
- Alternate currency system: `Client::AddAlternateCurrencyValue()`
- Container activation hooks or command-based triggering

**Challenges:**
- Getting a persistent “Salvage All” button inside the bag window requires DLL. A `#salvage`
  command or repurposing the container’s combine button works for Phase 1.

**With DLL (eq-core-dll-main):**
- Add a custom UI button to the bag window
- Send a custom opcode to server when clicked
- Server handles the salvage logic
- Cleanest UX

**Recommendation:**
Phase 1: Use `#salvage` command or repurpose the container combine button. No catalyst.
Phase 2: Add DLL UI button for one-click polish.

---

### ✅ **7. Augment System (Slots, Merging, Levels)**

**Feasibility: 95% — Fully Doable with Minor Limits**

**Implementation:**
- Augments are standard EQ items with augment type restrictions
- Store augment "merge level" in `custom_data` (level 1-5)
- Dynamically scale augment stats based on level when sending packets
- Merge container: create a 4-slot container (3 aug slots + 1 catalyst slot)
- Merge logic triggered by quest script when all 4 slots filled correctly
- Validate: 3 augs are identical name AND level, catalyst tier matches

**EQEmu Support:**
- Augment system already exists in EQEmu
- Aug types and slot restrictions fully supported
- Custom data storage on augments
- Container-based combine systems (tradeskills pattern)

**Challenges:**
- **Aug slot count scaling (1→2→3→4 by tier):** This is tricky. EQEmu items have fixed aug slot counts defined in the database.
  - **Workaround:** Use `ItemInst::SetAugmentSlot()` dynamically when equipping items. When a Base item is equipped, disable slots 2-4. When Enchanted, enable slot 2, etc.
  - Requires hooking item equip to refresh aug slot availability.
  - Client might get confused if it sees 4 slots but server says only 2 are usable.

- **Better solution:** Define items in DB with max aug slots (4 for 1H, 6 for 2H), but server validates insertion based on current tier. Block attempts to socket augs in locked slots.

**No Client Modification Needed** for basic functionality. DLL could improve UI feedback (grayed-out slots).

---

### ✅ **8. Essence Currency Economy**

**Feasibility: 100% — Fully Doable**

**Implementation:**
- Use EQ’s **built-in alternate currency system** to track Common and Rare Essence
  - The client already has a currency display tab (Alt Currency window) that shows
    server-defined currencies with quantities — no DLL needed for basic display
  - Define two new alternate currencies in the `alternate_currency` table
  - Use `Client::AddAlternateCurrencyValue()` / `Client::GetAlternateCurrencyValue()`
    to grant and check balances
- Vendors can natively sell items for alternate currency (merchant lists reference currency ID)
- Salvage system awards Essence to magic items only (non-magic items rejected). Essence =
  max(1, iLevel - 100) × tier bonus × era multiplier. See ITEM_LEVEL_SYSTEM.md for full
  formula and per-era progression server tuning.
- `#essence` command available as a quick balance check

**EQEmu Support:**
- Alternate currency is a first-class EQEmu system with DB tables, client UI, and vendor integration
- No data bucket workaround needed — alt currency is purpose-built for this
- Client shows currency names, icons, and balances in the Alternate Currency tab
- Merchant pricing can reference alt currency directly

**No DLL Required for Currency Display.** The RoF2 client already supports the currency tab.

**Caveats:**
- Alt currency icons need to be mapped to existing item icons in the client files
- Consider adding a `#essence` command too for at-a-glance access without opening the window

---

### ⚠️ **9. Visual Effects (Item Name Colors, Particles)**

**Feasibility: 40% — Limited Server-Side, Full with DLL**

**Your Design:**
- Enchanted items: Blue name
- Legendary items: Gold name
- Mythic items: Orange name
- Visual particles on tier-up

**Server-Side Only:**
- **Item name color:** You can prepend color codes to item names in the packet (`§001001255Enchanted Hategiver` for blue). This works but is hacky and may not render correctly in all clients.
- **Particles:** Cannot create custom particle effects server-side. The client controls all visual rendering.
- **Workaround:** Apply existing item proc visual effects (like "glowing" items with certain spell effects), but this is limited.

**With DLL (eq-core-dll-main):**
- Detect item tier from custom data in item packets
- Render name in appropriate color in inventory/inspect windows
- Trigger particle effects on tier-up events (flash, glow, aura)
- **Full control over visuals**

**Recommendation:**
Phase 1: Skip custom visuals or use hacky name color codes. Phase 2: Implement properly via DLL.

---

### ✅ **10. Drop Tier Chances**

**Feasibility: 100% — Fully Doable**

**Implementation:**
- Hook loot generation (`EVENT_LOOT_ZONE` or custom loot script)
- On item drop, roll RNG for tier:
  - 80% Base, 15% Enchanted, 4% Legendary, 1% Mythic
- Create ItemInstance with appropriate tier stored in custom_data
- Apply stat scaling before adding to corpse loot

**EQEmu Support:**
- Full control over loot table generation
- Quest scripts can modify loot on-the-fly
- RNG functions available (`math.random()` in Lua, `rand()` in Perl)

**No Client Modification Needed.**

---

## Major Implementation Challenges

### 1. **Augment Slot Scaling**

**Problem:** Items have fixed aug slot counts in the database. Dynamically enabling/disabling slots as items tier up is non-trivial.

**Solutions:**
- **Option A (Server Validation):** Define items with max slots (4/6), but server blocks socketiing augs into "locked" slots based on tier. Simple error message: "This item must be Enchanted to use slot 2."
- **Option B (Dynamic Slot Injection):** Modify item packet to show fewer slots, then increase as tier rises. More complex, may confuse client.
- **Option C (DLL):** Client-side validation to gray out locked slots and provide visual feedback.

**Recommended:** Option A for Phase 1, Option C for polish.

---

### 2. **UI/UX Without DLL**

**Problem:** Many quality-of-life features (Salvage button, Essence display, right-click feed) require UI customization.

**Solutions:**
- Use consumable items as "buttons" (Feeding Stone)
- Use chat commands (`#feed`, `#salvage`, `#essence`)
- Use NPC dialogs for vendor interactions
- Accept slightly clunkier UX in exchange for no client dependency

**With DLL:** All of these become polished, integrated UI elements.

**Recommended:** Start server-side only to prove the system works, then layer in DLL features for UX polish.

---

### 3. **Item Stat Recalculation on Tier-Up**

**Problem:** When an item tiers up, its stats change. The client has a cached copy of the item.

**Solutions:**
- Use the **delete + limbo + re-add pattern** you're already familiar with from your item scaling work
- Fire this whenever tier changes (XP threshold met)
- Send feedback message: "Your Hategiver has become Enchanted!"
- Re-equip the item (if it was equipped) to apply new stats

**EQEmu Support:** Already proven pattern in your codebase.

---

### 4. **Preventing Exploits**

**Key Exploit Vectors:**
1. **Power Slot swapping:** Player swaps items in/out of Power Slot rapidly to manipulate XP or projection.
   - **Fix:** Lock Power Slot changes during combat (`InCombat()` check), or add 10-second cooldown.

2. **Ghost item duplication:** If ghost projection has bugs, player might duplicate items.
   - **Fix:** Mark ghosts as `NODROP` and track their "parent" Power Slot item ID. Delete ghost if parent is removed.

3. **Feeding low-level items:** Player farms trivial content for junk items to feed.
   - **Fix:** You already addressed this — only exact duplicates can be fed, others must be salvaged. Essence yields scale with item tier/level.

4. **Aug slot unlocking:** Player tries to socket augs into locked slots.
   - **Fix:** Server validates tier before allowing aug insertion. Block invalid attempts.

**All exploits are preventable with server-side validation.**

---

## Recommended Implementation Phases

### **Phase 1: Core Systems (Server-Side Only)**

**Goal:** Prove the gameplay loop works.

**Features:**
- Item tier progression (Base → Mythic) with stat scaling
- Power Slot XP tracking from mob kills
- Stat projection (simplified: just grant stats, skip ghost items)
- Feeding system via command (`#feed`)
- Salvage system via `#salvage` command (no catalyst needed)
- Essence currency tracking (alt currency system)
- Basic augment merging via container
- Vendor augs and drop system

**Estimated Work:** 3-4 weeks of focused development.

**Validation:** Can you level an item to Legendary and feel the progression? Does salvage → Essence → vendor aug → merge work?

---

### **Phase 2: UX Polish (DLL Integration)**

**Goal:** Make it feel professional.

**Features:**
- Custom UI button for Salvage Satchel
- Essence display in UI (AA window or custom)
- Right-click "Feed to Power Slot" context menu
- Item name color coding (Enchanted = blue, etc.)
- Particle effects on tier-up
- Visual indicators for locked aug slots
- XP bar for Power Slot progression

**Estimated Work:** 1-2 weeks with DLL expertise.

**Validation:** Does it feel like a modern ARPG?

---

### **Phase 3: Tuning & Content**

**Goal:** Balance the numbers and populate the world.

**Features:**
- Adjust XP thresholds and Essence yields based on playtesting
- Create named mob aug drops
- Implement quest chain reward augs
- Add zone-specific flavor augs
- Tune con color multipliers and tier chances
- Build endgame Essence sinks (catalysts, vendor items)

**Estimated Work:** Ongoing.

---

## Final Feasibility Scores

| Feature | Server-Only | With DLL | Notes |
|---|---|---|---|
| Item tier progression | 100% | 100% | Core system fully doable |
| Stat scaling formulas | 100% | 100% | Pure math, trivial |
| Power Slot XP tracking | 100% | 100% | Data buckets + event hooks |
| Stat projection (simple) | 100% | 100% | Just send bonus stats |
| Ghost items (complex projection) | 75% | 95% | Workable but edge cases exist |
| Feeding system (command) | 90% | 100% | DLL adds right-click menu |
| Salvage Satchel (one-click) | 85% | 100% | DLL adds UI button, command works Phase 1 |
| Essence economy | 100% | 100% | EQ alt currency system is purpose-built for this |
| Augment system | 95% | 100% | Slot scaling needs validation logic |
| Augment merging | 100% | 100% | Container combines already exist |
| Drop tier chances | 100% | 100% | Quest script RNG |
| Visual effects (colors/particles) | 40% | 95% | DLL required for clean implementation |
| Duplicate feeding only (anti-exploit) | 100% | 100% | Server validation prevents exploits |

**Overall System Feasibility: 85-95%** depending on how much you leverage the DLL.

---

## Key Recommendations

1. **Start with server-side only.** Prove the core loop (kill mobs → XP → tier up → stronger items) works before adding polish.

2. **Use commands and consumables for UX.** `#feed`, `#salvage`, and clickable catalyst items are "good enough" to test gameplay.

3. **Leverage your DLL after core systems work.** The eq-core-dll-main gives you everything you need for professional UX. Don't tackle client-side and server-side simultaneously — get one working, then enhance.

4. **Simplify stat projection.** Skip ghost items initially. Just grant the stats invisibly. Add ghost visuals later if desired.

5. **Trust your existing patterns.** You've already solved item scaling and packet manipulation. This system is a natural extension of what you've built.

6. **Make everything tunable.** Use config files or database tables for all multipliers, thresholds, and costs. Balance comes from iteration, not perfect initial design.

---

## Technical Gotchas to Watch For

1. **Packet size limits:** If an item has many augments with scaled stats, the item packet could get large. Monitor for overflows.

2. **Client-side validation:** The client will sometimes reject server actions if they violate its rules (like equipping a shield with a 2H weapon). Test ghost projection extensively.

3. **Data bucket performance:** Thousands of players with multiple data buckets (XP, Essence, aug levels) could hit DB performance. Use caching or in-memory structures.

4. **Aug slot type restrictions:** Make sure your aug type definitions (1, 2, 3, 4, 5, 6) align with client expectations. RoF2 has specific slot type behaviors.

5. **Item ID exhaustion:** If you're generating many dynamic items (augs with different merge levels), ensure you're not creating new DB entries for each. Use `custom_data` on ItemInstances, not separate DB items.

---

## Conclusion

**This system is absolutely implementable in EQEmu with server-side code alone.**

The core progression mechanics, stat scaling, currency economy, and augment systems all leverage existing EQEmu capabilities. You'll need to make UX compromises (commands instead of buttons, consumables instead of UI elements), but the gameplay loop will work perfectly.

Your existing DLL integration removes the compromises. With client-side support, you can build a AAA-quality experience that rivals modern ARPGs.

**Start simple, iterate fast, add polish later.** The design is sound. The tech is there. Execute on the core first, then make it beautiful.
