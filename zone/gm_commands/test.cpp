#include "../client.h"
#include "../zonedb.h"
#include "../zone.h"
#include "../aa.h"

#include "../../common/classes.h"
#include "../../common/rulesys.h"
#include "../../common/seperator.h"
#include "../../common/spdat.h"
#include "../../common/strings.h"

#include <fmt/format.h>

#include <algorithm>
#include <atomic>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <ctime>
#include <unordered_map>
#include <unordered_set>
#include <vector>

extern Zone *zone;

namespace {
enum class TestState {
	Pass,
	Fail,
	Skip
};

struct TestResult {
	TestState state;
	std::string details;
};

struct TestDefinition {
	int id;
	const char *name;
	TestResult (*runner)(Client *c);
};

struct PendingClientProbe {
	uint32 test_id = 0;
	uint32 nonce = 0;
	uint32 started_epoch = 0;
	uint32 required_field_mask = 0;
	uint32 expected_class_mask = 0;
	bool check_expected_class_mask = false;
	int32 expected_hp_max = -1;
	bool check_expected_hp_max = false;
	int32 expected_mana_max = -1;
	bool check_expected_mana_max = false;
	int32 expected_end_max = -1;
	bool check_expected_end_max = false;
	uint8 mutation_class_id = 0;
	bool mutation_was_add = false;
	bool mutation_needs_restore = false;
};

enum ProbeFieldMask : uint32 {
	ProbeFieldNone       = 0,
	ProbeFieldGameState  = 1u << 0,
	ProbeFieldSpawnId    = 1u << 1,
	ProbeFieldTargetId   = 1u << 2,
	ProbeFieldHp         = 1u << 3,
	ProbeFieldMana       = 1u << 4,
	ProbeFieldEndurance  = 1u << 5,
	ProbeFieldClassMask  = 1u << 6,
};

using PendingProbeMapByNonce = std::unordered_map<uint32, PendingClientProbe>;
static std::unordered_map<uint32, PendingProbeMapByNonce> g_pending_client_probes;
static std::atomic<uint32> g_probe_nonce_counter { 1000 };

inline TestResult Pass(std::string details)
{
	return { TestState::Pass, std::move(details) };
}

inline TestResult Fail(std::string details)
{
	return { TestState::Fail, std::move(details) };
}

inline TestResult Skip(std::string details)
{
	return { TestState::Skip, std::move(details) };
}

uint32 NextProbeNonce()
{
	const uint32 now = static_cast<uint32>(std::time(nullptr) & 0xFFFF);
	const uint32 seq = ++g_probe_nonce_counter;
	return ((now << 16) ^ seq);
}

int32 ClampProbeInt32(int64 value)
{
	if (value > static_cast<int64>(INT32_MAX)) {
		return INT32_MAX;
	}
	if (value < static_cast<int64>(INT32_MIN)) {
		return INT32_MIN;
	}

	return static_cast<int32>(value);
}

bool ParseUnsignedArg(const char *arg, uint32 &value);
bool ParseSignedArg(const char *arg, int32 &value);
bool ParseSignedPairArg(const char *arg, int32 &left, int32 &right);

uint16 SafeArgCount(const Seperator *sep)
{
	if (!sep) {
		return 0;
	}

	return std::min<uint16>(sep->argnum, sep->GetMaxArgNum());
}

const char *GetArgIfPresent(const Seperator *sep, uint16 index)
{
	if (!sep || index > sep->GetMaxArgNum() || !sep->arg) {
		return nullptr;
	}

	return sep->arg[index];
}

bool ParseUnsignedArgAt(const Seperator *sep, uint16 index, uint32 &value)
{
	return ParseUnsignedArg(GetArgIfPresent(sep, index), value);
}

bool ParseSignedArgAt(const Seperator *sep, uint16 index, int32 &value)
{
	return ParseSignedArg(GetArgIfPresent(sep, index), value);
}

bool ParseSignedPairArgAt(const Seperator *sep, uint16 index, int32 &left, int32 &right)
{
	return ParseSignedPairArg(GetArgIfPresent(sep, index), left, right);
}

bool ParseUnsignedArg(const char *arg, uint32 &value)
{
	if (!arg || !arg[0]) {
		return false;
	}

	const std::string input = arg;
	for (const char ch : input) {
		if (ch < '0' || ch > '9') {
			return false;
		}
	}

	try {
		const auto parsed = std::stoull(input);
		if (parsed > std::numeric_limits<uint32>::max()) {
			return false;
		}

		value = static_cast<uint32>(parsed);
		return true;
	}
	catch (std::exception &) {
		return false;
	}
}

bool ParseSignedArg(const char *arg, int32 &value)
{
	if (!arg || !arg[0]) {
		return false;
	}

	const std::string input = arg;
	size_t pos = 0;
	if (input[0] == '-') {
		if (input.size() == 1) {
			return false;
		}
		pos = 1;
	}

	for (; pos < input.size(); ++pos) {
		if (input[pos] < '0' || input[pos] > '9') {
			return false;
		}
	}

	value = Strings::ToInt(input);
	return true;
}

bool ParseSignedPairArg(const char *arg, int32 &left, int32 &right)
{
	if (!arg || !arg[0]) {
		return false;
	}

	const std::string raw = arg;
	const auto delim_pos = raw.find(':');
	if (delim_pos == std::string::npos) {
		return false;
	}

	const std::string left_raw = raw.substr(0, delim_pos);
	const std::string right_raw = raw.substr(delim_pos + 1);
	return ParseSignedArg(left_raw.c_str(), left) && ParseSignedArg(right_raw.c_str(), right);
}

void RegisterPendingProbe(Client *c, const PendingClientProbe &pending)
{
	if (!c || pending.nonce == 0) {
		return;
	}

	auto &pending_by_nonce = g_pending_client_probes[c->CharacterID()];
	const uint32 now = static_cast<uint32>(std::time(nullptr));
	for (auto it = pending_by_nonce.begin(); it != pending_by_nonce.end();) {
		const auto &existing = it->second;
		if (existing.started_epoch > 0 && now > existing.started_epoch + 20) {
			it = pending_by_nonce.erase(it);
		}
		else {
			++it;
		}
	}

	pending_by_nonce[pending.nonce] = pending;
}

bool ValidatePendingProbe(Client *c, uint32 test_id, uint32 nonce, PendingClientProbe &pending_out)
{
	const auto character_it = g_pending_client_probes.find(c->CharacterID());
	if (character_it == g_pending_client_probes.end()) {
		c->Message(Chat::White, "TEST [%u] CLIENT FAIL | no pending probe for this character", test_id);
		return false;
	}

	auto &pending_by_nonce = character_it->second;
	const uint32 now = static_cast<uint32>(std::time(nullptr));
	for (auto it = pending_by_nonce.begin(); it != pending_by_nonce.end();) {
		const auto &existing = it->second;
		if (existing.started_epoch > 0 && now > existing.started_epoch + 20) {
			it = pending_by_nonce.erase(it);
		}
		else {
			++it;
		}
	}

	if (pending_by_nonce.empty()) {
		g_pending_client_probes.erase(character_it);
		c->Message(Chat::White, "TEST [%u] CLIENT FAIL | no pending probe for this character", test_id);
		return false;
	}

	const auto pending_it = pending_by_nonce.find(nonce);
	if (pending_it == pending_by_nonce.end()) {
		const auto &first_pending = pending_by_nonce.begin()->second;
		c->Message(
			Chat::White,
			"TEST [%u] CLIENT FAIL | nonce mismatch expected=%u got=%u (pending_count=%u)",
			test_id,
			first_pending.nonce,
			nonce,
			static_cast<uint32>(pending_by_nonce.size())
		);
		return false;
	}

	const PendingClientProbe pending = pending_it->second;
	if (pending.test_id != test_id) {
		pending_by_nonce.erase(pending_it);
		if (pending_by_nonce.empty()) {
			g_pending_client_probes.erase(character_it);
		}
		c->Message(
			Chat::White,
			"TEST [%u] CLIENT FAIL | test id mismatch for nonce=%u (expected_test=%u)",
			test_id,
			nonce,
			pending.test_id
		);
		return false;
	}

	if (pending.started_epoch > 0 && now > pending.started_epoch + 20) {
		pending_by_nonce.erase(pending_it);
		if (pending_by_nonce.empty()) {
			g_pending_client_probes.erase(character_it);
		}
		c->Message(Chat::White, "TEST [%u] CLIENT FAIL | probe response timeout (>20s)", test_id);
		return false;
	}

	pending_out = pending;
	pending_by_nonce.erase(pending_it);
	if (pending_by_nonce.empty()) {
		g_pending_client_probes.erase(character_it);
	}
	return true;
}

bool IsManaCapableClass(uint8 class_id)
{
	return IsCasterClass(class_id) || IsHybridClass(class_id);
}

bool SelectClassMutationCandidate(Client *c, bool prefer_mana_class, uint8 &class_id_out, bool &do_add_out)
{
	if (!c) {
		return false;
	}

	const int max_classes = RuleI(Custom, MulticlassMaxClasses);
	const int current_count = c->GetClassesCount();
	const bool can_add = (max_classes <= 0 || current_count < max_classes);

	auto find_add = [&](bool mana_only) -> uint8 {
		if (!can_add) {
			return 0;
		}

		for (uint8 cid = Class::Warrior; cid <= Class::Berserker; ++cid) {
			if (cid == c->GetClass() || c->HasClass(cid)) {
				continue;
			}
			if (mana_only && !IsManaCapableClass(cid)) {
				continue;
			}
			return cid;
		}

		return 0;
	};

	auto find_remove = [&](bool mana_only) -> uint8 {
		for (uint8 cid = Class::Warrior; cid <= Class::Berserker; ++cid) {
			if (cid == c->GetClass() || !c->HasClass(cid)) {
				continue;
			}
			if (mana_only && !IsManaCapableClass(cid)) {
				continue;
			}
			return cid;
		}

		return 0;
	};

	if (prefer_mana_class) {
		uint8 cid = find_add(true);
		if (cid != 0) {
			class_id_out = cid;
			do_add_out = true;
			return true;
		}

		cid = find_remove(true);
		if (cid != 0) {
			class_id_out = cid;
			do_add_out = false;
			return true;
		}
	}

	uint8 cid = find_add(false);
	if (cid != 0) {
		class_id_out = cid;
		do_add_out = true;
		return true;
	}

	cid = find_remove(false);
	if (cid != 0) {
		class_id_out = cid;
		do_add_out = false;
		return true;
	}

	return false;
}

void TryRestoreClassMutation(Client *c, const PendingClientProbe &pending)
{
	if (!c || !pending.mutation_needs_restore || pending.mutation_class_id == 0) {
		return;
	}

	const uint16 before_bits = c->GetClassesBitmask();
	bool restored = false;

	if (pending.mutation_was_add) {
		// Mutation added a class; restore by removing it.
		restored = c->RemoveExtraClass(pending.mutation_class_id);
	} else {
		// Mutation removed a class; restore by re-adding it.
		restored = c->AddExtraClass(pending.mutation_class_id);
	}

	const uint16 after_bits = c->GetClassesBitmask();
	c->Message(
		Chat::White,
		"TEST [%u] CLIENT %s | restore class_id=%u bits 0x%04X->0x%04X",
		pending.test_id,
		restored ? "INFO" : "WARN",
		static_cast<uint32>(pending.mutation_class_id),
		before_bits,
		after_bits
	);
}

bool HandleClientReplyCommand(Client *c, const Seperator *sep)
{
	// #test clientreply <test_id> <nonce> <game_state> <spawn_id> <target_id> <effective_mask> <hp_cur> <hp_max>
	if (!c || !sep) {
		return true;
	}

	const uint16 safe_arg_count = SafeArgCount(sep);
	if (safe_arg_count < 9) {
		c->Message(Chat::White, "TEST | clientreply error: expected 8 args, got %d", static_cast<int>(safe_arg_count) - 1);
		return true;
	}

	uint32 test_id = 0;
	uint32 nonce = 0;
	uint32 game_state = 0;
	uint32 spawn_id = 0;
	uint32 target_id = 0;
	uint32 effective_mask = 0;
	uint32 hp_cur = 0;
	uint32 hp_max = 0;

	if (!ParseUnsignedArgAt(sep, 2, test_id) ||
		!ParseUnsignedArgAt(sep, 3, nonce) ||
		!ParseUnsignedArgAt(sep, 4, game_state) ||
		!ParseUnsignedArgAt(sep, 5, spawn_id) ||
		!ParseUnsignedArgAt(sep, 6, target_id) ||
		!ParseUnsignedArgAt(sep, 7, effective_mask) ||
		!ParseUnsignedArgAt(sep, 8, hp_cur) ||
		!ParseUnsignedArgAt(sep, 9, hp_max)) {
		c->Message(Chat::White, "TEST | clientreply error: invalid numeric args");
		return true;
	}

	PendingClientProbe pending;
	if (!ValidatePendingProbe(c, test_id, nonce, pending)) {
		return true;
	}

	const bool pass = (spawn_id > 0) && (hp_max >= hp_cur) && (game_state > 0);
	if (!pass) {
		c->Message(
			Chat::White,
			"TEST [%u] CLIENT FAIL | gs=%u spawn=%u target=%u mask=0x%04X hp=%u/%u",
			test_id,
			game_state,
			spawn_id,
			target_id,
			effective_mask & 0xFFFF,
			hp_cur,
			hp_max
		);
		return true;
	}

	c->Message(
		Chat::White,
		"TEST [%u] CLIENT PASS | gs=%u spawn=%u target=%u mask=0x%04X hp=%u/%u",
		test_id,
		game_state,
		spawn_id,
		target_id,
		effective_mask & 0xFFFF,
		hp_cur,
		hp_max
	);

	return true;
}

bool HandleClientReplyV2Command(Client *c, const Seperator *sep)
{
	// Compact format to stay within command arg limits:
	// #test clientreplyv2 <test_id> <nonce> <response_mask> <game_state> <spawn_id> <target_id>
	//                     <hp_cur:hp_max> <mana_cur:mana_max> <end_cur:end_max> <class_mask>
	if (!c || !sep) {
		return true;
	}

	const uint16 safe_arg_count = SafeArgCount(sep);
	if (safe_arg_count < 11) {
		c->Message(Chat::White, "TEST | clientreplyv2 error: expected 10 args, got %d", static_cast<int>(safe_arg_count) - 1);
		return true;
	}

	uint32 test_id = 0;
	uint32 nonce = 0;
	uint32 response_mask = 0;
	int32 game_state = 0;
	uint32 spawn_id = 0;
	uint32 target_id = 0;
	int32 hp_cur = 0;
	int32 hp_max = 0;
	int32 mana_cur = 0;
	int32 mana_max = 0;
	int32 end_cur = 0;
	int32 end_max = 0;
	uint32 class_mask = 0;

	if (!ParseUnsignedArgAt(sep, 2, test_id) ||
		!ParseUnsignedArgAt(sep, 3, nonce) ||
		!ParseUnsignedArgAt(sep, 4, response_mask) ||
		!ParseSignedArgAt(sep, 5, game_state) ||
		!ParseUnsignedArgAt(sep, 6, spawn_id) ||
		!ParseUnsignedArgAt(sep, 7, target_id) ||
		!ParseSignedPairArgAt(sep, 8, hp_cur, hp_max) ||
		!ParseSignedPairArgAt(sep, 9, mana_cur, mana_max) ||
		!ParseSignedPairArgAt(sep, 10, end_cur, end_max) ||
		!ParseUnsignedArgAt(sep, 11, class_mask)) {
		c->Message(Chat::White, "TEST | clientreplyv2 error: invalid numeric args (pair format expected: cur:max)");
		return true;
	}

	PendingClientProbe pending;
	if (!ValidatePendingProbe(c, test_id, nonce, pending)) {
		return true;
	}

	const uint32 required_mask = pending.required_field_mask;
	if (required_mask != 0 && (response_mask & required_mask) != required_mask) {
		c->Message(
			Chat::White,
			"TEST [%u] CLIENT FAIL | response mask incomplete required=0x%04X got=0x%04X",
			test_id,
			required_mask & 0xFFFF,
			response_mask & 0xFFFF
		);
		return true;
	}

	bool pass = true;
	std::vector<std::string> failures;
	if ((required_mask & ProbeFieldGameState) && game_state <= 0) {
		pass = false;
		failures.emplace_back("game_state<=0");
	}
	if ((required_mask & ProbeFieldSpawnId) && spawn_id == 0) {
		pass = false;
		failures.emplace_back("spawn_id=0");
	}
	if ((required_mask & ProbeFieldTargetId) && target_id == 0) {
		pass = false;
		failures.emplace_back("target_id=0");
	}
	if ((required_mask & ProbeFieldHp) && (hp_cur < 0 || hp_max < hp_cur)) {
		pass = false;
		failures.emplace_back("hp invalid");
	}
	if ((required_mask & ProbeFieldMana) && (mana_cur < 0 || mana_max < mana_cur)) {
		pass = false;
		failures.emplace_back("mana invalid");
	}
	if ((required_mask & ProbeFieldEndurance) && (end_cur < 0 || end_max < end_cur)) {
		pass = false;
		failures.emplace_back("end invalid");
	}
	if ((required_mask & ProbeFieldClassMask) && class_mask == 0) {
		pass = false;
		failures.emplace_back("class_mask=0");
	}
	if (pending.check_expected_class_mask && class_mask != (pending.expected_class_mask & 0xFFFF)) {
		pass = false;
		failures.emplace_back(fmt::format(
			"class_mask mismatch got=0x{:04X} expected=0x{:04X}",
			class_mask & 0xFFFF,
			pending.expected_class_mask & 0xFFFF
		));
	}
	if (pending.check_expected_hp_max && hp_max != pending.expected_hp_max) {
		pass = false;
		failures.emplace_back(fmt::format(
			"hp_max mismatch got={} expected={}",
			hp_max,
			pending.expected_hp_max
		));
	}
	if (pending.check_expected_mana_max && mana_max != pending.expected_mana_max) {
		pass = false;
		failures.emplace_back(fmt::format(
			"mana_max mismatch got={} expected={}",
			mana_max,
			pending.expected_mana_max
		));
	}
	if (pending.check_expected_end_max && end_max != pending.expected_end_max) {
		pass = false;
		failures.emplace_back(fmt::format(
			"end_max mismatch got={} expected={}",
			end_max,
			pending.expected_end_max
		));
	}

	if (!pass) {
		c->Message(
			Chat::White,
			"TEST [%u] CLIENT FAIL | %s | mask=0x%04X gs=%d spawn=%u target=%u hp=%d/%d mana=%d/%d end=%d/%d class=0x%04X",
			test_id,
			Strings::Join(failures, ", ").c_str(),
			response_mask & 0xFFFF,
			game_state,
			spawn_id,
			target_id,
			hp_cur,
			hp_max,
			mana_cur,
			mana_max,
			end_cur,
			end_max,
			class_mask & 0xFFFF
		);
		TryRestoreClassMutation(c, pending);
		return true;
	}

	c->Message(
		Chat::White,
		"TEST [%u] CLIENT PASS | mask=0x%04X gs=%d spawn=%u target=%u hp=%d/%d mana=%d/%d end=%d/%d class=0x%04X",
		test_id,
		response_mask & 0xFFFF,
		game_state,
		spawn_id,
		target_id,
		hp_cur,
		hp_max,
		mana_cur,
		mana_max,
		end_cur,
		end_max,
		class_mask & 0xFFFF
	);
	TryRestoreClassMutation(c, pending);

	return true;
}

const std::vector<TestDefinition> &GetTests();

std::vector<int> BuildPackSmoke()
{
	return { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
}

std::vector<int> BuildPackCombat()
{
	return { 6, 12, 13, 14, 15 };
}

std::vector<int> BuildPackAutomated()
{
	std::vector<int> ids;
	ids.reserve(GetTests().size());
	for (const auto &t : GetTests()) {
		ids.push_back(t.id);
	}
	return ids;
}

void InsertPack(std::set<int> &selected, const std::vector<int> &ids, int min_id, int max_id)
{
	for (const auto id : ids) {
		if (id >= min_id && id <= max_id) {
			selected.insert(id);
		}
	}
}

bool IsPureIntCasterMulticlass(uint32 bits, uint32 int_caster_mask)
{
	const bool has_int_caster = (bits & int_caster_mask) != 0;
	const bool has_non_int_class = (bits & ~int_caster_mask) != 0;
	return has_int_caster && !has_non_int_class;
}

TestResult TestMulticlassRuleEnabled(Client *)
{
	if (RuleB(Custom, MulticlassingEnabled)) {
		return Pass("Custom:MulticlassingEnabled=true");
	}

	return Fail("Custom:MulticlassingEnabled=false");
}

TestResult TestDynamicAATimerRuleEnabled(Client *)
{
	if (RuleB(Custom, UseDynamicAATimers)) {
		return Pass("Custom:UseDynamicAATimers=true");
	}

	return Fail("Custom:UseDynamicAATimers=false");
}

TestResult TestBaseClassBitPresent(Client *c)
{
	const uint32 bits = c->GetClassesBits();
	const uint32 base_bit = GetPlayerClassBit(c->GetClass());
	if (base_bit == 0) {
		return Fail(fmt::format("invalid base class [{}]", static_cast<int>(c->GetClass())));
	}

	if ((bits & base_bit) == 0) {
		return Fail(fmt::format("classes_bits=0x{:08X} missing base_bit=0x{:04X}", bits, base_bit));
	}

	return Pass(fmt::format("classes_bits=0x{:08X} includes base_bit=0x{:04X}", bits, base_bit));
}

TestResult TestClassCountWithinLimit(Client *c)
{
	const int max_classes = RuleI(Custom, MulticlassMaxClasses);
	const int classes_count = c->GetClassesCount();

	if (max_classes <= 0) {
		return Skip(fmt::format("MulticlassMaxClasses={} (limit disabled)", max_classes));
	}

	if (classes_count > max_classes) {
		return Fail(fmt::format("classes_count={} exceeds MulticlassMaxClasses={}", classes_count, max_classes));
	}

	return Pass(fmt::format("classes_count={} within MulticlassMaxClasses={}", classes_count, max_classes));
}

TestResult TestGestaltBucketConsistency(Client *c)
{
	const uint32 bits = c->GetClassesBits();
	const uint32 base_bit = GetPlayerClassBit(c->GetClass());
	const std::string raw = c->GetBucket("GestaltClasses");

	if (raw.empty()) {
		if ((bits & base_bit) != 0) {
			return Skip(fmt::format("GestaltClasses bucket missing; fallback classes_bits=0x{:08X}", bits));
		}

		return Fail("GestaltClasses bucket missing and base bit not present in classes_bits");
	}

	const uint32 bucket_bits = Strings::ToUnsignedInt(raw, 0);
	const uint32 expected = (bucket_bits | base_bit);
	if (expected != bits) {
		return Fail(fmt::format(
			"GestaltClasses={} => expected_bits=0x{:08X}, runtime_bits=0x{:08X}",
			raw,
			expected,
			bits
		));
	}

	return Pass(fmt::format("GestaltClasses={} matches runtime_bits=0x{:08X}", raw, bits));
}

TestResult TestHasClassBitmaskConsistency(Client *c)
{
	const uint32 bits = c->GetClassesBits();
	int mismatch_count = 0;
	int first_mismatch_class = 0;

	for (uint8 class_id = Class::Warrior; class_id <= Class::Berserker; ++class_id) {
		const bool expected = (bits & GetPlayerClassBit(class_id)) != 0;
		const bool actual = c->HasClass(class_id);
		if (expected != actual) {
			++mismatch_count;
			if (first_mismatch_class == 0) {
				first_mismatch_class = class_id;
			}
		}
	}

	if (mismatch_count > 0) {
		return Fail(fmt::format(
			"HasClass mismatch_count={} first_class_id={}",
			mismatch_count,
			first_mismatch_class
		));
	}

	return Pass("HasClass(class_id) is consistent with GetClassesBits()");
}

TestResult TestLegacyBucketConsistency(Client *c)
{
	const uint32 bits = c->GetClassesBits();
	const uint32 base_bit = GetPlayerClassBit(c->GetClass());
	const std::string gestalt_raw = c->GetBucket("GestaltClasses");
	const std::string legacy_raw = c->GetBucket("multiclass.classes_bitmask");

	if (legacy_raw.empty()) {
		return Skip("legacy bucket multiclass.classes_bitmask not present");
	}

	const uint32 legacy_bits = Strings::ToUnsignedInt(legacy_raw, 0);
	if (!gestalt_raw.empty()) {
		const uint32 gestalt_bits = Strings::ToUnsignedInt(gestalt_raw, 0);
		if (legacy_bits != gestalt_bits) {
			return Fail(fmt::format(
				"legacy_bits={} differs from GestaltClasses={}",
				legacy_bits,
				gestalt_bits
			));
		}
		return Pass(fmt::format("legacy_bits={} matches GestaltClasses", legacy_bits));
	}

	const uint32 expected = (legacy_bits | base_bit);
	if (expected != bits) {
		return Fail(fmt::format(
			"legacy_bits={} => expected_bits=0x{:08X}, runtime_bits=0x{:08X}",
			legacy_bits,
			expected,
			bits
		));
	}

	return Pass(fmt::format("legacy_bits={} aligns with runtime_bits=0x{:08X}", legacy_bits, bits));
}

TestResult TestPlayerProfileClassesHydrated(Client *c)
{
	if (!RuleB(Custom, MulticlassingEnabled)) {
		return Skip("MulticlassingEnabled=false");
	}

	const uint32 bits = c->GetClassesBits();
	const uint32 profile_bits = c->GetPP().classes;
	const uint32 base_bit = GetPlayerClassBit(c->GetClass());

	if (profile_bits == 0) {
		return Fail(fmt::format("PlayerProfile.classes=0 while runtime_bits=0x{:08X}", bits));
	}

	if ((profile_bits | base_bit) != bits) {
		return Fail(fmt::format(
			"PlayerProfile.classes=0x{:08X} disagrees with runtime_bits=0x{:08X}",
			profile_bits,
			bits
		));
	}

	return Pass(fmt::format("PlayerProfile.classes=0x{:08X} matches runtime state", profile_bits));
}

TestResult TestNonDiscTimerTransform(Client *)
{
	if (!RuleB(Custom, MulticlassingEnabled)) {
		return Skip("MulticlassingEnabled=false");
	}

	int checked = 0;
	int failures = 0;
	int first_fail_spell_id = -1;

	for (int spell_id = 0; spell_id < SPDAT_RECORDS; ++spell_id) {
		if (!IsValidSpell(spell_id)) {
			continue;
		}

		if (spells[spell_id].is_discipline) {
			continue;
		}

		++checked;
		if (spells[spell_id].timer_id != -1) {
			++failures;
			if (first_fail_spell_id < 0) {
				first_fail_spell_id = spell_id;
			}
		}
	}

	if (checked == 0) {
		return Skip("no non-discipline spells checked");
	}

	if (failures > 0) {
		return Fail(fmt::format(
			"{} non-discipline spells do not have timer_id=-1 (first spell_id={})",
			failures,
			first_fail_spell_id
		));
	}

	return Pass(fmt::format("checked {} non-discipline spells with timer_id=-1", checked));
}

TestResult TestGroupClientPetTargetTransform(Client *)
{
	if (!RuleB(Custom, MulticlassingEnabled)) {
		return Skip("MulticlassingEnabled=false");
	}

	int remaining = 0;
	for (int spell_id = 0; spell_id < SPDAT_RECORDS; ++spell_id) {
		if (!IsValidSpell(spell_id)) {
			continue;
		}

		if (spells[spell_id].is_discipline) {
			continue;
		}

		if (spells[spell_id].target_type == ST_GroupClientAndPet) {
			++remaining;
		}
	}

	if (remaining > 0) {
		return Fail(fmt::format("{} non-discipline spells still target ST_GroupClientAndPet", remaining));
	}

	return Pass("no non-discipline spells remain with ST_GroupClientAndPet target");
}

TestResult TestGuildProjectionQueryShape(Client *c)
{
	const auto query = fmt::format(
		"SELECT c.`id`, c.`name`, c.`class`, c.`level`, db.`value` AS `class_bitmask` "
		"FROM `character_data` AS c "
		"LEFT JOIN `guild_members` AS g ON c.`id` = g.`char_id` "
		"LEFT JOIN `data_buckets` AS db ON c.`id` = db.`character_id` AND db.`key` = 'GestaltClasses' "
		"WHERE c.`id` = {} LIMIT 1",
		c->CharacterID()
	);

	auto results = database.QueryDatabase(query);
	if (!results.Success()) {
		return Fail("guild projection query failed");
	}

	if (results.RowCount() == 0) {
		return Fail("guild projection query returned 0 rows");
	}

	auto row = results.begin();
	const std::string class_bitmask = row[4] ? row[4] : "";
	if (!class_bitmask.empty()) {
		return Pass(fmt::format("guild projection query ok (class_bitmask={})", class_bitmask));
	}

	return Skip("guild projection query ok (no GestaltClasses row for this character)");
}

TestResult TestMnemonicRetentionAAOverride(Client *c)
{
	if (!RuleB(Custom, MulticlassingEnabled)) {
		return Skip("MulticlassingEnabled=false");
	}

	if (!zone) {
		return Skip("zone pointer unavailable");
	}

	auto *ability = zone->GetAlternateAdvancementAbility(aaMnemonicRetention);
	if (!ability || !ability->first) {
		return Skip("aaMnemonicRetention ability/rank not available in this data set");
	}

	if (!c->CanUseAlternateAdvancementRank(ability->first)) {
		return Fail("CanUseAlternateAdvancementRank failed for aaMnemonicRetention first rank");
	}

	return Pass("CanUseAlternateAdvancementRank passes for aaMnemonicRetention");
}

TestResult TestProcParityPreconditions(Client *c)
{
	if (!RuleB(Custom, MultipleTwoHandedProcs)) {
		return Fail("Custom:MultipleTwoHandedProcs=false");
	}

	auto results = database.QueryDatabase(
		"SELECT "
		"SUM(CASE WHEN itemtype IN (1,4,35) AND proceffect > 0 THEN 1 ELSE 0 END) AS two_h_proc_items, "
		"SUM(CASE WHEN itemtype = 5 AND proceffect > 0 THEN 1 ELSE 0 END) AS bow_proc_items "
		"FROM items"
	);

	if (!results.Success() || results.RowCount() == 0) {
		return Fail("failed to query items table for 2H/bow proc coverage");
	}

	auto row = results.begin();
	const int two_h_proc_items = Strings::ToInt(row[0] ? row[0] : "0");
	const int bow_proc_items = Strings::ToInt(row[1] ? row[1] : "0");

	if (two_h_proc_items <= 0) {
		return Fail("no 2H items with proceffect > 0 found in items table");
	}

	if (bow_proc_items <= 0) {
		return Fail("no bow items with proceffect > 0 found in items table");
	}

	return Pass(fmt::format(
		"2H_proc_items={} bow_proc_items={} OneProcPerWeapon={} MultipleTwoHandedProcs=true",
		two_h_proc_items,
		bow_proc_items,
		RuleB(Combat, OneProcPerWeapon) ? "true" : "false"
	));
}

TestResult TestPetBagCoverage(Client *c)
{
	if (!RuleB(Custom, EnablePetBags)) {
		return Fail("Custom:EnablePetBags=false");
	}

	const std::map<int, std::vector<int>> class_to_bag_map = {
		{Class::ShadowKnight, {899980}},
		{Class::Druid, {899981}},
		{Class::Bard, {899983}},
		{Class::Shaman, {899984}},
		{Class::Necromancer, {899985, 17727}},
		{Class::Magician, {899986, 900000}},
		{Class::Enchanter, {899987, 17726}},
		{Class::Beastlord, {899988, 17725}},
	};

	std::unordered_set<int> unique_bag_ids;
	for (const auto &entry : class_to_bag_map) {
		for (const auto bag_id : entry.second) {
			unique_bag_ids.insert(bag_id);
		}
	}

	std::vector<int> bag_ids(unique_bag_ids.begin(), unique_bag_ids.end());
	std::sort(bag_ids.begin(), bag_ids.end());

	std::vector<std::string> bag_id_strings;
	bag_id_strings.reserve(bag_ids.size());
	for (const auto bag_id : bag_ids) {
		bag_id_strings.emplace_back(std::to_string(bag_id));
	}

	const std::string in_clause = Strings::Join(bag_id_strings, ",");
	auto item_results = database.QueryDatabase(
		fmt::format("SELECT COUNT(*) FROM items WHERE id IN ({})", in_clause)
	);
	if (!item_results.Success() || item_results.RowCount() == 0) {
		return Fail("failed to query items table for pet bag ids");
	}

	auto item_row = item_results.begin();
	const int item_count = Strings::ToInt(item_row[0] ? item_row[0] : "0");
	if (item_count < static_cast<int>(bag_ids.size())) {
		return Fail(fmt::format(
			"pet bag items missing in DB (found={} expected={})",
			item_count,
			bag_ids.size()
		));
	}

	auto merchant_results = database.QueryDatabase(
		fmt::format(
			"SELECT COUNT(*) FROM merchantlist "
			"WHERE merchantid IN (52099, 382051, 394174) AND item IN ({})",
			in_clause
		)
	);
	if (!merchant_results.Success() || merchant_results.RowCount() == 0) {
		return Fail("failed to query merchantlist for pet bag rows");
	}

	auto merchant_row = merchant_results.begin();
	const int merchant_rows = Strings::ToInt(merchant_row[0] ? merchant_row[0] : "0");
	if (merchant_rows <= 0) {
		return Fail("no pet bag rows found on expected merchants (52099, 382051, 394174)");
	}

	int owned_pet_classes = 0;
	int active_bags = 0;
	for (const auto &entry : class_to_bag_map) {
		if (!c->HasClass(entry.first)) {
			continue;
		}

		++owned_pet_classes;
		if (c->GetActivePetBagSlot(entry.first) >= 0) {
			++active_bags;
		}
	}

	if (owned_pet_classes > 0 && active_bags == 0) {
		return Skip(fmt::format(
			"pet bag system configured; no active class pet bag found in inventory/bank for owned pet classes ({})",
			owned_pet_classes
		));
	}

	return Pass(fmt::format(
		"pet bag config ok (ids={}, merchant_rows={}, owned_pet_classes={}, active_bags={})",
		bag_ids.size(),
		merchant_rows,
		owned_pet_classes,
		active_bags
	));
}

TestResult TestFuryOfMagicPureCasterGate(Client *c)
{
	if (!RuleB(Custom, MulticlassingEnabled)) {
		return Skip("MulticlassingEnabled=false");
	}

	if (!zone) {
		return Skip("zone pointer unavailable");
	}

	auto *ability = zone->GetAlternateAdvancementAbility(aaFuryofMagic);
	if (!ability || !ability->first) {
		return Skip("aaFuryofMagic ability/rank not available in this data set");
	}

	AA::Rank *restricted_rank = ability->first;
	while (restricted_rank && !(restricted_rank->id > 772 && restricted_rank->id <= 4751)) {
		restricted_rank = restricted_rank->next;
	}

	if (!restricted_rank) {
		return Skip("no Fury of Magic rank in restricted (rank 6+) id range found");
	}

	const int expansion = RuleI(Expansion, CurrentExpansion);
	if (restricted_rank->expansion > expansion) {
		return Skip(fmt::format(
			"restricted Fury rank expansion [{}] exceeds current expansion [{}]",
			restricted_rank->expansion,
			expansion
		));
	}

	if (c->GetLevel() < restricted_rank->level_req) {
		return Skip(fmt::format(
			"client level [{}] below restricted Fury rank level requirement [{}]",
			c->GetLevel(),
			restricted_rank->level_req
		));
	}

	uint32 int_caster_mask = 0;
	for (uint8 class_id = Class::Warrior; class_id <= Class::Berserker; ++class_id) {
		if (IsINTCasterClass(class_id)) {
			int_caster_mask |= GetPlayerClassBit(class_id);
		}
	}

	const uint32 bits = c->GetClassesBits();
	const bool expected_allowed = IsPureIntCasterMulticlass(bits, int_caster_mask);
	const bool actual_allowed = c->CanUseAlternateAdvancementRank(restricted_rank);

	if (actual_allowed != expected_allowed) {
		return Fail(fmt::format(
			"Fury rank6+ gate mismatch (expected={} actual={} classes_bits=0x{:08X} int_caster_mask=0x{:08X} rank_id={})",
			expected_allowed ? "true" : "false",
			actual_allowed ? "true" : "false",
			bits,
			int_caster_mask,
			restricted_rank->id
		));
	}

	return Pass(fmt::format(
		"Fury rank6+ gate matches pure INT-caster policy (classes_bits=0x{:08X}, rank_id={})",
		bits,
		restricted_rank->id
	));
}

TestResult TestClientDllRoundTripProbe(Client *c)
{
	if (!c) {
		return Fail("client pointer unavailable");
	}

	if (c->ClientVersion() != EQ::versions::ClientVersion::RoF2) {
		return Skip("client probe is only available for RoF2 clients with the custom DLL");
	}

	const uint32 nonce = NextProbeNonce();
	constexpr uint32 required_mask =
		ProbeFieldGameState |
		ProbeFieldSpawnId |
		ProbeFieldHp |
		ProbeFieldMana |
		ProbeFieldEndurance |
		ProbeFieldClassMask;
	PendingClientProbe pending {};
	pending.test_id = 16;
	pending.nonce = nonce;
	pending.started_epoch = static_cast<uint32>(std::time(nullptr));
	pending.required_field_mask = required_mask;
	RegisterPendingProbe(c, pending);

	c->SendEdgeTestProbe(16, nonce, required_mask);

	return Skip(fmt::format(
		"client probe dispatched nonce={} mask=0x{:04X} (awaiting DLL callback via #test clientreplyv2)",
		nonce,
		required_mask & 0xFFFF
	));
}

TestResult TestClientDllClassMaskMutationRoundTrip(Client *c)
{
	if (!c) {
		return Fail("client pointer unavailable");
	}

	if (!RuleB(Custom, MulticlassingEnabled)) {
		return Skip("MulticlassingEnabled=false");
	}

	if (c->ClientVersion() != EQ::versions::ClientVersion::RoF2) {
		return Skip("class-mask mutation probe is only available for RoF2 clients with the custom DLL");
	}

	uint8 class_id = 0;
	bool do_add = false;
	if (!SelectClassMutationCandidate(c, false, class_id, do_add)) {
		return Skip("no eligible class found to mutate (already base-only with no free slots or no removable classes)");
	}

	const bool mutated = do_add ? c->AddExtraClass(class_id) : c->RemoveExtraClass(class_id);
	if (!mutated) {
		return Fail(fmt::format(
			"failed to {} class_id={} ({})",
			do_add ? "add" : "remove",
			static_cast<int>(class_id),
			GetClassIDName(class_id, c->GetLevel())
		));
	}

	const uint32 before_bits = c->GetClassesBits() & 0xFFFF;
	const uint32 expected_mask = c->GetClassesBits() & 0xFFFF;
	const uint32 nonce = NextProbeNonce();
	constexpr uint32 required_mask = ProbeFieldClassMask | ProbeFieldGameState | ProbeFieldSpawnId;
	PendingClientProbe pending {};
	pending.test_id = 17;
	pending.nonce = nonce;
	pending.started_epoch = static_cast<uint32>(std::time(nullptr));
	pending.required_field_mask = required_mask;
	pending.expected_class_mask = expected_mask;
	pending.check_expected_class_mask = true;
	pending.mutation_class_id = class_id;
	pending.mutation_was_add = do_add;
	pending.mutation_needs_restore = true;
	RegisterPendingProbe(c, pending);

	c->SendEdgeTestProbe(17, nonce, required_mask);

	return Skip(fmt::format(
		"class mask mutation probe dispatched nonce={} op={} class_id={}({}) expected_mask=0x{:04X} before_mask=0x{:04X}; state auto-restores after callback",
		nonce,
		do_add ? "add" : "remove",
		static_cast<int>(class_id),
		GetClassIDName(class_id, c->GetLevel()),
		expected_mask & 0xFFFF,
		before_bits & 0xFFFF
	));
}

TestResult TestClientDllManaMutationRoundTrip(Client *c)
{
	if (!c) {
		return Fail("client pointer unavailable");
	}

	if (!RuleB(Custom, MulticlassingEnabled)) {
		return Skip("MulticlassingEnabled=false");
	}

	if (c->ClientVersion() != EQ::versions::ClientVersion::RoF2) {
		return Skip("mana mutation probe is only available for RoF2 clients with the custom DLL");
	}

	uint8 class_id = 0;
	bool do_add = false;
	if (!SelectClassMutationCandidate(c, true, class_id, do_add)) {
		return Skip("no eligible class found for mana-focused mutation");
	}

	const int32 before_mana_max = ClampProbeInt32(c->GetMaxMana());
	const bool mutated = do_add ? c->AddExtraClass(class_id) : c->RemoveExtraClass(class_id);
	if (!mutated) {
		return Fail(fmt::format(
			"failed to {} class_id={} ({})",
			do_add ? "add" : "remove",
			static_cast<int>(class_id),
			GetClassIDName(class_id, c->GetLevel())
		));
	}

	const int32 expected_mana_max = ClampProbeInt32(c->GetMaxMana());
	const uint32 expected_mask = c->GetClassesBits() & 0xFFFF;
	const uint32 nonce = NextProbeNonce();
	constexpr uint32 required_mask =
		ProbeFieldClassMask |
		ProbeFieldMana |
		ProbeFieldGameState |
		ProbeFieldSpawnId;

	PendingClientProbe pending {};
	pending.test_id = 18;
	pending.nonce = nonce;
	pending.started_epoch = static_cast<uint32>(std::time(nullptr));
	pending.required_field_mask = required_mask;
	pending.expected_class_mask = expected_mask;
	pending.check_expected_class_mask = true;
	pending.expected_mana_max = expected_mana_max;
	pending.check_expected_mana_max = true;
	pending.mutation_class_id = class_id;
	pending.mutation_was_add = do_add;
	pending.mutation_needs_restore = true;
	RegisterPendingProbe(c, pending);

	c->SendEdgeTestProbe(18, nonce, required_mask);

	return Skip(fmt::format(
		"mana mutation probe dispatched nonce={} op={} class_id={}({}) expected_mask=0x{:04X} mana_max {}->{}; state auto-restores after callback",
		nonce,
		do_add ? "add" : "remove",
		static_cast<int>(class_id),
		GetClassIDName(class_id, c->GetLevel()),
		expected_mask & 0xFFFF,
		before_mana_max,
		expected_mana_max
	));
}

TestResult TestClientDllSnapshotParityRoundTrip(Client *c)
{
	if (!c) {
		return Fail("client pointer unavailable");
	}

	if (c->ClientVersion() != EQ::versions::ClientVersion::RoF2) {
		return Skip("snapshot parity probe is only available for RoF2 clients with the custom DLL");
	}

	const uint32 expected_mask = c->GetClassesBits() & 0xFFFF;
	const int32 expected_hp_max = ClampProbeInt32(c->GetMaxHP());
	const int32 expected_mana_max = ClampProbeInt32(c->GetMaxMana());
	const int32 expected_end_max = ClampProbeInt32(c->GetMaxEndurance());

	const uint32 nonce = NextProbeNonce();
	constexpr uint32 required_mask =
		ProbeFieldClassMask |
		ProbeFieldHp |
		ProbeFieldMana |
		ProbeFieldEndurance |
		ProbeFieldGameState |
		ProbeFieldSpawnId;

	PendingClientProbe pending {};
	pending.test_id = 19;
	pending.nonce = nonce;
	pending.started_epoch = static_cast<uint32>(std::time(nullptr));
	pending.required_field_mask = required_mask;
	pending.expected_class_mask = expected_mask;
	pending.check_expected_class_mask = true;
	pending.expected_hp_max = expected_hp_max;
	pending.check_expected_hp_max = true;
	pending.expected_mana_max = expected_mana_max;
	pending.check_expected_mana_max = true;
	pending.expected_end_max = expected_end_max;
	pending.check_expected_end_max = true;
	RegisterPendingProbe(c, pending);

	c->SendEdgeTestProbe(19, nonce, required_mask);

	return Skip(fmt::format(
		"snapshot parity probe dispatched nonce={} expected mask=0x{:04X} hp_max={} mana_max={} end_max={}; awaiting DLL callback",
		nonce,
		expected_mask & 0xFFFF,
		expected_hp_max,
		expected_mana_max,
		expected_end_max
	));
}

TestResult TestClientDllPersistedClassMaskParity(Client *c)
{
	if (!c) {
		return Fail("client pointer unavailable");
	}

	if (!RuleB(Custom, MulticlassingEnabled)) {
		return Skip("MulticlassingEnabled=false");
	}

	if (c->ClientVersion() != EQ::versions::ClientVersion::RoF2) {
		return Skip("persisted class-mask probe is only available for RoF2 clients with the custom DLL");
	}

	const uint32 runtime_bits = c->GetClassesBits() & 0xFFFF;
	const uint32 profile_bits = c->GetPP().classes & 0xFFFF;
	const uint32 gestalt_bits = Strings::ToUnsignedInt(c->GetBucket("GestaltClasses"), 0) & 0xFFFF;
	const uint32 legacy_bits = Strings::ToUnsignedInt(c->GetBucket("multiclass.classes_bitmask"), 0) & 0xFFFF;

	if (runtime_bits == 0 || profile_bits == 0 || gestalt_bits == 0 || legacy_bits == 0) {
		return Fail(fmt::format(
			"persisted source missing runtime=0x{:04X} profile=0x{:04X} gestalt=0x{:04X} legacy=0x{:04X}",
			runtime_bits,
			profile_bits,
			gestalt_bits,
			legacy_bits
		));
	}

	if (runtime_bits != profile_bits || runtime_bits != gestalt_bits || runtime_bits != legacy_bits) {
		return Fail(fmt::format(
			"persisted source mismatch runtime=0x{:04X} profile=0x{:04X} gestalt=0x{:04X} legacy=0x{:04X}",
			runtime_bits,
			profile_bits,
			gestalt_bits,
			legacy_bits
		));
	}

	const uint32 nonce = NextProbeNonce();
	constexpr uint32 required_mask = ProbeFieldClassMask | ProbeFieldGameState | ProbeFieldSpawnId;
	PendingClientProbe pending {};
	pending.test_id = 20;
	pending.nonce = nonce;
	pending.started_epoch = static_cast<uint32>(std::time(nullptr));
	pending.required_field_mask = required_mask;
	pending.expected_class_mask = runtime_bits;
	pending.check_expected_class_mask = true;
	RegisterPendingProbe(c, pending);

	c->SendEdgeTestProbe(20, nonce, required_mask);

	return Skip(fmt::format(
		"persisted class-mask probe dispatched nonce={} runtime/profile/gestalt/legacy all 0x{:04X}; awaiting DLL callback",
		nonce,
		runtime_bits
	));
}

const std::vector<TestDefinition> &GetTests()
{
	static const std::vector<TestDefinition> tests = {
		{ 1, "Rule: MulticlassingEnabled", TestMulticlassRuleEnabled },
		{ 2, "Rule: UseDynamicAATimers", TestDynamicAATimerRuleEnabled },
		{ 3, "Class Bits: Base Class Bit Present", TestBaseClassBitPresent },
		{ 4, "Class Bits: Count Within Max", TestClassCountWithinLimit },
		{ 5, "Bucket: GestaltClasses Consistency", TestGestaltBucketConsistency },
		{ 6, "Class API: HasClass Consistency", TestHasClassBitmaskConsistency },
		{ 7, "Bucket: Legacy Key Consistency", TestLegacyBucketConsistency },
		{ 8, "Profile: PlayerProfile.classes Hydrated", TestPlayerProfileClassesHydrated },
		{ 9, "Spells: Non-Disc Timer Transform", TestNonDiscTimerTransform },
		{ 10, "Spells: GroupClientAndPet Transform", TestGroupClientPetTargetTransform },
		{ 11, "Guild: Projection Query Shape", TestGuildProjectionQueryShape },
		{ 12, "AA: Mnemonic Retention Override", TestMnemonicRetentionAAOverride },
		{ 13, "Combat: 2H/Bow Proc Preconditions", TestProcParityPreconditions },
		{ 14, "Pet Bags: Rule + DB/Merchant Coverage", TestPetBagCoverage },
		{ 15, "AA: Fury of Magic Rank6+ Pure-Caster Gate", TestFuryOfMagicPureCasterGate },
		{ 16, "Client DLL: Edge Probe Round-Trip", TestClientDllRoundTripProbe },
		{ 17, "Client DLL: Class Mask Mutation Round-Trip", TestClientDllClassMaskMutationRoundTrip },
		{ 18, "Client DLL: Mana Mutation Round-Trip", TestClientDllManaMutationRoundTrip },
		{ 19, "Client DLL: Snapshot Parity Round-Trip", TestClientDllSnapshotParityRoundTrip },
		{ 20, "Client DLL: Persisted Class-Mask Parity Round-Trip", TestClientDllPersistedClassMaskParity }
	};
	return tests;
}

const TestDefinition *FindTestById(int id)
{
	const auto &tests = GetTests();
	auto it = std::find_if(
		tests.begin(),
		tests.end(),
		[id](const TestDefinition &t) { return t.id == id; }
	);

	return (it != tests.end()) ? &(*it) : nullptr;
}

bool ParseSelectionPart(const std::string &part, std::set<int> &selected, int min_id, int max_id, std::string &error)
{
	if (part.empty()) {
		return true;
	}

	const std::string lowered = Strings::ToLower(part);
	if (lowered == "smoke") {
		InsertPack(selected, BuildPackSmoke(), min_id, max_id);
		return true;
	}

	if (lowered == "combat") {
		InsertPack(selected, BuildPackCombat(), min_id, max_id);
		return true;
	}

	if (lowered == "regression") {
		for (int id = min_id; id <= max_id; ++id) {
			selected.insert(id);
		}
		return true;
	}

	if (lowered == "automated") {
		InsertPack(selected, BuildPackAutomated(), min_id, max_id);
		return true;
	}

	const auto dash_pos = part.find('-');
	if (dash_pos != std::string::npos) {
		const std::string left_raw = part.substr(0, dash_pos);
		const std::string right_raw = part.substr(dash_pos + 1);
		std::string left = left_raw;
		std::string right = right_raw;
		Strings::Trim(left);
		Strings::Trim(right);
		if (!Seperator::IsNumber(left.c_str()) || !Seperator::IsNumber(right.c_str())) {
			error = fmt::format("invalid range '{}'", part);
			return false;
		}

		int start = Strings::ToInt(left);
		int end = Strings::ToInt(right);
		if (start > end) {
			std::swap(start, end);
		}

		if (start < min_id || end > max_id) {
			error = fmt::format("range '{}' outside valid ids [{}-{}]", part, min_id, max_id);
			return false;
		}

		for (int id = start; id <= end; ++id) {
			selected.insert(id);
		}

		return true;
	}

	if (!Seperator::IsNumber(part.c_str())) {
		error = fmt::format("invalid test id '{}'", part);
		return false;
	}

	const int id = Strings::ToInt(part);
	if (id < min_id || id > max_id) {
		error = fmt::format("test id {} outside valid ids [{}-{}]", id, min_id, max_id);
		return false;
	}

	selected.insert(id);
	return true;
}

void SendUsage(Client *c)
{
	c->Message(Chat::White, "Usage: #test list");
	c->Message(Chat::White, "Usage: #test packs");
	c->Message(Chat::White, "Usage: #test all");
	c->Message(Chat::White, "Usage: #test <id> (example: #test 3)");
	c->Message(Chat::White, "Usage: #test 16 (DLL client probe round-trip)");
	c->Message(Chat::White, "Usage: #test 17 (DLL class-mask mutation round-trip with auto-restore)");
	c->Message(Chat::White, "Usage: #test 18 (DLL mana mutation round-trip with auto-restore)");
	c->Message(Chat::White, "Usage: #test 19 (DLL snapshot parity round-trip)");
	c->Message(Chat::White, "Usage: #test 20 (DLL persisted class-mask parity round-trip)");
	c->Message(Chat::White, "Usage: #test <start-end> (example: #test 1-10)");
	c->Message(Chat::White, "Usage: #test <id,id,...> (example: #test 1,3,7)");
	c->Message(Chat::White, "Usage: #test smoke | combat | automated | regression");
}

void SendList(Client *c)
{
	c->Message(Chat::White, "In-Game Test IDs:");
	for (const auto &t : GetTests()) {
		c->Message(Chat::White, "  [%d] %s", t.id, t.name);
	}
}

void SendPacks(Client *c)
{
	c->Message(Chat::White, "In-Game Test Packs:");
	c->Message(Chat::White, "  smoke      -> foundational multiclass sanity checks");
	c->Message(Chat::White, "  combat     -> combat/proc/pet/AA gate prechecks");
	c->Message(Chat::White, "  automated  -> all checks runnable by #test command + DLL callbacks");
	c->Message(Chat::White, "  regression -> all registered #test checks");
}
} // namespace

void command_test(Client *c, const Seperator *sep)
{
	const auto &tests = GetTests();
	if (tests.empty()) {
		c->Message(Chat::White, "No tests are currently registered.");
		return;
	}

	const int min_id = tests.front().id;
	const int max_id = tests.back().id;

	const uint16 safe_arg_count = SafeArgCount(sep);
	const char *first_raw_arg = GetArgIfPresent(sep, 1);
	if (!sep || safe_arg_count < 1 || !first_raw_arg || first_raw_arg[0] == '\0') {
		SendUsage(c);
		return;
	}

	const std::string first_arg = Strings::ToLower(first_raw_arg);
	if (first_arg == "clientreply") {
		HandleClientReplyCommand(c, sep);
		return;
	}
	if (first_arg == "clientreplyv2") {
		HandleClientReplyV2Command(c, sep);
		return;
	}

	std::set<int> selected;
	bool requested_list = false;
	bool requested_packs = false;
	bool requested_all = false;

	for (uint16 i = 1; i <= safe_arg_count; ++i) {
		const char *token_arg = GetArgIfPresent(sep, i);
		if (!token_arg || token_arg[0] == '\0') {
			continue;
		}

		std::string token = Strings::ToLower(token_arg);
		Strings::Trim(token);

		if (token == "list") {
			requested_list = true;
			continue;
		}

		if (token == "packs") {
			requested_packs = true;
			continue;
		}

		if (token == "all") {
			requested_all = true;
			continue;
		}

		for (auto part : Strings::Split(token, ',')) {
			Strings::Trim(part);
			if (part.empty()) {
				continue;
			}

			std::string error;
			if (!ParseSelectionPart(part, selected, min_id, max_id, error)) {
				c->Message(Chat::White, "TEST | input error: %s", error.c_str());
				SendUsage(c);
				return;
			}
		}
	}

	if (requested_list && !requested_all && selected.empty()) {
		SendList(c);
		return;
	}

	if (requested_packs && !requested_all && selected.empty()) {
		SendPacks(c);
		return;
	}

	if (requested_all) {
		for (const auto &t : tests) {
			selected.insert(t.id);
		}
	}

	if (selected.empty()) {
		SendUsage(c);
		return;
	}

	c->Message(Chat::White, "TEST | running %d test(s)...", static_cast<int>(selected.size()));

	int pass_count = 0;
	int fail_count = 0;
	int skip_count = 0;

	for (int id : selected) {
		const auto *test = FindTestById(id);
		if (!test) {
			++fail_count;
			c->Message(Chat::White, "TEST [%d] FAIL | unknown test id", id);
			continue;
		}

		const auto result = test->runner(c);
		switch (result.state) {
			case TestState::Pass:
				++pass_count;
				c->Message(Chat::White, "TEST [%d] PASS | %s | %s", test->id, test->name, result.details.c_str());
				break;
			case TestState::Skip:
				++skip_count;
				c->Message(Chat::White, "TEST [%d] SKIP | %s | %s", test->id, test->name, result.details.c_str());
				break;
			case TestState::Fail:
			default:
				++fail_count;
				c->Message(Chat::White, "TEST [%d] FAIL | %s | %s", test->id, test->name, result.details.c_str());
				break;
		}
	}

	c->Message(
		Chat::White,
		"TEST | summary: total=%d pass=%d fail=%d skip=%d",
		static_cast<int>(selected.size()),
		pass_count,
		fail_count,
		skip_count
	);
}
