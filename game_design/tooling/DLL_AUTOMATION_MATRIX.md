# DLL Automation Matrix

**Last Updated**: 2026-02-28  
**Scope**: All current tracker/work cases (`86` total)

## Goal

Define how each test/work case can be automated using the generic DLL probe path, and where DLL automation is not the right tool.

## Probe Contract (Current)

Server request (`EdgeStatLabel` keys):

- `900`: command (`2` = v2 generic probe)
- `901`: test id
- `902`: nonce
- `903`: protocol version (`2`)
- `904`: requested field mask
- `905-908`: reserved args (`arg0..arg3`)

Client callback:

- `#test clientreplyv2 <test_id> <nonce> <response_mask> <game_state> <spawn_id> <target_id> <hp_cur:hp_max> <mana_cur:mana_max> <end_cur:end_max> <class_mask>`

Field-mask bits:

- `0x0001`: game state
- `0x0002`: spawn id
- `0x0004`: target id
- `0x0008`: hp current/max
- `0x0010`: mana current/max
- `0x0020`: endurance current/max
- `0x0040`: class mask

## Classification Key

- `AUTO-NOW`: covered now by `#test automated` (server-only or server+DLL).
- `HYBRID-NOW`: has automated precheck, but still needs manual in-game confirmation.
- `DLL-NEXT`: can be fully/mostly automated with current v2 fields by adding a server test runner.
- `DLL-EXPAND`: needs new probe fields or new packet hooks first.
- `MANUAL`: better kept manual (UI/placement/content/ops judgment).

## Matrix

### Multiclass (`39`)

- `C-01` HYBRID-NOW - server bit/count invariants are automated; `#test 17` now automates add/remove propagation parity via DLL callback + auto-restore, but live command UX remains manual.
- `C-02` HYBRID-NOW - persistence invariants automated; full relog flow still manual.
- `C-03` HYBRID-NOW - `#test 18` now validates mana parity after class mutation; visible mana-bar UX remains manual.
- `C-04` DLL-EXPAND - needs implied-target context/retarget event visibility.
- `C-05` DLL-EXPAND - needs melee redirect event and aggro decision telemetry.
- `S-01` MANUAL - spellbook/scribing UX and content setup.
- `S-02` MANUAL - cast outcome and effect context still gameplay-driven.
- `S-03` DLL-EXPAND - needs buff-song pulse timing events.
- `S-04` DLL-EXPAND - needs buff slot/duration telemetry from client.
- `S-05` MANUAL - group propagation is scenario-heavy.
- `P-01` DLL-EXPAND - needs combat action timestamps to verify cooldown cadence.
- `P-02` DLL-EXPAND - same as `P-01` but from autofire path.
- `P-03` MANUAL - multipet behavior requires encounter-level validation.
- `P-04` MANUAL - persistence across zone/camp/relog is multi-step stateful.
- `P-11` HYBRID-NOW - DB/rule coverage automated; in-combat equip behavior manual.
- `P-12` DLL-EXPAND - needs pet inventory/equipment event capture.
- `K-01` MANUAL - trainer/skill UX path still better manual.
- `K-02` MANUAL - trainer window access is UI/content.
- `K-03` MANUAL - discipline learn/use flow is gameplay/content.
- `K-04` MANUAL - monk attack selection requires combat-context sampling.
- `I-01` MANUAL - equip allow path is content + UX interaction.
- `I-02` MANUAL - equip deny path is content + UX interaction.
- `I-03` MANUAL - race restriction checks are item/content driven.
- `I-04` MANUAL - right-click effect validation requires item/action context.
- `I-05` MANUAL - quest hand-in flow and NPC scripting.
- `X-01` DLL-EXPAND - needs XP gain delta telemetry to fully automate.
- `X-02` DLL-EXPAND - needs hunger tick and stat-impact telemetry.
- `X-03` DLL-EXPAND - needs regen tick timeline telemetry.
- `A-01` MANUAL - AA window visibility/category UX.
- `A-02` MANUAL - purchase/activate path includes targeting/content checks.
- `A-03` MANUAL - passive effect validation is long-window/statistical.
- `B-04` MANUAL - Bazaar NPC interactions are script/content placement.
- `B-05` MANUAL - NPC script branch validation.
- `C-06` AUTO-NOW - covered by server checks in `#test`.
- `S-06` AUTO-NOW - covered by server checks in `#test`.
- `A-04` AUTO-NOW - server AA gate policy checks covered in `#test`.
- `P-13` AUTO-NOW - server class gate parity check covered in `#test`.
- `B-08` HYBRID-NOW - class ownership API precheck automated; title UX manual.
- `G-01` HYBRID-NOW - query shape automated; roster presentation/manual verification remains.

### Classes (`7`)

- `CL-01` MANUAL - AA visibility + activation UX and combat behavior.
- `CL-02` MANUAL - AA visibility + activation UX and combat behavior.
- `CL-03` HYBRID-NOW - export presence can be scripted; in-client parity still manual.
- `CL-06` DLL-EXPAND - needs ranged rejection/shot event telemetry.
- `CL-07` DLL-EXPAND - needs proc+self-heal event timeline telemetry.
- `CL-08` DLL-EXPAND - needs ranged hit stream capture + hDEX bucketing.
- `CL-09` DLL-EXPAND - needs frenzy event stream and target HP-band capture.

### Mechanics (`2`)

- `MECH-01` DLL-EXPAND - needs proc event counters separated by weapon mode.
- `MECH-02` DLL-EXPAND - needs pet/NPC instance-weapon proc event visibility.

### Quests (`9`)

- `QST-01` MANUAL - map-object click/UI open behavior.
- `QST-02` MANUAL - dialogue+currency+feature unlock chain.
- `QST-03` HYBRID-NOW - trigger spawn/database prechecks scriptable; discovery UX manual.
- `QST-04` HYBRID-NOW - merchant data wiring scriptable; purchase/equip flow manual.
- `QST-05` HYBRID-NOW - runtime undefined-method scans scriptable; full behavior manual.
- `QST-06` MANUAL - instance request/lockout flow with temporal gate.
- `QST-07` MANUAL - no-respawn behavior requires elapsed-time verification.
- `QST-08` MANUAL - farming suppression requires spawn-content walkthrough.
- `QST-09` HYBRID-NOW - Lua helper existence/script load can be automated; branch behavior manual.

### QoL (`1`)

- `QOL-01` DLL-EXPAND - needs probe position/instance context and AA reuse timestamp telemetry.

### Tooling (`10`)

- `TOOL-01` MANUAL - layout usability.
- `TOOL-02` MANUAL - log focus interaction UX.
- `TOOL-03` MANUAL - tracker discovery/navigation UX.
- `TOOL-04` MANUAL - pane resize persistence UX.
- `TOOL-05` AUTO-NOW - `#test 16` DLL round-trip.
- `TOOL-06` HYBRID-NOW - runbook/work-tracker entry presence can be script-validated; human workflow quality still manual.
- `TOOL-07` AUTO-NOW - `#test 17` class-mask mutation round-trip + auto-restore.
- `TOOL-08` AUTO-NOW - `#test 18` mana mutation parity + auto-restore.
- `TOOL-09` AUTO-NOW - `#test 19` snapshot parity (`class/hp/mana/end`).
- `TOOL-10` AUTO-NOW - `#test 20` persisted class-mask parity round-trip.

### Operations Work Tracker (`18`)

- `O-01` HYBRID-NOW - command existence checks scriptable; permission context manual.
- `O-02` HYBRID-NOW - command outputs scriptable; zone stability manual.
- `O-03` AUTO-NOW - file/script existence checks scriptable.
- `B-01` HYBRID-NOW - status output scriptable; live interpretation manual.
- `B-02` MANUAL - in-zone pull behavior and placement context.
- `B-03` MANUAL - spawn move/save/repop visual verification.
- `B-04` MANUAL - iterative placement workflow.
- `B-05` HYBRID-NOW - staged count checks scriptable; zone walk manual.
- `B-06` MANUAL - physical accessibility walk.
- `B-07` DLL-EXPAND - can automate with client position probe extensions.
- `C-01` HYBRID-NOW - command mode output checks scriptable.
- `C-02` HYBRID-NOW - valid/invalid target command behavior scriptable.
- `C-03` HYBRID-NOW - spawn lifecycle command output scriptable; safety still manual.
- `C-04` HYBRID-NOW - depop/repop command checks scriptable.
- `C-05` HYBRID-NOW - command-loop coverage and DB persistence checks scriptable; final visual polish remains manual.
- `D-01` MANUAL - operational audit discipline.
- `D-02` AUTO-NOW - file export timestamp checks scriptable.
- `D-03` HYBRID-NOW - service startup checks scriptable; quick smoke still manual.

## What `#test automated` Means Right Now

- Executes all registered in-server checks (`#test` suite), including DLL callback probes (`#test 16-20`).
- Best used as a baseline gate before manual scenario testing.

## Highest-Value Next Additions (Using Current v2 Fields)

1. Promote `C-02` from HYBRID toward AUTO with a dedicated relog/zone harness hook (currently still manual for full relog flow).
2. Add AA-window snapshot telemetry to reduce manual work for `A-01`, `CL-01`, `CL-02`.
3. Add targeting trace telemetry for `C-04`/`C-05` smart-target automation.

## Full-Automation Exploration Summary (All 86 Cases)

- `AUTO-NOW`: 11  
- `HYBRID-NOW`: 22  
- `DLL-NEXT`: 0  
- `DLL-EXPAND`: 18  
- `MANUAL`: 35  

## Implementation Lanes

1. Lane A: Expand `#test` server-only assertions where no DLL data is required.
2. Lane B: Add `DLL-NEXT` tests using current v2 probe fields (no protocol changes).
3. Lane C: Add protocol v3 fields/events to unlock `DLL-EXPAND` cases.
4. Lane D: Keep explicit manual suites for placement/UX/content validation.

## Probe Expansion Backlog

To unlock `DLL-EXPAND` cases, add:

1. Position and heading fields.
2. Active buff slot + duration snapshot.
3. Combat action events (swing/autoskill/proc/recast timestamps).
4. XP and regen tick deltas.
5. Optional server-initiated sampling window (`arg0..arg3`) for timed tests.

## Telemetry-To-Test Unlock Map

1. `Position/Heading + Zone/Instance Context`
- Unlocks: `QOL-01`, `B-07`, plus waypoint/return-point validation prechecks.

2. `Buff Snapshot (slot, spell_id, ticks_remaining, is_song)`
- Unlocks: `S-03`, `S-04` automation candidates.

3. `Combat Event Stream (attack_type, skill, timestamp_ms, reuse_gate, proc_id)`
- Unlocks: `P-01`, `P-02`, `CL-06`, `CL-07`, `CL-08`, `CL-09`, `MECH-01`, `MECH-02`, `K-04`.

4. `Pet Equipment Snapshot (pet_id, slot, item_id)`
- Unlocks: `P-12` and pet bag/equip synchronization verification depth.

5. `Progression Deltas (xp_before/after, regen tick deltas, hunger state)`
- Unlocks: `X-01`, `X-02`, `X-03`.

6. `Targeting Decision Trace (requested target, implied target, final target, reason)`
- Unlocks: `C-04`, `C-05` smart-targeting and melee implied-target automation.
