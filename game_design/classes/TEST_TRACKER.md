# Classes Test Tracker

**Status**: Active Testing
**Tracker Area**: Classes
**Tracker State**: Active
**Last Updated**: 2026-02-27

## Purpose

Tracks class-specific combat behavior, custom class abilities, and class-tuned formula paths.

## How To Use

1. Fill out `Session Header`.
2. Run `Smoke Run` first.
3. Mark each test `Pass` or `Fail`.
4. Add direct evidence (chat lines, logs, damage samples).

## Session Header

**Date**: __________
**Tester**: __________
**Build/Branch**: __________
**Rules Snapshot**: `UseNewDexFormulas=__`, `ScaleBowByHDex=__`, `ScaleBowMinimumDamageDivisor=__`, `ScaleBowMinimumDamageMultiplier=__`, `DevastatingFrenzyDamageMultiplier=__`

## Run Packs

### Smoke Run

Run: `CL-01, CL-02, CL-06`

### Full Regression

Run all tests in this document.

---

## 1) Custom Class AAs

### [CL-01] Heroic Throw AA Visibility + Activation

**Goal**: Verify custom warrior AA `Heroic Throw` is visible and functional.
**Steps**:

1. Use a warrior with required rank/level.
2. Open AA window and confirm `Heroic Throw` entry is present.
3. Activate AA on valid target and observe result.
4. Re-activate before cooldown ends.
**Expected**: AA is visible, activates successfully on target, and enforces cooldown/recast.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [CL-02] Colossal Smash AA Visibility + Activation

**Goal**: Verify custom warrior AA `Colossal Smash` is visible and functional.
**Steps**:

1. Use a warrior with required rank/level.
2. Open AA window and confirm `Colossal Smash` entry is present.
3. Activate AA in combat on valid target.
4. Confirm expected combat output and recast behavior.
**Expected**: AA is visible, ability fires with expected result, and recast rules apply.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [CL-03] Warrior AA Export/Client Data Parity

**Goal**: Verify server spell/DB entries for custom warrior AAs match client exports.
**Steps**:

1. Confirm server has expected spell and AA rows for `Heroic Throw` and `Colossal Smash`.
2. Run export path for client spell/dbstr assets.
3. Validate client can see AA names/descriptions and activate both AAs.
**Expected**: No client/server mismatch for names, descriptions, spell IDs, or activation behavior.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 2) Class Combat Behavior

### [CL-06] Ranger Point-Blank Autofire

**Goal**: Verify close-range ranger autofire behavior.
**Legacy ID**: `P-06`
**Steps**:

1. Ranger class available, bow equipped.
2. Stand at melee distance.
3. Enable autofire.
**Expected**: No `RANGED_TOO_CLOSE` rejection for ranger path.
**Status**: [X] Pass  [ ] Fail
**Notes**: Good as of 2/24

### [CL-07] Bow Augment Self-Heal Proc (THJ Style)

**Goal**: Verify bow-only heal proc augments trigger and heal self during ranged combat.
**Legacy ID**: `P-07`
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

### [CL-08] Legacy Bow Scaling (hDEX + Minimum Clamp)

**Goal**: Verify THJ-style legacy bow scaling behavior when new dex formulas are disabled.
**Legacy ID**: `P-08`
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

### [CL-09] Devastating Frenzy Legacy Scaling

**Goal**: Verify THJ-style berserker frenzy scaling against lower target HP.
**Legacy ID**: `P-09`
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
