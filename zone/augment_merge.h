/*
 * augment_merge.h — Augment Forgemaster merge system (Step 9)
 *
 * Provides the ProcessForgemaster() function called from HandleCombine
 * when the player clicks "Combine" on an Augment Forge container.
 *
 * Merge rule: 3 identical-family + identical-level augs + 1 correct-tier
 * Merge Catalyst → consume all 4, produce 1 aug of the next level.
 */

#pragma once

class Client;

namespace AugmentMerge {

/**
 * Process a combine attempt in the Augment Forge container.
 *
 * @param user          The client performing the combine
 * @param container_slot  The inventory slot of the Augment Forge bag
 */
void ProcessForgemaster(Client* user, int16 container_slot);

} // namespace AugmentMerge
