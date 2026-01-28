# THJServer Multiclass Port: Test Tracker

This is the running test checklist for the “THJServer parity” multiclass port (`TODO_THJSERVER_MULTICLASS_PORT.md`).
**Master Technical Document:** [game_design/multiclass/IMPLEMENTATION_PLAN.md](game_design/multiclass/IMPLEMENTATION_PLAN.md)

---
## Current Bugs / Observations
- [ ] Trainer/spell vendor: mage spells still appear as `Ranger L255` on a `Ranger/Mage/Monk` (expected: usable + shows sane required level; ideally shows the correct class/level).
- [ ] Spell merchant "Show usable items": wizard-only spells (e.g., `Spell: Column of Frost` item id `15380`) still show up for non-wizard multiclass.
- [ ] Spell vendor details: shows base-class label for aggregated spells (e.g., `RNG(2)` instead of `MAG(2)`), plus `255` entries for unrelated classes.
- [ ] AA window: missing AAs for added classes (and sometimes missing even base-class AAs).
- [ ] Skills window: added-class skills (e.g., Mend/Tracking) inconsistent / missing.
- [ ] Item class masks: items that are `ALL` can appear restricted to the owned class trio (verify class mask logic and client display).
- [ ] Merchant filter source-of-truth unclear (item class mask vs spell-level usability); needs definitive instrumentation.
- [ ] Spell vendor UI shows base-class labels for aggregated spells (e.g., `RNG(2)` instead of `MAG(2)`) and `255` entries for unrelated classes.

## Smoke Tests (run after each port batch)

### A) Class Bits / Persistence
- [ ] Create new character; run `#multiclassdiag` and verify `GestaltClasses` bucket exists and matches base class bit.
- [ ] `#addclass <id>` then `#multiclassdiag` shows bit set; relog and verify it persists.
- [ ] `#removeclass <id>` then `#multiclassdiag` shows bit cleared; relog and verify it persists.

### B) UI Sync (RoF2 + DLL)
- [ ] On login, `#multiclassdiag refresh` and confirm `EdgeStatLabel` contains the expected class bitmask.
- [ ] HP/mana/end bars behave correctly when adding/removing caster classes.

### C) Spells (scribe + mem + cast)
- [ ] Add a caster class, scribe a low-level spell, memorize it (bar completes), then cast it.
- [ ] Remove that class and verify the spell fails cleanly (soft-locked) without client hangs.

### D) Spell Merchant / “Show Usable Items”
- [ ] With a multiclass that includes a caster class, toggle “Show usable items”.
  - Expected: includes spells for any owned caster class.
  - Expected: required levels are not all `255`.

### E) AAs
- [ ] Open AA window on single-class; confirm baseline AAs appear.
- [ ] Add a class with known class-only AAs; confirm they appear.
- [ ] Remove that class; confirm AAs are hidden/blocked per design.

### F) Skills
- [ ] Add Monk; verify Mend appears and can be used.
- [ ] Add Ranger; verify Tracking appears and can be used.
- [ ] Remove class and confirm skills fail/are gated (value may remain stored).

## Diagnostics to Capture When Something Breaks
- [ ] Client `dinput8_debug.log` around the action (vendor list open, trainer list open, AA window open).
- [ ] Zone console around add/remove class and any packet sends.
- [ ] `#multiclassdiag` output before and after the action.
