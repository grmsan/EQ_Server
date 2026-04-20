/*****************************************************************************
 * WaypointPOCWnd.cpp - Generic SIDL tool host for custom DLL UI modules
 *
 * Current tools:
 *   - Waypoints: account waypoint list + travel action
 *   - GM Dashboard: common development / ops commands from the runbook
 *
 * Compatibility:
 *   - /waypointpoc still opens the waypoint tool
 *   - /toolwnd opens the generic host
 *   - /gmdashboard opens the GM/dev dashboard tool
 *****************************************************************************/

#include "WaypointPOCWnd.h"
#include "eqgame.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

extern bool isDebugLoggingEnabled;
extern bool isWaypointPOCLoggingEnabled;

#define XWM_LCLICK    1
#define XWM_CLOSE     10
#define XWM_NEWVALUE  14

namespace {
struct WaypointPOCEntry {
	int32_t category_id = 0;
	int32_t waypoint_id = 0;
	bool enabled = false;
	std::string name;
};

struct DashboardEntry {
	const char* category;
	const char* title;
	const char* command;
	const char* description;
	bool runnable;
};

std::vector<WaypointPOCEntry> g_waypoint_entries;
bool g_group_enabled = false;
bool g_expedition_enabled = false;
bool g_group_selected = false;
bool g_autoconfirm_selected = false;
bool g_force_show = false;
DWORD g_last_waypoint_packet_tick = 0;
GenericToolMode g_active_tool = GenericToolMode::Waypoints;

const DashboardEntry kDashboardEntries[] = {
	{ "Inspect", "Print Location", "#loc", "Capture exact coordinates and heading for placement work.", true },
	{ "Zone", "Reload Quests", "#reloadquests", "Reload quest scripts in the current zone without restarting.", true },
	{ "Zone", "Repop Zone", "#repop", "Repopulate the current zone from database spawns.", true },
	{ "Spawns", "Show Spawn Status", "#show spawn_status all", "List spawn state for the current zone.", true },
	{ "Spawns", "Persist Target", "#spawnfix", "Save targeted NPC location and heading to spawn2.", true },
	{ "Spawns", "Save NPC Visuals", "#npcedit save", "Persist the target's current appearance and spawn position.", true },
	{ "Waypoints", "Spawn Runestone", "#npcspawn create 999300", "Spawn a faded runestone at your current location.", true },
	{ "Waypoints", "Unlock City Test", "#wp unlock qeynos2", "Example waypoint unlock command for quick client testing.", true },
	{ "Templates", "Spawn NPC By ID", "#spawn <npc_type_id>", "Template command. Replace <npc_type_id> manually in chat for ad-hoc spawns.", false },
	{ "Templates", "Goto NPC Name", "#goto <npc_name>", "Template command. Replace <npc_name> manually in chat for quick movement.", false },
};

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

const char* ToolWindowTitle(GenericToolMode mode)
{
	switch (mode) {
	case GenericToolMode::Waypoints:
		return "Waypoint Tools";
	case GenericToolMode::GMDashboard:
		return "GM Dashboard";
	default:
		return "Custom Tools";
	}
}

const char* ToolInfoLine(GenericToolMode mode)
{
	switch (mode) {
	case GenericToolMode::Waypoints:
		return "Select a waypoint and click Travel.";
	case GenericToolMode::GMDashboard:
		return "Select a command and click Run. Template rows describe manual commands.";
	default:
		return "";
	}
}

bool SelectWaypointPacketLayout(const char* buf, size_t size, size_t& entries_off, size_t& stride, uint32_t& count)
{
	struct Candidate {
		size_t count_off;
		size_t entry_stride;
		int score;
	};

	const Candidate candidates[] = {
		{ 8, 76, 4 },
		{ 8, 73, 3 },
		{ 5, 76, 2 },
		{ 5, 73, 1 }
	};

	int best_score = -1;
	bool found = false;
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
		if (required != size) {
			continue;
		}

		if (c.score > best_score) {
			best_score = c.score;
			entries_off = local_entries_off;
			stride = c.entry_stride;
			count = local_count;
			found = true;
		}
	}

	return found;
}

void SetButtonText(CButtonWnd* button, const char* text)
{
	if (button && text) {
		((CXWnd*)button)->SetWindowTextA(CXStr(text));
	}
}

void ExecuteSayCommand(const char* command)
{
	if (!pLocalPlayer || !command || !command[0]) {
		return;
	}

	char cmd[256] = { 0 };
	sprintf_s(cmd, "/say %s", command);
	DoCommand((PSPAWNINFO)pLocalPlayer, cmd);
}

void ExecuteClientCommand(const char* command)
{
	if (!pLocalPlayer || !command || !command[0]) {
		return;
	}

	DoCommand((PSPAWNINFO)pLocalPlayer, const_cast<char*>(command));
}

const DashboardEntry* GetSelectedDashboardEntry(const CListWnd* list)
{
	if (!list) {
		return nullptr;
	}

	const int row = list->GetCurSel();
	if (row < 0) {
		return nullptr;
	}

	const uint32_t index = list->GetItemData(row);
	if (index >= (sizeof(kDashboardEntries) / sizeof(kDashboardEntries[0]))) {
		return nullptr;
	}

	return &kDashboardEntries[index];
}
}

CWaypointPOCWnd* g_pWaypointPOCWnd = nullptr;
DWORD g_waypoint_packet_trace_until = 0;

CWaypointPOCWnd::CWaypointPOCWnd()
	: CCustomWnd("WaypointPOCWnd")
	, pModeWaypointsBtn(nullptr)
	, pModeDashboardBtn(nullptr)
	, pToolList(nullptr)
	, pPrimaryBtn(nullptr)
	, pSecondaryBtn(nullptr)
	, pCloseBtn(nullptr)
	, pStatusLabel(nullptr)
	, pInfoLabel(nullptr)
	, bShow(false)
	, bUIValid(true)
	, dwLastRefreshRequest(0)
{
	pModeWaypointsBtn = (CButtonWnd*)GetChildItem("GT_WaypointModeBtn");
	pModeDashboardBtn = (CButtonWnd*)GetChildItem("GT_DashboardModeBtn");
	pToolList = (CListWnd*)GetChildItem("GT_List");
	pPrimaryBtn = (CButtonWnd*)GetChildItem("GT_PrimaryBtn");
	pSecondaryBtn = (CButtonWnd*)GetChildItem("GT_SecondaryBtn");
	pCloseBtn = (CButtonWnd*)GetChildItem("GT_CloseBtn");
	pStatusLabel = GetChildItem("GT_StatusLabel");
	pInfoLabel = GetChildItem("GT_InfoLabel");

	if (!pModeWaypointsBtn || !pModeDashboardBtn || !pToolList || !pPrimaryBtn || !pSecondaryBtn || !pCloseBtn) {
		bUIValid = false;
		LogDebug("GenericToolWnd: WARNING - required child controls not found");
	}

	SetWndNotification(CWaypointPOCWnd);
	CloseOnESC = 0;

	if (pToolList) {
		pToolList->SetColumnWidth(0, 110);
		pToolList->SetColumnWidth(1, 330);
		pToolList->SetColumnWidth(2, 60);
	}

	UpdateToolChrome();
	RebuildActiveTool();
	LogDebug("GenericToolWnd: created (bUIValid=%d active_tool=%d)", bUIValid ? 1 : 0, static_cast<int>(g_active_tool));
}

CWaypointPOCWnd::~CWaypointPOCWnd()
{
	LogDebug("GenericToolWnd: destroyed");
}

void CWaypointPOCWnd::UpdateToolChrome()
{
	((CXWnd*)this)->SetWindowTextA(CXStr((char*)ToolWindowTitle(g_active_tool)));

	if (pToolList) {
		if (g_active_tool == GenericToolMode::Waypoints) {
			pToolList->SetColumnWidth(0, 110);
			pToolList->SetColumnWidth(1, 230);
			pToolList->SetColumnWidth(2, 60);
		} else {
			pToolList->SetColumnWidth(0, 110);
			pToolList->SetColumnWidth(1, 330);
			pToolList->SetColumnWidth(2, 60);
		}
	}

	if (g_active_tool == GenericToolMode::Waypoints) {
		SetButtonText(pPrimaryBtn, "Refresh");
		SetButtonText(pSecondaryBtn, "Travel");
	} else {
		SetButtonText(pPrimaryBtn, "Run");
		SetButtonText(pSecondaryBtn, "Details");
	}
	SetButtonText(pModeWaypointsBtn, "Waypoints");
	SetButtonText(pModeDashboardBtn, "GM Dash");
	SetButtonText(pCloseBtn, "Close");
}

void CWaypointPOCWnd::UpdateSelectionDetails()
{
	if (!pToolList || !pInfoLabel) {
		return;
	}

	const int row = pToolList->GetCurSel();
	if (row < 0) {
		pInfoLabel->SetWindowTextA(CXStr((char*)ToolInfoLine(g_active_tool)));
		return;
	}

	char info[256] = { 0 };
	if (g_active_tool == GenericToolMode::Waypoints) {
		const uint32_t waypoint_id = pToolList->GetItemData(row);
		if (waypoint_id > 0) {
			sprintf_s(info, "Selected waypoint id: %u", waypoint_id);
		} else {
			sprintf_s(info, "Selected waypoint is locked. Unlock it before travel.");
		}
	} else {
		const DashboardEntry* entry = GetSelectedDashboardEntry(pToolList);
		if (entry) {
			sprintf_s(info, "%s", entry->description);
		} else {
			sprintf_s(info, "%s", ToolInfoLine(g_active_tool));
		}
	}

	pInfoLabel->SetWindowTextA(CXStr(info));
}

void CWaypointPOCWnd::RequestWaypointListFromServer()
{
	if (!pLocalPlayer) {
		return;
	}

	dwLastRefreshRequest = GetTickCount();
	if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
		LogDebug("[WAYPOINT_SIDL] request list");
	}
	WaypointPOC_BeginPacketTrace("sidl");
	ExecuteSayCommand("#wppoc list");
}

void CWaypointPOCWnd::TravelToSelectedWaypoint()
{
	if (!pToolList || !pLocalPlayer) {
		return;
	}

	const int row = pToolList->GetCurSel();
	if (row < 0) {
		WriteChatColor("Waypoint Tools: select a waypoint first.", 0x0E);
		return;
	}

	const uint32_t waypoint_id = pToolList->GetItemData(row);
	if (waypoint_id == 0) {
		WriteChatColor("Waypoint Tools: selected waypoint is locked. Use #wp unlock <shortname> for testing.", 0x0E);
		return;
	}

	if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
		LogDebug("[WAYPOINT_SIDL] travel waypoint_id=%u", waypoint_id);
	}

	char cmd[64] = { 0 };
	sprintf_s(cmd, "#wppoc travel %u", waypoint_id);
	ExecuteSayCommand(cmd);
}

void CWaypointPOCWnd::RunSelectedDashboardCommand(bool dry_run)
{
	if (!pToolList) {
		return;
	}

	const DashboardEntry* entry = GetSelectedDashboardEntry(pToolList);
	if (!entry) {
		WriteChatColor("GM Dashboard: select a command first.", 0x0E);
		return;
	}

	if (dry_run || !entry->runnable) {
		char line[256] = { 0 };
		sprintf_s(line, "GM Dashboard: %s", entry->description);
		WriteChatColor(line, 0x0E);
		if (pInfoLabel) {
			pInfoLabel->SetWindowTextA(CXStr((char*)entry->description));
		}
		return;
	}

	if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
		LogDebug("[GENERIC_TOOL] gm_dashboard run command=%s", entry->command);
	}
	ExecuteClientCommand(entry->command);
}

void CWaypointPOCWnd::RebuildActiveTool()
{
	if (!pToolList) {
		return;
	}

	UpdateToolChrome();
	pToolList->DeleteAll();

	if (g_active_tool == GenericToolMode::Waypoints) {
		int available_count = 0;
		const int total_count = static_cast<int>(g_waypoint_entries.size());

		for (const auto& e : g_waypoint_entries) {
			const int row = pToolList->AddString(" ", 0xFFFFFFFF, static_cast<uint32_t>(e.waypoint_id), nullptr);
			if (row < 0) {
				continue;
			}

			if (e.enabled) {
				++available_count;
			}

			pToolList->SetItemData(row, e.enabled ? static_cast<uint32_t>(e.waypoint_id) : 0);

			CXStr cat((char*)CategoryName(e.category_id));
			std::string display_name = e.name;
			if (!e.enabled) {
				display_name += " [Locked]";
			}
			CXStr name((char*)display_name.c_str());

			char id_buf[16] = { 0 };
			sprintf_s(id_buf, "%d", e.waypoint_id);
			CXStr id(id_buf);

			pToolList->SetItemText(row, 0, &cat);
			pToolList->SetItemText(row, 1, &name);
			pToolList->SetItemText(row, 2, &id);

			const DWORD color = e.enabled ? 0xFFFFFFFF : 0xFF808080;
			pToolList->SetItemColor(row, 0, color);
			pToolList->SetItemColor(row, 1, color);
			pToolList->SetItemColor(row, 2, color);
		}

		if (pStatusLabel) {
			char status[256] = { 0 };
			sprintf_s(
				status,
				"Waypoints: %d available / %d total | Group: %s | Auto: %s",
				available_count,
				total_count,
				g_group_selected ? "On" : "Off",
				g_autoconfirm_selected ? "On" : "Off"
			);
			pStatusLabel->SetWindowTextA(CXStr(status));
		}

		if (pInfoLabel) {
			char info[256] = { 0 };
			const DWORD now = GetTickCount();
			sprintf_s(
				info,
				"Last packet: %u ms ago | Expedition: %s | ForceShow: %s",
				(g_last_waypoint_packet_tick > 0) ? (now - g_last_waypoint_packet_tick) : 0,
				g_expedition_enabled ? "Yes" : "No",
				g_force_show ? "Yes" : "No"
			);
			pInfoLabel->SetWindowTextA(CXStr(info));
		}
	} else {
		for (uint32_t i = 0; i < (sizeof(kDashboardEntries) / sizeof(kDashboardEntries[0])); ++i) {
			const DashboardEntry& e = kDashboardEntries[i];
			const int row = pToolList->AddString(" ", 0xFFFFFFFF, i, nullptr);
			if (row < 0) {
				continue;
			}

			CXStr category((char*)e.category);
			CXStr title((char*)e.title);
			CXStr mode((char*)(e.runnable ? "Run" : "Manual"));
			pToolList->SetItemText(row, 0, &category);
			pToolList->SetItemText(row, 1, &title);
			pToolList->SetItemText(row, 2, &mode);

			const DWORD color = e.runnable ? 0xFFFFFFFF : 0xFFB0A060;
			pToolList->SetItemColor(row, 0, color);
			pToolList->SetItemColor(row, 1, color);
			pToolList->SetItemColor(row, 2, color);
		}

		if (pStatusLabel) {
			char status[256] = { 0 };
			sprintf_s(
				status,
				"GM Dashboard: %u commands | target-aware placement, zone refresh, and waypoint helpers",
				static_cast<unsigned>(sizeof(kDashboardEntries) / sizeof(kDashboardEntries[0]))
			);
			pStatusLabel->SetWindowTextA(CXStr(status));
		}

		if (pInfoLabel) {
			pInfoLabel->SetWindowTextA(CXStr((char*)ToolInfoLine(g_active_tool)));
		}
	}

	UpdateSelectionDetails();
}

void CWaypointPOCWnd::SwitchTool(GenericToolMode mode, bool ensure_visible)
{
	g_active_tool = mode;
	RebuildActiveTool();
	if (ensure_visible) {
		bShow = true;
		((CXWnd*)this)->Show(1, 1);
	}
}

void CWaypointPOCWnd::Toggle()
{
	bShow = !bShow;
	((CXWnd*)this)->Show(bShow ? 1 : 0, bShow ? 1 : 0);
	if (bShow) {
		RebuildActiveTool();
	}
}

int CWaypointPOCWnd::WndNotification(CXWnd* pWnd, unsigned int Message, void* unknown)
{
	(void)unknown;

	if (Message == XWM_LCLICK) {
		if (pWnd == (CXWnd*)pModeWaypointsBtn) {
			SwitchTool(GenericToolMode::Waypoints, true);
			return 0;
		}
		if (pWnd == (CXWnd*)pModeDashboardBtn) {
			SwitchTool(GenericToolMode::GMDashboard, true);
			return 0;
		}
		if (pWnd == (CXWnd*)pPrimaryBtn) {
			if (g_active_tool == GenericToolMode::Waypoints) {
				RequestWaypointListFromServer();
			} else {
				RunSelectedDashboardCommand(false);
			}
			return 0;
		}
		if (pWnd == (CXWnd*)pSecondaryBtn) {
			if (g_active_tool == GenericToolMode::Waypoints) {
				TravelToSelectedWaypoint();
			} else {
				RunSelectedDashboardCommand(true);
			}
			return 0;
		}
		if (pWnd == (CXWnd*)pCloseBtn) {
			bShow = false;
			((CXWnd*)this)->Show(0, 0);
			return 0;
		}
	} else if (Message == XWM_NEWVALUE) {
		if (pWnd == (CXWnd*)pToolList) {
			UpdateSelectionDetails();
		}
	} else if (Message == XWM_CLOSE) {
		bShow = false;
		((CXWnd*)this)->Show(0, 0);
		return 0;
	}

	return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
}

void WaypointPOCWnd_OnWaypointListPacket(const char* buf, size_t size)
{
	if (!buf || size < 5) {
		return;
	}

	g_group_enabled = (buf[0] != 0);
	g_expedition_enabled = (buf[1] != 0);
	g_group_selected = (buf[2] != 0);
	g_force_show = (buf[3] != 0);
	g_autoconfirm_selected = (buf[4] != 0);

	size_t entries_off = 0;
	size_t stride = 0;
	uint32_t count = 0;
	if (!SelectWaypointPacketLayout(buf, size, entries_off, stride, count)) {
		LogDebug("GenericToolWnd: failed to parse OP_WaypointList packet (size=%zu)", size);
		return;
	}

	std::vector<WaypointPOCEntry> parsed;
	parsed.reserve(count);

	for (uint32_t i = 0; i < count; ++i) {
		const size_t off = entries_off + (static_cast<size_t>(i) * stride);
		if (off + 73 > size) {
			break;
		}

		WaypointPOCEntry e;
		memcpy(&e.category_id, buf + off, sizeof(int32_t));
		memcpy(&e.waypoint_id, buf + off + 4, sizeof(int32_t));
		e.enabled = (*(reinterpret_cast<const uint8_t*>(buf + off + 8)) != 0);

		char name_buf[65] = { 0 };
		memcpy(name_buf, buf + off + 9, 64);
		name_buf[64] = '\0';
		e.name = name_buf;

		parsed.push_back(std::move(e));
	}

	g_waypoint_entries = std::move(parsed);
	g_last_waypoint_packet_tick = GetTickCount();

	LogDebug(
		"GenericToolWnd: parsed waypoint packet count=%u parsed=%u stride=%zu group_enabled=%d expedition_enabled=%d",
		count,
		static_cast<unsigned>(g_waypoint_entries.size()),
		stride,
		g_group_enabled ? 1 : 0,
		g_expedition_enabled ? 1 : 0
	);

	if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
		unsigned enabled_count = 0;
		for (const auto& e : g_waypoint_entries) {
			if (e.enabled) {
				++enabled_count;
			}
		}
		LogDebug("[WAYPOINT_SIDL] packet parsed total=%u enabled=%u force_show=%d",
			static_cast<unsigned>(g_waypoint_entries.size()),
			enabled_count,
			g_force_show ? 1 : 0);
	}

	if (g_pWaypointPOCWnd) {
		g_pWaypointPOCWnd->SwitchTool(GenericToolMode::Waypoints, g_force_show);
	}
}

void WaypointPOC_BeginPacketTrace(const char* source)
{
	g_waypoint_packet_trace_until = GetTickCount() + 8000;
	if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
		LogDebug("[WAYPOINT_TRACE] armed source=%s window_ms=8000", source ? source : "unknown");
	}
}

void WaypointPOC_LogIncomingOpcode(uint16_t opcode, size_t size)
{
	const DWORD now = GetTickCount();
	if ((LONG)(g_waypoint_packet_trace_until - now) <= 0) {
		return;
	}

	if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
		LogDebug("[WAYPOINT_TRACE] opcode=0x%04X size=%zu", opcode, size);
	}
}

bool WaypointPOC_IsPacketTraceActive()
{
	const DWORD now = GetTickCount();
	return ((LONG)(g_waypoint_packet_trace_until - now) > 0);
}

void WaypointPOCWnd_Create()
{
	if (g_pWaypointPOCWnd) {
		WaypointPOCWnd_Destroy();
	}

	if (pSidlMgr && ((CSidlManager*)pSidlMgr)->FindScreenPieceTemplate("WaypointPOCWnd")) {
		g_pWaypointPOCWnd = new CWaypointPOCWnd();
		if (g_pWaypointPOCWnd && g_pWaypointPOCWnd->bShow) {
			((CXWnd*)g_pWaypointPOCWnd)->Show(1, 1);
		}
		LogDebug("GenericToolWnd_Create: created");
	} else {
		LogDebug("GenericToolWnd_Create: template 'WaypointPOCWnd' not found (missing EQUI_WaypointPOCWnd.xml/default.xml entry)");
	}
}

void WaypointPOCWnd_Destroy()
{
	if (g_pWaypointPOCWnd) {
		delete g_pWaypointPOCWnd;
		g_pWaypointPOCWnd = nullptr;
	}
}

void WaypointPOCWnd_Pulse()
{
	if (!g_pWaypointPOCWnd || gGameState != GAMESTATE_INGAME) {
		return;
	}
}

void WaypointPOCWnd_CleanUI()
{
	WaypointPOCWnd_Destroy();
}

void WaypointPOCWnd_ReloadUI()
{
	if (gGameState == GAMESTATE_INGAME && pCharSpawn) {
		WaypointPOCWnd_Create();
	}
}

void WaypointPOCWnd_SetGameState(DWORD gs)
{
	if (gs == GAMESTATE_INGAME) {
		WaypointPOCWnd_Create();
	} else {
		WaypointPOCWnd_Destroy();
	}
}

void WaypointPOCCmd(PSPAWNINFO pChar, PCHAR szLine)
{
	(void)pChar;

	if (!g_pWaypointPOCWnd) {
		WaypointPOCWnd_Create();
	}
	if (!g_pWaypointPOCWnd) {
		return;
	}

	if (!szLine || !szLine[0] || !_stricmp(szLine, "show")) {
		g_pWaypointPOCWnd->SwitchTool(GenericToolMode::Waypoints, true);
		return;
	}

	g_pWaypointPOCWnd->SwitchTool(GenericToolMode::Waypoints, true);

	if (!_stricmp(szLine, "refresh")) {
		g_pWaypointPOCWnd->RequestWaypointListFromServer();
		return;
	}

	if (!_stricmp(szLine, "travel")) {
		g_pWaypointPOCWnd->TravelToSelectedWaypoint();
		return;
	}

	WriteChatColor("Usage: /waypointpoc [show|refresh|travel]", 0x0E);
}

void GenericToolWndCmd(PSPAWNINFO pChar, PCHAR szLine)
{
	(void)pChar;

	if (!g_pWaypointPOCWnd) {
		WaypointPOCWnd_Create();
	}
	if (!g_pWaypointPOCWnd) {
		return;
	}

	if (!szLine || !szLine[0] || !_stricmp(szLine, "show")) {
		g_pWaypointPOCWnd->SwitchTool(g_active_tool, true);
		return;
	}

	if (!_stricmp(szLine, "waypoints")) {
		g_pWaypointPOCWnd->SwitchTool(GenericToolMode::Waypoints, true);
		return;
	}

	if (!_stricmp(szLine, "gm") || !_stricmp(szLine, "dashboard")) {
		g_pWaypointPOCWnd->SwitchTool(GenericToolMode::GMDashboard, true);
		return;
	}

	if (!_stricmp(szLine, "refresh")) {
		if (g_active_tool == GenericToolMode::Waypoints) {
			g_pWaypointPOCWnd->RequestWaypointListFromServer();
		} else {
			g_pWaypointPOCWnd->RebuildActiveTool();
		}
		return;
	}

	if (!_stricmp(szLine, "primary")) {
		if (g_active_tool == GenericToolMode::Waypoints) {
			g_pWaypointPOCWnd->RequestWaypointListFromServer();
		} else {
			g_pWaypointPOCWnd->RunSelectedDashboardCommand(false);
		}
		return;
	}

	if (!_stricmp(szLine, "secondary")) {
		if (g_active_tool == GenericToolMode::Waypoints) {
			g_pWaypointPOCWnd->TravelToSelectedWaypoint();
		} else {
			g_pWaypointPOCWnd->RunSelectedDashboardCommand(true);
		}
		return;
	}

	WriteChatColor("Usage: /toolwnd [show|waypoints|gm|refresh|primary|secondary]", 0x0E);
}

void GMDashboardCmd(PSPAWNINFO pChar, PCHAR szLine)
{
	(void)pChar;

	if (!g_pWaypointPOCWnd) {
		WaypointPOCWnd_Create();
	}
	if (!g_pWaypointPOCWnd) {
		return;
	}

	if (!szLine || !szLine[0] || !_stricmp(szLine, "show")) {
		g_pWaypointPOCWnd->SwitchTool(GenericToolMode::GMDashboard, true);
		return;
	}

	g_pWaypointPOCWnd->SwitchTool(GenericToolMode::GMDashboard, true);

	if (!_stricmp(szLine, "run")) {
		g_pWaypointPOCWnd->RunSelectedDashboardCommand(false);
		return;
	}

	if (!_stricmp(szLine, "details")) {
		g_pWaypointPOCWnd->RunSelectedDashboardCommand(true);
		return;
	}

	if (!_stricmp(szLine, "refresh")) {
		g_pWaypointPOCWnd->RebuildActiveTool();
		return;
	}

	WriteChatColor("Usage: /gmdashboard [show|run|details|refresh]", 0x0E);
}
