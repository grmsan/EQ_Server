# Multiclass Test Tracker

**Status**: Active Testing
**Tracker Area**: Multiclass
**Tracker State**: Active
**Last Updated**: 2026-04-21
**Technical Plan**: [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)

## Purpose

This tracker is the single source of truth for multiclass validation across long dev gaps.

Use it in two modes:

1. `Smoke Run` (15-25 min): go/no-go checks after fresh code changes.
2. `Full Regression` (60-120 min): broad validation before merge/release.

## Return-After-Break Quickstart (10 Minutes)

Use this when it has been days/weeks since last test session.

1. Fill out `Session Header` first (date, build/branch, key rules).
2. Verify you are on the expected zone build and database.
3. Run `#test smoke` and `#test combat` to catch obvious wiring regressions.
4. Pick one known-good sanity test (`C-01` or `P-01`) and confirm expected output still matches prior behavior.
5. Start full testing only after the sanity pass is stable.

## Current Execution Order (2026-04-21)

Use this sequence if the goal is to knock out the remaining multiclass backlog in one focused session.

1. **Fresh build and baseline sanity**
   - Build `zone`; build `world` and the DLL before presentation checks.
   - Run `#test smoke`, `#test combat`, and the DLL probe suite `#test 16-20`.
2. **Recently closed gameplay gaps**
   - Validate item, augment, XP-cap, and inventory-label changes first.
   - Run `I-01` to `I-07`, `X-04`, and `D-04`.
3. **AA validation and policy**
   - Verify live AA window, purchase, activation, passive ownership, and special AA gates.
   - Run `A-01` to `A-05`, then record current behavior for `E-02`.
4. **Caster, spell, skill, and discipline checks**
   - Verify secondary-caster UI, spell soft-lock cleanup, skills window exposure, and discipline timers.
   - Run `C-03`, `E-01`, `K-01` to `K-05`, and `S-01` to `S-07`.
5. **World/presentation parity**
   - Verify character select, `/who`, `#mystats`, titles, and guild/class projection.
   - Run `D-01`, `D-02`, `D-03`, `B-08`, and `G-01`.
6. **Finish with tracker sync**
   - Re-run any failed category after fixes, then update statuses only where there is objective in-game or log evidence.

## End-To-End Workflow

1. Select scope: `Smoke Run`, `Full Regression`, or one category (`P-*`, `B-*`, etc.).
2. Read each test's `Goal`, then execute listed `Steps` exactly.
3. Validate against `Expected` and collect objective evidence:
   - in-game chat lines,
   - server log lines,
   - rule values/command output.
4. Mark `Status` as `Pass` or `Fail`.
5. Add concise notes with reproduction commands and what actually happened.
6. If failing, include one immediate next action (code path or SQL/data target).

## Pass/Fail Rules

- `Pass`: Expected behavior occurs consistently without manual interpretation.
- `Fail`: Any mismatch, intermittency, missing output, or dependency break.
- `Blocker`: If environment is invalid (missing SQL, stale assets, wrong rules), mark `Fail` and note `Blocked by environment`.

## Evidence Format (Use in Notes)

Keep notes short but reproducible:

`Observed:` actual behavior in one line.
`Evidence:` exact chat/log snippet or command output.
`Commands:` minimal command sequence to reproduce.
`Next:` likely code/data area if failed.

## Confirmation Guidance By Category

- `C-*` core/persistence: confirm with `#addclass list` and `#multiclassdiag` before/after relog/zone.
- `S-*` spells/casting: confirm cast result, mana/timer behavior, and target correctness.
- `P-*` combat/procs/pets: gather multiple rounds (not one hit) and verify cooldown/proc cadence.
- `I-*` item/equip: verify both allow and deny paths to avoid false positives.
- `A-*` AA: verify visibility, purchase, activate, and cooldown behavior.
- `B-*` bazaar/scripts: verify NPC presence, dialogue branches, and quest/script side effects.

## Related Trackers

- Class/ability combat and class-specific AA tests: [../classes/TEST_TRACKER.md](../classes/TEST_TRACKER.md)
- Core combat/proc mechanics tests: [../mechanics/TEST_TRACKER.md](../mechanics/TEST_TRACKER.md)
- Quest and instance script flow tests: [../quests/TEST_TRACKER.md](../quests/TEST_TRACKER.md)
- QoL travel/system tests: [../qol/TEST_TRACKER.md](../qol/TEST_TRACKER.md)
- Server manager tooling tests: [../tooling/TEST_TRACKER.md](../tooling/TEST_TRACKER.md)

## How To Use

1. Run tests in ID order unless doing a scoped run.
2. Mark each test `Pass` or `Fail` immediately after execution.
3. Record notes with enough detail to retest the same case a month later.

## Global Commands

- `#addclass <id>`: Add class (`1=WAR, 2=CLR, 3=PAL, 4=RNG, 5=SHD, 6=DRU, 7=MNK, 8=BRD, 9=ROG, 10=SHM, 11=NEC, 12=WIZ, 13=MAG, 14=ENC, 15=BST, 16=BER`)
- `#removeclass <id>`
- `#addclass list`
- `#multiclassdiag`: Show multiclass diagnostics for current character
- `#level <n>`
- `#setskill <id> <value>`
- `#set aa_points <n>`
- `#autoskill list`
- `#showstats`
- `#test list`: Show available in-game sanity tests
- `#test packs`: Show named in-game test packs
- `#test <id>`: Run one test (example: `#test 3`)
- `#test <start-end>`: Run a range (example: `#test 1-10`)
- `#test 16`: Validate DLL client probe round-trip (`EdgeStatLabel` request -> `clientreplyv2` callback)
- `#test 17`: Validate class-mask mutation propagation + auto-restore
- `#test 18`: Validate mana mutation parity via DLL callback + auto-restore
- `#test 19`: Validate client snapshot parity (`class/hp/mana/end`) via DLL callback
- `#test 20`: Validate runtime/profile/bucket class-mask persistence + DLL callback parity
- `#test smoke`: Foundational sanity pack
- `#test combat`: Combat/proc/pet/AA precheck pack
- `#test automated`: Run all automated checks (server + DLL probe callbacks)
- `#test regression`: Run full registered automated suite
- `#test all`: Run all registered tests

## Run Packs

### Smoke Run

Run these first: `C-01, C-02, C-03, S-02, P-01, P-02, P-03, P-04, C-06, S-06`

### Full Regression

Run all tests in this document.

## `#test` Automation Coverage (Current)

Legend:
- `Auto`: `#test` can directly validate the server-side objective.
- `Partial`: `#test` validates invariants/preconditions, but in-game behavior still needs manual play.
- `Manual`: Not meaningfully covered by current `#test` harness.

### Auto (2/51)

| Tracker ID | Coverage | `#test` IDs | Notes |
|---|---|---|---|
| `C-06` | Auto | `5, 8` | Validates `GestaltClasses`/`PlayerProfile.classes` hydration consistency on live character state. |
| `S-06` | Auto | `9, 10` | Validates multiclass spell target transforms are active in loaded spell data. |

### Partial (9/51)

| Tracker ID | Coverage | `#test` IDs | Notes |
|---|---|---|---|
| `C-01` | Partial | `3, 4` | Confirms class bit integrity and max-class enforcement; does not execute add/remove flow by itself. |
| `C-02` | Partial | `5, 8` | Confirms persistence state is internally consistent after login/zone; relog flow is still manual. |
| `S-05` | Partial | `10` | Confirms spell target transform; actual group propagation cast remains manual. |
| `A-04` | Partial | `12` | Covers the Mnemonic Retention multiclass override precheck; live AA window behavior remains manual. |
| `A-05` | Partial | `15` | Covers the Fury of Magic pure-caster gate precheck; live AA progression and window behavior remain manual. |
| `P-11` | Partial | `14` | Confirms pet bag rule + DB + merchant wiring; live summon/sync behavior remains manual. |
| `P-13` | Partial | `6` | Confirms `HasClass` parity with bitmask; combat gate behavior remains manual. |
| `B-08` | Partial | `6` | Confirms class ownership API consistency; title unlock UI/eligibility remains manual. |
| `G-01` | Partial | `11` | Validates guild query projection shape plus live class-mutation/level-update refreshes and forced guild-members-list reloads; roster presentation validation remains manual. |

### Manual (40/51)

`C-03, C-04, C-05, S-01, S-02, S-03, S-04, S-07, P-01, P-02, P-03, P-04, P-12, K-01, K-02, K-03, K-04, K-05, I-01, I-02, I-03, I-04, I-05, I-06, I-07, X-01, X-02, X-03, X-04, A-01, A-02, A-03, B-04, B-05, D-01, D-02, D-03, D-04, E-01, E-02`

### Fast Automation Commands

- Baseline multiclass sanity: `#test 1-10` or `#test smoke`
- Combat/proc/pet/AA prechecks: `#test combat` (IDs `6,12,14,15`; mechanics tracker covers proc parity follow-up)
- Guild/AA sanity: `#test 11-15`
- DLL probe suite: `#test 16-20`
- Full current automated suite: `#test automated` (alias: `#test regression` / `#test all`)

## Automated Validation Snapshot (2026-02-26)

- Build/runtime sanity:
1. `cmake --build build --target zone world --parallel 8` -> passed.
2. Zone CLI tests passed: `tests:databuckets`, `tests:zone-state`, `tests:npc-handins`, `tests:npc-handins-multiquest`.
3. Full stack boot (`start_server.py`) succeeded; world/zone/login/ucs/queryserv/eqlaunch all connected and static zones launched.
- DB checks:
1. `Custom:MulticlassingEnabled=true`, `Custom:UseDynamicAATimers=true`.
2. `data_buckets.GestaltClasses` rows present and readable.
3. New guild projection join/query shape (`character_data` + `guild_members` + `data_buckets.GestaltClasses`) executes successfully.
- Notes:
1. This snapshot validates server health and multiclass plumbing, not player-input combat execution.
2. `#test` now covers server-side assertions for `C-06` and `S-06`, and partial prechecks for `A-04`, `A-05`, and `P-11`; split manual validation is still required for discipline timer behavior (`S-07`) and live AA window behavior.

---

## Session Header

**Date**: __________
**Tester**: __________
**Build/Branch**: __________
**Server Rules Snapshot**: `AbsolutePetLimit=__`, `EnablePetBags=__`, `MultipleTwoHandedProcs=__`, `DoubleAttackSkillRanged=__`, `ScaleBowByHDex=__`, `ScaleAutoAttackByHStr=__`, `DevastatingFrenzyDamageMultiplier=__`
`StaticInstanceVersion=__`, `StaticInstanceTemplateVersion=__`, `FarmingInstanceVersion=__`, `FarmingInstanceTemplateVersion=__`

---

## 0) Tooling UX (Moved)

Tooling/Server Manager UI validation was moved to:
[../tooling/TEST_TRACKER.md](../tooling/TEST_TRACKER.md)

---

## 1) Core Multiclass & Persistence

### [C-01] Add/Remove Class Bits

**Goal**: Verify class bitmask mutates correctly.
**Steps**:

1. `#addclass 1`
2. `#addclass list`
3. `#removeclass 1`
4. `#addclass list`
**Expected**: Warrior appears after add and disappears after remove.
**Status**: [x] Pass  [ ] Fail
**Notes**: ______________________________

### [C-02] Session Persistence

**Goal**: Verify multiclass data survives camp/relog.
**Steps**:

1. `#addclass 1` and `#addclass 2`
2. `#camp`, then log back in
3. `#addclass list`
**Expected**: Warrior + Cleric still present.
**Status**: [x] Pass  [ ] Fail
**Notes**: ______________________________

### [C-03] Client Sync (Mana Bar)

**Goal**: Verify the client-facing mana bar becomes visible and stays visible for a newly owned caster class.
**Steps**:

1. Base Warrior.
2. `#addclass 12`
3. Check Player Window immediately and after relog.
**Expected**: The mana bar becomes visible immediately after adding the caster class and is still visible after relog. This case validates user-facing mana UI only; callback/value parity is covered separately by tooling.
**Status**: [x] Pass  [ ] Fail
**Notes**: ______________________________

### [C-04] Smart Spell Targeting

**Goal**: Verify implied targeting behavior.
**Steps**:

1. Ensure implied targeting rule is enabled.
2. Target hostile NPC targeting you.
3. Cast beneficial single-target spell.
4. Cast detrimental spell on same setup.
**Expected**: Beneficial reroutes to valid friendly implied target; detrimental remains hostile.
**Status**: [x] Pass  [ ] Fail
**Notes**: ______________________________

### [C-05] Smart Melee Implied Targeting

**Goal**: Verify melee implied targeting redirects correctly for player/player-pet attacks.
**Steps**:

1. Target a group member (or their pet) who is actively targeting a hostile NPC.
2. Enter melee range and attack.
3. Repeat while the hostile has no aggro on you/group.
**Expected**: Melee swings redirect to the hostile only when aggro checks pass; otherwise attacks remain on original target.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 2) Spells & Casting

### [S-01] Hybrid Scribing

**Goal**: Verify secondary-class spells can be learned.
**Steps**:

1. Base Warrior, `#addclass 12`, `#level 50`
2. Scribe a Wizard spell (`#scribe ...` or scroll)
**Expected**: Spell is learned and visible in spellbook.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [S-02] Hybrid Casting

**Goal**: Verify secondary-class casting works.
**Steps**:

1. Memorize secondary-class spell.
2. Cast on valid target.
**Expected**: Cast succeeds, mana spent, effect applies.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [S-03] Bard Song Pulse

**Goal**: Verify bard song refresh behavior.
**Steps**:

1. `#addclass 8`
2. Start beneficial song.
3. Observe duration at expiry.
**Expected**: Song refreshes instead of fading when appropriate.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [S-04] Infinite Buff Behavior (Non-Bard)

**Goal**: Verify expected persistent beneficial buff behavior.
**Steps**:

1. Cast normal beneficial buff on self.
2. Observe duration over ticks.
**Expected**: Behavior matches current intended ruleset (persistent/reset as configured).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [S-05] Group Spell Targeting

**Goal**: Verify group spell propagation for multiclass caster.
**Steps**:

1. Group with player/bot.
2. Cast group buff from secondary class.
**Expected**: Group spell lands correctly on valid members.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 3) Combat, Pets, Autoskills, Procs

### [P-01] Autoskills from Melee Auto-Attack

**Goal**: Verify autoskill trigger path in melee.
**Steps**:

1. Enable `Kick` autoskill (`#autoskill ...`).
2. Enable auto-attack in melee range.
3. Observe multiple rounds.
**Expected**: Kick fires automatically at reuse intervals.
**Status**: [X] Pass  [ ] Fail
**Notes**: Good as of 2/24

### [P-02] Autoskills While Autofire

**Goal**: Verify autoskill loop while autofire is active.
**Steps**:

1. Equip bow/arrows and enable autofire.
2. Keep autoskill enabled.
3. Observe combat output.
**Expected**: Autoskill loop continues during autofire; no recovery spam from autoskill context.
**Status**: [X] Pass  [ ] Fail
**Notes**: Good as of 2/24

### [P-03] Multi-Pet Runtime Behavior

**Goal**: Verify active/focused pet behavior with multiple pets.
**Steps**:

1. Set `AbsolutePetLimit >= 2`.
2. Summon two permanent pets.
3. Use pet commands and switch focus.
**Expected**: Commands apply to focused/active pet correctly; pet window remains coherent.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [P-04] Multi-Pet Persistence (Zone/Camp/Relog)

**Goal**: Verify full pet persistence for multiple pets.
**Steps**:

1. Summon 2+ permanent pets.
2. Zone and verify restore.
3. Camp/relog and verify restore.
**Expected**: All permanent pets restore (HP/mana/buffs/items) within configured limits.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

Core proc/combat-system parity validations were moved to:
[../mechanics/TEST_TRACKER.md](../mechanics/TEST_TRACKER.md)

Class-specific ranged/frenzy and custom combat ability validations were moved to:
[../classes/TEST_TRACKER.md](../classes/TEST_TRACKER.md)

### [P-11] Pet Bag Equip Sync (Per Pet Class)

**Goal**: Verify THJ-style class pet bag equipment is applied to pets.
**Setup**:

1. Confirm `Custom:EnablePetBags = true`.
2. Obtain class pet bag from merchant (`899980, 899981, 899983, 899984, 899985, 899986, 899987, 899988`).
3. Put equippable pet items into the bag.
**Steps**:

1. Summon a matching class pet (or zone with one saved).
2. Observe pet equipment/appearance and combat behavior.
3. Move/replace an item inside the pet bag.
4. Destroy or remove the pet bag and observe pet equipment update.
**Expected**: Matching class pet mirrors bag contents; bag item edits resync live; destroying/removing bag flushes bag-applied pet equipment.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [P-12] Charmed Pet Inventory Restore After Pet Bag Sync

**Goal**: Verify charm + pet bag flow does not permanently lose original charmed inventory.
**Setup**:

1. Confirm `Custom:EnablePetBags = true`.
2. Have an active class pet bag for the relevant class.
**Steps**:

1. Charm an NPC that has visible equipment/loot.
2. Confirm pet bag sync applies while charmed.
3. Break charm (wait fade or force break).
4. Inspect NPC inventory/appearance post-break.
**Expected**: Charmed NPC inventory is restored after charm breaks (no permanent bag-sync overwrite).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 4) Skills, Trainers, Disciplines

### [K-01] Skill Cap Uses Best Class

**Goal**: Verify skill cap selection across owned classes.
**Steps**:

1. Base Wizard, add Warrior, level up.
2. Raise a melee skill (ex: 1H Blunt).
**Expected**: Skill cap follows best owned class, not base class only.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [K-02] Trainer Window Access

**Goal**: Verify multiclass-aware trainer access.
**Steps**:

1. Base Warrior + add Wizard.
2. Use Wizard trainer.
3. End training session.
**Expected**: Training opens and closes cleanly without base-class denial.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [K-03] Discipline Learn + Use

**Goal**: Verify tome learning and activation.
**Steps**:

1. Add class with target discipline support.
2. Consume discipline tome.
3. Activate discipline from combat ability window.
**Expected**: Discipline learned and activates correctly.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [K-05] Skills Window Exposure For Added Class

**Goal**: Verify the client skills window exposes skills granted by owned secondary classes.
**Steps**:

1. Base a character in a class with limited native skill coverage.
2. Add a class that grants additional visible skills.
3. Open the Skills window and inspect the newly granted skills.
4. Confirm one of those skills can also be used or trained normally.
**Expected**: The Skills window shows added-class skills and the client does not hide server-granted skill access.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [K-04] Monk Special Attack Selection

**Goal**: Verify monk special path when monk is secondary.
**Steps**:

1. Base Warrior + add Monk.
2. Train flying kick and kick.
3. Auto-attack target.
**Expected**: Monk special behavior appears, not fallback-only kick behavior.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 5) Items & Equip Rules

### [I-01] Positive Equip Gate (Owned Secondary Class)

**Goal**: Verify class restriction allows owned secondary class.
**Steps**:

1. Base Warrior + add Wizard.
2. Equip Wizard-only item.
**Expected**: Item can be equipped if other restrictions pass.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [I-02] Negative Equip Gate (Non-Owned Class)

**Goal**: Verify class restriction blocks non-owned class.
**Steps**:

1. Base Ranger + add Warrior/Mage.
2. Attempt Enchanter-only item.
**Expected**: Equip denied; inventory state remains consistent.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [I-03] Race Restriction Still Enforced

**Goal**: Verify multiclass does not bypass race checks.
**Steps**:

1. Attempt race-restricted item on invalid race.
**Expected**: Denied due to race.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [I-04] Class-Locked Right-Click Item Effect

**Goal**: Verify right-click effect honors multiclass ownership.
**Steps**:

1. Add matching secondary class.
2. Use class-restricted clickable.
**Expected**: Effect activates if class requirement is satisfied by owned class set.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [I-05] Class-Dependent Quest Turn-In

**Goal**: Verify secondary-class epic/quest hand-ins.
**Steps**:

1. Attempt turn-in for secondary-class quest NPC.
**Expected**: NPC accepts appropriate hand-ins based on multiclass ownership.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [I-06] Merchant Class Filter Uses Owned Classes

**Goal**: Verify merchant filtering honors multiclass ownership instead of base class only.
**Steps**:

1. Use a merchant that applies class filtering through `merchantlist.classes_required` or item class usability.
2. Open the merchant window on a base-class-only character and record visible items.
3. Add a qualifying secondary class and reopen the merchant window.
4. Toggle any usable-item filtering path if applicable.
**Expected**: Items restricted to the owned secondary class appear once that class is owned, and non-owned class items remain filtered out.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [I-07] Class-Restricted Augment Insert

**Goal**: Verify augment insertion honors owned multiclass bits instead of the legacy single class.
**Steps**:

1. Use a valid augmentable item plus a class-restricted augment for a class the character does not own.
2. Attempt to insert the augment.
3. Add the qualifying class.
4. Attempt the same insert again.
**Expected**: Insert is denied while the qualifying class is not owned, then succeeds once the character owns that class.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 6) XP, Regen, AA

### [X-01] XP Modifier Behavior

**Goal**: Verify XP logic responds to owned classes as intended.
**Steps**:

1. Kill baseline mob.
2. Add/remove target bonus class.
3. Repeat kill comparison.
**Expected**: XP pattern matches configured class modifier logic.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [X-02] Hunger/Food Tick Behavior

**Goal**: Verify class-influenced consumption behavior.
**Steps**:

1. Add class with known hunger-rate differences.
2. Observe over time.
**Expected**: Consumption matches intended class logic.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [X-03] Regen Behavior

**Goal**: Verify class/race regen calculations.
**Steps**:

1. Set test race/class mix.
2. Sit and observe HP ticks.
**Expected**: Regen follows intended formula outcomes.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [A-01] AA Visibility Across Owned Classes

**Goal**: Verify AA window reflects multiclass ownership.
**Steps**:

1. Level and add two AA-rich classes.
2. Open AA window.
**Expected**: Relevant AA lines/tabs appear for owned classes.
**Status**: [x] Pass  [ ] Fail
**Notes**: ______________________________

### [A-02] AA Purchase + Activate

**Goal**: Verify active AA from secondary class works.
**Steps**:

1. Grant AA points.
2. Buy secondary-class active AA.
3. Activate AA.
**Expected**: Purchase and activation succeed.
**Status**: [x] Pass  [ ] Fail
**Notes**: ______________________________

### [A-03] Passive AA Effect

**Goal**: Verify passive AA effects apply only while the owning class is present.
**Steps**:

1. Buy passive AA.
2. Validate effect in behavior/stats.
3. Remove the class that grants that AA.
4. Re-check the same behavior/stats after a normal bonus refresh path (relog, zone, or class refresh).
**Expected**: Passive bonus is applied while the class is owned and stops applying once that class is removed.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 7) Custom Instances (Moved)

Instance and quest-helper validation moved to:
[../quests/TEST_TRACKER.md](../quests/TEST_TRACKER.md)

---

## 8) THJ Bazaar + Waypoints (Multiclass-Specific Subset)

Bazaar waypoint/map/quest API smoke tests moved to:
[../quests/TEST_TRACKER.md](../quests/TEST_TRACKER.md)

### [B-04] Class Add NPCs (Guildmasters Class 20..35)

**Goal**: Verify Bazaar class-add NPCs drive global multiclass add flow.
**Steps**:

1. Hail one of the Bazaar class guildmasters (class 20..35 NPC).
2. Follow `class_select` -> `class_confirm`.
3. Run `#multiclassdiag` / `#autoskill list` / class-sensitive command to confirm ownership.
**Expected**: Class bitmask gains selected class and class systems react immediately.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [B-05] Vision_of_Ayonae Class Removal

**Goal**: Verify class removal/reroll NPC path is available in Bazaar.
**Steps**:

1. Hail `Vision_of_Ayonae`.
2. Choose class removal path for one owned class.
3. Validate class is removed and cooldown/cost behavior is enforced.
**Expected**: Removal succeeds when requirements are met; lockout/cost rules apply.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [C-06] Zone-Load GestaltClasses Hydration

**Goal**: Verify login and first-zone hydration rebuild runtime class state from persisted `GestaltClasses` without requiring a new class mutation.
**Steps**:

1. Log out with a multi-class character.
2. Log back in and run `#multiclassdiag` immediately after entering world.
3. Zone once and run `#multiclassdiag` again.
4. Do not add or remove any class bits during this test.
**Expected**: Runtime and profile class state are already correct immediately at login, and remain unchanged after the first zone hop. This case validates hydration, not long-term persistence after editing class ownership.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [S-06] Multiclass Spell Target Transform

**Goal**: Verify multiclass spell-load target transforms in `shareddb` are active.
**Steps**:

1. Cast a spell with `ST_GroupClientAndPet` target behavior from a multiclass character.
2. Observe the resulting targeting behavior on the valid target set.
3. Repeat once after a relog or fresh zone load.
**Expected**: Group+pet target behavior transforms to the intended multiclass target behavior consistently on live cast. This case validates spell target transform only.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [S-07] Discipline Timer Family Deconfliction

**Goal**: Verify disciplines from different owned classes do not incorrectly share lockout timers.
**Steps**:

1. Use two disciplines from different owned classes that would conflict under legacy single-class timer handling.
2. Activate the first discipline.
3. Attempt to activate the second discipline on the same character.
4. Re-use each discipline after its own recast window.
**Expected**: Each discipline respects its intended multiclass timer handling, and one discipline does not incorrectly lock out the other.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [A-04] Mnemonic Retention Multiclass Override

**Goal**: Verify Mnemonic Retention remains available/usable under the multiclass-specific AA override.
**Steps**:

1. Use a multiclass character that should qualify for Mnemonic Retention under the intended rules.
2. Open the AA window and locate Mnemonic Retention.
3. Purchase or validate usability if not already owned.
**Expected**: Mnemonic Retention follows the intended multiclass override policy and is not hidden or blocked by legacy class-only gating.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [A-05] Fury of Magic Pure-Caster Gate

**Goal**: Verify Fury of Magic rank 6+ follows the intended pure-caster multiclass gate.
**Steps**:

1. On a non-pure-caster multiclass, attempt Fury of Magic rank 6+ progression.
2. Record whether purchase/visibility is blocked.
3. Repeat the same progression on a pure-caster multiclass.
**Expected**: Fury rank 6+ is blocked for non-pure casters and allowed for pure casters.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [P-13] HasClass Combat Gate Parity (Ranger/Berserker)

**Goal**: Verify combat gates use owned classes, not only base class.
**Steps**:

1. Use a non-ranger-base character with Ranger owned class; test archery double-damage conditions.
2. Use a non-berserker-base character with Berserker owned class; test frenzy min-cap behavior.
3. Confirm behavior disappears when those classes are removed.
**Expected**: Ranger/Berserker combat gates trigger based on class ownership (`HasClass`) not base class.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [B-08] Multiclass Title Eligibility

**Goal**: Verify titles gated by class can unlock via owned secondary classes.
**Steps**:

1. Acquire a title whose class requirement is a non-base owned class.
2. Open title list and confirm eligibility.
3. Remove that class and re-check eligibility.
**Expected**: Eligibility follows `HasClass` ownership, not only base class.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [G-01] Guild Member Multiclass Projection

**Goal**: Verify guild roster presentation uses the multiclass projection after a roster refresh event.
**Steps**:

1. Put a known multiclass character in a guild.
2. Trigger a guild-roster refresh path (fresh login, level update, or class mutation).
3. Open guild member list (or inspect guild roster API output).
4. Compare class/level presentation to expected multiclass representation.
**Expected**: Guild member info reflects the multiclass projection after refresh and does not fall back to stale single-class presentation.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 9) Display, Entitlement, and Scaling Follow-Up

### [X-04] Level Cap / Exp Cap Logic

**Goal**: Verify multiclass ownership does not break intended level-cap or XP-cap rules.
**Steps**:

1. Use a character near any relevant level or XP cap rule edge.
2. Add or remove a class that could affect cap logic.
3. Gain XP and observe level/XP behavior.
**Expected**: XP gain and level progression follow the intended ruleset without granting unintended cap bypasses.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [D-01] Character Select Multiclass Display

**Goal**: Verify character select presentation reflects multiclass state as intended.
**Steps**:

1. Log out with a known multiclass character.
2. Return to character select.
3. Observe class label/presentation for that character.
**Expected**: Character select reflects multiclass presentation according to the current client/DLL design.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [D-02] `/who` Multiclass Display

**Goal**: Verify `/who` output reflects multiclass presentation as intended.
**Steps**:

1. Log in with a known multiclass character.
2. Run `/who` and `/who all <name>` from another client or comparable observer path.
3. Compare returned class label/presentation to expected multiclass behavior.
**Expected**: `/who` output matches the current intended multiclass presentation policy without corrupt or unknown class output.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [D-03] `#mystats` Multiclass Info Output

**Goal**: Verify `#mystats` or equivalent diagnostics expose multiclass ownership information.
**Steps**:

1. Use a known multiclass character.
2. Run `#mystats`.
3. Confirm owned-class or class-bitmask information appears and is accurate.
**Expected**: Diagnostic output includes accurate multiclass ownership information.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [D-04] Inventory Window Multiclass Display

**Goal**: Verify the inventory window shows multiclass labels instead of the single compatibility/base class.
**Steps**:

1. Log in with a known multiclass character.
2. Open the inventory window.
3. Observe the class text shown in the inventory UI, including the abbreviated class label if the active XML exposes it.
4. Add or remove a class, then reopen or refresh the inventory window.
**Expected**: Inventory class labels reflect the current multiclass presentation policy and update from the owned multiclass mask rather than leaking the legacy compatibility/base class.
**Status**: [ ] Pass  [ ] Fail
**Notes**: DLL path now rewrites `IW_Class` / `IW_ClassAbbr`; validate against both default and THJ inventory XML when available. ______________________________

### [E-01] Class Removal Unmemorizes Invalid Spells

**Goal**: Verify class removal soft-locks or clears spell access according to multiclass entitlement rules.
**Steps**:

1. Add a caster class and memorize spells only available through that owned class.
2. Remove that class.
3. Re-open spellbook/spell gems, attempt to cast affected spells, and try to re-memorize one from the spellbook.
**Expected**: Spells that are no longer valid after class removal are blocked or cleared according to the intended multiclass entitlement behavior, and re-memorizing an invalid spell fails immediately with an error instead of leaving the spellbar memorize UI hanging.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [E-02] Class Removal AA Entitlement Policy

**Goal**: Verify removed-class AA ownership follows the chosen entitlement policy.
**Steps**:

1. Purchase or grant AAs tied to a removable owned class.
2. Remove that class.
3. Re-open AA window, re-check AA point totals, and attempt to activate any affected AA.
**Expected**: Pending design decision — either removed-class AAs remain purchased but unusable (soft-lock) or the system performs a refund/reset path. Record which policy the code currently follows.
**Status**: [ ] Pass  [ ] Fail
**Notes**: Pending design decision on final entitlement policy. ______________________________

---

## Suggested Workflow

Recommended cadence:

1. Daily coding loop: run `#test smoke` plus changed-category tests only.
2. Before DB/script changes go live: run multiclass ownership/state checks (`C-01`, `C-02`, `C-06`) and then run domain-specific trackers (`quests`, `qol`, `classes`) as needed.
3. Pre-merge gate: run `Smoke Run` + full `P-*` + full `A-*`.
4. Release candidate: run full tracker once with clean session notes.

Handoff format for failed items:

1. `Test ID`: example `C-01`.
2. `Build`: git hash/branch.
3. `Repro`: exact command sequence.
4. `Observed vs Expected`: one-line delta.
5. `Evidence`: chat/log snippet.
