# Intelligence (INT) Implementation Plan

> Design sources: `game_design/stats/INT.md`, `game_design/stats/INT_WIS.md`, `game_design/stats/OVERVIEW.md`, `game_design/stats/PET_SCALING.md`.
> Intent: INT is the Stat of Brilliance — offensive spell power, cooldown speed, and mana efficiency. Keep scaling strong but bounded so DEX/CHA synergies do not blow up encounter balance.
> Current architecture: keep legacy INT behavior unless `Combat:UseNewIntSystems` or the relevant spell-bonus feature gate is enabled. Tuning should be hotfixable through `zone/combat_balance.ini`; `zone/combat_balance_config.h` should hold fallback defaults only.

## Vision
- Make INT the primary offensive scaler for casters and a secondary utility stat for everyone (CDR + efficiency).
- Remove old diminishing returns on mana and damage, replace with linear or softcapped curves that feel great early and controllable at extreme values.
- Gate the new INT systems with `Combat:UseNewIntSystems` and expose tuning through runtime combat-balance config.

## Core Mechanics & Formulas (with guardrails)
- **Spell Damage Mod**  
  `spell_mod = (INT / INT_SPELL_DIVISOR) * class_mult`, then softcap: `spell_mod = ApplySoftcap(spell_mod, INT_SPELL_SOFTCAP, INT_SPELL_SOFTCAP_POWER)`.  
  *Defaults:* divisor 5 (doc), softcap 3.0x, power 0.75. Class mults: Wizard 1.5, Mage/Nec/Enc 1.0, Healers/Hybrids 0.8, Melee 0.5 (for disc/AA spells only).

- **Wizard Crit Damage Bonus ("Arcane Overload")**  
  `wiz_crit_bonus = INT / WIZ_INT_CRIT_DIVISOR`, softcapped.  
  *Defaults:* divisor 10 (doc gives +100% at 1000), cap 1.25x to prevent runaway when combined with DEX overflow; optional rule to uncap for power fantasy.

- **Cooldown Reduction (Spells + Discs)**  
  `cdr = (CDR_CAP * INT) / (INT + INT_CDR_DIVISOR)` (asymptotic).  
  *Defaults:* cap 50%, divisor 500 (doc), clamp total CDR with other sources at 65% to prevent infinite chains.

- **Mana Cost Reduction**  
  `mana_discount = min((INT * Level) / INT_MANA_COST_DIVISOR, INT_MANA_COST_CAP)`  
  *Defaults:* divisor 2000 (doc gives 35% at 70/1000), cap 50%.

- **Mana Pool ("Power Mana")**  
  Replace diminishing returns with linear: `MaxMana = (INT + WIS) * Level * class_mult * INT_WIS_MANA_SCALAR`.  
  *Defaults:* scalar 1.0, class mult from existing tables; keep a global clamp to avoid integer overflow and to keep low-level HP/MP parity.

- **Lifetap Efficiency (Necro/SK)**  
  `lifetap_heal_mult = 1 + (INT / INT_LIFETAP_DIVISOR) * INT_LIFETAP_CLASS_MULT`; start from doc’s +50% at 1000 INT.  
  *Defaults:* divisor 2000, class mult 1.0 for Necro/SK (tunable).

- **Necro DoT Power**  
  Separate DoT scalar: `dot_mod = spell_mod * INT_DOT_MULT`; default 1.1 so DoTs get a slight bump without eclipsing nukes.

- **Magician Pet INT Share**  
  INT pet offense should use the shared pet-scaling model in `PET_SCALING.md`. `PET_OWNER_INT_TRANSFER` controls the owner INT contribution, while CHA/class multipliers, pet gear weight, and final caps shape the final pet output.

## Runtime/Fallback Knobs
- Toggle: `Combat:UseNewIntSystems` with legacy-safe default unless intentionally enabled for a test shard.
- Spell power: `INT_SPELL_DIVISOR = 5.0f`, `INT_SPELL_SOFTCAP = 3.0f`, `INT_SPELL_SOFTCAP_POWER = 0.75f`, class multipliers table.
- Wizard crit: `WIZ_INT_CRIT_DIVISOR = 10.0f`, `WIZ_INT_CRIT_CAP = 1.25f`, `RULE_BOOL(Combat, WizardIntCritUncapped)` default false.
- CDR: `INT_CDR_DIVISOR = 500.0f`, `INT_CDR_CAP = 0.50f`, `INT_CDR_TOTAL_CAP = 0.65f`.
- Mana efficiency: `INT_MANA_COST_DIVISOR = 2000.0f`, `INT_MANA_COST_CAP = 0.50f`.
- Mana pool: `INT_WIS_MANA_SCALAR = 1.0f`, optional `INT_WIS_MANA_GLOBAL_CAP` to avoid overflow.
- Lifetap: `INT_LIFETAP_DIVISOR = 2000.0f`, `INT_LIFETAP_CLASS_MULT = 1.0f`.
- DoTs: `INT_DOT_MULT = 1.1f`.
- Pet share: use shared pet-scaling keys from `PET_SCALING.md`, especially `PET_OWNER_INT_TRANSFER`, `PET_OWNER_TRANSFER_GLOBAL_SCALAR`, class scalars, pet gear weight, and final pet output caps.

## Implementation Steps
1) **Config/Rules**  
   - Add fallback constants, runtime keys, and RuleB toggle. Wire class multiplier table (wizard, pure casters, priests/hybrids, melee).

2) **Spell Damage Pipeline**  
   - In spell damage calc (`Mob::GetActSpellDamage`, `Mob::CalcSpellDamage`), apply `spell_mod` multiplier before crits/twincast.  
   - Necro DoTs: apply `INT_DOT_MULT` on top of `spell_mod` for DoT effect types.

3) **Wizard Crit Damage**  
   - In spell crit resolution, add `wiz_crit_bonus` to crit damage mod for wizards only; apply softcap. Respect rule to uncap if enabled.

4) **Cooldown Reduction**  
   - Integrate `cdr` into reuse timers for spells (`GetActSpellCasttime`/reuse) and disciplines. Clamp combined CDR with item/song effects to `INT_CDR_TOTAL_CAP`.

5) **Mana Cost Reduction**  
   - In mana cost calculation (`Mob::CalcSpellManaCost`), subtract `mana_discount` percent. Apply after any class/focus adjustments to avoid double-dipping.

6) **Mana Pool Redesign**  
   - Replace diminishing returns in `Client::CalcBaseMana` with the linear INT+WIS formula and class mult, gated by `USE_NEW_INT_SYSTEMS`. Maintain legacy path when off.

7) **Class Hooks**  
   - Lifetap healing: in lifetap heal portion, multiply by `lifetap_heal_mult` for Necro/SK.  
   - Magician pet INT share: during pet stat build, route owner INT through the shared pet-scaling helper instead of adding a separate `pet_int_share` stat path.
   - Hybrid/Healer spells: ensure class multiplier table applies to their spell damage where applicable (offensive spells only).

8) **Telemetry/Debug**  
   - Add optional debug output for `spell_mod`, `cdr`, `mana_discount`, `wiz_crit_bonus`, and mana pool values for tuning sessions.

## Safety & Fun Guardrails
- Softcap on spell power keeps high-end INT from eclipsing STR/DEX builds while still honoring doc numbers at ~1000 INT.  
- Wizard crit bonus capped by default; toggle available for “go nuts” servers.  
- Total CDR clamp prevents cooldown-free rotations when combined with AA/gear.  
- Mana discount cap at 50% keeps efficiency meaningful without trivializing resource management.  
- Linear mana pool uses class mult + global cap to avoid overflow and maintain HP/MP balance at very high stats.  
- Pet INT transfer must share the same final pet output caps as CHA pet power and pet gear so stacked channels cannot create absurd totals.

## Testing Matrix
- **Spell DPS:** Level 20/50/70 caster at 200/500/1000 INT; verify spell damage and crit mods match formulas and softcaps.  
- **CDR:** Measure reuse on a 30s spell and 30m disc at INT 100/500/1000 to confirm asymptotic curve and total cap with other CDR sources.  
- **Mana Efficiency:** Parse mana spend over 2-minute rotation with and without INT discount; confirm 35% at 70/1000 aligns.  
- **Mana Pool:** Compare legacy vs new formula at INT/WIS 200/400/800; ensure no truncation/overflow.  
- **Lifetap/DoT:** Validate Necro/SK lifetap heals and DoT ticks scale with INT multipliers; ensure DoT multiplier does not affect nukes.  
- **Pet Share:** Magician pet DPS at 0 vs 800 INT with CHA constant; confirm INT transfer goes through the shared pet model, pet gear is independently tunable, and final caps apply.

## Deliverables
- Runtime keys + fallback constants + RuleB entry; updated spell damage/cost/CDR/mana pool code paths; class-specific hooks for wizard crit bonus and lifetap efficiency; shared pet-scaling hook for Magician pet INT transfer; debug helpers for tuning.
