/*****************************************************************************
 * PowerSlotWnd.cpp - POC custom SIDL window for Power Slot progression
 *
 * Demonstrates:
 *   - Creating a CCustomWnd subclass backed by SIDL XML
 *   - Receiving server data from EdgeStatLabel (opcode 0x1338)
 *   - Handling button click events via WndNotification
 *   - Updating list, label, and gauge widgets from live data
 *   - Proper lifecycle: create on GAMESTATE_INGAME, destroy on CleanUI
 *
 * The XML template (EQUI_PowerSlotWnd.xml) must be placed in:
 *   <EQ_Client_Dir>/uifiles/default/EQUI_PowerSlotWnd.xml
 * And its name added to uifiles/default/default.xml so the SIDL manager
 * loads it automatically.
 *****************************************************************************/

#include "PowerSlotWnd.h"
#include "EQClasses.h"
#include "eqgame.h"
#include <cstdio>

// External logging function from eqgame.cpp (also declared in PowerSlotWnd.h)
// extern void LogDebug(const char* format, ...);  // already in header

// WndNotification message IDs (from MQ2Main.h)
#define XWM_LCLICK    1
#define XWM_RCLICK    3
#define XWM_CLOSE     10
#define XWM_NEWVALUE  14

// ---- Global state ----
CPowerSlotWnd* g_pPowerSlotWnd = nullptr;
static bool g_bXMLAdded = false;

// ---- Constructor ----
CPowerSlotWnd::CPowerSlotWnd()
    : CCustomWnd("PowerSlotWnd")  // SIDL template name must match XML <ScreenID>
    , pTitleLabel(nullptr)
    , pXPGauge(nullptr)
    , pStatusLabel(nullptr)
    , pSlotList(nullptr)
    , pRefreshBtn(nullptr)
    , pSalvageBtn(nullptr)
    , pCloseBtn(nullptr)
    , pInfoLabel(nullptr)
    , bShow(true)
    , bUIValid(true)
    , dwLastUpdate(0)
{
    // Resolve child widget pointers from SIDL XML element names.
    // GetChildItem returns CXWnd*; we cast to the specific type.
    // If any fail, we set bUIValid=false but continue (graceful degradation).
    pTitleLabel   = GetChildItem("PS_TitleLabel");
    pXPGauge      = (CGaugeWnd*)GetChildItem("PS_XPGauge");
    pStatusLabel  = GetChildItem("PS_StatusLabel");
    pSlotList     = (CListWnd*)GetChildItem("PS_SlotList");
    pRefreshBtn   = (CButtonWnd*)GetChildItem("PS_RefreshBtn");
    pSalvageBtn   = (CButtonWnd*)GetChildItem("PS_SalvageBtn");
    pCloseBtn     = (CButtonWnd*)GetChildItem("PS_CloseBtn");
    pInfoLabel    = GetChildItem("PS_InfoLabel");

    if (!pSlotList || !pRefreshBtn || !pCloseBtn) {
        bUIValid = false;
        LogDebug("PowerSlotWnd: WARNING - some child items not found in SIDL template");
    }

    // Route WndNotification to our override (patches vtable slot 34)
    SetWndNotification(CPowerSlotWnd);

    CloseOnESC = 0;

    // Set up list columns if valid
    if (pSlotList) {
        // The columns are defined in XML, but we can adjust here if needed.
        // CListWnd columns should already be set from SIDL.
    }

    // Initial data populate
    UpdateFromEdgeStats();

    LogDebug("PowerSlotWnd: created successfully (bUIValid=%d)", bUIValid);
}

// ---- Destructor ----
CPowerSlotWnd::~CPowerSlotWnd()
{
    LogDebug("PowerSlotWnd: destroyed");
}

// ---- WndNotification: handle button clicks and other UI events ----
int CPowerSlotWnd::WndNotification(CXWnd *pWnd, unsigned int Message, void *unknown)
{
    if (Message == XWM_LCLICK) {
        if (pWnd == (CXWnd*)pRefreshBtn) {
            // Refresh button clicked — re-read from EdgeStatLabel cache
            LogDebug("PowerSlotWnd: Refresh button clicked");
            UpdateFromEdgeStats();

            // Also send a chat command to request fresh data from server
            if (pLocalPlayer) {
                DoCommand((PSPAWNINFO)pLocalPlayer, "/say #powerslots");
            }
            return 0;
        }
        else if (pWnd == (CXWnd*)pSalvageBtn) {
            // Salvage button clicked — placeholder
            LogDebug("PowerSlotWnd: Salvage button clicked (placeholder)");
            WriteChatColor("Salvage feature coming soon!", 0x0E);
            return 0;
        }
        else if (pWnd == (CXWnd*)pCloseBtn) {
            // Close button clicked — hide window
            LogDebug("PowerSlotWnd: Close button clicked");
            bShow = false;
            ((CXWnd*)this)->Show(0, 0);
            return 0;
        }
    }
    else if (Message == XWM_CLOSE) {
        bShow = false;
        ((CXWnd*)this)->Show(0, 0);
        return 0;
    }

    // Default: pass to base class
    return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
}

// ---- Update the window content from EdgeStatLabel cached data ----
void CPowerSlotWnd::UpdateFromEdgeStats()
{
    char szTemp[256];

    // Read overall stats from EdgeStatLabel keys
    const int slotCount = GetEdgeStatI(PSKeys::kSlotCount);
    const int focusTier = GetEdgeStatI(PSKeys::kFocusTier);
    const int focusXP   = GetEdgeStatI(PSKeys::kFocusXP);
    const int focusMax  = GetEdgeStatI(PSKeys::kFocusXPMax);

    // Update status label
    if (pStatusLabel) {
        if (HasEdgeStat(PSKeys::kFocusTier)) {
            sprintf_s(szTemp, "Tier: %d | XP: %d / %d | Slots: %d",
                focusTier, focusXP, focusMax, slotCount);
        } else {
            sprintf_s(szTemp, "Waiting for server data...");
        }
        pStatusLabel->SetWindowTextA(CXStr(szTemp));
    }

    // Update XP gauge text (we set the text overlay; actual fill requires offset work)
    if (pXPGauge) {
        if (focusMax > 0) {
            int pct = (focusXP * 100) / focusMax;
            if (pct > 100) pct = 100;
            sprintf_s(szTemp, "%d%% (%d / %d)", pct, focusXP, focusMax);
        } else {
            sprintf_s(szTemp, "-- / --");
        }
        // Set text on the gauge (displays as overlay text inside the filled area)
        ((CXWnd*)pXPGauge)->SetWindowTextA(CXStr(szTemp));
    }

    // Update the slot list
    if (pSlotList) {
        // Clear existing rows
        pSlotList->DeleteAll();

        const int displayCount = (slotCount > 0 && slotCount <= PSKeys::kMaxSlots) ? slotCount : PSKeys::kMaxSlots;

        for (int i = 0; i < displayCount; ++i) {
            const int tier   = GetEdgeStatI(PSKeys::kSlotTierBase + i);
            const int xp     = GetEdgeStatI(PSKeys::kSlotXPBase + i);
            const int xpMax  = GetEdgeStatI(PSKeys::kSlotXPMaxBase + i);
            const int itemId = GetEdgeStatI(PSKeys::kSlotItemBase + i);

            // Add a row
            pSlotList->AddString(CXStr(" "), 0xFFFFFFFF, 0, NULL);

            // Column 0: Slot name
            const char* slotName = (i < 10) ? kSlotNames[i] : "??";
            pSlotList->SetItemText(i, 0, &CXStr(slotName));

            // Column 1: Item (show ID for now; real version would look up item name)
            if (itemId > 0) {
                sprintf_s(szTemp, "Item #%d", itemId);
            } else {
                sprintf_s(szTemp, "(empty)");
            }
            pSlotList->SetItemText(i, 1, &CXStr(szTemp));

            // Column 2: Tier
            sprintf_s(szTemp, "T%d", tier);
            pSlotList->SetItemText(i, 2, &CXStr(szTemp));

            // Column 3: XP progress
            if (xpMax > 0) {
                int pct = (xp * 100) / xpMax;
                sprintf_s(szTemp, "%d%%", pct);
            } else {
                sprintf_s(szTemp, "--");
            }
            pSlotList->SetItemText(i, 3, &CXStr(szTemp));

            // Color: green for items with progress, gray for empty
            DWORD color = (itemId > 0) ? 0xFF00FF00 : 0xFF808080;
            for (int col = 0; col < 4; col++) {
                pSlotList->SetItemColor(i, col, color);
            }
        }
    }

    // Update info label with timestamp
    if (pInfoLabel) {
        DWORD tick = GetTickCount();
        sprintf_s(szTemp, "Last update: %u ms ago | POC SIDL + EdgeStatLabel",
            (dwLastUpdate > 0) ? (tick - dwLastUpdate) : 0);
        pInfoLabel->SetWindowTextA(CXStr(szTemp));
        dwLastUpdate = tick;
    }
}

// ---- Toggle visibility ----
void CPowerSlotWnd::Toggle()
{
    bShow = !bShow;
    ((CXWnd*)this)->Show(bShow ? 1 : 0, bShow ? 1 : 0);
    if (bShow) {
        UpdateFromEdgeStats();
    }
}

// ===========================================================================
// Lifecycle functions
// ===========================================================================

void PowerSlotWnd_Create()
{
    if (g_pPowerSlotWnd) {
        PowerSlotWnd_Destroy();
    }

    // Check if the SIDL template is available
    if (pSidlMgr && ((CSidlManager*)pSidlMgr)->FindScreenPieceTemplate("PowerSlotWnd")) {
        g_pPowerSlotWnd = new CPowerSlotWnd();
        if (g_pPowerSlotWnd && g_pPowerSlotWnd->bShow) {
            ((CXWnd*)g_pPowerSlotWnd)->Show(1, 1);
        }
        LogDebug("PowerSlotWnd_Create: window created");
    } else {
        LogDebug("PowerSlotWnd_Create: SIDL template 'PowerSlotWnd' not found. "
                 "Ensure EQUI_PowerSlotWnd.xml is in uifiles/default/ and listed in default.xml");
    }
}

void PowerSlotWnd_Destroy()
{
    if (g_pPowerSlotWnd) {
        LogDebug("PowerSlotWnd_Destroy: destroying window");
        delete g_pPowerSlotWnd;
        g_pPowerSlotWnd = nullptr;
    }
}

void PowerSlotWnd_Pulse()
{
    if (!g_pPowerSlotWnd || gGameState != GAMESTATE_INGAME) {
        return;
    }

    // Auto-refresh every 2 seconds if visible
    if (g_pPowerSlotWnd->bShow) {
        DWORD now = GetTickCount();
        if (now - g_pPowerSlotWnd->dwLastUpdate > 2000) {
            g_pPowerSlotWnd->UpdateFromEdgeStats();
        }
    }
}

void PowerSlotWnd_CleanUI()
{
    PowerSlotWnd_Destroy();
}

void PowerSlotWnd_ReloadUI()
{
    if (gGameState == GAMESTATE_INGAME && pCharSpawn) {
        PowerSlotWnd_Create();
    }
}

void PowerSlotWnd_SetGameState(DWORD gs)
{
    if (gs == GAMESTATE_INGAME) {
        PowerSlotWnd_Create();
    } else {
        PowerSlotWnd_Destroy();
    }
}

// ---- Slash command: /powerslots ----
void PowerSlotCmd(PSPAWNINFO pChar, PCHAR szLine)
{
    if (!szLine || !szLine[0] || !_stricmp(szLine, "show")) {
        // Toggle window
        if (g_pPowerSlotWnd) {
            g_pPowerSlotWnd->Toggle();
        } else {
            PowerSlotWnd_Create();
        }
        return;
    }

    if (!_stricmp(szLine, "refresh")) {
        if (g_pPowerSlotWnd) {
            g_pPowerSlotWnd->UpdateFromEdgeStats();
            WriteChatColor("Power Slots refreshed.", 0x0D);
        }
        return;
    }

    if (!_stricmp(szLine, "debug")) {
        char msg[256];
        sprintf_s(msg, "PowerSlotWnd: ptr=%p valid=%d show=%d edgeTier=%d",
            g_pPowerSlotWnd,
            g_pPowerSlotWnd ? g_pPowerSlotWnd->bUIValid : -1,
            g_pPowerSlotWnd ? g_pPowerSlotWnd->bShow : -1,
            GetEdgeStatI(PSKeys::kFocusTier));
        WriteChatColor(msg, 0x0D);
        return;
    }

    // Usage help
    WriteChatColor("Usage: /powerslots [show|refresh|debug]", 0x0E);
}
