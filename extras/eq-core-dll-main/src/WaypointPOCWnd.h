#pragma once

#include "MQ2Main.h"
#include "MQ2Internal.h"
#include "EQClasses.h"
#include <cstddef>

extern void LogDebug(const char* format, ...);

enum class GenericToolMode : int
{
	Waypoints = 0,
	GMDashboard = 1,
};

class CWaypointPOCWnd : public CCustomWnd
{
public:
	CWaypointPOCWnd();
	~CWaypointPOCWnd();

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unknown);
	void RebuildActiveTool();
	void RequestWaypointListFromServer();
	void TravelToSelectedWaypoint();
	void RunSelectedDashboardCommand(bool dry_run);
	void SwitchTool(GenericToolMode mode, bool ensure_visible);
	void Toggle();
	void UpdateToolChrome();
	void UpdateSelectionDetails();

	CButtonWnd* pModeWaypointsBtn;
	CButtonWnd* pModeDashboardBtn;
	CListWnd* pToolList;
	CButtonWnd* pPrimaryBtn;
	CButtonWnd* pSecondaryBtn;
	CButtonWnd* pCloseBtn;
	CXWnd* pStatusLabel;
	CXWnd* pInfoLabel;

	bool bShow;
	bool bUIValid;
	DWORD dwLastRefreshRequest;
};

extern CWaypointPOCWnd* g_pWaypointPOCWnd;

void WaypointPOCWnd_Create();
void WaypointPOCWnd_Destroy();
void WaypointPOCWnd_Pulse();
void WaypointPOCWnd_CleanUI();
void WaypointPOCWnd_ReloadUI();
void WaypointPOCWnd_SetGameState(DWORD gs);
void WaypointPOCWnd_OnWaypointListPacket(const char* buf, size_t size);
void WaypointPOC_BeginPacketTrace(const char* source);
void WaypointPOC_LogIncomingOpcode(uint16_t opcode, size_t size);
bool WaypointPOC_IsPacketTraceActive();

void WaypointPOCCmd(PSPAWNINFO pChar, PCHAR szLine);
void GenericToolWndCmd(PSPAWNINFO pChar, PCHAR szLine);
void GMDashboardCmd(PSPAWNINFO pChar, PCHAR szLine);
