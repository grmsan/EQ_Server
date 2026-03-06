#include "../client.h"
#include "../salvage.h"
#include "../../common/rulesys.h"

/*
 * #salvage — GM/admin command to manually trigger salvage processing.
 *
 * Normally players use the Salvage Satchel's Combine button (which hooks
 * into HandleCombine).  This command exists for admin testing.
 */

void command_salvage(Client *c, const Seperator *sep)
{
	const uint32 satchel_item_id = RuleI(ItemProgression, SalvageSatchelItemID);

	// Find the Salvage Satchel in general inventory
	EQ::InventoryProfile &inv = c->GetInv();

	for (int16 slot = EQ::invslot::GENERAL_BEGIN; slot <= EQ::invslot::GENERAL_END; slot++) {
		const EQ::ItemInstance *inst = inv.GetItem(slot);
		if (inst && inst->GetItem() && inst->GetItem()->ID == satchel_item_id) {
			Salvage::ProcessSatchel(c, slot);
			return;
		}
	}

	c->Message(Chat::Red, "You do not have a Salvage Satchel in your inventory.");
	c->Message(Chat::Yellow, "The Salvage Satchel (item %u) must be in a general inventory slot.", satchel_item_id);
}
