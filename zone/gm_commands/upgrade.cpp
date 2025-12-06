/**
 * GM Command: #upgrade
 * Upgrades an equipped item by one level using the DynamicItemManager
 */

#include "../client.h"
#include "../command.h"
#include "../dynamic_item_manager.h"
#include "../../common/eqemu_logsys.h"

void command_upgrade(Client *c, const Seperator *sep)
{
	uint16 arguments = sep->argnum;

	// Syntax: #upgrade [slot_number] [levels]
	//   - If no slot: upgrade cursor item by 1 level
	//   - If slot: upgrade item in that equipment slot by N levels
	//   - Special: #upgrade all set <level>  -> set all equipped items to an absolute level

	int16 slot_id = EQ::invslot::slotCursor;  // Default to cursor
	int levels_to_add = 1;  // Default to 1 level
	bool upgrade_all = false;
	bool absolute_set = false;
	int absolute_level = 0;

	// Parse arguments
	if (arguments >= 1) {
		// check for "all set <level>"
		if (Strings::EqualFold(sep->arg[1], "all")) {
			upgrade_all = true;
			if (arguments >= 3 && Strings::EqualFold(sep->arg[2], "set") && sep->IsNumber(3)) {
				absolute_set = true;
				absolute_level = Strings::ToInt(sep->arg[3]);
			} else {
				c->Message(Chat::Red, "Usage: #upgrade all set <level>");
				return;
			}
		} else if (sep->IsNumber(1)) {
			slot_id = static_cast<int16>(Strings::ToInt(sep->arg[1]));
		}
	}

	if (!upgrade_all && arguments >= 2 && sep->IsNumber(2)) {
		levels_to_add = Strings::ToInt(sep->arg[2]);
	}

	// Validate slot range (allow cursor sentinel explicitly)
	if (!upgrade_all && slot_id != EQ::invslot::slotCursor && (slot_id < EQ::invslot::slotCharm || slot_id > EQ::invslot::slotAmmo)) {
		c->Message(
			Chat::Red,
			fmt::format(
				"Invalid slot ID: {}. Must be 0-21 for equipment slots or 9999 for cursor.",
				slot_id
			).c_str()
		);
		return;
	}

	// Get DynamicItemManager
	auto& mgr = EQ::DynamicItemManager::Get();

	auto upgrade_item = [&](int16 slot, int target_level, bool absolute) -> bool {
		auto* inst = c->GetInv().GetItem(slot);
		if (!inst) {
			return false;
		}
		const EQ::ItemData* item_data = inst->GetItem();
		if (!item_data) {
			return false;
		}

		uint32 current_id = inst->GetID();
		int current_level = mgr.GetItemLevel(current_id);
		uint32 base_id = mgr.GetBaseItemID(current_id);
		int new_level = absolute ? target_level : current_level + target_level;

		Log(Logs::General, Logs::Status,
			"Upgrade command: current_id=%u, current_level=%d, base_id=%u, new_level=%d",
			current_id, current_level, base_id, new_level);

		const EQ::ItemData* scaled_item = mgr.GenerateScaledItem(base_id, new_level);

		if (!scaled_item) {
			c->Message(Chat::Red, fmt::format("Failed to generate upgraded item (base ID: {}, level: {})", base_id, new_level).c_str());
			return false;
		}

		// Create new item instance with same charges/augments
		auto* new_inst = database.CreateItem(scaled_item, inst->GetCharges());
		if (!new_inst) {
			c->Message(Chat::Red, "Failed to create upgraded item instance.");
			return false;
		}

		// Copy augments
		for (int aug_slot = EQ::invaug::SOCKET_BEGIN; aug_slot <= EQ::invaug::SOCKET_END; ++aug_slot) {
			auto* aug = inst->GetAugment(aug_slot);
			if (aug) {
				new_inst->PutAugment(&database, aug_slot, *aug);
			}
		}

		// Copy attuned status
		if (inst->IsAttuned()) {
			new_inst->SetAttuned(true);
		}

		// Replace the item
		c->DeleteItemInInventory(slot, 0, true);
		c->PutItemInInventory(slot, *new_inst, true);

		c->Message(Chat::Yellow, fmt::format("Your {} has been upgraded! ({} +{} -> +{})",
			item_data->Name, item_data->Name, current_level, new_level).c_str());

		safe_delete(new_inst);
		return true;
	};

	if (upgrade_all) {
		int successes = 0;
		for (int16 slot = EQ::invslot::slotCharm; slot <= EQ::invslot::slotAmmo; ++slot) {
			if (upgrade_item(slot, absolute_level, true)) {
				++successes;
			}
		}
		c->Message(Chat::Yellow, fmt::format("Upgraded {} equipped item(s) to level {}.", successes, absolute_level).c_str());
		return;
	}

	// single-slot path
	if (!upgrade_item(slot_id, levels_to_add, false)) {
		return;
	}

	Log(Logs::General, Logs::Status,
		"Upgrade complete: %s upgraded slot %d by %d level(s)",
		c->GetName(), slot_id, levels_to_add);
}
