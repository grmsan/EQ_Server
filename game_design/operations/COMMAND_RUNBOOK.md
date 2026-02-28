# Operations Command Runbook

**Last Updated**: 2026-02-27

## Purpose

This runbook captures operational commands used for server setup and Bazaar migration work.

Use it with [WORK_TRACKER.md](WORK_TRACKER.md) so every command run has a recorded outcome.

## Prerequisites

1. Operator has required GM command access for placement commands.
2. Zone is `bazaar` when running Bazaar-only commands.
3. Current SQL script baseline is captured in session notes before making changes.

## Core Bazaar Placement Workflow

1. Zone to Bazaar.
2. Check staged count: `#bazaarpull status`
3. Pull one unplaced NPC near you: `#bazaarpull`
4. Target the intended NPC and move into final position.
5. Persist that location: `#spawnfix`
6. Repeat steps 2-5 until `#bazaarpull status` reports `0`.
7. Run `#repop` if you need a full in-zone refresh.

## Command Catalog

### Placement / Spawn Management

- `#bazaarpull [count|status]`
  - Purpose: Pull staged/unplaced Bazaar NPCs to your location.
  - Notes: Works only in Bazaar. `status` shows remaining staged count.
- `#spawnfix`
  - Purpose: Save targeted NPC current location/heading into `spawn2`.
  - Notes: Use immediately after moving each NPC to final position.
- `#npcspawn add [respawntime]`
  - Purpose: Create spawn entries for targeted NPC type at current location.
- `#npcspawn clone [respawntime]`
  - Purpose: Clone targeted NPC spawn at your current location.
- `#npcspawn create [respawntime]`
  - Purpose: Create new NPC type + spawn using targeted NPC as template.
- `#npcspawn remove [remove_spawngroups]`
  - Purpose: Remove targeted `spawn2` row (optional group cleanup).
- `#dbspawn2 [spawngroup_id] [respawn] [variance] [condition_id] [condition_min]`
  - Purpose: Spawn NPC from existing `spawn2`/spawngroup references.

### Verification / Discovery

- `#loc`
  - Purpose: Print exact coordinates and heading.
- `#list npcs <search>`
  - Purpose: Search currently spawned NPCs in zone.
- `#find npctype <search>`
  - Purpose: Search NPC type definitions by name.

### Appearance Tools (NPC Polish)

- `#randomize [mirror [npc_type_id]|look|gear]`
  - Purpose: Quickly apply a target visual profile.
  - Notes: No arg defaults to `mirror` (DB donor NPC appearance, class-aware). `mirror <npc_type_id>` clones that exact `npc_types.id` appearance. `look` keeps full chaos randomization. `gear` randomizes only materials/tints/weapons.
- `#randomizegear`
  - Purpose: Randomize target materials/tints/weapons only.
  - Notes: Equivalent to `#randomize gear`.
- `#randomfeatures`
  - Purpose: Randomize target facial features quickly.
  - Notes: Use `#npcedit featuresave` if you want to persist selected facial features.
- `#npcedit save`
  - Purpose: Save targeted NPC's current visual state and spawn position/heading in one command.
  - Notes: Best "commit" command after iterative `#randomize` passes.
- `#npcedit weapon [primary_model] [secondary_model]`
  - Purpose: Set displayed weapon models.
  - Notes: Uses weapon model numbers (`idfile` numeric, e.g. `IT10727` -> `10727`), not item IDs.
- `#npcedit featuresave`
  - Purpose: Persist current facial feature state to DB.
  - Notes: Use when you only want face/body feature persistence without full appearance commit.

### Zone Refresh / Recovery

- `#depop`
  - Purpose: Depop targeted NPC (optionally start timer).
- `#depopzone`
  - Purpose: Depop zone entities.
- `#repop [force]`
  - Purpose: Repop zone using DB spawns.

## Safety Guardrails

- Always run `#bazaarpull status` before and after a placement session.
- Do not bulk-delete spawn groups during placement passes.
- Save each NPC with `#spawnfix` before pulling next NPC.
- Keep SQL scripts idempotent and store them under `utils/sql/custom`.
- Record every applied script in `WORK_TRACKER.md` notes with timestamp.

## Rollback / Recovery

1. If placement goes wrong, stop and capture evidence first (`#loc`, screenshots, logs).
2. Re-run the relevant SQL bootstrap/repair script for Bazaar spawn staging if needed.
3. Use `#repop` after rollback data changes.
4. Re-run `B-01` through `B-03` in [WORK_TRACKER.md](WORK_TRACKER.md) to confirm recovery.
