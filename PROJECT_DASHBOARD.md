<!-- markdownlint-disable MD007 MD022 MD029 MD031 MD032 MD049 MD058 MD060 -->

# EQ Server Project Dashboard

**Your single entry point for picking up where you left off.**

> This dashboard consolidates all TODO items, progress tracking, and implementation plans across the project. Each section links to detailed trackers for deep-dive work.

---

## 🚀 Return After Break (5-Minute Quickstart)

**When you come back after days/weeks away:**

1. **Review the Executive Priority Board** in [PROJECT_WORKSTREAMS.md](PROJECT_WORKSTREAMS.md)
2. **Check Decisions Needed** below and choose what should be unblocked
3. **Pick or assign one work packet** from [PROJECT_WORKSTREAMS.md](PROJECT_WORKSTREAMS.md)
4. **Read the Session Log** only if you need detailed recent history
5. **Run sanity tests** if you made code changes:
   - Quick: `#test smoke` in-game
   - Or: `cmake --build build --target zone --config RelWithDebInfo -- /m:1 /p:BuildInParallel=false /p:TrackFileAccess=false`

**Key Commands:**
- Start server: `python server_manager.py` → Server Control → Start All
- Build: `cmake --build build --config RelWithDebInfo -- /m:1 /p:BuildInParallel=false /p:TrackFileAccess=false`
- In-game tests: `#test smoke`, `#test combat`, `#test automated`

---

## 🧭 Management View

Use this section when acting as project owner or manager.

| Document | Use It For |
|----------|------------|
| [PROJECT_MANAGEMENT.md](PROJECT_MANAGEMENT.md) | Operating model, priority definitions, update ritual, manager review cadence |
| [PROJECT_WORKSTREAMS.md](PROJECT_WORKSTREAMS.md) | Executive priority board, active packets, dependencies, decisions |
| [AGENT_HANDOFF.md](AGENT_HANDOFF.md) | Instructions and templates for assigning work to agents |

### Decisions Needed

| Decision | Current Recommendation | Blocks |
|----------|------------------------|--------|
| Which THJ gaps are must-port | Run `THJ-GAP-00` to establish method, then `THJ-GAP-01` to `THJ-GAP-04` by subsystem | Implementation priority, release scope |
| Removed-class AA entitlement policy | Validate current behavior first with `E-02`, then choose soft-lock vs refund/reset | `MC-VAL-02`, release readiness |
| Player-facing `/pet assist` command/UI | Keep as explicit THJ delta until pet validation proves server behavior stable | Mechanics parity closure |
| AA level requirement removal | Defer research until multiclass validation pass completes unless AA tests fail | Future AA design work |
| DLL visual polish scope | Defer until P1 validation work is moving | Infinite progression Step 12 |

### Ready To Assign

| Packet | Priority | Role | Summary |
|--------|----------|------|---------|
| `THJ-GAP-00` | P1 | Explorer / Doc Steward | Establish current THJ comparison inputs and gap triage method |
| `THJ-GAP-01` | P1 | Explorer | Identify meaningful multiclass core gaps from THJ |
| `THJ-GAP-02` | P1 | Explorer | Identify combat, pet, familiar, proc, and taunt gaps from THJ |
| `THJ-GAP-03` | P1 | Explorer | Identify Bazaar, quest, waypoint, and script API gaps from THJ |
| `THJ-GAP-04` | P1 | Explorer | Identify DB, rules, opcode, schema, and infrastructure gaps from THJ |

---

## 📋 Session Log

*Record what you worked on. Newest entries at top.*

| Date | Area | Accomplishments | Next Steps |
|------|------|-----------------|------------|
| 2026-04-25 | THJ gap discovery | Reprioritized the project board so source-level THJ gap identification is now the top priority. Added [THJ_GAP_REGISTER.md](THJ_GAP_REGISTER.md) and new `THJ-GAP-*` explorer packets to classify missing THJ behavior before assigning more implementation. | Assign `THJ-GAP-00` first if the comparison inputs need review, otherwise assign `THJ-GAP-01` for multiclass core gap triage. Runtime validation packets remain parked until client access is available or gap triage says they are needed. |
| 2026-04-25 | Multiclass validation | Ran packet `MC-VAL-01` through the assigned prep work: `zone`, `world`, and EQ core DLL all built cleanly on the current branch, and the multiclass tracker was updated for `I-01` to `I-07`, `X-04`, and `D-04`. Validation could not proceed past build evidence because there is no active RoF2 client process and no built headless client binary in this environment. | Unblock a live RoF2 client session or provide a controllable headless-client path, then rerun `MC-VAL-01` before opening new multiclass implementation work. |
| 2026-04-25 | Project management | Added a management layer with an operating model, workstream assignment board, and agent handoff protocol so work can be assigned by packet ID instead of broad project area. Dashboard now has a manager-facing decision view and ready-to-assign packet list. | Use [PROJECT_WORKSTREAMS.md](PROJECT_WORKSTREAMS.md) for assignment, start with `MC-VAL-01`, `MC-VAL-02`, `MECH-PET-01`, or `TOOL-DLL-01`, and require agents to follow [AGENT_HANDOFF.md](AGENT_HANDOFF.md). |
| 2026-04-20 | Pets / Mechanics validation | Resolved the latest uncommitted pet regressions from review: removed the stale legacy taunt overwrite during pet restore, tightened familiar lookup/cleanup so variant familiar spells cannot coexist or orphan each other, and limited `/pet assist` retargeting to real owner actions instead of passive target selection. Confirmed `zone` builds cleanly on the stable non-parallel CMake/MSBuild path and updated server-manager build behavior to use that same invocation. | Run `MECH-07` to `MECH-15`, with special focus on `MECH-11`, `MECH-14`, and `MECH-15`, then update the mechanics tracker from actual in-game evidence. |
| 2026-04-21 | Pets / Mechanics validation | Addressed the uncommitted pet review findings: familiars now carry normal ownership semantics for pet-aware server paths, familiar buff fade now dismisses the matching familiar directly, and the pet taunt parity patch was narrowed back to the engaged target instead of force-peeling nearby mobs. Added dedicated mechanics cases for familiar cleanup and ownership validation. | Re-run the expanded pet mechanics pack `MECH-07` to `MECH-13`, with special attention to `MECH-11`, `MECH-12`, and `MECH-13`, then update tracker status from actual in-game results. |
| 2026-04-21 | Pets / THJ parity | Ported the remaining server-side pet parity foundation: persisted pet command states, summon-time restore for taunt/hold/ghold/focus/spellhold, dedicated familiar spawning from active buffs, client-owned pet equipment-aware melee skill formulas, and assist-aware pet engagement hooks for melee/spell combat. `zone` now builds cleanly after the changes. | Run the new mechanics pet test cases `MECH-07` to `MECH-11`, apply the pet-command-state DB migration before persistence tests, and keep the remaining THJ delta explicit: add a player-facing `/pet assist` command/UI surface if we want full THJ UX parity. |
| 2026-04-21 | Multiclass / DLL UI | Added a DLL-side inventory label projection so the inventory window now rewrites `IW_Class` and `IW_ClassAbbr` from the multiclass mask instead of showing only the compatibility/base class; EQ core DLL Release Win32 v143 build succeeded after the change. | Validate the new inventory presentation with `D-04`, plus a quick char-select and `/who` spot check to confirm all client-facing class displays stay aligned. |
| 2026-04-20 | Combat / Multiclass | Patched pet taunt parity so taunting pets now force stronger hate on their main target and nearby mobs attacking the owner, and fixed invalid post-class-removal spell re-memorize attempts so they now fail immediately with an error and reset the spellbar UI instead of hanging. | Re-run the pet tanking scenario plus `E-01`; keep `C-05` deprioritized unless it starts blocking spell-targeting or combat validation. |
| 2026-04-21 | Multiclass | Fixed the XP cap drift in `zone/exp.cpp`: server `Character:MaxExpLevel` / `MaxLevel` rules stay authoritative again, and client max level now acts only as an extra clamp instead of replacing the configured exp cap. | Rebuild `zone`, then run `X-04` plus a quick sanity pass around leveling/AA gain to confirm capped characters no longer overshoot the intended server exp limit. |
| 2026-04-21 | Multiclass | Closed the remaining augment-gating gap in the server path: `OP_AugmentItem` now rejects class-restricted augments unless the player owns a qualifying class, and it also restores THJ's wear-slot safety guard before finalizing the augmented item. | Rebuild `zone`, then run the new `I-07` augment validation case plus the rest of `I-01` to `I-07` to confirm item and augment gating behave correctly in-game. |
| 2026-04-20 | Multiclass | Fixed the follow-up review issues in compatibility-class handling: the legacy `character_data.class` value is now derived deterministically from the owned-class bitmask instead of mutation history, and login hydration reuses the shared persisted-bucket fallback logic before syncing runtime multiclass state. | Re-run `C-01`, then relog and inspect char-select to confirm compatibility-class projection stays stable across class mutations and legacy bucket fallback cases. |
| 2026-04-20 | Multiclass | Removed the remaining legacy base-class leakage from compatibility paths: class mutations and login hydration now keep `character_data.class` synced to an actually owned compatibility class, and char-select shaping no longer ORs the removed base class back into `GestaltClasses`. | Rebuild with `world.exe` and `zone.exe` unlocked, then rerun `C-01` plus a relog/char-select check to confirm the compatibility class stays aligned after removing the original starting class. |
| 2026-04-20 | Multiclass | Removed the last base-class-only restriction from owned-class mutation: `GetClassesBits()`/`SetClassesBits()` now treat persisted `GestaltClasses` as authoritative, `#removeclass` can remove the starting class like any other owned class, and the only remaining safety rule is that a character must keep at least one class bit. | Re-run `C-01` and a relog pass on a character whose starting class was removed, then confirm downstream UI/display paths still behave correctly with a non-owned `character_data.class`. |
| 2026-04-20 | Multiclass | Final pre-test cleanup landed cleanly: `/waypointpoc`, `/toolwnd`, and `/gmdashboard` now reliably show the requested tool instead of toggling it closed, and class-removal spell cleanup now enforces the same class-plus-level entitlement rules used at memorize time. | Proceed to runtime validation, especially `E-01` plus the QoL SIDL window checks, then update tracker status from actual test results. |
| 2026-04-20 | Multiclass | Fixed the pre-testing review findings: GM dashboard actions now execute real client commands instead of going through `/say`, multiclass spell memorization now shares one eligibility helper with class-removal cleanup, the removed-spell warning text was corrected, and passive AA gating now checks current entitlement directly in the bonus path. | Move into runtime validation with `A-01` to `A-04`, `C-03`, `E-01`, `I-01` to `I-06`, `D-01`, and `D-02`; only reopen implementation if those tests fail. |
| 2026-04-20 | Multiclass | Verified world presentation is also further along than the backlog implied: world char-select shaping reads `GestaltClasses`, `/who` intentionally stays single-class on the wire for stock compatibility, and the DLL rewrites `/who` and char-select class labels to multiclass abbreviations. | Treat `/who` and character select as fresh-build validation items (`D-01`, `D-02`) unless runtime testing shows a real gap. |
| 2026-04-20 | Multiclass | Implemented spell soft-lock cleanup for class removal: removed-class spells remain scribed, but invalid memorized gems are now cleared immediately and in-progress invalid casts are interrupted. | Validate `E-01` alongside the AA, mana/UI, and item verification pass; keep AA entitlement policy as the remaining class-removal design decision. |
| 2026-04-20 | Multiclass | Verified more multiclass support already exists than the docs implied: server refreshes mana on class changes, the DLL overrides mana display from server values, merchant class filtering uses owned-class bitmasks, and class-locked item clicks already check `GetClassesBits()`. | Treat mana/UI and most item gating as validation-first work; add missing tracker coverage for skills window and merchant filtering, then focus coding on true remaining gaps. |
| 2026-04-20 | Multiclass | Confirmed `#mystats` already exposes multiclass ownership details via the stats window/chat output, so that plan item is no longer an open implementation gap. | Keep the tracker case for validation and focus coding effort on mana/UI, item gating, and world presentation. |
| 2026-04-20 | Multiclass | Expanded the multiclass tracker to cover the remaining open plan items: level/XP cap behavior, character select, `/who`, `#mystats`, spell entitlement after class removal, and AA entitlement policy after class removal. | Continue implementation on runtime/item display paths and use the new tracker cases during testing. |
| 2026-04-20 | Multiclass | Added passive AA ownership gating so removed-class passive AAs no longer continue applying through the bonus path; implementation plan updated to reflect AA purchase/passive support in code. | Manually validate `A-01` to `A-04`, then move to `C-03` mana/UI verification. |
| 2026-04-20 | Multiclass | Audited multiclass docs and target files; confirmed today's work should focus on AA parity, mana/skill UI verification, item/equip gating, and world display parity. | Start with `zone/aa.cpp`, then validate `A-01` to `A-04`, then move to `C-03` and `I-01` to `I-05`. |
| _yyyy-mm-dd_ | _e.g., Multiclass_ | _Brief summary_ | _What to do next_ |
| | | | |
| | | | |

---

## 🔥 Active Work Items (Prioritized)

### HIGH PRIORITY

#### 1. THJ Gap Discovery (Active Priority)
- **Status**: Gap triage now top priority
- **THJ Parity Baseline**: Multiclass reference report currently shows 36 same / 66 differ / 10 THJ-only files; this is an input, not a final gap list.
- **Recent Work**: Added [THJ_GAP_REGISTER.md](THJ_GAP_REGISTER.md) and `THJ-GAP-*` work packets so agents can classify missing THJ behavior by subsystem.
- **Today Goal**: Identify which THJ deltas are must-port, validate-first, design decisions, intentional divergences, or ignorable.
- **Where We Stand**:
  - Runtime validation is partially blocked by lack of live RoF2/headless client access.
  - Source-level THJ comparison can proceed now using `extras/THJServer/`, `tools/output/multiclass_references.csv`, and [game_design/multiclass/PORT_CHECKLIST.md](game_design/multiclass/PORT_CHECKLIST.md).
  - Existing validation packets stay useful, but they should be driven by gap triage instead of broad assumptions.
- **Next Concrete Work**: Assign `THJ-GAP-00` to verify comparison inputs and method, then `THJ-GAP-01` for multiclass core gap triage.
- **After That**: Assign `THJ-GAP-02`, `THJ-GAP-03`, and `THJ-GAP-04`; convert meaningful gaps into implementation, validation, or design-decision packets.
- **Design Decision Still Needed**: what level of THJ parity we actually want for each gap class; not every diff should be ported.
- **Primary Files / Inputs**:
  - `extras/THJServer/` — reference implementation
  - `tools/output/multiclass_references.csv` — reference hit report
  - [THJ_GAP_REGISTER.md](THJ_GAP_REGISTER.md) — gap register and classifications
  - [game_design/multiclass/PORT_CHECKLIST.md](game_design/multiclass/PORT_CHECKLIST.md) — current parity checklist
- **Details**: [PROJECT_WORKSTREAMS.md](PROJECT_WORKSTREAMS.md)
- **Gap Register**: [THJ_GAP_REGISTER.md](THJ_GAP_REGISTER.md)

##### THJ Gap Immediate Next Steps
1. Run `THJ-GAP-00` to confirm the reference inputs and seed the first high-value gap candidates.
2. Run `THJ-GAP-01` for multiclass core: client class state, AA, spells, skills, item gates, world presentation.
3. Run `THJ-GAP-02` for mechanics and pets: assist, familiar, command persistence, proc/taunt behavior.
4. Run `THJ-GAP-03` for Bazaar, quests, waypoint, and script API gaps.
5. Run `THJ-GAP-04` for DB, rules, opcode, and infrastructure gaps.
6. Convert only meaningful gaps into implementation or validation packets.

#### 2. Multiclass System (Parked Validation Queue)
- **Status**: Core APIs implemented; runtime validation partially blocked by client access
- **THJ Parity**: Pending gap triage from `THJ-GAP-01`
- **Recent Work**: April multiclass cleanup closed compatibility-class leakage, class-removal spell cleanup, passive AA gating, augment gating, XP-cap drift, guild refresh projection, and inventory class-label projection.
- **Today Goal**: Preserve validation queue and use it after gap triage or client access unblocks runtime checks.
- **Where We Stand**:
  - Fresh `zone`, `world`, and DLL builds have succeeded.
  - Runtime multiclass validation cannot be completed objectively without live RoF2 or headless client access.
  - Phase 2 Spells is mostly complete; mana bar/UI sync appears to have both server and DLL support and now needs direct validation rather than broad implementation.
  - Phase 3 AA now has purchase, activation, window visibility, dynamic timers, and passive ownership gating implemented in code; the next step is manual validation.
  - Phase 4 Skills is mostly complete but still needs skills-window visibility verification.
  - Phase 5 Items/Equipment has more implementation in place than the docs previously reflected: equip paths, merchant class filtering, and item click class checks are already multiclass-aware; remaining work is mainly verification plus any uncovered edge cases.
  - World presentation also appears implemented through the world + DLL path; `/who` and character select now look like validation items rather than net-new coding work.
  - Class removal now preserves spellbook progress while immediately clearing invalid memorized gems; the remaining class-removal gap is the AA entitlement policy decision.
- **Next Concrete Work**: Rebuild `zone`; rebuild `world` and the DLL before presentation checks; validate `I-01` through `I-07`, `X-04`, and `D-04` first because those cover the latest closed gaps.
- **After That**: Finish AA/manual runtime validation (`A-01` through `A-05`, then `E-02`), skills/caster checks (`C-03`, `K-05`, `E-01`), and world presentation (`D-01`, `D-02`, `D-03`, `B-08`, `G-01`).
- **Design Decision Still Needed**: final policy for removed-class AA entitlements/refunds is now tracked explicitly and should come back to you before we lock behavior.
- **Primary Files**:
  - `zone/aa.cpp` — AA visibility, purchase, activation, passive ownership gating
  - `zone/client_mods.cpp` — skills/stat exposure follow-up
  - `zone/inventory.cpp` — class-restricted item/equip validation
  - `world/clientlist.cpp` — `/who` and character presentation parity
- **Details**: [game_design/multiclass/IMPLEMENTATION_PLAN.md](game_design/multiclass/IMPLEMENTATION_PLAN.md)
- **Testing**: [game_design/multiclass/test_tracker.md](game_design/multiclass/test_tracker.md)
- **Parity Tracking**: [game_design/multiclass/PORT_CHECKLIST.md](game_design/multiclass/PORT_CHECKLIST.md)

##### Multiclass Immediate Next Steps
1. Build `zone`; build `world` and the DLL before display checks.
2. Run the latest-gap validation set: `I-01` to `I-07`, `X-04`, and `D-04`.
3. Run AA verification: `A-01` to `A-05`, then record the current `E-02` removed-class AA behavior.
4. Verify caster/skill/spell cleanup: `C-03`, `K-05`, and `E-01`.
5. Verify presentation and projection: `D-01`, `D-02`, `D-03`, `B-08`, and `G-01`.
6. Research removing AA level requirements only after the validation pass, unless a failed AA test makes it blocking.

##### Multiclass Today Execution Order
1. **AA Pass**
  - Target: `zone/aa.cpp`
  - Goal: validate AA visibility, purchase, activation, passive ownership gating, and special multiclass AA cases.
  - Validate: `A-01`, `A-02`, `A-03`, `A-04`, `A-05`, and `E-02`.
2. **Caster UI / Skills Verification**
  - Target: `zone/client_mods.cpp` plus DLL/runtime verification.
  - Goal: confirm caster secondary classes expose mana correctly and that added-class skills are visible/usable.
  - Validate: `C-03`, `K-05`, and `E-01`.
3. **Item and Equip Rules**
  - Target: `zone/inventory.cpp`
  - Goal: verify existing union-of-classes equip/use gating, merchant filtering, and click restrictions without breaking race restrictions; patch only uncovered edge cases.
  - Validate: `I-01` through `I-07`.
4. **World / Presentation Parity**
  - Target: `world/clientlist.cpp` and any linked world-side class presentation code.
  - Goal: verify `/who` and related multiclass presentation are aligned with the current server/client behavior, including post-mutation guild roster refreshes and guild-members-list reloads.
  - Validate: `D-01`, `D-02`, `D-03`, `D-04`, `G-01`, `B-08`, plus any `/who` checks after rebuild.
5. **Regression and Tracker Sync**
  - Run `#test smoke`, targeted multiclass checks, and update tracker statuses and dashboard session log with results.

#### 3. DEX Migration Follow-Ups
- **Status**: New DEX precision system active (`Combat:UseNewDexFormulas`)
- **Blocked Items**:
  - Slay Undead (paladin AA) — bypassed in new path
  - Legacy pet crit behavior — bypassed
  - Class-specific crit nuances — need audit
  - Whirlwind divisor — needs skill/discipline hook
- **Next**: Playtest DEX crit/proc/twincast divisors, decide on `ENABLE_DEX_DOT_TWINCAST`
- **Details**: [TODO_DEX_MIGRATION.md](TODO_DEX_MIGRATION.md)

#### 4. Infinite Item Progression
- **Status**: Steps 1-11 complete (iLevel, Tiers, Power Slot XP, Essence/Salvage, Consume AAs, Ghost Copy, Drop Tiers, Vendors, Aug Merge, Zone/Boss Augs, Infusion)
- **Next**: Step 12 - DLL Visual Polish
- **Details**: [game_design/infinite_progression/IMPLEMENTATION_STEPS.md](game_design/infinite_progression/IMPLEMENTATION_STEPS.md)
- **Testing**: [game_design/infinite_progression/TEST_TRACKER.md](game_design/infinite_progression/TEST_TRACKER.md)

### MEDIUM PRIORITY

#### 5. Combat Mechanics Parity
- **Status**: THJ-aligned proc behavior ported
- **Focus Areas**: 2H/Bow procs, Pet/NPC weapon procs
- **Open THJ Delta TODO**: server-side pet assist behavior is now ported, but the player-facing `/pet assist` command/UI surface is still not exposed in the live tree. Either add the command path or deliberately decide to keep assist as an internal/default-only behavior.
- **Pet Validation Pack**: run `MECH-07` to `MECH-15` after any future pet parity change; do not treat THJ pet work as done without either an implementation or an explicit TODO entry here.
- **Current Pet Focus**: `MECH-11` is still marked failed in tracker history, and the latest uncommitted fix set added explicit familiar, restore-precedence, and passive-retarget regression coverage (`MECH-12` to `MECH-15`). Prioritize those before calling the pet pass stable.
- **Testing**: [game_design/mechanics/TEST_TRACKER.md](game_design/mechanics/TEST_TRACKER.md)

#### 6. Class-Specific Abilities
- **Status**: Custom warrior AAs (Heroic Throw, Colossal Smash) implemented
- **Testing**: [game_design/classes/TEST_TRACKER.md](game_design/classes/TEST_TRACKER.md)

#### 7. Quest/Waypoint System
- **Status**: THJ Bazaar + waypoint foundation complete
- **Testing**: [game_design/quests/TEST_TRACKER.md](game_design/quests/TEST_TRACKER.md)

### LOWER PRIORITY

#### 8. QoL Features
- **Items**: Bazaar and Back AA, Waypoint UI POCs, SIDL tool windows
- **Testing**: [game_design/qol/TEST_TRACKER.md](game_design/qol/TEST_TRACKER.md)

#### 9. Stat Implementation Plans
- **Status**: DEX active, others in design
- **Details**:
  - [game_design/stats/DEX_IMPLEMENTATION_PLAN.md](game_design/stats/DEX_IMPLEMENTATION_PLAN.md)
  - [game_design/stats/STR_IMPLEMENTATION_PLAN.md](game_design/stats/STR_IMPLEMENTATION_PLAN.md)
  - [game_design/stats/STA_IMPLEMENTATION_PLAN.md](game_design/stats/STA_IMPLEMENTATION_PLAN.md)
  - [game_design/stats/AGI_IMPLEMENTATION_PLAN.md](game_design/stats/AGI_IMPLEMENTATION_PLAN.md)
  - [game_design/stats/INT_IMPLEMENTATION_PLAN.md](game_design/stats/INT_IMPLEMENTATION_PLAN.md)
  - [game_design/stats/WIS_IMPLEMENTATION_PLAN.md](game_design/stats/WIS_IMPLEMENTATION_PLAN.md)
  - [game_design/stats/CHA_IMPLEMENTATION_PLAN.md](game_design/stats/CHA_IMPLEMENTATION_PLAN.md)

---

## 📊 Progress Overview

| Domain | Status | Tests Passed | Last Updated |
|--------|--------|--------------|--------------|
| **THJ Gap Discovery** | 🟡 Active | Gap register started; `THJ-GAP-*` packets ready | 2026-04-25 |
| **Multiclass** | 🟡 Active Dev | Auto: 2/51, Partial: 9/51 | 2026-04-21 |
| **Infinite Progression** | 🟢 Steps 1-11 Done | — | 2026-03-06 |
| **Operations** | 🟢 Active | — | 2026-02-28 |
| **Classes** | 🟡 Testing | CL-06 ✓ | 2026-02-27 |
| **Mechanics** | 🟡 Testing | `MECH-11` failed, `MECH-12` to `MECH-15` added | 2026-04-20 |
| **Quests** | 🟡 Testing | — | 2026-02-28 |
| **QoL** | 🟡 Testing | — | 2026-03-13 |
| **Tooling** | 🟢 Stable | TOOL-01,03,04 ✓ | 2026-02-28 |
| **DEX Migration** | 🟡 Follow-ups | — | — |

Legend: 🟢 Stable/Complete | 🟡 Active/In Progress | 🔴 Blocked

---

## 📁 All Trackers (Quick Links)

### Test Trackers
| Tracker | Location | Smoke Run |
|---------|----------|-----------|
| Multiclass | [test_tracker.md](game_design/multiclass/test_tracker.md) | C-01, C-02, C-03, S-02, P-01... |
| Operations | [WORK_TRACKER.md](game_design/operations/WORK_TRACKER.md) | O-01, O-02, B-01, B-02... |
| Classes | [TEST_TRACKER.md](game_design/classes/TEST_TRACKER.md) | CL-01, CL-02, CL-06 |
| Mechanics | [TEST_TRACKER.md](game_design/mechanics/TEST_TRACKER.md) | MECH-01, MECH-03, MECH-04 |
| Quests | [TEST_TRACKER.md](game_design/quests/TEST_TRACKER.md) | QST-01, QST-03, QST-05, QST-06 |
| QoL | [TEST_TRACKER.md](game_design/qol/TEST_TRACKER.md) | QOL-01, QOL-02, QOL-03 |
| Tooling | [TEST_TRACKER.md](game_design/tooling/TEST_TRACKER.md) | TOOL-01, TOOL-02, TOOL-03 |
| Infinite Progression | [TEST_TRACKER.md](game_design/infinite_progression/TEST_TRACKER.md) | Step 1-3 Core |

### Implementation Plans & Checklists
| Document | Purpose |
|----------|---------|
| [PROJECT_MANAGEMENT.md](PROJECT_MANAGEMENT.md) | Project operating model and manager review rules |
| [PROJECT_WORKSTREAMS.md](PROJECT_WORKSTREAMS.md) | Executive priority board and assignable work packets |
| [AGENT_HANDOFF.md](AGENT_HANDOFF.md) | Agent assignment, evidence, and closeout protocol |
| [THJ_GAP_REGISTER.md](THJ_GAP_REGISTER.md) | THJ gap classification register and triage workflow |
| [IMPLEMENTATION_PLAN.md](game_design/multiclass/IMPLEMENTATION_PLAN.md) | Multiclass master technical document |
| [PORT_CHECKLIST.md](game_design/multiclass/PORT_CHECKLIST.md) | THJServer parity file-by-file |
| [IMPLEMENTATION_CHECKLIST.md](game_design/abilities/IMPLEMENTATION_CHECKLIST.md) | Spell/Discipline/AA implementation guide |
| [DLL_AUTOMATION_MATRIX.md](game_design/tooling/DLL_AUTOMATION_MATRIX.md) | Test automation roadmap (86 cases) |
| [TESTING_SYSTEM.md](game_design/TESTING_SYSTEM.md) | Tracker meta-system documentation |

### Standalone TODOs
| File | Purpose |
|------|---------|
| [TODO_DEX_MIGRATION.md](TODO_DEX_MIGRATION.md) | DEX stat migration follow-ups |

---

## 📦 In-Progress Data Files

*Temporary work files in root directory — context for future sessions:*

| File | Purpose | Status |
|------|---------|--------|
| `tmp_quest_spawn_audit.json` | Quest spawn audit data (pre-apply) | Temporary |
| `tmp_quest_spawn_audit_post_apply.json` | Quest spawn audit (post-apply, versioned) | Temporary |
| `tmp_quest_spawn_audit_post_apply_anyver.json` | Quest spawn audit (post-apply, any version) | Temporary |
| `tmp_quest_numeric_pairs.json` | Quest numeric pair mapping | Temporary |
| `tmp_quest_spawn_autogen_meta.json` | Auto-generated quest spawn metadata | Temporary |

*These files support quest/spawn migration work. Safe to delete after quest system is stable.*

---

## 🛠️ Development Quick Reference

### Build Commands
```bash
# Full build
cmake --build build --config RelWithDebInfo -- /m:1 /p:BuildInParallel=false /p:TrackFileAccess=false

# Zone only (fastest for combat/quest changes)
cmake --build build --target zone --config RelWithDebInfo -- /m:1 /p:BuildInParallel=false /p:TrackFileAccess=false

# World only
cmake --build build --target world --config RelWithDebInfo -- /m:1 /p:BuildInParallel=false /p:TrackFileAccess=false
```

### Server Management
```bash
# Start server (GUI)
python server_manager.py

# Start server (script)
python start_server.py
python stop_server.py
```

### In-Game Test Commands
```bash
#test smoke          # Quick sanity (15-25 min)
#test combat         # Combat/proc/pet/AA checks
#test automated      # All automated tests
#test 1-10           # Run range of tests
#multiclassdiag      # Multiclass diagnostics
#addclass list       # Show current classes
```

### DLL Build (Client-Side)
- Use Server Manager → `Build+Copy DLL` button
- Or: Build `extras/eq-core-dll-main/eq-core-dll-visualstudio2022.sln` (x86/Release)

---

## 🔗 Key Documentation

| Topic | Primary Document |
|-------|------------------|
| Project Setup | [README.md](README.md), [BUILD.md](BUILD.md) |
| Multiclass Quick Start | [game_design/multiclass/QUICK_START.md](game_design/multiclass/QUICK_START.md) |
| DLL Integration | [game_design/multiclass/DLL_INTEGRATION.md](game_design/multiclass/DLL_INTEGRATION.md) |
| Quest Systems | [game_design/quests/README.md](game_design/quests/README.md) |
| Combat/Stats | [game_design/stats/](game_design/stats/) |
| THJ Reference | `extras/THJServer/` (read-only reference implementation) |

---

## 📝 Notes

*Use this space for persistent notes that don't fit elsewhere:*

- Assign work by packet ID from [PROJECT_WORKSTREAMS.md](PROJECT_WORKSTREAMS.md), not by broad area name.
- Agents should follow [AGENT_HANDOFF.md](AGENT_HANDOFF.md) and update tracker evidence before closing work.
-
-

---

*Last dashboard update: 2026-04-25*
*Dashboard created to consolidate 28 scattered tracking documents into one entry point.*
