# EQ Core DLL Feature Reference

Last updated: 2026-03-13

## Purpose
This document maps the important feature surfaces in `extras/eq-core-dll-main` so server and client work can be planned against the real implementation, not assumptions.

## Runtime Architecture

### 1) Injection and bootstrap
- The shipped DLL is a `dinput8.dll` proxy that forwards to system `dinput8.dll` and wraps interfaces.
- Entry and proxy flow:
  - `extras/eq-core-dll-main/src/dllmain.cpp`
  - `extras/eq-core-dll-main/src/InterfaceQuery.cpp`
  - `extras/eq-core-dll-main/src/IDirectInput8A.cpp` (+ W/device variants)

### 2) Hook and feature init
- Main hook initializer: `InitHooks()` in `extras/eq-core-dll-main/src/eqgame.cpp`.
- Feature toggles: `extras/eq-core-dll-main/src/_options.h`.
- Core patch toggles (models/zones/map/etc): `extras/eq-core-dll-main/src/core_init.h`.
- Active project file (what actually compiles): `extras/eq-core-dll-main/src/eq-core-dll-vs2022.vcxproj`.

### 3) Main-thread pulse/lifecycle
- Main heartbeat detour: `Detour_ProcessGameEvents()` in `extras/eq-core-dll-main/src/MQ2Pulse.cpp`.
- Game state transitions fan out to custom window lifecycle through:
  - `PowerSlotWnd_SetGameState()`
  - `PowerSlotWnd_Pulse()`
  - `WaypointPOCWnd_SetGameState()`
  - `WaypointPOCWnd_Pulse()`
  - `WaypointLuaImGuiPOC_Pulse()`

## Important Implemented Features

### Server-authoritative stats and multiclass bridge
- Packet ingest and cache logic:
  - `HandleWorldMessage_Detour()`
  - `ApplyEdgeStatLabelPacket()`
  - `OP_ServerStatsUpdate` parsing
  - file: `extras/eq-core-dll-main/src/eqgame.cpp`
- UI value override hooks:
  - `GetGaugeValueFromEQ_Detour()`
  - `GetLabelFromEQ_Detour()`
  - max/cur stat detours for HP/Mana/Endurance
- Multiclass client behavior overrides:
  - `EQCharacter_GetUsableClasses_Detour()`
  - spellcaster gating detours
  - class label overrides (`GetClassDesc` / three-letter code detours)

### Custom SIDL window proof-of-concept
- `PowerSlotWnd` demonstrates:
  - custom XML-backed `CCustomWnd`
  - server-fed data via EdgeStatLabel keys 300-343
  - command surface (`/powerslots`)
  - proper create/destroy on game state and UI reload/cleanup
- files:
  - `extras/eq-core-dll-main/src/PowerSlotWnd.h`
  - `extras/eq-core-dll-main/src/PowerSlotWnd.cpp`
  - command registration in `extras/eq-core-dll-main/src/MQ2CommandAPI.cpp`

### Generic SIDL tool host
- `WaypointPOCWnd` demonstrates:
  - one XML-backed `CCustomWnd` hosting multiple tools
  - waypoint packet parsing and travel interaction
  - GM/dev dashboard commands and reusable list/detail workflow
  - action callbacks to server via `#wppoc ...` and runbook command bridges
- files:
  - `extras/eq-core-dll-main/src/WaypointPOCWnd.h`
  - `extras/eq-core-dll-main/src/WaypointPOCWnd.cpp`
  - packet hook callsite in `extras/eq-core-dll-main/src/eqgame.cpp`
  - UI XML in `extras/eq-core-dll-main/uifiles/EQUI_WaypointPOCWnd.xml`

### Waypoint overlay POC (no SIDL, no ImGui)
- `WaypointOverlayPOC` demonstrates:
  - waypoint packet parsing and local cache
  - lightweight HUD render via `DrawHUDText`
  - command-driven list navigation and travel action callback
- files:
  - `extras/eq-core-dll-main/src/WaypointOverlayPOC.h`
  - `extras/eq-core-dll-main/src/WaypointOverlayPOC.cpp`
  - command registration in `extras/eq-core-dll-main/src/MQ2CommandAPI.cpp`

### Waypoint Lua/ImGui runtime POC
- `WaypointLuaImGuiPOC` demonstrates:
  - shared packet/data/action plumbing for a script-first UI path
  - working standalone Lua + Dear ImGui host window
  - active research path for embedded legacy-client render integration
- files:
  - `extras/eq-core-dll-main/src/WaypointLuaImGuiPOC.h`
  - `extras/eq-core-dll-main/src/WaypointLuaImGuiPOC.cpp`
  - `extras/eq-core-dll-main/scripts/waypoint_imgui_poc.lua`

### Command and HUD extension points
- Command interception and registration:
  - `InitializeMQ2Commands()` in `extras/eq-core-dll-main/src/MQ2CommandAPI.cpp`
- Minimal custom HUD overlay:
  - `/customhud` + draw path in `extras/eq-core-dll-main/src/MQ2HUD.cpp`
- Waypoint POC commands:
  - `/waypointpoc`
  - `/toolwnd`
  - `/gmdashboard`
  - `/waypointoverlay`
  - `/waypointimgui`

### Custom item payload parsing hook
- Item packet tail parser looks for marker `0x1337C0DE` and currently logs/strips appended custom stats.
- file: `extras/eq-core-dll-main/src/eqgame.cpp`

### Diagnostics and observability
- Crash and packet diagnostics are extensive and currently active by default in `_options.h`.
- Important toggles:
  - `isDebugLoggingEnabled`
  - `isRecentPacketTraceEnabled`
  - `isEdgeStatLabelLoggingEnabled`
  - dump toggles for opcodes (1338, 575b)
- Logging path note:
  - there are hardcoded repo paths in `eqgame.cpp` (`C:\Users\marsh\OneDrive\Documents\GitHub\EQ_Server\logs\...`).

## Server->DLL Interfaces In Use
- Custom raw opcode `0x1338` (EdgeStatLabel key/value bus).
- Custom opcode `0x7330` (`OP_ServerStatsUpdate` structured stats packet).
- Appended custom item payload marker `0x1337C0DE` in item packets.
- See `game_design/dll/SERVER_DLL_DATA_CHANNELS.md` for wire details.

## What Is Not Implemented Yet
- Embedded in-client Dear ImGui rendering for the live EQ render loop is not stable yet.
- A generic data-driven module registry for the SIDL host is not implemented yet.
- D3D sample hook files exist (`d3d_example.cpp`, `main.cpp`) but are not currently part of the active VS2022 project compile list.

## Practical Extension Points For New Features
Use this repeatable pattern for custom client UX:

1. Server publishes feature state on a stable packet channel (`0x1338` or new protocol envelope).
2. DLL parses and caches feature state in one place.
3. UI module reads cache and renders (generic SIDL host now; ImGui later if embedded render becomes stable).
4. User actions send callbacks to server (prefer dedicated custom opcode over `/say` once promoted from diagnostics).
5. Server validates action and returns updated state snapshot.

## Notes For Planned Features
- Command buttons:
  - already easy with the generic SIDL host + command callbacks.
- Waypoint/fast travel:
  - native waypoint opcodes already exist server-side; can be wrapped with custom DLL UI for better UX.
- Bag interception/replacement:
  - class surfaces are present, but interception needs validated container-related offsets before production rollout.
