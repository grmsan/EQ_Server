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
#include <map>
#include <set>
#include <string>
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

std::vector<int> BuildPackSmoke()
{
	return { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
}

std::vector<int> BuildPackCombat()
{
	return { 6, 12, 13, 14, 15 };
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
		{ 15, "AA: Fury of Magic Rank6+ Pure-Caster Gate", TestFuryOfMagicPureCasterGate }
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
	c->Message(Chat::White, "Usage: #test <start-end> (example: #test 1-10)");
	c->Message(Chat::White, "Usage: #test <id,id,...> (example: #test 1,3,7)");
	c->Message(Chat::White, "Usage: #test smoke | combat | regression");
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

	if (!sep || sep->argnum < 1 || sep->arg[1][0] == '\0') {
		SendUsage(c);
		return;
	}

	std::set<int> selected;
	bool requested_list = false;
	bool requested_packs = false;
	bool requested_all = false;

	for (int i = 1; i <= sep->argnum; ++i) {
		if (sep->arg[i][0] == '\0') {
			continue;
		}

		std::string token = Strings::ToLower(sep->arg[i]);
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
