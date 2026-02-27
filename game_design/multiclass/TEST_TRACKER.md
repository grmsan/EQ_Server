# Multiclass Test Tracker

**Status**: Active Testing
**Last Updated**: 2026-02-27
**Technical Plan**: [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)

## Purpose

This tracker is structured for fast in-game verification.

Use it in two modes:

1. `Smoke Run` (15-25 min): critical go/no-go checks.
2. `Full Regression` (60-120 min): broader multiclass coverage.

## How To Use

1. Run tests in ID order.
2. Mark each test `Pass` or `Fail`.
3. Add one-line notes with evidence (chat line, behavior, log file).

## Global Commands

- `#addclass <id>`: Add class (`1=WAR, 2=CLR, 3=PAL, 4=RNG, 5=SHD, 6=DRU, 7=MNK, 8=BRD, 9=ROG, 10=SHM, 11=NEC, 12=WIZ, 13=MAG, 14=ENC, 15=BST, 16=BER`)
- `#removeclass <id>`
- `#addclass list`
- `#level <n>`
- `#setskill <id> <value>`
- `#autoskill list`
- `#test list`: Show available in-game sanity tests
- `#test packs`: Show named in-game test packs
- `#test <id>`: Run one test (example: `#test 3`)
- `#test <start-end>`: Run a range (example: `#test 1-10`)
- `#test smoke`: Foundational sanity pack
- `#test combat`: Combat/proc/pet/AA precheck pack
- `#test regression`: Run full registered automated suite
- `#test all`: Run all registered tests

## Run Packs

### Smoke Run

Run these first: `C-01, C-02, C-03, I-01, I-02, S-02, P-01, P-02, P-03, P-04, P-11, Z-01, Z-02`

### Full Regression

Run all tests in this document.

## `#test` Automation Coverage (Current)

Legend:
- `Auto`: `#test` can directly validate the server-side objective.
- `Partial`: `#test` validates invariants/preconditions, but in-game behavior still needs manual play.
- `Manual`: Not meaningfully covered by current `#test` harness.

### Auto (2/57)

| Tracker ID | Coverage | `#test` IDs | Notes |
|---|---|---|---|
| `C-06` | Auto | `5, 8` | Validates `GestaltClasses`/`PlayerProfile.classes` hydration consistency on live character state. |
| `S-06` | Auto | `9, 10` | Validates multiclass spell timer and target transforms are active in loaded spell data. |

### Partial (9/57)

| Tracker ID | Coverage | `#test` IDs | Notes |
|---|---|---|---|
| `C-01` | Partial | `3, 4` | Confirms class bit integrity and max-class enforcement; does not execute add/remove flow by itself. |
| `C-02` | Partial | `5, 8` | Confirms persistence state is internally consistent after login/zone; relog flow is still manual. |
| `S-05` | Partial | `10` | Confirms spell target transform; actual group propagation cast remains manual. |
| `A-04` | Partial | `12, 15` | Covers Mnemonic Retention override and Fury rank 6+ pure-caster gate policy check. |
| `P-05` | Partial | `13` | Confirms THJ proc preconditions (rules + 2H/bow proc item coverage); live proc firing remains manual. |
| `P-11` | Partial | `14` | Confirms pet bag rule + DB + merchant wiring; live summon/sync behavior remains manual. |
| `P-13` | Partial | `6` | Confirms `HasClass` parity with bitmask; combat gate behavior remains manual. |
| `B-08` | Partial | `6` | Confirms class ownership API consistency; title unlock UI/eligibility remains manual. |
| `G-01` | Partial | `11` | Validates guild query projection shape; roster presentation validation remains manual. |

### Manual (46/57)

`T-01, T-02, C-03, C-04, C-05, S-01, S-02, S-03, S-04, P-01, P-02, P-03, P-04, P-06, P-07, P-08, P-09, P-10, P-12, K-01, K-02, K-03, K-04, I-01, I-02, I-03, I-04, I-05, X-01, X-02, X-03, A-01, A-02, A-03, Z-01, Z-02, Z-03, Z-04, B-01, B-02, B-03, B-04, B-05, B-06, B-07, B-09`

### Fast Automation Commands

- Baseline multiclass sanity: `#test 1-10` or `#test smoke`
- Combat/proc/pet/AA prechecks: `#test combat` (IDs `6,12,13,14,15`)
- Guild/AA sanity: `#test 11-15`
- Full current automated suite: `#test all`

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
2. `#test` now covers server-side assertions for `C-06` and `S-06`, and partial prechecks for `A-04`, `P-05`, and `P-11`; manual in-game behavior validation is still required.

---

## Session Header

**Date**: __________
**Tester**: __________
**Build/Branch**: __________
**Server Rules Snapshot**: `AbsolutePetLimit=__`, `EnablePetBags=__`, `MultipleTwoHandedProcs=__`, `DoubleAttackSkillRanged=__`, `ScaleBowByHDex=__`, `ScaleAutoAttackByHStr=__`, `DevastatingFrenzyDamageMultiplier=__`
`StaticInstanceVersion=__`, `StaticInstanceTemplateVersion=__`, `FarmingInstanceVersion=__`, `FarmingInstanceTemplateVersion=__`

---

## 0) Tooling UX (Optional)

### [T-01] Server Manager Main Layout

**Goal**: Verify new layout is usable for live operation.
**Steps**:

1. Open `server_manager.py`.
2. Go to `Server Control`.
3. Confirm left operations rail + right live workspace are visible.
4. Confirm service cards and runtime log viewer are usable.
**Expected**: Layout is coherent and actions are discoverable quickly.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [T-02] Runtime Log Focus Buttons

**Goal**: Verify rapid log targeting.
**Steps**:

1. Open Runtime Log Viewer.
2. Click `Active Zone Log`.
3. Click `Active World Log`.
4. Click `Most Recent Log`.
**Expected**: Log source changes correctly each time.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

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
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [C-02] Session Persistence

**Goal**: Verify multiclass data survives camp/relog.
**Steps**:

1. `#addclass 1` and `#addclass 2`
2. `#camp`, then log back in
3. `#addclass list`
**Expected**: Warrior + Cleric still present.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [C-03] Client Sync (Mana Bar)

**Goal**: Verify multiclass client state sync.
**Steps**:

1. Base Warrior.
2. `#addclass 12`
3. Check Player Window immediately and after relog.
**Expected**: Mana bar is visible and behaves correctly.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [C-04] Smart Spell Targeting

**Goal**: Verify implied targeting behavior.
**Steps**:

1. Ensure implied targeting rule is enabled.
2. Target hostile NPC targeting you.
3. Cast beneficial single-target spell.
4. Cast detrimental spell on same setup.
**Expected**: Beneficial reroutes to valid friendly implied target; detrimental remains hostile.
**Status**: [ ] Pass  [ ] Fail
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

### [P-05] Proc Parity: 2H and Bow

**Goal**: Verify THJ-aligned proc behavior.
**Steps**:

1. Verify `MultipleTwoHandedProcs = true`.
2. Test 2H weapon + proc aug rounds.
3. Test bow/ranged proc rounds.
**Expected**: 2H and bow proc behavior follows configured parity expectations.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [P-06] Ranger Point-Blank Autofire

**Goal**: Verify close-range ranger autofire behavior.
**Steps**:

1. Ranger class available, bow equipped.
2. Stand at melee distance.
3. Enable autofire.
**Expected**: No `RANGED_TOO_CLOSE` rejection for ranger path.
**Status**: [X] Pass  [ ] Fail
**Notes**: Good as of 2/24

### [P-07] Bow Augment Self-Heal Proc (THJ Style)

**Goal**: Verify bow-only heal proc augments trigger and heal self during ranged combat.
**Setup**:

1. Buy one of the new augs from `Gemcrafter_Tessu` (`NPC 52099`) or `Gemcrafter_Anuk` (`NPC 382051`) or `Gemcrafter_Lentos` (`NPC 394174`):
2. `1152012000` (`Lesser Bowstone of Mending`) `Level 20`
3. `1152012001` (`Bowstone of Mending`) `Level 40`
4. `1152012002` (`Greater Bowstone of Mending`) `Level 60`
5. `1152012003` (`Grand Bowstone of Mending`) `Level 80`
**Steps**:

1. Insert selected augment into bow.
2. Enable autofire and fight a valid target for multiple rounds.
3. Watch combat/chat output for heal proc messages.
4. Repeat at a level below the augment gate to verify it does not proc early.
**Expected**: Proc fires during ranged combat and heal lands on self; proc is blocked below its required level.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [P-08] Legacy Bow Scaling (hDEX + Minimum Clamp)

**Goal**: Verify THJ-style legacy bow scaling behavior when new dex formulas are disabled.
**Setup**:

1. Confirm `Combat:UseNewDexFormulas = false`.
2. Confirm `Custom:ScaleBowByHDex > 0`.
3. Confirm `Custom:ScaleBowMinimumDamageDivisor > 0` and `Custom:ScaleBowMinimumDamageMultiplier > 0`.
**Steps**:

1. Use a bow with low base damage and engage a valid target with autofire.
2. Record several hit values with low hDEX gear.
3. Repeat with high hDEX gear.
**Expected**: Higher hDEX produces higher ranged damage profile; floor clamping prevents unexpectedly low legacy-archery hits.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [P-09] Devastating Frenzy Legacy Scaling

**Goal**: Verify THJ-style berserker frenzy scaling against lower target HP.
**Setup**:

1. Confirm `Combat:UseNewDexFormulas = false`.
2. Confirm `Custom:DevastatingFrenzyDamageMultiplier > 0`.
3. Test character has Berserker class and uses Frenzy.
**Steps**:

1. Hit a target near full HP with Frenzy and record crit/frenzy damage range.
2. Lower target HP in ~20% bands and continue Frenzy attacks.
3. Compare damage profile as target HP drops.
**Expected**: Frenzy critical output scales upward as target HP decreases, with occasional large spike behavior matching THJ-style legacy scaling.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [P-10] Pet/NPC Weapon Instance Proc Parity

**Goal**: Verify pet/NPC attacks evaluate weapon-instance proc paths (including aug-based procs).
**Setup**:

1. Use a pet or controlled NPC with an equipped weapon instance (not just raw item ID fallback).
2. Ensure weapon/aug proc effects are valid for level and proc rules.
**Steps**:

1. Engage a target and let pet/NPC perform sustained melee rounds.
2. Observe combat output for innate weapon procs and augment procs.
3. Repeat with offhand if applicable.
**Expected**: Proc checks include instance-backed weapon data, and expected weapon/aug procs can fire during pet/NPC attacks.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

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
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [A-02] AA Purchase + Activate

**Goal**: Verify active AA from secondary class works.
**Steps**:

1. Grant AA points.
2. Buy secondary-class active AA.
3. Activate AA.
**Expected**: Purchase and activation succeed.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [A-03] Passive AA Effect

**Goal**: Verify passive AA effects apply correctly.
**Steps**:

1. Buy passive AA.
2. Validate effect in behavior/stats.
**Expected**: Passive bonus is applied and persistent.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 7) Custom Instances (THJ-Style)

### [Z-01] Non-Respawning Instance Request + Daily Lockout

**Goal**: Verify non-respawning expedition request is available and locked out for 24 hours after creation.
**Steps**:

1. Hail `Echo_of_the_Past` and choose `Non-Respawning`.
2. Confirm expedition is created and can be entered.
3. Attempt to request another non-respawning instance immediately.
**Expected**: First request succeeds; second request is blocked by replay lockout until 24 hours have elapsed.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [Z-02] Non-Respawning Spawn Behavior

**Goal**: Verify static instance kills do not naturally repop while instance is active.
**Steps**:

1. Enter a `Non-Respawning` instance and kill a normal mob.
2. Wait longer than that mob's normal respawn window.
3. Zone out/in and re-check the kill location.
**Expected**: Killed mob does not return during normal respawn windows.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [Z-03] Farming Instance Raid Suppression

**Goal**: Verify `Respawning` farming instance suppresses long-respawn/raid-style spawns.
**Steps**:

1. Enter a `Respawning` instance of a zone with known long-respawn raid targets.
2. Verify trash/XP mobs spawn as expected.
3. Verify known long-respawn raid target spawn points remain suppressed.
**Expected**: XP/trash population exists; raid-style long-respawn bosses are not present.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [Z-04] Lua Helper Availability (`eq.is_static_instance`, `eq.is_farming_instance`)

**Goal**: Verify THJ quest scripts can call custom instance helpers without runtime errors.
**Steps**:

1. Trigger a Lua quest script path that uses `eq.is_static_instance()` or `eq.is_farming_instance()`.
2. Repeat in both non-respawning and respawning instances.
**Expected**: No Lua nil-function errors; helper-based branch behavior matches instance type.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 8) THJ Bazaar + Waypoints

### [B-01] Magic Map Door Opens Waypoint UI

**Goal**: Verify Bazaar map object hook is wired (`doorid=146` -> `SendWaypointList`).
**Steps**:

1. Zone into `bazaar`.
2. Click the magic map object/disc at the Bazaar hub.
**Expected**: Waypoint list UI opens (no quest/script errors).
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [B-02] Tearel Group Feature Unlock

**Goal**: Verify Tearel dialogue and group-feature unlock path.
**Steps**:

1. Hail `Tearel`.
2. Follow dialogue for `[anchor yourself]` and pay required EOM.
3. Re-open map UI.
**Expected**: Group toggle / expedition return options are enabled after unlock.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [B-03] Waypoint Discovery Trigger

**Goal**: Verify `#TPTriggerN` discovery unlock flow.
**Setup**:

1. Apply `utils/sql/custom/2026_02_26_thj_waypoints_bazaar_bootstrap.sql` (includes `#TPTriggerN` spawn seeding from `thj_waypoints`).
**Steps**:

1. Enter a zone with `#TPTriggerN` proximity trigger.
2. Observe discovery message.
3. Re-open map UI and confirm zone appears unlocked.
**Expected**: First pass unlocks waypoint; repeat pass shows already-known messaging.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [B-04] Class Add NPCs (Guildmasters Class 20..35)

**Goal**: Verify Bazaar class-add NPCs drive global multiclass add flow.
**Steps**:

1. Hail one of the Bazaar class guildmasters (class 20..35 NPC).
2. Follow `class_select` -> `class_confirm`.
3. Run `#mcdiag` / `#autoskill list` / class-sensitive command to confirm ownership.
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

### [B-06] Pet Armory Merchant in Bazaar

**Goal**: Verify pet bag vendor availability in Bazaar.
**Steps**:

1. Open merchant window on `Pet_Armory_Quartermaster`.
2. Confirm class pet armory items are listed.
3. Purchase one bag and test active pet bag behavior with a pet class.
**Expected**: Merchant sells THJ pet bags and purchased bag works with pet equipment sync.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [B-07] THJ Quest API Binding Smoke Test

**Goal**: Verify imported THJ quest methods are fully bound (no undefined method errors at runtime).
**Setup**:

1. Use a build that includes the latest zone script binding changes.
2. Ensure zone logs are visible in the runtime log viewer.
**Steps**:

1. Trigger a script path that calls `$client->HasClass(\"Cleric\")` (example: `ikkinz/#Phantasmal_Priest.pl` dialog).
2. Trigger Bazaar pet rename path (`151061.pl`) that calls `$client->IsPetNameChangeAllowed()` and `GrantPetNameChange(class_id)`.
3. Trigger spell script `quests/global/spells/17785.pl` (`ConsumeItemOnCursor`) and `36892.pl` (`ConsumeUnspentAA`).
4. Trigger a progression/slayer script path using `IsSeasonal`, `GetKillCount`, and `CheckTitle`.
**Expected**: No Perl/Lua undefined-method errors; script behaviors execute normally.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [C-06] Zone-Load GestaltClasses Hydration

**Goal**: Verify `PlayerProfile.classes` is loaded from `data_buckets.GestaltClasses` on zone entry.
**Steps**:

1. Log out with a multi-class character.
2. Log back in and run `#mcdiag` immediately after entering world.
3. Zone once and run `#mcdiag` again.
**Expected**: Class bitmask is correct immediately at login and remains stable after zoning.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [S-06] Multiclass Spell Timer/Target Transform

**Goal**: Verify multiclass spell-load transforms in `shareddb` are active.
**Steps**:

1. Cast a spell with `ST_GroupClientAndPet` target behavior from a multiclass character and validate targeting behavior.
2. Use two disciplines from different owned classes that normally share timer families.
3. Re-use each discipline and observe lockout behavior.
**Expected**: Group+pet target transforms to direct target under multiclass rules; discipline timers are deconflicted by class.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [A-04] AA Multiclass Special Gates (Mnemonic/Fury)

**Goal**: Verify THJ-style AA special handling in multiclass mode.
**Steps**:

1. On a multiclass character, check visibility/usability of Mnemonic Retention.
2. On a non-pure caster multiclass, attempt Fury of Magic rank 6+ progression.
3. Repeat Fury of Magic rank 6+ on a pure caster multiclass.
**Expected**: Mnemonic Retention is allowed; Fury rank 6+ is blocked for non-pure casters and allowed for pure casters.
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

### [B-09] Bazaar and Back AA (Instance-Aware Return)

**Goal**: Verify THJ-style `Bazaar and Back` AA is auto-granted, ports to Bazaar, and returns to saved location including instances across relog.
**Steps**:

1. Log in on a fresh character and open AA window; confirm `Bazaar and Back` is present.
2. In a normal zone, activate `Bazaar and Back` and verify you port to Bazaar.
3. Activate it again in Bazaar and verify you return to your original coordinates.
4. Repeat from an instance (save location in instance, port to Bazaar, camp/relog in Bazaar, then use AA again).
5. Trigger AA twice quickly and verify reuse lockout message/cooldown behavior (~60s).
**Expected**: AA is auto-granted to all characters, uses 60s cooldown, and return location persists across relog with instance-aware restore.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [G-01] Guild Member Multiclass Projection

**Goal**: Verify guild roster payload includes `GestaltClasses` projection behavior.
**Steps**:

1. Put a known multiclass character in a guild.
2. Open guild member list (or inspect guild roster API output).
3. Compare class/level presentation to expected multiclass representation.
**Expected**: Guild member info reflects multiclass projection from `data_buckets.GestaltClasses`.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Suggested Workflow

If this still feels heavy, use this weekly cadence:

1. Daily dev loop: run only `Smoke Run`.
2. Pre-merge gate: run `Smoke Run` + all `P-*` tests.
3. Release candidate: run full tracker once.
