#ifndef ZONE_DYNAMIC_ITEM_MANAGER_H
#define ZONE_DYNAMIC_ITEM_MANAGER_H

#include "../common/item_data.h"
#include "../common/item_instance.h"
#include <map>
#include <vector>
#include <string>

namespace EQ {

// Configuration for stat scaling formulas
struct ScalingConfig {
	// Tiered scaling - increment increases per tier
	int tier_size = 10;  // Levels per tier

	// Base increments (tier 0, levels 1-10)
	int ac_base_increment = 1;      // +1 AC per level
	int hp_base_increment = 4;      // +4 HP per level (was 2)
	int mana_base_increment = 1;    // +1 Mana per level
	int stat_base_increment = 1;    // +1 stat per level
	int attack_base_increment = 2;  // +2 attack per level
	int damage_base_increment = 4;  // +4 damage per level

	// Tier bonuses (added per tier)
	int ac_tier_bonus = 1;          // Each tier adds +1 to increment
	int hp_tier_bonus = 4;          // Each tier adds +4 to increment (was 2)
	int mana_tier_bonus = 1;        // Each tier adds +1 to increment
	int stat_tier_bonus = 1;        // Each tier adds +1 to increment
	int attack_tier_bonus = 2;      // Each tier adds +2 to increment
	int damage_tier_bonus = 4;      // Each tier adds +4 to increment

	// Combat stats (slow scaling, ~10 at level 100)
	int combat_base_increment = 0;  // No gain in tier 0
	int combat_tier_bonus = 0;      // Flat +1 per 10 levels via milestone

	// Caster stats (aggressive scaling, ~1000 at level 100)
	int caster_base_increment = 5;  // +5 per level
	int caster_tier_bonus = 5;      // Each tier adds +5 to increment

	// Regen stats (moderate scaling, ~100-200 at level 100)
	int regen_base_increment = 1;   // +1 per level
	int regen_tier_bonus = 1;       // Each tier adds +1 to increment

	// Client limits
	int base_stat_cap = 127;        // EQ client hard cap
	int resist_cap = 127;           // Resist cap
	int haste_cap = 100;            // Haste cap (%)

	// Heroic milestones
	int heroic_start_level = 50;
	int heroic_per_levels = 5;      // +1 heroic per 5 levels

	// Haste progression (only applies to waist, back, range slots)
	int haste_start_level = 1;      // Start at level 1
	int haste_slow_divisor = 3;     // Levels 1-25: (level - 1) / 3 = hits 8% at level 25
	int haste_fast_divisor = 1;     // Levels 26+: 8 + (level - 25) / 1 = hits 100% at level 117

	// Focus effects
	int focus_minor_level = 100;
	int focus_major_level = 200;
	int focus_epic_level = 500;

	// Random stats
	float new_stat_chance = 0.5f;   // 50%
	int new_stat_interval = 5;      // Every 5 levels
	int max_random_stats = 8;       // Limit to 8 different stats
};

struct RandomStat {
	std::string type;  // "STR", "WIS", "FR", etc.
	int value;
	int added_at_level;
};

class DynamicItemManager {
public:
	// Singleton access
	static DynamicItemManager& Get();

	// Dynamic ID management
	uint32 GenerateDynamicID(uint32 base_id, int level);
	int GetItemLevel(uint32 item_id);
	uint32 GetBaseItemID(uint32 item_id);
	bool IsDynamicItem(uint32 item_id);

	// Item generation
	EQ::ItemData* GenerateScaledItem(uint32 base_item_id, int level);
	EQ::ItemInstance* CreateDynamicInstance(uint32 base_item_id, int level);

	// Stat scaling (tiered formulas)
	int CalculateTieredStat(int base_value, int level, int base_increment, int tier_bonus);
	void ApplyLevelScaling(EQ::ItemData* item, const EQ::ItemData* base_item, int level);
	void ApplyMilestoneBonus(EQ::ItemData* item, int level);

	// Heroic overflow (handle 127 cap)
	void ApplyStatCap(int& base_stat, int& heroic_stat, int raw_value);

	// Item fusion
	EQ::ItemInstance* FuseItems(EQ::ItemInstance* donor, EQ::ItemInstance* receiver);
	// Fuse using a charge level (e.g., a 'fusion charge' consumable that captures donor level)
	EQ::ItemInstance* FuseWithCharge(int donorLevel, EQ::ItemInstance* receiver);

	// Configuration
	ScalingConfig& GetConfig() { return m_config; }

	// Caching
	void CacheItem(uint32 item_id, EQ::ItemData* item);
	EQ::ItemData* GetCachedItem(uint32 item_id);
	void ClearCache();

	// Database persistence
	void InsertItemIntoDatabase(uint32 dynamic_id, uint32 base_id, const EQ::ItemData* item);

private:
	DynamicItemManager() = default;
	~DynamicItemManager();

	ScalingConfig m_config;
	std::map<uint32, EQ::ItemData*> m_item_cache;
	std::map<uint32, std::vector<RandomStat>> m_random_stats;
};

} // namespace EQ

#endif // ZONE_DYNAMIC_ITEM_MANAGER_H
