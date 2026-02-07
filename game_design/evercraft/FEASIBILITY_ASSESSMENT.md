# EveCraft Implementation Feasibility & Technical Assessment

**Date:** February 6, 2025
**Status:** FEASIBLE ✅
**Scope:** Server-side only, no external APIs

---

## Executive Summary

**EveCraft—a dynamic infinite crafting system** is **entirely feasible** within the constraints of an EQEmu server using the RoF2 client.

### Key Findings

✅ **No external APIs needed**
- All combination logic runs on the server
- Deterministic results (hash-based, not random)
- No AI/ML calls required

✅ **Uses existing EQEmu infrastructure**
- Trades on established tradeskill system
- Leverages data_buckets for result caching
- Reuses item instance and inventory systems
- Compatible with Lua/Perl scripting

✅ **No client modifications**
- Uses existing combine packets
- No new opcodes
- No DLL patches needed

✅ **Minimal performance impact**
- In-memory caching (~1ms per lookup)
- Expected throughput: 100+ combines/second
- ~10KB per cached combination

✅ **Fully optional & reversible**
- Single rule toggle to enable/disable
- Parallel to existing recipe system (no conflicts)
- Can be turned off anytime

---

## What Is EveCraft?

A **player-driven crafting system** where:

1. **Any two items can combine** → generates contextually appropriate result
2. **Results scale with skill** → higher skill = better quality, unique variants
3. **Results are deterministic** → same inputs + skill = same output always
4. **Results are cached** → no recomputation on repeat combines
5. **Items auto-generate** → new items created on-demand, stored in items table

### Example

```
Player combines: Knife + Wolf Hide

Skill 1:    → "Roughly Wrapped Wolf Fur" (40% success, 20 attack)
Skill 100:  → "Leather Wrapped Blade" (80% success, 35 attack)
Skill 250:  → "Masterwork Wolf-Skin Blade" (99% success, 50 attack)
            OR Unique: "Legendary Blade of the Moonlit Hunt" (5% chance)
```

---

## Technical Feasibility Assessment

### Architecture: Parallel to Existing Crafting

```
Player attempts combine
    ↓
Check: Recipe exists? (existing system)
    ├─ YES → Use recipe (existing code path)
    └─ NO ↓
        Check: EveCraft enabled? (new rule)
            ├─ NO → Fail (existing behavior)
            └─ YES → EveCraft::ProcessCombine() (new code path)
```

**Result:** Zero disruption to existing recipe system.

### Data Storage: No New Tables Required

| Data | Storage | Notes |
|------|---------|-------|
| Result cache | `data_buckets` table | Key: `evercraft_combo_{item1}_{item2}` |
| Dynamic items | `items` table | Normal items table, auto-inserted |
| Player history | `data_buckets` table | Optional, per-character tracking |

**Existing infrastructure handles all data needs.**

### Code Integration: Minimal Changes

**Files to modify:**
- `zone/tradeskills.cpp` — Add check before recipe lookup (~10 lines)
- `rules.cpp` — Add EveCraft configuration rules (~8 new rules)
- `CMakeLists.txt` — Include new `.cpp` file (~1 line)

**Files to create:**
- `zone/evercraft.h` — Header (~150 lines of declarations)
- `zone/evercraft.cpp` — Implementation (~500 lines of logic)

**Total new code:** ~750 lines across 2 new files + 20 lines of modifications.

### Performance: Negligible Impact

- **Cache lookups:** ~1ms per combo (in-memory JSON parse)
- **Cache hit rate:** Expected >90% after warmup
- **Database writes:** Only for new dynamic items (~50ms, rare)
- **Throughput:** 100+ combines/second per zone (easily sustained)
- **Memory:** 10,000 cached combos = ~100 MB (negligible for modern servers)

---

## What Exists Today (EQEmu Infrastructure)

### ✅ Tradeskill System
```cpp
zone/tradeskills.cpp::HandleCombine()
  └─ Detects combine attempts
  └─ Manages item removal and result creation
  └─ Calls skill increase logic
```

**Reused by EveCraft:** Item removal, result summoning, skill increases.

### ✅ Item System
```cpp
common/item_instance.h
  └─ Represents item instances
  └─ Stores custom data (JSON-like)
  └─ Maintains augments, charges, dyes
```

**Reused by EveCraft:** Custom data for tracking recipe discovery.

### ✅ Data Buckets
```cpp
common/data_bucket.h
  └─ Persistent key-value storage
  └─ Character/zone/account scoped
  └─ JSON support, TTL expiration
```

**Reused by EveCraft:** Cache combination results, track player history.

### ✅ Skill System
```cpp
zone/client.h::GetSkill(), CheckIncreaseTradeskill()
  └─ Tracks skill levels (0–300)
  └─ Computes success rates
  └─ Handles skill increases
```

**Reused by EveCraft:** Skill-based success rates and scaling.

### ✅ Inventory Management
```cpp
common/inventory_profile.h
  └─ Get/put items
  └─ Delete items
  └─ Iterate slots
```

**Reused by EveCraft:** Item removal and creation.

---

## Implementation Strategy

### Phase 1: Core Logic (1-2 weeks)
1. Create `zone/evercraft.h` and `.cpp` with stub functions
2. Implement `DetermineCombination()` with caching
3. Add rule toggles to `rules.cpp`
4. Integrate into `HandleCombine()`
5. GM commands for testing

### Phase 2: Stat Generation (1 week)
1. Implement `GenerateResultItem()` stat blending
2. Implement `GenerateItemName()` contextual naming
3. Test stat scaling across skill tiers
4. Tune balance parameters

### Phase 3: Quality Assurance (1 week)
1. Unit tests for determinism
2. Integration tests with inventory
3. Performance profiling
4. Balance review

### Phase 4: Live Deployment (1 week)
1. Alpha test on internal server
2. Beta test on single zone
3. Gather feedback
4. Global rollout

**Total: ~4 weeks** (modest effort for major feature).

---

## What Cannot Be Done (Constraints)

❌ **Real-time AI analysis** → Not feasible without external API
   *Alternative:* Hardcoded combination templates + contextual logic

❌ **LLM-generated descriptions** → Would require external service
   *Alternative:* Template-based naming (e.g., "{quality}_{type}_{material}")

❌ **True randomness** → Breaks determinism
   *Alternative:* Deterministic hashing of inputs (repeatable results)

**None of these are required.** The system works perfectly with server-side determinism.

---

## Risk Mitigation

### Economy Risk
- **Concern:** Infinite combinations could devalue existing items/recipes
- **Mitigation:**
  - Make EveCraft optional per-zone
  - Tune stat generation to avoid power creep
  - Create lore groups preventing certain combos
  - Monitor via event logs

### Player Experience Risk
- **Concern:** Overwhelming number of combinations
- **Mitigation:**
  - Start with limited item types (whitelist)
  - Provide NPC hints for popular combos (future)
  - Community voting on discoverable recipes (future)

### Data Risk
- **Concern:** Items table pollution with thousands of auto-generated items
- **Mitigation:**
  - Version number in rules (invalidates old generated items)
  - Cleanup script to remove unused dynamic items
  - Separate "source" items (player-created) via naming convention

---

## Recommended Configuration (Live)

```cpp
// rules.cpp defaults

{ "EveCraft:Enabled", false, "Enable EveCraft (default OFF for safety)" },
{ "EveCraft:SkillMin", 1, "Minimum crafting skill to use EveCraft" },
{ "EveCraft:SkillMax", 300, "Maximum skill cap" },
{ "EveCraft:SuccessCurve", 2.0, "Difficulty scaling (higher = harder)" },
{ "EveCraft:AllowNoDrop", false, "Can combine no-drop items? (NO for safety)" },
{ "EveCraft:CacheResults", true, "Cache results in data buckets?" },
{ "EveCraft:Variant", 1, "Result formula version (bump to invalidate cache)" },
```

**Start conservative.** Enable per-zone after balance review.

---

## Comparison to Similar Systems

### Infinite Craft (Mobile Game)
- ✅ Any item + item → new item
- ✅ Deterministic results
- ❌ No skill progression (EveCraft adds this)
- ❌ No failure states (EveCraft includes)

### EQ Transmog System (hypothetical)
- ✅ Player-driven item creation
- ❌ Requires external scripting
- ❌ Not self-contained on server

### EveCraft (This System)
- ✅ All features of Infinite Craft
- ✅ Skill progression like traditional EQ crafting
- ✅ Failure/salvage mechanics
- ✅ Server-contained (no external deps)
- ✅ Fully optional toggle

---

## Conclusion

**EveCraft is fully feasible** as a server-side crafting system for EQEmu. It:

- Requires **no external APIs** (all logic server-side)
- Uses **existing EQEmu infrastructure** (zero new tables)
- Adds **minimal code** (~750 lines, ~20 line modifications)
- Has **negligible performance impact** (<1ms per lookup)
- Is **fully optional** (single rule toggle)
- Is **backward compatible** (existing recipes unchanged)
- Provides **significant player engagement** (discovery-driven gameplay)

### Next Steps

1. **Review design:** Read [game_design/evercraft/EVERCRAFT_SYSTEM.md](game_design/evercraft/EVERCRAFT_SYSTEM.md)
2. **Review technical:** Read [game_design/evercraft/TECHNICAL_IMPLEMENTATION.md](game_design/evercraft/TECHNICAL_IMPLEMENTATION.md)
3. **Implement Phase 1:** Create `zone/evercraft.cpp` with stubs
4. **Test internally:** Run on dev server
5. **Gather feedback:** Iterate on balance
6. **Deploy:** Roll out per-zone

---

## Documentation Location

All documents are in: **`game_design/evercraft/`**

- **README.md** — Quick overview
- **EVERCRAFT_SYSTEM.md** — Game design & balance
- **TECHNICAL_IMPLEMENTATION.md** — Code architecture & integration guide
