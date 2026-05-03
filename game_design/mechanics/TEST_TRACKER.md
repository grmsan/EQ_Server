# Mechanics Test Tracker

**Status**: Active Testing
**Tracker Area**: Mechanics
**Tracker State**: Active
**Last Updated**: 2026-05-02

## Purpose

Tracks combat-system and proc-engine behavior that is cross-class and not multiclass-specific.

## Status Rules

- `Not Run`: Ready for validation but not executed in the current build.
- `In Progress`: Actively being tested or partially executed.
- `Blocked`: Cannot be validated due to environment, data, or design dependency.
- `Pass`: Expected behavior confirmed with objective evidence.
- `Fail`: Behavior mismatches expected results or produces a regression.

## Evidence Format

`Observed:` one-line actual result.
`Evidence:` exact chat/log snippet, command output, or screenshot reference.
`Commands:` minimal command sequence to reproduce.
`Next:` immediate code/data target if blocked or failed.

## Current Validation Focus

1. Run **MECH-01** and **MECH-07** after every build to confirm proc parity and pet persistence.
2. Run **MECH-03** and **MECH-04** after any inventory or lore-rule changes.
3. Run **MECH-09**, **MECH-10**, and **MECH-11** after combat AI or aggro changes.
4. Run **MECH-12**, **MECH-13**, and **MECH-14** after familiar or pet system changes.
5. **MECH-11** is currently `Fail` — pet taunt aggro hold is a known open issue.

## Session Header

**Date**: __________
**Tester**: __________
**Build/Branch**: __________
**Rules Snapshot**: `MultipleTwoHandedProcs=__`, `UseNewDexFormulas=__`

## Run Packs

### Smoke Run

Run: `MECH-01, MECH-03, MECH-04, MECH-07, MECH-14`

### Full Regression

Run all tests in this document.

## Open TODOs

- THJ parity delta still open: add a player-facing `/pet assist` command or equivalent UI surface if we want full THJ behavior parity. The server-side assist state and persistence are implemented, but the explicit toggle UX is not yet exposed in the live tree.

---

## 1) Proc System Parity

### [MECH-01] Proc Parity: 2H and Bow

**Goal**: Verify THJ-aligned proc behavior for 2H and bow weapons.
**Legacy ID**: `P-05`
**Steps**:

1. Verify `MultipleTwoHandedProcs = true`.
2. Equip a 2H weapon with a proc augment and fight a mob for multiple rounds — confirm proc fires.
3. Equip a bow with a proc augment and autofire — confirm ranged proc fires.

**Expected**: 2H and bow proc behavior follows configured parity expectations; both proc types fire at reasonable rates.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [MECH-02] Pet/NPC Weapon Instance Proc Parity

**Goal**: Verify pet/NPC attacks evaluate weapon-instance proc paths (including aug-based procs).
**Legacy ID**: `P-10`
**Setup**:

1. Use a pet or controlled NPC with an equipped weapon instance (not just raw item ID fallback).
2. Ensure weapon/aug proc effects are valid for level and proc rules.

**Steps**:

1. Engage a target and let pet/NPC perform sustained melee rounds.
2. Observe combat output for innate weapon procs and augment procs.
3. Repeat with offhand if applicable.

**Expected**: Proc checks include instance-backed weapon data, and expected weapon/aug procs can fire during pet/NPC attacks.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 2) Lore and Item Socket Mechanics

### [MECH-03] Lore-Equipped-Only Carry Behavior

**Goal**: Verify lore duplicates are allowed in inventory when `Items:LoreEquippedOnly=true`.
**Steps**:

1. Confirm rule with `#rules get Items LoreEquippedOnly` (or DB query).
2. Obtain two copies of the same lore item (example: cast `Summon Orb` twice).
3. Keep both copies in inventory/cursor/bank without equipping both.

**Expected**: No lore pickup rejection; no duplicate-lore destroy behavior while carrying.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [MECH-04] Lore-Equipped-Only Equip Gate

**Goal**: Verify duplicate lore items are blocked only at equip time.
**Steps**:

1. Carry two copies of the same lore item.
2. Equip one copy.
3. Attempt to equip the second copy in another valid slot.

**Expected**: Equip is denied with lore message (`You may only equip one of that lore item.`); inventory item remains intact.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [MECH-05] Global Weapon Aug Slot Expansion Coverage

**Goal**: Verify DB migration coverage for new weapon aug slot policy.
**Setup**:

1. Apply `utils/sql/custom/2026_02_28_aug_slot_expansion_pass.sql`.

**Steps**:

1. Validate 1H weapons have at least one type-4 slot.
2. Validate 2H/bow weapons have at least two type-4 slots where free slots exist.
3. Spot-check in-game by linking/equipping random 1H, 2H, and bow items and inspecting aug slots.

**Expected**: 1H policy and 2H/bow policy are broadly satisfied; exceptions limited to fully occupied-slot legacy items.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [MECH-06] Stat Gear Additional Socket Coverage

**Goal**: Verify stat-bearing gear gained at least one extra type-7 slot where free slots exist.
**Setup**:

1. Apply `utils/sql/custom/2026_02_28_aug_slot_expansion_pass.sql`.

**Steps**:

1. Spot-check armor, jewelry, shield, and weapon pieces with stats.
2. Confirm at least one type-7 slot exists on sampled items unless all 6 aug slots were already populated.

**Expected**: Most stat-bearing equippable items have at least one type-7 slot after migration.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 3) Pet and Familiar THJ Parity

### [MECH-07] Pet Command Persistence Across Zone

**Goal**: Verify permanent pets restore the chosen command-state mix after zoning or relogging.
**Setup**:

1. Apply the custom DB migration that creates `character_pet_command_states`.
2. Summon a normal permanent pet.
3. Use a UF-or-later client so pet button states are visible.

**Steps**:

1. Toggle `taunt`, `hold`, `ghold`, `focus`, and `spellhold` into a known state mix.
2. Zone or camp/relog.
3. Reacquire the pet and inspect button states and actual behavior.
4. Optionally confirm rows exist in `character_pet_command_states` for the pet's origin class.

**Expected**: Pet state after zone-in matches the pre-zone command mix, and the restored buttons align with actual pet behavior.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [MECH-08] Familiar Respawn From Active Buff

**Goal**: Verify familiars are recreated from the active familiar buff instead of relying on normal pet persistence.
**Steps**:

1. Cast a familiar spell and confirm the familiar appears.
2. Zone or relog while the buff is still active.
3. Confirm the familiar respawns automatically after entering the zone.
4. Let the familiar buff fade or remove it manually.

**Expected**: The familiar respawns when the buff is present and disappears when the familiar buff ends.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [MECH-09] Pet Assist On Player Melee

**Goal**: Verify assist-enabled pets acquire or retarget only when the owner performs a melee action.
**Setup**:

1. Summon a permanent pet and ensure it is not on `hold`, `ghold`, or `stop`.
2. Use a target that survives multiple rounds.

**Steps**:

1. Engage target A with normal melee and observe pet acquisition and message feedback.
2. While the pet is engaged, switch to target B and perform an actual melee attack on target B.
3. Observe whether the pet retargets only after the owner's melee action on target B.
4. Repeat with a swarm-pet source if available.

**Expected**: Assist-enabled pets and swarm pets pick up the player's melee target immediately and retarget cleanly when the owner actually melees a new target.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [MECH-10] Pet Assist On Detrimental Spell Cast

**Goal**: Verify assist-enabled pets acquire targets from player-cast detrimental spells.
**Setup**:

1. Summon a permanent pet and leave it able to attack.
2. Use a simple direct-damage or debuff spell on an NPC target.

**Steps**:

1. Start out of melee range if possible.
2. Cast the detrimental spell on the target.
3. Observe whether the pet acquires the target and engages.
4. Repeat with a second target after the assist rate-limit window has expired.

**Expected**: Assist-enabled pets and eligible swarm pets acquire the spell target without needing a direct `/pet attack` command.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [MECH-11] Pet Tanking and Taunt Hold

**Goal**: Verify taunting pets continue to hold aggro while the owner enters melee.
**Setup**:

1. Summon a taunting permanent pet.
2. Engage a durable NPC that can survive several rounds.

**Steps**:

1. Send the pet first and let it build aggro.
2. Enter melee with the owner after the pet has engaged.
3. Observe target swaps and aggro behavior for several rounds.
4. Repeat with taunt off as a comparison case.

**Expected**: With taunt on, the pet remains the effective tank in the common case and does not immediately lose aggro just because the owner joins melee.
**Status**: [ ] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [x] Fail
**Notes**: Known fail — pet taunt aggro hold is an open issue.

### [MECH-12] Familiar Fade and Replacement Cleanup

**Goal**: Verify familiar cleanup follows the active buff state and does not leave orphaned familiars behind.
**Setup**:

1. Use a familiar-capable client character with at least one familiar spell.
2. Start in a zone where the familiar can be summoned and visually inspected.

**Steps**:

1. Cast a familiar spell and confirm exactly one familiar appears.
2. Manually click off or otherwise remove the familiar buff.
3. Confirm the familiar depops promptly when the buff ends.
4. Recast the same familiar spell and confirm only one familiar exists.
5. If a second familiar spell is available, cast it and confirm the old familiar is replaced rather than duplicated.

**Expected**: Familiar buffs clean up their matching familiar on fade/replacement, and the player never ends up with multiple lingering familiars.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [MECH-13] Familiar Ownership Semantics

**Goal**: Verify familiars are treated as owned pets for pet-aware server paths without becoming controllable combat pets.
**Setup**:

1. Summon a familiar from an active familiar buff.
2. Use a client that can issue standard pet commands and inspect nearby spawns.

**Steps**:

1. Confirm the familiar follows the owner and remains associated with that character after zoning or relogging with the buff active.
2. Issue non-`/pet get lost` pet commands and confirm the familiar ignores them.
3. Target the familiar and inspect any owner-reporting behavior available in client or GM tools.
4. Remove the buff and confirm the familiar depops instead of remaining as an orphaned NPC.

**Expected**: The familiar is recognized as belonging to the player for ownership checks, but stays non-combat/non-commandable aside from `get lost`, and disappears cleanly when the buff ends.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [MECH-14] Restored Pet Command Precedence

**Goal**: Verify the new per-command persistence source wins when legacy pet-info state disagrees.
**Setup**:

1. Apply the custom DB migration that creates `character_pet_command_states`.
2. Summon a permanent pet on a UF-or-later client.
3. Create or simulate a mismatch between `character_pet_command_states` and legacy `character_pet_info.taunting`.

**Steps**:

1. Record the mismatched values in both tables before zoning or camp/relog.
2. Zone or camp/relog with the pet restorable.
3. Confirm the pet comes back using the `character_pet_command_states` taunt value, not the legacy value.
4. Confirm the pet-window button state matches the active restored behavior.

**Expected**: The restored pet uses the new per-command state as the authority when legacy and new persistence sources disagree.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [MECH-15] Pet Assist Does Not Retarget On Passive Target Change

**Goal**: Verify assist-enabled pets do not swap targets merely because the owner changes selection without taking an action.
**Setup**:

1. Summon a permanent pet and enable assist behavior.
2. Use two attackable NPCs close enough to tab between while one is already engaged.

**Steps**:

1. Send the pet into combat through melee or a detrimental spell so it is actively engaged on target A.
2. Change the owner's target to target B without attacking, casting, or issuing `/pet attack`.
3. Observe the pet for several seconds.
4. Then perform a real action against target B and confirm the assist retarget happens at that point.

**Expected**: The pet stays on target A during passive target selection changes and only retargets when the owner actually attacks or casts on target B.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Agent Closeout Requirement

When an agent validates or changes mechanics work, it must update this tracker:

1. Set one clear status per case.
2. Add concise notes using `Observed`, `Evidence`, `Commands`, and `Next`.
3. Mark blocked cases as `Blocked`, not `Fail`, unless game behavior itself failed.
4. Prefer updating existing scenario cases over adding new micro-cases.
