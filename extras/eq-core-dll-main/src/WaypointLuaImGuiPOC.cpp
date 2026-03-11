#include "WaypointLuaImGuiPOC.h"
#include "MQ2Main.h"

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

// 0 = scaffold mode (no embedded Lua/ImGui runtime in current repo build)
// 1 = real runtime mode (requires x86 Lua + ImGui libs and hook glue)
#define WAYPOINT_LUA_IMGUI_RUNTIME_AVAILABLE 0

namespace {
struct WaypointEntry {
	int32_t category_id = 0;
	int32_t waypoint_id = 0;
	std::string name;
};

std::vector<WaypointEntry> g_entries;
bool g_enabled = false;
bool g_have_packet = false;
int g_selected_index = 0;

int MinInt(int a, int b)
{
	return (a < b) ? a : b;
}

int MaxInt(int a, int b)
{
	return (a > b) ? a : b;
}

int ClampIndex(int value, int min_value, int max_value)
{
	if (value < min_value) {
		return min_value;
	}
	if (value > max_value) {
		return max_value;
	}
	return value;
}

bool SelectLayout(const char* buf, size_t size, size_t& entries_off, size_t& stride, uint32_t& count)
{
	struct Candidate { size_t count_off; size_t entry_stride; };
	const Candidate candidates[] = { {8,76}, {8,73}, {5,76}, {5,73} };

	for (const auto& c : candidates) {
		if (size < c.count_off + sizeof(uint32_t)) {
			continue;
		}

		uint32_t local_count = 0;
		memcpy(&local_count, buf + c.count_off, sizeof(uint32_t));
		if (local_count > 4096) {
			continue;
		}

		const size_t local_entries_off = c.count_off + sizeof(uint32_t);
		const size_t required = local_entries_off + (static_cast<size_t>(local_count) * c.entry_stride);
		if (required > size) {
			continue;
		}

		entries_off = local_entries_off;
		stride = c.entry_stride;
		count = local_count;
		return true;
	}

	return false;
}

void RequestList()
{
	if (pLocalPlayer) {
		DoCommand((PSPAWNINFO)pLocalPlayer, "/say #wppoc list");
	}
}

void TravelSelected()
{
	if (!pLocalPlayer || g_entries.empty()) {
		WriteChatColor("Lua/ImGui POC: no waypoint selected.", 0x0E);
		return;
	}

	g_selected_index = ClampIndex(g_selected_index, 0, static_cast<int>(g_entries.size()) - 1);
	const auto& selected = g_entries[g_selected_index];

	char cmd[128] = { 0 };
	sprintf_s(cmd, "/say #wppoc travel %d", selected.waypoint_id);
	DoCommand((PSPAWNINFO)pLocalPlayer, cmd);
}
}

void WaypointLuaImGuiPOC_OnWaypointListPacket(const char* buf, size_t size)
{
	if (!buf || size < 5) {
		return;
	}

	size_t entries_off = 0;
	size_t stride = 0;
	uint32_t count = 0;
	if (!SelectLayout(buf, size, entries_off, stride, count)) {
		return;
	}

	std::vector<WaypointEntry> parsed;
	parsed.reserve(count);

	for (uint32_t i = 0; i < count; ++i) {
		const size_t off = entries_off + (static_cast<size_t>(i) * stride);
		if (off + 73 > size) {
			break;
		}

		const uint8_t enabled = *(reinterpret_cast<const uint8_t*>(buf + off + 8));
		if (!enabled) {
			continue;
		}

		WaypointEntry e;
		memcpy(&e.category_id, buf + off, sizeof(int32_t));
		memcpy(&e.waypoint_id, buf + off + 4, sizeof(int32_t));

		char name_buf[65] = { 0 };
		memcpy(name_buf, buf + off + 9, 64);
		name_buf[64] = '\0';
		e.name = name_buf;
		parsed.push_back(std::move(e));
	}

	g_entries = std::move(parsed);
	g_have_packet = true;
	if (g_selected_index >= static_cast<int>(g_entries.size())) {
		g_selected_index = MaxInt(0, static_cast<int>(g_entries.size()) - 1);
	}
}

void WaypointLuaImGuiPOC_Draw()
{
	if (!g_enabled || gGameState != GAMESTATE_INGAME) {
		return;
	}

#if WAYPOINT_LUA_IMGUI_RUNTIME_AVAILABLE
	// Real runtime placeholder:
	// - Begin ImGui frame
	// - Call bound Lua draw function (e.g., waypoint_imgui.draw(state))
	// - End frame
#else
	// Scaffold mode visual so this path is testable in current build.
	const int x = 20;
	int y = 320;
	char line[256] = { 0 };

	DrawHUDText("[Lua+ImGui Waypoint POC] STUB MODE (runtime deps missing)", x, y, 0xFFFF8080, 2);
	y += 16;

	if (!g_have_packet) {
		DrawHUDText("Use /waypointimgui refresh to request waypoint data", x, y, 0xFFFFFF00, 2);
		return;
	}

	sprintf_s(line, "entries=%d selected=%d", static_cast<int>(g_entries.size()), g_selected_index + 1);
	DrawHUDText(line, x, y, 0xFFE0E0E0, 2);
	y += 14;

	const int show = MinInt(4, static_cast<int>(g_entries.size()));
	for (int i = 0; i < show; ++i) {
		const int idx = ClampIndex(g_selected_index + i, 0, static_cast<int>(g_entries.size()) - 1);
		const auto& e = g_entries[idx];
		sprintf_s(line, "[%d] %s", e.waypoint_id, e.name.c_str());
		DrawHUDText(line, x, y, (i == 0) ? 0xFF80FF80 : 0xFFD0D0D0, 1);
		y += 12;
	}
#endif
}

void WaypointLuaImGuiPOC_CleanUI()
{
	g_selected_index = 0;
}

void WaypointLuaImGuiPOCCmd(PSPAWNINFO pChar, PCHAR szLine)
{
	(void)pChar;

	if (!szLine || !szLine[0] || !_stricmp(szLine, "toggle") || !_stricmp(szLine, "show")) {
		g_enabled = !g_enabled;
		if (g_enabled && !g_have_packet) {
			RequestList();
		}
		WriteChatColor(g_enabled ? "Lua/ImGui waypoint POC: ON" : "Lua/ImGui waypoint POC: OFF", 0x0D);
		return;
	}

	if (!_stricmp(szLine, "status")) {
#if WAYPOINT_LUA_IMGUI_RUNTIME_AVAILABLE
		WriteChatColor("Lua/ImGui waypoint POC: runtime available", 0x0D);
#else
		WriteChatColor("Lua/ImGui waypoint POC: scaffold mode (x86 Lua/ImGui deps missing)", 0x0E);
#endif
		return;
	}

	if (!_stricmp(szLine, "refresh")) {
		RequestList();
		return;
	}

	if (!_stricmp(szLine, "next")) {
		if (!g_entries.empty()) {
			g_selected_index = MinInt(static_cast<int>(g_entries.size()) - 1, g_selected_index + 1);
		}
		return;
	}

	if (!_stricmp(szLine, "prev")) {
		if (!g_entries.empty()) {
			g_selected_index = MaxInt(0, g_selected_index - 1);
		}
		return;
	}

	if (!_stricmp(szLine, "travel")) {
		TravelSelected();
		return;
	}

	if (!_stricmp(szLine, "reload")) {
		WriteChatColor("Lua/ImGui waypoint POC: reload requested (runtime not active in scaffold mode).", 0x0E);
		return;
	}

	WriteChatColor("Usage: /waypointimgui [toggle|status|refresh|next|prev|travel|reload]", 0x0E);
}
