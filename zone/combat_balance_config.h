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

	//=============================================================================
	// STRENGTH - SPELL INTERRUPTION RESISTANCE
	//=============================================================================

	/**
	 * @brief Enable STR-based per-hit spell interruption resistance
	 *
	 * TRUE: Each melee hit while casting has a % chance to interrupt based on STR/level
	 * FALSE: Use legacy channeling skill system only
	 *
	 * Current: TRUE (enabled by default)
	 *
	 * Design: Per-hit interrupt chance modified by STR, Channeling skill, and level difference.
	 * Low-level mobs with high defender STR can have 0% interrupt chance (full immunity).
	 */
	constexpr bool ENABLE_STR_INTERRUPT_RESISTANCE = true;

	/**
	 * @brief Base interrupt chance per melee hit while casting
	 *
	 * Current: 5.0f (5% base chance before modifiers)
	 *
	 * This is the starting point before STR, Channeling, and level difference modify it.
	 * Small enough that single hits rarely interrupt, but multiple hits add up.
	 */
	constexpr float INTERRUPT_BASE_CHANCE_PER_HIT = 5.0f;

	/**
	 * @brief STR divisor for interrupt resistance
	 *
	 * Formula: interrupt_reduction = STR / DIVISOR
	 *
	 * Current: 100.0f (every 100 STR = -1% interrupt chance)
	 *
	 * Examples:
	 * - 200 STR: -2% interrupt chance
	 * - 800 STR: -8% interrupt chance
	 * - 1000 STR: -10% interrupt chance
	 */
	constexpr float STR_INTERRUPT_RESISTANCE_DIVISOR = 100.0f;

	/**
	 * @brief Level difference modifier for interrupt chance
	 *
	 * Formula: level_modifier = (attacker_level - defender_level) * MULTIPLIER
	 *
	 * Current: 1.5f (each level difference = ±1.5% interrupt chance)
	 *
	 * Examples:
	 * - Attacker 3 levels higher: +4.5% interrupt chance
	 * - Attacker 15 levels lower: -22.5% interrupt chance
	 *
	 * Design: Level difference is a major factor - low-level mobs rarely interrupt.
	 */
	constexpr float INTERRUPT_LEVEL_DIFFERENCE_MULTIPLIER = 1.5f;

	/**
	 * @brief Channeling skill divisor for interrupt resistance
	 *
	 * Formula: channeling_reduction = Channeling_Skill / DIVISOR
	 *
	 * Current: 50.0f (every 50 Channeling skill = -1% interrupt chance)
	 *
	 * Example: 250 Channeling skill = -5% interrupt chance
	 *
	 * This preserves the value of Channeling skill while adding STR contribution.
	 */
	constexpr float INTERRUPT_CHANNELING_DIVISOR = 50.0f;

	//=============================================================================
	// STRENGTH - STUN RESISTANCE
	//=============================================================================

	/**
	 * @brief Enable STR contribution to stun resistance
	 *
	 * TRUE: STR adds to both Frontal and Regular stun resist (stacks with items/AAs)
	 * FALSE: Only item/spell/AA bonuses apply
	 *
	 * Current: TRUE (enabled by default)
	 *
	 * Design: Two-layer system:
	 * 1. FrontalStunResist: Can reach 100% immunity from frontal attacks (medium STR)
	 * 2. StunResist: Asymptotic curve that never reaches 100% (for rear attacks)
	 */
	constexpr bool ENABLE_STR_STUN_RESIST = true;

	/**
	 * @brief STR divisor for FRONTAL stun resistance
	 *
	 * Formula: FrontalStunResist = min(STR / DIVISOR, 100)
	 *
	 * Current: 10.0f (every 10 STR = 1% frontal stun resist, caps at 100%)
	 *
	 * Examples:
	 * - 500 STR = 50% frontal resist
	 * - 1000 STR = 100% frontal resist (completely immune from front)
	 * - 2000 STR = 100% (capped)
	 *
	 * This allows medium-high STR characters to be immune to frontal stuns (bash from front).
	 * Rear attacks bypass this layer and go straight to regular StunResist check.
	 */
	constexpr float STR_FRONTAL_STUN_RESIST_DIVISOR = 10.0f;

	/**
	 * @brief STR scaling factor for REGULAR stun resistance (asymptotic curve)
	 *
	 * Formula: StunResist = 100 * (1 - e^(-STR / SCALE_FACTOR))
	 *
	 * Current: 2000.0f (controls how quickly resist approaches 100%)
	 *
	 * This creates a diminishing returns curve that NEVER reaches 100% resist.
	 * Used for rear attacks and as secondary layer if frontal resist fails.
	 *
	 * Examples:
	 * - 500 STR = 22% resist
	 * - 1000 STR = 39% resist
	 * - 2000 STR = 63% resist
	 * - 5000 STR = 92% resist
	 * - 10000 STR = 99.3% resist (asymptotically approaches 100%, never reaches it)
	 *
	 * TUNING: Lower value = resist ramps up faster but plateaus sooner
	 *         Higher value = resist grows slower but continues scaling longer
	 */
	constexpr float STR_STUN_RESIST_SCALE_FACTOR = 2000.0f;

	//=============================================================================
	// STAMINA SYSTEM CONFIGURATION
	//=============================================================================
	//
	// ENABLE/DISABLE: Use RuleB(Combat, UseNewStaminaFormula) to toggle the new
	// STA systems on/off. Set in common/ruletypes.h or via /reload rules.
	//
	// DESIGN PHILOSOPHY:
	// - Stamina is the "Sustainability Stat" - fuels combat through HP, regen, and mitigation
	// - All systems use LINEAR scaling for predictability (no exponentials)
	// - Conservative tuning prevents 50-70k HP pools
	// - AC×STA synergy for mitigation (both stats remain valuable)
	//=============================================================================

	//=============================================================================
	// STAMINA - HP SCALING (Iron Constitution)
	//=============================================================================

	/**
	 * @brief Enable STA-based HP scaling system
	 *
	 * TRUE: Use new linear HP formula with class multipliers
	 * FALSE: Use legacy HP formula
	 *
	 * Current: TRUE (enabled by default)
	 *
	 * Formula: BASE_HP + (BASE_HP_PER_LEVEL * Level * ClassMult) + (STA * Level * ClassMult / STA_DIVISOR)
	 */
	constexpr bool ENABLE_STA_HP_SCALING = true;

	/**
	 * @brief Base HP for all characters
	 *
	 * Current: 100 (survivable start for level 1)
	 */
	constexpr int STA_BASE_HP = 100;

	/**
	 * @brief Flat HP gained per level (before class multiplier)
	 *
	 * Current: 5 (conservative tuning)
	 *
	 * This provides smooth HP progression independent of STA investment.
	 * Lower value = more reliant on STA for HP growth
	 * Higher value = less reliant on STA, all classes get more base HP
	 */
	constexpr int STA_BASE_HP_PER_LEVEL = 5;

	/**
	 * @brief STA divisor for HP scaling
	 *
	 * Formula: (STA * Level * ClassMult) / STA_HP_DIVISOR
	 *
	 * Current: 10.0f (conservative - prevents massive HP pools)
	 *
	 * TUNING (Primary Knob):
	 * - Lower value (7-8): More HP from STA
	 * - Higher value (12-15): Less HP from STA
	 *
	 * Examples at L70 with 1000 STA, 1.5x class mult:
	 * - Divisor 10: 10,500 HP from STA
	 * - Divisor 7: 15,000 HP from STA
	 * - Divisor 15: 7,000 HP from STA
	 */
	constexpr float STA_HP_DIVISOR = 10.0f;

	/**
	 * @brief Global HP scalar (emergency tuning knob)
	 *
	 * Multiplies entire HP formula result.
	 * Current: 1.0f (no scaling)
	 *
	 * Use only for global adjustments - prefer tuning STA_HP_DIVISOR and BASE_HP_PER_LEVEL
	 */
	constexpr float STA_HP_LEVEL_SCALAR = 1.0f;

	/**
	 * @brief Class-specific HP multipliers
	 *
	 * Applied to both level scaling AND STA scaling portions of HP formula.
	 * Allows fine-tuning each class's HP progression independently.
	 *
	 * Current values:
	 * - Tanks (War/Pal/SK): 1.5x (highest HP)
	 * - Melee (Rog/Ber/Mnk/Rng/Brd/Bst): 1.2x (medium-high HP)
	 * - Priests (Clr/Dru/Shm): 1.0x (baseline)
	 * - Casters: 0.8x-1.3x (varies by class design)
	 *   - Necro: 1.3x (tanky caster)
	 *   - Wiz/Mag: 0.8x (squishy nukers)
	 *   - Enc: 0.9x (slightly less squishy)
	 */
	constexpr float STA_HP_WARRIOR_MULTIPLIER = 1.5f;
	constexpr float STA_HP_CLERIC_MULTIPLIER = 1.0f;
	constexpr float STA_HP_PALADIN_MULTIPLIER = 1.5f;
	constexpr float STA_HP_RANGER_MULTIPLIER = 1.2f;
	constexpr float STA_HP_SHADOWKNIGHT_MULTIPLIER = 1.5f;
	constexpr float STA_HP_DRUID_MULTIPLIER = 1.0f;
	constexpr float STA_HP_MONK_MULTIPLIER = 1.2f;
	constexpr float STA_HP_BARD_MULTIPLIER = 1.2f;
	constexpr float STA_HP_ROGUE_MULTIPLIER = 1.2f;
	constexpr float STA_HP_SHAMAN_MULTIPLIER = 1.0f;
	constexpr float STA_HP_NECROMANCER_MULTIPLIER = 1.3f;  // Higher HP for caster
	constexpr float STA_HP_WIZARD_MULTIPLIER = 0.8f;
	constexpr float STA_HP_MAGICIAN_MULTIPLIER = 0.8f;
	constexpr float STA_HP_ENCHANTER_MULTIPLIER = 0.9f;
	constexpr float STA_HP_BEASTLORD_MULTIPLIER = 1.2f;
	constexpr float STA_HP_BERSERKER_MULTIPLIER = 1.2f;

	//=============================================================================
	// STAMINA - REGENERATION (Undying Vitality)
	//=============================================================================

	/**
	 * @brief Enable STA-based HP regeneration
	 *
	 * TRUE: STA adds to HP regen (self-sufficiency)
	 * FALSE: Use legacy HP regen only
	 *
	 * Current: TRUE
	 *
	 * Formula: (STA * Level) / HP_REGEN_DIVISOR HP per tick
	 */
	constexpr bool ENABLE_STA_HP_REGEN = true;

	/**
	 * @brief STA divisor for HP regeneration
	 *
	 * Current: 20.0f (conservative - ~5% max HP per tick at end-game)
	 *
	 * Examples at L70 with 1000 STA:
	 * - Divisor 20: 3,500 HP/tick (~5% of 70k total HP)
	 * - Divisor 10: 7,000 HP/tick (~10% of 70k total HP)
	 * - Divisor 30: 2,333 HP/tick (~3.3% of 70k total HP)
	 *
	 * TUNING:
	 * - Too strong (god mode)? Increase divisor (20 → 30 → 40)
	 * - Too weak (always dying)? Decrease divisor (20 → 15 → 10)
	 */
	constexpr float STA_HP_REGEN_DIVISOR = 20.0f;

	/**
	 * @brief Enable STA-based mana regeneration
	 *
	 * TRUE: STA adds to mana regen for casters (resource sustainability)
	 * FALSE: Use legacy mana regen only
	 *
	 * Current: TRUE
	 *
	 * Formula: (STA * Level) / MANA_REGEN_DIVISOR mana per tick (casters only)
	 */
	constexpr bool ENABLE_STA_MANA_REGEN = true;

	/**
	 * @brief STA divisor for mana regeneration
	 *
	 * Current: 50.0f (moderate - ~3-7% max mana per tick)
	 *
	 * Examples at L70:
	 * - 500 STA: 700 mana/tick (~3.5% of 20k mana pool)
	 * - 1000 STA: 1,400 mana/tick (~7% of 20k mana pool)
	 *
	 * TUNING:
	 * - Trivializes mana management? Increase divisor (50 → 70 → 100)
	 * - Can't sustain rotations? Decrease divisor (50 → 40 → 30)
	 */
	constexpr float STA_MANA_REGEN_DIVISOR = 50.0f;

	/**
	 * @brief Enable STA-based endurance regeneration
	 *
	 * TRUE: STA adds to endurance regen (discipline uptime)
	 * FALSE: Use legacy endurance regen only
	 *
	 * Current: TRUE
	 *
	 * Formula: (STA * Level) / ENDURANCE_REGEN_DIVISOR endurance per tick
	 */
	constexpr bool ENABLE_STA_ENDURANCE_REGEN = true;

	/**
	 * @brief STA divisor for endurance regeneration
	 *
	 * Current: 20.0f (high regen for near-permanent discipline uptime)
	 *
	 * Examples at L70:
	 * - 1000 STA: 3,500 End/tick
	 * - 1500 STA: 5,250 End/tick
	 *
	 * TUNING: Adjust based on discipline endurance costs
	 */
	constexpr float STA_ENDURANCE_REGEN_DIVISOR = 20.0f;

	//=============================================================================
	// STAMINA - DAMAGE MITIGATION (Thick Skin) - Hybrid AC×STA Model
	//=============================================================================

	/**
	 * @brief Enable Hybrid AC×STA mitigation system
	 *
	 * TRUE: STA amplifies AC-based mitigation (synergy between defensive stats)
	 * FALSE: Use legacy AC-only mitigation
	 *
	 * Current: TRUE
	 *
	 * Philosophy: AC is primary mitigation source, STA amplifies AC effectiveness.
	 * STA never produces mitigation alone - only enhances what AC already provides.
	 * This ensures both AC and STA remain valuable (no stat obsolescence).
	 *
	 * Formula:
	 * 1. ac_mitigation = ComputeACMitigation() (existing system)
	 * 2. sta_ceiling = (STA * Level) / STA_MIT_DIVISOR
	 * 3. sta_bonus = min(sta_ceiling, ac_mitigation) if STA_MATCH_AC enabled
	 * 4. total_mitigation = ac_mitigation + sta_bonus
	 * 5. total_mitigation = min(total_mitigation, COMBAT_MAX_MITIGATION_PERCENT)
	 */
	constexpr bool ENABLE_HYBRID_AC_STA_MITIGATION = true;

	/**
	 * @brief STA divisor for mitigation ceiling calculation
	 *
	 * Current: 2000.0f (conservative - prevents easy 90% mitigation)
	 *
	 * Examples at L70:
	 * - 1000 STA: 35% ceiling
	 * - 1500 STA: 52.5% ceiling
	 * - 500 STA: 17.5% ceiling
	 *
	 * TUNING (Primary STA Mitigation Knob):
	 * - Too strong (tanks unkillable)? Increase divisor (2000 → 3000)
	 * - Too weak (no noticeable benefit)? Decrease divisor (2000 → 1500)
	 */
	constexpr float STA_MIT_DIVISOR = 2000.0f;

	/**
	 * @brief Absolute maximum STA contribution to mitigation
	 *
	 * Current: 50.0f (50% max, even at extreme STA values)
	 *
	 * Safety cap to prevent weird edge cases at 5000+ STA.
	 */
	constexpr float STA_MIT_MAX_PERCENT = 50.0f;

	/**
	 * @brief Whether STA mitigation bonus is limited by AC mitigation
	 *
	 * TRUE: STA can only amplify existing AC mitigation (enforces synergy)
	 * FALSE: STA provides full ceiling regardless of AC
	 *
	 * Current: TRUE (recommended - ensures AC remains valuable)
	 *
	 * Example (TRUE):
	 * - AC provides 15% mitigation
	 * - STA ceiling is 35%
	 * - STA bonus limited to 15% (matches AC)
	 * - Total: 15% + 15% = 30%
	 *
	 * Example (FALSE):
	 * - AC provides 15% mitigation
	 * - STA ceiling is 35%
	 * - STA bonus gets full 35%
	 * - Total: 15% + 35% = 50%
	 */
	constexpr bool STA_MATCH_AC = true;

	/**
	 * @brief Maximum mitigation AC alone can provide
	 *
	 * Current: 50.0f (AC caps at 50% before STA amplification)
	 *
	 * TUNING (Primary AC Mitigation Knob):
	 * - AC too strong? Lower cap (50 → 40 → 35)
	 * - AC too weak? Raise cap (50 → 60)
	 */
	constexpr float AC_MITIGATION_HARD_CAP = 50.0f;

	/**
	 * @brief Global maximum total mitigation (AC + STA combined)
	 *
	 * Current: 90.0f (prevents immunity, always take 10% minimum damage)
	 *
	 * TUNING (Safety Valve):
	 * - Damage still too trivial? Lower cap (90 → 85 → 80)
	 * - Players dying too fast? Raise cap (90 → 95)
	 */
	constexpr float COMBAT_MAX_MITIGATION_PERCENT = 90.0f;

	/**
	 * @brief Global scalar for AC mitigation effectiveness
	 *
	 * Current: 1.0f (no scaling)
	 *
	 * Multiplies AC's mitigation contribution before combining with STA.
	 * Use to globally tune AC effectiveness without changing caps.
	 */
	constexpr float AC_MITIGATION_SOFTCAP_SCALAR = 1.0f;

	//=============================================================================
	// STAMINA - ENVIRONMENTAL RESISTANCE
	//=============================================================================

	/**
	 * @brief Enable STA-based poison resistance bonus
	 *
	 * TRUE: STA adds to poison resist
	 * FALSE: Use legacy resist only
	 *
	 * Current: TRUE
	 *
	 * Formula: Poison_Resist += STA / POISON_RESIST_DIVISOR
	 */
	constexpr bool ENABLE_STA_POISON_RESIST = true;

	/**
	 * @brief STA divisor for poison resistance
	 *
	 * Current: 5.0f (every 5 STA = +1 resist)
	 *
	 * Example: 1000 STA = +200 poison resist
	 */
	constexpr float STA_POISON_RESIST_DIVISOR = 5.0f;

	/**
	 * @brief Enable STA-based disease resistance bonus
	 *
	 * TRUE: STA adds to disease resist
	 * FALSE: Use legacy resist only
	 *
	 * Current: TRUE
	 *
	 * Formula: Disease_Resist += STA / DISEASE_RESIST_DIVISOR
	 */
	constexpr bool ENABLE_STA_DISEASE_RESIST = true;

	/**
	 * @brief STA divisor for disease resistance
	 *
	 * Current: 5.0f (every 5 STA = +1 resist)
	 *
	 * Example: 1000 STA = +200 disease resist
	 */
	constexpr float STA_DISEASE_RESIST_DIVISOR = 5.0f;

	/**
	 * @brief Enable STA-based breath timer bonus
	 *
	 * TRUE: STA extends underwater breath timer
	 * FALSE: Use legacy breath timer only
	 *
	 * Current: TRUE
	 *
	 * Formula: Breath_Timer += STA / BREATH_BONUS_DIVISOR seconds
	 */
	constexpr bool ENABLE_STA_BREATH_BONUS = true;

	/**
	 * @brief STA divisor for breath timer bonus
	 *
	 * Current: 10.0f (every 10 STA = +1 second underwater)
	 *
	 * Example: 1000 STA = +100 seconds breath
	 */
	constexpr float STA_BREATH_BONUS_DIVISOR = 10.0f;

} // namespace CombatBalance

#endif // COMBAT_BALANCE_CONFIG_H
