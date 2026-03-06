#ifndef ZONE_GHOST_COPY_H
#define ZONE_GHOST_COPY_H

#include <cstdint>

class Client;

namespace EQ {
	struct ItemData;
}

// Step 6 — Ghost Copy System
//
// When a "progression item" (weapon, armor, etc.) sits in the Power Source slot,
// a ghost copy of that item is placed into the item's native equipment slot so
// the client can see and use the weapon/armor while it levels up in the Power Source.
//
// Ghost copies are NOT saved to the database — they are regenerated on
// login/zone‑in via UpdateGhostCopy() called from Client::CalcBonuses().
//
// The Power Source slot itself never contributes stats for progression items;
// all stats come from the ghost copy in the native equipment slot.
//
// Picking up from either the Power Source slot or the ghost slot removes BOTH
// copies and places the original Power Source item on the cursor.

namespace GhostCopy {

	// Returns true if the item has equippable slots beyond just Power Source.
	// Real power-source-only items return false and are handled normally.
	bool IsProgressionItem(const EQ::ItemData* item);

	// Scans the PS item's Slots bitmask for an empty equipment slot.
	// Returns the first eligible slot_id, or -1 if none.
	// Handles 2H weapons (requires both Primary + Secondary empty).
	int16 FindTargetSlot(Client* c);

	// Master update: places or removes ghost copies as needed.
	// Called from Client::CalcBonuses() after all bonuses are computed.
	void UpdateGhostCopy(Client* c);

	// Intercept handler called early in Client::SwapItem().
	// Returns true if the move was fully handled (caller should return).
	bool HandleMoveItem(Client* c, int16 from_slot, int16 to_slot);

	// Remove ghost copy if one exists.  Safe to call anytime.
	void RemoveGhostCopy(Client* c);
}

#endif // ZONE_GHOST_COPY_H
