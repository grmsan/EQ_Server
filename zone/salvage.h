#ifndef SALVAGE_H
#define SALVAGE_H

class Client;

/*
 * Salvage System — Step 4
 *
 * Core salvage logic for the Salvage Satchel.  Called from:
 *   1. HandleCombine() when the player clicks "Combine" on the Satchel
 *   2. #salvage GM command (for testing / admin override)
 *
 * container_slot: the inventory slot of the Salvage Satchel container
 */
namespace Salvage {
	// Process all items in the satchel at the given slot. Deletes consumed items,
	// awards Essence currencies, and sends feedback messages to the client.
	void ProcessSatchel(Client *c, int16 container_slot);
}

#endif // SALVAGE_H
