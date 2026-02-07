# EveCraft Documentation Map

## 📍 You Are Here

This folder contains **complete documentation** for EveCraft, a dynamic infinite crafting system for EQEmu.

---

## 📚 Document Selection Guide

### ❓ "I want to understand if this is possible"
**→ Read:** [FEASIBILITY_ASSESSMENT.md](FEASIBILITY_ASSESSMENT.md) (15 min)

**Covers:**
- Yes/no feasibility with evidence
- Server-side only (no APIs needed)
- Performance & risk assessment
- Timeline & effort

---

### ❓ "How does the game experience work?"
**→ Read:** [EVERCRAFT_SYSTEM.md](EVERCRAFT_SYSTEM.md) (30 min)

**Covers:**
- Player perspective & discovery
- Skill progression (tiers 0-3)
- Success rates & failure mechanics
- Prohibited combinations
- Game balance considerations

---

### ❓ "How does it ensure combinations make sense?"
**→ Read BOTH (45 min total):**

**1. First:** [CONTEXTUAL_COMBINATION_ENGINE.md](CONTEXTUAL_COMBINATION_ENGINE.md) (25 min)
- Item attribute extraction (theme, type, rarity)
- Template matching system (priority-based rules)
- How templates work (code structure)
- How to extend with new templates

**2. Then:** [CONTEXTUAL_LOGIC_EXAMPLES.md](CONTEXTUAL_LOGIC_EXAMPLES.md) (20 min)
- Your three examples traced step-by-step
- Fire + Armor = themed armor (shown)
- Fire + Water = ashes (shown)
- Dragon Helm + Dragon Leather = enhanced helm (shown)

---

### ❓ "How do I implement this?"
**→ Read:** [TECHNICAL_IMPLEMENTATION.md](TECHNICAL_IMPLEMENTATION.md) (45 min)

**Covers:**
- Code integration points (zone/tradeskills.cpp)
- Data storage architecture (data_buckets, items table)
- Code structure (evercraft.h/cpp stubs)
- Stat generation algorithm
- GM commands & debugging
- Performance tuning

---

### ❓ "Give me a quick overview"
**→ Read:** [QUICK_START.md](QUICK_START.md) (5 min)

**Covers:**
- TL;DR for each role (designer, programmer, admin, manager)
- Quick links to relevant docs
- Common questions answered

---

### ❓ "I want a navigation guide"
**→ You're reading it!**
This document (INDEX.md) maps all documents by use case.

---

## 🎯 Quick Lookup Table

| Question | Document | Time |
|----------|----------|------|
| Is it feasible? | FEASIBILITY_ASSESSMENT.md | 15 min |
| How does the player experience work? | EVERCRAFT_SYSTEM.md | 30 min |
| How do combinations make sense? | CONTEXTUAL_COMBINATION_ENGINE.md | 25 min |
| Show me examples of your three combinations | CONTEXTUAL_LOGIC_EXAMPLES.md | 20 min |
| How do I code it? | TECHNICAL_IMPLEMENTATION.md | 45 min |
| I'm in a hurry | QUICK_START.md | 5 min |
| I need an overview | README.md | 10 min |

---

## 📖 Reading Paths by Role

### 👨‍💼 **Project Manager**
1. FEASIBILITY_ASSESSMENT.md (learn it's possible)
2. QUICK_START.md (understand resource needs)
3. EVERCRAFT_SYSTEM.md (understand gameplay value)

**Time: 50 min**
**Outcome:** Confidence in feasibility, resource planning

### 🎮 **Game Designer**
1. EVERCRAFT_SYSTEM.md (core design)
2. CONTEXTUAL_LOGIC_EXAMPLES.md (see it in action)
3. CONTEXTUAL_COMBINATION_ENGINE.md (understand templating)

**Time: 75 min**
**Outcome:** Design mastery, ability to tune balance

### 👨‍💻 **Programmer**
1. CONTEXTUAL_COMBINATION_ENGINE.md (understand logic)
2. TECHNICAL_IMPLEMENTATION.md (understand code structure)
3. CONTEXTUAL_LOGIC_EXAMPLES.md (reference while coding)

**Time: 90 min**
**Outcome:** Ready to implement

### 🛠️ **Server Admin**
1. README.md (overview)
2. QUICK_START.md (find admin section)
3. TECHNICAL_IMPLEMENTATION.md (configuration rules section)

**Time: 30 min**
**Outcome:** Know how to enable/configure

---

## 🌳 Document Hierarchy

```
FEASIBILITY_ASSESSMENT.md ← START HERE (yes/no decision)
    ↓
    ├─→ README.md (overview all docs)
    │
    ├─→ QUICK_START.md (role-based intro)
    │
    ├─→ EVERCRAFT_SYSTEM.md (game design & balance)
    │   ↓
    │   └─→ CONTEXTUAL_COMBINATION_ENGINE.md
    │       └─→ CONTEXTUAL_LOGIC_EXAMPLES.md
    │
    └─→ TECHNICAL_IMPLEMENTATION.md (code & architecture)
        ↓
        └─→ CONTEXTUAL_COMBINATION_ENGINE.md (reference)
```

---

## 📋 Document Details

| File | Purpose | Audience | Length |
|------|---------|----------|--------|
| **FEASIBILITY_ASSESSMENT.md** | Is it possible? Risk & timeline | Managers, decision-makers | 15 min |
| **README.md** | Overview & navigation | Everyone | 10 min |
| **QUICK_START.md** | Quick entry by role | Everyone in a hurry | 5 min |
| **EVERCRAFT_SYSTEM.md** | Game design & balance | Designers, players | 30 min |
| **CONTEXTUAL_COMBINATION_ENGINE.md** | How contextual logic works | Programmers, designers | 25 min |
| **CONTEXTUAL_LOGIC_EXAMPLES.md** | Step-by-step example flows | Programmers (reference), designers | 20 min |
| **TECHNICAL_IMPLEMENTATION.md** | Code architecture & integration | Programmers | 45 min |
| **INDEX.md** (this file) | Documentation map | Everyone needing direction | 5 min |

---

## 🎬 Getting Started

### Scenario 1: "I want to decide if we should build this"
```
1. Read FEASIBILITY_ASSESSMENT.md (yes/no)
2. If yes → Read EVERCRAFT_SYSTEM.md (gameplay value)
3. Done: You can make a decision
```

### Scenario 2: "We're building this, I need to understand the design"
```
1. Read EVERCRAFT_SYSTEM.md (game design)
2. Read CONTEXTUAL_LOGIC_EXAMPLES.md (concrete examples)
3. Read CONTEXTUAL_COMBINATION_ENGINE.md (deep understanding)
4. Done: You can critique & balance the design
```

### Scenario 3: "I need to implement this"
```
1. Skim CONTEXTUAL_LOGIC_EXAMPLES.md (understand what you're building)
2. Read CONTEXTUAL_COMBINATION_ENGINE.md (understand the algorithm)
3. Read TECHNICAL_IMPLEMENTATION.md (understand code structure)
4. Start coding: Use code stubs in TECHNICAL_IMPLEMENTATION.md
5. Reference CONTEXTUAL_LOGIC_EXAMPLES.md while implementing
6. Done: Implementation underway
```

### Scenario 4: "I have 5 minutes"
```
1. Read QUICK_START.md (find your role)
2. Jump to relevant document from link provided
3. Done: Oriented & linked to right doc
```

---

## 🔗 Cross-References

Most documents reference each other. Some key connections:

- **FEASIBILITY_ASSESSMENT.md** → "See [EVERCRAFT_SYSTEM.md]() for game design"
- **EVERCRAFT_SYSTEM.md** → "See [CONTEXTUAL_LOGIC_EXAMPLES.md]() for examples"
- **CONTEXTUAL_COMBINATION_ENGINE.md** → "See [CONTEXTUAL_LOGIC_EXAMPLES.md]() for walkthroughs"
- **TECHNICAL_IMPLEMENTATION.md** → "See [CONTEXTUAL_COMBINATION_ENGINE.md]() for algorithm reference"

---

## ✅ Completeness Checklist

These 7 documents cover:

- ✅ Feasibility & risk assessment
- ✅ Game design & balance
- ✅ Contextual logic explained
- ✅ Concrete examples
- ✅ Technical architecture
- ✅ Code stubs & integration
- ✅ Quick reference guides

**Nothing is missing.** All major questions are answered in the appropriate document.

---

## 🚀 Next Step

**Pick your scenario above and start reading.**

All documents are in: **`game_design/evercraft/`**

Enjoy!
