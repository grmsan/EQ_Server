# Multiclass Test Tracker

**Last Updated:** 2026-01-31
**Master Technical Document:** [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)
**Port Checklist:** [PORT_CHECKLIST.md](PORT_CHECKLIST.md)

---

## Setup & Quick Reference

### Test Character
- Use `#addclass <class_id>` to add a second class (e.g., `#addclass 6` for Druid)
- Use `#addclass list` to see all classes with status (X = enabled)
- Use `#multiclassdiag` for full diagnostic dump
- Class IDs: 1=WAR, 2=CLR, 3=PAL, 4=RNG, 5=SHD, 6=DRU, 7=MNK, 8=BRD, 9=ROG, 10=SHM, 11=NEC, 12=WIZ, 13=MAG, 14=ENC, 15=BST, 16=BER

### Log Locations
| Log Type | Location | How to Enable |
|----------|----------|---------------|
| Server multiclass debug | `logs/zone/Custom_MulticlassDebug.log` | Already enabled via LogSys |
| DLL debug log | `<RoF2 client folder>/dinput8_debug.log` | Automatic when DLL loaded |
| Server zone log | `logs/zone/zone_*.log` | Standard zone logging |

### Debug Commands
- `#addclass list` - Shows all 16 classes with X for enabled ones
- `#multiclassdiag` - Full multiclass state dump (bitmask, EdgeStat, etc.)
- `#multiclassdiag refresh` - Force resend EdgeStatLabel to client DLL
- `#logs set Custom:MulticlassDebug 3` - Enable multiclass debug logging (level 3 = detail)

### Quick Test Commands
```
#addclass 13              # Add Magician class
#addclass list            # See all classes (X = enabled)
#multiclassdiag           # Full diagnostic dump
#multiclassdiag refresh   # Force resend EdgeStatLabel to DLL
```

---

## Current Bugs / Observations

### Fixed
- [x] Melee Progression: Berserker Frenzy and Monk special attacks (Flying Kick, etc) use bitmask logic ([special_attacks.cpp](special_attacks.cpp)).
- [x] Defense Scaling: Any character with Warrior/Knight bits receives native AC softcaps and return scalars ([attack.cpp](attack.cpp)).
- [x] Spell Casting: Bard song scaling/casting level logic uses bitmask ([spells.cpp](spells.cpp)).
- [x] Tradeskills: Alchemy and Poison making unlocked for any character with Shaman/Rogue bits ([tradeskills.cpp](tradeskills.cpp)).
- [x] Base Stats: HP/Endurance regeneration now uses the "Best of Class" union logic ([client_mods.cpp](client_mods.cpp)).
- [x] Mixed-Class Procs: ShadowKnight "Vampiric Embrace" and similar procs now trigger for multiclass SKs ([spell_effects.cpp](spell_effects.cpp)).
- [x] Item Click/Bard UI: Casting logic for item clicks respects Bard rules for multiclass Bards ([client_packet.cpp](client_packet.cpp)).
- [x] Skill Caps/Training: Skills learn at the earliest level and cap at the highest value among all owned classes ([skill_manager.cpp](skill_manager.cpp)).
- [x] ~~Spell merchant "Show usable items": spells for non-owned classes visible~~ **FIXED 2026-01-31** - DLL GetUsableClasses whitelist corrected
- [x] ~~Spell tooltips showing L255 for all classes~~ **FIXED 2026-01-31** - Now shows correct native levels

### Open
- [ ] DEX Twincast Scaling: Verify if high DEX grants twincast chance for any multiclass bit (Refactor pending in [mob.cpp](mob.cpp)).
- [ ] Spell vendor details: shows base-class label for aggregated spells (e.g., `RNG(2)` instead of `MAG(2)`), plus `255` entries for unrelated classes.
- [ ] AA window: missing AAs for added classes (and sometimes missing even base-class AAs).
- [ ] Skills window: added-class skills (e.g., Mend/Tracking) inconsistent / missing.
- [ ] Item class masks: items that are `ALL` can appear restricted to the owned class trio (verify class mask logic and client display).

---

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

### D) Spell Merchant / "Show Usable Items"
- [x] With a multiclass that includes a caster class, toggle "Show usable items".
  - ✓ Includes spells for any owned caster class.
  - ✓ Hides spells for non-owned caster classes.
  - ✓ Required levels display correctly (not all `255`).

### E) AAs
- [ ] Open AA window on single-class; confirm baseline AAs appear.
- [ ] Add a class with known class-only AAs; confirm they appear.
- [ ] Remove that class; confirm AAs are hidden/blocked per design.

### F) Skills
- [ ] Add Monk; verify Mend appears and can be used.
- [ ] Add Ranger; verify Tracking appears and can be used.
- [ ] Remove class and confirm skills fail/are gated (value may remain stored).

---

## Detailed User Verification Procedures

Use these procedures to verify the core multiclass logic. Replace `<id>` with your character ID or use on yourself.

### 1. Verification: Best-of-Class Melee Special
**Goal**: Verify a Monk/Warrior uses Flying Kick instead of regular Kick.
1.  `#level 60`
2.  `#addclass 1` (Warrior)
3.  `#addclass 7` (Monk)
4.  `#setskill 38 400` (Skill: Flying Kick)
5.  `#setskill 30 400` (Skill: Kick)
6.  `#spawn 10` (Any mob)
7.  Turn on auto-attack.
8.  **Expected**: You should see "You try to flying kick..." in the combat log. If you remove the Monk bit (`#removeclass 7`), you should revert to "You try to kick...".

### 2. Verification: Defensive Parity (AC Returns)
**Goal**: Verify a Wizard/Warrior gains Warrior-tier AC scaling.
1.  `#level 60`
2.  `#addclass 12` (Wizard)
3.  `#addclass 1` (Warrior)
4.  Equip high-AC plate armor.
5.  Note your "Total AC" in inventory.
6.  `#removeclass 1` (Warrior bit gone)
7.  **Expected**: Total AC should drop significantly (Wizards have a 0.20 return scalar, Warriors have 0.35). Restoring the bit (`#addclass 1`) should bring the AC back up.

### 3. Verification: Skill Training Union
**Goal**: Verify a Ranger can learn Kick at Level 1 (via Warrior bit).
1.  Create a fresh Level 1 character (e.g. Ranger).
2.  `#addclass 4` (Confirm Ranger)
3.  `#addclass 1` (Add Warrior)
4.  `#setskill 30 1` (Try to set Kick)
5.  **Expected**: The skill should successfully be set to 1. Without the Warrior bit, a Level 1 Ranger cannot train Kick (requires Level 5 natively).

### 4. Verification: Tradeskill Unlocking
**Goal**: Verify a Cleric/Rogue can create poisons.
1.  `#level 60`
2.  `#addclass 2` (Cleric)
3.  `#addclass 9` (Rogue)
4.  `#setskill 56 200` (Skill: Make Poison)
5.  Obtain poison components and a Mortar & Pestle.
6.  Attempt a combine.
7.  **Expected**: The combine should proceed. Without the Rogue bit, the character would be blocked from the "Make Poison" skill.

### 5. Verification: Bard Song Scaling
**Goal**: Verify a Paladin/Bard scales songs correctly.
1.  `#level 60`
2.  `#addclass 3` (Paladin)
3.  `#addclass 8` (Bard)
4.  Memorize a low-level Bard song (e.g. *Selo's Accelerando*).
5.  Cast the song.
6.  **Expected**: The buff duration and movement speed should scale for Level 60. Without the multiclass logic, the Bard level would default to 1, resulting in a 1-tick duration.

---

## Diagnostics to Capture When Something Breaks
- [ ] Client `dinput8_debug.log` around the action (vendor list open, trainer list open, AA window open).
- [ ] Zone console around add/remove class and any packet sends.
- [ ] `#multiclassdiag` output before and after the action.

---

## Equipment & Item Use Tests

### G) Item Equipping (multiclass)
- [ ] Equip a Cleric-only item on a Ranger/Warrior multiclass → should fail immediately in UI (red text/icon).
- [ ] Equip a Magician-only item on a Warrior/Mage multiclass → should succeed and show valid in UI.
- [ ] Item Tooltip: Verify that a Cleric-only item shows "Classes: CLR" but indicates "(Can Equip)" status correctly based on multiclass bits.

### H) Combat Effectiveness (Best-of-Class)
- [ ] Equip Warrior/Wizard; confirm AC Softcap is high (Warrior tier) vs low (Wizard tier).
- [ ] Equip Monk/Warrior; confirm Flying Kick triggers during auto-attack if leveled.
- [ ] Equip Rogue/Mage; confirm "Hide" result uses Rogue level evasion logic.
- [ ] Check Shaman/Rogue; confirm Alchemy and Poison combines are successful at low level.
- [ ] Check Bard/Warrior; confirm songs scale with player level/instruments correctly.

---

## Spell Damage & Heal Bonus Tests

### I) Spell Damage Bonus (multiclass)
- [ ] Cast a Ranger nuke on Ranger/Mage → item SpellDmg should apply if spell level is within 5 of caster level.
- [ ] Cast a Mage nuke on Ranger/Mage → item SpellDmg should apply (tests multiclass best-level lookup).
- [ ] Cast a Wizard nuke (if not owned) on Ranger/Mage → SpellDmg should NOT apply (class not owned).

### J) Heal Bonus (multiclass)
- [ ] Cast a Druid heal on Ranger/Druid → item HealAmt should apply.
- [ ] Cast a Cleric heal (if not owned) on Ranger/Druid → HealAmt should NOT apply.

### K) Clairvoyance Mana Return (multiclass)
- [ ] Cast a spell with Clairvoyance item bonus on multiclass → should return mana based on best spell level.

---

## Discipline & Combat Tome Tests

### L) Discipline Use (multiclass)
- [ ] Use a Warrior discipline on Ranger/Warrior → should activate.
- [ ] Use a Monk discipline (if not owned) on Ranger/Warrior → should fail with class error.

---

## Spell Focus Effect Tests

### M) Focus Effect Level Limits (multiclass)
- [ ] Equip a focus item with `LimitMaxLevel` → should use best (lowest) spell level across owned classes.
- [ ] Equip a focus item with `LimitMinLevel` → should pass if ANY owned class meets the minimum.

---

## Fixed Functions (Server-Side)

The following functions have been updated to support multiclass (GetClassesBits):

### zone/spells.cpp
- [x] `CheckItemRaceClassDietyRestrictionsOnCast()` - Item click validation now uses GetClassesBits()
- [x] `CheckFizzle()` - Already had multiclass support via GetBestSpellLevelForMulticlass()

### zone/effects.cpp
- [x] `GetActSpellDamage()` - SpellDmg bonus now uses MeetsSpellLevelForBonusDamage() helper
- [x] `GetActDoTDamage()` - DOT damage bonus now uses MeetsSpellLevelForBonusDamage() helper
- [x] `GetActSpellHealing()` - HealAmt bonus now uses MeetsSpellLevelForBonusDamage() helper
- [x] `GetActSpellCost()` - Clairvoyance mana return now uses MeetsSpellLevelForBonusDamage() helper
- [x] `UseDiscipline()` - Discipline class check now uses GetClassesBits() loop

### zone/spell_effects.cpp
- [x] `CalcFocusEffect()` - LimitMaxLevel now uses GetBestSpellLevelForFocus() helper
- [x] `CalcFocusEffect()` - LimitMinLevel now uses GetBestSpellLevelForFocus() helper
- [x] `GetFocusEffect()` - LimitMaxLevel now uses GetBestSpellLevelForFocus() helper
- [x] `GetFocusEffect()` - LimitMinLevel now uses GetBestSpellLevelForFocus() helper

### zone/mob.cpp
- [x] `GetDecayEffectValue()` - Spell level for decay now uses GetBestSpellLevelForMob() helper

### zone/inventory.cpp
- [x] `SwapItem()` calls - Already updated to pass GetClassesBits() for item validation

### zone/bonuses.cpp
- [x] Item equip class checks - Already uses GetClassesBits() at lines 288 and 549

### zone/aa.cpp
- [x] AA validation - Already has multiclass support via MulticlassingEnabled rule check

---

## DLL Files Modified

| File | Change |
|------|--------|
| `extras/eq-core-dll-main/src/eqgame.cpp` | GetUsableClasses_Detour whitelist, GetSpellLevelNeeded_Detour filter logic |
