/*	EQEmu: Everquest Server Emulator
	Copyright (C) 2001-2026 EQEmu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	Infinite Item Progression — Item Tier Scaling Engine
	See: game_design/infinite_progression/INFINITE_ITEM_PROGRESSION_DESIGN.md
	See: game_design/infinite_progression/IMPLEMENTATION_STEPS.md  (Step 2)
*/

#ifndef COMMON_ITEM_TIER_H
#define COMMON_ITEM_TIER_H

#include "item_data.h"

namespace ItemProgression {

	// ----- Tier constants -----
	enum ItemTier : int {
		TierBase      = 0,
		TierEnchanted = 1,
		TierLegendary = 2,
		TierMythic    = 3,
		TierMax       = 3
	};

	// ID offset between tiers.  Enchanted = base + 250K, Legendary = base + 500K, Mythic = base + 750K.
	// Kept within 20-bit item-link mask (0xFFFFF = 1,048,575) so chat links work.
	//
	// CONSTRAINT: Base item IDs must be < 298,575 (hard limit enforced by generator).
	// Mythic ceiling: base_id + 3 * 250,000 must be <= 1,048,575  →  max base_id = 298,575.
	// The generator (tools/generate_tiered_items.py) enforces this and skips out-of-range items.
	// If the base item pool ever grows past 250K IDs, TIER_ID_OFFSET must be revisited.
	constexpr uint32 TIER_ID_OFFSET = 250'000;

	// ----- ID mapping helpers (DB-backed tier items) -----

	// Convert a base item ID + tier to the DB row ID for that tier.
	// Returns the base_id unchanged if tier == TierBase.
	inline uint32 GetTieredItemID(uint32 base_id, int tier) {
		return base_id + static_cast<uint32>(tier) * TIER_ID_OFFSET;
	}

	// Extract the original base item ID from any tiered item ID.
	inline uint32 GetBaseItemID(uint32 item_id) {
		return item_id % TIER_ID_OFFSET;
	}

	// Return the tier (0-3) encoded in an item ID.
	inline int GetTierFromItemID(uint32 item_id) {
		int t = static_cast<int>(item_id / TIER_ID_OFFSET);
		return (t >= 0 && t <= TierMax) ? t : 0;
	}

	// True if the item ID belongs to a generated tiered row (tier >= 1).
	inline bool IsTieredItem(uint32 item_id) {
		return item_id >= TIER_ID_OFFSET;
	}

	// Apply tier-based stat scaling to a scaled item copy.
	// scaledItem should already be a memcpy of baseItem before calling.
	// Modifies scaledItem in-place with multiplicative stat changes.
	//
	// Tier formulas (defaults, loaded from Rules):
	//   Enchanted : all stats ×2.0, haste +3, SpellDmg = base_INT, HealAmt = base_WIS
	//   Legendary : combat (DMG/AC/HP/Mana/Endur) ×2.6, attrs ×2, heroics = base 1:1,
	//               haste +5, attack += base_dmg×2, SpellDmg = INT×2, HealAmt = WIS×2,
	//               small combat-effect bonus (+1 shielding/strikethrough/etc.)
	//   Mythic    : same stats as Legendary (no increase; only +1 aug slot)
	void ApplyTierScaling(EQ::ItemData* scaledItem, const EQ::ItemData* baseItem, int tier);

	// Apply tier-based augment slot adjustments to a scaled item.
	// Opens additional aug slots according to tier, preserving existing slot types.
	void ApplyTierAugSlots(EQ::ItemData* scaledItem, const EQ::ItemData* baseItem, int tier);

	// Returns true if this item type is a 2H weapon or Bow (gets double aug slots).
	bool Is2HOrBow(uint8 item_type);

	// Get the number of aug slots for a given tier and item type.
	int GetTierAugSlotCount(int tier, bool is_2h_or_bow);

	// Get the display name for a tier.
	const char* GetTierName(int tier);

	// Get the chat color code for a tier (Chat:: constant value).
	uint16 GetTierChatColor(int tier);

} // namespace ItemProgression

#endif // COMMON_ITEM_TIER_H
