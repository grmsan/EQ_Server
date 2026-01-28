# Client DLL Integration Guide

**DLL Project:** `extras/eq-core-dll-main/`
**Output:** `dinput8.dll`
**Client:** RoF2 (Rain of Fear 2)

---

## Overview

The eq-core-dll provides client-side hooks that enable the RoF2 client to properly display and filter multiclass-related UI elements. The server is always authoritative; the DLL only affects what the client shows.

---

## Build Instructions

### Prerequisites
- Visual Studio 2022 (Community or higher)
- Windows SDK 10.0+

### Building
```powershell
# Option 1: VS Code task
# Use "Build EQ Core DLL (Release)" task

# Option 2: Command line
cd extras/eq-core-dll-main
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" ^
    eq-core-dll-visualstudio2022.sln /p:Configuration=Release /p:Platform=Win32

# Output: extras/eq-core-dll-main/bin/dinput8.dll
```

### Installation
1. Copy `dinput8.dll` to your RoF2 client folder (same folder as `eqgame.exe`)
2. **Restart the client** after any DLL update
3. The DLL loads automatically when the client starts

---

## Configuration (_options.h)

Key options that control multiclass behavior:

```cpp
// Enable server-authoritative stats via EdgeStatLabel opcode
bool isServerAuthoritativeStatsEnabled = true;

// Override GetUsableClasses to use server-provided multiclass mask
bool isMulticlassUsableClassesOverrideEnabled = true;

// Enable spellbook/mana UI for any owned caster class
bool isMulticlassSpellUiOverrideEnabled = true;

// Display multiclass names in /who and character select
bool isMulticlassClassNameOverrideEnabled = true;

// Debug logging to dinput8_debug.log
bool isDebugLoggingEnabled = true;

// Track recent packets for debugging
bool isRecentPacketTraceEnabled = true;

// Log EdgeStatLabel packets
bool isEdgeStatLabelLoggingEnabled = true;
```

To modify: Edit `extras/eq-core-dll-main/src/_options.h` and rebuild.

---

## Server Communication

### EdgeStatLabel Protocol (Opcode 0x1338)

The server sends stat updates to the DLL using a custom opcode. The DLL parses these and caches them for use in detours.

**Packet Structure:**
```cpp
// Header
uint32_t count;  // Number of key-value pairs

// Repeated `count` times:
struct Entry {
    uint32_t key;
    uint64_t value;
};
```

**Key Allocations:**
| Key | Description | Type |
|-----|-------------|------|
| 2 | Current HP | int |
| 3 | Current Mana | int |
| 4 | Current Endurance | int |
| 5 | Max HP | int |
| 6 | Max Mana | int |
| 7 | Max Endurance | int |
| 24-30 | STR/STA/DEX/AGI/INT/WIS/CHA | int |
| **200** | **Classes Bitmask** | uint16 |

**Key 200 (Classes Bitmask):**
- Sent as uint64 but only lower 16 bits used
- Same format as item/spell class bitmasks
- Bit N set = character owns class (N+1)

---

## Detour Functions

### EQCharacter_GetUsableClasses

**Purpose:** Returns which classes the character can use for UI filtering

**Original Behavior:** Returns single class mask based on base class

**Multiclass Override:**
```cpp
if (isMulticlassUsableClassesOverrideEnabled && g_serverUsableClassesMask != 0) {
    return g_serverUsableClassesMask;  // Server-provided mask
}
```

**Effect:** AA window, spell filters, and item usability checks use all owned classes

---

### EQSpell_GetSpellLevelNeeded

**Purpose:** Returns required level for a spell by class

**Original Behavior:** Returns level from spell data for requested class

**Multiclass Override:**
```cpp
// Find minimum required level across all owned classes
int best = 255;
for (int classId = 1; classId <= 16; ++classId) {
    if (!(g_serverUsableClassesMask & (1 << (classId - 1)))) continue;
    int req = spell->Level[classId - 1];
    if (req > 0 && req < best) best = req;
}
return best;
```

**Effect:**
- Spell merchant "Show Usable Items" includes spells for any owned class
- Spellbook shows usable spells at correct levels
- Tooltips show correct level requirements

---

### PcZoneClient_GetPcSkillLimit

**Purpose:** Returns skill cap for skills window display

**Original Behavior:** Returns cap based on base class only

**Multiclass Override:**
```cpp
if (nativeCap > 0) return nativeCap;  // Base class has cap

// If server granted a non-zero skill value, expose it
if (charInfo->Skill[skillId] > 0) {
    return charInfo->Skill[skillId];  // Show server-granted value
}
```

**Effect:** Skills window shows skills that the server has granted via multiclassing

---

## Debug Output

### dinput8_debug.log

Location: RoF2 client folder

**Sample Output:**
```
[2026-01-28 10:30:15] EDGE_STAT multiclass classes_bitmask=0x1009 (4105)
[2026-01-28 10:30:16] CLIENT_DETOUR stat=UsableClasses ret=0x1009 server_mask=0x1009
[2026-01-28 10:30:20] CLIENT_DETOUR stat=SpellLevelNeeded class=1 native=255 used=4 mask=0x1009
```

**What to Look For:**
1. `EDGE_STAT multiclass` - Confirms server sent classes bitmask
2. `UsableClasses ret` - Confirms detour is returning multiclass mask
3. `SpellLevelNeeded` - Shows spell level lookups with multiclass aggregation

### Enabling Verbose Logging

In `_options.h`:
```cpp
bool isDebugLoggingEnabled = true;
bool isRecentPacketTraceEnabled = true;
bool isMQ2LabelsPerSidlLoggingEnabled = true;  // Warning: very verbose
```

---

## Troubleshooting

### DLL Not Loading
1. Verify `dinput8.dll` is in the same folder as `eqgame.exe`
2. Check that the DLL is not blocked (right-click → Properties → Unblock)
3. Ensure RoF2 client, not another version

### Multiclass Mask Not Received
1. Check server logs for `SendEdgeStats` calls
2. Verify `Custom:ServerAuthStats` rule is true
3. Run `#multiclassdiag refresh` in-game to force send
4. Check `dinput8_debug.log` for `EDGE_STAT` entries

### Spells Still Filtered Wrong
1. Verify `isMulticlassUsableClassesOverrideEnabled = true`
2. Check that client was restarted after DLL update
3. Look for `CLIENT_DETOUR stat=SpellLevelNeeded` in log
4. Confirm `g_serverUsableClassesMask` is non-zero

### Skills Not Showing
1. Verify `isMulticlassUsableClassesOverrideEnabled = true`
2. Check that server seeded skill values (use `#myskills` in-game)
3. Look for `CLIENT_DETOUR stat=PcSkillLimit` in log

### Class Name Not Updated
1. Verify `isMulticlassClassNameOverrideEnabled = true`
2. Rebuild `world.exe` if `/who` is wrong
3. Check `UpdateWho()` is called after class change

---

## Code Reference

### Key Files
| File | Purpose |
|------|---------|
| `eqgame.cpp` | Main detours, EdgeStatLabel parsing, multiclass logic |
| `_options.h` | Feature toggles and configuration |
| `EQClasses.h` | EQ client structure definitions |
| `MQ2Main.h` | MQ2-style globals and helpers |

### Important Functions in eqgame.cpp
| Function | Lines (approx) | Purpose |
|----------|----------------|---------|
| `ApplyEdgeStatLabelPacket` | 885-990 | Parse EdgeStatLabel, set g_serverUsableClassesMask |
| `EQCharacter_GetUsableClasses_Detour` | 2060-2100 | Override usable classes |
| `EQSpell_GetSpellLevelNeeded_Detour` | 2105-2285 | Override spell level checks |
| `PcZoneClient_GetPcSkillLimit_Detour` | 2295-2340 | Override skill caps |
| `BuildMulticlassAbbrevList` | 2350-2400 | Build "WAR/RNG/MAG" display string |

### Global State
```cpp
// Server-provided multiclass bitmask (from EdgeStatLabel key 200)
static uint32_t g_serverUsableClassesMask = 0;

// Last logged value (avoid log spam)
static int g_last_logged_usable_classes_ret = INT_MIN;

// Merchant spell helper state
static volatile LONG g_merchant_spell_best_class = 0;
static volatile LONG g_merchant_spell_best_level = 0;
```

---

## Extending the DLL

### Adding a New Detour

1. Define trampoline:
```cpp
DETOUR_TRAMPOLINE_EMPTY(ReturnType __fastcall FunctionName_Tramp(void*, void*, Args...));
```

2. Implement detour:
```cpp
ReturnType __fastcall FunctionName_Detour(void* This, void* edx, Args...) {
    // Your logic
    return FunctionName_Tramp(This, edx, args...);
}
```

3. Install in `InstallDetours()`:
```cpp
EzDetourwName(address, FunctionName_Detour, FunctionName_Tramp, "FunctionName");
```

### Adding EdgeStatLabel Keys

1. Server side (`zone/client.cpp::SendEdgeStats`):
```cpp
entries.push_back({YOUR_KEY, your_value});
```

2. DLL side (`eqgame.cpp::ApplyEdgeStatLabelPacket`):
```cpp
constexpr uint32_t kYourKey = YOUR_KEY;
if (key == kYourKey) {
    g_yourGlobal = static_cast<Type>(value);
}
```

---

*See also: [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) for full system documentation*
