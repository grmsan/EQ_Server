# DEX Attribute Implementation Plan

> Design source: `game_design/stats/DEX.md`, system context: `game_design/stats/OVERVIEW.md`.
> Goal: Make Dexterity the Stat of Precision — crit reliability + crit power, multi-proc chains, bow force (rangers), spell penetration — with clear tuning knobs in code/config.

## Executive Summary
DEX drives four pillars:
1) **Crit Mastery & Overflow** (melee + spell): chance scales with DEX×Level; overflow above 100% converts to crit damage.
2) **Base Crit Damage**: DEX directly adds to crit damage even before overflow.
3) **Proc Mastery**: multi-proc chains on weapon procs based on DEX.
4) **Ranger Bow Force & Spell Penetration**: rangers use DEX for ranged damage + crit damage; casters get resist penetration and twincast from DEX.

## Core Formulas (from DEX.md)
- Crit Chance (both melee & spells): `CritChance% = (DEX * Level) / DEX_CRIT_DIVISOR` (design doc uses 500).
- Crit Overflow: if total crit chance > 100%, overflow adds to crit damage `%`.
- Base Crit Damage: `BaseCritDmg% = DEX / 20` (design doc).
- Twincast: `Twincast% = (DEX * Level) / 2000`.
- Multi-Proc chains (per swing, sequential):
  - 1st: `DEX / (DEX + 100)`
  - 2nd: `DEX / (DEX + 200)`
  - 3rd: `DEX / (DEX + 400)`
  - 4th: `DEX / (DEX + 1500)`
- Ranger Bow Damage: for rangers, ranged base uses DEX instead of STR: `BaseDamage = WeaponDmg + (DEX * Level / 10)`.
- Ranger Bow Crit Damage: `CritDmgMod += DEX / 500`.
- Spell Resist Penetration: `Resist_Ignore = DEX / 10`.

## Tuning Knobs (centralize in `zone/combat_balance_config.h`)
- `DEX_CRIT_DIVISOR` (default 500): raise to lower crit chance, lower to raise.
- `DEX_BASE_CRIT_DMG_DIVISOR` (default 20): base crit damage per DEX.
- `DEX_TWINCAST_DIVISOR` (default 2000).
- Multi-proc denominators: 100 / 200 / 400 / 1500 (expose as constants).
- `DEX_BOW_LEVEL_DIVISOR` (default 10) for ranger ranged bonus.
- `DEX_BOW_CRIT_DMG_DIVISOR` (default 500).
- `DEX_RESIST_PENETRATION_DIVISOR` (default 10).
- Caps/Overflow: allow overflow → crit damage; optional clamp toggles if needed.
- Class toggles: enable/disable DEX-based bow force for rangers only; apply crit/twincast to all casters or DD-only.
- Slot/curve tuning: keep DEX item scaling via `item_scaling.json` curves & slot multipliers to control how much DEX appears on gear.
- **Early-game crit floor**: Add a minimum crit chance floor driven by DEX for low levels (e.g., `DEX_CRIT_MIN = (DEX / DEX_CRIT_MIN_DIVISOR)`, default divisor ~3000–4000). For tuning: at level 20 with 200 DEX, aim for ~5% total crit; below 100 DEX at level 1–10 should be very low but non-zero. Combine floor + main formula, clamp to 100%.
- **Crit step feel**: Consider an optional “step” contribution (e.g., +1% per X DEX chunk) if the floor formula alone doesn’t feel granular; keep this as a knob.
- **Pet scaling**: Apply DEX crit/twincast to pets at a reduced scalar: `PET_DEX_CRIT_SCALAR` (e.g., 0.5) and `PET_DEX_TWINCAST_SCALAR` (e.g., 0.5). Consider a general “pet stat carryover” scalar for future consistency.
- **Suggested first guesses (tune after playtests):**
  - `DEX_CRIT_DIVISOR = 500` (matches DEX.md examples).
  - `DEX_CRIT_MIN_DIVISOR = 3500` (gives ~5% crit at level 20, 200 DEX; very low at sub-100 DEX early).
  - Overflow-to-crit-damage scalar: start at 1.0 (1:1); lower to 0.5 if top-end is too explosive.
  - Multi-proc max bands: +1 allowed proc per 500 DEX (0–499=1, 500–999=2, 1000–1499=3, etc.).
  - Multi-hit bonuses (BS/Frenzy/Flurry): add `DEX / 5000` as a small bonus chance per skill (cap with a max); start conservative.
  - `PET_DEX_CRIT_SCALAR = 0.5`, `PET_DEX_TWINCAST_SCALAR = 0.5`.
  - `DEX_BOW_LEVEL_DIVISOR = 10`, `DEX_BOW_CRIT_DMG_DIVISOR = 500` (from DEX.md).
  - `DEX_RESIST_PENETRATION_DIVISOR = 10` but treat as “minor”; increase divisor if stacking with CHA.
  - DOT crits: same DEX crit/overflow; DOT twincast toggle off by default until tested.

## Implementation Steps (code)
1) **Crit Chance & Overflow (melee + spells)**
   - File: `zone/attack.cpp` (melee crit calc) and `zone/spells.cpp` / `zone/spell_effects.cpp` (spell crits).
   - Add helper `GetDexCritChance()` using DEX×Level/`DEX_CRIT_DIVISOR` plus an optional floor `DEX_CRIT_MIN` term so low levels with high DEX still crit. Clamp at 100%; overflow → crit damage.
   - When total crit chance > 100%, set chance=100 and add overflow to crit damage mod.
   - Add `DEX_BASE_CRIT_DMG_DIVISOR` to crit damage mod even before overflow.

2) **Twincast from DEX**
   - File: `zone/spell_effects.cpp` (spell cast resolution).
   - Add a twincast roll using `Twincast% = DEX*Level/DEX_TWINCAST_DIVISOR`; stack with existing twincast sources.

3) **Multi-Proc Mastery**
   - File: `zone/attack.cpp` or wherever procs are resolved (`TryTriggerProc`).
   - After a proc fires, roll sequential chances using the denominators above. Stop chain on first failure.
   - Guard with a rule/constant to avoid excessive loops; log for testing. Add a hard max procs tiered by DEX bands (e.g., 0–499 = 1 proc max, 500–999 = 2, 1000–1499 = 3, etc.) with tunable band size.

4) **Ranger Bow Force**
   - File: ranged attack calculation (`zone/attack.cpp` ranged path).
   - For class ranger, replace STR contribution with `DEX * Level / DEX_BOW_LEVEL_DIVISOR` when item is a bow/ranged attack.
   - Add DEX-based bow crit damage mod (`DEX / DEX_BOW_CRIT_DMG_DIVISOR`).

5) **Spell Resist Penetration**
   - File: resist check (`zone/spell_effects.cpp` / `zone/spells.cpp` resist calculations).
   - Subtract `DEX / DEX_RESIST_PENETRATION_DIVISOR` from target resists (or add to caster penetration) for DD spells.

6) **DOT Crits**
   - Extend spell crit logic to allow DOT ticks to crit using the same DEX crit chance/overflow rules (with a separate rule toggle if desired).
   - Optional: DOT “twincast” effect (duplicate tick) using the same twincast roll; guard with a toggle.

7) **Multi-Hit Skills (Rogue BS, Berserker Frenzy, Monk Flurry)**
   - Add a DEX-based bonus chance on multi-hit triggers, layered on top of skill-based chance.
   - Example: `extra_proc_chance += DEX / DEX_MULTI_HIT_DIVISOR` (tune per skill; e.g., BS/Frenzy/Flurry divisors could differ). Keep it small but noticeable now; allow future expansion (e.g., very high DEX → more hits).
   - Guard with class/skill checks and a max cap to avoid runaway multi-hits.

8) **Pet Scaling**
   - Apply DEX crit/twincast to pets using `PET_DEX_CRIT_SCALAR` and `PET_DEX_TWINCAST_SCALAR`.
   - For pet DOT crits, reuse the same scalar.
   - Keep a general “pet stat carryover” scalar to align with broader stat inheritance plans.

9) **Scaling Data Alignment**
   - Ensure `item_scaling.json` keeps DEX growth in line with desired power; adjust `AttributeCurve` and slot multipliers if DEX is over/under represented on gear.
   - Add a JSON curve for any DEX-specific effects if needed (e.g., if bow-specific bonuses are data-driven).

10) **Rule Toggles (optional)**
   - Add rules to enable/disable DEX crit overflow, DEX bow force, DEX twincast, DEX DOT crits + DOT twincast, DEX multi-hit bonuses, and DEX resist penetration for easier balancing. Consider a single “UseNewDexFormulas” rule to gate the whole package, with sub-toggles for specific features.

## Files to Modify
- `zone/combat_balance_config.h`: add DEX constants/toggles.
- `zone/attack.cpp`: melee crit chance/overflow, multi-proc, ranger bow force, DEX crit damage mod.
- `zone/spell_effects.cpp` / `zone/spells.cpp`: spell crit chance/overflow (including DOT ticks), twincast, resist penetration.
- `zone/attack.cpp` proc handling: multi-proc chain logic.
- `zone/special_attacks.cpp` / class-specific code: DEX-based bonus for multi-hit skills (backstab, frenzy, flurry).
- Pet handling (e.g., `zone/mob.cpp`, pet crit/twincast paths): apply pet scalars.
- `item_scaling.json` (if gear DEX amounts need tuning).

## Validation & Examples
- **Crit example (Level 70, 1000 DEX):** CritChance = (1000*70)/500 = 140% → 100% crit, +40% overflow to crit damage. Base crit dmg from DEX = 1000/20 = 50%. Total DEX-driven crit dmg = +90% before other mods.
- **Early-level crit example (Level 20, 200 DEX):** Main formula gives ~8% (200*20/500=8%). Floor term (e.g., DEX/800=25%) yields ~25%; clamp to sum with other sources and cap at 100%. Tuning the floor/divisor sets how “alive” crits feel early.
- **Twincast example (Level 70, 1000 DEX):** (1000*70)/2000 = 35% twincast.
- **Multi-proc example (1000 DEX):** 1st 91%, 2nd 83%, 3rd 71%, 4th 40% (approx, per table).
- **Bow example (Level 70 ranger, 1000 DEX, 20 dmg bow):** Bow bonus = 1000*70/10 = 7000 added to base; plus crit dmg bonus 1000/500 = +200% on crits.
- **Resist penetration example (1000 DEX):** ignore 100 resist on DD spells.
- **DOT crit example:** DOT ticks use same crit chance/overflow; apply DEX crit chance each tick; overflow adds to crit damage multiplier for DOT crits as well (toggleable).
- **Multi-hit example (Rogue BS):** Add a small DEX-based bonus to double/triple backstab chance (e.g., +DEX/5000) layered on skill chance; similarly for Berserker Frenzy and Monk flurry/rapid strikes.
- **Pet example:** Pet crit/twincast chance = owner-based DEX crit/twincast * pet scalar (e.g., 0.5) so pets benefit but at reduced rate.

## Testing Plan
- Unit/functional tests:
  - Crit chance/overflow math at multiple DEX/levels (melee & spells).
  - Twincast rate vs expected percentages.
  - Multi-proc chain frequency vs table at 100/500/1000 DEX.
  - Bow damage for rangers vs STR build for others; verify only ranged uses DEX replacement.
  - Resist penetration effect on high-resist targets; log hit/miss rates.
- In-game scenarios:
  - Level 1/10/50/70 characters with low/medium/high DEX; verify DPS and crit frequency.
  - Ranger ranged DPS with DEX stacking; compare to STR-stacked melee.
  - Casters with high DEX: crit/twincast/resist penetration feel.

## Rollout Notes
- Keep defaults conservative (match DEX.md numbers) and adjust `DEX_CRIT_DIVISOR`, `DEX_BASE_CRIT_DMG_DIVISOR`, and proc denominators after playtests.
- Document knob changes in the JSON/changelogs so designers can iterate without code changes where appropriate (gear/stat curves), and in `combat_balance_config.h` for code-level knobs.
