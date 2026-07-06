/**
 * @file combat_balance_runtime.cpp
 * @brief Runtime-reloadable combat balance value store - Phase 0 skeleton.
 *
 * Phase 0: All public methods are no-op stubs or return fallback values.
 *          No parser, no GM command, no combat call-site integration.
 *
 * Phase 1 will implement:
 *   - INI parser for [CombatBalance] and [Curve.*] sections.
 *   - Validated typed getters backed by an enum-keyed cache.
 *   - Atomic cache swap on reload (parse-then-swap, keep previous on failure).
 *
 * See: game_design/stats/COMBAT_BALANCE_HOT_RELOAD_PLAN.md
 */

#include "combat_balance_runtime.h"
#include "../common/eqemu_logsys.h"
#include "../common/rulesys.h"

// ---------------------------------------------------------------------------
// Phase 0 internal state - stubs only.
// Phase 1 will replace these with a validated cache structure.
// ---------------------------------------------------------------------------

static int  s_loaded_key_count  = 0;
static int  s_invalid_key_count = 0;
static bool s_load_attempted    = false;
static std::string s_last_load_path;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void CombatBalanceRuntime::Load(const std::string& ini_path)
{
	// Phase 0: no-op stub. Log that load was requested.
	s_last_load_path  = ini_path;
	s_load_attempted  = true;
	s_loaded_key_count  = 0;
	s_invalid_key_count = 0;

	// Log the load event so operators can confirm the module is present.
	// Phase 1 will replace this with real parse results.
	LogLoadEvent(ini_path, 0, 0, false /*not yet implemented*/);
}

// ---------------------------------------------------------------------------
// Value accessors - Phase 0 returns fallback unconditionally.
// Phase 1 will add: rule check, cache lookup, and validated return.
// ---------------------------------------------------------------------------

float CombatBalanceRuntime::GetFloat(CombatBalanceKey key, float fallback)
{
	// Phase 0 skeleton: rule check not yet wired to a live cache.
	// When Combat:UseHotfixableCombatBalance is true in Phase 1+ this will
	// consult the loaded cache. For now always return fallback.
	return fallback;
}

int CombatBalanceRuntime::GetInt(CombatBalanceKey key, int fallback)
{
	return fallback;
}

bool CombatBalanceRuntime::GetBool(CombatBalanceKey key, bool fallback)
{
	return fallback;
}

CurveProfile CombatBalanceRuntime::GetCurveProfile(CombatBalanceCurve key)
{
	// Returns a default-constructed CurveProfile (linear, slopes=1, caps disabled).
	// Phase 1 will populate these from [Curve.*] INI sections.
	return CurveProfile{};
}

// ---------------------------------------------------------------------------
// Status helpers
// ---------------------------------------------------------------------------

int CombatBalanceRuntime::GetLoadedKeyCount()
{
	return s_loaded_key_count;
}

int CombatBalanceRuntime::GetInvalidKeyCount()
{
	return s_invalid_key_count;
}

bool CombatBalanceRuntime::IsRuntimeActive()
{
	// Runtime is active when the rule is enabled AND at least one key is loaded.
	// Phase 0: always false because no keys are ever loaded yet.
	return RuleB(Combat, UseHotfixableCombatBalance) && s_loaded_key_count > 0;
}

std::string CombatBalanceRuntime::GetLastLoadPath()
{
	return s_last_load_path;
}

// ---------------------------------------------------------------------------
// Internal logging stubs
// These provide structured log points that Phase 1+ will emit real data from.
// ---------------------------------------------------------------------------

void CombatBalanceRuntime::LogLoadEvent(const std::string& path, int loaded, int invalid, bool success)
{
	if (success) {
		LogCombat(
			"[CombatBalanceRuntime] Loaded runtime config from '{}': {} keys loaded, {} invalid.",
			path, loaded, invalid
		);
	} else {
		// Phase 0: parser not yet implemented; log a skeleton notice.
		LogCombat(
			"[CombatBalanceRuntime] Load requested for '{}' (Phase 0 skeleton - parser not yet implemented). "
			"Runtime values are not active. All getters return compile-time fallbacks.",
			path
		);
	}
}

void CombatBalanceRuntime::LogFallbackEvent(CombatBalanceKey key, const char* reason)
{
	// Phase 1 will call this when a cache miss or invalid value forces fallback.
	LogCombat(
		"[CombatBalanceRuntime] Key {} falling back to compile-time constant: {}",
		static_cast<uint32_t>(key), reason ? reason : "unknown"
	);
}

void CombatBalanceRuntime::LogParseWarning(const std::string& key_name, const std::string& raw_value, const char* reason)
{
	// Phase 1 will call this for malformed or out-of-range INI values.
	LogCombat(
		"[CombatBalanceRuntime] Parse warning for key '{}' value '{}': {}",
		key_name, raw_value, reason ? reason : "unknown"
	);
}
