# Power Slot Window — POC Deployment Guide

## What This Proves

This proof-of-concept demonstrates:
1. **Custom SIDL window** — A `CCustomWnd` subclass rendered by the EQ client's native UI engine
2. **Server → client data pipeline** — EdgeStatLabel opcode (0x1338) delivers power slot data (keys 300-343)
3. **Interactive UI widgets** — Labels, gauge bar, list box (per-row color), three buttons (Refresh / Salvage / Close)
4. **Lifecycle management** — Window survives CleanUI/ReloadUI, responds to game-state changes
5. **Slash command** — `/powerslots` opens the window, plus `show`, `refresh`, `debug` subcommands

This validates the foundation for future custom interfaces (gear management, upgrade UIs, progression dashboards).

---

## Files Changed

### DLL-side (extras/eq-core-dll-main/)
| File | Change |
|------|--------|
| `src/PowerSlotWnd.h` | NEW — CPowerSlotWnd class, PSKeys namespace, EdgeStat helpers |
| `src/PowerSlotWnd.cpp` | NEW — Full implementation: constructor, WndNotification, UpdateFromEdgeStats, lifecycle |
| `src/eqgame.cpp` | `g_edgeStatValue[]` / `g_edgeStatHas[]` changed from `static` to extern |
| `src/MQ2CleanUI.cpp` | Added PowerSlotWnd lifecycle hooks (CleanUI / ReloadUI), uncommented DrawNetStatus |
| `src/MQ2Pulse.cpp` | Added PowerSlotWnd_SetGameState() + PowerSlotWnd_Pulse() to Heartbeat |
| `src/MQ2CommandAPI.cpp` | Registered `/powerslots` slash command |
| `src/eq-core-dll-vs2022.vcxproj` | Added PowerSlotWnd.cpp/.h to build |
| `uifiles/EQUI_PowerSlotWnd.xml` | NEW — SIDL XML template for the window |

### Server-side
| File | Change |
|------|--------|
| `zone/client.cpp` | `SendEdgeStats()` expanded: now sends power slot keys 300-343 (slot count, focus tier/XP, per-slot tier/XP/itemID) |

---

## Deployment Steps

### 1. Build the DLL

```powershell
# From repo root — VS Code task or command line
cmake --build build --target zone --config RelWithDebInfo --parallel   # server
# DLL:
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' `
    extras\eq-core-dll-main\eq-core-dll-visualstudio2022.sln `
    /p:Configuration=Release /p:Platform=Win32
```

Output: `extras/eq-core-dll-main/bin/dinput8.dll`

### 2. Copy DLL to EQ Client

Copy `dinput8.dll` to your RoF2 client folder (same directory as `eqgame.exe`).

### 3. Install the SIDL XML Template

Copy `extras/eq-core-dll-main/uifiles/EQUI_PowerSlotWnd.xml` to:
```
<EQ Client>/uifiles/default/EQUI_PowerSlotWnd.xml
```

Then edit `<EQ Client>/uifiles/default/EQUI.xml` and add this line inside the `<XML>` block:
```xml
<Include>EQUI_PowerSlotWnd.xml</Include>
```

> **Note:** The exact manifest file varies by UI skin. `EQUI.xml` (or `default.xml`) is the master include list for `uifiles/default/`.

### 4. Restart Server + Client

- Restart the zone process (or full server) so the new `SendEdgeStats()` code runs
- Restart the EQ client so it loads the new DLL + XML

### 5. Test In-Game

```
/powerslots          — Toggle the Power Slot Manager window
/powerslots show     — Force show the window
/powerslots refresh  — Force data refresh from EdgeStat cache
/powerslots debug    — Dump raw EdgeStat values (keys 300-343) to chat
```

**Expected behavior:**
- Window appears with title "Power Slot Manager"
- Status label shows slot count and focus tier
- XP gauge shows focus slot XP progress
- List shows equipped power slots with tier and XP columns
- Rows are color-coded: green (tier 5+), cyan (any tier), gray (empty)
- Refresh button re-reads cached EdgeStat data
- Salvage button prints a placeholder message (not wired to server action yet)
- Window remembers position between sessions

---

## EdgeStat Key Map (Power Slots)

| Key | Meaning | Type |
|-----|---------|------|
| 300 | Slot count | uint64 |
| 301 | Focus tier | uint64 |
| 302 | Focus XP (current) | uint64 |
| 303 | Focus XP (max for next tier) | uint64 |
| 304-307 | Slot 0: tier, XP, XPMax, itemID | uint64 each |
| 308-311 | Slot 1: tier, XP, XPMax, itemID | uint64 each |
| ... | ... | ... |
| 340-343 | Slot 9: tier, XP, XPMax, itemID | uint64 each |

Slot index mapping:
- Slot 0 = PowerSource (inventory slot 21)
- Slot 1 = Head (2), Slot 2 = Chest (17), Slot 3 = Arms (7)
- Slot 4 = Wrist1 (9), Slot 5 = Hands (12), Slot 6 = Legs (18)
- Slot 7 = Feet (19), Slot 8 = Primary (13), Slot 9 = Secondary (14)

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| `/powerslots` does nothing | DLL not loaded or command not registered | Check dinput8.dll is in EQ client folder, restart client |
| Window opens but is blank | XML not installed or not in manifest | Verify EQUI_PowerSlotWnd.xml is in uifiles/default/ and included in EQUI.xml |
| "Window XML not found" in debug log | pSidlMgr can't find "PowerSlotWnd" template | Check XML filename and ScreenID match |
| Data shows all zeros | Server not sending power slot keys | Verify zone.exe is rebuilt, character has items equipped |
| DLL crash on load | Build mismatch | Rebuild DLL in Release/Win32, ensure VS2022 v143 toolset |

---

## Architecture Notes

```
Server (zone.exe)                     Client (eqgame.exe + dinput8.dll)
─────────────────                     ────────────────────────────────
Client::SendEdgeStats()               ApplyEdgeStatLabelPacket()
  builds {key, value} pairs     →       stores to g_edgeStatValue[]
  for keys 300-343                      g_edgeStatHas[] = 1
  sends OP_EdgeStatLabel (0x1338)
                                      PowerSlotWnd_Pulse() [every 2s]
                                        reads g_edgeStatValue[300..343]
                                        updates labels, gauge, list

                                      /powerslots command
                                        Toggle() → Create or Show/Hide
                                        CPowerSlotWnd(CSidlScreenWnd*)
                                          GetChildItem() for widgets
                                          SetWndNotification(this)
```

## What's Next

After validating this POC works in-game:
- **12B:** Wire Salvage button to server action (new opcode or repurposed channel)
- **12C:** Add item icons/images to the list (CTextureAnimation)
- **12D:** Build additional windows (character stats dashboard, quest tracker overlay)
- **Future:** Evaluate MQ Next Lua scripting for rapid UI prototyping
