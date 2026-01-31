# Multiclass System Vision

> *"Any class combination should feel intentional, not hacked together."*

---

## Why Multiclass?

Classic EverQuest locks players into a single class at character creation - a decision made at level 1 that defines the next 1000+ hours of gameplay. This creates:

- **Alt fatigue:** Players create multiple characters to experience different playstyles
- **Group dependency:** Solo players hit hard walls without specific class abilities
- **Stale gameplay:** No meaningful character evolution beyond gear and AAs

The multiclass system solves this by letting a single character grow into multiple roles over time, while preserving the identity and fantasy of each class.

---

## The Vision

A player should be able to:

1. **Start as any class** and have a complete, viable experience
2. **Earn additional classes** through meaningful progression (not just a GM command)
3. **Mix abilities** from owned classes in ways that feel powerful but fair
4. **Retain their identity** - a Warrior/Mage should feel different than a Mage/Warrior
5. **Never feel locked out** - if you earned something, you keep it (soft-lock, not delete)

---

## Core Design Principles

### 1. All Classes Are Equal
No "base class" advantages. A character who started as Warrior and added Mage should have identical capabilities to one who started as Mage and added Warrior.

**In practice:**
- Use `GetClassesBits()` everywhere, not `GetClass()`
- Union-of-classes for all eligibility checks
- No special treatment for the `character_data.class` field

### 2. Server Authoritative
The server decides what a character can do. The client only decides what to show.

**In practice:**
- All gameplay validation happens server-side
- DLL hooks are for UI visibility only
- A hacked client cannot grant abilities the server doesn't authorize
- EdgeStatLabel pushes truth from server to client

### 3. Soft-Lock, Not Delete
When a class is removed, abilities become *unusable*, not *unlearned*. The player's investment is preserved.

**In practice:**
- Spells remain scribed but can't be memorized/cast
- AAs remain purchased but can't be activated
- Skills retain their values but can't be used
- Re-adding the class immediately restores access

### 4. Minimal Client Modification
The stock RoF2 client should work. DLL enhances but isn't required for gameplay.

**In practice:**
- Without DLL: gameplay works, UI may be confusing
- With DLL: UI accurately reflects multiclass state
- No mandatory client patches or custom opcodes

### 5. Predictable Behavior
Players should be able to reason about what their character can do without consulting documentation.

**In practice:**
- If you own a class, you can use its stuff
- No hidden interactions or edge cases
- Clear feedback when something is blocked and why

---

## Player Experience Goals

### The New Player
*"I picked Warrior because I wanted to tank, but now I want to heal too."*

- Adding Cleric should feel like unlocking a new dimension, not starting over
- Spells appear, mana bar appears, healing works
- Existing Warrior abilities remain fully functional

### The Veteran
*"I've been playing this character for years. I want it to grow."*

- Multiclassing is a reward for investment, not a replacement
- Adds depth without invalidating previous choices
- Enables new group compositions and solo strategies

### The Optimizer
*"I want to find the perfect 3-class combo."*

- All combinations should be viable (no trap choices)
- Some synergies are better than others (depth)
- No combination should be mandatory (balance)

### The Casual
*"I just want to play, not manage spreadsheets."*

- Defaults should work well
- UI should clearly show what's usable
- No need to understand bitmasks or packet structures

---

## What "Done" Looks Like

When multiclass is complete, a player should be able to:

- [ ] Add/remove classes through in-game progression (not just GM commands)
- [ ] See all usable spells, AAs, skills, and items in the appropriate UI windows
- [ ] Cast any spell from any owned class at the correct level
- [ ] Use any skill from any owned class up to the best cap
- [ ] Activate any AA from any owned class
- [ ] Equip any item that any owned class can wear
- [ ] Use any discipline from any owned class
- [ ] See their full class list in /who, character select, and inspect
- [ ] Have their class list persist through logout, server restart, and client restart
- [ ] Remove a class and have abilities soft-lock (not delete)
- [ ] Re-add a class and have abilities immediately work again

---

## Non-Goals (What We're NOT Building)

### Not a "Classless" System
Classes still exist and matter. This is class *combination*, not class *elimination*. Each class retains its identity, spells, AAs, and flavor.

### Not Unlimited Classes
The cap exists (default: 3) for balance and identity reasons. A character with all 16 classes would be a generic blob, not a unique build.

### Not Automatic Synergy
We're not building custom abilities that only exist for specific combos. Monk/Wizard gets Monk stuff + Wizard stuff, not a special "Mystic Fist" ability.

### Not a Respec System
Adding/removing classes is a progression choice, not a respec. There may be costs, cooldowns, or requirements. This isn't "pick 3 classes daily."

### Not Perfect Balance
Some combinations will be stronger than others. That's fine. We aim for "all viable" not "all identical."

---

## Guiding Questions

When facing a design decision, ask:

1. **Does this treat all owned classes equally?**
   - If base class gets special treatment, reconsider.

2. **Is the server making this decision?**
   - If the client decides gameplay outcomes, reconsider.

3. **Can a player reason about this without documentation?**
   - If it requires wiki diving to understand, simplify.

4. **Does this preserve player investment?**
   - If removing a class deletes progress, use soft-lock instead.

5. **Would this work without the DLL?**
   - If the DLL is required for gameplay (not just UI), reconsider.

---

## Reference Implementations

### THJServer (The Heroes Journey)
Our primary reference. Proven multiclass implementation with years of player testing.
- Location: `extras/THJServer/`
- Docs: `extras/THJServer/docs/multiclass.md`

### Key Patterns from THJ
- `GetClassesBits()` for union-of-classes
- Data bucket for class storage
- EdgeStatLabel for server→client sync
- Soft-lock on class removal

---

## Success Metrics

How we'll know multiclass is working:

1. **Zero "but I own that class" bugs** - If you have the class bit, you can use the stuff
2. **No base class advantages** - Started-as-X vs added-X characters are identical
3. **Clean class removal** - Removing a class never corrupts character state
4. **UI accuracy** - What you see is what you can do (with DLL)
5. **Server authority** - Hacked clients can't bypass class restrictions

---

*This document defines the "why" and "what." For "how," see [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md).*
