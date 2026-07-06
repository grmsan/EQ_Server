#pragma once

/**
 * @file combat_balance_runtime.h
 * @brief Runtime-reloadable combat balance value store.
 *
 * Phase 0 skeleton: API shape defined, no parser or combat integration yet.
 *
 * NOTE: The CombatBalanceKey enum is a preliminary set for Phase 0 API shape.
 * It does not claim complete final coverage. Phase 1 will finalize key names,
 * add contribution-channel scalars, and align naming with INI spelling convention.
 *
 * Architecture:
 *   Combat:UseHotfixableCombatBalance (default false) gates whether runtime values
 *   are consulted. When false, all getters return the provided fallback immediately.
 *
 *   Formula gate hierarchy (callers enforce this - this module does NOT check formula gates):
 *     1. Per-stat formula rule (UseNewStrDamageFormula, UseNewDexFormulas, etc.)
 *        -> if false, call site must use legacy formula and must NOT call these getters.
 *     2. Combat:UseHotfixableCombatBalance
 *        -> if false, getters return fallback (compile-time constant or rule value).
 *        -> if true,  getters return runtime-loaded value if present and valid,
 *                     otherwise fallback.
 *
 * This file is intentionally included by no combat call sites during Phase 0.
 * Combat integration begins in Phase 3.
 *
 * See: game_design/stats/COMBAT_BALANCE_HOT_RELOAD_PLAN.md
 *      zone/combat_balance_config.h  (compile-time fallback constants)
 */

#include <cstdint>
#include <string>

// ---------------------------------------------------------------------------
// Key enumerations
// These enum values are the hot-path identifiers passed to runtime getters.
// String names are used by the parser, dump, and status commands only.
// ---------------------------------------------------------------------------

enum class CombatBalanceKey : uint32_t {
	// STR damage formula scalars (Phase 3 integration)
	STR_LEVEL_DIVISOR = 0,
	STR_LEVEL_EXPONENT,
	STR_MIN_LEVEL_MULTIPLIER,
	ENABLE_STR_DIMINISHING_RETURNS,
	STR_DIMINISHING_START,
	STR_DIMINISHING_POWER,

	// STA formula scalars (Phase 5A integration)
	STA_HP_DIVISOR,
	STA_REGEN_DIVISOR,
	STA_MITIGATION_DIVISOR,

	// DEX formula scalars (Phase 5B integration)
	DEX_CRIT_DIVISOR,
	DEX_CRIT_MIN_DIVISOR,
	DEX_CRIT_OVERFLOW_SCALAR,
	DEX_BASE_CRIT_DMG_DIVISOR,
	DEX_TWINCAST_DIVISOR,
	DEX_BOW_LEVEL_DIVISOR,
	DEX_BOW_CRIT_DMG_DIVISOR,
	DEX_RESIST_PENETRATION_DIVISOR,

	// AGI formula scalars (Phase 5C integration)
	AGI_AVOID_DIVISOR,
	AGI_HASTE_DIVISOR,
	AGI_CAST_SPEED_DIVISOR,

	// INT/WIS SpellDmg/HealAmt extra bonus scalars (Phase 6 integration)
	INTWIS_EXTRA_SPELL_BONUS_STAT_DIVISOR,
	INTWIS_EXTRA_SPELL_BONUS_STAT_EXPONENT,
	INTWIS_EXTRA_SPELL_BONUS_MULTIPLIER_CAP,
	INTWIS_DAMAGE_SECONDARY_WIS_SCALAR,
	INTWIS_HEALING_SECONDARY_INT_SCALAR,

	// CHA formula scalars (Phase 7 integration)
	CHA_RARE_LOOT_DIVISOR,
	CHA_PET_HP_DIVISOR,
	CHA_PET_DMG_DIVISOR,
	CHA_DAMAGE_REDUCTION_DIVISOR,
	CHA_RESIST_PENETRATION_DIVISOR,
	CHA_CRIT_BONUS_DIVISOR,
	CHA_CHARM_RELIABILITY_DIVISOR,
	CHA_BARD_SONG_DIVISOR,

	KEY_COUNT // sentinel - must be last
};

// ---------------------------------------------------------------------------
// Curve profile enumerations
// One curve profile is associated with each stat's output formula.
// Curve profiles are loaded from [Curve.<NAME>] INI sections (Phase 1+).
// ---------------------------------------------------------------------------

enum class CombatBalanceCurve : uint32_t {
	STR_DAMAGE = 0,
	STA_HP,
	STA_MITIGATION,
	DEX_CRIT,
	DEX_PROC,
	AGI_AVOID,
	INT_SPELL_POWER,
	INT_COOLDOWN_REDUCTION,
	INT_MANA_EFFICIENCY,
	WIS_SPELL_MITIGATION,
	WIS_HEAL_POWER,
	WIS_CC_RESIST,
	INTWIS_SPELL_DAMAGE_BONUS,
	INTWIS_HEAL_AMOUNT_BONUS,
	CHA_RARE_LOOT,
	CHA_PET_POWER,
	CHA_DAMAGE_REDUCTION,
	CHA_RESIST_PENETRATION,
	CHA_CRIT_BONUS,
	CHA_CHARM_CC,
	CHA_BARD_SONG,

	CURVE_COUNT // sentinel - must be last
};

// ---------------------------------------------------------------------------
// Curve mode enum (used inside CurveProfile)
// ---------------------------------------------------------------------------

enum class CurveMode : uint8_t {
	Linear = 0,
	PiecewiseLinear,
	Power,
	Logistic
};

// ---------------------------------------------------------------------------
// CurveProfile
// Holds all runtime-configurable knobs for one stat-effect curve.
// Callers receive a copy; the runtime store owns the authoritative copy.
// Phase 0: fields defined for API shape. Parser populates them in Phase 1.
// ---------------------------------------------------------------------------

struct CurveProfile {
	CurveMode mode            = CurveMode::Linear;

	// Ramp shape
	float     curve_start     = 0.0f;    // stat threshold where accelerated scaling begins
	float     base_slope      = 1.0f;    // slope below curve_start
	float     ramp_slope      = 1.0f;    // slope above curve_start
	float     power           = 1.0f;    // exponent for Power mode
	float     logistic_mid    = 0.0f;    // midpoint for Logistic mode
	float     logistic_k      = 1.0f;    // steepness for Logistic mode

	// Soft/hard caps
	float     softcap_start   = 0.0f;    // 0 = disabled
	float     softcap_power   = 1.0f;    // compression exponent above softcap
	float     hardcap_value   = 0.0f;    // 0/negative = disabled
	float     min_output      = 0.0f;    // optional output floor

	// Contribution-channel weights (Phase 4+ wiring)
	float     base_weight     = 1.0f;
	float     item_weight     = 1.0f;
	float     spell_weight    = 1.0f;
	float     aa_weight       = 1.0f;

	// Low-base catch-up (Phase 4+ wiring)
	bool      catchup_enabled    = false;
	float     catchup_threshold  = 0.0f;
	float     catchup_slope      = 0.0f;

	// Final-output caps
	float     final_softcap_start  = 0.0f;
	float     final_softcap_power  = 1.0f;
	float     final_hardcap_value  = 0.0f;
};

// ---------------------------------------------------------------------------
// CombatBalanceRuntime
//
// Phase 0 responsibilities:
//   - Define the public API that combat call sites will use (Phase 3+).
//   - All getters return `fallback` unconditionally in this phase.
//   - Load() / Reload() are no-op stubs that log the event.
//
// Thread safety: single-threaded zone process; no locking required for now.
// ---------------------------------------------------------------------------

class CombatBalanceRuntime {
public:
	// -----------------------------------------------------------------------
	// Lifecycle - called during zone startup and by the GM command (Phase 2+).
	// -----------------------------------------------------------------------

	// Attempt to load/reload values from combat_balance.ini.
	// Phase 0: no-op stub; logs the load event.
	static void Load(const std::string& ini_path = "combat_balance.ini");

	// -----------------------------------------------------------------------
	// Value accessors - to be called only from inside new-formula paths.
	//
	// Resolution order (Phase 1+):
	//   1. If Combat:UseHotfixableCombatBalance is false  -> return fallback.
	//   2. If key present and valid in cache              -> return cached value.
	//   3. Otherwise                                      -> log fallback event, return fallback.
	//
	// Phase 0: all getters return fallback immediately.
	// -----------------------------------------------------------------------

	static float       GetFloat(CombatBalanceKey key, float fallback);
	static int         GetInt(CombatBalanceKey key, int fallback);
	static bool        GetBool(CombatBalanceKey key, bool fallback);
	static CurveProfile GetCurveProfile(CombatBalanceCurve key);

	// -----------------------------------------------------------------------
	// Status helpers - used by the GM command (Phase 2+).
	// -----------------------------------------------------------------------

	static int         GetLoadedKeyCount();
	static int         GetInvalidKeyCount();
	static bool        IsRuntimeActive();
	static std::string GetLastLoadPath();

private:
	// Internal helpers (Phase 1+ implementation)
	static void LogLoadEvent(const std::string& path, int loaded, int invalid, bool success);
	static void LogFallbackEvent(CombatBalanceKey key, const char* reason);
	static void LogParseWarning(const std::string& key_name, const std::string& raw_value, const char* reason);
};
