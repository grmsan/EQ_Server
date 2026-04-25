# Agent Handoff Protocol

**Last Updated:** 2026-04-25
**Management Doc:** [PROJECT_MANAGEMENT.md](PROJECT_MANAGEMENT.md)
**Assignment Board:** [PROJECT_WORKSTREAMS.md](PROJECT_WORKSTREAMS.md)

Use this when handing work to another agent or when resuming work yourself after a gap.

---

## Start Of Assignment

Before changing files:

1. Read the assigned packet in [PROJECT_WORKSTREAMS.md](PROJECT_WORKSTREAMS.md).
2. Read the linked tracker and implementation docs.
3. Run `git status --short` and assume unrelated dirty files belong to someone else.
4. State the exact scope you will touch.
5. If the packet is validation-only, do not make code changes unless the manager authorizes a follow-up implementation packet.

---

## Agent Rules

- Keep the assigned packet small. Do not expand scope because nearby code looks messy.
- Prefer validation evidence over assumptions.
- Update docs in the same turn when project state changes.
- Record exact commands, test IDs, rule values, and observed behavior.
- If a failure is found, create or recommend a focused follow-up packet.
- Do not mark a tracker case passed without objective evidence.
- Do not overwrite unrelated worktree changes.

---

## Required Closeout

Every agent final response should include:

1. Packet ID and role.
2. Files changed.
3. Tests/builds run, with results.
4. Tracker statuses updated.
5. New blockers or decisions needed.
6. Suggested next packet.

If no files changed, say that explicitly.

---

## Work Packet Template

```md
### PACKET-ID - Short Title

**Priority:** P1
**Role:** Validator | Explorer | Implementer | Doc Steward | Release Coordinator
**Status:** Ready
**Objective:** One concrete outcome.
**Scope:** Files, trackers, systems in bounds.
**Out of Scope:** Explicit exclusions.
**Inputs:**
- Link to dashboard/workstream/tracker/design docs
**Suggested Steps:**
1. Step one
2. Step two
**Done Criteria:** Evidence required to close the packet.
```

---

## Validation Evidence Template

```md
Test ID: I-07
Build/Branch: <branch or commit>
Rules Snapshot: <relevant rules>
Commands: <commands or in-game steps>
Observed: <actual result>
Expected: <expected result>
Evidence: <chat/log/output snippet>
Status: Pass | Fail
Next: <follow-up code/data/doc target if failed>
```

---

## Implementation Closeout Template

```md
Packet: MC-FIX-01
Files changed:
- path/to/file.cpp
- game_design/.../TEST_TRACKER.md

What changed:
- Concise behavior summary.

Verification:
- `cmake --build ...` -> passed/failed
- `#test ...` -> passed/failed/not run

Docs updated:
- Tracker case IDs
- Dashboard session log
- Workstream packet status

Remaining risk:
- Anything not validated.
```

---

## Manager Review Checklist

Use this before accepting an agent handoff:

- Did the agent stay inside the packet scope?
- Did they update the detailed tracker or explain why not?
- Is every pass/fail backed by evidence?
- Did they avoid unrelated cleanup?
- Are blockers phrased as decisions or concrete follow-up packets?
- Is the next packet obvious?

