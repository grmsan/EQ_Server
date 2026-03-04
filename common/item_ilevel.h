/*	EQEmu: Everquest Server Emulator
	Copyright (C) 2001-2026 EQEmu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	Infinite Item Progression — iLevel Calculation Engine
	See: game_design/infinite_progression/ITEM_LEVEL_SYSTEM.md
*/

#ifndef COMMON_ITEM_ILEVEL_H
#define COMMON_ITEM_ILEVEL_H

#include "item_data.h"

namespace ItemProgression {

	// Calculate the iLevel (power score) for any item.
	// Weapons use DPS + weighted combat stats.
	// Armor uses AC + HP/Mana + weighted non-combat stats.
	// Returns max(1, floor(power_score)).
	// Weights are read from Rules(ItemProgression, *).
	int CalculateILevel(const EQ::ItemData* item);

	// Calculate Essence yield from salvaging this item.
	// Gate 1: item must have Magic flag set (returns 0 otherwise).
	// Gate 2: max(1, iLevel - ESSENCE_OFFSET) × tier_bonus × era_mult.
	// tier: 0=Base, 1=Enchanted, 2=Legendary, 3=Mythic (unsalvageable).
	int CalculateEssenceYield(const EQ::ItemData* item, int tier = 0);

	// Calculate Essence cost to tier up an item.
	// Uses iLevel² power curve: floor + round(iLevel² × scale).
	// target_tier: 1=B→E, 2=E→L, 3=L→M.
	int CalculateTierCost(int ilevel, int target_tier);

	// Helper: returns true if the item type is any weapon (1H, 2H, bow, martial).
	bool IsWeaponType(uint8 item_type);

} // namespace ItemProgression

#endif // COMMON_ITEM_ILEVEL_H
