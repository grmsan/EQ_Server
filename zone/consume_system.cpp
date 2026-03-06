/*	EQEmu: Everquest Server Emulator
	Infinite Item Progression — Consume Item / Consume Essence AAs (Step 5)
	See: game_design/infinite_progression/IMPLEMENTATION_STEPS.md

	HandleConsumeItem:
	  Cursor item must match Power Slot item (same base ID via GetBaseItemID).
	  XP granted % of threshold based on tier comparison (same/lower/higher).
	  Cursor item is destroyed after consumption.

	HandleConsumeEssence:
	  Spends Common Essence from the player's alternate-currency balance.
	  Calculates remaining XP to next tier, converts to Essence at the
	  configurable ratio (ConsumeEssencePerXP), and consumes only what
	  is needed (excess stays in balance).
*/

#include "consume_system.h"
#include "client.h"
#include "power_slot_xp.h"
#include "../common/item_tier.h"
#include "../common/item_ilevel.h"
#include "../common/rulesys.h"
#include "../common/strings.h"
#include "../common/emu_constants.h"

namespace ConsumeSystem {

// ========================================================================
// Consume Item — cursor item → Power Slot XP
// ========================================================================
void HandleConsumeItem(Client* c)
{
	if (!c) {
		return;
	}

	// --- Validate Power Slot ---
	auto* pow_item = c->GetInv().GetItem(EQ::invslot::slotPowerSource);
	if (!pow_item || !pow_item->GetItem()) {
		c->Message(Chat::Red, "You must have an item in your Power Source slot to consume.");
		return;
	}

	const EQ::ItemData* pow_data = pow_item->GetItem();
	uint32 pow_current_id = pow_data->ID;
	uint32 pow_base_id    = ItemProgression::GetBaseItemID(pow_current_id);
	int    pow_tier       = ItemProgression::GetTierFromItemID(pow_current_id);

	if (pow_tier >= ItemProgression::TierMax) {
		c->Message(Chat::Yellow, "Your Power Source item is already at maximum tier.");
		return;
	}

	// --- Validate Cursor Item ---
	auto* cur_item = c->GetInv().GetItem(EQ::invslot::slotCursor);
	if (!cur_item || !cur_item->GetItem()) {
		c->Message(Chat::Red, "Place an item on your cursor to consume.");
		return;
	}

	const EQ::ItemData* cur_data = cur_item->GetItem();
	uint32 cur_id      = cur_data->ID;
	uint32 cur_base_id = ItemProgression::GetBaseItemID(cur_id);
	int    cur_tier    = ItemProgression::GetTierFromItemID(cur_id);

	// Must be same base item
	if (cur_base_id != pow_base_id) {
		c->Message(Chat::Red, fmt::format(
			"Only duplicates of your Power Source item can be consumed. "
			"(Expected base ID {}, got {}.)",
			pow_base_id, cur_base_id
		).c_str());
		return;
	}

	// Reject attuned items (THJ parity)
	if (cur_item->IsAttuned()) {
		c->Message(Chat::Red, "You cannot consume an attuned item.");
		return;
	}

	// --- Calculate XP ---
	int threshold = PowerSlotXP::GetThresholdForNextTier(pow_tier);
	if (threshold <= 0) {
		c->Message(Chat::Red, "No XP threshold found for current tier.");
		return;
	}

	int pct = 0;
	if (cur_tier > pow_tier) {
		pct = RuleI(ItemProgression, ConsumeItemHigherTierPct);
	} else if (cur_tier == pow_tier) {
		pct = RuleI(ItemProgression, ConsumeItemSameTierPct);
	} else {
		pct = RuleI(ItemProgression, ConsumeItemLowerTierPct);
	}

	int xp_granted = (threshold * pct) / 100;
	if (xp_granted < 1) {
		xp_granted = 1;
	}

	// --- Destroy cursor item ---
	c->DeleteItemInInventory(EQ::invslot::slotCursor, 0, true, true);

	// --- Inform + add XP ---
	c->Message(Chat::Experience, fmt::format(
		"You consume a {} {} — +{} item XP!",
		ItemProgression::GetTierName(cur_tier),
		cur_data->Name,
		xp_granted
	).c_str());

	PowerSlotXP::AddXP(c, xp_granted);
}

// ========================================================================
// Consume Essence — Common Essence → Power Slot XP
// ========================================================================
void HandleConsumeEssence(Client* c)
{
	if (!c) {
		return;
	}

	// --- Validate Power Slot ---
	auto* pow_item = c->GetInv().GetItem(EQ::invslot::slotPowerSource);
	if (!pow_item || !pow_item->GetItem()) {
		c->Message(Chat::Red, "You must have an item in your Power Source slot to infuse with Essence.");
		return;
	}

	const EQ::ItemData* pow_data = pow_item->GetItem();
	uint32 pow_current_id = pow_data->ID;
	int    pow_tier       = ItemProgression::GetTierFromItemID(pow_current_id);

	if (pow_tier >= ItemProgression::TierMax) {
		c->Message(Chat::Yellow, "Your Power Source item is already at maximum tier.");
		return;
	}

	// --- Calculate XP remaining ---
	std::string xp_str = pow_item->GetCustomData("Exp");
	int current_xp     = xp_str.empty() ? 0 : Strings::ToInt(xp_str);
	int threshold       = PowerSlotXP::GetThresholdForNextTier(pow_tier);

	if (threshold <= 0) {
		c->Message(Chat::Red, "No XP threshold found for current tier.");
		return;
	}

	int remaining_xp = threshold - current_xp;
	if (remaining_xp <= 0) {
		// Should normally trigger tier-up; force a minimal add to trigger it
		remaining_xp = 1;
	}

	// --- Calculate Essence needed ---
	float essence_per_xp = RuleR(ItemProgression, ConsumeEssencePerXP);
	if (essence_per_xp <= 0.0f) {
		essence_per_xp = 1.0f;
	}

	int essence_to_fill = static_cast<int>(std::ceil(
		static_cast<float>(remaining_xp) * essence_per_xp
	));

	// --- Check balance ---
	uint32 currency_id = RuleI(ItemProgression, CommonEssenceCurrencyID);

	if (!zone->DoesAlternateCurrencyExist(currency_id)) {
		c->Message(Chat::Red, "Common Essence currency is not configured on this server.");
		return;
	}

	int balance = static_cast<int>(c->GetAlternateCurrencyValue(currency_id));

	if (balance <= 0) {
		c->Message(Chat::Red, "You have no Common Essence to consume.");
		return;
	}

	// Consume only what is needed, or full balance if not enough
	int consume_amount = std::min(essence_to_fill, balance);

	// Convert consumed Essence back to XP
	int xp_from_essence = static_cast<int>(
		static_cast<float>(consume_amount) / essence_per_xp
	);
	if (xp_from_essence < 1) {
		xp_from_essence = 1;
	}

	// Cap XP at remaining to prevent over-fill beyond one tier
	if (xp_from_essence > remaining_xp) {
		xp_from_essence = remaining_xp;
		// Recalculate actual Essence cost precisely
		consume_amount = static_cast<int>(std::ceil(
			static_cast<float>(xp_from_essence) * essence_per_xp
		));
	}

	// --- Deduct Essence ---
	c->AddAlternateCurrencyValue(currency_id, -consume_amount);

	// --- Inform + add XP ---
	int new_balance = static_cast<int>(c->GetAlternateCurrencyValue(currency_id));

	c->Message(Chat::Experience, fmt::format(
		"You infuse {} Common Essence into your Power Source — +{} item XP! "
		"({} Essence remaining.)",
		consume_amount,
		xp_from_essence,
		new_balance
	).c_str());

	PowerSlotXP::AddXP(c, xp_from_essence);
}

} // namespace ConsumeSystem
