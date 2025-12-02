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
	// If no slot: upgrade cursor item
	// If slot: upgrade item in that equipment slot
	// Levels defaults to 1

	int16 slot_id = EQ::invslot::slotCursor;  // Default to cursor
	int levels_to_add = 1;  // Default to 1 level

	// Parse arguments
	if (arguments >= 1 && sep->IsNumber(1)) {
		slot_id = static_cast<int16>(Strings::ToInt(sep->arg[1]));
	}

	if (arguments >= 2 && sep->IsNumber(2)) {
		levels_to_add = Strings::ToInt(sep->arg[2]);
	}

	// Validate slot range
	if (slot_id < EQ::invslot::slotCharm || slot_id > EQ::invslot::slotAmmo) {
		c->Message(
			Chat::Red,
			fmt::format(
				"Invalid slot ID: {}. Must be 0-21 for equipment slots or 9999 for cursor.",
				slot_id
			).c_str()
		);
		return;
	}

	// Get item from slot
	auto* inst = c->GetInv().GetItem(slot_id);
	if (!inst) {
		c->Message(
			Chat::Red,
			fmt::format(
				"No item found in slot {}.",
				slot_id
			).c_str()
		);
		return;
	}

	const EQ::ItemData* item_data = inst->GetItem();
	if (!item_data) {
		c->Message(Chat::Red, "Item data not found.");
		return;
	}

	// Get DynamicItemManager
	auto& mgr = EQ::DynamicItemManager::Get();

	// Determine current level and base ID
	uint32 current_id = inst->GetID();
	int current_level = mgr.GetItemLevel(current_id);
	uint32 base_id = mgr.GetBaseItemID(current_id);
	int new_level = current_level + levels_to_add;

	Log(Logs::General, Logs::Status,
		"Upgrade command: current_id=%u, current_level=%d, base_id=%u, new_level=%d",
		current_id, current_level, base_id, new_level);

	// Generate new dynamic item
	uint32 new_item_id = mgr.GenerateDynamicID(base_id, new_level);
	const EQ::ItemData* scaled_item = mgr.GenerateScaledItem(base_id, new_level);

	if (!scaled_item) {
		c->Message(
			Chat::Red,
			fmt::format(
				"Failed to generate upgraded item (base ID: {}, level: {})",
				base_id,
				new_level
			).c_str()
		);
		return;
	}

	// Create new item instance with same charges/augments
	auto* new_inst = database.CreateItem(
		scaled_item,
		inst->GetCharges()
	);

	if (!new_inst) {
		c->Message(Chat::Red, "Failed to create upgraded item instance.");
		return;
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

	// Delete old item and put new one in slot
	c->DeleteItemInInventory(slot_id, 0, true);
	c->PutItemInInventory(slot_id, *new_inst, true);

	// Notify player
	c->Message(
		Chat::Yellow,
		fmt::format(
			"Your {} has been upgraded! ({} +{} → +{})",
			item_data->Name,
			item_data->Name,
			current_level,
			new_level
		).c_str()
	);

	Log(Logs::General, Logs::Status,
		"Upgrade complete: %s upgraded %s from level %d to %d (slot %d)",
		c->GetName(), item_data->Name, current_level, new_level, slot_id);

	safe_delete(new_inst);
}
