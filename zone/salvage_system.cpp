#include "client.h"
#include "salvage.h"
#include "augment_merge_data.h"
#include "../common/item_ilevel.h"
#include "../common/item_tier.h"
#include "../common/rulesys.h"
#include "../common/inventory_profile.h"
#include "zone.h"

/*
 * Salvage::ProcessSatchel — core salvage logic.
 *
 * Iterates the container at container_slot, evaluates each item for salvage
 * eligibility, calculates Essence yield, deletes consumed items, and awards
 * alternate currency.
 *
 * Augments are handled specially via the Transmutation table (§8.2):
 *   L1 = 125 CE, L2 = 375 CE, L3 = 1000 CE + 25 RE,
 *   L4 = 2500 CE + 75 RE, L5 = 6000 CE + 200 RE.
 * Non-mergeable augs (zone drops, named drops) transmute at L1 value.
 *
 * Called from HandleCombine (player clicks Combine on the Satchel) and
 * from #salvage GM command.
 */

// Transmutation return table for augments (index 0 = L1)
struct TransmuteReturn {
	int common;
	int rare;
};
static constexpr TransmuteReturn TRANSMUTE_TABLE[] = {
	{  125,   0 },  // L1
	{  375,   0 },  // L2
	{ 1000,  25 },  // L3
	{ 2500,  75 },  // L4
	{ 6000, 200 },  // L5
};
static constexpr int TRANSMUTE_TABLE_SIZE = sizeof(TRANSMUTE_TABLE) / sizeof(TRANSMUTE_TABLE[0]);

static const TransmuteReturn& GetTransmuteReturn(int level) {
	int idx = std::clamp(level, 1, TRANSMUTE_TABLE_SIZE) - 1;
	return TRANSMUTE_TABLE[idx];
}

static int GetAugEffectiveLevel(uint32_t item_id) {
	int family = -1, level = -1;
	if (AugMergeData::GetAugInfo(item_id, family, level)) {
		return level;
	}
	return 1; // Non-mergeable augs default to L1
}

void Salvage::ProcessSatchel(Client *c, int16 container_slot)
{
	if (!c) {
		return;
	}

	const uint32 common_currency_id = RuleI(ItemProgression, CommonEssenceCurrencyID);
	const uint32 rare_currency_id   = RuleI(ItemProgression, RareEssenceCurrencyID);

	EQ::InventoryProfile &inv = c->GetInv();
	const EQ::ItemInstance *satchel_inst = inv.GetItem(container_slot);

	if (!satchel_inst || !satchel_inst->GetItem()) {
		c->Message(Chat::Red, "Salvage error: container not found.");
		return;
	}

	if (!satchel_inst->IsType(EQ::item::ItemClassBag)) {
		c->Message(Chat::Red, "Salvage error: not a valid container.");
		return;
	}

	// Collect results before deleting
	struct SalvageResult {
		uint8       bag_slot;
		int         common_yield = 0;
		int         rare_yield   = 0;
		bool        rejected     = false;
		bool        mythic       = false;
		bool        transmuted   = false;
		std::string item_name;
	};
	std::vector<SalvageResult> results;

	const int bag_slots = satchel_inst->GetItem()->BagSlots;
	for (uint8 i = EQ::invbag::SLOT_BEGIN; i < bag_slots && i <= EQ::invbag::SLOT_END; i++) {
		const EQ::ItemInstance *slot_inst = satchel_inst->GetItem(i);
		if (!slot_inst || !slot_inst->GetItem()) {
			continue;
		}

		const EQ::ItemData *item_data = slot_inst->GetItem();
		SalvageResult res = {};
		res.bag_slot  = i;
		res.item_name = item_data->Name;

		// --- Augment? Use transmutation table instead of normal salvage ---
		if (item_data->ItemType == EQ::item::ItemTypeAugmentation) {
			int aug_level = GetAugEffectiveLevel(item_data->ID);
			const TransmuteReturn& tr = GetTransmuteReturn(aug_level);
			res.common_yield = tr.common;
			res.rare_yield   = tr.rare;
			res.transmuted   = true;
			results.push_back(res);
			continue;
		}

		// --- Normal item salvage ---

		// Determine tier from item ID
		int tier = ItemProgression::GetTierFromItemID(item_data->ID);

		// Mythic items cannot be salvaged
		if (tier >= 3) {
			res.mythic = true;
			results.push_back(res);
			continue;
		}

		// Calculate essence yield (handles magic gate internally)
		int yield = ItemProgression::CalculateEssenceYield(item_data, tier);
		if (yield <= 0) {
			res.rejected = true;
			results.push_back(res);
			continue;
		}

		res.common_yield = yield;

		// Rare Essence proc chance
		// Use iLevel as proxy for "named quality" — items above raid-tier threshold
		int ilevel = ItemProgression::CalculateILevel(item_data);
		float rare_chance = (ilevel >= RuleI(ItemProgression, RaidTierMinLevel))
			? RuleR(ItemProgression, SalvageRareChanceNamed)
			: RuleR(ItemProgression, SalvageRareChanceNormal);

		if (rare_chance > 0.0f && zone->random.Real(0.0, 1.0) < rare_chance) {
			int rare_min = RuleI(ItemProgression, SalvageRareAmountMin);
			int rare_max = RuleI(ItemProgression, SalvageRareAmountMax);
			res.rare_yield = (rare_min >= rare_max)
				? rare_min
				: zone->random.Int(rare_min, rare_max);
		}

		results.push_back(res);
	}

	// Check if satchel was empty
	if (results.empty()) {
		c->Message(Chat::Yellow, "Your Salvage Satchel is empty. Place items inside and try again.");
		return;
	}

	// Tally before deleting so we can report
	int total_common      = 0;
	int total_rare        = 0;
	int items_salvaged    = 0;
	int items_transmuted  = 0;
	int items_rejected    = 0;
	int items_mythic      = 0;

	for (const auto &res : results) {
		if (res.mythic) {
			items_mythic++;
		} else if (res.rejected) {
			items_rejected++;
		} else {
			if (res.transmuted) {
				items_transmuted++;
			} else {
				items_salvaged++;
			}
			total_common += res.common_yield;
			total_rare   += res.rare_yield;
		}
	}

	// Nothing actually salvageable or transmutable
	if (items_salvaged == 0 && items_transmuted == 0) {
		c->Message(Chat::Yellow, "No salvageable items found.");
		if (items_rejected > 0) {
			c->Message(Chat::Red, "%d item%s rejected (not magic).",
				items_rejected, items_rejected == 1 ? "" : "s");
		}
		if (items_mythic > 0) {
			c->Message(Chat::Red, "%d Mythic item%s cannot be salvaged.",
				items_mythic, items_mythic == 1 ? "" : "s");
		}
		return;
	}

	// Delete salvaged/transmuted items (reverse order to avoid slot shift issues)
	for (auto it = results.rbegin(); it != results.rend(); ++it) {
		if (it->rejected || it->mythic) {
			continue; // Leave rejected/mythic items in satchel
		}
		int16 abs_slot = EQ::InventoryProfile::CalcSlotId(container_slot, it->bag_slot);
		c->DeleteItemInInventory(abs_slot, 0, true);
	}

	// Award currencies
	if (total_common > 0) {
		c->AddAlternateCurrencyValue(common_currency_id, total_common);
	}
	if (total_rare > 0) {
		c->AddAlternateCurrencyValue(rare_currency_id, total_rare);
	}

	// Summary message
	int total_processed = items_salvaged + items_transmuted;
	std::string msg;

	if (items_salvaged > 0 && items_transmuted > 0) {
		msg = fmt::format(
			"Salvaged {} item{} and transmuted {} augment{} for {} Common Essence",
			items_salvaged, items_salvaged == 1 ? "" : "s",
			items_transmuted, items_transmuted == 1 ? "" : "s",
			total_common
		);
	} else if (items_transmuted > 0) {
		msg = fmt::format(
			"Transmuted {} augment{} into {} Common Essence",
			items_transmuted, items_transmuted == 1 ? "" : "s",
			total_common
		);
	} else {
		msg = fmt::format(
			"Salvaged {} item{} for {} Common Essence",
			items_salvaged, items_salvaged == 1 ? "" : "s",
			total_common
		);
	}

	if (total_rare > 0) {
		msg += fmt::format(" and {} Rare Essence", total_rare);
	}
	msg += ".";

	if (items_rejected > 0) {
		msg += fmt::format(" {} item{} rejected (not magic).",
			items_rejected, items_rejected == 1 ? "" : "s");
	}
	if (items_mythic > 0) {
		msg += fmt::format(" {} Mythic item{} returned (unsalvageable).",
			items_mythic, items_mythic == 1 ? "" : "s");
	}

	c->Message(Chat::Yellow, "%s", msg.c_str());

	// Per-item breakdown
	for (const auto &res : results) {
		if (res.rejected) {
			c->Message(Chat::Red, "  [REJECTED] %s - not magic", res.item_name.c_str());
		} else if (res.mythic) {
			c->Message(Chat::Red, "  [RETURNED] %s - Mythic items cannot be salvaged", res.item_name.c_str());
		} else if (res.transmuted) {
			std::string detail = fmt::format("  [TRANSMUTED] {} - {} Common", res.item_name, res.common_yield);
			if (res.rare_yield > 0) {
				detail += fmt::format(" + {} Rare", res.rare_yield);
			}
			c->Message(Chat::Yellow, "%s", detail.c_str());
		} else {
			std::string detail = fmt::format("  {} - {} Common", res.item_name, res.common_yield);
			if (res.rare_yield > 0) {
				detail += fmt::format(" + {} Rare!", res.rare_yield);
			}
			c->Message(Chat::Green, "%s", detail.c_str());
		}
	}
}
