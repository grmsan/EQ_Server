# TODO: Multiclass (RoF2)

This file tracks multiclass-related work items and a repeatable in-game test checklist.

## Current Status / Known Bugs (keep updated)

**As of 2025-12-14**
- Multiclass bitmask is databucket-backed (`Custom:MulticlassBucketKey`) and can be modified via `#addclass/#removeclass`.
- `#mystats` chat output shows multiclass (classes_bitmask/count/classes) but the stats **window** still needs verification after changes.
- Skills seeding on `#addclass` was implemented (intended to make Tracking/Mend/etc appear) and `SkillCaps::GetSkillTrainLevel()` was fixed (previously could always return 0). Needs re-test.
- AA filtering/sync is still unstable (reports: missing AAs even for base class; missing AAs for added classes). Needs focused debug.
- Spell usability/UI is still unstable:
  - Reports: can scribe spells for added class but memorize bar can hang (progress never completes).
  - Reports: spell merchant "Show usable items" either (a) only filters to original class or (b) shows everything but all class required levels show as `255`.
- `/who` multiclass display is inconsistent (often shows only the original class). Must confirm `world.exe` is rebuilt/restarted and that `UpdateWho()` is triggered on class changes.
- Fast camp testing rules existed but camp still took ~30s; server now disconnects on camp timer expiry (re-test with the rules enabled).

**Recent changes that require re-test**
- DLL: `EQ_Spell::GetSpellLevelNeeded` detour now tries to auto-detect class-id vs class-index arguments and uses the multiclass bitmask to choose the best required level.
- DLL: `EQ_Character::GetUsableClasses` detour now falls back to base-class-only (instead of “all classes”) until the server multiclass mask arrives.
- DLL: `PcZoneClient::GetPcSkillLimit` detour now exposes skills that the server has already granted (so they appear in the skills window even if base class cap would be 0).
- World: `/who` multiclass suffix now reads the databucket even if `Custom:MulticlassingEnabled` is out-of-sync between world/zone (still requires rebuild/restart).

**Blockers (P0)**
- [ ] Spell merchant "Show usable items" must include spells for any owned class (not just original).
- [ ] Memorize spell must complete or fail cleanly (no stuck progress bar).
- [ ] AA window must show at least all base-class AAs; then extend to added classes.
- [ ] “All classes equal” goal: remove “base class feels special” in client filters (vendor/spell/AA/skills) by using multiclass ownership everywhere.

## Quick Smoke Test (repeat after each change)

**Setup**
- Use a fresh login (client restart) after updating `dinput8.dll`.
- Confirm server binaries were restarted after rebuild (especially `zone.exe`/`world.exe`).
- In-game: `#mystats` (confirm `classes_bitmask`, `count`, and class list)
- In-game: `#addclass list` (confirm IDs and which are enabled)
- Optional: `#logs set console 83 2` (packet S->C) and `#logs set console 46 2` (HP Update) while diagnosing sync issues

**Core stats/UI**
- Verify HP bar moves when damaged (`#damage 1000`) and matches `#mystats`
- Verify mana/endurance behavior matches expected classes
- Verify AA window shows AAs appropriate to owned classes

**Spells (hybrids/casters)**
- Add a casting class (example): `#addclass 2` (Cleric) or `#addclass 12` (Wizard) or `#addclass 4` (Ranger)
- Verify spellbook opens
- Verify mana bar appears when applicable
- Verify scribing a spell scroll works
- Verify memorizing a spell works (bar completes). If it hangs, capture `dinput8_debug.log` + zone console output around OP_MemorizeSpell.

**Spell merchant usability filter**
- Add a caster class (Mage/Wizard/Cleric)
- Go to a spell vendor, toggle “Show usable items”
  - Expected: spells for all owned caster classes are included (not only original class)
  - Expected: still filters “unlearned only” as the client normally does

**Disciplines**
- Add a discipline class (example): `#addclass 4` (Ranger)
- Verify learning a discipline from a tome works

**Regression**
- Remove a class: `#removeclass <id>`
- Verify UI/AA/spells update (may require rezone/relog depending on what changed)

## Suggested Test Matrix (small, repeatable)

Run these as minimal “acceptance tests” as multiclass grows.

**T0: Baseline**
- No extra classes (single-class).
- Verify: HP/mana/end, spellbook behavior, skills window, AAs all match classic expectations.

**T1: Warrior + Ranger (hybrid)**
- `#addclass 4`
- Verify: spellbook opens, mana bar appears, scribe/mem a ranger spell, learn/use a ranger discipline, ranger AAs visible.
- `#removeclass 4`
- Verify: ranger spell/AA/disc becomes unusable (soft-lock); no crashes/desync.

**T2: Warrior + Cleric (caster)**
- `#addclass 2`
- Verify: spellbook opens, mana bar appears, scribe/mem cleric spell, cleric AAs visible.

**T3: Warrior + Wizard (INT caster)**
- `#addclass 12`
- Verify: spellbook opens, mana bar appears, specialization skills appear at/after the correct level, wizard AAs visible.

**T3b: Ranger + Mage (merchant “usable items”)**
- Start Ranger, `#addclass 13` (Mage)
- Spell merchant: “Show usable items” should include Mage spells.
- Spellbook: scribe + mem a Mage spell should work; bar should not hang.

**T4: Add/Remove churn**
- Start with 3 classes (example): Warrior + Cleric + Wizard
- Remove one, add a different one, repeat.
- Verify: no "stuck UI" (spellbook/mana/AA/skills) and server consistently enforces `classes_bitmask`.

## Work Items (Prioritized)

### P0 (player-facing correctness)
- [ ] “Show usable items” / spell vendor filter
  - Goal: vendor filter uses multiclass ownership, not only original class.
  - Repro: add Mage to Ranger, open spell vendor, toggle “Show usable items”, confirm Mage spells appear.
  - Debug: capture `dinput8_debug.log` + confirm DLL was updated in correct client folder.

- [ ] Spell memorize must not hang
  - Goal: either mem completes or server denies and the client UI resets immediately (no stuck progress bar).
  - Repro: add Mage to non-caster, scribe Mage spell, attempt mem; observe progress behavior.
  - Debug: zone console around `OPMemorizeSpell`; enable `#logs set console 83 2` and capture `dinput8_debug.log`.

- [ ] AA window completeness (base + added)
  - Goal: AA window shows all AAs for any owned class; base class should never “lose” AAs.
  - Repro: on a fresh single-class character, verify AA list baseline; then add a class and verify union list (or at least added class AAs appear).
  - Debug: use `#reloadaa` (if enabled) / server AA logging; confirm `aa_ability.classes` bitmask semantics.

- [ ] Persistence + gating policy (soft-lock)
  - Goal: learned spells, purchased AAs, and trained skills remain stored on the character; entitlement is checked at use-time (and ideally at display-time).
  - Test: learn something class-specific, remove that class, confirm it becomes unusable (not deleted), then re-add class and confirm it works again.
    - Spells: learn/scribe a spell, remove that class, verify you cannot mem/cast it, re-add and verify you can mem/cast.
    - Skills: train a skill high (e.g., Kick) then remove all classes that can use it; verify server rejects using it; re-add a valid class and verify it works again.

- [ ] Class removal UX: grant a free AA reset entitlement (do not auto-wipe immediately)
  - Goal: removing a class grants “you may reset AA for free” (player chooses when to spend it); don’t wipe all AAs instantly.
  - Test: remove a class, verify entitlement increments/appears; verify the player can reset AA later (and that the entitlement is consumed only when used).

- [ ] Audit “use-time” enforcement (server is always authoritative)
  - Goal: any action that produces gameplay effect must validate against `classes_bitmask` (not `GetClass()`).
  - Typical action surfaces to audit and test:
    - Skills: hotbar “Kick/Taunt/Backstab” etc (client sends skill/ability packet)
    - Spells: mem, cast, interrupts, and spell bar usability
    - Disciplines: tome learn + discipline activate
    - AAs: purchase (if restricted), activate (AAAction), passive benefit application
    - Combat abilities: disciplines/combat abilities that are class-limited
  - Test: pick one class-only action per system, remove that class, attempt to use it, verify server rejects cleanly (no desync, no crash).

- [ ] Passive effects enforcement (removed classes stop granting benefits)
  - Goal: if a class bit is removed, any passive AA bonuses / passive granted effects tied to that class should stop applying.
  - Test pattern:
    - Add class → acquire passive AA that changes a visible stat (HP/Mana/AC/regen/haste) → record baseline (`#mystats`, `#hpdiag`)
    - Remove class → verify the bonus disappears
    - Re-add class → verify bonus returns (or returns after resync/relog if that’s the chosen design)

- [ ] Skills: union-of-classes availability + caps (server + DLL)
  - Goal: skills window / trainers / caps reflect the union of owned classes (not just base class).
  - Server tests:
    - Add Wizard to a non-caster, verify specialization skills are trainable/available at the correct level.
    - Remove Wizard, verify specialization cannot be trained/used (value may remain stored).
  - Client/UI tests:
    - Skill window shows the skill when any owned class supports it (and hides/greys it when no owned class supports it).
    - Trainer UI offers appropriate skills when any owned class supports it.

- [ ] Skill execution gating (even if the value exists)
  - Goal: having a non-zero skill value must not allow using a skill unless at least one owned class can use it.
  - Test: give/train a skill, remove the enabling class, attempt to use the skill via hotbar; verify the server rejects with a clear message.

- [ ] Spell required-level checks (client should pick a valid owned class)
  - Goal: for a multiclass character, required-level lookups should use “best owned class” so spells don’t incorrectly grey out.
  - Test: pick a spell where class level requirements differ; verify it is not greyed and that mem/cast works when at/above the best valid class level.

- [ ] Trainers (policy decision)
  - Option A: trainer must match any owned class (classic-ish, but multiclass-aware).
  - Option B: allow cross-class trainers universally (simpler UX).
  - Test: base class Warrior + add Wizard; try training Wizard-only skills at a Wizard trainer and at a Warrior trainer; confirm chosen behavior.

- [ ] Refresh behavior (define what requires relog/rezone)
  - Goal: define when we can live-refresh vs when we require rezone/relog (AA list, spellbook gating, skill list, mana bar).
  - Test: add/remove class and attempt to observe the change without relog; if it doesn't refresh, document the minimum reliable action (close/reopen window, rezone, relog).

### P1 (polish/UX)
- [ ] Decide "presentation class" strategy (fixed class vs selectable vs derived)
- [ ] Class change UX: decide whether to auto-unmem invalid spells / remove invalid combat abilities from hotbars (or just fail cleanly)
- [ ] Add "effective class" visibility in `#mystats` (mana_class/skill_class/spell_req_class decisions)
- [ ] `/who` formatting: confirm the chosen display pattern works in all client views
- [ ] Add a single `#multiclassdiag` command to dump: classes bitmask, derived "effective class" choices (mana/skills/spells), last sent edge stats
- [ ] Login screen should show all classes assigned to that character. i.e. Instead of 'warrior' we show 'warrior/druid/magician'

### P2 (design expansion)
- [ ] “Effective class” rules per system (mana, skills, AA, spellcasting, disciplines, item procs)
- [ ] Multi-class leveling rules (how/when does each class unlock spells/AAs/skills)
- [ ] Presentation-class storage (per-character) if “selectable presentation class” is chosen
- [ ] Cooldowns/limits: rules for adding/removing classes (costs, lockouts, free changes) and how they interact with AA reset entitlements

## When Something Breaks (what to capture)

**Client**
- `dinput8_debug.log` from the EQ client folder
- Confirm `_options.h` toggles in use (MQ injects on/off, debug logging, multiclass overrides)
- Confirm you copied the new `dinput8.dll` into the correct RoF2 client directory (and restarted the client).

**Server**
- Zone console output around the event
- `#hpdiag` output if HP/mana/end issues are involved
- The exact class mask before/after (copy `#mystats` line)
- Confirm server was restarted after rebuild (no stale `zone.exe`/`world.exe`).

## Testing Rules / Toggles (for rapid iteration)

**Multiclass core**
- `Custom:MulticlassingEnabled`
- `Custom:MulticlassBucketKey`
- `Custom:MulticlassMaxClasses`

**Fast camp (testing)**
- `Character:EnableHackedFastCamp` (bool)
- `Character:HackedFastCampTimerMS` (int)

## Notes / Decisions Log

Add short entries here when we learn something new (what changed, what broke, what fixed it).
