// Step 6 — Ghost Copy System
//
// When a progression item is in the Power Source slot and a native equipment
// slot is free, a ghost copy of that item is placed in the native slot so the
// client sees (and can use) the weapon/armor.  The Power Source slot itself
// never contributes stat bonuses for progression items — those come exclusively
// from the ghost copy occupying the native slot.
//
// Ghost copies are transient: they are NOT saved to the database and are
// regenerated every time CalcBonuses() runs (which covers login, zone-in,
// equip changes, and any other inventory mutation).
//
// Picking up from either the Power Source slot or the ghost slot removes BOTH
// and places the original item on the cursor.

#include "ghost_copy.h"

#include "client.h"
#include "../common/emu_constants.h"
#include "../common/item_data.h"
#include "../common/item_instance.h"
#include "../common/inventory_profile.h"
#include "../common/rulesys.h"

// Custom-data key written onto ghost-copy ItemInstances (debug marker).
static constexpr const char* GHOST_KEY = "GhostCopy";

// ---------------------------------------------------------------------------
// IsProgressionItem
// ---------------------------------------------------------------------------
bool GhostCopy::IsProgressionItem(const EQ::ItemData* item)
{
	if (!item) {
		return false;
	}

	constexpr uint32 ps_bit     = (1u << EQ::invslot::slotPowerSource);
	constexpr uint32 equip_mask = static_cast<uint32>(EQ::invslot::EQUIPMENT_BITMASK);
	uint32 non_ps_equip_bits    = (item->Slots & equip_mask) & ~ps_bit;

	return non_ps_equip_bits != 0;
}

// ---------------------------------------------------------------------------
// FindTargetSlot
// ---------------------------------------------------------------------------
// Returns the first empty equipment slot the PS item could occupy.
// Ghost-copy slots are treated as empty (they will be replaced).
// Returns -1 when no slot is available.
int16 GhostCopy::FindTargetSlot(Client* c)
{
	if (!c) {
		return -1;
	}

	const auto* ps_inst = c->GetInv().GetItem(EQ::invslot::slotPowerSource);
	if (!ps_inst) {
		return -1;
	}

	const auto* item = ps_inst->GetItem();
	if (!item || !IsProgressionItem(item)) {
		return -1;
	}

	uint32 slots  = item->Slots;
	int16 ghost   = c->GetGhostCopySlot();

	bool is_2h = (
		item->ItemType == EQ::item::ItemType2HSlash  ||
		item->ItemType == EQ::item::ItemType2HBlunt  ||
		item->ItemType == EQ::item::ItemType2HPiercing
	);

	for (int16 slot = EQ::invslot::EQUIPMENT_BEGIN; slot <= EQ::invslot::EQUIPMENT_END; slot++) {
		if (slot == EQ::invslot::slotPowerSource) {
			continue;
		}

		// Item can't go in this slot
		if (!(slots & (1u << slot))) {
			continue;
		}

		// Slot occupied by a real item (our own ghost doesn't count)
		const auto* existing = c->GetInv().GetItem(slot);
		if (existing != nullptr && slot != ghost) {
			continue;
		}

		// 2H requires both hands free
		if (is_2h && slot == EQ::invslot::slotPrimary) {
			const auto* sec = c->GetInv().GetItem(EQ::invslot::slotSecondary);
			if (sec != nullptr && EQ::invslot::slotSecondary != ghost) {
				continue;
			}
		}

		return slot;
	}

	return -1;
}

// ---------------------------------------------------------------------------
// RemoveGhostCopy
// ---------------------------------------------------------------------------
void GhostCopy::RemoveGhostCopy(Client* c)
{
	if (!c) {
		return;
	}

	int16 ghost_slot = c->GetGhostCopySlot();
	if (ghost_slot < 0) {
		return;
	}

	const auto* ghost_inst = c->GetInv().GetItem(ghost_slot);
	if (ghost_inst) {
		// client_update=true (client sees removal), update_db=false (ghost is transient)
		c->DeleteItemInInventory(ghost_slot, 0, true, false);
	}

	c->SetGhostCopySlot(-1);
}

// ---------------------------------------------------------------------------
// UpdateGhostCopy
// ---------------------------------------------------------------------------
void GhostCopy::UpdateGhostCopy(Client* c)
{
	if (!c) {
		return;
	}

	if (!RuleB(ItemProgression, StatProjectionEnabled)) {
		// Clean up any existing ghost before bailing out, so disabling
		// the rule at runtime doesn't leave a stale ghost in inventory.
		RemoveGhostCopy(c);
		return;
	}

	// Reentrance guard — placing/removing items can trigger CalcBonuses()
	if (c->IsUpdatingGhost()) {
		return;
	}
	c->SetUpdatingGhost(true);

	int16 current_ghost = c->GetGhostCopySlot();
	const auto* ps_inst = c->GetInv().GetItem(EQ::invslot::slotPowerSource);

	// --- Determine whether we need a ghost and where ---
	bool  need_ghost  = false;
	int16 target_slot = -1;

	if (ps_inst && ps_inst->GetItem() && IsProgressionItem(ps_inst->GetItem())) {
		target_slot = FindTargetSlot(c);
		if (target_slot >= 0) {
			need_ghost = true;
		}
	}

	// --- Validate existing ghost ---
	if (current_ghost >= 0) {
		const auto* ghost_inst = c->GetInv().GetItem(current_ghost);
		bool ghost_exists      = (ghost_inst != nullptr);

		// Detect tier-up: PS item ID changed → force ghost refresh
		bool id_match = ghost_exists && ps_inst && (ghost_inst->GetID() == ps_inst->GetID());

		bool should_remove = (
			!ghost_exists                                   ||
			!need_ghost                                     ||
			!id_match                                       ||
			(need_ghost && current_ghost != target_slot)
		);

		if (should_remove) {
			if (ghost_exists) {
				c->DeleteItemInInventory(current_ghost, 0, true, false);
			}
			c->SetGhostCopySlot(-1);
			current_ghost = -1;
		}
	}

	// --- Place new ghost if needed ---
	if (need_ghost && current_ghost < 0) {
		auto* copy = ps_inst->Clone();
		copy->SetCustomData(GHOST_KEY, "1");

		// Place in inventory WITHOUT CalcBonuses or DB save
		c->GetInv().PutItem(target_slot, *copy);
		c->SendItemPacket(target_slot, copy, ItemPacketTrade);

		// Update visual model (weapon in hand, armor appearance, etc.)
		uint8 mat_slot = EQ::InventoryProfile::CalcMaterialFromSlot(target_slot);
		if (mat_slot != EQ::textures::materialInvalid) {
			c->SendWearChange(mat_slot);
		}

		safe_delete(copy);
		c->SetGhostCopySlot(target_slot);

		if (c->Connected()) {
			c->Message(
				Chat::Yellow,
				"Your Power Source item appears in your %s slot.",
				EQ::invslot::GetInvPossessionsSlotName(target_slot)
			);
		}
	}

	c->SetUpdatingGhost(false);
}

// ---------------------------------------------------------------------------
// HandleMoveItem
// ---------------------------------------------------------------------------
// Called at the top of Client::SwapItem().  Returns true when the move has
// been fully handled and SwapItem should return immediately.
bool GhostCopy::HandleMoveItem(Client* c, int16 from_slot, int16 to_slot)
{
	if (!c) {
		return false;
	}

	int16 ghost_slot = c->GetGhostCopySlot();
	if (ghost_slot < 0) {
		return false;   // no ghost active
	}

	// ---- Case 1: Picking up FROM the ghost slot ----
	// The player clicked the ghost copy.  We remove the ghost and hand them
	// the *real* Power Source item on the cursor.
	if (from_slot == ghost_slot) {
		// Remove ghost (transient, no DB)
		c->DeleteItemInInventory(ghost_slot, 0, true, false);
		c->SetGhostCopySlot(-1);

		// Move the real item from Power Source → cursor
		const auto* ps_inst = c->GetInv().GetItem(EQ::invslot::slotPowerSource);
		if (ps_inst) {
			c->PushItemOnCursor(*ps_inst);
			c->DeleteItemInInventory(EQ::invslot::slotPowerSource, 0, true, true);
		}

		c->CalcBonuses();
		return true;   // fully handled
	}

	// ---- Case 2: Picking up FROM Power Source while ghost is active ----
	// Remove ghost first, then let normal SwapItem handle the PS removal.
	if (from_slot == EQ::invslot::slotPowerSource) {
		RemoveGhostCopy(c);
		return false;  // continue normal SwapItem
	}

	// ---- Case 3: Moving TO the ghost slot (equipping over the ghost) ----
	// Remove ghost so SwapItem sees an empty target slot and does a simple equip.
	if (to_slot == ghost_slot) {
		RemoveGhostCopy(c);
		return false;  // continue normal SwapItem
	}

	return false;
}
