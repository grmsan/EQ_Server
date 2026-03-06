/*
 * augment_infusion.cpp — Augment Infusion Pool system (Step 11)
 *
 * Intercepts the "Combine" action on the Infusion Pool container,
 * validates 1 augment + 1 infusion catalyst, and applies +1 to
 * the augment's primary heroic stat via custom_data.
 *
 * Container: 2-slot "Infusion Pool" (item ID from rule InfusionPoolItemID)
 * Slot 0: The augment to infuse
 * Slot 1: An Infusion Catalyst (purchased from vendor with Common Essence)
 *
 * The catalyst tier must match the next infusion level.  E.g., if the aug
 * has 0 infusions, you need Infusion Catalyst I; with 2 infusions, Catalyst III.
 *
 * Cap: base_primary_stat + infusion_bonus <= L5 max for that aug family.
 *      Also hard-capped at 5 infusions total.
 *
 * Design reference: game_design/infinite_progression/AUGMENT_SYSTEM.md §8.1
 */

#include "augment_infusion.h"
#include "augment_merge_data.h"
#include "client.h"
#include "../common/item_instance.h"
#include "../common/inventory_profile.h"
#include "../common/rulesys.h"

static constexpr int MAX_INFUSIONS = 5;
static constexpr int L5_SINGLE_STAT_CAP_DEFAULT = 21;

// Infusion Catalyst item IDs — tiers 1-5
// These are defined in augment_merge_data.h or as rule values.
// Convention: 200520-200524 (right after merge catalysts and forge container)
static constexpr uint32_t INFUSION_CATALYST_IDS[] = {
	200520, // Infusion Catalyst I   (for infusion level 1)
	200521, // Infusion Catalyst II  (for infusion level 2)
	200522, // Infusion Catalyst III (for infusion level 3)
	200523, // Infusion Catalyst IV  (for infusion level 4)
	200524, // Infusion Catalyst V   (for infusion level 5)
};
static constexpr int NUM_INFUSION_CATALYSTS = sizeof(INFUSION_CATALYST_IDS) / sizeof(INFUSION_CATALYST_IDS[0]);

// Returns the infusion tier (1-5) if item_id is an infusion catalyst, else 0.
static int GetInfusionCatalystTier(uint32_t item_id) {
	for (int i = 0; i < NUM_INFUSION_CATALYSTS; i++) {
		if (item_id == INFUSION_CATALYST_IDS[i]) return i + 1;
	}
	return 0;
}

// Identify the "primary stat" — the heroic stat with the highest base value.
struct PrimaryStat {
	std::string key;      // custom_data key prefix e.g. "HEROIC_STR"
	std::string display;  // display name e.g. "Heroic STR"
	int         value;    // base stat value
};

static PrimaryStat GetPrimaryStat(const EQ::ItemData* item) {
	struct StatEntry {
		const char* key;
		const char* display;
		int         value;
	};

	StatEntry stats[] = {
		{"HEROIC_STR", "Heroic STR", item->HeroicStr},
		{"HEROIC_STA", "Heroic STA", item->HeroicSta},
		{"HEROIC_DEX", "Heroic DEX", item->HeroicDex},
		{"HEROIC_AGI", "Heroic AGI", item->HeroicAgi},
		{"HEROIC_INT", "Heroic INT", item->HeroicInt},
		{"HEROIC_WIS", "Heroic WIS", item->HeroicWis},
		{"HEROIC_CHA", "Heroic CHA", item->HeroicCha},
	};

	PrimaryStat best = {"", "", 0};
	for (const auto& s : stats) {
		if (s.value > best.value) {
			best.key     = s.key;
			best.display = s.display;
			best.value   = s.value;
		}
	}
	return best;
}

// Get the L5 cap for the augment's primary stat.
static int GetL5Cap(const EQ::ItemData* item) {
	int family = -1, level = -1;
	if (AugMergeData::GetAugInfo(item->ID, family, level)) {
		uint32_t l5_id = AugMergeData::GetItemID(family, AugMergeData::MAX_AUG_LEVEL);
		const EQ::ItemData* l5_item = database.GetItem(l5_id);
		if (l5_item) {
			PrimaryStat l5_stat = GetPrimaryStat(l5_item);
			if (l5_stat.value > 0) {
				return l5_stat.value;
			}
		}
	}
	return L5_SINGLE_STAT_CAP_DEFAULT;
}

void AugmentInfusion::ProcessInfusionPool(Client *c, int16 container_slot)
{
	if (!c) {
		return;
	}

	EQ::InventoryProfile &inv = c->GetInv();
	const EQ::ItemInstance *container = inv.GetItem(container_slot);

	if (!container || !container->GetItem()) {
		c->Message(Chat::Red, "Infusion error: container not found.");
		return;
	}

	if (!container->IsType(EQ::item::ItemClassBag)) {
		c->Message(Chat::Red, "Infusion error: not a valid container.");
		return;
	}

	// Collect items from the 2-slot container
	EQ::ItemInstance *aug_inst = nullptr;
	const EQ::ItemData *aug_data = nullptr;
	uint8 aug_bag_slot = 0;

	uint32_t catalyst_id = 0;
	int catalyst_tier = 0;
	uint8 catalyst_bag_slot = 0;
	bool found_catalyst = false;

	int total_items = 0;
	const int bag_slots = container->GetItem()->BagSlots;

	for (uint8 i = EQ::invbag::SLOT_BEGIN; i < bag_slots && i <= EQ::invbag::SLOT_END; i++) {
		const EQ::ItemInstance *slot_inst = container->GetItem(i);
		if (!slot_inst || !slot_inst->GetItem()) {
			continue;
		}
		total_items++;

		uint32_t item_id = slot_inst->GetItem()->ID;
		int cat_tier = GetInfusionCatalystTier(item_id);

		if (cat_tier > 0) {
			if (found_catalyst) {
				c->Message(Chat::Red, "Only one Infusion Catalyst is allowed.");
				return;
			}
			catalyst_id = item_id;
			catalyst_tier = cat_tier;
			catalyst_bag_slot = i;
			found_catalyst = true;
		}
		else if (slot_inst->GetItem()->ItemType == EQ::item::ItemTypeAugmentation) {
			if (aug_inst) {
				c->Message(Chat::Red, "Only one augment can be infused at a time.");
				return;
			}
			// Need mutable access for SetCustomData later — get from absolute slot
			int16 abs_slot = EQ::InventoryProfile::CalcSlotId(container_slot, i);
			aug_inst = inv.GetItem(abs_slot);
			aug_data = aug_inst ? aug_inst->GetUnscaledItem() : nullptr;
			if (!aug_data && aug_inst) aug_data = aug_inst->GetItem();
			aug_bag_slot = i;
		}
		else {
			c->Message(Chat::Red, "'%s' cannot be used in the Infusion Pool. Place one augment and one Infusion Catalyst.",
				slot_inst->GetItem()->Name);
			return;
		}
	}

	// Validate: exactly 2 items
	if (total_items != 2 || !aug_inst || !found_catalyst) {
		c->Message(Chat::Yellow,
			"Place one augment and one Infusion Catalyst in the Infusion Pool, then click Combine.");
		return;
	}

	// Identify primary heroic stat
	PrimaryStat primary = GetPrimaryStat(aug_data);
	if (primary.value <= 0) {
		c->Message(Chat::Red, "'%s' has no heroic stats and cannot be infused.", aug_data->Name);
		return;
	}

	// Read current infusion level
	std::string infuse_str = aug_inst->GetCustomData("infuse_level");
	int current_infuse = infuse_str.empty() ? 0 : std::max(0, atoi(infuse_str.c_str()));

	// Validate catalyst tier matches the next infusion level
	int next_infuse = current_infuse + 1;
	if (catalyst_tier != next_infuse) {
		c->Message(Chat::Red,
			"Wrong catalyst tier. This augment needs Infusion Catalyst %s for its next infusion.",
			next_infuse == 1 ? "I" : next_infuse == 2 ? "II" : next_infuse == 3 ? "III" :
			next_infuse == 4 ? "IV" : "V");
		return;
	}

	// Cap check: hard limit of 5 infusions
	if (current_infuse >= MAX_INFUSIONS) {
		c->Message(Chat::Red, "'%s' is already at maximum infusion level (%d).",
			aug_data->Name, MAX_INFUSIONS);
		return;
	}

	// Cap check: base + infusion <= L5 max
	int l5_cap = GetL5Cap(aug_data);
	if (primary.value + current_infuse >= l5_cap) {
		c->Message(Chat::Red, "'%s' is at maximum power. (%s: %d, cap: %d)",
			aug_data->Name, primary.display.c_str(), primary.value + current_infuse, l5_cap);
		return;
	}

	// --- All validation passed. Consume catalyst, apply infusion. ---

	// Delete the catalyst
	int16 cat_abs_slot = EQ::InventoryProfile::CalcSlotId(container_slot, catalyst_bag_slot);
	c->DeleteItemInInventory(cat_abs_slot, 0, true);

	// Apply infusion to the aug's custom_data
	int new_infuse = current_infuse + 1;
	aug_inst->SetCustomData("infuse_level", new_infuse);

	// Track per-stat infusion bonus
	std::string bonus_key = "infuse_" + primary.key;
	std::string existing_bonus_str = aug_inst->GetCustomData(bonus_key);
	int existing_bonus = existing_bonus_str.empty() ? 0 : atoi(existing_bonus_str.c_str());
	aug_inst->SetCustomData(bonus_key, existing_bonus + 1);

	// Update the actual stat key for ApplyCustomStats() to pick up
	std::string existing_stat_str = aug_inst->GetCustomData(primary.key);
	int existing_stat_val = existing_stat_str.empty() ? 0 : atoi(existing_stat_str.c_str());
	aug_inst->SetCustomData(primary.key, existing_stat_val + 1);

	// Save
	c->Save(1);

	int new_stat = primary.value + new_infuse;
	c->Message(Chat::Yellow,
		"The pool surges with power! %s infused to level %d. (%s: %d -> %d)",
		aug_data->Name, new_infuse, primary.display.c_str(),
		primary.value + current_infuse, new_stat);

	LogInfo("AugmentInfusion: [{}] infused [{}] (id={}) to level {}, {} {} -> {}",
		c->GetName(), aug_data->Name, aug_data->ID, new_infuse,
		primary.display, primary.value + current_infuse, new_stat);
}
