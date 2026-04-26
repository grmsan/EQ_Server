# Project Management Operating Model

**Last Updated:** 2026-04-25
**Audience:** Project owner, coordinating agents, implementation agents, validation agents

This document defines how work is prioritized, assigned, updated, and reviewed. It does not replace detailed design docs or trackers; it sits above them so work can be split into clear chunks and handed to agents without losing project state.

---

## Management Goals

1. Keep one high-level view that a project owner can review quickly.
2. Convert broad areas into work packets that can be assigned to agents.
3. Require every agent to leave the docs more accurate than they found them.
4. Separate implementation, validation, design decisions, and operations.
5. Avoid broad refactors unless a tracked work packet explicitly calls for them.

---

## Source Of Truth

| Layer | Document | Purpose |
|---|---|---|
| Executive entry point | [PROJECT_DASHBOARD.md](PROJECT_DASHBOARD.md) | What matters now, current priority order, quick return-after-break context |
| Work assignment | [PROJECT_WORKSTREAMS.md](PROJECT_WORKSTREAMS.md) | Workstreams, active packets, dependencies, owner/agent briefs |
| Agent protocol | [AGENT_HANDOFF.md](AGENT_HANDOFF.md) | Instructions every agent should follow before, during, and after work |
| Validation details | `game_design/**/TEST_TRACKER.md`, `WORK_TRACKER.md` | Concrete pass/fail cases and evidence |
| Technical plans | `game_design/**/IMPLEMENTATION*.md`, `README.md`, `PORT_CHECKLIST.md` | Design and implementation details |
| Session history | Dashboard session log plus tracker notes | What changed and what remains |

Rule: if a detail conflicts, use the more specific tracker/implementation plan for behavior, then update the higher-level management docs so they stop disagreeing.

---

## Priority Levels

| Priority | Meaning | Manager Action |
|---|---|---|
| P0 | Broken core workflow, blocks validation, or risks data loss | Assign immediately; no unrelated work |
| P1 | Active release work or high-value feature validation | Assign next; keep scope tight |
| P2 | Important follow-up, polish, or automation | Schedule after P1 validation clears |
| P3 | Research, concept, backlog cleanup | Keep visible but do not interrupt active work |
| Hold | Waiting on design decision, environment, or external dependency | Do not assign implementation until unblocked |

---

## Work Packet Standard

Every assigned task should be one packet with:

- **Packet ID:** Stable ID from [PROJECT_WORKSTREAMS.md](PROJECT_WORKSTREAMS.md).
- **Objective:** One outcome, not a theme.
- **Scope:** Files, trackers, or systems the agent may touch.
- **Out of Scope:** What not to change.
- **Inputs:** Required docs/tests/build commands.
- **Done Criteria:** Objective evidence required before closing.
- **Update Requirements:** Which docs must be updated before handoff.

Good packet size: 30 minutes to 4 hours for validation or focused code work. Larger efforts should be split into sequenced packets.

---

## Agent Roles

| Role | Use For | Typical Output |
|---|---|---|
| Explorer | Read-only investigation, diff review, root-cause narrowing | Findings, file references, recommended packet |
| Implementer | Focused code/data/doc changes in an owned scope | Patch, build/test result, tracker updates |
| Validator | In-game tests, command tests, smoke/regression runs | Pass/fail evidence, screenshots/log snippets where useful |
| Doc Steward | Dashboard/tracker/roadmap cleanup | Updated docs and conflict notes |
| Release Coordinator | Pre-merge checklist, risk review, packaging readiness | Release gate status and blockers |

Agents can do more than one role, but each packet should name the primary role.

---

## Update Ritual

Every agent must finish with these updates:

1. Update the detailed tracker or implementation plan touched by the work.
2. Add or update one dashboard session-log entry if meaningful project state changed.
3. Update [PROJECT_WORKSTREAMS.md](PROJECT_WORKSTREAMS.md) packet status if the packet moved.
4. Record blockers as explicit decisions needed, not vague TODOs.
5. Include exact build/test commands run, or state that tests were not run.

If the agent only investigates, the update can be a concise finding note in the relevant packet rather than a broad doc rewrite.

---

## Weekly Executive Review

Review these sections in order:

1. Dashboard priority board and active work items.
2. Workstream packet statuses in [PROJECT_WORKSTREAMS.md](PROJECT_WORKSTREAMS.md).
3. Any `Decision Needed` entries.
4. Failed or blocked tracker cases.
5. Work packets ready for assignment.

Executive decisions should be recorded as:

```md
### YYYY-MM-DD - Decision Title
Decision: ...
Why: ...
Impact: ...
Follow-up packet: ...
```

Use [PROJECT_WORKSTREAMS.md](PROJECT_WORKSTREAMS.md) for active decisions and archive resolved details in the relevant implementation plan when they become permanent design.

---

## Current Management Assessment

The project is not missing detailed work. It is missing assignment discipline. The highest-value change is to stop treating large areas like "multiclass" or "mechanics" as single tasks and instead assign small validation-first packets with clear done criteria.

Current management stance:

- THJ gap discovery is the top active workstream. Classify gaps before assigning more broad implementation.
- Runtime validation remains important, but some cases are parked until live RoF2 or headless-client access is available.
- Pet/mechanics parity stays high priority because it has a known failed tracker case and visible THJ UX delta.
- Tooling/DLL validation supports both gap confirmation and runtime validation, so it should be assigned when it unblocks those tracks.
- Infinite progression, DEX, and broader QoL should not interrupt THJ gap discovery unless you intentionally reprioritize them.
