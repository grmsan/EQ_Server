/*	EQEmu: Everquest Server Emulator
	Infinite Item Progression — Consume Item / Consume Essence AAs (Step 5)
	See: game_design/infinite_progression/IMPLEMENTATION_STEPS.md

	Two AA-activated abilities:
	  1. Consume Item  — cursor item must match Power Slot base ID.
	     XP granted based on tier comparison.
	  2. Consume Essence — spends Common Essence to add XP to Power Slot item.
	     Only consumes what is needed (or all balance if not enough for full tier).
*/

#ifndef ZONE_CONSUME_SYSTEM_H
#define ZONE_CONSUME_SYSTEM_H

class Client;

namespace ConsumeSystem {

	// Called when the Consume Item AA is activated.
	// Expects a matching item on the player's cursor.
	void HandleConsumeItem(Client* c);

	// Called when the Consume Essence AA is activated.
	// Automatically spends Common Essence from the player's balance.
	void HandleConsumeEssence(Client* c);

} // namespace ConsumeSystem

#endif // ZONE_CONSUME_SYSTEM_H
