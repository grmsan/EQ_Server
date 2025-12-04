# EQEmulator Server AI Instructions

## Project Overview
This is the **EQEmulator Server**, a C++20 open-source server emulator for EverQuest. It uses a multi-process architecture backed by MariaDB/MySQL.

## Architecture & Components
The server consists of several distinct processes that communicate via TCP/UDP and Shared Memory:
- **`world`**: The central coordinator. Manages client connections, zone selection, and inter-process communication.
- **`zone`**: The game logic engine. Runs the actual gameplay for a specific zone. Multiple instances run simultaneously.
- **`loginserver`**: Handles user authentication and server list management.
- **`ucs`**: Universal Chat Service. Handles chat channels and cross-zone communication.
- **`queryserv`**: Asynchronous database query handler to offload blocking operations from `world`/`zone`.
- **`shared_memory`**: A utility that loads static data (items, spells, loot) into OS shared memory for other processes to access. **Must run first.**
- **`eqlaunch`**: Manages dynamic launching of `zone` processes.

## Build System (CMake)
The project uses **CMake** with **C++20**.
- **Configure**: `cmake -S . -B build`
- **Build**: `cmake --build build --config RelWithDebInfo --parallel`
- **Clean**: Remove the `build/` directory.
- **Dependencies**: Managed via `vcpkg` or pre-built libs in `dependencies/`.

## Development Workflows
- **Server Manager**: Use `python server_manager.py` for a GUI to build, configure, and run the server processes.
- **Startup Sequence**:
  1. `shared_memory` (Essential: loads data)
  2. `loginserver`
  3. `world`
  4. `ucs`
  5. `queryserv`
  6. `eqlaunch` (starts `zone` processes)
- **Database**: Schema changes are handled via migration scripts. Core schema is in `utils/sql/`.

### Diagnostic Tools
- **DB Viewer**: There is a small CLI database viewer in `tools/db_viewer.py` which can be run with Python. It connects to the project's `eqemu_config.json` by default and supports quick commands such as `--list-tables`, `--describe <table>`, `--rows <table>`, `--count <table>`, `--query "<sql>"`, and `--shell`. When debugging, Copilot can use this tool to diagnose issues, inspect schemas and table contents, and run ad-hoc queries (use caution with write queries). Install the dependency via `pip install mysql-connector-python`.

- **EQ Core DLL (Client Injection)**: The `extras/eq-core-dll-main` folder contains a client DLL project that can be used to inject behavior into the EverQuest client (e.g., intercept packets or apply local overlays for items). This is an advanced tool for testing or client-side diagnostic features — it can also be used to avoid global client cache pollution by applying per-client overlays on the client side.
  - Building the DLL: Use the included PowerShell helper `extras/eq-core-dll-main/build_dll.ps1` to run MSBuild and copy resulting DLLs to `extras/eq-core-dll-main/bin`.
  - Usage: After building, copy the appropriate DLL (e.g., `dinput8.dll`) into your local client directory (backup original DLL first). Consult `extras/eq-core-dll-main/README.md` for further instructions.

## Code Conventions & Patterns
- **C++ Standard**: Use C++20 features (concepts, ranges, etc.) where appropriate.
- **Data Access**: Use the **Repository Pattern** (e.g., `character_data_repository.h`) for database interactions. Avoid raw SQL queries in game logic if a repository exists.
- **Entity Hierarchy**:
  - `Entity` -> `Mob` -> `Client` (Players)
  - `Entity` -> `Mob` -> `NPC`
- **Logging**: Use `Log(Logs::General, Logs::Status, "Message")` or `eqemu_logsys.h` macros. Do not use `std::cout` or `printf`.
- **Scripting**: The server embeds **Lua** and **Perl**. Changes to game logic often involve exposing C++ methods to these languages.
- **Packets**:
  - Opcodes are defined in `opcodes.conf` and mapped in `patch_*.conf`.
  - Packet handling logic is often in `zone/client_packet.cpp` or specific handlers.
  - Use `BasePacket` and derived classes for network messages.

## Key Files
- `CMakeLists.txt`: Root build configuration.
- `server_manager.py`: Main developer tool for running the server.
- `zone/client.cpp`: Main player logic.
- `common/eqemu_logsys.h`: Logging system.
- `common/repositories/`: Database access layer.

## Procedural Loot & Item Scaling (Updated)

- **Mechanics**: Items can be modified via two primary mechanisms:
  - **Scaling**: The server provides `ScaleItem()` for CharmFile/exp-based scaling and `ScaleDynamicItem(int level)` for items that use a `dynamic_level`. Both functions compute `m_scaledItem` from the base `ItemData` and apply scaling formulas for attributes, HP, AC, heroics, resistances, weapon damage and selected mod2 fields.
  - **Custom Data Overlays**: `ItemInstance::SetCustomData` stores `m_custom_data` which is applied by `ItemInstance::ApplyCustomStats()` after scaling. This method processes keys such as attribute modifiers (`STR`, `STA`, etc.), heroic stats (`HEROIC_*`), resistances (`MR`, `FR`, `CR`, `DR`, `PR`), and many mod2-like keys (`ATTACK`, `HASTE`, `HP_REGEN`, `SPELL_DMG`, etc.). If a custom key is not explicitly mapped in `ApplyCustomStats()`, it will be ignored — add mapping to `ApplyCustomStats()` if a new key should be supported.

- **Client-side behavior & cache**: The standard EQ client caches Item Definitions by ID. If a server sends modified `ItemBodyStruct` data that updates an item’s base definition, the client will reflect updated stats across all instances of that item ID. To avoid global cache pollution for instance-specific customizations, the server should send base item definitions unmodified and transmit custom overlays separately. Depending on client behavior/version, additional measures such as a client-side DLL or non-standard packet channels may be required to ensure instance-only changes are applied locally and not persisted to the global item def.

- **Best practices**:
  - Use `inst->GetUnscaledItem()` for any `ItemBodyStruct` sent as the baseline to avoid broadcasting modified base definitions.
  - For instance-only changes, keep custom data in `m_custom_data` and call `Client::SendItemScale(inst)` to `ApplyCustomStats()` and send a re-add to the client (Delete + Limbo + ItemPacket) so the client updates its UI for that specific player.
  - Keep `ApplyCustomStats()` mapping consistent and update `logs/inf/item_scaling.log` to record dynamic scaling operations for diagnostics.

- **Advanced note**: If you need to make custom per-player, per-instance item stats visible without affecting other players, client-side code (DLL) or custom packet-handling that applies local overlays may be necessary.

### Core changes and design guidance
- This project actively investigates and sometimes modifies core server functionality related to combat, scaling, and items. If you're exploring or debugging scaling and loot mechanics, please remember:
  - We are actively looking to modify and change core server functions to support new features and balance changes; Copilot (and contributors) may propose or implement changes but should do so thoughtfully and always reference `game_design/` for context and requirements.
  - Always consult the `game_design/` folder for design guidance, feature trackers, and intended gameplay decisions.
  - Prioritize good game design: when modifying core behaviors consider balance, backward compatibility, and minimization of behavioral regressions.
  - For diagnostic work, Copilot and developers should use `tools/db_viewer.py` for quick schema checks and `logs/inf/item_scaling.log` to review scaling details.

