# Progression Systems Documentation

This folder contains complete documentation for implementing account-wide progression systems.

**For Junior Developers:** Start with the [IMPLEMENTATION_WALKTHROUGH.md](IMPLEMENTATION_WALKTHROUGH.md) for a step-by-step guide.

## Quick Links

| Document | Purpose | Read This When... |
|----------|---------|-------------------|
| [ACCOUNT_PROGRESSION.md](ACCOUNT_PROGRESSION.md) | Design concepts and system overview | Understanding what to build |
| [DATABASE_SCHEMA.md](DATABASE_SCHEMA.md) | SQL tables, indexes, sample data | Setting up the database |
| [API_REFERENCE.md](API_REFERENCE.md) | C++ classes, Lua functions, commands | Writing code that uses the system |
| [CONFIGURATION.md](CONFIGURATION.md) | Tuning values, server-type examples | Configuring without code changes |
| [TESTING_GUIDE.md](TESTING_GUIDE.md) | Unit tests, integration tests, checklist | Testing your implementation |
| [IMPLEMENTATION_WALKTHROUGH.md](IMPLEMENTATION_WALKTHROUGH.md) | Step-by-step coding guide | Actually building it |

## Implementation Order

```
1. Read ACCOUNT_PROGRESSION.md     → Understand the system
2. Run SQL from DATABASE_SCHEMA.md → Create tables
3. Follow IMPLEMENTATION_WALKTHROUGH.md → Build incrementally
4. Reference API_REFERENCE.md      → For method signatures
5. Use TESTING_GUIDE.md            → Verify each phase
6. Tune with CONFIGURATION.md      → Adjust for your server
```

## Overview

### Account-Based Progression

Systems that track progress at the account level rather than per-character:

- **Account Unlocks** - One-time achievements (zone keys, expansion access)
- **Content Gates** - Restrictions until conditions are met (level caps, zone locks)
- **Account Perks** - Permanent bonuses for all characters (stats, AAs, teleports)

### Design Philosophy

1. **Earn Once, Benefit Always** - Tedious unlocks shouldn't be repeated on every alt
2. **Meaningful Milestones** - First completion should still feel rewarding
3. **Progressive Access** - Content unlocks naturally through gameplay
4. **Alt-Friendly** - New characters benefit from account progress

## Example Progression Flow

```
New Account
    └── Level Cap: 50, Classic Zones Only
         │
         ├── Kill Nagafen + Lady Vox
         │    └── Unlock: Level Cap 60, Kunark Zones
         │
         ├── Complete Sebilis Key Quest
         │    └── All characters can enter Sebilis
         │
         ├── Complete Neriak Heritage Chain
         │    └── All characters get Gate to Neriak AA
         │
         └── Complete Oggok Quest
              └── All characters get +2 STR
```

## Estimated Implementation Time

| Phase | Feature | Hours |
|-------|---------|-------|
| 1 | Database + Repositories | 4-6 |
| 2 | Core Manager Class | 4-6 |
| 3 | Account Unlocks | 2-4 |
| 4 | Zone Access Checks | 2-4 |
| 5 | Non-Stackable Perks | 3-4 |
| 6 | Stackable Perks + Diminishing | 4-6 |
| 7 | Veteran Bonuses | 4-6 |
| 8 | Lua Bindings | 4-6 |
| 9 | Commands | 2-3 |
| 10 | Testing + Polish | 4-8 |
| **Total** | | **35-55 hours** |

## Implementation Status

- [ ] Database tables created (Phase 1)
- [ ] Repository classes created (Phase 1)
- [ ] AccountProgressionManager class (Phase 2)
- [ ] Account unlocks working (Phase 3)
- [ ] Zone entry gating working (Phase 4)
- [ ] Account perks working (Phases 5-6)
- [ ] Veteran bonuses working (Phase 7)
- [ ] Lua API complete (Phase 8)
- [ ] Commands implemented (Phase 9)
- [ ] All tests passing (Phase 10)

## Related Documentation

- [Quest Systems](../quests/README.md) - Implementing unlock quests
- [Infinite Progression](../infinite_progression/) - Character scaling systems
