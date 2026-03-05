/*	EQEmu: Everquest Server Emulator
	Copyright (C) 2001-2026 EQEmu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	Infinite Item Progression — Item Tier Scaling Engine
	See: game_design/infinite_progression/INFINITE_ITEM_PROGRESSION_DESIGN.md
	See: game_design/infinite_progression/IMPLEMENTATION_STEPS.md  (Step 2)
*/

#include "item_tier.h"
#include "rulesys.h"
#include "emu_constants.h"

#include <algorithm>
#include <cmath>

namespace ItemProgression {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool Is2HOrBow(uint8 item_type)
{
	using namespace EQ::item;
	switch (item_type) {
		case ItemType2HSlash:
		case ItemType2HBlunt:
		case ItemType2HPiercing:
		case ItemTypeBow:
			return true;
		default:
			return false;
	}
}

int GetTierAugSlotCount(int tier, bool is_2h_or_bow)
{
	if (is_2h_or_bow) {
		switch (tier) {
			case TierBase:      return RuleI(ItemProgression, AugSlots2HBase);
			case TierEnchanted: return RuleI(ItemProgression, AugSlots2HEnchanted);
			case TierLegendary: return RuleI(ItemProgression, AugSlots2HLegendary);
			case TierMythic:    return RuleI(ItemProgression, AugSlots2HMythic);
			default:            return RuleI(ItemProgression, AugSlots2HBase);
		}
	}
	else {
		switch (tier) {
			case TierBase:      return RuleI(ItemProgression, AugSlots1HBase);
			case TierEnchanted: return RuleI(ItemProgression, AugSlots1HEnchanted);
			case TierLegendary: return RuleI(ItemProgression, AugSlots1HLegendary);
			case TierMythic:    return RuleI(ItemProgression, AugSlots1HMythic);
			default:            return RuleI(ItemProgression, AugSlots1HBase);
		}
	}
}

const char* GetTierName(int tier)
{
	switch (tier) {
		case TierBase:      return "Base";
		case TierEnchanted: return "Enchanted";
		case TierLegendary: return "Legendary";
		case TierMythic:    return "Mythic";
		default:            return "Unknown";
	}
}

uint16 GetTierChatColor(int tier)
{
	// Chat:: constants from eq_constants.h
	switch (tier) {
		case TierBase:      return 0;   // Chat::White
		case TierEnchanted: return 4;   // Chat::LightBlue
		case TierLegendary: return 15;  // Chat::Yellow (gold)
		case TierMythic:    return 5;   // Chat::Magenta (purple-red)
		default:            return 0;
	}
}

// ---------------------------------------------------------------------------
// Tier Stat Scaling
// ---------------------------------------------------------------------------

void ApplyTierScaling(EQ::ItemData* scaledItem, const EQ::ItemData* baseItem, int tier)
{
	if (!scaledItem || !baseItem || tier <= TierBase) {
		return;
	}

	// Helper: clamp a scaled value into int8 range [-128, 127]
	auto clamp8 = [](double val) -> int8 {
		int v = static_cast<int>(std::round(val));
		return static_cast<int8>(std::clamp(v, -128, 127));
	};

	if (tier == TierEnchanted) {
		// ----------------------------------------------------------
		// Enchanted: all stats × mult, haste +bonus, spell/heal from INT/WIS
		// ----------------------------------------------------------
		double mult       = RuleR(ItemProgression, EnchantedMultiplier);
		int    haste_bonus = RuleI(ItemProgression, EnchantedHasteBonus);

		// Combat stats × mult
		scaledItem->Damage = static_cast<uint32>(std::round(baseItem->Damage * mult));
		scaledItem->AC     = static_cast<int32>(std::round(baseItem->AC * mult));
		scaledItem->HP     = static_cast<int32>(std::round(baseItem->HP * mult));
		scaledItem->Mana   = static_cast<int32>(std::round(baseItem->Mana * mult));
		scaledItem->Endur  = static_cast<int32>(std::round(baseItem->Endur * mult));

		// Attributes × mult
		scaledItem->AStr = clamp8(baseItem->AStr * mult);
		scaledItem->ASta = clamp8(baseItem->ASta * mult);
		scaledItem->AAgi = clamp8(baseItem->AAgi * mult);
		scaledItem->ADex = clamp8(baseItem->ADex * mult);
		scaledItem->AInt = clamp8(baseItem->AInt * mult);
		scaledItem->AWis = clamp8(baseItem->AWis * mult);
		scaledItem->ACha = clamp8(baseItem->ACha * mult);

		// Resists × mult
		scaledItem->MR = clamp8(baseItem->MR * mult);
		scaledItem->FR = clamp8(baseItem->FR * mult);
		scaledItem->CR = clamp8(baseItem->CR * mult);
		scaledItem->DR = clamp8(baseItem->DR * mult);
		scaledItem->PR = clamp8(baseItem->PR * mult);

		// Haste (additive bonus, only if item already has haste)
		if (baseItem->Haste > 0) {
			scaledItem->Haste = baseItem->Haste + haste_bonus;
		}

		// Regen × mult
		if (baseItem->Regen > 0) {
			scaledItem->Regen = static_cast<int32>(std::round(baseItem->Regen * mult));
		}
		if (baseItem->ManaRegen > 0) {
			scaledItem->ManaRegen = static_cast<int32>(std::round(baseItem->ManaRegen * mult));
		}
		if (baseItem->EnduranceRegen > 0) {
			scaledItem->EnduranceRegen = static_cast<int32>(std::round(baseItem->EnduranceRegen * mult));
		}

		// Spell Power appears if item has INT (= base INT)
		if (baseItem->AInt > 0) {
			scaledItem->SpellDmg = static_cast<int32>(baseItem->AInt);
		}

		// Heal Power appears if item has WIS (= base WIS)
		if (baseItem->AWis > 0) {
			scaledItem->HealAmt = static_cast<int32>(baseItem->AWis);
		}

		// Attack × mult (if present on base)
		if (baseItem->Attack > 0) {
			scaledItem->Attack = static_cast<int32>(std::round(baseItem->Attack * mult));
		}

		// DamageShield × mult
		if (baseItem->DamageShield > 0) {
			scaledItem->DamageShield = static_cast<int32>(std::round(baseItem->DamageShield * mult));
		}
	}
	else if (tier >= TierLegendary) {
		// ----------------------------------------------------------
		// Legendary & Mythic: combat ×2.6, attrs ×2, heroics = base 1:1,
		// haste +5, attack += base_dmg×2, spell/heal ×2, combat effects +1
		// Mythic has identical stats (only +1 aug slot, handled separately)
		// ----------------------------------------------------------
		double combat_mult = RuleR(ItemProgression, LegendaryCombatMultiplier);
		double attr_mult   = RuleR(ItemProgression, LegendaryAttributeMultiplier);
		int    haste_bonus  = RuleI(ItemProgression, LegendaryHasteBonus);
		int    ce_bonus     = RuleI(ItemProgression, LegendaryCombatEffectBonus);

		// Combat stats × combat_mult
		scaledItem->Damage = static_cast<uint32>(std::round(baseItem->Damage * combat_mult));
		scaledItem->AC     = static_cast<int32>(std::round(baseItem->AC * combat_mult));
		scaledItem->HP     = static_cast<int32>(std::round(baseItem->HP * combat_mult));
		scaledItem->Mana   = static_cast<int32>(std::round(baseItem->Mana * combat_mult));
		scaledItem->Endur  = static_cast<int32>(std::round(baseItem->Endur * combat_mult));

		// Attributes × attr_mult (stays at ×2 — their power bump comes from heroics)
		scaledItem->AStr = clamp8(baseItem->AStr * attr_mult);
		scaledItem->ASta = clamp8(baseItem->ASta * attr_mult);
		scaledItem->AAgi = clamp8(baseItem->AAgi * attr_mult);
		scaledItem->ADex = clamp8(baseItem->ADex * attr_mult);
		scaledItem->AInt = clamp8(baseItem->AInt * attr_mult);
		scaledItem->AWis = clamp8(baseItem->AWis * attr_mult);
		scaledItem->ACha = clamp8(baseItem->ACha * attr_mult);

		// Resists × attr_mult
		scaledItem->MR = clamp8(baseItem->MR * attr_mult);
		scaledItem->FR = clamp8(baseItem->FR * attr_mult);
		scaledItem->CR = clamp8(baseItem->CR * attr_mult);
		scaledItem->DR = clamp8(baseItem->DR * attr_mult);
		scaledItem->PR = clamp8(baseItem->PR * attr_mult);

		// Heroic stats = base attribute (1:1 with base)
		scaledItem->HeroicStr = static_cast<int32>(baseItem->AStr);
		scaledItem->HeroicSta = static_cast<int32>(baseItem->ASta);
		scaledItem->HeroicDex = static_cast<int32>(baseItem->ADex);
		scaledItem->HeroicAgi = static_cast<int32>(baseItem->AAgi);
		scaledItem->HeroicInt = static_cast<int32>(baseItem->AInt);
		scaledItem->HeroicWis = static_cast<int32>(baseItem->AWis);
		scaledItem->HeroicCha = static_cast<int32>(baseItem->ACha);

		// Haste (additive bonus, only if item already has haste)
		if (baseItem->Haste > 0) {
			scaledItem->Haste = baseItem->Haste + haste_bonus;
		}

		// Regen × combat_mult
		if (baseItem->Regen > 0) {
			scaledItem->Regen = static_cast<int32>(std::round(baseItem->Regen * combat_mult));
		}
		if (baseItem->ManaRegen > 0) {
			scaledItem->ManaRegen = static_cast<int32>(std::round(baseItem->ManaRegen * combat_mult));
		}
		if (baseItem->EnduranceRegen > 0) {
			scaledItem->EnduranceRegen = static_cast<int32>(std::round(baseItem->EnduranceRegen * combat_mult));
		}

		// Attack += base_damage × 2 (design doc: "attack_bonus = base_damage × 2.0")
		if (baseItem->Damage > 0) {
			scaledItem->Attack = static_cast<int32>(baseItem->Attack + baseItem->Damage * 2);
		}
		else if (baseItem->Attack > 0) {
			scaledItem->Attack = static_cast<int32>(std::round(baseItem->Attack * combat_mult));
		}

		// Spell Power = base INT × 2 (if item has INT)
		if (baseItem->AInt > 0) {
			scaledItem->SpellDmg = static_cast<int32>(baseItem->AInt) * 2;
		}

		// Heal Power = base WIS × 2 (if item has WIS)
		if (baseItem->AWis > 0) {
			scaledItem->HealAmt = static_cast<int32>(baseItem->AWis) * 2;
		}

		// DamageShield × combat_mult
		if (baseItem->DamageShield > 0) {
			scaledItem->DamageShield = static_cast<int32>(std::round(baseItem->DamageShield * combat_mult));
		}

		// Combat effect bonuses (small additive, only if base has the stat)
		if (baseItem->Shielding > 0) {
			scaledItem->Shielding = clamp8(static_cast<double>(baseItem->Shielding) + ce_bonus);
		}
		if (baseItem->StrikeThrough > 0) {
			scaledItem->StrikeThrough = clamp8(static_cast<double>(baseItem->StrikeThrough) + ce_bonus);
		}
		if (baseItem->StunResist > 0) {
			scaledItem->StunResist = clamp8(static_cast<double>(baseItem->StunResist) + ce_bonus);
		}
		if (baseItem->SpellShield > 0) {
			scaledItem->SpellShield = clamp8(static_cast<double>(baseItem->SpellShield) + ce_bonus);
		}
		if (baseItem->Avoidance > 0) {
			scaledItem->Avoidance = clamp8(static_cast<double>(baseItem->Avoidance) + ce_bonus);
		}
		if (baseItem->Accuracy > 0) {
			scaledItem->Accuracy = clamp8(static_cast<double>(baseItem->Accuracy) + ce_bonus);
		}
		if (baseItem->CombatEffects > 0) {
			scaledItem->CombatEffects = clamp8(static_cast<double>(baseItem->CombatEffects) + ce_bonus);
		}
	}
}

// ---------------------------------------------------------------------------
// Tier Aug Slots
// ---------------------------------------------------------------------------

void ApplyTierAugSlots(EQ::ItemData* scaledItem, const EQ::ItemData* baseItem, int tier)
{
	if (!scaledItem || !baseItem || tier < TierBase) {
		return;
	}

	bool is_2h = Is2HOrBow(baseItem->ItemType);
	int target_slots = GetTierAugSlotCount(tier, is_2h);

	// Count how many slots the base item already has open and find the last used type
	int base_slots = 0;
	uint8 last_base_type = 0;
	for (int i = 0; i < EQ::invaug::SOCKET_COUNT; ++i) {
		if (baseItem->AugSlotType[i] != 0) {
			++base_slots;
			last_base_type = baseItem->AugSlotType[i];
		}
	}

	// Use max of base count and tier count — never close slots that the base item has
	int final_slots = std::max(base_slots, target_slots);

	// Cap to SOCKET_COUNT (6 for RoF2)
	final_slots = std::min(final_slots, static_cast<int>(EQ::invaug::SOCKET_COUNT));

	// Determine how many slots Legendary would have (used to identify Mythic-specific slots)
	int legendary_slots = std::max(base_slots, GetTierAugSlotCount(TierLegendary, is_2h));
	legendary_slots = std::min(legendary_slots, static_cast<int>(EQ::invaug::SOCKET_COUNT));

	// Fallback type if no base slots exist
	uint8 fill_type = last_base_type > 0 ? last_base_type
	                                      : static_cast<uint8>(RuleI(ItemProgression, TierAugSlotType));

	for (int i = 0; i < EQ::invaug::SOCKET_COUNT; ++i) {
		if (i < final_slots) {
			// Preserve existing base slot types
			if (scaledItem->AugSlotType[i] == 0) {
				// Mythic-specific extra slots (beyond Legendary count) get type 4
				if (tier == TierMythic && i >= legendary_slots) {
					scaledItem->AugSlotType[i] = 4;
				}
				else {
					scaledItem->AugSlotType[i] = fill_type;
				}
			}
			// Ensure visible
			scaledItem->AugSlotVisible[i] = 1;
		}
		// Don't close slots that were open on the base item
	}
}

} // namespace ItemProgression
