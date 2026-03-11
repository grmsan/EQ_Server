# Waypoint UI POC Matrix

Last updated: 2026-03-10

## Purpose
Compare three client-side UI paths against the same feature: waypoint list + travel action.

## POC Paths

1. SIDL window path
- Client command: `/waypointpoc`
- Server bridge: `#wppoc list`, `#wppoc travel <id>`
- Client prerequisite:
  - `EQUI_WaypointPOCWnd.xml` must be in `uifiles/default/` and referenced by `default.xml`
- Implementation files:
  - `extras/eq-core-dll-main/src/WaypointPOCWnd.cpp`
  - `extras/eq-core-dll-main/uifiles/EQUI_WaypointPOCWnd.xml`

2. C++ overlay path (no SIDL, no ImGui)
- Client command: `/waypointoverlay`
- Subcommands: `toggle`, `refresh`, `next`, `prev`, `travel`
- Implementation file:
  - `extras/eq-core-dll-main/src/WaypointOverlayPOC.cpp`

3. Lua+ImGui path (scaffold mode in current repo state)
- Client command: `/waypointimgui`
- Subcommands: `toggle`, `status`, `refresh`, `next`, `prev`, `travel`, `reload`
- Current state:
  - scaffold mode with runtime-unavailable HUD indicator
  - real Lua+ImGui runtime is not wired yet (x86 deps missing in repo)
- Files:
  - `extras/eq-core-dll-main/src/WaypointLuaImGuiPOC.cpp`
  - `extras/eq-core-dll-main/scripts/waypoint_imgui_poc.lua`

## Common Server Backend
- Player command: `#wppoc`
  - `list`
  - `travel <waypoint_id>`
  - `expedition`
- File:
  - `zone/gm_commands/wppoc.cpp`

## Quick Bring-Up Sequence

1. On server:
- Use `#wppoc list` to verify `OP_WaypointList` generation.

2. On client DLL build:
- Test SIDL path: `/waypointpoc`, click `Refresh`, then `Travel`.
- Test overlay path: `/waypointoverlay toggle`, `/waypointoverlay next`, `/waypointoverlay travel`.
- Test Lua/ImGui scaffold path: `/waypointimgui toggle`, `/waypointimgui status`, `/waypointimgui travel`.

## Evaluation Checklist

Use the same checklist while testing each path:

1. Interaction quality
- Selection clarity
- Travel action speed
- Error feedback quality

2. Engineering ergonomics
- Files touched for small UI change
- Rebuild/hot-reload cycle time
- Debuggability

3. Runtime stability
- Zone in/out behavior
- UI reload (`/loadskin`) behavior
- Packet parse resilience

4. Future maintainability
- Cost to add a second feature panel
- Team skill alignment (C++ UI vs script UI)
- Risk of client-version-specific fragility

## Current Practical Recommendation
- Best immediate production path: SIDL window.
- Best long-term velocity path: Lua+ImGui after runtime dependency integration.
- Best debugging/utility path: C++ overlay.

## Verification Snapshot
- 2026-03-10:
  - server `zone` target build succeeded with `#wppoc` command path
  - DLL `eq-core-dll-vs2022.vcxproj` `Release|Win32` build succeeded with:
    - `WaypointPOCWnd`
    - `WaypointOverlayPOC`
    - `WaypointLuaImGuiPOC`
