#include "dynamic_item_manager.h"
#include "../common/database.h"
#include "../common/repositories/items_repository.h"
#include "../common/strings.h"
#include "../common/eqemu_logsys.h"
#include <cmath>
#include <algorithm>
#include <filesystem>

namespace EQ {

DynamicItemManager& DynamicItemManager::Get() {
	static DynamicItemManager instance;

	// Ensure logs/inf/ directory exists
	static bool log_dir_created = false;
	if (!log_dir_created) {
		try {
			std::filesystem::create_directories("logs/inf");
			Log(Logs::General, Logs::Quests, "DynamicItemManager: Initialized - logs/inf/ directory ready");
		} catch (const std::exception& e) {
			Log(Logs::General, Logs::Error, "DynamicItemManager: Failed to create logs/inf/ directory: %s", e.what());
		}
		log_dir_created = true;
	}

	return instance;
}DynamicItemManager::~DynamicItemManager() {
	ClearCache();
}

// Generate dynamic item ID from base ID + level
// Format: 1LLLIIIIII (1 + level + base ID) - MUST fit in MySQL signed int(11)!
// First digit: 1 (constant prefix to avoid conflicts with base items)
// Next 3 digits: Level with offset 100 (101 = level 1, 250 = level 150)
// Last 6 digits: Base Item ID (0-999999)
// Example: Cloth Cap (1001) at level 1 = 1_101_001001 = 1101001001
// Example: Short Sword (9998) at level 127 = 1_227_009998 = 1227009998
// Max: 1,250,999999 (fits in signed int32 max 2,147,483,647) ✓
uint32 DynamicItemManager::GenerateDynamicID(uint32 base_id, int level) {
	if (level == 0) {
		Log(Logs::Detail, Logs::Quests, "GenerateDynamicID: level=0, returning base_id=%u", base_id);
		return base_id;  // Level 0 = base item
	}

	const uint32 LEVEL_OFFSET = 100;
	const uint32 MAX_LEVEL = 150;  // Hard cap (reduced to fit in signed int32)

	if (level > (int)MAX_LEVEL) {
		Log(Logs::General, Logs::Error, "GenerateDynamicID: level=%d exceeds MAX_LEVEL=%u, capping to MAX_LEVEL", level, MAX_LEVEL);
		level = MAX_LEVEL;
	}

	uint32 encoded_level = LEVEL_OFFSET + std::min(level, (int)MAX_LEVEL);
	uint32 encoded_base = base_id % 1000000;  // Limit to 6 digits

	// Format: 1LLLIIIIII (1 + 3-digit level + 6-digit base ID)
	uint32 dynamic_id = 1000000000U + (encoded_level * 1000000U) + encoded_base;

	Log(Logs::General, Logs::Quests, "GenerateDynamicID: base_id=%u, level=%u -> dynamic_id=%u", base_id, level, dynamic_id);

	return dynamic_id;
}

int DynamicItemManager::GetItemLevel(uint32 item_id) {
	const uint32 LEVEL_OFFSET = 100;
	const uint32 PREFIX = 1000000000U;

	// Check if it's a dynamic item (starts with 1)
	if (item_id < PREFIX) {
		Log(Logs::Detail, Logs::Quests, "GetItemLevel: item_id=%u is not dynamic, returning 0", item_id);
		return 0;  // Base item (not dynamic)
	}

	// Extract level: remove prefix, divide by 1M
	uint32 level_part = (item_id - PREFIX) / 1000000;

	if (level_part < LEVEL_OFFSET) {
		Log(Logs::General, Logs::Error, "GetItemLevel: Invalid level_part=%u for item_id=%u", level_part, item_id);
		return 0;
	}

	int level = level_part - LEVEL_OFFSET;
	Log(Logs::Detail, Logs::Quests, "GetItemLevel: item_id=%u -> level=%u", item_id, level);
	return level;
}

uint32 DynamicItemManager::GetBaseItemID(uint32 dynamic_id) {
	const uint32 PREFIX = 1000000000U;

	if (dynamic_id < PREFIX) {
		Log(Logs::Detail, Logs::Quests, "GetBaseItemID: dynamic_id=%u is base item", dynamic_id);
		return dynamic_id;  // Already a base item
	}

	uint32 base_id = dynamic_id % 1000000;  // Last 6 digits
	Log(Logs::Detail, Logs::Quests, "GetBaseItemID: dynamic_id=%u -> base_id=%u", dynamic_id, base_id);
	return base_id;
}

bool DynamicItemManager::IsDynamicItem(uint32 item_id) {
	const uint32 PREFIX = 1000000000U;
	return item_id >= PREFIX;
}
// Calculate stat using tiered formula
// Example: Level 25 with base_increment=1, tier_bonus=1
//   Tier 0 (1-10): +10 total
//   Tier 1 (11-20): +20 total
//   Tier 2 (21-25): +15 total (5 levels * 3/level)
//   Total: 10 + 20 + 15 = 45
int DynamicItemManager::CalculateTieredStat(int base_value, int level, int base_increment, int tier_bonus) {
	if (level <= 0) return base_value;

	int total = base_value;
	int tier = level / m_config.tier_size;

	// Add full tiers
	for (int t = 0; t < tier; t++) {
		int increment = base_increment + (t * tier_bonus);
		total += m_config.tier_size * increment;
	}

	// Add remaining levels in current tier
	int remaining_levels = level % m_config.tier_size;
	int current_tier_increment = base_increment + (tier * tier_bonus);
	total += remaining_levels * current_tier_increment;

	Log(Logs::Detail, Logs::Quests, "CalculateTieredStat: base=%u, level=%u, base_inc=%u, tier_bonus=%u -> result=%u (tier=%u)", base_value, level, base_increment, tier_bonus, total, tier);

	return total;
}

// Apply stat cap (127 max for base stats) with heroic overflow
void DynamicItemManager::ApplyStatCap(int& base_stat, int& heroic_stat, int raw_value) {
	const int CAP = m_config.base_stat_cap;  // Should be 127

	if (raw_value > CAP) {
		// Split: base gets capped, overflow goes to heroic
		base_stat = CAP;
		heroic_stat += (raw_value - CAP);

		Log(Logs::Detail, Logs::Quests, "ApplyStatCap: raw=%d exceeds cap=%d, split to base=%d, heroic=%d (overflow=%d)",
			raw_value, CAP, base_stat, heroic_stat, raw_value - CAP);
	} else {
		// Under cap, just set base
		base_stat = raw_value;

		Log(Logs::Detail, Logs::Quests, "ApplyStatCap: raw=%d under cap=%d, base=%d, heroic=%d",
			raw_value, CAP, base_stat, heroic_stat);
	}
}

// Apply level-based scaling to item
void DynamicItemManager::ApplyLevelScaling(EQ::ItemData* item, const EQ::ItemData* base_item, int level) {
	if (!item || !base_item || level <= 0) return;

	// Primary stats with tiered scaling
	item->AC = CalculateTieredStat(base_item->AC, level, m_config.ac_base_increment, m_config.ac_tier_bonus);
	item->HP = CalculateTieredStat(base_item->HP, level, m_config.hp_base_increment, m_config.hp_tier_bonus);
	item->Mana = CalculateTieredStat(base_item->Mana, level, m_config.mana_base_increment, m_config.mana_tier_bonus);
	item->Endur = CalculateTieredStat(base_item->Endur, level, m_config.hp_base_increment, m_config.hp_tier_bonus);

	// Attribute stats with 127 cap + heroic overflow
	if (base_item->AStr > 0) {
		int raw_str = CalculateTieredStat(base_item->AStr, level, m_config.stat_base_increment, m_config.stat_tier_bonus);
		int base_str = 0, heroic_str = 0;  // Start at 0, ApplyStatCap will set correctly
		ApplyStatCap(base_str, heroic_str, raw_str);
		item->AStr = static_cast<int8>(base_str);          // Safe: base_str is capped at 127
		item->HeroicStr = static_cast<int16>(heroic_str);  // Use int16 for heroics
	}

	if (base_item->ASta > 0) {
		int raw_sta = CalculateTieredStat(base_item->ASta, level, m_config.stat_base_increment, m_config.stat_tier_bonus);
		int base_sta = 0, heroic_sta = 0;
		ApplyStatCap(base_sta, heroic_sta, raw_sta);
		item->ASta = static_cast<int8>(base_sta);
		item->HeroicSta = static_cast<int16>(heroic_sta);
	}

	if (base_item->AAgi > 0) {
		int raw_agi = CalculateTieredStat(base_item->AAgi, level, m_config.stat_base_increment, m_config.stat_tier_bonus);
		int base_agi = 0, heroic_agi = 0;
		ApplyStatCap(base_agi, heroic_agi, raw_agi);
		item->AAgi = static_cast<int8>(base_agi);
		item->HeroicAgi = static_cast<int16>(heroic_agi);
	}

	if (base_item->ADex > 0) {
		int raw_dex = CalculateTieredStat(base_item->ADex, level, m_config.stat_base_increment, m_config.stat_tier_bonus);
		int base_dex = 0, heroic_dex = 0;
		ApplyStatCap(base_dex, heroic_dex, raw_dex);
		item->ADex = static_cast<int8>(base_dex);
		item->HeroicDex = static_cast<int16>(heroic_dex);
	}

	if (base_item->AInt > 0) {
		int raw_int = CalculateTieredStat(base_item->AInt, level, m_config.stat_base_increment, m_config.stat_tier_bonus);
		int base_int = 0, heroic_int = 0;
		ApplyStatCap(base_int, heroic_int, raw_int);
		item->AInt = static_cast<int8>(base_int);
		item->HeroicInt = static_cast<int16>(heroic_int);
	}

	if (base_item->AWis > 0) {
		int raw_wis = CalculateTieredStat(base_item->AWis, level, m_config.stat_base_increment, m_config.stat_tier_bonus);
		int base_wis = 0, heroic_wis = 0;
		ApplyStatCap(base_wis, heroic_wis, raw_wis);
		item->AWis = static_cast<int8>(base_wis);
		item->HeroicWis = static_cast<int16>(heroic_wis);
	}

	if (base_item->ACha > 0) {
		int raw_cha = CalculateTieredStat(base_item->ACha, level, m_config.stat_base_increment, m_config.stat_tier_bonus);
		int base_cha = 0, heroic_cha = 0;
		ApplyStatCap(base_cha, heroic_cha, raw_cha);
		item->ACha = static_cast<int8>(base_cha);
		item->HeroicCha = static_cast<int16>(heroic_cha);
	}

	// Attack (weapons only)
	if (base_item->Damage > 0) {
		item->Attack = CalculateTieredStat(base_item->Attack, level, m_config.attack_base_increment, m_config.attack_tier_bonus);
	}

	// Resistances with cap
	if (base_item->FR > 0) {
		item->FR = std::min(CalculateTieredStat(base_item->FR, level, m_config.stat_base_increment, m_config.stat_tier_bonus), m_config.resist_cap);
	}
	if (base_item->CR > 0) {
		item->CR = std::min(CalculateTieredStat(base_item->CR, level, m_config.stat_base_increment, m_config.stat_tier_bonus), m_config.resist_cap);
	}
	if (base_item->MR > 0) {
		item->MR = std::min(CalculateTieredStat(base_item->MR, level, m_config.stat_base_increment, m_config.stat_tier_bonus), m_config.resist_cap);
	}
	if (base_item->PR > 0) {
		item->PR = std::min(CalculateTieredStat(base_item->PR, level, m_config.stat_base_increment, m_config.stat_tier_bonus), m_config.resist_cap);
	}
	if (base_item->DR > 0) {
		item->DR = std::min(CalculateTieredStat(base_item->DR, level, m_config.stat_base_increment, m_config.stat_tier_bonus), m_config.resist_cap);
	}

	// Combat stats (slow scaling via milestones in ApplyMilestoneBonus)
	// Shielding, StrikeThrough, StunResist, SpellShield, Avoidance, Accuracy, CombatEffects

	// Caster stats (aggressive scaling ~1000 at level 100)
	if (base_item->HealAmt > 0) {
		item->HealAmt = CalculateTieredStat(base_item->HealAmt, level, m_config.caster_base_increment, m_config.caster_tier_bonus);
	}
	if (base_item->SpellDmg > 0) {
		item->SpellDmg = CalculateTieredStat(base_item->SpellDmg, level, m_config.caster_base_increment, m_config.caster_tier_bonus);
	}

	// Damage Shield and Dot Shielding (moderate scaling)
	if (base_item->DamageShield > 0) {
		item->DamageShield = CalculateTieredStat(base_item->DamageShield, level, m_config.combat_base_increment + 1, m_config.combat_tier_bonus + 1);
	}
	if (base_item->DotShielding > 0) {
		item->DotShielding = CalculateTieredStat(base_item->DotShielding, level, m_config.combat_base_increment + 1, m_config.combat_tier_bonus + 1);
	}

	// Regen stats (moderate scaling ~100-200 at level 100)
	if (base_item->ManaRegen > 0) {
		item->ManaRegen = CalculateTieredStat(base_item->ManaRegen, level, m_config.regen_base_increment, m_config.regen_tier_bonus);
	}
	if (base_item->EnduranceRegen > 0) {
		item->EnduranceRegen = CalculateTieredStat(base_item->EnduranceRegen, level, m_config.regen_base_increment, m_config.regen_tier_bonus);
	}

	// Heroic Resistances (moderate scaling)
	if (base_item->HeroicMR > 0) {
		item->HeroicMR = CalculateTieredStat(base_item->HeroicMR, level, m_config.regen_base_increment, m_config.regen_tier_bonus);
	}
	if (base_item->HeroicFR > 0) {
		item->HeroicFR = CalculateTieredStat(base_item->HeroicFR, level, m_config.regen_base_increment, m_config.regen_tier_bonus);
	}
	if (base_item->HeroicCR > 0) {
		item->HeroicCR = CalculateTieredStat(base_item->HeroicCR, level, m_config.regen_base_increment, m_config.regen_tier_bonus);
	}
	if (base_item->HeroicDR > 0) {
		item->HeroicDR = CalculateTieredStat(base_item->HeroicDR, level, m_config.regen_base_increment, m_config.regen_tier_bonus);
	}
	if (base_item->HeroicPR > 0) {
		item->HeroicPR = CalculateTieredStat(base_item->HeroicPR, level, m_config.regen_base_increment, m_config.regen_tier_bonus);
	}
	if (base_item->HeroicSVCorrup > 0) {
		item->HeroicSVCorrup = CalculateTieredStat(base_item->HeroicSVCorrup, level, m_config.regen_base_increment, m_config.regen_tier_bonus);
	}

	// Apply milestones (haste, heroics, focus effects, etc.)
	ApplyMilestoneBonus(item, level);
}

// Apply milestone bonuses (haste, heroic stats, focus effects)
void DynamicItemManager::ApplyMilestoneBonus(EQ::ItemData* item, int level) {
	if (!item) return;

	// Haste (only on waist, back, range slots - starts at level 1)
	// Check if item is equippable in waist, back, or range slots
	bool is_haste_slot = false;
	if (item->Slots) {
		is_haste_slot = (item->Slots & (1 << EQ::invslot::slotWaist)) ||
		                (item->Slots & (1 << EQ::invslot::slotBack)) ||
		                (item->Slots & (1 << EQ::invslot::slotRange));
	}

	if (is_haste_slot && level >= m_config.haste_start_level) {
		int haste = 0;
		if (level <= 100) {
			// Levels 1-100: Linear scaling to 100%
			// Level 10 = 20%, Level 40 = 40%, Level 100 = 100%
			// Formula: level * 1.0 gives us the progression we want
			haste = level;
		} else {
			// Levels 100+: Slow increase beyond cap (for future overhaste support)
			// Every 10 levels adds 1% beyond 100
			haste = 100 + (level - 100) / 10;
		}
		item->Haste = std::min(haste, m_config.haste_cap);
	}

	// Heroic stats from milestones (bonus beyond overflow)
	if (level >= m_config.heroic_start_level) {
		int milestone_heroic = (level - m_config.heroic_start_level) / m_config.heroic_per_levels;

		// Add milestone heroics to all stats that exist
		if (item->AStr > 0 || item->HeroicStr > 0) item->HeroicStr += milestone_heroic;
		if (item->ASta > 0 || item->HeroicSta > 0) item->HeroicSta += milestone_heroic;
		if (item->AAgi > 0 || item->HeroicAgi > 0) item->HeroicAgi += milestone_heroic;
		if (item->ADex > 0 || item->HeroicDex > 0) item->HeroicDex += milestone_heroic;
		if (item->AInt > 0 || item->HeroicInt > 0) item->HeroicInt += milestone_heroic;
		if (item->AWis > 0 || item->HeroicWis > 0) item->HeroicWis += milestone_heroic;
		if (item->ACha > 0 || item->HeroicCha > 0) item->HeroicCha += milestone_heroic;
	}

	// HP Regeneration (unlocks at level 50, moderate scaling)
	if (level >= 50) {
		item->Regen = (level - 50) / 5;  // +1 per 5 levels
	}

	// Combat stats (slow scaling ~10 at level 100, cap at 127)
	// These get +1 per 10 levels
	if (level >= 10) {
		int combat_bonus = level / 10;  // +1 per 10 levels
		if (item->Shielding > 0) item->Shielding = std::min(item->Shielding + combat_bonus, 127);
		if (item->StrikeThrough > 0) item->StrikeThrough = std::min(item->StrikeThrough + combat_bonus, 127);
		if (item->StunResist > 0) item->StunResist = std::min(item->StunResist + combat_bonus, 127);
		if (item->SpellShield > 0) item->SpellShield = std::min(item->SpellShield + combat_bonus, 127);
		if (item->Avoidance > 0) item->Avoidance = std::min(item->Avoidance + combat_bonus, 127);
		if (item->Accuracy > 0) item->Accuracy = std::min(item->Accuracy + combat_bonus, 127);
		if (item->CombatEffects > 0) item->CombatEffects = std::min(item->CombatEffects + combat_bonus, 127);
	}

	// Focus effects (milestones at 100, 200, 500)
	// TODO: Implement focus effect system
	// if (level >= m_config.focus_minor_level) { ... }
}

// Generate scaled item data
EQ::ItemData* DynamicItemManager::GenerateScaledItem(uint32 base_item_id, int level) {
	Log(Logs::General, Logs::Quests, "GenerateScaledItem: START - base_item_id=%u, level=%u", base_item_id, level);

	// Check cache first
	uint32 dynamic_id = GenerateDynamicID(base_item_id, level);
	auto* cached = GetCachedItem(dynamic_id);
	if (cached) {
		Log(Logs::General, Logs::Quests, "GenerateScaledItem: Cache HIT - dynamic_id=%u, returning cached item '%s'", dynamic_id, cached->Name);
		return cached;
	}

	Log(Logs::General, Logs::Quests, "GenerateScaledItem: Cache MISS - dynamic_id=%u, generating new item", dynamic_id);

	// Load base item from database
	auto base_item_data = database.GetItem(base_item_id);
	if (!base_item_data) {
		Log(Logs::General, Logs::Error, "GenerateScaledItem: FAILED - Base item [%u] not found in database", base_item_id);
		return nullptr;
	}

	Log(Logs::General, Logs::Quests, "GenerateScaledItem: Base item loaded - id=%u, name='%s', AC=%d, HP=%d, STR=%d", base_item_data->ID, base_item_data->Name, base_item_data->AC,
		base_item_data->HP, base_item_data->AStr);

	// Create copy for scaling
	auto* scaled_item = new EQ::ItemData(*base_item_data);
	scaled_item->ID = dynamic_id;

	// Apply scaling
	Log(Logs::General, Logs::Quests, "GenerateScaledItem: Applying level scaling...");
	ApplyLevelScaling(scaled_item, base_item_data, level);

	Log(Logs::General, Logs::Quests, "GenerateScaledItem: Scaling complete - AC=%d, HP=%d, STR=%d/%d", scaled_item->AC, scaled_item->HP, scaled_item->AStr, scaled_item->HeroicStr);

	// Update name to show level
	if (level > 0) {
		std::string new_name = std::string(scaled_item->Name) + " +" + std::to_string(level);
		strncpy(scaled_item->Name, new_name.c_str(), sizeof(scaled_item->Name) - 1);
		scaled_item->Name[sizeof(scaled_item->Name) - 1] = '\0';

		Log(Logs::General, Logs::Quests, "GenerateScaledItem: Renamed to '%s'", scaled_item->Name);
	}

	// Insert into database for persistence
	InsertItemIntoDatabase(dynamic_id, base_item_id, scaled_item);

	// Cache the generated item
	CacheItem(dynamic_id, scaled_item);

	Log(Logs::General, Logs::Quests, "GenerateScaledItem: SUCCESS - Cached dynamic_id=%u, cache_size=%zu", dynamic_id, m_item_cache.size());

	return scaled_item;
}

// Create item instance with dynamic stats
EQ::ItemInstance* DynamicItemManager::CreateDynamicInstance(uint32 base_item_id, int level) {
	auto* scaled_item = GenerateScaledItem(base_item_id, level);
	if (!scaled_item) return nullptr;

	auto* inst = new EQ::ItemInstance(scaled_item, 1);
	return inst;
}

// Fuse two items (transfer level from donor to receiver)
EQ::ItemInstance* DynamicItemManager::FuseItems(EQ::ItemInstance* donor, EQ::ItemInstance* receiver) {
	if (!donor || !receiver) {
		Log(Logs::General, Logs::Error, "FuseItems: FAILED - null donor or receiver");
		return nullptr;
	}

	int donor_level = GetItemLevel(donor->GetID());
	uint32 donor_base_id = GetBaseItemID(donor->GetID());
	uint32 receiver_base_id = GetBaseItemID(receiver->GetID());

	Log(Logs::General, Logs::Quests, "FuseItems: START - Donor: id=%u (base=%u, level=%u), Receiver: id=%u (base=%u)", donor->GetID(), donor_base_id, donor_level,
		receiver->GetID(), receiver_base_id);

	if (donor_level == 0) {
		Log(Logs::General, Logs::Error, "FuseItems: FAILED - Donor has no levels to transfer");
		return nullptr;
	}

	// Create new item with receiver's base + donor's level
	auto* result = CreateDynamicInstance(receiver_base_id, donor_level);

	if (result) {
		Log(Logs::General, Logs::Quests, "FuseItems: SUCCESS - Created %s with %d levels from donor", result->GetItem()->Name, donor_level);
	} else {
		Log(Logs::General, Logs::Error, "FuseItems: FAILED - Could not create result instance");
	}

	return result;
}

// Cache management
void DynamicItemManager::CacheItem(uint32 item_id, EQ::ItemData* item) {
	// Evict old items if cache is too large (simple LRU would be better)
	const int MAX_CACHE_SIZE = 1000;
	if (m_item_cache.size() >= MAX_CACHE_SIZE) {
		delete m_item_cache.begin()->second;
		m_item_cache.erase(m_item_cache.begin());
	}

	m_item_cache[item_id] = item;
}

EQ::ItemData* DynamicItemManager::GetCachedItem(uint32 item_id) {
	auto it = m_item_cache.find(item_id);
	if (it != m_item_cache.end()) {
		return it->second;
	}
	return nullptr;
}

void DynamicItemManager::ClearCache() {
	for (auto& pair : m_item_cache) {
		delete pair.second;
	}
	m_item_cache.clear();
}

// Insert dynamic item into database by copying base item and updating scaled stats
void DynamicItemManager::InsertItemIntoDatabase(uint32 dynamic_id, uint32 base_id, const EQ::ItemData* item) {
	if (!item) {
		Log(Logs::General, Logs::Error, "InsertItemIntoDatabase: FAILED - null item");
		return;
	}

	// Check if item already exists
	std::string check_query = fmt::format("SELECT id FROM items WHERE id = {}", dynamic_id);
	auto results = database.QueryDatabase(check_query);
	if (results.Success() && results.RowCount() > 0) {
		Log(Logs::General, Logs::Status, "InsertItemIntoDatabase: Item %u already exists, skipping", dynamic_id);
		return;
	}

	// Step 1: Copy the base item row with new ID
	// List all columns explicitly (excluding id which we provide separately)
	std::string copy_query = fmt::format(
		"INSERT INTO items (id, minstatus, Name, aagi, ac, accuracy, acha, adex, aint, artifactflag, asta, astr, attack, "
		"augrestrict, augslot1type, augslot1visible, augslot2type, augslot2visible, augslot3type, augslot3visible, "
		"augslot4type, augslot4visible, augslot5type, augslot5visible, augslot6type, augslot6visible, augtype, "
		"avoidance, awis, bagsize, bagslots, bagtype, bagwr, banedmgamt, banedmgraceamt, banedmgbody, banedmgrace, "
		"bardtype, bardvalue, book, casttime, casttime_, charmfile, charmfileid, classes, color, combateffects, "
		"extradmgskill, extradmgamt, price, cr, damage, damageshield, deity, delay, augdistiller, dotshielding, dr, "
		"clicktype, clicklevel2, elemdmgtype, elemdmgamt, endur, factionamt1, factionamt2, factionamt3, factionamt4, "
		"factionmod1, factionmod2, factionmod3, factionmod4, filename, focuseffect, fr, fvnodrop, haste, clicklevel, "
		"hp, regen, icon, idfile, itemclass, itemtype, ldonprice, ldontheme, ldonsold, light, lore, loregroup, magic, "
		"mana, manaregen, enduranceregen, material, herosforgemodel, maxcharges, mr, nodrop, norent, pendingloreflag, "
		"pr, procrate, races, `range`, reclevel, recskill, reqlevel, sellrate, shielding, size, skillmodtype, "
		"skillmodvalue, slots, clickeffect, spellshield, strikethrough, stunresist, summonedflag, tradeskills, favor, "
		"weight, UNK012, UNK013, benefitflag, UNK054, UNK059, booktype, recastdelay, recasttype, guildfavor, UNK123, "
		"UNK124, attuneable, nopet, updated, comment, UNK127, pointtype, potionbelt, potionbeltslots, stacksize, "
		"notransfer, stackable, UNK134, UNK137, proceffect, proctype, proclevel2, proclevel, UNK142, worneffect, "
		"worntype, wornlevel2, wornlevel, UNK147, focustype, focuslevel2, focuslevel, UNK152, scrolleffect, scrolltype, "
		"scrolllevel2, scrolllevel, UNK157, serialized, verified, serialization, source, UNK033, lorefile, UNK014, "
		"svcorruption, skillmodmax, UNK060, augslot1unk2, augslot2unk2, augslot3unk2, augslot4unk2, augslot5unk2, "
		"augslot6unk2, UNK120, UNK121, questitemflag, UNK132, clickunk5, clickunk6, clickunk7, procunk1, procunk2, "
		"procunk3, procunk4, procunk6, procunk7, wornunk1, wornunk2, wornunk3, wornunk4, wornunk5, wornunk6, wornunk7, "
		"focusunk1, focusunk2, focusunk3, focusunk4, focusunk5, focusunk6, focusunk7, scrollunk1, scrollunk2, "
		"scrollunk3, scrollunk4, scrollunk5, scrollunk6, scrollunk7, UNK193, purity, evoitem, evoid, evolvinglevel, "
		"evomax, clickname, procname, wornname, focusname, scrollname, dsmitigation, heroic_str, heroic_int, heroic_wis, "
		"heroic_agi, heroic_dex, heroic_sta, heroic_cha, heroic_pr, heroic_dr, heroic_fr, heroic_cr, heroic_mr, "
		"heroic_svcorrup, healamt, spelldmg, clairvoyance, backstabdmg, created, elitematerial, ldonsellbackrate, "
		"scriptfileid, expendablearrow, powersourcecapacity, bardeffect, bardeffecttype, bardlevel2, bardlevel, bardunk1, "
		"bardunk2, bardunk3, bardunk4, bardunk5, bardname, bardunk7, UNK214, subtype, UNK220, UNK221, heirloom, UNK223, "
		"UNK224, UNK225, UNK226, UNK227, UNK228, UNK229, UNK230, UNK231, UNK232, UNK233, UNK234, placeable, UNK236, "
		"UNK237, UNK238, UNK239, UNK240, UNK241, epicitem) "
		"SELECT {}, minstatus, Name, aagi, ac, accuracy, acha, adex, aint, artifactflag, asta, astr, attack, "
		"augrestrict, augslot1type, augslot1visible, augslot2type, augslot2visible, augslot3type, augslot3visible, "
		"augslot4type, augslot4visible, augslot5type, augslot5visible, augslot6type, augslot6visible, augtype, "
		"avoidance, awis, bagsize, bagslots, bagtype, bagwr, banedmgamt, banedmgraceamt, banedmgbody, banedmgrace, "
		"bardtype, bardvalue, book, casttime, casttime_, charmfile, charmfileid, classes, color, combateffects, "
		"extradmgskill, extradmgamt, price, cr, damage, damageshield, deity, delay, augdistiller, dotshielding, dr, "
		"clicktype, clicklevel2, elemdmgtype, elemdmgamt, endur, factionamt1, factionamt2, factionamt3, factionamt4, "
		"factionmod1, factionmod2, factionmod3, factionmod4, filename, focuseffect, fr, fvnodrop, haste, clicklevel, "
		"hp, regen, icon, idfile, itemclass, itemtype, ldonprice, ldontheme, ldonsold, light, lore, loregroup, magic, "
		"mana, manaregen, enduranceregen, material, herosforgemodel, maxcharges, mr, nodrop, norent, pendingloreflag, "
		"pr, procrate, races, `range`, reclevel, recskill, reqlevel, sellrate, shielding, size, skillmodtype, "
		"skillmodvalue, slots, clickeffect, spellshield, strikethrough, stunresist, summonedflag, tradeskills, favor, "
		"weight, UNK012, UNK013, benefitflag, UNK054, UNK059, booktype, recastdelay, recasttype, guildfavor, UNK123, "
		"UNK124, attuneable, nopet, updated, comment, UNK127, pointtype, potionbelt, potionbeltslots, stacksize, "
		"notransfer, stackable, UNK134, UNK137, proceffect, proctype, proclevel2, proclevel, UNK142, worneffect, "
		"worntype, wornlevel2, wornlevel, UNK147, focustype, focuslevel2, focuslevel, UNK152, scrolleffect, scrolltype, "
		"scrolllevel2, scrolllevel, UNK157, serialized, verified, serialization, source, UNK033, lorefile, UNK014, "
		"svcorruption, skillmodmax, UNK060, augslot1unk2, augslot2unk2, augslot3unk2, augslot4unk2, augslot5unk2, "
		"augslot6unk2, UNK120, UNK121, questitemflag, UNK132, clickunk5, clickunk6, clickunk7, procunk1, procunk2, "
		"procunk3, procunk4, procunk6, procunk7, wornunk1, wornunk2, wornunk3, wornunk4, wornunk5, wornunk6, wornunk7, "
		"focusunk1, focusunk2, focusunk3, focusunk4, focusunk5, focusunk6, focusunk7, scrollunk1, scrollunk2, "
		"scrollunk3, scrollunk4, scrollunk5, scrollunk6, scrollunk7, UNK193, purity, evoitem, evoid, evolvinglevel, "
		"evomax, clickname, procname, wornname, focusname, scrollname, dsmitigation, heroic_str, heroic_int, heroic_wis, "
		"heroic_agi, heroic_dex, heroic_sta, heroic_cha, heroic_pr, heroic_dr, heroic_fr, heroic_cr, heroic_mr, "
		"heroic_svcorrup, healamt, spelldmg, clairvoyance, backstabdmg, created, elitematerial, ldonsellbackrate, "
		"scriptfileid, expendablearrow, powersourcecapacity, bardeffect, bardeffecttype, bardlevel2, bardlevel, bardunk1, "
		"bardunk2, bardunk3, bardunk4, bardunk5, bardname, bardunk7, UNK214, subtype, UNK220, UNK221, heirloom, UNK223, "
		"UNK224, UNK225, UNK226, UNK227, UNK228, UNK229, UNK230, UNK231, UNK232, UNK233, UNK234, placeable, UNK236, "
		"UNK237, UNK238, UNK239, UNK240, UNK241, epicitem FROM items WHERE id = {}",
		dynamic_id, base_id
	);

	auto copy_results = database.QueryDatabase(copy_query);
	if (!copy_results.Success()) {
		Log(Logs::General, Logs::Error, "InsertItemIntoDatabase: FAILED to copy base item %u - %s", base_id, copy_results.ErrorMessage().c_str());
		return;
	}

	// Step 2: Update the scaled stats (use multiple UPDATE statements to avoid fmt limits)
	std::string name_escaped = item->Name;
	// Simple escape - replace single quotes
	size_t pos = 0;
	while ((pos = name_escaped.find("'", pos)) != std::string::npos) {
		name_escaped.replace(pos, 1, "''");
		pos += 2;
	}

	std::string update_query = fmt::format(
		"UPDATE items SET name = '{}', damage = {}, hp = {}, mana = {}, endur = {}, ac = {} WHERE id = {}",
		name_escaped, item->Damage, item->HP, item->Mana, item->Endur, item->AC, dynamic_id
	);
	auto update_results = database.QueryDatabase(update_query);
	if (!update_results.Success()) {
		Log(Logs::General, Logs::Error, "InsertItemIntoDatabase: FAILED to update basic stats - %s", update_results.ErrorMessage().c_str());
		return;
	}

	update_query = fmt::format(
		"UPDATE items SET astr = {}, asta = {}, aagi = {}, adex = {}, awis = {}, aint = {}, acha = {} WHERE id = {}",
		item->AStr, item->ASta, item->AAgi, item->ADex, item->AWis, item->AInt, item->ACha, dynamic_id
	);
	database.QueryDatabase(update_query);

	update_query = fmt::format(
		"UPDATE items SET fr = {}, cr = {}, mr = {}, pr = {}, dr = {}, svcorruption = {} WHERE id = {}",
		item->FR, item->CR, item->MR, item->PR, item->DR, item->SVCorruption, dynamic_id
	);
	database.QueryDatabase(update_query);

	update_query = fmt::format(
		"UPDATE items SET heroic_str = {}, heroic_sta = {}, heroic_agi = {}, heroic_dex = {}, heroic_wis = {}, heroic_int = {}, heroic_cha = {} WHERE id = {}",
		item->HeroicStr, item->HeroicSta, item->HeroicAgi, item->HeroicDex, item->HeroicWis, item->HeroicInt, item->HeroicCha, dynamic_id
	);
	database.QueryDatabase(update_query);

	update_query = fmt::format(
		"UPDATE items SET heroic_fr = {}, heroic_cr = {}, heroic_mr = {}, heroic_pr = {}, heroic_dr = {}, heroic_svcorrup = {} WHERE id = {}",
		item->HeroicFR, item->HeroicCR, item->HeroicMR, item->HeroicPR, item->HeroicDR, item->HeroicSVCorrup, dynamic_id
	);
	database.QueryDatabase(update_query);

	update_query = fmt::format(
		"UPDATE items SET haste = {}, regen = {}, manaregen = {}, enduranceregen = {} WHERE id = {}",
		item->Haste, item->Regen, item->ManaRegen, item->EnduranceRegen, dynamic_id
	);
	database.QueryDatabase(update_query);

	update_query = fmt::format(
		"UPDATE items SET attack = {}, strikethrough = {}, accuracy = {}, stunresist = {}, avoidance = {} WHERE id = {}",
		item->Attack, item->StrikeThrough, item->Accuracy, item->StunResist, item->Avoidance, dynamic_id
	);
	database.QueryDatabase(update_query);

	update_query = fmt::format(
		"UPDATE items SET shielding = {}, dotshielding = {}, spellshield = {}, healamt = {}, spelldmg = {}, clairvoyance = {}, backstabdmg = {} WHERE id = {}",
		item->Shielding, item->DotShielding, item->SpellShield, item->HealAmt, item->SpellDmg, item->Clairvoyance, item->DSMitigation, dynamic_id
	);
	database.QueryDatabase(update_query);

	Log(Logs::General, Logs::Status, "InsertItemIntoDatabase: SUCCESS - Created item %u (%s) from base %u", dynamic_id, item->Name, base_id);
}

} // namespace EQ

