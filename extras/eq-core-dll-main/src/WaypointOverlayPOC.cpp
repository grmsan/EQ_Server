#include "WaypointOverlayPOC.h"
#include "MQ2Main.h"

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

namespace {
struct WaypointEntry {
	int32_t category_id = 0;
	int32_t waypoint_id = 0;
	std::string name;
};

std::vector<WaypointEntry> g_entries;
bool g_overlay_enabled = false;
int g_selected_index = 0;
bool g_have_packet = false;

int MinInt(int a, int b)
{
	return (a < b) ? a : b;
}

int MaxInt(int a, int b)
{
	return (a > b) ? a : b;
}

const char* CategoryName(int category_id)
{
	switch (category_id) {
	case 0: return "Antonica";
	case 1: return "Faydwer";
	case 2: return "Odus";
	case 3: return "Kunark";
	case 4: return "Velious";
	case 5: return "Luclin";
	case 6: return "Planes";
	case 7: return "Dungeons";
	case 8: return "Other";
	case 9: return "Utility";
	default: return "Unknown";
	}
}

bool SelectLayout(const char* buf, size_t size, size_t& entries_off, size_t& stride, uint32_t& count)
{
	struct Candidate {
		size_t count_off;
		size_t entry_stride;
	};

	const Candidate candidates[] = {
		{ 8, 76 },
		{ 8, 73 },
		{ 5, 76 },
		{ 5, 73 }
	};

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

void RequestListFromServer()
{
	if (!pLocalPlayer) {
		return;
	}
	DoCommand((PSPAWNINFO)pLocalPlayer, "/say #wppoc list");
}

void TravelSelected()
{
	if (!pLocalPlayer || g_entries.empty()) {
		WriteChatColor("Waypoint Overlay POC: no available waypoints loaded.", 0x0E);
		return;
	}

	if (g_selected_index < 0) {
		g_selected_index = 0;
	}
	if (g_selected_index >= static_cast<int>(g_entries.size())) {
		g_selected_index = static_cast<int>(g_entries.size()) - 1;
	}

	const auto& selected = g_entries[g_selected_index];
	char cmd[128] = { 0 };
	sprintf_s(cmd, "/say #wppoc travel %d", selected.waypoint_id);
	DoCommand((PSPAWNINFO)pLocalPlayer, cmd);
}
}

void WaypointOverlayPOC_OnWaypointListPacket(const char* buf, size_t size)
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

void WaypointOverlayPOC_Draw()
{
	if (!g_overlay_enabled || gGameState != GAMESTATE_INGAME) {
		return;
	}

	const int x = 20;
	int y = 120;

	char line[256] = { 0 };
	sprintf_s(line, "[Waypoint Overlay POC] entries=%d selected=%d", static_cast<int>(g_entries.size()), g_selected_index + 1);
	DrawHUDText(line, x, y, 0xFF80FFFF, 2);
	y += 16;

	if (!g_have_packet) {
		DrawHUDText("Waiting for waypoint list. Use /waypointoverlay refresh", x, y, 0xFFFFFF00, 2);
		y += 16;
	}

	const int window = 8;
	int start = MaxInt(0, g_selected_index - (window / 2));
	if (start + window > static_cast<int>(g_entries.size())) {
		start = MaxInt(0, static_cast<int>(g_entries.size()) - window);
	}

	for (int i = 0; i < window; ++i) {
		const int idx = start + i;
		if (idx >= static_cast<int>(g_entries.size())) {
			break;
		}

		const auto& e = g_entries[idx];
		const bool is_selected = (idx == g_selected_index);
		sprintf_s(
			line,
			"%c [%d] %s (%s)",
			is_selected ? '>' : ' ',
			e.waypoint_id,
			e.name.c_str(),
			CategoryName(e.category_id)
		);
		DrawHUDText(line, x, y, is_selected ? 0xFF00FF00 : 0xFFE0E0E0, 2);
		y += 14;
	}

	DrawHUDText("Controls: /waypointoverlay next|prev|travel|refresh", x, y + 6, 0xFFB0B0B0, 1);
}

void WaypointOverlayPOC_CleanUI()
{
	g_selected_index = 0;
	g_have_packet = false;
	g_entries.clear();
}

void WaypointOverlayPOCCmd(PSPAWNINFO pChar, PCHAR szLine)
{
	(void)pChar;

	if (!szLine || !szLine[0] || !_stricmp(szLine, "show") || !_stricmp(szLine, "toggle")) {
		g_overlay_enabled = !g_overlay_enabled;
		if (g_overlay_enabled && !g_have_packet) {
			RequestListFromServer();
		}
		WriteChatColor(g_overlay_enabled ? "Waypoint Overlay POC: ON" : "Waypoint Overlay POC: OFF", 0x0D);
		return;
	}

	if (!_stricmp(szLine, "refresh")) {
		RequestListFromServer();
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

	WriteChatColor("Usage: /waypointoverlay [toggle|refresh|next|prev|travel]", 0x0E);
}
