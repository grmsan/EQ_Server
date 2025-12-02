/**
 * dynamic_item_integration.cpp
 * Integrates DynamicItemManager with the existing item loading system
 */

#include "dynamic_item_manager.h"
#include "../common/item_instance.h"
#include "../common/item_data.h"
#include "../common/eqemu_logsys.h"
#include "client.h"

// Forward declare external database
extern ZoneDatabase database;

namespace EQ {

/**
 * Zone-specific item loader that handles both static and dynamic items
 * Call this instead of database.GetItem() when you want dynamic item support
 */
const ItemData* GetItemWithDynamic(uint32 item_id) {
	Log(Logs::Detail, Logs::Quests, "GetItemWithDynamic: item_id=%u", item_id);

	// Check if ID is in dynamic range (LLLLIIIIII format, level offset 101-4294)
	// Dynamic items have level_part (item_id / 1000000) in range 101-4294
	uint32 level_part = item_id / 1000000;
	if (level_part > 100 && level_part <= 4294) {
		Log(Logs::General, Logs::Quests, "GetItemWithDynamic: Dynamic item detected (level_part=%u), routing to DynamicItemManager", level_part);

		auto& mgr = DynamicItemManager::Get();
		uint32 level = mgr.GetItemLevel(item_id);
		uint32 base_id = mgr.GetBaseItemID(item_id);

		Log(Logs::General, Logs::Quests, "GetItemWithDynamic: Decoded - base_id=%u, level=%u", base_id, level);

		// Generate scaled item (uses internal cache)
		auto* result = mgr.GenerateScaledItem(base_id, level);

		if (result) {
			Log(Logs::General, Logs::Quests, "GetItemWithDynamic: SUCCESS - Returned '%s'", result->Name);
		} else {
			Log(Logs::General, Logs::Error, "GetItemWithDynamic: FAILED - Could not generate item");
		}

		return result;
	}

	// Standard database lookup
	Log(Logs::Detail, Logs::Quests, "GetItemWithDynamic: Standard item, using database.GetItem()");
	return database.GetItem(item_id);
}

/**
 * Creates an ItemInstance with dynamic item support
 * Use this for summon/creation commands
 */
ItemInstance* CreateItemInstance(uint32 item_id, int16 charges) {
	Log(Logs::General, Logs::Quests, "CreateItemInstance: item_id=%u, charges=%d", item_id, charges);

	const ItemData* item_data = GetItemWithDynamic(item_id);
	if (!item_data) {
		Log(Logs::General, Logs::Error, "CreateItemInstance: FAILED - Item data not found for id=%u", item_id);
		return nullptr;
	}

	// Create instance using the standard constructor
	auto* inst = new ItemInstance(item_data, charges);

	Log(Logs::General, Logs::Quests, "CreateItemInstance: SUCCESS - Created instance of '%s'",
		item_data->Name);

	return inst;
}

} // namespace EQ