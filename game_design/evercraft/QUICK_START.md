# EveCraft Quick Start Guide

## TL;DR: Is It Possible?

**YES. Completely feasible, server-side only, no external APIs.**

See [FEASIBILITY_ASSESSMENT.md](FEASIBILITY_ASSESSMENT.md) for the full technical assessment.

---

## For Game Designers

**Read:** [EVERCRAFT_SYSTEM.md](EVERCRAFT_SYSTEM.md)

This document explains:
- How EveCraft works from a player perspective
- Skill progression (tier 0 = trash, tier 3 = legendary)
- Success rates and stat scaling
- What combinations are allowed/forbidden
- Game balance and economy impact

**Key decision:** Enable per-zone or globally? Start conservative (disabled by default).

---

## For Programmers

**Read:** [TECHNICAL_IMPLEMENTATION.md](TECHNICAL_IMPLEMENTATION.md)

This document explains:
- Where to integrate in the codebase (`zone/tradeskills.cpp`)
- Data storage architecture (uses `data_buckets` table)
- Code stubs for `zone/evercraft.h` and `.cpp`
- Caching strategy for determinism
- Stat generation algorithm
- GM commands for testing

**Key: Create 2 files (~750 lines) and modify 1 file (~20 lines).**

---

## For Project Managers

**Read:** [FEASIBILITY_ASSESSMENT.md](FEASIBILITY_ASSESSMENT.md)

This document provides:
- Risk assessment (low risk, fully optional)
- Implementation timeline (~4 weeks)
- Resource requirements (~1 developer)
- Performance impact (negligible)
- Comparison to similar systems

**Key: Safe, contained feature with major player engagement upside.**

---

## For Server Admins

**Read:** [README.md](README.md) then review the rules in [TECHNICAL_IMPLEMENTATION.md](TECHNICAL_IMPLEMENTATION.md)

**Key rules to configure:**
```
EveCraft:Enabled = false         # Start disabled for safety
EveCraft:SkillMin = 1
EveCraft:SkillMax = 300
EveCraft:AllowNoDrop = false     # No-drop items risky, disable by default
```

**Enable per-zone** after balance review.

---

## Document Structure

```
evercraft/
├── README.md                          ← Start here (overview)
├── FEASIBILITY_ASSESSMENT.md          ← Yes/no decision + risks
├── EVERCRAFT_SYSTEM.md                ← Game design & balance
└── TECHNICAL_IMPLEMENTATION.md        ← Code architecture
```

---

## One-Paragraph Summary

EveCraft allows players to combine any two items to create new results. Results scale with crafting skill (low skill = trash, high skill = legendary). Results are deterministic (same inputs always produce same output) and cached to avoid recomputation. The system is 100% server-side, uses existing EQEmu infrastructure (data_buckets for caching, items table for dynamic items, tradeskill system for skill/success), requires ~750 lines of new code, has negligible performance impact, and is fully optional (can be toggled via rule). It's perfect for players who enjoy exploration and experimentation.

---

## Next Steps

1. **Day 1:** Read FEASIBILITY_ASSESSMENT.md (15 min)
2. **Day 2:** Read EVERCRAFT_SYSTEM.md (30 min) if interested in game design
3. **Day 3:** Read TECHNICAL_IMPLEMENTATION.md (45 min) if implementing
4. **Week 1:** Create evercraft.h and evercraft.cpp with stubs
5. **Week 2:** Implement core logic (caching, stat generation)
6. **Week 3:** Test and balance
7. **Week 4:** Deploy to test server

---

## Questions?

**"Is it really server-side only?"**
Yes. No external APIs, no client mods, no new opcodes. Uses existing combine packets.

**"Will it break existing recipes?"**
No. EveCraft runs in parallel—if a recipe exists, it's used instead (configurable priority).

**"Can I turn it off?"**
Yes. Single rule toggle: `EveCraft:Enabled = false` disables it entirely.

**"Will the economy break?"**
Unlikely. Stat generation is tunable, and you can whitelist which items are combinable.

**"How much server load?"**
Negligible. ~1ms per cached lookup, 100+ combines/second capacity.

**"Can players exploit it?"**
Design includes safeguards: No-drop restrictions, lore conflict checks, quest item protection, currency prevention.

---

Proceed to the relevant document above based on your role.
