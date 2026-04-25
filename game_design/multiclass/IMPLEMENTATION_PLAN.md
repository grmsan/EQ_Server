# Multiclass (Gestalt) System - Master Implementation Plan

**Version:** 1.1
**Last Updated:** 2026-04-21
**Status:** Active Development

This document serves as the authoritative technical design and implementation guide for the multiclass (gestalt) system. It consolidates requirements, architecture decisions, and work priorities into a single reference.

---

## Table of Contents
1. [Executive Summary](#executive-summary)
2. [System Architecture](#system-architecture)
3. [Server Implementation](#server-implementation)
4. [Client DLL Integration](#client-dll-integration)
5. [Data Model](#data-model)
6. [Implementation Phases](#implementation-phases)
7. [Testing Strategy](#testing-strategy)
8. [Known Issues & Solutions](#known-issues--solutions)
9. [File Reference](#file-reference)

---

## Executive Summary

### Goals
Allow a single character to simultaneously hold up to 3 classes (configurable) with:
- Shared level, inventory, AA pool, spellbook, and skill table
- Union-of-classes access to equipment, spells, AAs, skills, and disciplines
- Server-authoritative gameplay rules (server validates; client displays)
- Stock RoF2 client compatibility via custom DLL hooks

### Key Design Principles
| Principle | Description |
|-----------|-------------|
| **Server Authoritative** | All gameplay effects validate against `GetClassesBits()`, not `GetClass()` |
| **Union-of-Classes** | Class eligibility checks consider ALL owned classes |
| **Soft-Lock Persistence** | Learned spells/AAs/skills remain stored; entitlement checked at use-time |
| **Minimal Client Changes** | DLL for UI visibility only; no mandatory client patches |

### Reference Implementation
This system is based on **THJServer** (The Heroes Journey), which has a proven multiclass implementation:
- THJServer code: `extras/THJServer/`
- THJServer docs: `extras/THJServer/docs/multiclass.md`, `extras/THJServer/docs/database_examples.md`
- Client DLL: `extras/eq-core-dll-main/`

---

## System Architecture

### Component Overview
```
┌─────────────────────────────────────────────────────────────────────┐
│                         RoF2 Client                                  │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │  dinput8.dll (eq-core-dll-main)                             │    │
│  │  - Parses EdgeStatLabel (0x1338) for classes_bitmask        │    │
│  │  - Hooks GetUsableClasses() for UI filtering                │    │
│  │  - Hooks GetSpellLevelNeeded() for merchant/spellbook       │    │
│  │  - Hooks GetPcSkillLimit() for skills window                │    │
│  │  - Overrides class name display (/who, char select)         │    │
│  └─────────────────────────────────────────────────────────────┘    │
└──────────────────────────────────┬──────────────────────────────────┘
                                   │ Network (RoF2 opcodes + custom)
┌──────────────────────────────────┴──────────────────────────────────┐
│                         EQEmu Server                                 │
│  ┌────────────────┐  ┌────────────────┐  ┌────────────────────────┐ │
│  │  world.exe     │  │  zone.exe      │  │  shared_memory.exe     │ │
│  │  - /who output │  │  - GetClassesBits()                        │ │
│  │  - char list   │  │  - HasClass()                              │ │
│  │                │  │  - AddExtraClass()                         │ │
│  │                │  │  - SendEdgeStats()                         │ │
│  │                │  │  - All gameplay gating                     │ │
│  └────────────────┘  └────────────────┘  └────────────────────────┘ │
│                              │                                       │
│  ┌───────────────────────────┴───────────────────────────────────┐  │
│  │  Database (MariaDB/MySQL)                                      │  │
│  │  - character_data.class (primary class)                        │  │
│  │  - data_buckets.GestaltClasses (bitmask)                       │  │
│  └────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

### Class Bitmask Format
Classes are stored as a 16-bit bitmask where bit position = (class_id - 1):
```cpp
// From common/classes.h
static std::map<uint8, uint16> player_class_bitmasks = {
    {Class::Warrior,      1},     // bit 0  = 0x0001
    {Class::Cleric,       2},     // bit 1  = 0x0002
    {Class::Paladin,      4},     // bit 2  = 0x0004
    {Class::Ranger,       8},     // bit 3  = 0x0008
    {Class::ShadowKnight, 16},    // bit 4  = 0x0010
    {Class::Druid,        32},    // bit 5  = 0x0020
    {Class::Monk,         64},    // bit 6  = 0x0040
    {Class::Bard,         128},   // bit 7  = 0x0080
    {Class::Rogue,        256},   // bit 8  = 0x0100
    {Class::Shaman,       512},   // bit 9  = 0x0200
    {Class::Necromancer,  1024},  // bit 10 = 0x0400
    {Class::Wizard,       2048},  // bit 11 = 0x0800
    {Class::Magician,     4096},  // bit 12 = 0x1000
    {Class::Enchanter,    8192},  // bit 13 = 0x2000
    {Class::Beastlord,    16384}, // bit 14 = 0x4000
    {Class::Berserker,    32768}, // bit 15 = 0x8000
};

// Example: Warrior + Ranger + Mage = 1 + 8 + 4096 = 4105 (0x1009)
```

---

## Server Implementation

### Core API (zone/client.h, zone/client.cpp)

```cpp
// Already implemented:
uint32 GetClassesBits() const;      // Returns union bitmask (all owned classes)
uint16 GetClassesBitmask() const;   // 16-bit version for packets
bool SetClassesBits(uint32 bits);   // Update bitmask + sync to client
uint8 GetClassesCount();            // Count of set bits
bool HasClass(uint8 class_id) const;// Check if class_id bit is set
bool AddExtraClass(uint8 class_id); // Add class + refresh UI/skills
bool RemoveExtraClass(uint8 class_id); // Remove class + soft-lock abilities

// EdgeStatLabel integration:
void SendEdgeStats();               // Pushes classes_bitmask (key 200) to client DLL
```

### Rules Configuration (common/ruletypes.h)
```cpp
RULE_CATEGORY(Custom)
RULE_BOOL(Custom, MulticlassingEnabled, true, "Enable multiclass foundations")
RULE_INT(Custom, MulticlassMaxClasses, 3, "Maximum classes allowed (includes base)")
RULE_STRING(Custom, MulticlassBucketKey, "GestaltClasses", "Data bucket key for class bitmask")
RULE_BOOL(Custom, ServerAuthStats, true, "Server-authoritative stats for DLL integration")
RULE_BOOL(Custom, UseDynamicAATimers, true, "Dynamic AA timers (deconflict multiclass)")
RULE_BOOL(Custom, MulticlassDebug, false, "Debug logging")
RULE_CATEGORY_END()
```

### Systems Requiring Union-of-Classes Checks

| System | File | Check Pattern | Status |
|--------|------|---------------|--------|
| Spells | zone/spells.cpp | Memorize, Cast, Level requirements | Complete |
| Bard Pulse | zone/spell_effects.cpp | Custom `ApplyBardPulse()` hook in `DoBuffTic` | Complete |
| Infinite Buffs | zone/spell_effects.cpp | Bypass tick decrement for non-detrimental spells | Complete |
| AAs | zone/aa.cpp | Purchase, Activate, Display, Passive bonuses | Implemented; needs manual validation and class-removal entitlement decision |
| Skills | zone/client_mods.cpp, DLL | MaxSkill(), GetRawSkillCap(), Skills window exposure | Implemented; validate K-05 |
| Items | zone/inventory.cpp | CanEquip(), Class mask, Augment insert | Implemented; validate I-01 to I-07 |
| Disciplines | zone/effects.cpp | UseDiscipline(), timer families | Implemented; validate K-03 and S-07 |
| Merchants | zone/client_packet.cpp, zone/client_process.cpp | Spell/item vendor filtering | Implemented; validate I-06 and spell vendor smoke |
| Trainers | zone/client_packet.cpp, zone/client_process.cpp | Skill trainers | Implemented; validate K-02 |
| Pets | zone/pets.cpp and mechanics tracker | Pet command/persistence parity | Tracked under mechanics; validate MECH-07 to MECH-15 after pet changes |

### Canonical Check Patterns
```cpp
// AA eligibility (zone/aa.cpp): current code should gate on owned classes.
if ((ability->classes >> 1) & GetClassesBits() || (ability->classes & (1 << GetClass()))) {
    // Character can use this AA
}

// Item class check: use owned-class bitmask, not only compatibility class.
if (item->Classes & GetClassesBits()) {
    // Character can equip/use the item if other restrictions pass
}

// Spell level check: loop through owned classes and use the best qualifying level.
```

---

## Client DLL Integration

### DLL Location
- Source: `extras/eq-core-dll-main/src/`
- Output: `dinput8.dll` (placed in RoF2 client directory)
- Build: VS2022, x86/Win32, Release

### Key DLL Options (_options.h)
```cpp
bool isServerAuthoritativeStatsEnabled = true;        // Enable EdgeStatLabel parsing
bool isMulticlassUsableClassesOverrideEnabled = true; // Override GetUsableClasses
bool isMulticlassSpellUiOverrideEnabled = true;       // Spellbook/mana UI gating
bool isMulticlassClassNameOverrideEnabled = true;     // /who and char select labels
bool isDebugLoggingEnabled = true;                    // dinput8_debug.log output
```

### EdgeStatLabel Protocol (Opcode 0x1338)
Server sends stat updates to DLL via custom opcode:
```cpp
// Server side (zone/client.cpp::SendEdgeStats)
struct EdgeStatEntry {
    uint32_t key;
    uint64_t value;
};
// Key 200 = classes_bitmask (uint16 cast to uint64)

// DLL side (eqgame.cpp::ApplyEdgeStatLabelPacket)
constexpr uint32_t kClassesBitmask = 200;
g_serverUsableClassesMask = static_cast<uint32_t>(g_edgeStatValue[kClassesBitmask] & 0xFFFF);
```

### DLL Detours (eqgame.cpp)

| Function | Purpose | Effect |
|----------|---------|--------|
| `EQCharacter_GetUsableClasses` | Returns usable class mask | Returns `g_serverUsableClassesMask` instead of base class |
| `EQSpell_GetSpellLevelNeeded` | Spell required level | Returns min level across owned classes |
| `PcZoneClient_GetPcSkillLimit` | Skill cap | Exposes server-granted skills even if base class cap=0 |

### Debug Output
- File: `<RoF2 client dir>/dinput8_debug.log`
- Captures: EdgeStatLabel parsing, detour invocations, multiclass state

---

## Data Model

### Database Tables

**character_data** (existing table)
```sql
-- Single owned class used for stock packet/client compatibility
`class` TINYINT UNSIGNED NOT NULL DEFAULT 0  -- Compatibility class ID (1-16)
```

**data_buckets** (existing table)
```sql
-- Multiclass bitmask stored here
INSERT INTO data_buckets (`key`, `value`, `character_id`) VALUES
    ('GestaltClasses', '4105', 12345);  -- Warrior(1) + Ranger(8) + Mage(4096)
```

### Character Load Flow
1. `zone/client_packet.cpp` loads character.
2. `character_data.class` hydrates the legacy compatibility class slot.
3. Query `data_buckets` for `GestaltClasses` key.
4. Parse value into the authoritative owned-class bitmask cache and profile class mask.
5. `GetClassesBits()` returns the persisted owned-class bitmask; it no longer silently ORs the legacy starting class back in.
6. If a compatibility class is needed for stock packets, pick a deterministic class from the owned bitmask.

### Character Save Flow
1. `SetClassesBits()` updates `m_classes_bits_cache`
2. Writes to `data_buckets` via `SetBucket("GestaltClasses", ...)`
3. `SendEdgeStats()` pushes update to client DLL

---

## Implementation Phases

### Phase 1: Foundation & Persistence (Implemented; Validate)
- [x] Rules in common/ruletypes.h
- [x] GetClassesBits() / SetClassesBits() / HasClass() API
- [x] AddExtraClass() / RemoveExtraClass()
- [x] GM commands: #addclass, #removeclass, #multiclassdiag
- [x] Data bucket persistence (GestaltClasses)
- [x] EdgeStatLabel integration (SendEdgeStats)
- [x] Character creation seeds GestaltClasses bucket
- [x] Starting/base class can be removed as long as at least one owned class remains
- [ ] Validate C-01, C-02, C-06 after fresh rebuild

### Phase 2: Spell System, Mana UI, and Class Removal (Implemented; Validate)
- [x] Memorize spell checks union-of-classes (Verified OP_MemorizeSpell logic)
- [x] Cast spell checks union-of-classes (Verified CheckFizzle logic)
- [x] Spell merchant "Show Usable Items" filter (Verified classes_required check)
- [x] Spell tooltips show correct class levels (Handled by DLL)
- [x] Spellbook UI shows all usable spells (Handled by DLL)
- [x] Spell scribe validates class ownership (Verified client_process.cpp logic)
- [x] Server and DLL mana refresh path are implemented for owned caster classes
- [x] Class removal clears invalid memorized gems and interrupts invalid casts while preserving scribed spells
- [ ] Validate C-03, S-01 to S-07, and E-01

### Phase 3: AA System (Implemented; Validate + Decide Policy)
- [x] AA purchase checks union-of-classes
- [x] AA activation checks union-of-classes
- [x] AA window displays AAs for all owned classes (Server side mask sending implemented)
- [x] Dynamic AA timers (UseDynamicAATimers rule)
- [x] Passive AA effects gated by current ownership
- [x] Mnemonic Retention and Fury of Magic multiclass gates have automation prechecks
- [ ] Validate A-01 to A-05 in the live AA window
- [ ] Decide E-02: removed-class AA policy should be soft-lock or refund/reset before release

### Phase 4: Skills, Training, and Disciplines (Implemented; Validate)
- [x] MaxSkill() uses best owned class cap
- [x] Skill trainers allow training for any owned class
- [x] Skills window exposure path is implemented through DLL skill-limit override
- [x] Skill use validates class ownership at runtime (via CheckIncreaseSkill)
- [x] Discipline tome learning and activation use multiclass-aware checks
- [ ] Validate K-01 to K-05 and S-07

### Phase 5: Items, Equipment, Merchants, and Augments (Implemented; Validate)
- [x] Item class mask checks union-of-classes in core equip/click paths
- [x] Merchant item filtering uses union-of-classes
- [x] Equip validation for class-restricted items
- [x] Item click/equip-cast restrictions use `GetClassesBits()`
- [x] Augment validation for class-restricted augs validates owned classes and preserves THJ wear-slot safety guard
- [ ] Validate I-01 to I-07, including deny paths and race restrictions

### Phase 6: Combat and Class Gates (Implemented; Validate)
- [x] Combat ability class restrictions (Refactored OPCombatAbility checks)
- [x] Damage Caps (Refactored DoDamageCaps in zone/attack.cpp)
- [x] Bard Cast-While-Attacking (Refactored zone/attack.cpp)
- [x] Ranger/Berserker combat class gates use `HasClass`
- [ ] Validate P-13; broader pet/proc parity is tracked in mechanics

### Phase 7: Experience, Scaling, and Presentation (Implemented; Validate)
- [x] Hybrid penalties/bonuses (Refactored zone/exp.cpp)
- [x] Level Cap / Exp Cap logic (Restored THJ-style `MaxExpLevel` clamp so client max level no longer overrides the server exp cap; verify with X-04)
- [x] Guild roster sync on class mutation / level update (`AddExtraClass`, `RemoveExtraClass`, and the level-up guild refresh now publish `GetClassesBits()` when multiclassing is enabled, and guild member updates now force a members-list refresh; verify with G-01)
- [x] Character select multiclass display path is implemented via world + DLL shaping
- [x] `/who` multiclass abbreviations are implemented via world + DLL class-name overrides
- [x] Inventory window rewrites `IW_Class` and `IW_ClassAbbr` from the multiclass mask
- [x] #mystats shows multiclass info
- [ ] Validate X-04, D-01 to D-04, B-08, and G-01

### Phase 8: Roadmap Decisions Before Release
- [ ] E-02: choose removed-class AA entitlement policy.
- [ ] Decide whether AA level requirements should be removed globally, rule-gated, or left as-is.
- [ ] Decide whether player-facing `/pet assist` command/UI parity is required or whether internal assist behavior is sufficient.
- [ ] Expand automation for high-value manual cases after the runtime validation pass identifies stable repro flows.

---

## Testing Strategy

### Smoke Test Sequence (After Each Change)
```
1. Start client with fresh DLL (copy dinput8.dll, restart client)
2. Restart server (zone.exe, world.exe)
3. Log in, run #multiclassdiag
4. #addclass <id> - verify bitmask updates
5. Check dinput8_debug.log for EdgeStatLabel receipt
6. Test specific system (spell/AA/skill)
7. #removeclass <id> - verify soft-lock behavior
```

### Test Matrix

| Test | Setup | Expected |
|------|-------|----------|
| T1: Warrior + Ranger | #addclass 4 | Spellbook opens, mana bar appears, ranger spells usable |
| T2: Warrior + Cleric | #addclass 2 | Cleric spells/AAs accessible |
| T3: Ranger + Mage | #addclass 13 | Mage spells in vendor, spell memorize works |
| T4: Add/Remove churn | Multiple add/remove | No stuck UI, bitmask consistent |
| T5: Relog persistence | Relog after #addclass | GestaltClasses bucket persists |

### Diagnostic Commands
```
#multiclassdiag          - Full multiclass state dump
#multiclassdiag refresh  - Force SendEdgeStats()
#addclass list           - Show all classes with ownership status
#mystats                 - Shows classes_bitmask in output
```

---

## Known Issues & Solutions

### Issue: Spell merchant shows wizard spells for non-wizard
**Cause:** DLL GetUsableClasses was returning multiclass mask for spell filter RVAs
**Solution:** Removed spell filter RVAs from GetUsableClasses whitelist; spell filtering uses GetSpellLevelNeeded returning 255
**Status:** FIXED 2026-01-31

### Issue: AA window missing AAs for added classes
**Cause:** AA list built at zone-in, not refreshed on class change
**Solution:** Call `SendAlternateAdvancementTable()` after class change
**Status:** Implemented; validate A-01 to A-05 in the live AA window

### Issue: Skills for added classes not visible
**Cause:** Client skill caps from base class only
**Solution:** DLL `PcZoneClient_GetPcSkillLimit_Detour` exposes server-granted skills
**Status:** Implemented; validate K-05

### Issue: Spell memorize hangs (progress bar stuck)
**Cause:** Server rejects but client UI not reset
**Solution:** Invalid post-removal re-memorize attempts now fail immediately and reset the spellbar UI
**Status:** Implemented; validate E-01

### Issue: /who shows only base class
**Cause:** Usually stale `world.exe` or DLL build, not missing server logic
**Solution:** Rebuild `world.exe` and refresh the client DLL so the world packet shaping and class-name detours are in sync
**Status:** Implemented, verify with fresh build

---

## File Reference

### Server Files (Validation Hotspots)
| File | Purpose | Current Focus |
|------|---------|------------|
| zone/client.cpp | Core multiclass API | Validate class mutation, persistence, spell cleanup, and EdgeStat sync |
| zone/client.h | Client class declarations | Keep helper contracts aligned with implementation |
| zone/aa.cpp | AA filtering and activation | Validate A-01 to A-05 and E-02 behavior |
| zone/spells.cpp | Spell casting logic | Validate S-01 to S-07 and E-01 |
| zone/client_packet.cpp | Packet handlers | Validate profile hydration, memorize flow, merchants, and augments |
| zone/client_mods.cpp | Stat calculations | Validate C-03 and K-05 |
| zone/bonuses.cpp | Bonus application | Validate passive AA ownership gating |
| common/ruletypes.h | Rules definitions | Confirm rules are set correctly before test runs |

### DLL Files
| File | Purpose |
|------|---------|
| extras/eq-core-dll-main/src/eqgame.cpp | Main detours and EdgeStatLabel |
| extras/eq-core-dll-main/src/_options.h | Feature toggles |
| extras/eq-core-dll-main/src/EQClasses.h | EQ class structures |

### Documentation
| File | Purpose |
|------|---------|
| game_design/multiclass/README.md | Overview and quick links |
| game_design/multiclass/IMPLEMENTATION_PLAN.md | This document (master technical reference) |
| game_design/multiclass/TEST_TRACKER.md | Bug tracking and test checklist |
| game_design/multiclass/PORT_CHECKLIST.md | File-by-file THJ port status |
| game_design/multiclass/DLL_INTEGRATION.md | Client DLL build and debug guide |
| game_design/multiclass/QUICK_START.md | 15-minute setup guide |

---

## Quick Start: Getting Multiclass Running Locally

### Prerequisites
1. EQEmu server built and running (see BUILD.md)
2. RoF2 client installed
3. Visual Studio 2022 (for DLL build)
4. MariaDB/MySQL database

### Steps
1. **Build Server**
   ```powershell
   cmake -S . -B build
   cmake --build build --config RelWithDebInfo --parallel
   ```

2. **Build DLL**
   ```powershell
   # Use VS2022 task or:
   cd extras/eq-core-dll-main
   msbuild eq-core-dll-visualstudio2022.sln /p:Configuration=Release /p:Platform=Win32
   ```

3. **Install DLL**
   - Copy `extras/eq-core-dll-main/bin/dinput8.dll` to RoF2 client folder
   - Restart client after any DLL update

4. **Enable Rules** (in-game or DB)
   ```sql
   INSERT INTO rule_values (ruleset_id, rule_name, rule_value) VALUES
       (1, 'Custom:MulticlassingEnabled', 'true'),
       (1, 'Custom:MulticlassMaxClasses', '3'),
       (1, 'Custom:ServerAuthStats', 'true');
   ```
   Or in-game: `#rules set Custom:MulticlassingEnabled true`

5. **Test**
   ```
   #addclass list          -- See available classes
   #addclass 4             -- Add Ranger
   #multiclassdiag         -- Verify state
   ```

6. **Check Logs**
   - Server: Zone console output
   - Client: `<RoF2 dir>/dinput8_debug.log`

---

## Next Steps (Immediate Priorities)

1. **Rebuild the touched runtime pieces**
   - Build `zone`; build `world` and the DLL before presentation checks.

2. **Validate newly closed gameplay gaps**
   - Run `I-01` to `I-07`, `X-04`, and `D-04` first.

3. **Finish AA validation**
   - Run `A-01` to `A-05`, then record the observed behavior for `E-02`.

4. **Validate UI/presentation and skills**
   - Run `C-03`, `K-05`, `D-01`, `D-02`, `D-03`, `B-08`, and `G-01`.

5. **Update tracker from evidence**
   - Mark only tests with objective in-game/log evidence as pass; convert failures into focused code tasks.

---

*Document maintained by: Development Team*
*Last Updated: 2026-04-21*
*See also: [TEST_TRACKER.md](TEST_TRACKER.md), [PORT_CHECKLIST.md](PORT_CHECKLIST.md), `/extras/THJServer/docs/`*
