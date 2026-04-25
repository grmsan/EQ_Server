# Multiclass (Gestalt) System

**Status:** Active Implementation
**Max Classes:** 3 (configurable via `Custom:MulticlassMaxClasses`)
**Reference Implementation:** THJServer (`/extras/THJServer/`)

---

## Overview

The multiclass system allows a single character to simultaneously hold multiple classes (up to 3 by default). All owned classes share:
- One level, one inventory, one AA pool
- One spellbook (union of all class spells)
- One skill table (max caps from any owned class)

**Key Principle:** Server authoritative, client displays via DLL hooks.

---

## Quick Links

| Document | Description |
|----------|-------------|
| [VISION.md](VISION.md) | **Design philosophy and guiding principles** |
| [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) | Master technical design document |
| [TEST_TRACKER.md](TEST_TRACKER.md) | Bug tracking and test checklist |
| [PORT_CHECKLIST.md](PORT_CHECKLIST.md) | File-by-file THJ parity status |
| [DLL_INTEGRATION.md](DLL_INTEGRATION.md) | Client DLL build and debug guide |
| [QUICK_START.md](QUICK_START.md) | 15-minute setup guide |
| [/extras/THJServer/docs/multiclass.md](/extras/THJServer/docs/multiclass.md) | THJ reference documentation |

---

## How It Works

### Class Storage
- **Compatibility class:** `character_data.class` (single owned class used for stock packet compatibility)
- **All owned classes:** `data_buckets.GestaltClasses` (16-bit bitmask)

### Bitmask Format
```
Bit 0  = Warrior (1)      Bit 8  = Rogue (256)
Bit 1  = Cleric (2)       Bit 9  = Shaman (512)
Bit 2  = Paladin (4)      Bit 10 = Necromancer (1024)
Bit 3  = Ranger (8)       Bit 11 = Wizard (2048)
Bit 4  = ShadowKnight (16) Bit 12 = Magician (4096)
Bit 5  = Druid (32)       Bit 13 = Enchanter (8192)
Bit 6  = Monk (64)        Bit 14 = Beastlord (16384)
Bit 7  = Bard (128)       Bit 15 = Berserker (32768)

Example: Warrior + Ranger + Mage = 1 + 8 + 4096 = 4105
```

### Server-Client Communication
1. Server stores bitmask in `GestaltClasses` data bucket
2. Server sends `EdgeStatLabel` packet (opcode 0x1338) with key 200 = classes_bitmask
3. DLL (`dinput8.dll`) parses packet, caches mask
4. DLL overrides client functions to use server mask for UI filtering

---

## GM Commands

```
#addclass <id>      Add a class to target (or self)
#addclass list      Show all classes with ownership status
#removeclass <id>   Remove a class from target
#multiclassdiag     Full multiclass diagnostic dump
#mystats            Shows classes_bitmask in output
```

---

## Server Rules

```cpp
Custom:MulticlassingEnabled   true    // Master toggle
Custom:MulticlassMaxClasses   3       // Max classes allowed
Custom:MulticlassBucketKey    "GestaltClasses"  // Data bucket key
Custom:ServerAuthStats        true    // Enable EdgeStatLabel
Custom:UseDynamicAATimers     true    // Deconflict AA timers
```

---

## Client DLL Setup

1. Build DLL: `extras/eq-core-dll-main/eq-core-dll-visualstudio2022.sln`
2. Copy `dinput8.dll` to RoF2 client directory
3. Restart client after any DLL update
4. Check `dinput8_debug.log` for diagnostics

Key DLL options (`_options.h`):
- `isMulticlassUsableClassesOverrideEnabled = true`
- `isMulticlassSpellUiOverrideEnabled = true`
- `isMulticlassClassNameOverrideEnabled = true`

---

## Current Status

See [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) for detailed phase status.
See [TEST_TRACKER.md](TEST_TRACKER.md) for current bugs and test results.

**Working:**
- Class bitmask persistence and API
- GM commands (#addclass, #removeclass, #multiclassdiag)
- EdgeStatLabel integration
- DLL detours for equipment and spell filters
- Spell merchant "Show usable items" filter
- Spell tooltips show correct class levels
- AA purchase, activation, window table, dynamic timers, and passive ownership gating
- Skills/training server paths and DLL skill-window exposure path
- Equip, click, merchant item filtering, and class-restricted augment gating
- Character select, `/who`, guild projection, `#mystats`, and inventory class-label presentation paths

**In Progress:**
- Runtime validation for the implemented paths above; see [TEST_TRACKER.md](TEST_TRACKER.md)
- Final removed-class AA entitlement policy (`E-02`)
- Optional design research for removing AA level requirements

**Known Issues:**
- Most remaining multiclass work is validation-first, not broad THJ porting.
- Any failed validation case should become a focused code/data task with evidence in the tracker.

---

## Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Storage | Data bucket bitmask | Flexible, no schema changes needed |
| Communication | EdgeStatLabel opcode | Reuses existing DLL infrastructure |
| Skill caps | Max from any owned class | "All classes equal" principle |
| Spell access | Union of all class spells | Full access to owned class abilities |
| Soft-lock | Retain learned, gate at use-time | Player-friendly, reversible |

---

## Testing Checklist

Quick smoke test after changes:
1. Fresh client restart with updated DLL
2. Server restart after rebuild
3. `#multiclassdiag` - verify state
4. `#addclass <id>` - verify bitmask update
5. Check `dinput8_debug.log` for EdgeStatLabel
6. Test specific feature (spell/AA/skill)
7. `#removeclass <id>` - verify soft-lock

---

## Legacy Notes (Original Exploration)

The original exploration document proposed:
- Custom opcode `OP_MultiClassInfo` - **Replaced by EdgeStatLabel key 200**
- Array of 3 class IDs - **Replaced by 16-bit bitmask**
- Profile byte stashing - **Not needed with EdgeStatLabel approach**

The current implementation follows THJServer patterns which proved more robust.

---

*See [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) for complete technical documentation.*
