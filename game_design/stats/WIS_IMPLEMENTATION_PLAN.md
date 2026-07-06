# Wisdom (WIS) Implementation Plan

> Design sources: `game_design/stats/WIS.md`, `game_design/stats/INT_WIS.md`, `game_design/stats/OVERVIEW.md`.
> Intent: Wisdom is the Stat of Warding — spell mitigation, healing/ward amplification, and crowd-control resistance, plus shared mana pool scaling. Keep defenses potent but capped so encounters stay threatening.
> Current architecture: keep legacy WIS behavior unless `Combat:UseNewWisSystems` or the relevant shared INT/WIS feature gate is enabled. Tuning should be hotfixable through `zone/combat_balance.ini`; `zone/combat_balance_config.h` should hold fallback defaults only.

## Vision
- Make WIS the primary defensive scaler for magic-heavy content and the main heal/ward amplifier for priests and hybrids.
- Share mana pool scaling with INT using the linear, no-DR model from `INT_WIS.md`.
- Cap and soften the curves so stacking WIS with STA/CHA does not trivialize boss damage or crowd control.

## Core Mechanics & Formulas (with guardrails)
- **Spell Mitigation ("Divine Barrier")**  
  `spell_reduce = min((WIS * WIS_SPELL_CAP) / (WIS + WIS_SPELL_DIVISOR), WIS_SPELL_CAP)`; apply multiplicatively with other reductions (STA/CHA).  
  *Defaults:* cap 0.50 (50%), divisor 500 (doc table). Optional softcap to taper past 1000 WIS.

- **Heal Power ("Potency of Spirit")**  
  `heal_mod = ApplySoftcap(WIS / WIS_HEAL_DIVISOR, WIS_HEAL_SOFTCAP, WIS_HEAL_SOFTCAP_POWER)` applied to all heals (spells, procs, potions).  
  *Defaults:* divisor 10 (doc), softcap 2.0x, power 0.8 to avoid runaway at extreme WIS.

- **Rune/Shield Power**  
  `rune_mod = ApplySoftcap(WIS / WIS_RUNE_DIVISOR, WIS_RUNE_SOFTCAP, WIS_RUNE_SOFTCAP_POWER)`; multiply absorb value.  
  *Defaults:* divisor 10, softcap 2.0x, power 0.8.

- **Crowd-Control Resistance ("Iron Will")**  
  `cc_resist = min((WIS * Level) / WIS_CC_DIVISOR, WIS_CC_CAP)` chance to ignore stun/fear/charm/silence after the spell lands.  
  *Defaults:* divisor 1000 (doc), cap 0.8 (never full immunity), apply separately from Magic resist.

- **Mana Pool ("Reservoir")**  
  Linear formula shared with INT: `MaxMana = (INT + WIS) * Level * class_mult * INT_WIS_MANA_SCALAR` (same scalar as INT plan).

- **Class Masteries (high level hooks)**  
  - Cleric: overheal converts to temporary rune: `overheal_absorb = overheal * CLERIC_OVERHEAL_ABSORB_MULT` (doc 50%). Holy Smite damage bonus scales with WIS via `CLERIC_HOLY_DMG_DIVISOR` and ignores undead-only restriction.  
  - Shaman: melee/proc bonus from WIS via `SHM_WIS_MELEE_DIVISOR`; DoT poison/disease bonus multiplier `SHM_WIS_DOT_MULT`.  
  - Druid: damage shield bonus curve `druid_ds_mult = 1 + ApplySoftcap(WIS / DRUID_DS_DIVISOR, DRUID_DS_SOFTCAP, DRUID_DS_POWER)`; keep exponential growth optional.

## Runtime/Fallback Knobs
- Toggle: `Combat:UseNewWisSystems` with legacy-safe default unless intentionally enabled for a test shard.
- Spell mitigation: `WIS_SPELL_CAP = 0.50f`, `WIS_SPELL_DIVISOR = 500.0f`, optional `WIS_SPELL_SOFTCAP_START = 1000`, `WIS_SPELL_SOFTCAP_POWER = 0.75f`.
- Heal/Runes: `WIS_HEAL_DIVISOR = 10.0f`, `WIS_HEAL_SOFTCAP = 2.0f`, `WIS_HEAL_SOFTCAP_POWER = 0.8f`; `WIS_RUNE_DIVISOR = 10.0f`, `WIS_RUNE_SOFTCAP = 2.0f`, `WIS_RUNE_SOFTCAP_POWER = 0.8f`.
- CC resist: `WIS_CC_DIVISOR = 1000.0f`, `WIS_CC_CAP = 0.80f`, `WIS_CC_MIN = 0.0f`.
- Mana: reuse `INT_WIS_MANA_SCALAR`; optional `INT_WIS_MANA_GLOBAL_CAP`.
- Cleric: `CLERIC_OVERHEAL_ABSORB_MULT = 0.50f`, `CLERIC_HOLY_DMG_DIVISOR = 20.0f`, `ENABLE_CLERIC_HOLY_ON_ALL` (default true).
- Shaman: `SHM_WIS_MELEE_DIVISOR = 50.0f`, `SHM_WIS_PROC_DIVISOR = 40.0f`, `SHM_WIS_DOT_MULT = 1.15f`.
- Druid: `DRUID_DS_DIVISOR = 100.0f`, `DRUID_DS_SOFTCAP = 2.0f`, `DRUID_DS_POWER = 0.7f`, optional `ENABLE_DRUID_DS_EXPONENTIAL` (default false to start).

## Implementation Steps
1) **Config/Rules**  
   - Add fallback constants, runtime keys, and RuleB toggle; share mana scalar with INT.

2) **Spell Mitigation**  
   - In magic damage resolution (`Mob::ResistSpell`/spell damage stage), apply `spell_reduce` multiplicatively after resists and before runes. Clamp stacking with STA/CHA to keep total reduction sane.

3) **Heal & Rune Mods**  
   - In heal calculations (spells, procs, potions), multiply by `heal_mod`.  
   - In rune absorb creation, multiply base absorb by `rune_mod`. Ensure DS and procs that heal also use the mod.

4) **CC Resistance**  
   - After a control effect lands, roll `cc_resist` to ignore it; apply to stun/fear/charm/silence categories only. Respect cap so immunity is unreachable.

5) **Mana Pool**  
   - Replace diminishing returns in `Client::CalcBaseMana` with linear INT+WIS formula when `USE_NEW_WIS_SYSTEMS` or `USE_NEW_INT_SYSTEMS` is on; keep legacy path otherwise.

6) **Class Masteries**  
   - Cleric: add overheal-to-rune conversion and holy damage bonus for nukes/melee procs using WIS divisors.  
   - Shaman: add WIS-based melee/proc bonuses and DoT poison/disease multiplier.  
   - Druid: add WIS-based damage shield multiplier respecting softcap and toggle.  
   - Paladin/SK/Bst: ensure lifetap/heal procs benefit from `heal_mod`.

7) **Telemetry/Debug**  
   - Debug prints/GM command to show `spell_reduce`, `heal_mod`, `rune_mod`, `cc_resist`, and mana values for tuning sessions.

## Safety & Fun Guardrails
- Spell mitigation capped at 50% and multiplicative ensures STA/CHA/WIS stacking cannot nullify raid AEs.  
- Heal/rune softcaps prevent >3x healing at extreme WIS while still delivering doc’s +100% around 1000 WIS.  
- CC resist cap at 80% keeps danger from control mechanics intact.  
- Druid DS exponential toggle defaults off to avoid runaway thorns until playtested.  
- Cleric overheal shields and Shaman melee bonuses are bounded by divisors to keep them strong but not mandatory for all builds.

## Testing Matrix
- **Spell Mitigation:** Take scripted AE at WIS 0/300/600/1000; confirm reduction curve and total stack with STA/CHA stays within expected band.  
- **Healing/Runes:** Parse heal output from spells/procs at WIS 0/500/1000; verify +100% near 1000 and softcap behavior beyond.  
- **CC Resist:** Repeated stun/fear/charm tests at level 70 with 200/500/1000 WIS; observe ~14%/35%/70% resist rates (before cap).  
- **Mana Pool:** Compare legacy vs new mana at various INT/WIS; ensure values scale linearly and no overflow.  
- **Class Masteries:** Cleric overheal-to-rune behavior, Shaman melee/proc gain, Druid DS scaling; ensure toggles/softcaps work.

## Deliverables
- Runtime keys + fallback constants + RuleB entry; updated mitigation, heal/rune, CC resist, mana pool, and class-specific logic; debug hooks for tuning; doc updates to `WIS.md` after mechanics settle.
