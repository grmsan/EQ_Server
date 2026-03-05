/*	EQEmu: Everquest Server Emulator
	Infinite Item Progression — Power Slot XP + Kill-Based Tier-Up (Step 3)
	See: game_design/infinite_progression/IMPLEMENTATION_STEPS.md

	Awards item XP to the Power Source slot item on mob kills.
	When XP thresholds are reached, the item automatically tiers up
	(Base -> Enchanted -> Legendary -> Mythic).
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

std::string GetBucketKey(uint32 base_item_id)
{
	return fmt::format("power_xp_{}", base_item_id);
}

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

	uint32 base_id = ItemProgression::GetBaseItemID(pow_item->GetItem()->ID);
	std::string xp_str = c->GetBucket(GetBucketKey(base_id));

	return xp_str.empty() ? 0 : Strings::ToInt(xp_str);
}

// Perform the tier-up: swap the item in the Power Slot to the next tier.
// Returns true if the tier-up was successful.
static bool DoTierUp(Client* c, uint32 base_item_id, int old_tier, int new_tier)
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

	// --- Load current XP and add ---
	std::string bucket_key = GetBucketKey(base_id);
	std::string xp_str     = killer->GetBucket(bucket_key);
	int current_xp         = xp_str.empty() ? 0 : Strings::ToInt(xp_str);
	int threshold           = GetThresholdForNextTier(cur_tier);

	if (threshold <= 0) {
		return; // Safety: shouldn't happen since we checked TierMax above
	}

	// Calculate old percentage for milestone detection
	int old_percent = (threshold > 0) ? (current_xp * 100 / threshold) : 0;

	int new_xp = current_xp + awarded_xp;

	// --- Check for tier-up ---
	if (new_xp >= threshold) {
		int new_tier = cur_tier + 1;

		// Reset XP for the new tier
		killer->SetBucket(bucket_key, "0");

		// Perform the tier swap
		if (DoTierUp(killer, base_id, cur_tier, new_tier)) {
			// Level-up sound!
			killer->SendSound();

			// Get new item data for the message
			uint32 new_id = ItemProgression::GetTieredItemID(base_id, new_tier);
			const EQ::ItemData* new_data = database.GetItem(new_id);
			const char* new_name = new_data ? new_data->Name : "Unknown";
			const char* tier_name = ItemProgression::GetTierName(new_tier);

			// Send tier-up celebration message
			killer->Message(
				ItemProgression::GetTierChatColor(new_tier),
				fmt::format(
					"*** Your {} has achieved {} status! ***",
					new_name,
					tier_name
				).c_str()
			);

			// If not yet Mythic, show next threshold
			if (new_tier < ItemProgression::TierMax) {
				int next_threshold = GetThresholdForNextTier(new_tier);
				const char* next_tier_name = ItemProgression::GetTierName(new_tier + 1);
				killer->Message(
					Chat::White,
					fmt::format(
						"Next tier: {} — 0 / {} item XP (0%)",
						next_tier_name, next_threshold
					).c_str()
				);
			} else {
				killer->Message(Chat::White, "Your Power Source item has reached maximum tier!");
			}

			Log(Logs::General, Logs::Status,
				"PowerSlotXP: %s tier-up %s -> %s (base_id=%u)",
				killer->GetCleanName(),
				ItemProgression::GetTierName(cur_tier),
				tier_name,
				base_id);
		} else {
			// Tier-up failed (missing DB row?) — don't lose XP
			killer->SetBucket(bucket_key, std::to_string(current_xp));
			killer->Message(Chat::Red,
				"Item tier-up failed — tiered item not found in database. XP preserved.");
		}

		return;
	}

	// --- No tier-up yet; store updated XP ---
	killer->SetBucket(bucket_key, std::to_string(new_xp));

	// --- Display XP gain message ---
	if (RuleB(ItemProgression, PowerSlotXPMessages)) {
		int new_percent = (threshold > 0) ? (new_xp * 100 / threshold) : 0;
		const char* next_tier_name = ItemProgression::GetTierName(cur_tier + 1);

		killer->Message(
			Chat::Experience,
			fmt::format(
				"(+{} item XP, {}% to {})",
				awarded_xp, new_percent, next_tier_name
			).c_str()
		);
	}

	// --- Milestone messages ---
	if (RuleB(ItemProgression, PowerSlotMilestoneMessages)) {
		int new_percent = (threshold > 0) ? (new_xp * 100 / threshold) : 0;
		const char* next_tier_name = ItemProgression::GetTierName(cur_tier + 1);

		// Check each milestone: 25, 50, 75, 90
		static const int milestones[] = { 25, 50, 75, 90 };
		for (int ms : milestones) {
			if (old_percent < ms && new_percent >= ms) {
				killer->Message(
					Chat::Yellow,
					fmt::format(
						"** Your Power Source item is {}% of the way to {}! **",
						ms, next_tier_name
					).c_str()
				);
				break; // Only show one milestone per kill
			}
		}
	}
}

} // namespace PowerSlotXP
