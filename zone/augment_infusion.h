#ifndef AUGMENT_INFUSION_H
#define AUGMENT_INFUSION_H

class Client;

/*
 * Augment Infusion System — Step 11
 *
 * Uses the Infusion Pool container (2 slots): aug + infusion catalyst.
 * Player clicks Combine to infuse +1 to the aug's primary heroic stat.
 * The catalyst is consumed; the aug is returned with updated custom_data.
 *
 * Catalyst tiers correspond to infusion levels 1-5 and are purchased
 * from vendors with Common Essence.
 *
 * Design reference: game_design/infinite_progression/AUGMENT_SYSTEM.md §8.1
 */
namespace AugmentInfusion {
	void ProcessInfusionPool(Client *c, int16 container_slot);
}

#endif // AUGMENT_INFUSION_H
