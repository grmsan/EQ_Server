/*	EQEmu: Everquest Server Emulator
	Copyright (C) 2001-2026 EQEmu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	Infinite Item Progression — iLevel Calculation Engine
	See: game_design/infinite_progression/ITEM_LEVEL_SYSTEM.md
*/

#include "item_ilevel.h"
#include "rulesys.h"

#include <algorithm>
#include <cmath>

namespace ItemProgression {

bool IsWeaponType(uint8 item_type)
{
	using namespace EQ::item;
	switch (item_type) {
		case ItemType1HSlash:
		case ItemType2HSlash:
		case ItemType1HPiercing:
		case ItemType1HBlunt:
		case ItemType2HBlunt:
		case ItemTypeBow:
		case ItemTypeMartial:
		case ItemType2HPiercing:
			return true;
		default:
			return false;
	}
}

int CalculateILevel(const EQ::ItemData* item)
{
	if (!item) {
		return 1;
	}

	double power = 0.0;

	// Sum base attributes
	double attrs = static_cast<double>(item->AStr) + item->ASta + item->AAgi
	             + item->ADex + item->AWis + item->AInt + item->ACha;

	// Sum heroic stats
	double heroics = static_cast<double>(item->HeroicStr) + item->HeroicSta
	               + item->HeroicAgi + item->HeroicDex + item->HeroicWis
	               + item->HeroicInt + item->HeroicCha;

	if (IsWeaponType(item->ItemType)) {
		// Weapon formula — DPS is the primary driver
		double dps = 0.0;
		if (item->Delay > 0) {
			dps = (static_cast<double>(item->Damage) * 100.0)
			    / static_cast<double>(item->Delay);
		}

		double w_ac     = RuleR(ItemProgression, WeaponACWeight);
		double w_hp     = RuleR(ItemProgression, WeaponHPWeight);
		double w_mana   = RuleR(ItemProgression, WeaponManaWeight);
		double w_attr   = RuleR(ItemProgression, WeaponAttrWeight);
		double w_haste  = RuleR(ItemProgression, WeaponHasteWeight);
		double w_heroic = RuleR(ItemProgression, WeaponHeroicWeight);

		power = dps
		      + (item->AC * w_ac)
		      + (item->HP * w_hp)
		      + (item->Mana * w_mana)
		      + (attrs * w_attr)
		      + item->Attack
		      + (item->Haste * w_haste)
		      + (heroics * w_heroic);
	}
	else {
		// Armor formula — AC is the primary driver
		double resists = static_cast<double>(item->MR) + item->FR + item->CR
		               + item->DR + item->PR;

		double a_ac     = RuleR(ItemProgression, ArmorACWeight);
		double a_hp     = RuleR(ItemProgression, ArmorHPWeight);
		double a_mana   = RuleR(ItemProgression, ArmorManaWeight);
		double a_attr   = RuleR(ItemProgression, ArmorAttrWeight);
		double a_resist = RuleR(ItemProgression, ArmorResistWeight);
		double a_heroic = RuleR(ItemProgression, ArmorHeroicWeight);

		power = (item->AC * a_ac)
		      + (item->HP * a_hp)
		      + (item->Mana * a_mana)
		      + (attrs * a_attr)
		      + (resists * a_resist)
		      + (heroics * a_heroic);
	}

	return std::max(1, static_cast<int>(std::floor(power)));
}

int CalculateEssenceYield(const EQ::ItemData* item, int tier)
{
	if (!item) {
		return 0;
	}

	// Gate 1: Only magic items can be salvaged
	if (RuleB(ItemProgression, SalvageRequireMagic) && !item->Magic) {
		return 0;
	}

	// Gate 2: Offset subtracted from iLevel, minimum 1
	int ilevel = CalculateILevel(item);
	int offset = RuleI(ItemProgression, EssenceOffset);
	int base_essence = std::max(1, ilevel - offset);

	// Tier bonus
	double tier_bonus = 1.0;
	switch (tier) {
		case 0: tier_bonus = 1.0; break;
		case 1: tier_bonus = RuleR(ItemProgression, SalvageTierBonusEnchanted); break;
		case 2: tier_bonus = RuleR(ItemProgression, SalvageTierBonusLegendary); break;
		case 3: return 0; // Mythic items cannot be salvaged
		default: break;
	}

	return std::max(1, static_cast<int>(std::round(base_essence * tier_bonus)));
}

int CalculateTierCost(int ilevel, int target_tier)
{
	// target_tier: 1 = Base→Enchanted, 2 = Enchanted→Legendary, 3 = Legendary→Mythic
	int    tier_floor = 0;
	double tier_scale = 0.0;

	switch (target_tier) {
		case 1:
			tier_floor = RuleI(ItemProgression, TierFloorBaseToEnchanted);
			tier_scale = RuleR(ItemProgression, TierScaleBaseToEnchanted);
			break;
		case 2:
			tier_floor = RuleI(ItemProgression, TierFloorEnchantedToLegendary);
			tier_scale = RuleR(ItemProgression, TierScaleEnchantedToLegendary);
			break;
		case 3:
			tier_floor = RuleI(ItemProgression, TierFloorLegendaryToMythic);
			tier_scale = RuleR(ItemProgression, TierScaleLegendaryToMythic);
			break;
		default:
			return 0;
	}

	double base_cost = tier_floor + (static_cast<double>(ilevel) * ilevel) * tier_scale;
	return std::max(1, static_cast<int>(std::round(base_cost)));
}

} // namespace ItemProgression
