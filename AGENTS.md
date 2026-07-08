# AGENTS.md

Guidance for coding agents working in this repository.

## Command Rules

- Always prefix shell commands with `rtk`.
- In command chains, prefix every segment: `rtk git status && rtk git diff`.
- Use `rtk proxy <cmd>` when a command needs raw output or RTK has no useful filter.
- Prefer `rg`/`rtk grep` and `rtk find` for search.

## Repository Overview

This is an EQEmulator server repository. The core server is C++ and CMake-based,
with supporting Lua/Perl quest scripting, SQL/data utilities, Python helper
scripts, and an optional Windows client DLL under `extras/eq-core-dll-main`.

Important paths:

- `zone/`, `world/`, `loginserver/`, `ucs/`, `queryserv/`, `eqlaunch/`: core C++ services.
- `common/`, `libs/`, `shared/`, `shared_memory/`: shared C++ code.
- `quests/`, `plugins/`, `lua_modules/`: quest and scripting content.
- `utils/`, `tools/`: development, database, and migration utilities.
- `game_design/`: design docs, trackers, validation plans, and project-specific notes.
- `extras/eq-core-dll-main`: optional client-side DLL diagnostics/features.
- `tests/`: CMake/C++ test targets.

## Project Docs To Check

- `README.md`: high-level project overview and tool notes.
- `BUILD.md`: build dependencies and CMake examples.
- `AGENT_HANDOFF.md`: agent workflow, packet handoff, validation evidence, and closeout format.
- `PROJECT_MANAGEMENT.md`, `PROJECT_WORKSTREAMS.md`, `PROJECT_DASHBOARD.md`: project coordination docs.
- Relevant `game_design/**/README.md` or trackers before changing game design behavior.

## Workflow Expectations

- Run `rtk git status --short` before edits and preserve unrelated user changes.
- Keep changes scoped to the requested packet/task.
- Do not do unrelated cleanup or broad refactors while fixing a narrow issue.
- Prefer existing local patterns, helpers, naming, and CMake conventions.
- Update the relevant tracker/design doc in the same turn when project state changes.
- Record exact commands and objective validation evidence when closing work.

## Build And Test Notes

- CMake is the primary build system; see `BUILD.md`.
- Common configure options include `EQEMU_BUILD_TESTS=ON`, `EQEMU_BUILD_LOGIN=ON`, and `EQEMU_BUILD_ZLIB=ON`.
- Windows builds commonly use Visual Studio/MSBuild and may use vcpkg.
- Linux builds commonly use Unix Makefiles.
- For optional client DLL work, build from `extras/eq-core-dll-main` with its PowerShell helper.

When possible, verify C++ changes with the narrowest relevant build/test target first, then broaden if the touched code is shared.

## Database And Runtime Tools

- `eqemu_config.json` is the default local config file for helper scripts.
- `tools/db_viewer.py` can inspect configured database tables and run ad-hoc SQL.
- Be careful with write SQL; helper scripts execute what they are given.
- Local logs and generated files may exist in `logs/`, `build/`, `.tokensave/`, and other tool directories.

## Closeout Format

For implementation or validation work, include:

- Packet/task ID if applicable.
- Files changed.
- Tests/builds run and result.
- Tracker/docs updated.
- Blockers or decisions needed.
- Suggested next packet or next concrete follow-up when useful.


## File editing rules for Codex on Windows

When editing files, avoid using shell quoting as the transport for file contents.

Preferred order:

1. Use apply_patch for small and medium edits.
2. For large or generated file content, create a temporary Python script using pathlib to write the file.
3. For repeated mechanical edits, use a small temporary Python script.
4. Do not use PowerShell Set-Content, Add-Content, echo, here-strings, Out-File, or long inline quoted commands to write files unless the user explicitly requests it.
5. After editing, show git diff or summarize the exact files changed.
6. Run the smallest relevant validation command.
7. If apply_patch fails on Windows, do not keep retrying with fragile PowerShell quoting. Fall back to a temporary Python script.
