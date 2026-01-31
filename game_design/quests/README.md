# Quest Systems Documentation

This folder contains complete documentation for implementing quests in EQEmu servers.

**For Junior Developers:** Start with the [IMPLEMENTATION_WALKTHROUGH.md](IMPLEMENTATION_WALKTHROUGH.md) for a step-by-step guide.

## Quick Links

| Document | Purpose | Read This When... |
|----------|---------|-------------------|
| [SCRIPTED_QUESTS.md](SCRIPTED_QUESTS.md) | Lua script-based quests | Building NPC dialogue, turn-ins |
| [TASK_SYSTEM.md](TASK_SYSTEM.md) | Database-driven tasks with UI | Building tracked kill/collect quests |
| [EXAMPLES.md](EXAMPLES.md) | 10+ practical examples | Looking for patterns to copy |
| [API_REFERENCE.md](API_REFERENCE.md) | Complete Lua function reference | Looking up method signatures |
| [DATABASE_REFERENCE.md](DATABASE_REFERENCE.md) | SQL schemas, queries, templates | Working with quest database tables |
| [TESTING_GUIDE.md](TESTING_GUIDE.md) | GM commands, debugging, checklist | Testing your quests |
| [IMPLEMENTATION_WALKTHROUGH.md](IMPLEMENTATION_WALKTHROUGH.md) | Step-by-step creation guide | Building your first quest |

## Which Document Do I Need?

```
"I want to create a quest"
    └── Start with IMPLEMENTATION_WALKTHROUGH.md

"I need to look up a Lua function"
    └── Use API_REFERENCE.md

"I need SQL for a task"
    └── Use DATABASE_REFERENCE.md → Task Templates

"My quest isn't working"
    └── Use TESTING_GUIDE.md → Common Issues

"I want to see how X is done"
    └── Use EXAMPLES.md
```

## System Overview

### 1. Scripted Quests (Lua/Perl)

**Location:** `/quests/` folder, organized by zone

Traditional EverQuest quests implemented through server-side scripts. NPCs react to player dialogue, item turn-ins, and other events. Players must remember quest requirements and track progress manually.

**Best For:**
- Classic quest experiences
- NPC dialogue and lore
- Item turn-in quests
- Simple faction rewards
- Zone-specific encounters

**Key Features:**
- No client UI tracking
- Script-driven logic
- Flexible and customizable
- Supports Lua (preferred) or Perl

### 2. Task System (Database-driven)

**Location:** Database tables `tasks`, `task_activities`

Modern quest system introduced in Gates of Discord expansion. Tasks appear in the client's Quest Journal with objectives, progress tracking, and completion status.

**Best For:**
- Multi-step quests with progress tracking
- Kill X mobs objectives
- Exploration objectives
- Shared/Group tasks
- Timed quests
- Complex quest chains

**Key Features:**
- Client UI integration (Quest Journal)
- Automatic progress tracking
- Multiple activity types (kill, collect, explore, etc.)
- Level requirements and replay timers
- Shared Tasks for groups

## When to Use Each System

| Scenario | Recommended System |
|----------|-------------------|
| "Bring me 10 wolf pelts" with no tracking | Scripted Quest |
| "Kill 10 wolves" with progress bar | Task System |
| NPC tells story and gives faction | Scripted Quest |
| Multi-zone quest chain with waypoints | Task System |
| One-time item exchange | Scripted Quest |
| Repeatable daily quest | Task System |
| Group expedition content | Task System (Shared Task) |

## Combining Both Systems

The two systems can work together:
- A scripted NPC can assign a task via `eq.task_selector()`
- Tasks can trigger script events (`event_task_complete`)
- Scripts can update task progress manually

## File Organization

```
/quests/
├── global/           # Scripts that apply to all zones
│   ├── global_npc.lua
│   └── global_player.lua
├── {zone_name}/      # Zone-specific scripts
│   ├── {NPC_Name}.lua
│   └── {NPC_Name}.pl
├── plugins/          # Reusable Perl functions
├── lua_modules/      # Reusable Lua modules
└── task_sql/         # Example task definitions
```

## Quick Start: Your First Quest

1. **Design** - What does the player do? What do they get?
2. **Create** - Script in `/quests/zonename/NPC_Name.lua`
3. **Test** - `#reloadqst zonename` and interact with NPC
4. **Iterate** - Fix issues, reload, test again

See [IMPLEMENTATION_WALKTHROUGH.md](IMPLEMENTATION_WALKTHROUGH.md) for detailed instructions.

## External Resources

- [EQEmu Quest API Wiki](https://docs.eqemu.io/quest-api/)
- [Lua Quest Functions](https://docs.eqemu.io/quest-api/lua/)
- [Perl Quest Functions](https://docs.eqemu.io/quest-api/perl/)
- [Task System Database](https://docs.eqemu.io/schema/tasks/)

