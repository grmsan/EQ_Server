# Lua + ImGui Integration in eq-core-dll

Last updated: 2026-03-10

## Purpose
This is a living implementation document for adding Lua-driven ImGui UI to `extras/eq-core-dll-main` without breaking legacy EQ client behavior.

## Current Reality Check

### Implemented now
- [x] Server -> DLL custom data bus via EdgeStatLabel raw opcode `0x1338`
- [x] Server -> DLL structured stats packet via `OP_ServerStatsUpdate` (`0x7330` for RoF2)
- [x] DLL-side stat/multiclass detours and UI overrides (labels/gauges/class filters)
- [x] Custom UI proof-of-concept via SIDL (`PowerSlotWnd`)
- [x] Waypoint travel SIDL POC (`WaypointPOCWnd`) parsing `OP_WaypointList` (`0x1402`) and issuing travel actions via server command bridge (`#wppoc`)
- [x] Waypoint overlay POC (`WaypointOverlayPOC`) using HUD text and command-driven interaction
- [x] Lua/ImGui waypoint scaffold POC (`WaypointLuaImGuiPOC`) with command/data/action plumbing ready for runtime swap-in
- [x] Command registration and HUD overlay extension points (`/powerslots`, `/customhud`)

### Not implemented yet (for this Lua/ImGui plan)
- [ ] Embedded Lua runtime inside `eq-core-dll-main`
- [ ] ImGui renderer integrated into active DLL build
- [ ] D3D9 Present/EndScene hook in the active project path
- [ ] Production DLL -> server action RPC channel (current callback is diagnostic chat command)

## Waypoint POC Snapshot (Three Paths)

1. SIDL window path
- Command: `/waypointpoc`
- Files:
  - `extras/eq-core-dll-main/src/WaypointPOCWnd.cpp`
  - `extras/eq-core-dll-main/uifiles/EQUI_WaypointPOCWnd.xml`

2. C++ overlay path (no SIDL, no ImGui)
- Command: `/waypointoverlay`
- File:
  - `extras/eq-core-dll-main/src/WaypointOverlayPOC.cpp`

3. Lua+ImGui path (scaffold mode)
- Command: `/waypointimgui`
- Files:
  - `extras/eq-core-dll-main/src/WaypointLuaImGuiPOC.cpp`
  - `extras/eq-core-dll-main/scripts/waypoint_imgui_poc.lua`
- Current status:
  - packet ingest + action callback path is wired
  - actual embedded Lua + ImGui runtime is not wired yet

## Build Verification

- 2026-03-10: `eq-core-dll-vs2022.vcxproj` built successfully for `Release|Win32` with all three waypoint POC modules compiled and linked.

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

## Retarget Verification

- 2026-03-10:
  - `eq-core-dll-vs2019.vcxproj` builds with `v143` on this machine.
  - Validation build used `OutDir=..\bin\vs2019\` to avoid file-lock collisions with existing `..\bin\dinput8.*` artifacts.

## Findings Added During DLL Review

1. The active VS2022 project (`src/eq-core-dll-vs2022.vcxproj`) does not currently compile a Lua/ImGui integration path.
2. `d3d_example.cpp` and `main.cpp` exist but are not active integration points in the current build.
3. The most practical existing custom UI pattern is `PowerSlotWnd` (SIDL + server-fed data + lifecycle hooks).
4. The best current transport for new feature data is the existing `0x1338` key/value bus (or a new envelope on a dedicated custom opcode).
5. Server Lua already supports raw opcode packet bypass (`zone/lua_packet.cpp`, `zone/lua_client.cpp`), which can help rapid prototyping even before DLL-embedded Lua is added.

## Integration Strategy

### Phase 0: Stabilize current custom UI transport (completed baseline)
- Keep using EdgeStatLabel and `OP_ServerStatsUpdate` for authoritative state snapshots.
- Keep legacy UI operational when custom DLL paths are absent.

### Phase 1: Add Lua runtime to DLL
1. Add Lua runtime dependency (LuaJIT or Lua 5.4) to `eq-core-dll-main` dependencies.
2. Create a minimal script host in DLL init/shutdown lifecycle.
3. Expose a constrained API surface first:
   - read-only stat cache
   - player/target metadata
   - safe command/action emit stubs

### Phase 2: Add ImGui render loop
1. Add ImGui + Win32 + D3D9 backends to project.
2. Hook D3D9 device Present/EndScene in active runtime path.
3. Create frame loop:
   - begin frame
   - call Lua `draw()`
   - render
4. Handle device lost/reset cleanly.

### Phase 3: Feature module model
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
  - immediate candidate for first Lua/ImGui module.
- Custom bag UX/interception:
  - feasible, but requires validated container open/use hooks before full replacement.
- Waypoint/fast travel UX:
  - server waypoint subsystem already exists; ImGui can provide richer interaction layer without replacing transport logic.

## Implementation Notes To Keep Updated
When new steps are completed, update this file with:
1. exact files changed
2. opcodes/keys added
3. exposed Lua API functions
4. failure/fallback behavior for older clients or missing DLL

## Related Documentation
- `game_design/dll/EQ_CORE_DLL_FEATURE_REFERENCE.md`
- `game_design/dll/SERVER_DLL_DATA_CHANNELS.md`
