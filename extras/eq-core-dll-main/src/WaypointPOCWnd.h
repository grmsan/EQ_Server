#pragma once

#include "MQ2Main.h"
#include "MQ2Internal.h"
#include "EQClasses.h"
#include <cstddef>

// Logging helper from eqgame.cpp
extern void LogDebug(const char* format, ...);

class CWaypointPOCWnd : public CCustomWnd
{
public:
	CWaypointPOCWnd();
	~CWaypointPOCWnd();

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unknown);
	void RebuildWaypointList();
	void RequestWaypointListFromServer();
	void TravelToSelectedWaypoint();
	void Toggle();

	CListWnd* pWaypointList;
	CButtonWnd* pRefreshBtn;
	CButtonWnd* pTravelBtn;
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

void WaypointPOCCmd(PSPAWNINFO pChar, PCHAR szLine);
