# Character Budget Simulation Model

## Core Purpose

The loot-first approach is the wrong tool for the current balance problem.

What you actually need is:
- a target **character sheet budget**
- a target **ability baseline** (weapon damage, bow damage, spell base value)
- a target **enemy profile** (trash, named, raid)

That lets you ask the right question:
- "If a Kunark-capped ranger focuses DEX and reaches 400 DEX / 200 other stats, what does that do to average bow damage?"
- "If a wizard hits 400 INT / 200 DEX / 200 WIS, what does that do to average nuke damage and rotation DPS?"

This is much better than trying to infer character output from raw loot-table averages.

## Player Experience

This approach models the game the way players actually feel it:
- Players build toward a target sheet, not an item spreadsheet.
- Players judge power by combat output, not by abstract item medians.
- Focus builds matter. A ranger does not care about "average all-stat item value"; they care about what a DEX-heavy sheet does to bow output.

For designers, this is useful because it turns stat tuning into readable questions:
- Is 400 DEX too much for Kunark if it makes ranger shots average 2.5k on named targets?
- Is 400 INT too little if a wizard only lands 2k average casts?
- Is STR contributing enough to a warrior hit, or is weapon base still doing all the work?

## Incentives And Exploits

This model exposes the real balance levers:
- Raw stat budgets drive formula outputs directly.
- Weapon/spell baselines are separated from stat scaling, so you can see whether a class is overpowered because of the stat formula or because the ability baseline is too large.
- Enemy mitigation is explicit, so you stop guessing whether "damage is too high" because the player is overtuned or because targets are undertuned.

It also prevents a bad designer habit:
- using a single averaged "gear quality" number as a proxy for actual class performance

## Friction Vs Depth

This adds good depth, not bad complexity.

Good complexity:
- separate build budget from encounter profile
- separate raw sheet stats from output formulas
- compare classes using the same target assumptions

Bad complexity avoided:
- no pretending that all expansions can be summarized by one averaged stat number
- no pretending that one item metric predicts melee, archery, and spell throughput equally well

## Better Version Of The Problem

The right balance workflow is:

1. Pick a **budget preset**
2. Pick a **class focus**
3. Pick an **ability baseline**
4. Pick a **target profile**
5. Simulate output
6. Decide whether stats, abilities, or enemies need tuning

That means the balancing unit becomes:
- `character build -> expected combat result`

not:
- `expansion loot average -> hand-wavy guess`

## Recommended Budget Presets

These are design targets, not DB-derived truths.

| Preset | Level | Focused Primary | Other Stats |
|---|---:|---:|---:|
| `classic_30_focus` | 30 | 200 | 100 |
| `classic_50_focus` | 50 | 300 | 150 |
| `kunark_60_focus` | 60 | 400 | 200 |
| `velious_60_focus` | 60 | 475 | 225 |
| `luclin_65_focus` | 65 | 600 | 275 |
| `pop_65_focus` | 65 | 700 | 325 |

These are intentionally opinionated. They are supposed to be useful tuning anchors.

## Simulation Tool

Companion tool:
- [character_budget_sim.py](/C:/Users/marsh/OneDrive/Documents/GitHub/EQ_Server/tools/character_budget_sim.py)

It models:
- **Warrior** focused STR melee
- **Ranger** focused DEX archery
- **Wizard** focused INT spell damage

It also makes target assumptions explicit:
- `trash`
- `named`
- `raid`

## Current Formula Assumptions In The Tool

The simulator uses the current design-doc formula shapes, not a perfect replica of all code paths.

### Warrior
- STR bonus: `STR * max(Level / 60, 0.05)`
- Base hit: `WeaponDamage + DelayBonus + STRBonus`
- Crit chance from DEX
- Crit multiplier from DEX base crit bonus and overflow

### Ranger
- Bow base: `WeaponDamage + (DEX * Level / 10)`
- Crit chance from DEX
- Crit multiplier from DEX base crit bonus plus ranger bow crit scaling

### Wizard
- Spell multiplier from INT class scaling
- Spell crit chance from DEX
- Wizard crit damage bonus from INT
- Twincast chance from DEX
- Rotation time reduced by INT cooldown reduction

## Sample Outputs

Using `kunark_60_focus` and `named` target profile:

### Ranger
Command:

```powershell
rtk python tools/character_budget_sim.py --class ranger --preset kunark_60_focus --target named
```

Output summary:
- DEX = `400`
- Other stats = `200`
- Average landed shot: `4751`
- Average applied shot after target assumptions: `2649`

Design read:
- If you wanted "best Kunark ranger averages about 1k bow shots on named mobs," your current DEX bow formula is far too hot under these assumptions.

### Warrior
Command:

```powershell
rtk python tools/character_budget_sim.py --class warrior --preset kunark_60_focus --target named --two-handed
```

Output summary:
- STR = `400`
- Other stats = `200`
- Average landed hit: `546`
- Average applied hit: `304`

Design read:
- Warrior output is much more conservative than ranger archery under the same budget philosophy.
- That suggests either ranger DEX bow scaling is too aggressive, warrior STR scaling is too weak, or both.

### Wizard
Command:

```powershell
rtk python tools/character_budget_sim.py --class wizard --preset kunark_60_focus --target named
```

Output summary:
- INT = `400`
- DEX/WIS = `200`
- Spell multiplier: `2.2x`
- Average landed cast: `6977`
- Average applied cast: `5035`
- Rotation DPS: `847`

Design read:
- If your target was "good Kunark wizard casts for about 5k," this is actually close.
- If 5k is too high, the main knobs are INT spell multiplier, wizard crit bonus, or baseline spell values.

## What Is Good

The strongest part of this model is that it separates the major causes of power:
- stat sheet
- ability baseline
- enemy mitigation profile

That makes tuning discussions much cleaner.

Instead of:
- "Kunark gear seems high"

you get:
- "At 400 DEX, rangers overshoot our target by 2.5x on named mobs."

That is actionable.

## What Is Weak

This tool is still only a first-pass approximation.

Known limitations:
- It does not mirror every combat code path exactly.
- It does not yet simulate proc chains, AA stacks, or special attack cadence.
- It treats target mitigation as a profile input rather than reproducing full server-side combat resolution.
- It does not yet model class-specific weapon packages per expansion automatically.

That is acceptable for now. This should be a tuning tool, not a fake promise of exact runtime parity.

## Better Versions

### Safe Version

Keep this as a lightweight design-side simulator.

Use it to:
- set target build budgets
- compare class outputs at the same budget
- decide what needs deeper in-game testing

### Ambitious Version

Extend the simulator to pull:
- expansion/class weapon baselines
- representative spell lines by level
- target AC/HP/resist profiles

Then generate:
- per-class expansion tuning sheets
- projected DPS / burst / sustain tables

### Weird Version

Hook into the server tuning pipeline directly and build a "sheet stat sandbox" GM command that temporarily overrides a test client's stats and runs `#tune` style projections against a dummy target.

That would be closer to real combat, but much more engineering-heavy.

## Decision

**Keep but simplify**

Keep the new budget-first direction.

Do not keep using loot-average stat estimates as your main balance anchor.

Practical recommendation:
- Adopt budget presets like `kunark_60_focus = 400 primary / 200 others`
- Use the simulator to check whether those budgets create believable output
- Tune formulas until the outputs match your class fantasies
- After that, go back and make sure itemization can actually support those sheet budgets
