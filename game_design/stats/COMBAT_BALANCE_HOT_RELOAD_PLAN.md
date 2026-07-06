# Combat Balance Hot Reload Plan

Date: 2026-06-28
Owner: Server/Combat Systems
Status: Draft for implementation

## 1) High-Level Idea

Enable combat balance tuning without server restarts by loading runtime values from a config file and reloading them with a GM command.

Primary requirement:
- Keep existing behavior in place as the safe default.
- Preserve legacy combat/stat formulas unless the existing new formula rules are enabled.
- Add a hotfix rule under the new-stat formula path so runtime tuning can be turned on/off instantly.
- If anything breaks, switch the hotfix toggle off and immediately fall back to values in zone/combat_balance_config.h while still honoring the new/legacy formula rules.

## 2) Goals and Non-Goals

Goals:
- Preserve current combat behavior by default.
- Add runtime-overridable combat tuning values for every new primary-stat formula path:
  - STR, STA, DEX, AGI, INT, WIS, and CHA.
  - Existing in-flight scope includes STR/STA/DEX/AGI plus INT/WIS SpellDmg/HealAmt bonus scaling.
  - CHA scope is defined in `game_design/stats/CHA.md` and `game_design/stats/CHA_IMPLEMENTATION_PLAN.md`.
- Allow each stat formula's effectiveness to be scaled up or down without rebuilding or restarting the server.
- Add a GM command to reload runtime values in-zone.
- Keep rollback simple: one rule toggle to disable runtime overrides.
- Keep design forward-compatible so we can remove the toggle later after validation.

Non-goals (phase 1):
- Rewriting all combat systems at once.
- Removing existing constants from zone/combat_balance_config.h.
- Cross-process live broadcast reload from world to all zones (can be phase 2).

## 3) Key Items and Where They Live

Existing source of truth today:
- zone/combat_balance_config.h

Rules system (for rollout/rollback toggle):
- common/ruletypes.h
- zone/gm_commands/rules.cpp (existing #rules reload behavior)
- Existing repo rule to reuse or explicitly migrate:
  - Combat:UseHotfixableCombatBalance (bool, default false)

GM command registration:
- zone/command.cpp
- zone/command.h
- zone/gm_commands/ (new command file to add)
- zone/CMakeLists.txt (gm command source list)

Primary combat call sites that read CombatBalance values:
- zone/attack.cpp
- zone/client_mods.cpp
- zone/spell_effects.cpp
- zone/effects.cpp
- zone/mob.cpp
- zone/spells.cpp

Current uncommitted scaling work to include:
- common/ruletypes.h
  - Spells:EnableIntWisWeightedExtraSpellBonus
  - Spells:IntWisDamageSecondaryWisScalar
  - Spells:IntWisHealingSecondaryIntScalar
  - Spells:IntWisExtraSpellBonusStatDivisor
  - Spells:IntWisExtraSpellBonusStatExponent
  - Spells:IntWisScaleExtraSpellAmtByBase
  - Spells:IntWisExtraSpellBonusMinBase
  - Spells:IntWisExtraSpellBonusMaxBase
  - Spells:IntWisExtraSpellBonusBaseExponent
  - Spells:IntWisExtraSpellBonusMultiplierCap
- zone/effects.cpp
  - INT/WIS weighted scaling for SpellDmg and HealAmt extra bonus portions.
  - This must stay scoped to the extra bonus amount from GetExtraSpellAmt and must not modify base spell damage/healing formulas directly.

Existing CHA design scope to include:
- game_design/stats/CHA.md
- game_design/stats/CHA_IMPLEMENTATION_PLAN.md
- game_design/stats/STAT_IMPLEMENTATION_TRACKER.md
- game_design/stats/PET_SCALING.md
- Planned feature gate:
  - Combat:UseNewCharismaSystems
- Planned CHA systems:
  - rare loot bonus
  - pet HP/damage scaling and pet-class multipliers
  - enemy damage reduction
  - resist penetration with shared DEX+CHA cap
  - opportunity crit bonus
  - charm/crowd-control reliability
  - bard song power

Suggested new runtime config components:
- zone/combat_balance_runtime.h
- zone/combat_balance_runtime.cpp
- zone/combat_balance.ini (new file, optional overrides)
- zone/CMakeLists.txt (zone source list)

## 4) Proposed Architecture

### 4.1 Toggle Strategy (Rollback-Safe)

Use the existing rule:
- Combat:UseHotfixableCombatBalance (bool, default false)

Do not add a second runtime-combat-balance rule unless there is a deliberate migration plan. The current rule already exists in common/ruletypes.h and its description already points at runtime values from `combat_balance.ini`.

Formula gate hierarchy:
1. Existing feature rules decide whether the new stat formulas run at all:
  - Combat:UseNewStrDamageFormula
  - Combat:UseNewStaminaFormula
  - Combat:UseNewDexFormulas
  - Combat:UseNewAgiFormulas
  - Combat:UseNewIntSystems
  - Combat:UseNewWisSystems
  - Spells:EnableIntWisWeightedExtraSpellBonus
  - Combat:UseNewCharismaSystems
  - New formula gates should default to false unless the team intentionally wants that new scaling live by default.
2. If the relevant new formula rule is false, keep the legacy/base-game formula and do not consult runtime combat balance values.
3. If the relevant new formula rule is true, use the new formula path.
4. Inside the new formula path, Combat:UseHotfixableCombatBalance decides whether tunable constants come from runtime config or from zone/combat_balance_config.h.

Behavior:
- New formula rule false: use legacy/base-game combat behavior.
- New formula rule true + hotfix rule false: use new formula with compile-time constants/default rules.
- New formula rule true + hotfix rule true: resolve new-formula tuning values from runtime store first; if missing/invalid, fallback to zone/combat_balance_config.h or the existing rule default, depending on where that knob currently lives.

This guarantees two independent rollbacks:
- Disable Combat:UseHotfixableCombatBalance to keep the new formulas but fall back to compiled constants.
- Disable the specific new formula rule to return that stat path to legacy/base-game behavior.

### 4.2 Runtime Value Resolution

Introduce an accessor layer used by combat code:
- Example API shape:
  - CombatBalanceRuntime::GetFloat(CombatBalanceKey key, float fallback)
  - CombatBalanceRuntime::GetInt(CombatBalanceKey key, int fallback)
  - CombatBalanceRuntime::GetBool(CombatBalanceKey key, bool fallback)
  - CombatBalanceRuntime::GetCurveProfile(CombatBalanceCurve key)

Hot-path requirement:
- Combat formulas should use enum keys, not repeated string lookups, so per-hit combat code does not allocate or hash arbitrary strings.
- String names are still useful for parser, status, dump, and diagnostics.

Resolution order:
1. Call runtime accessors only from code already gated by the relevant new formula rule.
2. If Combat:UseHotfixableCombatBalance is false -> return fallback.
3. If true and key exists/valid in runtime cache -> return runtime value.
4. Otherwise -> return fallback (existing constant).

Reload safety:
- Parse into a temporary cache first.
- Validate the whole file.
- Swap the active cache only after parsing completes.
- If reload fails catastrophically, keep the previous active cache and report the failure.
- If individual keys are invalid, keep the cache but mark those keys invalid so callers fall back to constants.

### 4.3 Config File Format

File:
- zone/combat_balance.ini

Example:
- [CombatBalance]
- STR_LEVEL_DIVISOR=60.0
- STR_LEVEL_EXPONENT=1.0
- STR_MIN_LEVEL_MULTIPLIER=0.05
- ENABLE_STR_DIMINISHING_RETURNS=true
- STA_HP_DIVISOR=10.0
- INTWIS_EXTRA_SPELL_BONUS_STAT_DIVISOR=1200.0
- INTWIS_EXTRA_SPELL_BONUS_MULTIPLIER_CAP=1.75

Notes:
- Keep names exactly matching current constant names where possible.
- For existing rules-backed knobs, use names that clearly map to the rule, even if the INI spelling is shortened for readability.
- Parser should ignore unknown keys with warnings.
- Invalid values should log + fallback to existing constant.
- Missing file is not an error; it means zero overrides loaded.
- The default checked-in file should either be absent or contain commented examples only. Do not require duplicating every constant into the file for baseline behavior.
- Resolve the file relative to the server working directory, and document the exact expected runtime location for Windows/dev launches.

### 4.4 Reload Command

Add GM command (suggestion):
- #combatbalance reload

Behavior:
- Re-read INI file into runtime cache.
- Validate parse results.
- Return summary to GM:
  - loaded count
  - ignored/invalid count
  - current mode (runtime on/off)
- This reloads only the current zone process. Other active zones must run the command too, or use a later global/broadcast reload phase.

Optional helper subcommands:
- #combatbalance status
- #combatbalance dump

### 4.5 Curve Control Framework (Required for Fine Tuning)

Runtime overrides must support not just scalar tuning, but curve shaping so each stat can be tuned across low/mid/high power bands.

Curve goals:
- Keep low levels and low stat values near baseline when desired.
- Starting character stats generally range from about 60 to 150. That range should create flavor and small advantages, not extreme power gaps.
- Meaningful scaling should start becoming obvious once gear pushes stats well beyond racial baselines, for example around 250-300+ in a stat.
- Ramp aggressively in chosen ranges.
- Apply soft cap and/or hard cap safely.
- Allow truly linear behavior when needed.

Standard curve model per attribute effect:
1. Input normalization:
  - input_value = stat and/or level input (for example STR, level, or STR*level).
2. Low-end deadzone or baseline anchor (optional):
  - if input_value <= CURVE_START, output uses baseline/minimal slope.
3. Ramp segment:
  - output scales by RAMP_SLOPE or POWER curve above CURVE_START.
4. Soft cap segment (optional):
  - output transitions to reduced gain after SOFTCAP_START.
5. Hard cap segment (optional):
  - output is clamped at HARDCAP_VALUE.

Required runtime knobs (generic):
- <KEY>_CURVE_MODE = linear | piecewise_linear | power | logistic
- <KEY>_CURVE_START = threshold where accelerated scaling starts
- <KEY>_BASE_SLOPE = low-end slope (near baseline)
- <KEY>_RAMP_SLOPE = high-growth slope after threshold
- <KEY>_POWER = exponent for power curves (if mode=power)
- <KEY>_LOGISTIC_MIDPOINT = midpoint for logistic curves (if mode=logistic)
- <KEY>_LOGISTIC_STEEPNESS = steepness for logistic curves (if mode=logistic)
- <KEY>_SOFTCAP_START = value where diminishing begins
- <KEY>_SOFTCAP_POWER = how strongly post-softcap gains compress
- <KEY>_HARDCAP_VALUE = absolute max output (optional, 0/negative means disabled)
- <KEY>_MIN_OUTPUT = optional floor

Per-attribute examples to include in phase rollout:
- STR damage:
  - low baseline at early levels, fast ramp midgame, optional top-end softcap.
- STA HP/regen/mitigation:
  - conservative early growth, class-friendly mid ramp, hard safety caps for mitigation.
- DEX crit/proc/twincast:
  - low-level near baseline, stronger high-end scaling, overflow softcap for crit damage.
- AGI avoid/haste/cast speed:
  - smooth growth with hard floor/ceiling to avoid degenerate behavior.
- INT spell offense/efficiency:
  - spell damage, wizard crit damage, cooldown reduction, mana efficiency, mana pool, lifetap/DoT hooks, and pet offensive stat sharing.
- WIS spell defense/support:
  - spell mitigation, heal/rune strength, crowd-control resistance, shared mana pool, and priest class hooks.
- INT/WIS SpellDmg/HealAmt extra bonus:
  - INT primarily scales damage extra bonus, WIS contributes as a secondary scalar.
  - WIS primarily scales healing extra bonus, INT contributes as a secondary scalar.
  - Base spell value weighting keeps low-base spells from gaining too much and lets high-base spells ramp into the full multiplier.
  - The multiplier cap prevents SpellDmg/HealAmt from exploding at extreme stats.
- CHA support/combat formula:
  - Use the systems defined in `CHA_IMPLEMENTATION_PLAN.md`: rare loot, pet empowerment, damage reduction, resist penetration, opportunity crits, charm/CC reliability, and bard song power.
  - The same baseline/ramp/softcap/hardcap framework should apply so each CHA system can be tuned up or down without code changes.
- Pet scaling:
  - Use the shared model in `PET_SCALING.md` instead of one-off pet inheritance per stat.
  - Owner stats transfer at small configured rates, pet gear is weighted separately, and CHA is the main pet-power multiplier/class amplifier.
  - Pet scaling must expose global scalar, per-stat transfer rates, pet gear weight, CHA multiplier curve, class multipliers, and final pet output caps.

INI grouping recommendation:
- [Curve.STR_DAMAGE]
- [Curve.STA_HP]
- [Curve.STA_MIT]
- [Curve.DEX_CRIT]
- [Curve.DEX_PROC]
- [Curve.AGI_AVOID]
- [Curve.INT_SPELL_POWER]
- [Curve.INT_COOLDOWN_REDUCTION]
- [Curve.INT_MANA_EFFICIENCY]
- [Curve.WIS_SPELL_MITIGATION]
- [Curve.WIS_HEAL_POWER]
- [Curve.WIS_CC_RESIST]
- [Curve.INTWIS_SPELL_DAMAGE_BONUS]
- [Curve.INTWIS_HEAL_AMOUNT_BONUS]
- [Curve.CHA_RARE_LOOT]
- [Curve.CHA_DAMAGE_REDUCTION]
- [Curve.CHA_RESIST_PENETRATION]
- [Curve.CHA_CRIT_BONUS]
- [Curve.CHA_CHARM_CC]
- [Curve.CHA_BARD_SONG]
- [Curve.PET_OWNER_TRANSFER]
- [Curve.PET_CHA_POWER]
- [Curve.PET_HP]
- [Curve.PET_DAMAGE]
- [Curve.PET_CRIT]
- [Curve.PET_PROC]
- [Curve.PET_MITIGATION]

Validation requirement:
- Every curve-enabled key must log the active mode and effective parameters at reload time.
- Every curve-enabled computation must expose whether result hit softcap/hardcap (debug mode).
- Linear mode with default parameters must be identity-preserving where it replaces an existing scalar formula.
- Piecewise/power/logistic formulas must be implemented in one shared evaluator with direct tests before being wired into combat paths.
- Low-level racial baseline tests must confirm that 60-150 starting stat values do not create broken outcomes.
- Gear-growth tests must confirm that stat values around 250-300+ feel meaningfully stronger than baseline without making the starting race choice feel mandatory.

### 4.6 Racial Baseline and Gear Growth Targets

The new formulas should make gear upgrades feel meaningful without making level-1 racial stat differences dominate character viability.

Design constraints:
- A level 1 character with about 60-150 in a starting stat should remain close to baseline output.
- Racial differences should be noticeable but not punishing. Example: a gnome warrior around 85 STR and an ogre warrior around 150 STR can differ slightly in early STR damage, but the gap should not imply that one race was the wrong pick.
- Different races should be allowed to express different strengths. Example: the gnome may have better AGI-driven speed/avoidance while the ogre has better STR-driven damage.
- Gear should be the main source of large stat-scaling gains. A character reaching about 300 STR from gear should feel meaningfully stronger than a baseline character, but still inside softcap/hardcap safety limits.
- Race should shape flavor, not loot responsiveness. The same +10 STR item should produce a similar percentage-feel upgrade for an 80 STR gnome and a 150 STR ogre, even if the ogre remains slightly ahead in raw STR fantasy.
- Every stat formula needs independent hotfix knobs for:
  - baseline strength at 60-150 stat values
  - ramp start around the gear-growth threshold
  - high-stat slope
  - softcap behavior
  - hardcap/safety ceiling

Implementation direction:
- Treat racial/base stats and gear stats as separate contribution channels where the source data is available.
- For clients, base stats are available through `GetBaseSTR()`/`GetBaseSTA()`/etc. and item stats are available through `itembonuses.STR`/`itembonuses.STA`/etc.
- Do not blindly feed total `GetSTR()`/`GetSTA()`/etc. into new scaling formulas when race-vs-gear feel matters.
- Add a small helper/model that returns per-stat contribution pieces:
  - base/racial contribution
  - item contribution
  - spell/buff contribution
  - AA/permanent progression contribution
  - heroic stat contribution where relevant
- NPCs, pets, bots, and mercs may not have the same clean racial/item split. For those paths, document whether they use total stat, inherited owner stat, or a reduced player-channel model.

Recommended contribution model:
1. Compress the racial/base contribution early.
2. Let item contribution begin producing visible value from the first upgrade.
3. Apply optional catch-up slope for low-baseline characters so early gear is not muted by poor starting stats.
4. Combine weighted contributions into an effective stat for the new formula.
5. Apply final-output softcap/hardcap after the formula result, not just on raw stat input.

Example effective-stat shape:
- `effective_stat = base_stat * BASE_WEIGHT + item_stat * ITEM_WEIGHT + spell_stat * SPELL_WEIGHT + aa_stat * AA_WEIGHT`
- `base_stat` can be passed through base compression before weighting.
- `item_stat` can have a minimum-impact floor so small upgrades are still noticeable.
- Final formula output still goes through softcap/hardcap safety.

Hotfix knobs to add per stat/formula:
- <KEY>_BASE_STAT_WEIGHT
- <KEY>_ITEM_STAT_WEIGHT
- <KEY>_SPELL_STAT_WEIGHT
- <KEY>_AA_STAT_WEIGHT
- <KEY>_BASE_COMPRESSION_START
- <KEY>_BASE_COMPRESSION_POWER
- <KEY>_ITEM_RAMP_START
- <KEY>_ITEM_MIN_IMPACT_FLOOR
- <KEY>_LOW_BASE_CATCHUP_ENABLED
- <KEY>_LOW_BASE_CATCHUP_THRESHOLD
- <KEY>_LOW_BASE_CATCHUP_SLOPE
- <KEY>_FINAL_OUTPUT_SOFTCAP_START
- <KEY>_FINAL_OUTPUT_SOFTCAP_POWER
- <KEY>_FINAL_OUTPUT_HARDCAP_VALUE

Shared pet-scaling knobs should live beside the stat knobs:
- PET_OWNER_TRANSFER_GLOBAL_SCALAR
- PET_OWNER_<STAT>_TRANSFER for STR/STA/DEX/AGI/INT/WIS/CHA
- PET_GEAR_STAT_WEIGHT
- PET_CHA_MULTIPLIER_MIN
- PET_CHA_MULTIPLIER_MAX
- PET_CLASS_SCALAR_<CLASS_OR_ARCHETYPE>
- PET_OUTPUT_SOFTCAP_START
- PET_OUTPUT_SOFTCAP_POWER
- PET_OUTPUT_HARDCAP_VALUE

Rule-of-thumb tuning target:
- The same +10 item should produce close percentage-feel gains across races, not identical gains.
- Racial edge should be a small constant identity advantage, not a multiplier that snowballs through every gear upgrade.
- Gear progression should be the primary long-term driver of the power fantasy.

## 5) Detailed Implementation Plan (Step by Step)

Each phase below is intended to be assignable to one agent. The agent should complete only that phase, run the listed validation, and return a short review packet before the next phase starts.

Review packet required from every phase:
- Changed files.
- What behavior is now possible.
- Validation commands/results.
- Known limitations or deferred items.
- Confirmation that unrelated files/behavior were not intentionally changed.
- Confirmation that `STAT_IMPLEMENTATION_TRACKER.md` was updated for every stat effect touched.

### Phase 0 - Guardrails and Build Skeleton

Goal:
- Establish the safe feature gates and compile-time skeleton without changing runtime combat behavior.

Agent scope:
- Confirm and reuse `Combat:UseHotfixableCombatBalance` with default false.
- Document that this rule only affects new formula paths; it must not cause legacy formulas to consult runtime config.
- Confirm `Spells:EnableIntWisWeightedExtraSpellBonus` default. If preserving legacy spell bonus behavior by default is required, set it to false before shipping.
- Add `zone/combat_balance_runtime.h/.cpp` skeleton with no live combat usage yet.
- Add `zone/combat_balance_runtime.cpp` to `zone_sources` in `zone/CMakeLists.txt`.
- Add initial logging stubs for load/parse/fallback events.

Out of scope:
- No parser implementation beyond no-op skeleton.
- No GM command.
- No combat call-site integration.

Validation:
- Build the zone target.
- Verify `Combat:UseHotfixableCombatBalance` exists and defaults false.
- Verify no combat source file calls runtime getters yet.

Completion criteria:
- Server compiles.
- Default runtime behavior is unchanged.
- The skeleton can be reviewed independently before parser work begins.

Review gate:
- Stop after this phase and review the skeleton/API shape.

### Phase 1 - Runtime Loader, Cache, and Unit-Testable Curves

Goal:
- Implement the runtime config loader/cache and curve evaluator without wiring it into combat formulas.

Agent scope:
- Implement INI parser logic for `[CombatBalance]`.
- Implement parser support for curve sections such as `[Curve.STR_DAMAGE]`.
- Implement enum-backed typed getters:
  - `GetFloat(CombatBalanceKey key, float fallback)`
  - `GetInt(CombatBalanceKey key, int fallback)`
  - `GetBool(CombatBalanceKey key, bool fallback)`
  - `GetCurveProfile(CombatBalanceCurve key)`
- Add strict validation:
  - bool parsing (`true/false/1/0`)
  - numeric parse checks
  - finite-value checks (reject NaN/inf)
  - range checks for dangerous knobs
- Parse into a temporary cache and swap only after successful parse.
- Keep previous valid cache if a reload fails catastrophically.
- Add unit-testable curve evaluator code that does not require a live zone process.
- Add key registry entries for all planned stat systems, even if later phases wire only a subset:
  - STR, STA, DEX, AGI, INT, WIS, INT/WIS SpellDmg/HealAmt, CHA.
- Add contribution-channel model data structures/helpers, but do not wire them into combat yet.
- Add shared pet-scaling key registry entries and data structures from `PET_SCALING.md`, but do not wire them into pet combat yet.

Out of scope:
- No GM command.
- No combat call-site integration.
- No gameplay tuning decisions.

Validation:
- Add or run focused parser/evaluator tests if the repo has an appropriate test harness.
- If no test harness exists, add a small CLI/dev-only validation path or compile-time test function that can verify:
  - missing file loads as zero overrides
  - valid scalar values are available
  - invalid scalar values fall back
  - malformed reload keeps previous cache
  - linear curve is identity-preserving
  - softcap/hardcap curve behavior is deterministic
- Build the zone target.

Completion criteria:
- Loader can read config and expose validated values.
- Missing/invalid config cannot break combat math.
- Curve evaluator has direct validation independent of live combat.

Review gate:
- Stop after this phase and review parser behavior, key naming, curve semantics, and fallback policy.

### Phase 2 - GM Reload and Status Command

Goal:
- Let a GM reload and inspect runtime combat balance values in one active zone process.

Agent scope:
- Add `zone/gm_commands/combatbalance.cpp`.
- Add declaration in `zone/command.h`.
- Register `#combatbalance` in `zone/command.cpp`.
- Add `gm_commands/combatbalance.cpp` to `gm_command_sources` in `zone/CMakeLists.txt`.
- Implement:
  - `#combatbalance reload`
  - `#combatbalance status`
  - `#combatbalance dump <optional-filter>`
- Bound dump output so it cannot flood client chat.
- Include current mode, key counts, invalid count, last successful reload, and last failed reload.
- Add access checks consistent with balance-impacting commands.

Out of scope:
- No combat call-site integration.
- No global/broadcast reload.

Validation:
- Build the zone target.
- In a dev zone, run:
  - `#combatbalance status`
  - `#combatbalance reload`
  - `#combatbalance dump STR`
- Verify malformed config reports failure and keeps prior valid cache.
- Verify command permission level is enforced.

Completion criteria:
- GM can reload runtime values without restarting the zone.
- Command output is clear enough for tuning sessions.
- Reload remains local to the current zone and says so.

Review gate:
- Stop after this phase and review command UX before any combat behavior uses runtime values.

### Phase 3 - First Combat Integration: STR Scalar Proof

Goal:
- Prove the runtime path on a narrow, high-value STR damage slice while preserving legacy behavior.

Agent scope:
- Convert only the first STR scalar keys:
  - `STR_LEVEL_DIVISOR`
  - `STR_LEVEL_EXPONENT`
  - `STR_MIN_LEVEL_MULTIPLIER`
- Use runtime accessors only inside the existing `RuleB(Combat, UseNewStrDamageFormula)` path.
- Ensure this hierarchy:
  - `UseNewStrDamageFormula = false` -> legacy/base behavior
  - `UseNewStrDamageFormula = true` and `UseHotfixableCombatBalance = false` -> compiled fallback constants
  - both true -> runtime config with fallback constants
- Add minimal debug/log output only if needed for validation.

Out of scope:
- No curve application yet.
- No contribution-channel split yet.
- No STA/DEX/AGI/INT/WIS/CHA integration.

Validation:
- Build the zone target.
- Run a controlled melee test or existing combat test path for:
  - new STR formula off
  - new STR formula on + hotfix off
  - new STR formula on + hotfix on
- Change `STR_LEVEL_DIVISOR` in `combat_balance.ini`, run `#combatbalance reload`, and verify subsequent hits use the new value without restart.
- Verify invalid `STR_LEVEL_DIVISOR` logs/falls back.

Completion criteria:
- First STR values can be hot reloaded reliably.
- Legacy path remains untouched when `UseNewStrDamageFormula` is false.
- Hotfix toggle can instantly return STR tuning to compiled constants.

Review gate:
- Stop after this phase and review combat diff, validation output, and ergonomics before adding curves or more stats.

### Phase 4 - Curve and Contribution-Channel Proof on STR

Goal:
- Prove curve controls and race-vs-item contribution channels on one formula before expanding to other stats.

Agent scope:
- Add STR damage curve profile support, such as `[Curve.STR_DAMAGE]`.
- Wire `ApplyCurve(profile, raw_input)` to STR damage only.
- Implement player contribution-channel input for STR where source data is available:
  - base/racial STR
  - item STR
  - spell/buff STR
  - AA/permanent STR
  - heroic STR if relevant
- Add runtime keys for:
  - base/racial weight
  - item weight
  - item minimum-impact floor
  - low-base catch-up settings
  - final-output softcap/hardcap
- Document non-client behavior for pets/bots/mercs/NPCs.
- Do not implement STR-only pet inheritance; document how STR will feed the shared pet-scaling helper instead.

Out of scope:
- No additional stat systems.
- No final tuning numbers beyond safe defaults.

Validation:
- Build the zone target.
- Test starting STR examples around 60, 85, 100, and 150.
- Test same +10 STR item on low-base and high-base race examples; verify both get a noticeable gain and similar percentage-feel improvement.
- Test gear-growth points around 250, 300, and 500 STR.
- Verify final-output softcap/hardcap behavior.
- Verify all tests still fall back when hotfix is disabled.

Completion criteria:
- STR has validated scalar, curve, and contribution-channel runtime controls.
- Race base stats remain flavor, not mandatory race-choice power.
- Item upgrades feel immediately meaningful.

Review gate:
- Stop after this phase and review design feel before applying the model to more stats.

### Phase 5 - Core Stat Expansion Batches

Goal:
- Apply the proven runtime/curve/contribution model to STA, DEX, and AGI in small batches.

Agent scope:
- Batch 5A: STA HP/regen/mitigation controls in `zone/client_mods.cpp` and related paths.
- Batch 5B: DEX crit/proc/twincast/bow/penetration controls in `zone/attack.cpp`, `zone/effects.cpp`, `zone/mob.cpp`, and spell paths as needed.
- Batch 5C: AGI avoidance/haste/run/cast controls in `zone/attack.cpp`, `zone/mob.cpp`, and spell timing paths as needed.
- Each batch must preserve its existing formula gate:
  - `Combat:UseNewStaminaFormula`
  - `Combat:UseNewDexFormulas`
  - `Combat:UseNewAgiFormulas`
- Each batch should include per-stat effectiveness scalar, curve controls, and source-channel handling where source data is available.

Out of scope:
- INT/WIS broader systems.
- CHA systems.
- Production rollout.

Validation:
- Build after each batch.
- For each stat batch, test:
  - new formula rule off -> legacy unchanged
  - new formula rule on + hotfix off -> compiled fallback constants
  - new formula rule on + hotfix on -> runtime values after reload
  - invalid runtime value -> fallback
  - low baseline range 60-150 remains sane
  - gear-growth range 250/300/500 feels meaningful
- Record validation output separately for 5A, 5B, and 5C.

Completion criteria:
- STA, DEX, and AGI targeted values can be hot reloaded reliably.
- Each batch is independently reviewable and can be reverted or adjusted without blocking the others.

Review gate:
- Stop after each sub-batch for review. Do not start the next sub-batch until the previous one is accepted.

### Phase 6 - Spell Stat Expansion: INT, WIS, and INT/WIS Extra Bonus

Goal:
- Bring spell-facing stat systems under the same runtime tuning model.

Agent scope:
- Wire runtime controls for existing INT/WIS SpellDmg/HealAmt extra bonus work in `zone/effects.cpp`.
- Keep `Spells:EnableIntWisWeightedExtraSpellBonus` as the feature gate for extra bonus scaling.
- Add broader INT system controls where implemented:
  - `Combat:UseNewIntSystems`
  - spell power
  - wizard crit damage
  - cooldown reduction
  - mana efficiency
  - mana pool
  - lifetap/DoT hooks
  - pet offensive stat share
- Add broader WIS system controls where implemented:
  - `Combat:UseNewWisSystems`
  - spell mitigation
  - heal/rune strength
  - CC resistance
  - shared mana pool
  - priest class hooks
- Ensure INT/WIS extra bonus scaling only affects the extra `SpellDmg`/`HealAmt` portion, not base spell values.

Out of scope:
- CHA systems.
- Final balancing numbers.

Validation:
- Build the zone target.
- Verify `Spells:EnableIntWisWeightedExtraSpellBonus = false` preserves legacy SpellDmg/HealAmt behavior.
- Verify hotfix off uses compiled/rule fallback values.
- Verify hotfix on updates after `#combatbalance reload`.
- Test direct damage, DoT, heal, and reflected/dot paths touched by `zone/effects.cpp`.
- Verify base spell values are unchanged by extra-bonus scaling.

Completion criteria:
- INT/WIS spell-facing knobs are reloadable and feature-gated.
- Legacy spell behavior remains available.

Review gate:
- Stop after this phase and review spell math carefully before CHA or rollout work.

### Phase 7 - CHA Systems

Goal:
- Implement and hot-reload the CHA systems defined in `CHA_IMPLEMENTATION_PLAN.md`.

Agent scope:
- Add/confirm `Combat:UseNewCharismaSystems`.
- Add/confirm shared pet-scaling gate if needed, such as `Combat:UseNewPetStatScaling`.
- Add runtime/fallback keys and curve sections for:
  - rare loot bonus
  - pet HP/damage scaling and pet-class multipliers
  - enemy damage reduction
  - resist penetration with shared DEX+CHA cap
  - opportunity crit bonus
  - charm/crowd-control reliability
  - bard song power
- For pet HP/damage scaling, use the shared pet-scaling model from `PET_SCALING.md`:
  - owner stat transfer rates
  - pet gear weight
  - CHA pet-power multiplier
  - class/archetype multipliers
  - final pet output caps
- Implement the lowest-risk systems first if this phase is split:
  - 7A: opportunity crit/resist penetration
  - 7B: pet power
  - 7C: damage reduction
  - 7D: charm/CC and bard song
  - 7E: loot bonus, with extra care around loot-generation path

Out of scope:
- Global balance tuning.
- Removing fallback constants or feature gates.

Validation:
- Build after each CHA sub-batch.
- Verify `Combat:UseNewCharismaSystems = false` preserves legacy behavior.
- Verify hotfix off uses compiled fallback values.
- Verify hotfix on updates after reload.
- Run targeted checks listed in `CHA_IMPLEMENTATION_PLAN.md` for the system touched.
- Verify DEX+CHA shared penetration cap where applicable.
- Verify CHA pet scaling stacks safely with owner stat transfer and pet gear caps.

Completion criteria:
- CHA systems are feature-gated, hotfixable, and individually validated.
- High-impact systems such as loot and charm have explicit safeguards.

Review gate:
- Stop after each CHA sub-batch for review.

### Phase 8 - Diagnostics, Hardening, and Operational Validation

Goal:
- Make the runtime system safe and understandable during real tuning sessions.

Agent scope:
- Add verbose load diagnostics for troubleshooting.
- Add command output showing last successful reload, last failed reload, error count, loaded key count, invalid key count, and migrated-key coverage.
- Add sanity clamps for high-risk values.
- Ensure logs clearly show active source:
  - legacy path
  - compiled fallback constants
  - runtime override
- Add validation scenarios for malformed files, missing files, and partial invalid values.

Out of scope:
- New stat system integration.
- Production rollout.

Validation:
- Build the zone target.
- Run malformed/missing/valid config reload tests.
- Run `#combatbalance status` and verify it is understandable to a GM.
- Run `#rules reload` and verify hotfix toggle changes behavior immediately.
- Confirm command output does not flood client chat.

Completion criteria:
- Runtime system is stable with predictable fallback behavior.
- Operators can tell exactly which source is active and what failed.

Review gate:
- Stop after this phase and do an operational readiness review.

### Phase 9 - Controlled Rollout and Future Cleanup Decision

Goal:
- Roll out the system safely and decide whether/when to remove dual-path fallback.

Agent scope:
- Run with `Combat:UseHotfixableCombatBalance = false` in production first.
- Enable hotfix runtime values only in limited test windows.
- Track fallback events, invalid config events, and tuning changes.
- Document any values changed during live tuning.
- Decide later whether runtime config should become the default path and whether the hotfix toggle can be removed.

Out of scope:
- New feature development.
- Removing legacy formula paths during initial rollout.

Validation:
- Production/test-window checklist:
  - toggle off works
  - toggle on works
  - `#combatbalance reload` works
  - rollback path works without restart
  - affected zones are known because reload is local per zone

Completion criteria:
- Team agrees the system is safe enough for continued use or future cleanup.
- Rollback procedure has been exercised successfully.

## 6) Testing Checklist

Functional tests:
- [ ] New formula rule OFF -> legacy/base-game formula is used and runtime config is ignored.
- [ ] New formula rule ON + hotfix rule OFF -> new formula uses zone/combat_balance_config.h constants.
- [ ] New formula rule ON + hotfix rule ON + no combat_balance.ini file -> fallback to existing constants.
- [ ] New formula rule ON + hotfix rule ON + valid INI -> runtime values used.
- [ ] New formula rule ON + hotfix rule ON + invalid value -> warning + fallback to constant.
- [ ] Reload with malformed file keeps previous valid cache and reports failure.
- [ ] #combatbalance reload applies changed value without restart.
- [ ] Curve mode linear reproduces expected linear output.
- [ ] Curve mode piecewise/power/logistic honors low-range baseline and configured ramp behavior.
- [ ] Softcap transition reduces marginal gains as configured.
- [ ] Hardcap clamps output at expected threshold.
- [ ] Per-stat global effectiveness scalars can tune each new stat formula up and down after #combatbalance reload.
- [ ] Starting stat range tests around 60, 85, 100, 150 show modest differences only.
- [ ] Gear-growth tests around 250, 300, 500, and higher values show meaningful gains with safe softcap/hardcap behavior.
- [ ] Same-item tests: +10 stat item produces a clear gain for low-base and high-base races.
- [ ] Same-item tests: +10 stat item produces similar percentage-feel gains across races, with race still preserving a small identity edge.
- [ ] Catch-up tests: low-baseline characters do not need several early upgrades before seeing a noticeable benefit.

Regression tests:
- [ ] STR damage legacy behavior unchanged when Combat:UseNewStrDamageFormula is false.
- [ ] STA HP/regen legacy behavior unchanged when Combat:UseNewStaminaFormula is false.
- [ ] DEX/AGI legacy behavior unchanged when Combat:UseNewDexFormulas or Combat:UseNewAgiFormulas is false.
- [ ] INT/WIS legacy behavior unchanged when Combat:UseNewIntSystems or Combat:UseNewWisSystems is false.
- [ ] SpellDmg/HealAmt legacy behavior unchanged when Spells:EnableIntWisWeightedExtraSpellBonus is false.
- [ ] INT/WIS scaling only changes the extra SpellDmg/HealAmt bonus portion, not the base spell value.
- [ ] CHA legacy behavior remains unchanged when Combat:UseNewCharismaSystems is false.
- [ ] New formula compiled-constant behavior unchanged when Combat:UseHotfixableCombatBalance is false.

Operational tests:
- [ ] #rules reload still behaves normally.
- [ ] Turning Combat:UseHotfixableCombatBalance off with #rules set or #rules reload immediately bypasses runtime values.
- [ ] Command permission checks enforced.
- [ ] Logs clearly show active source (runtime vs fallback).
- [ ] New files are included in CMake and the zone target links on Windows.

## 7) Rollback Plan

Immediate rollback sequence:
1. Set Combat:UseHotfixableCombatBalance = false.
2. Run #rules reload in active zone(s).
3. (Optional) run #combatbalance status to confirm runtime path disabled.
4. If the new formula itself is the problem, disable the relevant formula rule, such as Combat:UseNewStrDamageFormula, and reload rules.

Expected result:
- First rollback keeps new formulas but reverts tuning values to zone/combat_balance_config.h.
- Second rollback returns the affected stat path to legacy/base-game behavior.
- No restart required for either rollback path.

## 8) Suggested Initial Scope (Safe First Pass)

First pass keys:
- STR_LEVEL_DIVISOR
- STR_LEVEL_EXPONENT
- STR_MIN_LEVEL_MULTIPLIER
- ENABLE_STR_DIMINISHING_RETURNS
- STR_DIMINISHING_START
- STR_DIMINISHING_POWER

Why this scope:
- High impact for tuning.
- Limited blast radius.
- Easy to validate with existing combat logs and test scenarios.

Initial curve-control rollout sequence:
- Phase 4: STR damage curve and contribution-channel proof first.
- Phase 5A: STA HP/regen/mitigation.
- Phase 5B: DEX crit/proc/twincast/bow/penetration.
- Phase 5C: AGI avoidance/haste/run/cast.
- Phase 6: INT/WIS SpellDmg/HealAmt and broader INT/WIS spell systems.
- Phase 7: CHA systems from `CHA_IMPLEMENTATION_PLAN.md`, starting with the lowest-risk runtime-only path.

## 9) Risks and Mitigations

Risk: mis-typed INI causes broken combat math.
- Mitigation: strict parse + range checks + fallback to constants.

Risk: partial migration creates inconsistent behavior.
- Mitigation: phase batching + clear list of migrated keys + command status output.

Risk: starting race stat differences become too dominant.
- Mitigation: low baseline slope for 60-150 stat values, racial baseline tests, and ramp thresholds that favor geared stat investment over character creation stats.

Risk: gear upgrades feel better for high-baseline races than low-baseline races.
- Mitigation: separate base and item contribution channels, item minimum-impact floors, and optional low-base catch-up slope.

Risk: gear stat scaling feels flat after safety clamps.
- Mitigation: separate low-stat baseline, gear-ramp, softcap, and hardcap knobs so midgame gear gains can be strong without breaking early game or top-end safety.

Risk: operator confusion about active source.
- Mitigation: explicit logs and #combatbalance status output showing mode and key counts.

## 10) Implementation Checklist (Master)

- [ ] Add rollout rule in common/ruletypes.h.
- [ ] Add runtime loader/cache module in zone/.
- [ ] Add INI file and parser.
- [ ] Add GM command + registration.
- [ ] Add generic curve profile model and evaluator.
- [ ] Add curve sections to INI with defaults matching current behavior.
- [ ] Add per-stat effectiveness scalar controls and baseline/ramp/softcap/hardcap controls.
- [ ] Add per-stat contribution-channel controls for base/racial, item, spell, AA, and final-output caps.
- [ ] Integrate first batch of values in attack.cpp.
- [ ] Integrate first curve (STR damage) and validate at low/mid/high levels.
- [ ] Validate starting-stat range behavior at 60/85/100/150.
- [ ] Validate same +10 item upgrade behavior for low-base and high-base race examples.
- [ ] Validate gear-growth behavior at 250/300/500+.
- [ ] Validate fallback behavior.
- [ ] Integrate additional values in client_mods.cpp.
- [ ] Integrate additional values in spell_effects.cpp.
- [ ] Integrate STA/DEX/AGI curve controls.
- [ ] Integrate INT/WIS SpellDmg/HealAmt controls.
- [ ] Add CHA controls from `CHA_IMPLEMENTATION_PLAN.md`.
- [ ] Add shared pet-scaling controls from `PET_SCALING.md`.
- [ ] Complete functional/regression testing.
- [ ] Run controlled rollout with toggle OFF then ON.
- [ ] Decide on eventual toggle removal after validation period.

---

If this plan is approved, implementation should start with Phase 0 only, return for review, and then proceed phase by phase through the review gates above.
