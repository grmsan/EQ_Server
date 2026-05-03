# Project Workstreams

**Last Updated:** 2026-04-25
**Management Doc:** [PROJECT_MANAGEMENT.md](PROJECT_MANAGEMENT.md)
**Agent Protocol:** [AGENT_HANDOFF.md](AGENT_HANDOFF.md)

This is the assignment board. Keep work packets small enough that another agent can pick one up, complete it, and leave evidence.

---

## Executive Priority Board

| Rank | Workstream | Priority | Current Objective | Decision Needed |
|---|---|---|---|---|
| 1 | THJ Gap Discovery | P1 | Identify meaningful missing THJ behavior and classify each gap before more broad implementation | Which THJ deltas are must-port vs intentional divergence |
| 2 | Multiclass Validation | P1 | Preserve current validation queue, but use it after THJ gap triage identifies what needs proof | Removed-class AA policy (`E-02`) |
| 3 | Pet / Mechanics Parity | P1 | Identify remaining THJ pet/combat gaps, then validate recent fixes | Whether player-facing `/pet assist` UI is required |
| 4 | Tooling / DLL Validation | P1 | Prove build-copy DLL and callback probes are reliable enough for validation work | None |
| 5 | Operations / Bazaar / Waypoints | P2 | Identify missing THJ script/API/data gaps and stabilize smoke coverage | None |
| 6 | Infinite Progression Polish | P2 | Step 12 DLL visual polish and broad regression planning | Scope of visual polish |
| 7 | DEX Migration Follow-Ups | P2 | Audit bypassed crit/proc/twincast paths | DEX DoT twincast policy |
| 8 | QoL UI POCs | P3 | Validate waypoint/tool windows and choose which POCs graduate | Which UI pattern becomes productized |
| 9 | Future Stat Plans | P3 | Keep STR/STA/AGI/INT/WIS/CHA plans staged but inactive | Which stat follows DEX |

---

## Active Work Packets

### THJ-GAP-00 - Gap Discovery Inventory And Method

**Priority:** P1
**Role:** Explorer / Doc Steward
**Status:** Ready
**Objective:** Establish the current THJ comparison inputs and turn them into a reliable gap triage method.
**Scope:** [THJ_GAP_REGISTER.md](THJ_GAP_REGISTER.md), [game_design/multiclass/PORT_CHECKLIST.md](game_design/multiclass/PORT_CHECKLIST.md), `tools/output/multiclass_references.csv`, `tools/output/multiclass_references.json`, `extras/THJServer/`.
**Out of Scope:** Code changes or porting.
**Suggested Steps:**
1. Confirm reference paths and report freshness.
2. Identify top diff clusters by file/system.
3. Update the gap register with triage categories and next packets.
**Done Criteria:** Gap discovery method is documented, stale inputs are called out, and at least the first 10 high-value gap candidates are registered or explicitly dismissed.

### THJ-GAP-01 - Multiclass Core Gap Triage

**Priority:** P1
**Role:** Explorer
**Status:** Review
**Objective:** Identify meaningful THJ gaps in multiclass core systems before more validation or implementation.
**Scope:** `zone/client.cpp`, `zone/client.h`, `zone/client_packet.cpp`, `zone/client_process.cpp`, `zone/aa.cpp`, `zone/spells.cpp`, `zone/client_mods.cpp`, `zone/bonuses.cpp`, `world/worlddb.cpp`, `world/clientlist.cpp`, related DLL class-display hooks.
**Out of Scope:** Runtime validation and code changes.
**Inputs:**
- [THJ_GAP_REGISTER.md](THJ_GAP_REGISTER.md)
- [game_design/multiclass/PORT_CHECKLIST.md](game_design/multiclass/PORT_CHECKLIST.md)
- `extras/THJServer/`
**Latest Finding:** Register updated with five fix/decision deltas plus one validation delta to split next: `/who` payload policy, stale base-class reintroduction in `world/clientlist.cpp`, bard item-click multiclass guards, AA-table timer preload parity, toggle-AA disabled-state parity, and the remaining memorize-time refresh check. Remaining trainer, mana, char-select, and core class-bit paths stay validation-first or dismissed as stronger local implementations.
**Done Criteria:** Each meaningful multiclass gap is classified as Must Port, Validate First, Design Decision, Intentional Divergence, or Ignore, with a recommended next packet.

### THJ-GAP-02 - Combat, Pet, And Mechanics Gap Triage

**Priority:** P1
**Role:** Explorer
**Status:** Ready
**Objective:** Identify remaining THJ combat/pet/mechanics gaps, especially around pet command persistence, assist, familiar ownership, proc behavior, and taunt.
**Scope:** `zone/pets.cpp`, `zone/npc.cpp`, `zone/npc.h`, `zone/mob.cpp`, `zone/mob.h`, `zone/attack.cpp`, `zone/special_attacks.cpp`, `zone/spell_effects.cpp`, [game_design/mechanics/TEST_TRACKER.md](game_design/mechanics/TEST_TRACKER.md).
**Out of Scope:** Adding `/pet assist` or changing combat behavior before classification.
**Done Criteria:** Gap register includes all meaningful pet/mechanics THJ deltas, including a manager-facing recommendation on `/pet assist` UX.

### THJ-GAP-03 - Bazaar, Quest, Waypoint, And Script API Gap Triage

**Priority:** P1
**Role:** Explorer
**Status:** Ready
**Objective:** Identify missing THJ APIs, scripts, DB rows, and waypoint/Bazaar behaviors that affect imported content.
**Scope:** `quests/`, `plugins/`, `zone/lua_*`, `zone/perl_*`, `zone/questmgr.cpp`, `zone/zonedb.cpp`, `utils/sql/custom/`, [game_design/quests/TEST_TRACKER.md](game_design/quests/TEST_TRACKER.md), [game_design/operations/WORK_TRACKER.md](game_design/operations/WORK_TRACKER.md).
**Done Criteria:** Missing script/API/data deltas are classified and converted into implementation, ops, or validation packets.

### THJ-GAP-04 - DB, Rules, And Infrastructure Gap Triage

**Priority:** P1
**Role:** Explorer
**Status:** Ready
**Objective:** Identify THJ gaps in DB manifests, schema helpers, rules, opcodes, and supporting tooling.
**Scope:** `common/database*`, `common/database_schema.h`, `common/ruletypes.h`, `common/eq_packet_structs.h`, `utils/patches/`, `tools/`, SQL migrations.
**Done Criteria:** Must-port infrastructure gaps are separated from upstream-only noise and documented in the gap register.

### THJ-GAP-05 - Bots, Mercs, And Lower-Priority Server Gap Triage

**Priority:** P2
**Role:** Explorer
**Status:** Ready after `THJ-GAP-01` to `THJ-GAP-04`
**Objective:** Review remaining THJ diff areas that may matter later but should not block core parity decisions.
**Scope:** bot, merc, raid, corpse, aggro, trade-skill, and lower-priority scripting diffs listed in [game_design/multiclass/PORT_CHECKLIST.md](game_design/multiclass/PORT_CHECKLIST.md).
**Done Criteria:** Remaining diffs are classified as backlog, ignore, or concrete future packets.

### MC-DEC-01 - `/who` Payload Policy

**Priority:** P1
**Role:** Manager / Explorer
**Status:** Ready
**Objective:** Decide whether multiclass `/who` and friends payloads should preserve stock-client compatibility class IDs or switch to THJ-style multiclass bitmask payloads for DLL-driven class-label parity.
**Scope:** `world/clientlist.cpp`, DLL class-name override behavior, multiclass presentation tracker cases `D-02` and related `/who` validation notes.
**Out of Scope:** Implementing the chosen policy.
**Inputs:**
- [THJ_GAP_REGISTER.md](THJ_GAP_REGISTER.md)
- `extras/THJServer/world/clientlist.cpp`
- `extras/eq-core-dll-main/src/eqgame.cpp`
**Done Criteria:** Written decision records the preferred `/who` payload policy, why it is preferred, and whether `MC-VAL-03` should validate stock-client compatibility or THJ/DLL parity next.

### MC-FIX-01 - World Multiclass Mask Hydration

**Priority:** P1
**Role:** Implementer
**Status:** Review
**Objective:** Remove stale base-class reintroduction from the world-side multiclass helper used by `/who` and friends display/filtering.
**Scope:** `world/clientlist.cpp`, [THJ_GAP_REGISTER.md](THJ_GAP_REGISTER.md).
**Out of Scope:** Changing the `/who` payload policy itself.
**Inputs:**
- [THJ_GAP_REGISTER.md](THJ_GAP_REGISTER.md)
- `extras/THJServer/world/clientlist.cpp`
**Latest Finding:** `world/clientlist.cpp::GetMulticlassBitsOrBase()` no longer ORs the legacy base-class bit back into persisted multiclass ownership, now short-circuits to the compatibility/base class when `Custom:MulticlassingEnabled` is off, and falls back from invalid zero masks to the compatibility/base class. Next runtime proof should target `CORE-01` and `UI-01` to confirm `/who` class filters and friends display stop surfacing removed starting classes. Rebuild is currently blocked if `build/bin/RelWithDebInfo/world.exe` is left running.
**Done Criteria:** World-side helper uses authoritative persisted multiclass ownership without silently OR-ing the legacy base class back in, and the gap register/workstream notes reflect the fix.

### MC-FIX-02 - Bard Item-Click Guard Parity

**Priority:** P1
**Role:** Implementer
**Status:** Ready
**Objective:** Port the THJ multiclass-safe bard item-click guard behavior so bard shortcuts only apply when the active cast context qualifies.
**Scope:** `zone/client_packet.cpp`, multiclass spell/item validation notes, [THJ_GAP_REGISTER.md](THJ_GAP_REGISTER.md).
**Out of Scope:** Broader spell or AA UI refresh work.
**Inputs:**
- [THJ_GAP_REGISTER.md](THJ_GAP_REGISTER.md)
- `extras/THJServer/zone/client_packet.cpp`
**Done Criteria:** Bard item-click interruption/casting logic matches the intended THJ multiclass guard behavior and a focused validation packet can verify it.

### MC-FIX-03A - AA Timer Preload Parity

**Priority:** P1
**Role:** Implementer
**Status:** Review
**Objective:** Add THJ-style dynamic AA timer cache preload before rebuilding the AA table, reusing existing local timer storage infrastructure.
**Scope:** `zone/aa.cpp`, `zone/client.h`, multiclass AA validation notes, [THJ_GAP_REGISTER.md](THJ_GAP_REGISTER.md).
**Out of Scope:** Adding toggle-AA disabled-state persistence.
**Inputs:**
- [THJ_GAP_REGISTER.md](THJ_GAP_REGISTER.md)
- `extras/THJServer/zone/aa.cpp`
**Latest Finding:** `zone/aa.cpp::SendAlternateAdvancementTable()` now preloads the persisted dynamic AA timer cache via `LoadDynamicAATimers()` before serializing ranks when `Custom:UseDynamicAATimers` is enabled. Next proof should run `AA-02` to confirm cooldown-family stability across zoning, relogging, and class-mutation-driven AA table rebuilds.
**Done Criteria:** AA table rebuild path preloads dynamic timer cache in a THJ-equivalent way without duplicating the existing local timer repository logic.

### MC-FIX-03B - Toggle-AA Disabled-State Parity

**Priority:** P1
**Role:** Implementer
**Status:** Ready
**Objective:** Add the missing THJ-style toggle-AA disabled-state cache and persistence path.
**Scope:** `zone/aa.cpp`, `zone/client.h`, any required repository wiring for per-character AA disabled state, [THJ_GAP_REGISTER.md](THJ_GAP_REGISTER.md).
**Out of Scope:** Reworking dynamic AA timer assignment.
**Inputs:**
- [THJ_GAP_REGISTER.md](THJ_GAP_REGISTER.md)
- `extras/THJServer/zone/aa.cpp`
**Done Criteria:** Local multiclass AA flow has an explicit toggle-AA disabled-state cache/persistence path comparable to THJ, and follow-up validation can exercise it.

### MC-VAL-01 - Latest Multiclass Gap Validation

**Priority:** P1
**Role:** Validator
**Status:** Blocked / Parked During THJ Gap Discovery
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
**Blocker:** Fresh `zone`, `world`, and DLL builds succeeded on 2026-04-25, but the current environment has no active RoF2 client session and no built headless client binary, so runtime multiclass cases cannot be executed objectively.

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

### 2026-04-25 - Priority Shift To THJ Gap Discovery
Decision: Make THJ gap identification the top active priority before further broad validation or implementation.
Why: Runtime validation is blocked by client availability, while source-level THJ comparison can proceed immediately and will produce clearer implementation priorities.
Impact: Assign `THJ-GAP-*` explorer packets first. Keep existing validation packets parked until gap triage identifies what must be proven in-game.
Follow-up packet: `THJ-GAP-00`, then `THJ-GAP-01` to `THJ-GAP-04`.

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
Assign packet: THJ-GAP-01
Role: Explorer
Read first: PROJECT_MANAGEMENT.md, PROJECT_WORKSTREAMS.md, AGENT_HANDOFF.md, THJ_GAP_REGISTER.md, game_design/multiclass/PORT_CHECKLIST.md
Do not touch: unrelated code or trackers
Deliver: classified gaps, file/function references, recommended next packets
```
