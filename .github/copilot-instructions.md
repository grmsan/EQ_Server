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

## Procedural Loot & Item Scaling
- **Mechanism**: Uses `ItemInstance::SetCustomData` (Lua) and `ItemInstance::ApplyCustomStats` (C++) to modify item stats dynamically.
- **Client Behavior**: The standard EQ client caches Item Definitions by ID. Sending modified stats in `OP_ItemPacket` (via `ItemBodyStruct`) updates the global definition, causing **all items of that ID to share the stats**.
- **Fixing "All Items Update"**:
  - Requires a custom DLL to intercept `OP_ItemPacket`.
  - Server must send **Base Stats** in `ItemBodyStruct` (use `inst->GetUnscaledItem()`) to prevent global cache pollution.
  - Server must send **Custom Stats** separately (e.g., in `EvolvingItem_Struct` or appended data).
  - DLL must apply custom stats to the local instance only.
- **Immediate Updates**: `Client::SendItemScale` must call `inst->ApplyCustomStats()` to ensure `m_scaledItem` includes custom data before serialization.
