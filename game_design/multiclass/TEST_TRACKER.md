# Multiclass Test Tracker

**Status**: Active Testing
**Tracker Area**: Multiclass
**Tracker State**: Active
**Last Updated**: 2026-04-26
**Technical Plan**: [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)

## Purpose

This tracker is the validation source of truth for multiclass work. Cases are intentionally scenario-level so an in-game tester can validate meaningful behavior without running repetitive micro-checks.

Use [../tooling/TEST_TRACKER.md](../tooling/TEST_TRACKER.md) for Server Manager and tooling UI tests. Use [../classes/TEST_TRACKER.md](../classes/TEST_TRACKER.md), [../mechanics/TEST_TRACKER.md](../mechanics/TEST_TRACKER.md), [../quests/TEST_TRACKER.md](../quests/TEST_TRACKER.md), and [../qol/TEST_TRACKER.md](../qol/TEST_TRACKER.md) for non-multiclass domain validation.

## Status Rules

- `Not Run`: Ready for validation but not executed in the current build.
- `In Progress`: Actively being tested or partially executed.
- `Blocked`: Cannot be objectively validated because of environment, data, client, or design dependency.
- `Pass`: Expected behavior is confirmed with objective in-game, command, or log evidence.
- `Fail`: Behavior mismatches expected results or produces a regression.

## Evidence Format

Keep notes short and reproducible:

`Observed:` one-line actual result.
`Evidence:` exact chat/log snippet, command output, screenshot reference, or visible UI result.
`Commands:` minimal command sequence.
`Next:` immediate code/data/design target if blocked or failed.

## Current Validation Focus

1. Start with `AUTO-01` and `CORE-01` after every fresh build.
2. Validate the main multiclass player loop with `CAST-01`, `AA-01`, `ITEM-01`, and `ENT-01`.
3. Run `COMBAT-01`, `PET-01`, and `SKILL-01` when combat/class behavior changed.
4. Run `WORLD-01`, `PROG-01`, and `UI-01` before merge/release or after client/DLL changes.

## Global Commands

- `#addclass <id>`: Add class (`1=WAR, 2=CLR, 3=PAL, 4=RNG, 5=SHD, 6=DRU, 7=MNK, 8=BRD, 9=ROG, 10=SHM, 11=NEC, 12=WIZ, 13=MAG, 14=ENC, 15=BST, 16=BER`)
- `#removeclass <id>`
- `#addclass list`
- `#multiclassdiag`
- `#level <n>`
- `#setskill <id> <value>`
- `#set aa_points <n>`
- `#autoskill list`
- `#showstats`
- `#test smoke`
- `#test combat`
- `#test automated`
- `#test regression`
- `#test 16-20`: DLL client probe suite

## Run Packs

### Smoke Run

Run `AUTO-01`, `CORE-01`, `CAST-01`, and `AA-01`.

### Gameplay Regression

Run `CORE-01`, `CAST-01`, `CAST-02`, `AA-01`, `COMBAT-01`, `PET-01`, `SKILL-01`, `ITEM-01`, and `ENT-01`.

### Release Regression

Run every active case in this tracker.

---

## 1) Automation And Baseline

### [AUTO-01] Automated Multiclass Sanity

**Goal**: Verify the automated server-side and DLL-probe checks are green before manual in-game validation.
**Steps**:

1. Build `zone`, `world`, and the EQ Core DLL.
2. Start the server stack.
3. Run `#test smoke`, `#test combat`, and `#test 16-20`.
4. Review server logs for failed assertions or missing DLL callback responses.
**Expected**: All automated checks complete without assertion failures; DLL callback tests report expected class/mana/snapshot parity.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [CORE-01] Class Ownership, Sync, And Persistence

**Goal**: Verify adding and removing classes updates the server, client-facing profile, and persisted state.
**Steps**:

1. On a clean test character, run `#addclass list`, then add two classes including one caster.
2. Confirm `#addclass list` and `#multiclassdiag` agree.
3. Check the client-facing class/mana UI after mutation.
4. Zone, camp, relog, and confirm the same ownership state remains.
5. Remove one added class and confirm diagnostics/client state update again.
**Expected**: Runtime class mask, profile/client state, mana visibility, and persisted `GestaltClasses` stay consistent through add, remove, zone, and relog.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 2) Spell And Casting Scenarios

### [CAST-01] Mage Add End-To-End

**Goal**: Validate the core player-facing multiclass caster loop using Magician as the added class.
**Steps**:

1. Start on a non-caster base class and add Magician (`#addclass 13`).
2. Confirm mana UI appears and multiclass diagnostics include Magician.
3. Scribe and memorize a Magician spell appropriate for the test level.
4. Cast the spell on a valid target and verify mana, cast result, cooldown, and spell effect.
5. Buy or grant a Magician AA, then use one active AA or verify one passive AA effect.
6. Try one Magician-restricted item, augment, or click effect that should now be allowed.
**Expected**: Added Magician ownership unlocks caster UI, spell learning/casting, relevant AA access, and class-restricted interactions without breaking the base class.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [CAST-02] Bard, Group, And Targeting Behavior

**Goal**: Verify special spell/song behavior remains correct when class ownership changes.
**Steps**:

1. Add Bard to a non-Bard and run one song long enough to observe pulse/refresh behavior.
2. Add a group-capable caster class and cast a group spell with valid and invalid group targets.
3. Test one beneficial implied target and one detrimental target case.
4. Confirm spell target transforms loaded by `#test` still match live cast behavior.
**Expected**: Songs pulse as intended, group spells hit valid recipients, implied targeting respects friendly/hostile rules, and loaded target transforms match live behavior.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 3) Combat, Pets, Skills, And Disciplines

### [COMBAT-01] Melee, Ranged, Autoskills, And Class Combat Gates

**Goal**: Verify owned secondary classes affect combat gates without creating spam, cooldown, or proc regressions.
**Steps**:

1. Add a melee class with an autoskill and verify the autoskill loop during melee.
2. Test autofire or ranged combat if the owned class should enable related behavior.
3. Add Ranger or Berserker and verify one combat gate that should use `HasClass` ownership.
4. Fight long enough to confirm cooldown cadence and no recovery spam.
**Expected**: Combat gates use owned classes, autoskills fire at valid intervals, ranged/autofire behavior remains stable, and no unexpected cooldown/proc spam appears.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [PET-01] Multi-Pet, Pet Bags, And Charmed Inventory

**Goal**: Verify multi-pet behavior and THJ-style pet equipment handling in one live pet scenario.
**Steps**:

1. Add a pet class and summon at least one permanent pet.
2. Add or swap another pet-capable class and verify active/focused pet commands.
3. Equip or edit a class pet bag and confirm matching pet equipment sync.
4. Zone/camp/relog and verify permanent pet state restores.
5. Charm a test NPC, exercise pet bag sync if applicable, then break charm and verify original inventory is restored.
**Expected**: Pet focus, commands, persistence, bag equipment sync, and charmed inventory restore behave coherently across owned pet classes.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [SKILL-01] Skills, Trainers, And Discipline Use

**Goal**: Verify class ownership opens expected skill/trainer/discipline paths.
**Steps**:

1. Add a class with visible skills and compare `#showstats`, Skills window, and trainer access.
2. Train at least one added-class skill if available.
3. Learn and activate one discipline from an owned secondary class.
4. Test two disciplines from different owned classes that should not share a lockout family.
5. Add Monk if needed and verify one monk special attack path.
**Expected**: Skill caps use the best owned class, trainer access does not base-class deny incorrectly, disciplines learn/use correctly, and timer families remain distinct.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 4) Items, AAs, Entitlement, And Progression

### [ITEM-01] Class-Restricted Items, Augments, Merchants, And Click Effects

**Goal**: Validate the main item entitlement surface for owned and non-owned classes.
**Steps**:

1. Attempt to equip a class-restricted item before and after adding the qualifying class.
2. Attempt one class-restricted augment before and after adding the qualifying class.
3. Use one class-restricted click effect that should be allowed by owned class state.
4. Open a merchant with class-filtered stock before and after class ownership changes.
5. On a Cleric/Monk/Magician or similar mixed-class character, equip a Monk-usable no-required-level weapon into Secondary with an empty offhand.
6. Confirm one invalid race or unrelated restriction still blocks correctly.
**Expected**: Owned class state allows valid class-restricted item paths, Monk-owned characters can use the Secondary slot at level 1, non-owned classes remain denied, race/other restrictions still apply, and merchant filtering updates from current ownership.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [AA-01] AA Visibility, Purchase, Activation, And Special Gates

**Goal**: Verify multiclass AA behavior from discovery through use.
**Steps**:

1. Add two AA-rich classes and open the AA window.
2. Grant AA points and purchase one active AA from a secondary class.
3. Activate the purchased AA and verify cooldown/effect.
4. Verify one passive AA only applies while the owning class is present.
5. Check special gates for Mnemonic Retention and Fury of Magic pure-caster policy if relevant to the build.
**Expected**: AA visibility, purchase, activation, passive effects, and special multiclass gates follow owned-class policy.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [AA-02] Dynamic AA Timer Stability Across AA Table Refresh

**Goal**: Verify multiclass AAs keep a stable dynamic cooldown family when the AA table is rebuilt.
**Steps**:

1. Confirm `Custom:UseDynamicAATimers` is enabled via `#test 2` or `#rules get Custom:UseDynamicAATimers`.
2. On a multiclass character with at least two active AAs that normally use shared recast timers, open the AA window and note each AA's displayed cooldown family/recast grouping.
3. Use one active AA, then trigger an AA table rebuild by zoning, relogging, or changing owned classes.
4. Reopen the AA window and verify the same AAs still present the same cooldown family grouping and reuse the expected cooldown instead of shifting to a different timer.
5. Activate the second AA if it should share the cooldown and confirm the server/client both reflect the expected remaining reuse timer.
**Expected**: Dynamic timer IDs remain stable across AA table refreshes, shared cooldown AAs stay grouped correctly, and no AA shifts to a different reuse family after zoning, relogging, or class mutation.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [ENT-01] Class Removal Soft-Lock Policy

**Goal**: Verify removing a class has a clear entitlement outcome for spells, AAs, items, and UI.
**Steps**:

1. Add a caster class, scribe/memorize a spell, purchase or grant one class AA, and equip or use one class-restricted item.
2. Remove that class.
3. Attempt to cast or rememorize the spell.
4. Reopen the AA window and attempt to activate the affected AA.
5. Recheck item equip/click behavior and record whether the system soft-locks, clears, refunds, or blocks each entitlement.
**Expected**: Removed-class entitlements follow the chosen policy consistently and do not leave hung spell gems, usable invalid AAs, stale UI, or invalid item access.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: Pending final design decision for removed-class AA refund/reset versus soft-lock policy.

### [PROG-01] XP, Level Cap, Regen, Hunger, And Scaling Rules

**Goal**: Verify multiclass ownership does not break progression or passive stat rules.
**Steps**:

1. Move a test character near a level or XP cap edge and grant XP.
2. Compare XP gain against configured class modifier expectations.
3. Observe regen and hunger/food tick behavior across relevant class/race combinations.
4. Confirm multiclass ownership does not bypass intended level cap or XP cap rules.
**Expected**: XP, level cap, regen, and hunger behavior follow the intended ruleset without unintended cap bypasses or passive stat drift.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 5) World, Scripts, And Presentation

### [WORLD-01] Bazaar Class NPCs, Titles, Quests, And Guild Projection

**Goal**: Verify world/script systems consume multiclass ownership correctly.
**Steps**:

1. Use Bazaar class-add NPCs to add at least one class and confirm the same state as command-based add.
2. Use Vision_of_Ayonae or the class removal path and confirm lockout/cost/reroll rules.
3. Attempt one class-dependent quest or hand-in.
4. Verify one class-gated title eligibility check.
5. Refresh guild roster/member display after class mutation.
**Expected**: NPC scripts, quest gates, title checks, and guild projection use current owned class state rather than only base class.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [UI-01] Client Presentation And Diagnostics

**Goal**: Verify visible client presentation matches the current multiclass display policy.
**Steps**:

1. Check character select after a known multiclass state is saved.
2. Run `/who` and verify no corrupt or unknown class output appears.
3. Run `#mystats` or `#multiclassdiag` and verify class ownership is visible and accurate.
4. Open inventory after a class mutation and verify class labels/abbreviations reflect the intended DLL policy.
5. Confirm the presentation remains correct after zoning and relog.
**Expected**: Character select, `/who`, diagnostics, inventory labels, and relog presentation reflect the chosen multiclass display policy without falling back to stale compatibility/base-class output.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Agent Closeout Requirement

When an agent validates or changes multiclass work, it must update this tracker through Server Manager or by editing the same fields:

1. Set one clear status.
2. Add concise notes using `Observed`, `Evidence`, `Commands`, and `Next`.
3. Mark blocked cases as `Blocked`, not `Fail`, unless the game behavior itself failed.
4. Prefer adding/updating scenario cases over creating micro-cases.
