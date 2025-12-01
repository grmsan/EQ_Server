# EQ Classless Mod DLL (dinput8.dll)

## Overview
This project is a custom DLL for EverQuest (specifically targeting the Rain of Fear 2 / RoF2 client) that implements "Classless" server features. It is built upon the **MacroQuest2 (MQ2)** framework but heavily modified to serve as a game mod rather than just a macro utility.

The DLL functions as a **DirectInput8 Proxy** (`dinput8.dll`). When placed in the EverQuest game directory, the game loads this DLL instead of the system's DirectX library, allowing it to inject code, hook functions, and modify game behavior from the moment the process starts.

## Key Features & Modifications

### Core Mechanics
-   **DirectInput Proxy:** Intercepts game startup via `dinput8.dll` export forwarding.
-   **Detours & Hooks:** Uses Microsoft Detours to intercept and modify internal game functions.
-   **MQ2 Core:** Includes MQ2's command parsing, data types, and plugin system (Map, ItemDisplay, etc.).

### Game Patches (Found in `eqgame.cpp`)
-   **Class/Race/Deity Unlocks:** Modifies character creation and selection logic (`SetCCreateCameraHook`, `SelectCharacterHook`).
-   **Custom Starting Location:** Forces new characters to spawn at specific coordinates (likely a custom hub zone).
-   **Stat Cap Removal:** Patches to remove or increase caps for Heroic Stats (Stamina, Int, Wis), HP, Mana, and Endurance.
-   **Merchant Hacks:** Allows selling items to merchants that are normally restricted (`ValueSellMerchantHook`).
-   **UI Modifications:**
    -   Disables specific windows (Adventure, Leadership, Tribute, etc.) via `CXWndActivateHook`.
    -   Skips splash screens (`SkipSplash`).
    -   Modifies UI behavior for "Classless" compatibility.
-   **Anti-Detection / Spoofing:** Includes logic to spoof HWID and MAC addresses sent to the server (Opcode `0xf13`).

### Recent Customizations
-   **Tooltip Hack:** Displays Item IDs in item tooltips (Implemented in `MQ2ItemDisplay.cpp`).
-   **Test Pulse:** "Hello World" debug message in the pulse loop (Implemented in `MQ2Pulse.cpp`).

## Build Instructions

### Prerequisites
-   Visual Studio 2022 (with C++ Desktop Development workload).
-   Microsoft Detours library (included in `../Detours`).
-   DirectX 9 SDK (included in `../dependencies/dx9`).

### Building
1.  Open the solution `eqgame_dll.sln` or use the command line.
2.  Select **Release** configuration and **Win32** (x86) platform.
3.  Build the `eqgame_dll` project.

**Command Line:**
`powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" "eqgame_dll\eqgame_dll.vcxproj" /p:Configuration=Release /p:Platform=x86
`

### Output
The build produces `dinput8.dll` in the `eqgame_dll\Release\` directory.

## Installation
1.  Navigate to your EverQuest RoF2 game directory.
2.  Backup the existing `dinput8.dll` if one exists (standard Windows installs do not have this file in the game folder, but other mods might).
3.  Copy the built `dinput8.dll` from `eqgame_dll\Release\` to the game directory.
4.  Launch `eqgame.exe` (with `patchme` argument).

## Project Structure

-   **`eqgame_dll/`**: Main source code.
    -   **`dllmain.cpp`**: Entry point, handles DirectInput8 proxying.
    -   **`eqgame.cpp`**: Contains the bulk of the game-specific hooks, patches, and logic. **Start here to understand game mods.**
    -   **`MQ2Main.cpp`**: Core MQ2 initialization and loop.
    -   **`MQ2*.cpp`**: Various MQ2 modules (Chat, Windows, Pulse, etc.).
    -   **`MQ2ItemDisplay.cpp`**: Handles item tooltips and display.
-   **`common/`**: Shared headers and utility files.
-   **`Detours/`**: Microsoft Detours library for function hooking.
-   **`dependencies/`**: External libs (DirectX, Boost, etc.).

## Troubleshooting
-   **Linker Errors:** Ensure all dependencies are correctly referenced. Common issues involve `extern "C"` linkage mismatches in MQ2 globals (e.g., `szItemSlot`).
-   **Crash on Load:** Check `dinput8.log` (created by the DLL) for initialization errors. Ensure the client version matches the offsets defined in the code.
