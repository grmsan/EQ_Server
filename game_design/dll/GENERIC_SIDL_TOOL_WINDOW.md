# Generic SIDL Tool Window

Last updated: 2026-03-13

## Purpose

Define the production-oriented pattern for new custom EQ client tools that should feel native, survive legacy client constraints, and avoid one bespoke XML window per feature.

## Current Implementation

The active generic host is implemented in:

- `extras/eq-core-dll-main/src/WaypointPOCWnd.cpp`
- `extras/eq-core-dll-main/src/WaypointPOCWnd.h`
- `extras/eq-core-dll-main/uifiles/EQUI_WaypointPOCWnd.xml`

Commands:

- `/toolwnd`
- `/waypointpoc`
- `/gmdashboard`

## Why This Is The Default Path

1. It renders inside the native EQ UI stack.
2. It already works with packet-fed data and server actions.
3. It is easier to ship safely than the current embedded ImGui attempt.
4. It can host multiple tools behind one reusable shell.

## Current Tool Modes

### 1. Waypoints

Purpose:
- show unlocked and locked waypoint rows
- refresh from server
- travel through `#wppoc travel <id>`

### 2. GM Dashboard

Purpose:
- expose common development and live-ops commands as a click-driven panel
- reduce repetitive manual command entry
- provide a pattern for future admin/debug tools

Current example actions:

- `#loc`
- `#reloadquests`
- `#repop`
- `#show spawn_status all`
- `#spawnfix`
- `#npcedit save`
- `#npcspawn create 999300`
- `#wp unlock qeynos2`

## Recommended Expansion Model

Do not create a new XML window for every feature.

Instead:

1. Keep one reusable shell with:
- mode buttons
- list/grid surface
- status label
- details/info label
- primary and secondary action buttons

2. Add feature modules in C++ behind a shared host contract:
- `rebuild rows`
- `selection changed`
- `primary action`
- `secondary action`
- `refresh`

3. Feed modules from server packets or command/action bridges.

4. Only add new XML when a feature truly needs a different layout class, such as:
- inventory grid
- tabbed editor
- modal wizard

## Good Fit Features

- waypoint and fast-travel tools
- GM/admin dashboards
- event control panels
- spawn management helpers
- quest/dev utilities
- custom feature toggles that were previously command-only

## Harder Features

These may still need dedicated shells or deeper client hooks:

- bag/container replacement
- spellbook or inventory-style dense grids
- drag/drop heavy editors
- highly animated custom layouts

## Next Refactor Target

The current host still keeps some waypoint-specific naming because it evolved from the waypoint POC.

Next cleanup pass should:

1. rename `WaypointPOCWnd.*` to a generic host name
2. split tool-mode logic into per-tool handlers
3. move dashboard entries into data/config instead of hardcoding them
4. add a generic packet/action routing layer so non-waypoint tools do not reuse waypoint terminology
