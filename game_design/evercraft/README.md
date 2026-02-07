# EveCraft: Infinite Crafting System

## Quick Overview

EveCraft is a dynamic crafting system inspired by the game "Infinite Craft" that allows EQEmu players to combine **any two items** to create new, contextually appropriate results.

## Documents in This Folder

### [EVERCRAFT_SYSTEM.md](EVERCRAFT_SYSTEM.md)
**Game Design & Balance Document**

Read this first if you're interested in:

**Key sections:**
### [CONTEXTUAL_LOGIC_EXAMPLES.md](CONTEXTUAL_LOGIC_EXAMPLES.md)
**Step-by-Step Walkthrough of Your Examples**

Read this for detailed flows showing:
- "Fire + Fire Armor = Enhanced Fire Armor" (theme matching)
- "Fire + Water = Steam Clouds" (opposing elements neutralization)
- "Dragon Helm + Dragon Leather = Dragon Helm" (theme + material)
- Each example traced through the entire combination engine
- How to add new templates

**Key sections:**
- Example 1: Fire + Armor (Theme Matching)
- Example 2: Fire + Water (Opposing Elements)
- Example 3: Dragon Helm + Dragon Leather (Material Infusion)
- The Key Insight: Why These Make Sense
- How to Add New Templates

### [TECHNICAL_IMPLEMENTATION.md](TECHNICAL_IMPLEMENTATION.md)
**Server Implementation & Code Architecture**

Read this if you're implementing or modifying:
- GM commands and debugging

- Dynamic Item Generation Algorithm
- Integration Checklist

---

## Is This Feasible?

**YES.** EveCraft is entirely feasible within EQEmu constraints:

### ✅ Server-Side Only
- No external APIs needed
- No client modifications required
- No new opcodes or packet structures
- Uses only existing EQEmu infrastructure

### ✅ Leverages Existing Systems
- **Tradeskill system** for combine detection and skill tracking
- **Item instance system** for stat storage and custom data
- **Data buckets** for result caching and determinism
- **Inventory system** for item management
- **Lua/Perl** for optional quest hooks and customization

### ✅ No New Database Tables
- Result caching uses `data_buckets` table
- Dynamic items stored in existing `items` table
- Character history tracked in `data_buckets` (optional)
- Item type classification uses existing `ItemClass` field

### ✅ Minimal Performance Impact
- In-memory cache: ~1ms lookup per combo
- No database hits for cached results
- Expected throughput: 100+ combines/second

---

## High-Level Implementation Approach

### 1. **Interception** (zone/tradeskills.cpp)
- Hook into existing `HandleCombine()` function
- Check if EveCraft is enabled and items are valid
- If valid, call `EveCraft::ProcessCombine()` instead of recipe lookup

### 2. **Deterministic Result Computation** (zone/evercraft.cpp)
- Hash the two item IDs + skill level
- Check data bucket cache for previously computed result
- If not cached, search database for matching item or generate new one
- Cache result for future reuse

### 3. **Skill-Based Execution** (zone/evercraft.cpp)
- Compute success chance from skill level
- Roll for success/failure
- Apply salvage on failure
- Handle skill increases via existing system

### 4. **Dynamic Item Creation** (zone/evercraft.cpp)
- Blend stats from both input items
- Generate contextual name
- Apply tier-based multipliers
- Store in items table (new entry)

---

## What Makes This Different from Traditional Crafting?

| Aspect | Traditional EQ | EveCraft |
|--------|---|---|
| **Discovery** | Recipes must be found/learned | Player experimentation |
| **Combinations** | Hardcoded recipes | Infinite possibilities |
| **Item Results** | Pre-existing only | Generated on-demand |
| **Skill Impact** | Binary (can/can't) | Continuous scaling (quality) |
| **Player Agency** | Follow recipe | Explore & create |

---

## Example Progression

### Scenario: Crafting a Blade from a Knife and Wolf Fur

**Skill 1 (Beginner):**
```
Input: Knife + Wolf Hide
→ 40% success chance
→ Result: "Roughly Wrapped Wolf Fur"
  (Attack: 20, AC: -5)
→ On failure: Lose wolf hide, keep knife
```

**Skill 100 (Journeyman):**
```
Input: Knife + Wolf Hide
→ 80% success chance
→ Result: "Leather Wrapped Blade"
  (Attack: 35, AC: -10)
→ On failure: Lose both items
```

**Skill 250 (Master):**
```
Input: Knife + Wolf Hide
→ 99% success chance
→ Result: "Masterwork Wolf-Skin Blade"
  (Attack: 50, AC: -15, +5% crit chance)
→ Unique variant (5% chance): "Legendary Blade of the Moonlit Hunt"
  (Attack: 50, AC: -15, +10% crit chance, unique name)
```

---

## Risk Assessment

### Low Risk ✅
- Entirely opt-in (rules-based toggle)
- No changes to existing recipe system (parallel implementation)
- Reversible (disable rule to turn off)
- Separate namespace avoids code conflicts

### Medium Risk ⚠️
- Economy balancing (need to tune stat generation)
- Player expectations (need clear documentation)
- Item database pollution (dynamic items table grows)

### Mitigation
- Start with limited subset of items (flag "evercraft_enabled")
- Tunable rules for success rates and stat scaling
- Regular monitoring of economy impact
- Eventual cleanup tool for unused dynamic items

---

## Future Enhancements (Out of Scope v1)

1. **Group crafting:** 3+ items for special results
2. **NPC recipes:** NPCs unlock special combination types
3. **Transmutation:** Transform existing items without combining
4. **Seasonal events:** Limited-time combinations
5. **PvP crafting:** Player-requested customs
6. **Crafting guilds:** Group achievements and shared recipes

---

## Getting Started

### To Understand the System
1. Read [EVERCRAFT_SYSTEM.md](EVERCRAFT_SYSTEM.md) for design overview
2. Review skill progression and balance notes
3. Check prohibited/allowed combinations

### To Implement the System
1. Read [TECHNICAL_IMPLEMENTATION.md](TECHNICAL_IMPLEMENTATION.md)
2. Follow the "Integration Checklist"
3. Create `zone/evercraft.cpp` and `zone/evercraft.h`
4. Modify `zone/tradeskills.cpp` per instructions
5. Add rules to `rules.cpp`
6. Compile and test with GM commands

---

## Questions?

Refer to the detailed documents:
- **"How does it work?"** → [EVERCRAFT_SYSTEM.md](EVERCRAFT_SYSTEM.md)
- **"How do I code it?"** → [TECHNICAL_IMPLEMENTATION.md](TECHNICAL_IMPLEMENTATION.md)
- **"What's the formula?"** → Dynamic Item Generation Algorithm in technical doc
- **"Is it balanced?"** → Game Balance Considerations in system doc

---

## Version History

- **v1.0** (2025-02-06): Initial design & architecture documentation
