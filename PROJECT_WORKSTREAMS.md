# Project Workstreams

**Last Updated:** 2026-04-25
**Management Doc:** [PROJECT_MANAGEMENT.md](PROJECT_MANAGEMENT.md)
**Agent Protocol:** [AGENT_HANDOFF.md](AGENT_HANDOFF.md)

This is the assignment board. Keep work packets small enough that another agent can pick one up, complete it, and leave evidence.

---

## Executive Priority Board

| Rank | Workstream | Priority | Current Objective | Decision Needed |
|---|---|---|---|---|
| 1 | Multiclass Validation | P1 | Validate latest item, XP, inventory, AA, skill, and presentation changes | Removed-class AA policy (`E-02`) |
| 2 | Pet / Mechanics Parity | P1 | Validate pet command persistence, familiar cleanup, pet assist, and failed taunt case | Whether player-facing `/pet assist` UI is required |
| 3 | Tooling / DLL Validation | P1 | Prove build-copy DLL and callback probes are reliable enough for validation work | None |
| 4 | Operations / Bazaar / Waypoints | P2 | Stabilize placement, waypoint, and THJ script smoke coverage | None |
| 5 | Infinite Progression Polish | P2 | Step 12 DLL visual polish and broad regression planning | Scope of visual polish |
| 6 | DEX Migration Follow-Ups | P2 | Audit bypassed crit/proc/twincast paths | DEX DoT twincast policy |
| 7 | QoL UI POCs | P3 | Validate waypoint/tool windows and choose which POCs graduate | Which UI pattern becomes productized |
| 8 | Future Stat Plans | P3 | Keep STR/STA/AGI/INT/WIS/CHA plans staged but inactive | Which stat follows DEX |

---

## Active Work Packets

### MC-VAL-01 - Latest Multiclass Gap Validation

**Priority:** P1
**Role:** Validator
**Status:** Ready
**Objective:** Validate the latest closed multiclass gaps before opening new implementation.
**Scope:** `game_design/multiclass/TEST_TRACKER.md`, `zone`, `world`, DLL runtime presentation.
**Out of Scope:** Broad THJ diff porting, AA level-requirement redesign.
**Inputs:**
- [PROJECT_DASHBOARD.md](PROJECT_DASHBOARD.md) multiclass section
- [game_design/multiclass/TEST_TRACKER.md](game_design/multiclass/TEST_TRACKER.md)
- [game_design/multiclass/IMPLEMENTATION_PLAN.md](game_design/multiclass/IMPLEMENTATION_PLAN.md)
**Suggested Steps:**
1. Build `zone`.
2. Build `world` and the DLL before display checks.
3. Run `I-01` to `I-07`, `X-04`, and `D-04`.
4. Update tracker statuses with evidence.
**Done Criteria:** Each listed test has pass/fail status plus reproducible notes. Any failure becomes a new implementation packet.

### MC-VAL-02 - AA Runtime And Entitlement Policy

**Priority:** P1
**Role:** Validator + Manager
**Status:** Ready
**Objective:** Confirm live AA behavior and capture the removed-class AA policy decision.
**Scope:** `zone/aa.cpp`, `zone/bonuses.cpp`, multiclass tracker `A-01` to `A-05`, `E-02`.
**Out of Scope:** Implementing AA level-requirement removal unless a failed validation case proves it is blocking.
**Suggested Steps:**
1. Run `A-01` to `A-05`.
2. Run `E-02` and record current behavior.
3. Present policy choice: soft-lock purchased AAs or refund/reset on class removal.
**Done Criteria:** AA validation evidence exists and `E-02` has a written decision or explicit manager blocker.

### MC-VAL-03 - Caster, Skill, Spell Cleanup, And Presentation

**Priority:** P1
**Role:** Validator
**Status:** Ready after MC-VAL-01 baseline
**Objective:** Validate caster UI, skill exposure, spell cleanup, and world presentation.
**Scope:** Tracker cases `C-03`, `K-05`, `E-01`, `D-01`, `D-02`, `D-03`, `B-08`, `G-01`.
**Done Criteria:** Tracker evidence confirms behavior or produces focused bug packets.

### MECH-PET-01 - Pet And Familiar Parity Validation

**Priority:** P1
**Role:** Validator
**Status:** Ready
**Objective:** Validate recent pet parity fixes and close or re-scope the known failed `MECH-11`.
**Scope:** [game_design/mechanics/TEST_TRACKER.md](game_design/mechanics/TEST_TRACKER.md), cases `MECH-07` to `MECH-15`.
**Out of Scope:** Adding player-facing `/pet assist` until the manager chooses that policy.
**Done Criteria:** `MECH-07` to `MECH-15` have current pass/fail evidence; `MECH-11` is either fixed or converted to an implementation packet.

### TOOL-DLL-01 - DLL Build And Callback Probe Validation

**Priority:** P1
**Role:** Validator / Implementer if tooling fails
**Status:** Ready
**Objective:** Confirm DLL build/copy and probe tests are reliable for multiclass and QoL validation.
**Scope:** [game_design/tooling/TEST_TRACKER.md](game_design/tooling/TEST_TRACKER.md), `TOOL-05`, `TOOL-07` to `TOOL-11`, Server Manager build-copy flow.
**Done Criteria:** Tooling tracker records pass/fail evidence. Any failed build-copy workflow has a focused fix packet.

### OPS-WAYPOINT-01 - Bazaar And Waypoint Operations Smoke

**Priority:** P2
**Role:** Validator / Operator
**Status:** Ready after P1 validation queue is moving
**Objective:** Confirm Bazaar service NPCs, waypoint discovery, and placement tooling remain operational.
**Scope:** [game_design/operations/WORK_TRACKER.md](game_design/operations/WORK_TRACKER.md), [game_design/quests/TEST_TRACKER.md](game_design/quests/TEST_TRACKER.md), [game_design/qol/TEST_TRACKER.md](game_design/qol/TEST_TRACKER.md).
**Suggested Cases:** `O-01`, `O-02`, `B-01` to `B-03`, `QST-01` to `QST-05`, `QOL-01`, `QOL-03`, `QOL-03A`.
**Done Criteria:** Smoke path is documented and blockers are split into ops/data/script packets.

### IP-POLISH-01 - Infinite Progression Step 12 Scope

**Priority:** P2
**Role:** Explorer / Manager
**Status:** Waiting on P1 queue
**Objective:** Define the actual Step 12 DLL visual polish deliverables before assigning implementation.
**Scope:** [game_design/infinite_progression/IMPLEMENTATION_STEPS.md](game_design/infinite_progression/IMPLEMENTATION_STEPS.md), [game_design/infinite_progression/TEST_TRACKER.md](game_design/infinite_progression/TEST_TRACKER.md), DLL docs.
**Done Criteria:** A small set of visual polish packets exists with acceptance criteria.

### DEX-AUDIT-01 - DEX Migration Follow-Up Audit

**Priority:** P2
**Role:** Explorer
**Status:** Waiting on P1 queue
**Objective:** Audit the known DEX migration bypasses and recommend implementation order.
**Scope:** [TODO_DEX_MIGRATION.md](TODO_DEX_MIGRATION.md), combat files, mechanics tracker.
**Done Criteria:** Each DEX follow-up has an owner packet, risk level, and test case.

### QOL-UI-01 - UI POC Graduation Review

**Priority:** P3
**Role:** Explorer / Manager
**Status:** Waiting
**Objective:** Decide which waypoint/tool-window POCs should become supported features.
**Scope:** [game_design/qol/TEST_TRACKER.md](game_design/qol/TEST_TRACKER.md), [game_design/dll/](game_design/dll/), DLL source.
**Done Criteria:** Keep/kill/punt recommendation for each POC with implementation packets only for kept items.

---

## Decision Log

### 2026-04-25 - Management Layer Added
Decision: Use `PROJECT_WORKSTREAMS.md` as the assignment board and keep detailed trackers as execution evidence.
Why: The project had detailed docs but no stable priority/ownership layer.
Impact: Agents should receive packet IDs instead of broad area requests when possible.
Follow-up packet: All active packets above.

---

## Packet Status Values

| Status | Meaning |
|---|---|
| Ready | Can be assigned now |
| In Progress | Assigned or actively being worked |
| Blocked | Needs decision, environment, or dependency |
| Review | Work done; needs manager review |
| Done | Done criteria met and docs updated |
| Superseded | Replaced by another packet |

---

## How To Assign An Agent

Use this format:

```md
Assign packet: MC-VAL-01
Role: Validator
Read first: PROJECT_MANAGEMENT.md, PROJECT_WORKSTREAMS.md, AGENT_HANDOFF.md, game_design/multiclass/TEST_TRACKER.md
Do not touch: unrelated code or trackers
Deliver: tracker evidence, changed files list, new packets for failures
```

