#ifndef COMBAT_BALANCE_CONFIG_H
#define COMBAT_BALANCE_CONFIG_H

/**
 * @file combat_balance_config.h
 * @brief Centralized Combat Balance Configuration
 *
 * This file contains all tunable constants for the Strength-based damage scaling system
 * and related combat mechanics. All scaling values are controlled here for easy balancing.
 *
 * ARCHITECTURE CHOICE: OPTION B (Additive Base Damage)
 * - STR bonus added to base damage alongside weapon delay bonus
 * - Both bonuses scale through existing percent modifier pipeline
 * - Predictable math, safer scaling, clear separation of concerns
 * - See game_design/STR_IMPLEMENTATION_PLAN.md for full rationale
 *
 * DESIGN PHILOSOPHY:
 * - All "magic numbers" related to STR scaling live here
 * - Easy to adjust if damage numbers are too high/low
 * - Single source of truth for balance discussions
 * - Version-controlled history of balance changes
 *
 * REFERENCED BY:
 * - game_design/stats/STR.md
 * - game_design/STR_IMPLEMENTATION_PLAN.md
 */

namespace CombatBalance {

	//=============================================================================
	// STRENGTH DAMAGE SCALING - CORE FORMULA
	//=============================================================================
	//
	// ENABLE/DISABLE: Use RuleB(Combat, UseNewStrDamageFormula) to toggle the new
	// STR damage system on/off. Set in common/ruletypes.h or via /reload rules.
	//
	// TESTING WORKFLOW:
	// 1. Set Combat:UseNewStrDamageFormula = false - verify legacy system works
	// 2. Set Combat:UseNewStrDamageFormula = true - test new system
	// 3. Compare damage logs between old and new
	// 4. Tune constants below based on results
	//=============================================================================

	/**
	 * @brief STR Damage Formula: Strength * ((Level^STR_LEVEL_EXPONENT) / STR_LEVEL_DIVISOR)
	 *
	 * TWO-KNOB SYSTEM for independent control of early vs late game scaling:
	 *
	 * STR_LEVEL_DIVISOR: Overall damage magnitude (higher = less STR damage at all levels)
	 * STR_LEVEL_EXPONENT: Curve shape (1.0 = linear, >1.0 = accelerating, <1.0 = decelerating)
	 *
	 * Current: Divisor = 20.0f, Exponent = 1.0f (linear scaling)
	 *
	 * DESIGN GOALS (based on user testing scenarios):
	 * - Level 1: Weapon dominates (4-5 dmg), STR adds small bonus (racial difference)
	 *   - Ogre (150 STR): ~12 total damage
	 *   - Gnome (80 STR): ~8 total damage
	 * - Level 60: Balanced (24 wpn + ~180 STR = ~200 total)
	 * - Level 70: STR stronger (120 wpn + ~4900 STR = ~5000 total)
	 * - Level 100: Weapon matters (2258 wpn + ~5000 STR = ~7300 total, 30/70 split)
	 *
	 * TUNING STRATEGY:
	 * 1. Adjust STR_LEVEL_DIVISOR first - changes damage at ALL levels proportionally
	 * 2. If low levels feel too weak OR high levels too strong, adjust STR_LEVEL_EXPONENT
	 *    - Exponent > 1.0: STR grows faster at high levels (quadratic/cubic growth)
	 *    - Exponent < 1.0: STR grows slower at high levels (sqrt-style deceleration)
	 * 3. Use STR_MIN_LEVEL_MULTIPLIER to set floor for level 1
	 */
	constexpr float STR_LEVEL_DIVISOR = 40.0f;

	/**
	 * @brief Exponent for level scaling in STR damage formula
	 *
	 * Formula: level_factor = (Level^EXPONENT) / DIVISOR
	 *
	 * Current: 1.0f (linear - simple and predictable)
	 *
	 * Alternative values:
	 * - 1.0f: Linear (Level 10 = 10x, Level 100 = 100x scaling difference)
	 * - 0.5f: Square root (Level 100 only ~3.16x Level 10, gentler high-level growth)
	 * - 1.5f: Accelerating (Level 100 = 31.6x Level 10, explosive high-level scaling)
	 *
	 * WARNING: Values != 1.0 create non-linear progression. Only adjust if
	 * linear scaling can't achieve desired balance between low and high levels.
	 *
	 * WHEN TO USE:
	 * - If level 1-20 feels too weak BUT level 70-100 feels too strong: try 1.2-1.5
	 * - If level 1-20 feels good BUT level 100 is out of control: try 0.7-0.9
	 */
	constexpr float STR_LEVEL_EXPONENT = 1.0f;

	/**
	 * @brief Minimum level multiplier for STR damage
	 *
	 * Formula: level_factor = max((Level^EXPONENT) / DIVISOR, STR_MIN_LEVEL_MULTIPLIER)
	 *
	 * Prevents STR from doing zero damage at very low levels.
	 * Ensures that even level 1 characters benefit from high STR items.
	 *
	 * Current: 0.05f (5% effectiveness minimum, matches Level 1 with divisor 20)
	 *
	 * Example: Level 1 with 200 STR gets at least 10 bonus damage (200 * 0.05)
	 *
	 * RACIAL BALANCE AT LEVEL 1:
	 * - Ogre (150 STR): 150 * 0.05 = 7.5 bonus  -> 4 weapon + 1 delay + 7.5 = ~12 total
	 * - Gnome (80 STR): 80 * 0.05 = 4.0 bonus   -> 4 weapon + 1 delay + 4.0 = ~9 total
	 * - STR difference creates meaningful racial choice even at level 1!
	 */
	constexpr float STR_MIN_LEVEL_MULTIPLIER = 0.05f;

	//=============================================================================
	// STRENGTH DAMAGE SCALING - DIMINISHING RETURNS (OPTIONAL)
	//=============================================================================

	/**
	 * @brief Enable smooth diminishing returns for very high STR values
	 *
	 * TRUE: STR uses curved formula above STR_DIMINISHING_START
	 * FALSE: STR is purely linear (simpler, recommended for initial tuning)
	 *
	 * Current: FALSE (start with linear, add curve only if needed)
	 *
	 * Use Case: If top-end geared players reach 2000+ STR and damage feels
	 * too explosive even after adjusting STR_LEVEL_DIVISOR, enable this to
	 * gently reduce returns on ultra-high STR without hard caps.
	 */
	constexpr bool ENABLE_STR_DIMINISHING_RETURNS = false;

	/**
	 * @brief STR value at which diminishing returns begin
	 *
	 * Below this value: STR is fully effective (linear)
	 * Above this value: Each point of STR is worth slightly less
	 *
	 * Current: 500.0f (well-geared level 60+ threshold)
	 *
	 * Rationale: Lets early/mid game feel linear and strong, only curves
	 * the extreme top end to prevent runaway scaling.
	 */
	constexpr float STR_DIMINISHING_START = 500.0f;

	/**
	 * @brief Power curve for diminishing returns (< 1.0 = diminishing)
	 *
	 * Formula: effective_str = START + (STR - START)^POWER
	 *
	 * Current: 0.85f (moderate curve - each point ~85% as effective above threshold)
	 *
	 * Examples:
	 * - 0.5f: Very aggressive curve (sqrt), high STR heavily diminished
	 * - 0.85f: Gentle curve, still rewarding but prevents exponential growth
	 * - 1.0f: No curve (linear) - equivalent to disabled
	 *
	 * TUNING: Adjust this if ENABLE_STR_DIMINISHING_RETURNS = true and the
	 * curve feels too harsh or too gentle.
	 */
	constexpr float STR_DIMINISHING_POWER = 0.85f;

	//=============================================================================
	// STRENGTH DAMAGE SCALING - RACIAL BALANCE (OPTIONAL)
	//=============================================================================

	/**
	 * @brief Enable racial STR compression to reduce early-game gaps
	 *
	 * TRUE: High-STR races (Ogre, Troll) and low-STR races (Gnome, Halfling)
	 *       have their differences slightly compressed for early game balance
	 * FALSE: Racial STR differences translate directly to damage (full fantasy)
	 *
	 * Current: FALSE (let racial differences shine, disable if level 1 gaps too large)
	 *
	 * Use Case: If an Ogre warrior (150 STR) hits 3x harder than a Gnome warrior
	 * (60 STR) at level 1 and this feels unfair, enable this to compress the gap
	 * to ~1.5x while still preserving racial identity.
	 */
	constexpr bool ENABLE_RACIAL_STR_COMPRESSION = false;

	/**
	 * @brief Baseline STR value for racial compression
	 *
	 * The "average" STR value around which racial differences are compressed.
	 * Races near this value are unaffected; extreme races are pulled toward it.
	 *
	 * Current: 100.0f (typical level 1-10 STR for mid-tier races)
	 */
	constexpr float STR_BASELINE_FOR_BALANCE = 100.0f;

	/**
	 * @brief Scalar that compresses racial STR differences
	 *
	 * Formula: adjusted_str = BASELINE + (STR - BASELINE) * SCALAR
	 *
	 * Current: 0.7f (reduces gaps by 30%)
	 *
	 * Examples:
	 * - Ogre with 150 STR: 100 + (150 - 100) * 0.7 = 135 effective STR
	 * - Gnome with 60 STR: 100 + (60 - 100) * 0.7 = 72 effective STR
	 * - Gap reduced from 90 STR to 63 STR (30% compression)
	 *
	 * TUNING: Lower values = more compression (races feel similar)
	 *         Higher values = less compression (races stay distinct)
	 */
	constexpr float STR_RACIAL_BONUS_SCALAR = 0.7f;

	//=============================================================================
	// STRENGTH DAMAGE SCALING - HAND/PET PENALTIES
	//=============================================================================

	/**
	 * @brief Offhand STR damage penalty multiplier
	 *
	 * Offhand attacks deal reduced STR damage to balance dual-wield.
	 * Current: 0.5f (50% of main hand STR damage)
	 *
	 * Rationale: Prevents dual-wield from being 2x better than 2H weapons.
	 * Dual-wield already gets double hits; they shouldn't also get double STR bonus.
	 *
	 * TUNING: Raise if dual-wield feels weak, lower if it dominates 2H.
	 */
	constexpr float OFFHAND_STR_PENALTY = 0.5f;

	/**
	 * @brief Pet STR damage scalar (separate from STR inheritance)
	 *
	 * This is applied AFTER pet inherits owner's STR via PET_STR_INHERITANCE.
	 * Allows independent tuning of pet damage vs. pet stats.
	 *
	 * Current: 1.0f (pets get full damage from inherited STR)
	 *
	 * Formula: pet_str_bonus = (owner_STR * PET_STR_INHERITANCE) * level_mult * PET_STR_DAMAGE_SCALAR
	 *
	 * Use Case: If pets inherit 50% of owner STR but still feel too strong,
	 * lower this to 0.8f instead of changing inheritance (keeps pet stats clean).
	 */
	constexpr float PET_STR_DAMAGE_SCALAR = 1.0f;

	/**
	 * @brief NPC STR damage scalar (global override)
	 *
	 * Multiplier for NPC use of STR damage formulas.
	 * Current: 0.0f (NPCs don't get STR scaling at all)
	 *
	 * Rationale: Player power fantasy - NPCs have fixed damage, players scale infinitely.
	 *
	 * TUNING: Set to 1.0f if you want NPCs to scale with STR like players.
	 *         Set to 0.5f if you want NPCs to scale but weaker than players.
	 */
	constexpr float NPC_STR_DAMAGE_SCALAR = 0.0f;	//=============================================================================
	// WEAPON DELAY SCALING (Kept for weapon identity, applies to all melee)
	//=============================================================================

	/**
	 * @brief Whether to include weapon delay bonuses alongside STR damage
	 *
	 * TRUE: Slower weapons get both STR bonus + delay bonus (higher damage)
	 * FALSE: Only STR bonus applies (weapon delay only affects attack speed)
	 *
	 * Current: TRUE (slow weapons should hit harder - weapon choice matters)
	 *
	 * ARCHITECTURE NOTE (Option B):
	 * BaseDamage = WeaponDamage + DelayBonus + StrengthBonus
	 * FinalDamage = BaseDamage * (all existing percent modifiers)
	 *
	 * Both bonuses are additive to base, then everything scales together.
	 */
	constexpr bool ENABLE_WEAPON_DELAY_BONUS = true;

	/**
	 * @brief Global scalar for weapon delay bonuses
	 *
	 * Multiplies the final delay bonus for easy tuning.
	 * Current: 1.0f (full delay bonus as calculated)
	 *
	 * Use Case: If STR scaling feels good but delay bonuses make slow weapons
	 * too dominant, lower this to 0.8f instead of changing the divisors.
	 *
	 * Formula: final_delay_bonus = calculated_delay_bonus * DELAY_BONUS_GLOBAL_SCALAR
	 */
	constexpr float DELAY_BONUS_GLOBAL_SCALAR = 1.0f;	/**
	 * @brief Delay bonus formula divisor for 1H weapons
	 *
	 * Formula: (delay - DELAY_THRESHOLD_1H) / DELAY_BONUS_DIVISOR_1H
	 * Higher value = less bonus from slow weapons
	 *
	 * Current: 3.0f (matches original EQ formula)
	 */
	constexpr float DELAY_BONUS_DIVISOR_1H = 3.0f;

	/**
	 * @brief Delay threshold for 1H weapon bonus to apply
	 *
	 * Weapons faster than this get no delay bonus.
	 * Current: 40 (aligns with original EQ formula)
	 */
	constexpr int DELAY_THRESHOLD_1H = 40;

	/**
	 * @brief Delay bonus formula divisor for 2H weapons
	 *
	 * 2H weapons get a more favorable bonus curve than 1H.
	 * Current: 2.5f (slightly better scaling than 1H)
	 */
	constexpr float DELAY_BONUS_DIVISOR_2H = 2.5f;

	/**
	 * @brief Delay threshold for 2H weapon bonus to apply
	 *
	 * Current: 30 (2H weapons start bonus earlier than 1H)
	 */
	constexpr int DELAY_THRESHOLD_2H = 30;

	//=============================================================================
	// PET STR INHERITANCE
	//=============================================================================

	/**
	 * @brief Percentage of owner's STR inherited by pets
	 *
	 * Controls how much of a summoner's STR affects their pet's damage.
	 * Current: 0.5f (50% inheritance)
	 *
	 * Rationale:
	 * - Prevents pets from being stronger than players
	 * - Makes STR a valuable stat for summoners without overshadowing INT/WIS
	 * - Follows precedent from AC/ATK pet bonuses
	 */
	constexpr float PET_STR_INHERITANCE = 0.5f;

	//=============================================================================
	// CLASS-SPECIFIC FEATURES: TANKS
	//=============================================================================

	/**
	 * @brief STR divisor for tank interrupt immunity
	 *
	 * Formula: Channeling + (STR / TANK_INTERRUPT_DIVISOR)
	 * Lower value = more interrupt immunity from STR
	 *
	 * Current: 4.0f (1000 STR = +250 channeling)
	 *
	 * Applies to: Warrior, Paladin, Shadowknight
	 */
	constexpr float TANK_INTERRUPT_DIVISOR = 4.0f;

	//=============================================================================
	// CLASS-SPECIFIC FEATURES: ROGUES
	//=============================================================================

	/**
	 * @brief Whether backstab includes STR damage bonus
	 *
	 * TRUE: Backstab = (Weapon + STR_Bonus) * Backstab_Mult
	 * FALSE: Backstab = Weapon * Backstab_Mult (ignores STR)
	 *
	 * Current: TRUE (STR scales backstab massively at high levels)
	 */
	constexpr bool BACKSTAB_INCLUDES_STR = true;

	//=============================================================================
	// CLASS-SPECIFIC FEATURES: CASTERS
	//=============================================================================

	/**
	 * @brief STR divisor for stun resistance percentage
	 *
	 * Formula: Stun_Resist% = STR / CASTER_STUN_DIVISOR
	 * Lower value = more stun resistance from STR
	 *
	 * Current: 50.0f (1000 STR = 20% stun resist)
	 *
	 * Applies to: Wizard, Enchanter, Magician, Necromancer
	 */
	constexpr float CASTER_STUN_DIVISOR = 50.0f;

	//=============================================================================
	// CLASS-SPECIFIC FEATURES: MONKS
	//=============================================================================

	/**
	 * @brief STR multiplier for monk weight limit
	 *
	 * Formula: MaxWeight = STR * MONK_WEIGHT_MULTIPLIER
	 * Higher value = more weight capacity per STR point
	 *
	 * Current: 10.0f (1000 STR = 10,000 weight capacity)
	 */
	constexpr float MONK_WEIGHT_MULTIPLIER = 10.0f;

	//=============================================================================
	// CLASS-SPECIFIC FEATURES: BARDS
	//=============================================================================

	/**
	 * @brief STR divisor for bard song interrupt immunity
	 *
	 * Formula: Singing + (STR / BARD_INTERRUPT_DIVISOR)
	 * Lower value = more interrupt immunity from STR
	 *
	 * Current: 4.0f (same as tanks - 1000 STR = +250 singing)
	 */
	constexpr float BARD_INTERRUPT_DIVISOR = 4.0f;

	//=============================================================================
	// SPECIAL ATTACKS
	//=============================================================================

	/**
	 * @brief Whether special attacks (bash, kick, frenzy) include STR damage
	 *
	 * TRUE: Special attacks scale with STR (universal physical damage scaling)
	 * FALSE: Special attacks use old formulas (no STR scaling)
	 *
	 * Current: TRUE (all physical damage scales with STR for consistency)
	 */
	constexpr bool SPECIAL_ATTACKS_USE_STR = true;

	//=============================================================================
	// APPLICABILITY FILTERS
	//=============================================================================

	/**
	 * @brief Whether NPCs get STR-based damage bonuses
	 *
	 * TRUE: NPCs use STR scaling (balanced difficulty)
	 * FALSE: NPCs use old damage formulas (only players get STR scaling)
	 *
	 * Current: FALSE (only players and player pets get new bonuses)
	 *
	 * Rationale:
	 * - Prevents NPCs from one-shotting players
	 * - Player power fantasy (players scale higher than NPCs)
	 * - Easier to balance (control NPC damage via stats, not formulas)
	 */
	constexpr bool NPCS_USE_STR_SCALING = false;

	/**
	 * @brief Whether player pets get STR-based damage bonuses
	 *
	 * TRUE: Pets use STR scaling (via inheritance)
	 * FALSE: Pets use old damage formulas
	 *
	 * Current: TRUE (pets inherit owner's STR at PET_STR_INHERITANCE rate)
	 */
	constexpr bool PETS_USE_STR_SCALING = true;

	/**
	 * @brief Whether charmed NPCs get STR-based damage bonuses
	 *
	 * TRUE: Charmed NPCs benefit from STR scaling
	 * FALSE: Charmed NPCs keep their original damage
	 *
	 * Current: FALSE (charm already powerful, no need for STR scaling)
	 */
	constexpr bool CHARMED_NPCS_USE_STR_SCALING = false;

	//=============================================================================
	// DAMAGE MITIGATION INTERACTION
	//=============================================================================

	/**
	 * @brief Whether STR bonus damage is affected by armor class
	 *
	 * TRUE: STR damage reduced by target's AC (normal physical damage)
	 * FALSE: STR damage bypasses AC (always full damage)
	 *
	 * Current: TRUE (STR is raw damage, should be mitigated normally)
	 *
	 * Note: DEX may get AC penetration in future (skilled attacks bypass armor)
	 */
	constexpr bool STR_DAMAGE_AFFECTED_BY_AC = true;

} // namespace CombatBalance

#endif // COMBAT_BALANCE_CONFIG_H
