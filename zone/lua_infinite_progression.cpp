#ifdef LUA_EQEMU

#include "lua.hpp"
#include <luabind/luabind.hpp>

#include "dynamic_item_manager.h"
#include "dynamic_item_integration.h"
#include "../common/eqemu_logsys.h"

// Lua-accessible function: Generate dynamic item ID
uint32 lua_generate_dynamic_id(uint32 base_item_id, int level) {
	auto& mgr = EQ::DynamicItemManager::Get();
	uint32 result = mgr.GenerateDynamicID(base_item_id, level);

	Log(Logs::General, Logs::Quests,
		"Lua: generate_dynamic_id(base=%u, level=%d) -> %u", base_item_id, level, result);

	return result;
}

// Lua-accessible function: Get item level from dynamic ID
int lua_get_item_level(uint32 item_id) {
	auto& mgr = EQ::DynamicItemManager::Get();
	int result = mgr.GetItemLevel(item_id);

	Log(Logs::Detail, Logs::Quests,
		"Lua: get_item_level(%u) -> %d", item_id, result);

	return result;
}

// Lua-accessible function: Get base item ID from dynamic ID
uint32 lua_get_base_item_id(uint32 item_id) {
	auto& mgr = EQ::DynamicItemManager::Get();
	uint32 result = mgr.GetBaseItemID(item_id);

	Log(Logs::Detail, Logs::Quests,
		"Lua: get_base_item_id(%u) -> %u", item_id, result);

	return result;
}

// Lua-accessible function: Check if item is dynamic
bool lua_is_dynamic_item(uint32 item_id) {
	auto& mgr = EQ::DynamicItemManager::Get();
	bool result = mgr.IsDynamicItem(item_id);

	Log(Logs::Detail, Logs::Quests,
		"Lua: is_dynamic_item(%u) -> %s", item_id, result ? "true" : "false");

	return result;
}

// Lua-accessible function: Clear item cache
void lua_clear_dynamic_cache() {
	Log(Logs::General, Logs::Quests, "Lua: clear_dynamic_cache()");

	auto& mgr = EQ::DynamicItemManager::Get();
	mgr.ClearCache();

	Log(Logs::General, Logs::Quests, "Lua: Cache cleared");
}

// Lua-accessible function: Get cache stats
luabind::object lua_get_cache_stats(lua_State* L) {
	// Create table with cache stats
	luabind::object table = luabind::newtable(L);
	table["enabled"] = true;
	table["max_size"] = 1000;

	Log(Logs::Detail, Logs::Quests, "Lua: get_cache_stats()");

	return table;
}

luabind::scope lua_register_infinite_progression() {
	return luabind::namespace_("inf")
	[
		luabind::def("generate_dynamic_id", &lua_generate_dynamic_id),
		luabind::def("get_item_level", &lua_get_item_level),
		luabind::def("get_base_item_id", &lua_get_base_item_id),
		luabind::def("is_dynamic_item", &lua_is_dynamic_item),
		luabind::def("clear_cache", &lua_clear_dynamic_cache),
		luabind::def("get_cache_stats", &lua_get_cache_stats)
	];
}

#endif