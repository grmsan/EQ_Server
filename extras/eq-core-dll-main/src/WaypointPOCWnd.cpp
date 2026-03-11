/*****************************************************************************
 * WaypointPOCWnd.cpp - POC custom SIDL window for waypoint travel
 *
 * Demonstrates:
 *   - Parsing server waypoint packets (OP_WaypointList / 0x1402 RoF2)
 *   - Rendering available waypoints in a custom CCustomWnd list
 *   - Sending travel actions back to server through # command bridge
 *
 * XML requirements:
 *   - EQUI_WaypointPOCWnd.xml in <EQ_Client_Dir>/uifiles/default/
 *   - Add this XML filename to uifiles/default/default.xml
 *****************************************************************************/

#include "WaypointPOCWnd.h"
#include "eqgame.h"

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <utility>

// WndNotification message IDs
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

std::vector<WaypointPOCEntry> g_waypoint_entries;
bool g_group_enabled = false;
bool g_expedition_enabled = false;
bool g_group_selected = false;
bool g_autoconfirm_selected = false;
bool g_force_show = false;
DWORD g_last_waypoint_packet_tick = 0;

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

bool SelectWaypointPacketLayout(const char* buf, size_t size, size_t& entries_off, size_t& stride, uint32_t& count)
{
	// Server build typically uses natural alignment:
	//   header count at offset 8, entry stride 76.
	// DLL build uses 1-byte member alignment, so we also accept packed fallbacks.
	struct Candidate {
		size_t count_off;
		size_t entry_stride;
		int score;
	};

	const Candidate candidates[] = {
		{ 8, 76, 4 }, // expected server layout
		{ 8, 73, 3 },
		{ 5, 76, 2 },
		{ 5, 73, 1 }  // fully packed fallback
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
		if (required > size) {
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
}

CWaypointPOCWnd* g_pWaypointPOCWnd = nullptr;

CWaypointPOCWnd::CWaypointPOCWnd()
	: CCustomWnd("WaypointPOCWnd")
	, pWaypointList(nullptr)
	, pRefreshBtn(nullptr)
	, pTravelBtn(nullptr)
	, pCloseBtn(nullptr)
	, pStatusLabel(nullptr)
	, pInfoLabel(nullptr)
	, bShow(false)
	, bUIValid(true)
	, dwLastRefreshRequest(0)
{
	pWaypointList = (CListWnd*)GetChildItem("WP_List");
	pRefreshBtn = (CButtonWnd*)GetChildItem("WP_RefreshBtn");
	pTravelBtn = (CButtonWnd*)GetChildItem("WP_TravelBtn");
	pCloseBtn = (CButtonWnd*)GetChildItem("WP_CloseBtn");
	pStatusLabel = GetChildItem("WP_StatusLabel");
	pInfoLabel = GetChildItem("WP_InfoLabel");

	if (!pWaypointList || !pRefreshBtn || !pTravelBtn || !pCloseBtn) {
		bUIValid = false;
		LogDebug("WaypointPOCWnd: WARNING - required child controls not found");
	}

	SetWndNotification(CWaypointPOCWnd);
	CloseOnESC = 0;

	RebuildWaypointList();
	RequestWaypointListFromServer();
	LogDebug("WaypointPOCWnd: created (bUIValid=%d)", bUIValid ? 1 : 0);
}

CWaypointPOCWnd::~CWaypointPOCWnd()
{
	LogDebug("WaypointPOCWnd: destroyed");
}

void CWaypointPOCWnd::RequestWaypointListFromServer()
{
	if (!pLocalPlayer) {
		return;
	}

	dwLastRefreshRequest = GetTickCount();
	DoCommand((PSPAWNINFO)pLocalPlayer, "/say #wppoc list");
}

void CWaypointPOCWnd::TravelToSelectedWaypoint()
{
	if (!pWaypointList || !pLocalPlayer) {
		return;
	}

	const int row = pWaypointList->GetCurSel();
	if (row < 0) {
		WriteChatColor("Waypoint POC: select a waypoint first.", 0x0E);
		return;
	}

	const uint32_t waypoint_id = pWaypointList->GetItemData(row);
	if (waypoint_id == 0) {
		WriteChatColor("Waypoint POC: invalid waypoint selection.", 0x0D);
		return;
	}

	char cmd[128] = { 0 };
	sprintf_s(cmd, "/say #wppoc travel %u", waypoint_id);
	DoCommand((PSPAWNINFO)pLocalPlayer, cmd);
}

void CWaypointPOCWnd::RebuildWaypointList()
{
	if (!pWaypointList) {
		return;
	}

	pWaypointList->DeleteAll();

	int available_count = 0;
	const int total_count = static_cast<int>(g_waypoint_entries.size());

	for (const auto& e : g_waypoint_entries) {
		if (!e.enabled) {
			continue;
		}

		const int row = pWaypointList->AddString(" ", 0xFFFFFFFF, static_cast<uint32_t>(e.waypoint_id), nullptr);
		if (row < 0) {
			continue;
		}

		++available_count;
		pWaypointList->SetItemData(row, static_cast<uint32_t>(e.waypoint_id));

		CXStr cat((char*)CategoryName(e.category_id));
		CXStr name((char*)e.name.c_str());

		char id_buf[16] = { 0 };
		sprintf_s(id_buf, "%d", e.waypoint_id);
		CXStr id(id_buf);

		pWaypointList->SetItemText(row, 0, &cat);
		pWaypointList->SetItemText(row, 1, &name);
		pWaypointList->SetItemText(row, 2, &id);
	}

	if (pStatusLabel) {
		char status[256] = { 0 };
		sprintf_s(
			status,
			"Available: %d  |  Total: %d  |  Group: %s  |  Auto: %s",
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
}

void CWaypointPOCWnd::Toggle()
{
	bShow = !bShow;
	((CXWnd*)this)->Show(bShow ? 1 : 0, bShow ? 1 : 0);
	if (bShow) {
		RebuildWaypointList();
	}
}

int CWaypointPOCWnd::WndNotification(CXWnd* pWnd, unsigned int Message, void* unknown)
{
	(void)unknown;

	if (Message == XWM_LCLICK) {
		if (pWnd == (CXWnd*)pRefreshBtn) {
			RequestWaypointListFromServer();
			return 0;
		}
		if (pWnd == (CXWnd*)pTravelBtn) {
			TravelToSelectedWaypoint();
			return 0;
		}
		if (pWnd == (CXWnd*)pCloseBtn) {
			bShow = false;
			((CXWnd*)this)->Show(0, 0);
			return 0;
		}
	}
	else if (Message == XWM_NEWVALUE) {
		if (pWnd == (CXWnd*)pWaypointList && pInfoLabel) {
			const int row = pWaypointList->GetCurSel();
			if (row >= 0) {
				const uint32_t waypoint_id = pWaypointList->GetItemData(row);
				char info[128] = { 0 };
				sprintf_s(info, "Selected waypoint id: %u", waypoint_id);
				pInfoLabel->SetWindowTextA(CXStr(info));
			}
		}
	}
	else if (Message == XWM_CLOSE) {
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
		LogDebug("WaypointPOCWnd: failed to parse OP_WaypointList packet (size=%zu)", size);
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
		"WaypointPOCWnd: parsed waypoint packet count=%u parsed=%u stride=%zu group_enabled=%d expedition_enabled=%d",
		count,
		static_cast<unsigned>(g_waypoint_entries.size()),
		stride,
		g_group_enabled ? 1 : 0,
		g_expedition_enabled ? 1 : 0
	);

	if (g_pWaypointPOCWnd) {
		g_pWaypointPOCWnd->RebuildWaypointList();
		if (g_force_show) {
			g_pWaypointPOCWnd->bShow = true;
			((CXWnd*)g_pWaypointPOCWnd)->Show(1, 1);
		}
	}
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
		LogDebug("WaypointPOCWnd_Create: created");
	} else {
		LogDebug("WaypointPOCWnd_Create: template 'WaypointPOCWnd' not found (missing EQUI_WaypointPOCWnd.xml/default.xml entry)");
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

	if (!szLine || !szLine[0] || !_stricmp(szLine, "show")) {
		if (g_pWaypointPOCWnd) {
			g_pWaypointPOCWnd->Toggle();
		} else {
			WaypointPOCWnd_Create();
		}
		return;
	}

	if (!_stricmp(szLine, "refresh")) {
		if (g_pWaypointPOCWnd) {
			g_pWaypointPOCWnd->RequestWaypointListFromServer();
		}
		return;
	}

	if (!_stricmp(szLine, "travel")) {
		if (g_pWaypointPOCWnd) {
			g_pWaypointPOCWnd->TravelToSelectedWaypoint();
		}
		return;
	}

	WriteChatColor("Usage: /waypointpoc [show|refresh|travel]", 0x0E);
}
