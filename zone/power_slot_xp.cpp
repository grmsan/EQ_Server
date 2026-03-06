/*	EQEmu: Everquest Server Emulator
	Infinite Item Progression — Power Slot XP + Kill-Based Tier-Up (Step 3)
	See: game_design/infinite_progression/IMPLEMENTATION_STEPS.md

	Awards item XP to the Power Source slot item on mob kills.
	When XP thresholds are reached, the item automatically tiers up
	(Base -> Enchanted -> Legendary -> Mythic).

	XP is stored per-instance via custom_data("Exp") on the ItemInstance,
	matching THJ-style per-item progression. Two copies of the same base
	item progress independently.
*/

#include "power_slot_xp.h"
#include "client.h"
#include "npc.h"
#include "mob.h"
#include "../common/item_tier.h"
#include "../common/item_ilevel.h"
#include "../common/rulesys.h"
#include "../common/emu_constants.h"
#include "../common/strings.h"

namespace PowerSlotXP {

// custom_data key used on each ItemInstance to track accumulated XP
static constexpr const char* CUSTOM_DATA_EXP_KEY = "Exp";

float GetConMultiplier(uint32 con_color)
{
	switch (con_color) {
	case ConsiderColor::Gray:          return RuleR(ItemProgression, ConMultGrey);
	case ConsiderColor::Green:         return RuleR(ItemProgression, ConMultGreen);
	case ConsiderColor::LightBlue:     return RuleR(ItemProgression, ConMultLightBlue);
	case ConsiderColor::DarkBlue:      return RuleR(ItemProgression, ConMultBlue);
	case ConsiderColor::White:
	case ConsiderColor::WhiteTitanium: return RuleR(ItemProgression, ConMultWhite);
	case ConsiderColor::Yellow:        return RuleR(ItemProgression, ConMultYellow);
	case ConsiderColor::Red:           return RuleR(ItemProgression, ConMultRed);
	default:                           return RuleR(ItemProgression, ConMultWhite);
	}
}

float GetSourceMultiplier(NPC* victim, uint8 player_level)
{
	float mult = 1.0f;

	// Named / rare spawn bonus (stacks multiplicatively with raid-tier)
	if (victim->IsRareSpawn()) {
		mult *= RuleR(ItemProgression, NamedXPMult);
	}

	// Raid-tier bonus: NPC must be at or above the configured level threshold
	if (victim->GetLevel() >= RuleI(ItemProgression, RaidTierMinLevel)) {
		mult *= RuleR(ItemProgression, RaidTierXPMult);
	}

	return mult;
}

int GetThresholdForNextTier(int current_tier)
{
	switch (current_tier) {
	case ItemProgression::TierBase:      return RuleI(ItemProgression, TierThresholdEnchanted);
	case ItemProgression::TierEnchanted: return RuleI(ItemProgression, TierThresholdLegendary);
	case ItemProgression::TierLegendary: return RuleI(ItemProgression, TierThresholdMythic);
	default:                             return 0; // Already Mythic
	}
}

int GetCurrentXP(Client* c)
{
	if (!c) {
		return 0;
	}

	auto* pow_item = c->GetInv().GetItem(EQ::invslot::slotPowerSource);
	if (!pow_item || !pow_item->GetItem()) {
		return 0;
	}

	std::string xp_str = pow_item->GetCustomData(CUSTOM_DATA_EXP_KEY);
	return xp_str.empty() ? 0 : Strings::ToInt(xp_str);
}

// Perform the tier-up: swap the item in the Power Slot to the next tier.
// overflow_xp is the excess XP beyond the threshold, carried into the new tier.
// Returns true if the tier-up was successful.
bool DoTierUp(Client* c, uint32 base_item_id, int old_tier, int new_tier, int overflow_xp)
{
	auto* pow_item = c->GetInv().GetItem(EQ::invslot::slotPowerSource);
	if (!pow_item || !pow_item->GetItem()) {
		return false;
	}

	uint32 new_id = ItemProgression::GetTieredItemID(base_item_id, new_tier);

	// Verify the tiered item exists in the database
	const EQ::ItemData* new_item_data = database.GetItem(new_id);
	if (!new_item_data) {
		Log(Logs::General, Logs::Error,
			"PowerSlotXP: Tier-up failed — tiered item ID %u not found in DB (base %u, tier %d->%d)",
			new_id, base_item_id, old_tier, new_tier);
		return false;
	}

	// Preserve augments
	uint32 aug_ids[EQ::invaug::SOCKET_COUNT] = {};
	for (int i = 0; i < EQ::invaug::SOCKET_COUNT; ++i) {
		aug_ids[i] = pow_item->GetAugmentItemID(i);
	}
	bool is_attuned = pow_item->IsAttuned();

	// Delete old item and summon the new tiered version
	c->DeleteItemInInventory(EQ::invslot::slotPowerSource, 0, true, true);
	c->SummonItem(
		new_id,
		-1,           // charges
		aug_ids[0], aug_ids[1], aug_ids[2],
		aug_ids[3], aug_ids[4], aug_ids[5],
		is_attuned,
		static_cast<uint16>(EQ::invslot::slotPowerSource)
	);

	// Set overflow XP on the newly-summoned instance and persist it.
	// SummonItem creates a fresh ItemInstance, so we retrieve it and stamp the XP.
	auto* new_item = c->GetInv().GetItem(EQ::invslot::slotPowerSource);
	if (new_item) {
		int carry_xp = (overflow_xp > 0) ? overflow_xp : 0;
		new_item->SetCustomData(CUSTOM_DATA_EXP_KEY, std::to_string(carry_xp));
		database.SaveInventory(c->CharacterID(), new_item, EQ::invslot::slotPowerSource);
	}

	return true;
}

void AwardKillXP(Client* killer, NPC* victim)
{
	if (!killer || !victim) {
		return;
	}

	if (!RuleB(ItemProgression, PowerSlotXPEnabled)) {
		return;
	}

	// --- Validate Power Slot has an item ---
	auto* pow_item = killer->GetInv().GetItem(EQ::invslot::slotPowerSource);
	if (!pow_item || !pow_item->GetItem()) {
		return;
	}

	const EQ::ItemData* item_data = pow_item->GetItem();
	uint32 current_id = item_data->ID;
	uint32 base_id    = ItemProgression::GetBaseItemID(current_id);
	int    cur_tier   = ItemProgression::GetTierFromItemID(current_id);

	// Already at max tier — no XP to gain
	if (cur_tier >= ItemProgression::TierMax) {
		return;
	}

	// --- Calculate XP ---
	uint32 con_color = Mob::GetLevelCon(killer->GetLevel(), victim->GetLevel());
	float con_mult   = GetConMultiplier(con_color);

	// Grey cons = 0 XP (anti-exploit)
	if (con_mult <= 0.0f) {
		return;
	}

	float source_mult = GetSourceMultiplier(victim, killer->GetLevel());
	int base_xp       = RuleI(ItemProgression, BaseItemXP);
	int awarded_xp    = static_cast<int>(static_cast<float>(base_xp) * con_mult * source_mult);

	if (awarded_xp <= 0) {
		return;
	}

	// --- Display XP gain message (before AddXP, which may trigger tier-up) ---
	if (RuleB(ItemProgression, PowerSlotXPMessages)) {
		auto* pow_item_msg = killer->GetInv().GetItem(EQ::invslot::slotPowerSource);
		if (pow_item_msg) {
			std::string xp_str_msg = pow_item_msg->GetCustomData(CUSTOM_DATA_EXP_KEY);
			int current_xp_msg     = xp_str_msg.empty() ? 0 : Strings::ToInt(xp_str_msg);
			int threshold_msg      = GetThresholdForNextTier(cur_tier);
			if (threshold_msg > 0) {
				int preview_xp  = current_xp_msg + awarded_xp;
				int preview_pct = (preview_xp * 100 / threshold_msg);
				if (preview_pct > 100) preview_pct = 100;
				const char* next_tier_name = ItemProgression::GetTierName(cur_tier + 1);
				killer->Message(
					Chat::Experience,
					fmt::format(
						"(+{} item XP, {}% to {})",
						awarded_xp, preview_pct, next_tier_name
					).c_str()
				);
			}
		}
	}

	// Delegate to shared AddXP (handles tier-up + milestones)
	AddXP(killer, awarded_xp);
}

bool AddXP(Client* c, int xp_amount)
{
	if (!c || xp_amount <= 0) {
		return false;
	}

	auto* pow_item = c->GetInv().GetItem(EQ::invslot::slotPowerSource);
	if (!pow_item || !pow_item->GetItem()) {
		return false;
	}

	const EQ::ItemData* item_data = pow_item->GetItem();
	uint32 current_id = item_data->ID;
	uint32 base_id    = ItemProgression::GetBaseItemID(current_id);
	int    cur_tier   = ItemProgression::GetTierFromItemID(current_id);

	if (cur_tier >= ItemProgression::TierMax) {
		return false;
	}

	std::string xp_str = pow_item->GetCustomData(CUSTOM_DATA_EXP_KEY);
	int current_xp     = xp_str.empty() ? 0 : Strings::ToInt(xp_str);
	int threshold       = GetThresholdForNextTier(cur_tier);

	if (threshold <= 0) {
		return false;
	}

	int old_percent = (current_xp * 100 / threshold);
	int new_xp      = current_xp + xp_amount;

	// --- Check for tier-up ---
	if (new_xp >= threshold) {
		int new_tier    = cur_tier + 1;
		int overflow_xp = new_xp - threshold;

		if (DoTierUp(c, base_id, cur_tier, new_tier, overflow_xp)) {
			c->SendSound();

			uint32 new_id = ItemProgression::GetTieredItemID(base_id, new_tier);
			const EQ::ItemData* new_data = database.GetItem(new_id);
			const char* new_name  = new_data ? new_data->Name : "Unknown";
			const char* tier_name = ItemProgression::GetTierName(new_tier);

			c->Message(
				ItemProgression::GetTierChatColor(new_tier),
				fmt::format(
					"*** Your {} has achieved {} status! ***",
					new_name, tier_name
				).c_str()
			);

			if (new_tier < ItemProgression::TierMax) {
				int next_threshold    = GetThresholdForNextTier(new_tier);
				const char* next_name = ItemProgression::GetTierName(new_tier + 1);
				int carry_pct = (next_threshold > 0) ? (overflow_xp * 100 / next_threshold) : 0;
				c->Message(
					Chat::White,
					fmt::format(
						"Next tier: {} — {} / {} item XP ({}%)",
						next_name, overflow_xp, next_threshold, carry_pct
					).c_str()
				);
			} else {
				c->Message(Chat::White, "Your Power Source item has reached maximum tier!");
			}

			Log(Logs::General, Logs::Status,
				"PowerSlotXP: %s tier-up %s -> %s (base_id=%u, overflow=%d)",
				c->GetCleanName(),
				ItemProgression::GetTierName(cur_tier),
				tier_name,
				base_id,
				overflow_xp);

			return true;
		} else {
			c->Message(Chat::Red,
				"Item tier-up failed — tiered item not found in database. XP preserved.");
			return false;
		}
	}

	// --- No tier-up; store updated XP ---
	pow_item->SetCustomData(CUSTOM_DATA_EXP_KEY, std::to_string(new_xp));
	database.SaveInventory(c->CharacterID(), pow_item, EQ::invslot::slotPowerSource);

	// --- Milestone messages ---
	if (RuleB(ItemProgression, PowerSlotMilestoneMessages)) {
		int new_percent = (new_xp * 100 / threshold);
		const char* next_tier_name = ItemProgression::GetTierName(cur_tier + 1);

		static const int milestones[] = { 25, 50, 75, 90 };
		for (int ms : milestones) {
			if (old_percent < ms && new_percent >= ms) {
				c->Message(
					Chat::Yellow,
					fmt::format(
						"** Your Power Source item is {}% of the way to {}! **",
						ms, next_tier_name
					).c_str()
				);
				break;
			}
		}
	}

	return false;
}

} // namespace PowerSlotXP
