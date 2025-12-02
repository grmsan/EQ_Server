/**
 * dynamic_item_integration.h
 * Helper functions for integrating dynamic items with zone server
 */

#ifndef EQEMU_DYNAMIC_ITEM_INTEGRATION_H
#define EQEMU_DYNAMIC_ITEM_INTEGRATION_H

#include <cstdint>

namespace EQ {

class ItemData;
class ItemInstance;

/**
 * Zone-specific item loader that handles both static and dynamic items
 * For dynamic IDs (>= 500000000), generates scaled items
 * For normal IDs, uses database.GetItem()
 */
const ItemData* GetItemWithDynamic(uint32 item_id);

/**
 * Creates an ItemInstance with dynamic item support
 * Returns nullptr if item doesn't exist
 */
ItemInstance* CreateItemInstance(uint32 item_id, int16 charges = 0);

} // namespace EQ

#endif // EQEMU_DYNAMIC_ITEM_INTEGRATION_H
