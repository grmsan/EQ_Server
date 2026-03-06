#include "../client.h"
#include "../power_slot_xp.h"
#include "../ghost_copy.h"
#include "../../common/item_tier.h"
#include "../../common/item_ilevel.h"
#include "../../common/rulesys.h"

/*
 * #powerslot [info]  — show Power Slot item, tier, XP progress, and % to next tier
 * #powerslot reset   — (GM) reset XP on current Power Slot item to 0
 * #powerslot setxp N — (GM) set XP on current Power Slot item to N
 */

void command_powerslot(Client *c, const Seperator *sep)
{
	Client *target = c;
	if (c->GetTarget() && c->GetTarget()->IsClient()) {
		target = c->GetTarget()->CastToClient();
	}

	auto* pow_item = target->GetInv().GetItem(EQ::invslot::slotPowerSource);
	if (!pow_item || !pow_item->GetItem()) {
		c->Message(Chat::Red, "POWERSLOT | No item in Power Source slot.");
		return;
	}

	const EQ::ItemData* item_data = pow_item->GetItem();
	uint32 current_id = item_data->ID;
	uint32 base_id    = ItemProgression::GetBaseItemID(current_id);
	int    cur_tier   = ItemProgression::GetTierFromItemID(current_id);

	// Fetch current XP from item instance custom_data
	std::string xp_str = pow_item->GetCustomData("Exp");
	int current_xp     = xp_str.empty() ? 0 : Strings::ToInt(xp_str);
	int threshold      = PowerSlotXP::GetThresholdForNextTier(cur_tier);

	// ---- Sub-commands ----
	if (sep->arg[1][0] != '\0') {
		std::string subcmd = Strings::ToLower(sep->arg[1]);

		if (subcmd == "reset") {
			pow_item->SetCustomData("Exp", std::to_string(0));
			database.SaveInventory(target->CharacterID(), pow_item, EQ::invslot::slotPowerSource);
			c->Message(Chat::Yellow, fmt::format(
				"POWERSLOT | Reset item XP to 0 for '{}' (base_id={}).",
				item_data->Name, base_id
			).c_str());
			return;
		}

		if (subcmd == "setxp" && sep->arg[2][0] != '\0') {
			int new_xp = Strings::ToInt(sep->arg[2]);
			if (new_xp < 0) new_xp = 0;
			pow_item->SetCustomData("Exp", std::to_string(new_xp));
			database.SaveInventory(target->CharacterID(), pow_item, EQ::invslot::slotPowerSource);
			c->Message(Chat::Yellow, fmt::format(
				"POWERSLOT | Set item XP to {} for '{}' (base_id={}).",
				new_xp, item_data->Name, base_id
			).c_str());
			return;
		}
	}

	// ---- Info display (default) ----
	const EQ::ItemData* base_data = database.GetItem(base_id);
	int ilevel = base_data ? ItemProgression::CalculateILevel(base_data) : 0;

	c->Message(
		ItemProgression::GetTierChatColor(cur_tier),
		fmt::format(
			"=== Power Source: {} ===",
			item_data->Name
		).c_str()
	);

	c->Message(Chat::White, fmt::format(
		"  Base Item: {} (ID: {})  |  iLevel: {}",
		base_data ? base_data->Name : "?", base_id, ilevel
	).c_str());

	c->Message(Chat::White, fmt::format(
		"  Current Tier: {} ({})  |  Item ID: {}",
		ItemProgression::GetTierName(cur_tier), cur_tier, current_id
	).c_str());

	if (cur_tier >= ItemProgression::TierMax) {
		c->Message(Chat::Yellow, "  ** Maximum tier reached! **");
	} else {
		int percent = (threshold > 0) ? (current_xp * 100 / threshold) : 0;
		const char* next_tier_name = ItemProgression::GetTierName(cur_tier + 1);

		c->Message(Chat::White, fmt::format(
			"  Item XP: {} / {} ({}% to {})",
			current_xp, threshold, percent, next_tier_name
		).c_str());

		// Show a progress bar
		int bar_len = 20;
		int filled  = (threshold > 0) ? (current_xp * bar_len / threshold) : 0;
		if (filled > bar_len) filled = bar_len;
		std::string bar(filled, '|');
		std::string empty(bar_len - filled, '-');

		c->Message(Chat::White, fmt::format(
			"  [{}{}] {}%",
			bar, empty, percent
		).c_str());

		// Dual-path display: show Essence cost to fill remaining XP
		int remaining_xp = threshold - current_xp;
		if (remaining_xp > 0) {
			float essence_per_xp = RuleR(ItemProgression, ConsumeEssencePerXP);
			if (essence_per_xp <= 0.0f) essence_per_xp = 1.0f;
			int essence_to_fill = static_cast<int>(std::ceil(
				static_cast<float>(remaining_xp) * essence_per_xp
			));
			c->Message(Chat::White, fmt::format(
				"  Essence to next tier: {} Common  |  Consume Item: {}% / {}% / {}% XP (higher/same/lower)",
				essence_to_fill,
				RuleI(ItemProgression, ConsumeItemHigherTierPct),
				RuleI(ItemProgression, ConsumeItemSameTierPct),
				RuleI(ItemProgression, ConsumeItemLowerTierPct)
			).c_str());
		}
	}

	// Show XP rates at current rules
	c->Message(Chat::White, fmt::format(
		"  Base XP/kill: {}  |  Named mult: {:.1f}x  |  Raid mult: {:.1f}x",
		RuleI(ItemProgression, BaseItemXP),
		RuleR(ItemProgression, NamedXPMult),
		RuleR(ItemProgression, RaidTierXPMult)
	).c_str());

	// Step 6: Ghost Copy status
	if (GhostCopy::IsProgressionItem(item_data)) {
		if (target->HasGhostCopy()) {
			int16 ghost_slot = target->GetGhostCopySlot();
			c->Message(Chat::Yellow, fmt::format(
				"  Ghost Copy: ACTIVE -> {} slot",
				EQ::invslot::GetInvPossessionsSlotName(ghost_slot)
			).c_str());
		} else {
			c->Message(Chat::White, "  Ghost Copy: INACTIVE (all native slots occupied)");
		}
	}

	c->Message(Chat::White, "  Use: #powerslot reset | #powerslot setxp <amount>");
}
