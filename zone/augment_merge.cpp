/*
 * augment_merge.cpp — Augment Forgemaster merge system (Step 9)
 *
 * Intercepts the "Combine" action on the Augment Forge container,
 * validates 3 matching augs + 1 correct catalyst, and produces
 * the next-level augment.
 *
 * Design reference: game_design/infinite_progression/AUGMENT_SYSTEM.md §6
 */

#include "augment_merge.h"
#include "augment_merge_data.h"  // auto-generated ID constants

#include "client.h"
#include "zonedb.h"
#include "../common/item_instance.h"
#include "../common/inventory_profile.h"
#include "../common/repositories/items_repository.h"

#include <vector>

extern Zone* zone;

void AugmentMerge::ProcessForgemaster(Client* user, int16 container_slot)
{
	if (!user) {
		return;
	}

	EQ::InventoryProfile& inv = user->GetInv();
	const EQ::ItemInstance* container = inv.GetItem(container_slot);

	if (!container || !container->GetItem()) {
		user->Message(Chat::Red, "Merge error: container not found.");
		return;
	}

	if (!container->IsType(EQ::item::ItemClassBag)) {
		user->Message(Chat::Red, "Merge error: not a valid container.");
		return;
	}

	// Collect all items in the 4-slot container
	struct SlotInfo {
		uint8   bag_slot;
		uint32  item_id;
		int     family;     // aug family index (-1 if catalyst or unknown)
		int     level;      // aug merge level (-1 if not an aug)
		int     cat_tier;   // catalyst target tier (0 if not a catalyst)
	};

	std::vector<SlotInfo> augs;
	SlotInfo catalyst = {0, 0, -1, -1, 0};
	bool found_catalyst = false;
	int total_items = 0;

	const int bag_slots = container->GetItem()->BagSlots;
	for (uint8 i = EQ::invbag::SLOT_BEGIN; i < bag_slots && i <= EQ::invbag::SLOT_END; i++) {
		const EQ::ItemInstance* slot_inst = container->GetItem(i);
		if (!slot_inst || !slot_inst->GetItem()) {
			continue;
		}

		total_items++;
		uint32 item_id = slot_inst->GetItem()->ID;

		int family = -1, level = -1;
		int cat_tier = AugMergeData::GetCatalystTier(item_id);

		if (cat_tier > 0) {
			// This is a catalyst
			if (found_catalyst) {
				user->Message(Chat::Red, "Only one Merge Catalyst is allowed per combine.");
				return;
			}
			catalyst = {i, item_id, -1, -1, cat_tier};
			found_catalyst = true;
		}
		else if (AugMergeData::GetAugInfo(item_id, family, level)) {
			augs.push_back({i, item_id, family, level, 0});
		}
		else {
			user->Message(Chat::Red, "'%s' is not a valid augment or catalyst.",
				slot_inst->GetItem()->Name);
			return;
		}
	}

	// Validate: exactly 4 items (3 augs + 1 catalyst)
	if (total_items != 4) {
		user->Message(Chat::Yellow,
			"Place exactly 3 matching augments and 1 Merge Catalyst in the forge.");
		return;
	}

	if (!found_catalyst) {
		user->Message(Chat::Red, "Merge requires a Merge Catalyst in the fourth slot.");
		return;
	}

	if (augs.size() != 3) {
		user->Message(Chat::Red,
			"Merge requires exactly 3 augments and 1 catalyst. Found %d augment(s).",
			static_cast<int>(augs.size()));
		return;
	}

	// Validate: all 3 augs are the same family and same level
	int merge_family = augs[0].family;
	int merge_level  = augs[0].level;

	for (size_t i = 1; i < augs.size(); i++) {
		if (augs[i].family != merge_family || augs[i].level != merge_level) {
			user->Message(Chat::Red,
				"Augments must be the same type and level to merge.");
			return;
		}
	}

	// Validate: not already at max level
	if (merge_level >= AugMergeData::MAX_AUG_LEVEL) {
		user->Message(Chat::Red,
			"These augments are already at maximum level (%d). They cannot be merged further.",
			AugMergeData::MAX_AUG_LEVEL);
		return;
	}

	int target_level = merge_level + 1;

	// Validate: catalyst tier matches the target level
	if (catalyst.cat_tier != target_level) {
		// Tell them which catalyst they need
		uint32 needed_cat_id = AugMergeData::GetRequiredCatalyst(merge_level);
		const EQ::ItemData* needed = database.GetItem(needed_cat_id);
		const char* needed_name = needed ? needed->Name : "a higher-tier catalyst";

		user->Message(Chat::Red,
			"This catalyst is not the right tier. You need '%s' to merge Level %d → %d.",
			needed_name, merge_level, target_level);
		return;
	}

	// Determine the output item
	uint32 output_item_id = AugMergeData::GetItemID(merge_family, target_level);
	if (output_item_id == 0) {
		user->Message(Chat::Red, "Merge error: could not determine output item.");
		LogError("AugmentMerge: GetItemID returned 0 for family={} level={}", merge_family, target_level);
		return;
	}

	// Verify output item exists in DB
	const EQ::ItemData* output_item = database.GetItem(output_item_id);
	if (!output_item) {
		user->Message(Chat::Red, "Merge error: output item %u not found in database.", output_item_id);
		LogError("AugmentMerge: item {} not found in database", output_item_id);
		return;
	}

	// Get the name of the input aug for the success message
	const EQ::ItemInstance* first_aug_inst = container->GetItem(augs[0].bag_slot);
	std::string input_name = first_aug_inst ? first_aug_inst->GetItem()->Name : "augment";

	// --- All validation passed. Consume inputs and produce output. ---

	// Delete all 4 items from the container (reverse order for safety)
	std::vector<uint8> slots_to_delete;
	for (const auto& aug : augs) {
		slots_to_delete.push_back(aug.bag_slot);
	}
	slots_to_delete.push_back(catalyst.bag_slot);

	// Sort descending so higher slots are deleted first
	std::sort(slots_to_delete.begin(), slots_to_delete.end(), std::greater<uint8>());

	for (uint8 bag_slot : slots_to_delete) {
		int16 abs_slot = EQ::InventoryProfile::CalcSlotId(container_slot, bag_slot);
		user->DeleteItemInInventory(abs_slot, 0, true);
	}

	// Summon the output item to cursor
	user->SummonItem(output_item_id, 1);

	// Success message
	user->Message(Chat::Yellow,
		"The forge glows brightly! Your augments have been merged into %s.",
		output_item->Name);

	LogInfo("AugmentMerge: [{}] merged 3x family={} lv={} + catalyst={} -> item={} ({})",
		user->GetName(), merge_family, merge_level, catalyst.item_id,
		output_item_id, output_item->Name);
}
