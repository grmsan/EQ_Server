# Agility (AGI) Implementation Plan

> Current architecture: keep legacy AGI behavior unless `Combat:UseNewAgiFormulas` is enabled. Tuning should be hotfixable through `zone/combat_balance.ini`; `zone/combat_balance_config.h` should hold fallback defaults only.

## Vision (aligns with AGI.md + OVERVIEW.md)
- AGI is the Stat of Velocity: faster actions (swing/cast/move) and harder to hit.
- Curve shape: asymptotic/diminishing returns (big early wins, tapering at the top).
- Applies to both melee and casters (swing speed, avoidance, run speed, cast/GCD speed).
- Class weighting for avoidance so light fighters feel it most; casters get a light benefit.

## Target Formula Shapes (runtime-tunable knobs)
Use CombatBalance runtime accessors with fallback constants and allow Rule toggle `Combat:UseNewAgiFormulas`.

- **Haste from AGI (swing speed)**  
  `haste_pct = HASTE_CAP * AGI / (AGI + HASTE_DIV)` (default cap 100%, div ~400).  
  Fold into the normal haste bucket and clamp at the standard haste cap (100%). Tune DIV to taste.

- **Avoidance from AGI (class weighted)**  
  `avoid_base = AVOID_CAP * AGI / (AGI + AVOID_DIV)` (cap ~75%).  
  `avoid_final = avoid_base * class_mult`. Class multipliers:  
  - Monks/Beastlords/Rogues/Bards: 1.5x (light fighters)  
  - Tanks/Rangers: 1.0x  
  - Casters/Healers: 0.75x (still benefit meaningfully)  
  Soft cap around ~85% for the light fighters.

- **Run speed from AGI**  
  `runspeed_pct = RUN_CAP * AGI / (AGI + RUN_DIV)` (cap ~110%, div ~200).  
  Take the max of AGI-derived speed vs spell/mount; do not stack additively beyond the cap.

- **Cast/GCD speed from AGI (asymptotic to a floor)**  
  Use an asymptotic reduction that approaches a floor (default 0.5s) but never crosses it. Example:  
  `cast_multiplier = floor + (1 - floor) * (CAST_DIV / (AGI + CAST_DIV))`  
  So cast_time = base_cast * cast_multiplier (and GCD similarly), never below the floor; high AGI can push long casts (9s) close to ~1s without going under the floor.

## Runtime/Fallback Knobs
- HASTE: `AGI_HASTE_DIVISOR`, `AGI_HASTE_CAP`
- AVOID: `AGI_AVOID_DIVISOR`, `AGI_AVOID_CAP`, class multipliers, `AGI_AVOID_SOFTCAP`
- RUN: `AGI_RUN_DIVISOR`, `AGI_RUN_CAP`
- CAST: `AGI_CAST_DIVISOR`, `AGI_CAST_FLOOR` (e.g., 0.5s), optional multiplier form rather than a hard % cap
- Rule toggle: `Combat:UseNewAgiFormulas`

## Implementation Steps
1) **Rules/config**  
   - Add/confirm RuleB `Combat:UseNewAgiFormulas` with legacy-safe default unless intentionally enabled for a test shard.
   - Add AGI fallback constants in `zone/combat_balance_config.h` and runtime keys in `zone/combat_balance.ini`.

2) **Swing speed / haste**  
   - In attack speed calc (e.g., `Mob::GetHaste()` / swing delay path in `attack.cpp`), add AGI haste term when rule is on; clamp to cap and combine with item/spell haste respecting overall haste caps.

3) **Avoidance**  
   - In avoidance checks (`AvoidDamage`, `CheckHitChance`, or central avoidance calc in `attack.cpp`/`mob.cpp`), add AGI avoidance term using class multiplier and soft cap. Apply before strikethrough; ensure it’s additive to existing avoidance bonuses but clamped.

4) **Run speed**  
   - In run speed computation (`Mob::GetRunspeed` or client movement mods), add AGI-based runspeed; pick the max of AGI vs spell/mount speed; obey existing caps to prevent runaway stacking.

5) **Cast/GCD speed**  
   - In casting time logic (`Mob::GetActSpellCasttime` or equivalent in `spells.cpp`), reduce cast time by AGI reduction pct (clamp min > 0).  
   - In global cooldown logic (client/server GCD application), apply the same reduction capped by CAST_CAP.

6) **Data/UI surfaces**  
   - Ensure AGI-derived haste/avoid/cast/run show up in client stats windows if applicable; at minimum, log/debug strings for tuning.

7) **Testing matrix**  
   - Levels: 1, 20, 50, 100; AGI: 50, 100, 200, 500, 1000, 2000.  
   - Archetypes: light fighter (monk), tank (war), caster (wiz).  
   - Verify: swing delay changes, avoidance % changes, run speed vs SoW, cast time reductions.

8) **Safety/Clamps**  
   - Cap haste at existing hard cap; cap avoidance with soft cap; run speed at RUN_CAP; cast/GCD never below 0.5s (configurable).

## Open Questions / Clarifications (to resolve during tuning)
- Exact class multipliers for avoidance (current defaults good?).
- Should AGI haste be part of the existing haste cap or a separate “skill” channel? (recommend: folded into total haste, but capped).
- Should AGI run speed override or stack with bard songs? (recommend: take max).
- Minimum cast time/GCD floor? (recommend 0.5s floor).

## Deliverables
- Code changes with rule toggle and constants.
- Updated docs (AGI.md, README links) reflecting final constants.
- Test log at key AGI/level breakpoints.
