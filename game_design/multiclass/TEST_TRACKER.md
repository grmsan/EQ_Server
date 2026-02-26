# Multiclass Test Tracker

**Status**: Active Testing
**Last Updated**: 2026-02-26
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

## Run Packs

### Smoke Run

Run these first: `C-01, C-02, C-03, I-01, I-02, S-02, P-01, P-02, P-03, P-04, P-11, Z-01, Z-02`

### Full Regression

Run all tests in this document.

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

## Suggested Workflow

If this still feels heavy, use this weekly cadence:

1. Daily dev loop: run only `Smoke Run`.
2. Pre-merge gate: run `Smoke Run` + all `P-*` tests.
3. Release candidate: run full tracker once.
