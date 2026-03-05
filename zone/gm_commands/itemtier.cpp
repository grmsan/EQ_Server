#include "../client.h"
#include "../../common/item_tier.h"
#include "../../common/item_ilevel.h"
#include "../../common/rulesys.h"

/*
 * #itemtier                    — show tier info for all equipped items on target (or self)
 * #itemtier <slot_id> <tier>   — set tier (0-3) on item in the given equipment slot
 *
 * DB-backed approach: each tier is a real item row in the database.
 *   ID scheme: Enchanted = base + 250,000  |  Legendary = base + 500,000  |  Mythic = base + 750,000
 * When changing tiers, the command deletes the old item and summons the tiered version,
 * preserving augments.
 */

void command_itemtier(Client *c, const Seperator *sep)
{
	// ---- set tier on a slot ----
	if (sep->arg[1][0] != '\0' && sep->arg[2][0] != '\0') {
		int slot_id = Strings::ToInt(sep->arg[1]);
		int tier    = Strings::ToInt(sep->arg[2]);

		if (tier < ItemProgression::TierBase || tier > ItemProgression::TierMax) {
			c->Message(Chat::Red, "ITEMTIER | Tier must be 0 (Base), 1 (Enchanted), 2 (Legendary), or 3 (Mythic).");
			return;
		}

		Client *target = c;
		if (c->GetTarget() && c->GetTarget()->IsClient()) {
			target = c->GetTarget()->CastToClient();
		}

		EQ::ItemInstance *inst = target->GetInv().GetItem(slot_id);
		if (!inst || !inst->GetItem()) {
			c->Message(Chat::Red, fmt::format("ITEMTIER | No item in slot {}.", slot_id).c_str());
			return;
		}

		// Determine the base item ID (strip any existing tier offset)
		uint32 current_id = inst->GetItem()->ID;
		uint32 base_id    = ItemProgression::GetBaseItemID(current_id);
		int    old_tier   = ItemProgression::GetTierFromItemID(current_id);

		if (old_tier == tier) {
			c->Message(Chat::White, fmt::format(
				"ITEMTIER | '{}' is already tier {} ({}).",
				inst->GetItem()->Name,
				tier,
				ItemProgression::GetTierName(tier)
			).c_str());
			return;
		}

		// Calculate new item ID
		uint32 new_id = ItemProgression::GetTieredItemID(base_id, tier);

		// Verify the new item exists in the database
		const EQ::ItemData *new_item_data = database.GetItem(new_id);
		if (!new_item_data) {
			c->Message(Chat::Red, fmt::format(
				"ITEMTIER | Tiered item ID {} not found in database. Run generate_tiered_items.py first.",
				new_id
			).c_str());
			return;
		}

		// Preserve augments from the current item
		uint32 aug_ids[EQ::invaug::SOCKET_COUNT] = {};
		for (int i = 0; i < EQ::invaug::SOCKET_COUNT; ++i) {
			aug_ids[i] = inst->GetAugmentItemID(i);
		}

		// Preserve attuned state
		bool is_attuned = inst->IsAttuned();

		// Get base item data for iLevel display
		const EQ::ItemData *base_data = database.GetItem(base_id);
		int ilevel = base_data ? ItemProgression::CalculateILevel(base_data) : 0;

		// Delete the old item from the slot
		target->DeleteItemInInventory(slot_id, 0, true, true);

		// Summon the new tiered item into the same slot with augments preserved
		target->SummonItem(
			new_id,
			-1,                // charges (default)
			aug_ids[0],        // aug1
			aug_ids[1],        // aug2
			aug_ids[2],        // aug3
			aug_ids[3],        // aug4
			aug_ids[4],        // aug5
			aug_ids[5],        // aug6
			is_attuned,        // attuned
			static_cast<uint16>(slot_id)  // target slot
		);

		// Report the change
		c->Message(
			ItemProgression::GetTierChatColor(tier),
			fmt::format(
				"ITEMTIER | [{}] '{}' tier: {} -> {} (iLevel {})  =>  new ID: {} '{}'",
				base_id,
				base_data ? base_data->Name : "?",
				ItemProgression::GetTierName(old_tier),
				ItemProgression::GetTierName(tier),
				ilevel,
				new_id,
				new_item_data->Name
			).c_str()
		);

		// Show stat comparison (base vs new tier)
		if (base_data && tier > 0) {
			c->Message(
				Chat::White,
				fmt::format(
					"  DMG: {} -> {}  |  AC: {} -> {}  |  HP: {} -> {}  |  Mana: {} -> {}",
					base_data->Damage, new_item_data->Damage,
					base_data->AC, new_item_data->AC,
					base_data->HP, new_item_data->HP,
					base_data->Mana, new_item_data->Mana
				).c_str()
			);

			c->Message(
				Chat::White,
				fmt::format(
					"  STR: {} -> {}  |  STA: {} -> {}  |  AGI: {} -> {}  |  DEX: {} -> {}",
					base_data->AStr, new_item_data->AStr,
					base_data->ASta, new_item_data->ASta,
					base_data->AAgi, new_item_data->AAgi,
					base_data->ADex, new_item_data->ADex
				).c_str()
			);

			if (tier >= ItemProgression::TierLegendary) {
				c->Message(
					Chat::Yellow,
					fmt::format(
						"  Heroic STR: {}  |  STA: {}  |  AGI: {}  |  DEX: {}  |  INT: {}  |  WIS: {}  |  CHA: {}",
						new_item_data->HeroicStr, new_item_data->HeroicSta, new_item_data->HeroicAgi,
						new_item_data->HeroicDex, new_item_data->HeroicInt, new_item_data->HeroicWis,
						new_item_data->HeroicCha
					).c_str()
				);
			}

			if (new_item_data->SpellDmg > 0 || new_item_data->HealAmt > 0 || new_item_data->Attack > 0) {
				c->Message(
					Chat::White,
					fmt::format(
						"  SpellDmg: {}  |  HealAmt: {}  |  Attack: {}",
						new_item_data->SpellDmg, new_item_data->HealAmt, new_item_data->Attack
					).c_str()
				);
			}

			if (new_item_data->Haste > 0) {
				c->Message(
					Chat::White,
					fmt::format("  Haste: {} -> {}", base_data->Haste, new_item_data->Haste).c_str()
				);
			}
		}

		// Log preserved augments
		for (int i = 0; i < EQ::invaug::SOCKET_COUNT; ++i) {
			if (aug_ids[i] > 0) {
				c->Message(Chat::White, fmt::format("  Augment slot {}: preserved (ID {})", i + 1, aug_ids[i]).c_str());
			}
		}

		target->Save();
		return;
	}

	// ---- list mode: show tier info for all equipped items ----
	Client *target = c;
	if (c->GetTarget() && c->GetTarget()->IsClient()) {
		target = c->GetTarget()->CastToClient();
	}

	c->Message(Chat::White, "ITEMTIER | Equipped Item Tier Summary:");
	c->Message(Chat::White, "  Slot | Tier       | iLevel | Name");
	c->Message(Chat::White, "  -----|------------|--------|--------------------");

	int count = 0;
	for (int slot = EQ::invslot::EQUIPMENT_BEGIN; slot <= EQ::invslot::EQUIPMENT_END; ++slot) {
		EQ::ItemInstance *inst = target->GetInv().GetItem(slot);
		if (!inst || !inst->GetItem()) {
			continue;
		}

		uint32 item_id = inst->GetItem()->ID;
		uint32 base_id = ItemProgression::GetBaseItemID(item_id);
		int    tier    = ItemProgression::GetTierFromItemID(item_id);

		const EQ::ItemData *base_data = database.GetItem(base_id);
		int ilevel = base_data ? ItemProgression::CalculateILevel(base_data) : 0;

		c->Message(
			ItemProgression::GetTierChatColor(tier),
			fmt::format(
				"  {:>4} | {:>10} | {:>6} | {} {}",
				slot,
				ItemProgression::GetTierName(tier),
				ilevel,
				inst->GetItem()->Name,
				tier > 0 ? fmt::format("[base: {}]", base_id) : ""
			).c_str()
		);
		++count;
	}

	if (count == 0) {
		c->Message(Chat::White, "  (no equipped items)");
	}

	c->Message(Chat::White, "Usage: #itemtier <slot_id> <tier>  (tier: 0=Base, 1=Enchanted, 2=Legendary, 3=Mythic)");
}
