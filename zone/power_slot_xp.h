/*	EQEmu: Everquest Server Emulator
	Infinite Item Progression — Power Slot XP + Kill-Based Tier-Up (Step 3)
	See: game_design/infinite_progression/IMPLEMENTATION_STEPS.md
*/

#ifndef ZONE_POWER_SLOT_XP_H
#define ZONE_POWER_SLOT_XP_H

class Client;
class NPC;

namespace PowerSlotXP {

	// Called from NPC::Death when a client earns kill credit.
	// Awards item XP to the Power Slot item, checks tier-up thresholds,
	// and sends UI messages. XP is stored per-instance via custom_data("Exp").
	void AwardKillXP(Client* killer, NPC* victim);

	// Generic "add XP to Power Slot" function.  Handles tier-up when
	// threshold is reached, sends celebration / milestone messages,
	// and persists XP.  Used by AwardKillXP, ConsumeItem, ConsumeEssence.
	// Returns true if a tier-up occurred.
	bool AddXP(Client* c, int xp_amount);

	// Perform the tier-up: swap the item in the Power Slot to the next tier.
	// overflow_xp is the excess XP beyond the threshold, carried into the new tier.
	// Returns true if the tier-up was successful.
	bool DoTierUp(Client* c, uint32 base_item_id, int old_tier, int new_tier, int overflow_xp);

	// Returns the current item XP for the item in the Power Slot.
	// Returns 0 if no item is equipped or no XP stored.
	int GetCurrentXP(Client* c);

	// Returns the XP threshold for the current tier -> next tier.
	// Returns 0 if already at max tier (Mythic).
	int GetThresholdForNextTier(int current_tier);

	// Returns the con-color multiplier for item XP.
	float GetConMultiplier(uint32 con_color);

	// Returns the source multiplier (named, raid-tier, or 1.0).
	float GetSourceMultiplier(NPC* victim, uint8 player_level);

} // namespace PowerSlotXP

#endif // ZONE_POWER_SLOT_XP_H
