# Lua + ImGui Integration in eq-core-dll

Last updated: 2026-03-13

## Purpose
This is a living implementation document for adding Lua-driven ImGui UI to `extras/eq-core-dll-main` without breaking legacy EQ client behavior.

## Current Reality Check

### Implemented now
- [x] Server -> DLL custom data bus via EdgeStatLabel raw opcode `0x1338`
- [x] Server -> DLL structured stats packet via `OP_ServerStatsUpdate` (`0x7330` for RoF2)
- [x] DLL-side stat/multiclass detours and UI overrides (labels/gauges/class filters)
- [x] Custom UI proof-of-concept via SIDL (`PowerSlotWnd`)
- [x] Generic SIDL tool host (`WaypointPOCWnd`) parsing `OP_WaypointList` and hosting both waypoint and GM/dev dashboard modes
- [x] Waypoint overlay POC (`WaypointOverlayPOC`) using HUD text and command-driven interaction
- [x] Lua/ImGui waypoint runtime POC (`WaypointLuaImGuiPOC`) with:
  - D3D9 `CreateDevice` -> `Present`/`Reset` hook chain
  - Dear ImGui DX9 + Win32 backend initialization
  - embedded Lua runtime (`LuaJIT`) and frame-time `draw()` execution
  - script bridge API (`imgui.*`, `waypoint.*`)
- [x] Command registration and HUD overlay extension points (`/powerslots`, `/customhud`)

### Not implemented yet (for this Lua/ImGui plan)
- [ ] Production DLL -> server action RPC channel (current callback path still uses command bridge via `#wppoc`)
- [ ] Multi-module script loader/hot-swap manager (currently single script: `waypoint_imgui_poc.lua`)

## Waypoint POC Snapshot (Three Paths)

1. SIDL generic tool host path
- Commands:
  - `/waypointpoc`
  - `/toolwnd`
  - `/gmdashboard`
- Files:
  - `extras/eq-core-dll-main/src/WaypointPOCWnd.cpp`
  - `extras/eq-core-dll-main/uifiles/EQUI_WaypointPOCWnd.xml`
- Current status:
  - one XML-backed `CCustomWnd` now hosts more than one tool
  - `Waypoints` is the first production-ready module
  - `GM Dashboard` proves the host is not waypoint-specific

2. C++ overlay path (no SIDL, no ImGui)
- Command: `/waypointoverlay`
- File:
  - `extras/eq-core-dll-main/src/WaypointOverlayPOC.cpp`

3. Lua+ImGui path (runtime mode)
- Command: `/waypointimgui`
- Files:
  - `extras/eq-core-dll-main/src/WaypointLuaImGuiPOC.cpp`
  - `extras/eq-core-dll-main/scripts/waypoint_imgui_poc.lua`
- Current status:
  - packet ingest + action callback path is wired
  - Lua state and ImGui render loop are active in a standalone DX9 host window
  - script reload available via `/waypointimgui reload`
  - embedded in-client render hook remains unresolved

## Build Verification

- 2026-03-10: `eq-core-dll-vs2022.vcxproj` built successfully for `Release|Win32` with all three waypoint POC modules compiled and linked.
- 2026-03-11: Lua/ImGui runtime implementation landed in `WaypointLuaImGuiPOC.cpp` and is now part of the active VS2022/VS2019 build targets.
- 2026-03-13: Generic SIDL tool host refactor built successfully in the active VS2022 target.
- 2026-03-13: Lua+ImGui POC works in a dedicated host window, while embedded EQ render hooking remains incomplete.

## PowerShell Dependency Install (Lua + ImGui)

- Installer script:
  - `extras/eq-core-dll-main/install_lua_imgui_deps.ps1`
- Default command:
  - `powershell -ExecutionPolicy Bypass -File .\extras\eq-core-dll-main\install_lua_imgui_deps.ps1`
- Default triplet:
  - `x86-windows-static-md`
- Installed package set:
  - `luajit`
  - `imgui[dx9-binding,win32-binding]`

## Project Wiring Status

- `extras/eq-core-dll-main/src/eq-core-dll-vs2022.vcxproj`
  - Added `LuaImGuiVcpkgRoot` user macro (`..\..\..\vcpkg\vcpkg-tool\installed\x86-windows-static-md`)
  - Added include dir: `$(LuaImGuiVcpkgRoot)\include`
  - Added lib dir: `$(LuaImGuiVcpkgRoot)\lib`
  - Added linker deps: `lua51.lib;imgui.lib`
- `extras/eq-core-dll-main/src/eq-core-dll-vs2019.vcxproj`
  - Retargeted `PlatformToolset` from `v142` to `v143`
  - Same Lua/ImGui wiring added as VS2022
  - Added missing compile entries to align with VS2022 project:
    - `MQ2KeyBinds.cpp`
    - `MQ2Protect.cpp`
  - Added linker dep for runtime hook path:
    - `d3d9.lib`

## Server Manager Deployment Notes

- `server_manager.py` now syncs:
  - DLL (`dinput8.dll`)
  - UI XML files (`EQUI_PowerSlotWnd.xml`, `EQUI_WaypointPOCWnd.xml`) + manifest includes
  - Lua script (`scripts/waypoint_imgui_poc.lua`) into client `<EQ>/scripts`
- Build command uses dedicated output folder (`..\bin\sm\`) to reduce file-lock contention on default `..\bin\` artifacts.

## Retarget Verification

- 2026-03-10:
  - `eq-core-dll-vs2019.vcxproj` builds with `v143` on this machine.
  - Validation build used `OutDir=..\bin\vs2019\` to avoid file-lock collisions with existing `..\bin\dinput8.*` artifacts.

## Findings Added During DLL Review

1. `d3d_example.cpp`/`main.cpp` remain standalone examples and are still not the active integration path.
2. Active Lua/ImGui runtime now lives in `WaypointLuaImGuiPOC.cpp` and is initialized from main DLL hook lifecycle.
3. The most practical existing custom UI pattern remains `PowerSlotWnd` (SIDL + server-fed data + lifecycle hooks).
4. The best current transport for new feature data remains existing custom packet/command bridge paths (`0x1338`, `OP_WaypointList`, and server command handlers).
5. The most practical production UI direction is now a reusable SIDL host with tool modes, not one XML file per feature.

## Integration Strategy

### Phase 0: Stabilize current custom UI transport (completed baseline)
- Keep using EdgeStatLabel and `OP_ServerStatsUpdate` for authoritative state snapshots.
- Keep legacy UI operational when custom DLL paths are absent.

### Phase 1: Add Lua runtime to DLL (completed baseline)
1. Added runtime dependency wiring via vcpkg (`LuaJIT` + `ImGui` with DX9/Win32 bindings).
2. Added script host lifecycle:
   - init in `WaypointLuaImGuiPOC_Initialize()`
   - shutdown in `WaypointLuaImGuiPOC_Shutdown()`
3. Exposed Lua API surface:
   - `waypoint.get_entries()`
   - `waypoint.refresh()`
   - `waypoint.travel(id)`
   - `waypoint.get_selected_index()/set_selected_index()`
   - `waypoint.has_packet()`
   - `imgui.*` minimal immediate-mode bindings used by POC script

### Phase 2: Add ImGui render loop (completed baseline)
1. Hooked D3D9 vtables in active runtime path:
   - `IDirect3D9::CreateDevice`
   - `IDirect3DDevice9::Present`
   - `IDirect3DDevice9::Reset`
2. Frame loop now runs in `Present` hook:
   - `ImGui_ImplDX9_NewFrame()`
   - `ImGui_ImplWin32_NewFrame()`
   - Lua `draw()` execution
   - `ImGui_ImplDX9_RenderDrawData(...)`
3. Device lost/reset flow uses `ImGui_ImplDX9_InvalidateDeviceObjects()` and `CreateDeviceObjects()`.

### Phase 3: Feature module model (next)
1. Add `feature_id` routing between packets and Lua modules.
2. Add a registration model for UI modules:
   - `on_packet(feature_id, payload)`
   - `on_draw()`
   - `on_action(...)`
3. Add hot-reload command (`/luareload`) and script error isolation.

### Phase 4: Production action path
1. Replace diagnostic `/say #test ...` callback usage with dedicated action packets.
2. Enforce server-side validation for all UI actions.
3. Add request/response IDs for idempotency and debugging.

## Feature Targets Mapped To This Plan
- Command-to-button UX:
  - already served well by the generic SIDL host / GM dashboard.
- Custom bag UX/interception:
  - feasible, but requires validated container open/use hooks before full replacement.
- Waypoint/fast travel UX:
  - server waypoint subsystem already exists; SIDL is already viable and ImGui can still provide a richer future interaction layer.

## Practical Architecture Direction

For the current codebase, the strongest path is:

1. Use SIDL as the native in-client rendering shell.
2. Reuse a small number of generic window templates instead of creating one-off XML for every feature.
3. Keep feature behavior/data server-driven and optionally script-driven.
4. Continue Lua+ImGui as an R&D lane until embedded render is stable in the legacy client.

## Implementation Notes To Keep Updated
When new steps are completed, update this file with:
1. exact files changed
2. opcodes/keys added
3. exposed Lua API functions
4. failure/fallback behavior for older clients or missing DLL

## Related Documentation
- `game_design/dll/EQ_CORE_DLL_FEATURE_REFERENCE.md`
- `game_design/dll/SERVER_DLL_DATA_CHANNELS.md`
