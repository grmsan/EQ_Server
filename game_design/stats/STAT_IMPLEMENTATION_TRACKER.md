# Stat Implementation Tracker

Date: 2026-06-29
Owner: Server/Combat Systems
Status: Working tracker for stat-scaling implementation and hotfix coverage

## Purpose

This file tracks what each stat is supposed to do, whether that behavior is currently wired in code, and how it should connect to the hotfixable combat-balance runtime.

Use this as the handoff checklist for implementation agents. The design docs describe desired gameplay. This tracker records implementation state.

Related design sources:

- `OVERVIEW.md`
- `COMBAT_BALANCE_HOT_RELOAD_PLAN.md`
- `PET_SCALING.md`
- Individual stat docs and implementation plans in this folder.

## Status Labels

- **Design only**: documented target behavior, no verified code path yet.
- **Partially wired**: code exists, but may not be fully gated, hotfixable, validated, or complete.
- **Runtime-ready**: code path exists and can use `CombatBalanceRuntime` values when `Combat:UseHotfixableCombatBalance` is enabled.
- **Validated**: behavior has an explicit test result showing legacy fallback, compiled fallback, runtime override, invalid-value fallback, and in-game/dev validation.
- **Needs audit**: source needs to be checked before work starts because current implementation status is uncertain.

## Global Rules

Every stat effect must follow this hierarchy:

1. Legacy/new formula gate decides whether the new stat behavior runs.
2. If the new formula gate is false, preserve legacy behavior and do not consult runtime combat-balance values.
3. If the new formula gate is true and `Combat:UseHotfixableCombatBalance` is false, use compiled fallback constants.
4. If both are true, use runtime values from `zone/combat_balance.ini`, falling back safely to compiled constants for missing or invalid keys.

Every implementation packet must update this tracker before review.

## Runtime Infrastructure

| Area | Status | Evidence / Notes | Next Work |
| :--- | :--- | :--- | :--- |
| Runtime module skeleton | Partially wired | `zone/combat_balance_runtime.h/.cpp` exists as Phase 0 skeleton. No combat call sites should use it yet. | Fix Phase 0 review items, then implement parser/cache in Phase 1. |
| Runtime parser/cache | Design only | Planned in `COMBAT_BALANCE_HOT_RELOAD_PLAN.md` Phase 1. | Implement typed key registry, INI parser, validation, parse-then-swap cache. |
| Curve evaluator | Design only | Curve model documented in hot-reload plan. | Implement shared evaluator with direct tests before combat wiring. |
| GM reload/status command | Design only | Planned as `#combatbalance reload/status/dump`. | Implement in Phase 2. |
| Contribution-channel model | Design only | Needed to separate base/racial stats from item/spell/AA/heroic stat contribution. | Implement after parser, prove on STR first. |
| Shared pet-scaling model | Design only | `PET_SCALING.md` defines owner stat transfer, pet gear weighting, CHA pet-power multiplier, class scalars, pet-type policy, and final pet caps. | Add registry keys in Phase 1, then wire after core runtime is proven. |

## Shared Pet Scaling Matrix

Pet scaling should be tracked as a shared subsystem. Individual stat rows should feed this model instead of creating separate pet-only formulas.

| Area | Desired Behavior | Current Status | Gate | Runtime / INI Wiring Needed | Target Code Area | Validation |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Owner stat transfer | Pets inherit a small configured portion of owner STR/STA/DEX/AGI/INT/WIS/CHA. | Design only | `Combat:UseNewPetStatScaling` candidate | `PET_OWNER_TRANSFER_GLOBAL_SCALAR`, `PET_OWNER_<STAT>_TRANSFER`, `[Curve.PET_OWNER_TRANSFER]`. | Pet stat/bonus construction paths | Exaggerate global scalar, reload, verify pet output changes, then restore. |
| STR pet transfer | Owner STR contributes to pet physical damage/ATK through the shared model. | Design only | `Combat:UseNewPetStatScaling` candidate | `PET_OWNER_STR_TRANSFER`, `[Curve.PET_DAMAGE]`, final pet caps. | Shared pet-scaling helper, pet attack/damage paths | Exaggerate STR transfer, reload, verify pet physical output changes; confirm no standalone `PET_STR_INHERITANCE` path. |
| STA pet transfer | Owner STA contributes to pet HP/mitigation through the shared model. | Design only | `Combat:UseNewPetStatScaling` candidate | `PET_OWNER_STA_TRANSFER`, `[Curve.PET_HP]`, `[Curve.PET_MITIGATION]`, final pet caps. | Shared pet-scaling helper, pet HP/mitigation calc | Exaggerate STA transfer, reload, verify pet HP/mitigation changes and caps apply. |
| DEX pet transfer | Owner DEX contributes to pet crit/proc/twincast-style offense through the shared model. | Design only | `Combat:UseNewPetStatScaling` candidate | `PET_OWNER_DEX_TRANSFER`, `[Curve.PET_CRIT]`, `[Curve.PET_PROC]`, final pet caps. | Shared pet-scaling helper, pet crit/proc paths | Exaggerate DEX transfer, reload, verify pet crit/proc output changes without adding DEX-only pet scalars. |
| AGI pet transfer | Owner AGI contributes to pet avoidance/haste where supported. | Design only | `Combat:UseNewPetStatScaling` candidate | `PET_OWNER_AGI_TRANSFER`, pet avoidance/haste curve keys, final pet caps. | Shared pet-scaling helper, pet avoidance/timing paths | Exaggerate AGI transfer, reload, verify pet avoidance/haste changes and caps/floors apply. |
| INT pet transfer | Owner INT contributes to pet offensive spell/ATK-equivalent power, especially for caster pets. | Design only | `Combat:UseNewPetStatScaling` candidate; may also depend on `Combat:UseNewIntSystems` for class hooks | `PET_OWNER_INT_TRANSFER`, pet offense curve keys, final pet caps. | Shared pet-scaling helper, pet offensive stat paths | Exaggerate INT transfer, reload, verify caster pet output changes and stacks safely with CHA. |
| WIS pet transfer | Owner WIS contributes to pet defensive/support scaling where supported. | Design only | `Combat:UseNewPetStatScaling` candidate; may also depend on `Combat:UseNewWisSystems` for class hooks | `PET_OWNER_WIS_TRANSFER`, pet mitigation/support curve keys, final pet caps. | Shared pet-scaling helper, pet defensive/support paths | Exaggerate WIS transfer, reload, verify relevant pet defensive/support output changes. |
| CHA pet multiplier | CHA is the main pet-power stat and amplifies owner transfer by class/archetype. | Design only | `Combat:UseNewPetStatScaling` + `Combat:UseNewCharismaSystems` | `[Curve.PET_CHA_POWER]`, `PET_CHA_MULTIPLIER_*`, `PET_CLASS_SCALAR_*`. | Pet stat/bonus construction paths | Exaggerate CHA multiplier, reload, verify high-CHA owner pet becomes stronger and caps apply. |
| Pet gear channel | Pet gear remains independently valuable and tunable. | Design only | `Combat:UseNewPetStatScaling` candidate | `PET_GEAR_STAT_WEIGHT`, final output caps. | Pet inventory/bonus paths | Equip pet gear and verify changing gear weight changes output independent of owner stats. |
| Pet class/archetype scalar | Pet specialists can have stronger pet scaling without duplicating formulas. | Design only | `Combat:UseNewPetStatScaling` candidate | `PET_CLASS_SCALAR_MAGICIAN`, `PET_CLASS_SCALAR_NECROMANCER`, `PET_CLASS_SCALAR_BEASTLORD`, `PET_CLASS_SCALAR_ENCHANTER`, `PET_CLASS_SCALAR_OTHER`. | Shared pet-scaling helper, owner class/pet archetype detection | Compare same owner stats across pet classes/archetypes; verify scalar changes after reload. |
| Final pet caps | Stacked owner stats, CHA, buffs, and pet gear cannot create runaway pets. | Design only | `Combat:UseNewPetStatScaling` candidate | `PET_OUTPUT_SOFTCAP_START`, `PET_OUTPUT_SOFTCAP_POWER`, `PET_OUTPUT_HARDCAP_VALUE`. | Shared pet-scaling helper | Stack high values and verify softcap/hardcap behavior. |
| Pet type policy | Permanent pets, swarm pets, charmed NPCs, bot pets, and NPC-owned pets have explicit opt-in behavior. | Design only | `Combat:UseNewPetStatScaling` candidate plus per-type policy keys if needed | Pet-type enable/scalar keys if implementation requires them. | Pet ownership/type detection paths | Verify permanent pets are affected; verify swarm/charmed/NPC-owned pets follow documented policy and do not inherit accidentally. |

## Stat Coverage Matrix

| Stat | Effect | Desired Player Impact | Current Status | Gate | Runtime / INI Wiring Needed | Target Code Area | Validation |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| STR | Melee base damage | STR should make physical hits meaningfully stronger, especially from gear growth beyond racial baseline. | Needs audit | `Combat:UseNewStrDamageFormula` | Scalar keys, `[Curve.STR_DAMAGE]`, contribution-channel weights. | `zone/attack.cpp` | Parse controlled hits at low/mid/high STR; reload divisor and verify next hits change without restart. |
| STR | Weapon-delay synergy | Slower weapons can keep identity while STR adds predictable base damage. | Needs audit | `Combat:UseNewStrDamageFormula` | Delay scalar/cap keys if not already covered by STR damage curve. | `zone/attack.cpp` | Compare fast/slow weapons at same STR before/after reload. |
| STR | Interrupt/stun resistance | STR can help physical stability without making casters immune. | Design only | STR/new combat gate to confirm | Resistance scalar, cap, curve. | Interrupt/stun handling paths | Test interrupt/stun rates at 100/300/800 STR. |
| STR | Pet physical transfer | STR can feed pet physical damage/ATK through the shared pet-scaling model. | Design only | `Combat:UseNewPetStatScaling` candidate plus STR gate if needed | `PET_OWNER_STR_TRANSFER`, `[Curve.PET_OWNER_TRANSFER]`, pet final caps. | Shared pet-scaling helper | Compare pet damage with owner STR changes and hot reload; verify no STR-only pet inheritance path is added. |
| STA | HP pool | STA should be the main durability stat without old high-stat penalties. | Needs audit | `Combat:UseNewStaminaFormula` | `STA_HP_*` scalars and `[Curve.STA_HP]`. | `zone/client_mods.cpp` | Check max HP at 100/300/800 STA with hotfix off/on. |
| STA | HP/mana/endurance regen | STA should improve sustain in bounded ways. | Needs audit | `Combat:UseNewStaminaFormula` | Separate HP/mana/endurance regen keys and curves. | Regen calculation paths | Reload extreme regen divisor and verify tick values change safely. |
| STA | Physical mitigation | STA can reduce incoming damage but must stack safely with WIS/CHA. | Needs audit | `Combat:UseNewStaminaFormula` | Mitigation scalar, cap, shared defensive cap. | Damage reduction paths | Incoming DPS test with STA-only, then STA+WIS+CHA. |
| STA | Poison/disease/breath endurance | STA supports bodily resistance fantasy. | Design only | STA/new combat gate to confirm | Resistance scalar/cap keys. | Resist/save paths | Resist-rate test at controlled STA values. |
| DEX | Crit chance and overflow | DEX should improve precision and crit reliability, with overflow softcaps. | Needs audit | `Combat:UseNewDexFormulas` | Crit scalar, overflow scalar, `[Curve.DEX_CRIT]`. | `zone/attack.cpp`, spell crit paths | Parse crit rate and crit damage at 100/300/800 DEX. |
| DEX | Proc activity/chains | DEX should make proc builds feel better without infinite chains. | Needs audit | `Combat:UseNewDexFormulas` | Proc chance keys, chain-depth cap, `[Curve.DEX_PROC]`. | Proc paths in combat/effects | Set proc scalar high, reload, verify increased but capped proc frequency. |
| DEX | Bow force | DEX should support ranged physical builds. | Needs audit | `Combat:UseNewDexFormulas` | Bow scalar, curve, softcap. | Ranged attack paths | Bow hit parse before/after runtime override. |
| DEX | Twincast | DEX can support opportunistic extra casts if bounded. | Needs audit | `Combat:UseNewDexFormulas` | Twincast divisor/cap keys. | Spell/effect paths | Twincast rate test with exaggerated divisor. |
| DEX | Resist penetration | DEX helps spells land, sharing a cap with CHA. | Needs audit | `Combat:UseNewDexFormulas` | DEX pen scalar/cap plus shared DEX+CHA cap. | Resist check paths | Land-rate test DEX-only, CHA-only, both. |
| AGI | Avoidance | AGI should improve not-getting-hit without reaching immunity. | Needs audit | `Combat:UseNewAgiFormulas` | `[Curve.AGI_AVOID]`, avoidance scalar/cap. | Avoidance combat paths | Incoming swing avoidance parse at 100/300/800 AGI. |
| AGI | Haste / attack speed | AGI can help faster-feeling combat while obeying haste caps. | Needs audit | `Combat:UseNewAgiFormulas` | Haste scalar, cap, source stacking policy. | Attack timing paths | Reload exaggerated scalar and verify cap/floor. |
| AGI | Run speed | AGI supports mobility but must obey movement caps. | Design only | `Combat:UseNewAgiFormulas` | Run scalar/cap keys. | Movement speed paths | Compare speed before/after reload and cap. |
| AGI | Cast/GCD speed | AGI can reduce cast/GCD time but never to zero. | Design only | `Combat:UseNewAgiFormulas` | Cast speed scalar, minimum cast/GCD floor. | Spell timing paths | Reload low floor and verify enforced minimum. |
| INT | Spell damage power | INT should improve offensive spell output. | Needs audit | `Combat:UseNewIntSystems` | `[Curve.INT_SPELL_POWER]`, spell power scalar/cap. | Spell damage paths | Direct damage and DoT parse at multiple INT values. |
| INT | Cooldown reduction | INT can reduce cooldowns with a hard cap. | Design only | `Combat:UseNewIntSystems` | CDR scalar/cap keys. | Timer/cooldown paths | Reload exaggerated CDR and verify cap. |
| INT | Mana efficiency | INT can reduce mana costs safely. | Design only | `Combat:UseNewIntSystems` | Mana discount scalar/cap keys. | Mana cost paths | Cast-cost test before/after reload. |
| INT | Mana pool with WIS | INT/WIS should jointly support caster mana growth. | Needs audit | `Combat:UseNewIntSystems` / `Combat:UseNewWisSystems` | Mana pool curve and class weights. | `zone/client_mods.cpp` | Max mana check by class/stat mix. |
| INT | Lifetap/DoT/pet offense hooks | INT can support offensive class identity; pet offense should use the shared pet-scaling model. | Design only | `Combat:UseNewIntSystems`; pet piece may also require `Combat:UseNewPetStatScaling` | Hook-specific spell scalars plus `PET_OWNER_INT_TRANSFER` for pet contribution. | Spell effect paths and shared pet-scaling helper | Focused spell parse per hook; pet output test should verify INT transfer goes through shared pet model. |
| WIS | Spell mitigation | WIS should reduce incoming spell damage within a cap. | Needs audit | `Combat:UseNewWisSystems` | `[Curve.WIS_SPELL_MITIGATION]`, mitigation cap. | Incoming spell damage paths | Incoming spell DPS at 100/300/800 WIS. |
| WIS | Heal/rune strength | WIS should make restoration and protection better. | Needs audit | `Combat:UseNewWisSystems` | `[Curve.WIS_HEAL_POWER]`, rune/heal scalars/caps. | Heal/rune effect paths | Heal/rune amount before/after runtime override. |
| WIS | Crowd-control resistance | WIS supports mental/spiritual resilience. | Design only | `Combat:UseNewWisSystems` | `[Curve.WIS_CC_RESIST]`, CC resist cap. | CC resist paths | Mez/root/charm resist-rate test. |
| INT/WIS | SpellDmg extra bonus scaling | INT/WIS should scale only the extra `SpellDmg` damage bonus, not base spell value. | Partially wired | `Spells:EnableIntWisWeightedExtraSpellBonus` | Move existing rule-backed knobs into runtime keys with fallbacks. | `zone/effects.cpp` | Verify base spell damage unchanged; reload multiplier/divisor and observe extra bonus change. |
| INT/WIS | HealAmt extra bonus scaling | WIS/INT should scale only the extra `HealAmt` bonus, not base heal value. | Partially wired | `Spells:EnableIntWisWeightedExtraSpellBonus` | Move existing rule-backed knobs into runtime keys with fallbacks. | `zone/effects.cpp` | Verify base heal unchanged; reload multiplier/divisor and observe extra bonus change. |
| CHA | Rare loot bonus | CHA should improve rare-table odds in a bounded, obvious way. | Design only | `Combat:UseNewCharismaSystems` | `CHA_RARE_LOOT_*` keys and `[Curve.CHA_RARE_LOOT]`. | Loot generation, likely `zone/loot.cpp` / NPC loot-drop path | Set rare bonus intentionally extreme in INI, reload, kill/test known loot table, confirm many extra rare drops, then restore sane values. |
| CHA | Pet HP/damage scaling | CHA should be the main pet-power multiplier/class amplifier, stacking safely with owner stat transfer and pet gear. | Design only | `Combat:UseNewCharismaSystems` plus `Combat:UseNewPetStatScaling` candidate | `[Curve.PET_CHA_POWER]`, `PET_CHA_MULTIPLIER_*`, `PET_CLASS_SCALAR_*`, `PET_OUTPUT_*`; avoid isolated `CHA_PET_*` unless mapped to shared keys. | Shared pet-scaling helper and pet stat recalculation paths | Exaggerate CHA multiplier, reload/resummon/recalc, verify pet HP/damage changes, pet gear remains separately tunable, and softcap works. |
| CHA | Enemy damage reduction | CHA should provide soft incoming damage reduction. | Design only | `Combat:UseNewCharismaSystems` | `CHA_DAMAGE_REDUCTION_*`, shared defensive cap. | Damage intake/reduction paths | Incoming DPS test at 0/300/800 CHA and stacked STA/WIS/CHA. |
| CHA | Resist penetration | CHA helps spells land, sharing cap with DEX. | Design only | `Combat:UseNewCharismaSystems` | `CHA_RESIST_PENETRATION_*`, shared DEX+CHA cap. | Resist check paths | Land-rate test CHA-only and DEX+CHA. |
| CHA | Opportunity crits | CHA gives a light universal crit/opportunity edge. | Design only | `Combat:UseNewCharismaSystems` | `CHA_CRIT_BONUS_*` and cap. | Crit aggregation paths | Parse crit rate with exaggerated CHA crit key. |
| CHA | Charm/CC reliability | CHA should make charm/CC more reliable but preserve floors unless explicitly allowed. | Design only | `Combat:UseNewCharismaSystems` plus optional permanent charm rule | `CHA_CHARM_*`, floor, optional permanent threshold. | Charm and CC break/resist paths | Charm duration/break-rate test; verify floor/permanent toggle. |
| CHA | Bard song power | CHA should improve song impact within caps. | Design only | `Combat:UseNewCharismaSystems` | `CHA_BARD_SONG_*` keys and cap. | Bard song effect paths | Reload exaggerated scalar and verify song effect changes but caps. |

## Per-Effect Implementation Packet Template

Each implementation agent should fill this in for the effect they touch.

```text
Effect:
Design doc:
Feature gate:
Legacy behavior when gate is false:
Compiled fallback constants:
Runtime INI keys:
Curve section:
Contribution channels used:
Pet-scaling impact:
Pet types affected:
Code paths touched:
Validation:
- Build:
- Gate false:
- Gate true + hotfix false:
- Gate true + hotfix true:
- Invalid INI fallback:
- Extreme-value tuning test:
Known limitations:
Tracker rows updated:
```

## CHA Loot Example Validation

Use this shape when CHA rare loot is implemented:

1. Enable `Combat:UseNewCharismaSystems`.
2. Enable `Combat:UseHotfixableCombatBalance`.
3. Set CHA rare loot runtime values to an intentionally extreme test profile in `zone/combat_balance.ini`.
4. Run `#combatbalance reload`.
5. Kill or simulate a target with a known rare table.
6. Confirm rare drops become obviously more frequent.
7. Restore conservative values.
8. Run `#combatbalance reload` again and confirm rates return to normal.

The exact numbers are not important during implementation. The validation goal is to prove the code path is wired, hot-reloadable, and safely reversible without restarting the server.
