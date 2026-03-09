#pragma once
/*****************************************************************************
 * PowerSlotWnd.h - POC custom SIDL window for Power Slot progression display
 *
 * Demonstrates:
 *   1. Creating a CCustomWnd from SIDL XML (uifiles/default/)
 *   2. Receiving server data via EdgeStatLabel (opcode 0x1338)
 *   3. Interactive buttons with WndNotification handling
 *   4. Gauge, list, label, and button UI elements
 *   5. Lifecycle hooks: create on GAMESTATE_INGAME, destroy on CleanUI
 *
 * Server-side keys (EdgeStatLabel, defined in zone/client.cpp):
 *   300 = equipped slot count
 *   301 = current focus tier
 *   302 = current focus XP (current)
 *   303 = current focus XP (needed for next tier)
 *   304-313 = per-slot tier (slot 0 through 9)
 *   314-323 = per-slot XP  (slot 0 through 9)
 *   324-333 = per-slot XP needed (slot 0 through 9)
 *   334-343 = per-slot item_id (slot 0 through 9)
 *****************************************************************************/

#include "MQ2Main.h"
#include "MQ2Internal.h"
#include "EQClasses.h"

// External logging function from eqgame.cpp
extern void LogDebug(const char* format, ...);

// ---- EdgeStatLabel key constants for power slot data ----
namespace PSKeys {
    constexpr uint32_t kSlotCount    = 300;  // number of equipped power slots (0-10)
    constexpr uint32_t kFocusTier    = 301;  // current focus/overall tier
    constexpr uint32_t kFocusXP      = 302;  // current focus XP
    constexpr uint32_t kFocusXPMax   = 303;  // XP needed for next tier
    // Per-slot arrays (10 slots max, index 0-9):
    constexpr uint32_t kSlotTierBase   = 304;  // +slot = tier of slot
    constexpr uint32_t kSlotXPBase     = 314;  // +slot = current XP
    constexpr uint32_t kSlotXPMaxBase  = 324;  // +slot = XP needed
    constexpr uint32_t kSlotItemBase   = 334;  // +slot = item ID
    // Max slots we track
    constexpr int kMaxSlots = 10;
}

// ---- Forward declare the g_edgeStat accessors (defined in eqgame.cpp) ----
// These are internal to the DLL; we extern them here for the POC.
extern uint64_t g_edgeStatValue[];
extern uint8_t  g_edgeStatHas[];
constexpr uint32_t kEdgeStatMaxKey_PS = 4096;

inline bool HasEdgeStat(uint32_t key) {
    return (key < kEdgeStatMaxKey_PS) && g_edgeStatHas[key];
}
inline uint64_t GetEdgeStat(uint32_t key) {
    return (key < kEdgeStatMaxKey_PS && g_edgeStatHas[key]) ? g_edgeStatValue[key] : 0;
}
inline int GetEdgeStatI(uint32_t key) {
    if (!HasEdgeStat(key)) return 0;
    uint64_t v = g_edgeStatValue[key];
    return (v > INT_MAX) ? INT_MAX : static_cast<int>(v);
}

// ---- Slot names for display ----
// Slot 0 = Power Source (the only slot with XP tracking currently)
// Slots 1-9 = equipped item slots (show tier info but no XP yet)
static const char* kSlotNames[] = {
    "Power Src",  // 0 - Power Source
    "Head",       // 1
    "Chest",      // 2
    "Arms",       // 3
    "Wrist",      // 4
    "Hands",      // 5
    "Legs",       // 6
    "Feet",       // 7
    "Primary",    // 8
    "Secondary",  // 9
};

// ---- POC Window Class ----
class CPowerSlotWnd : public CCustomWnd
{
public:
    CPowerSlotWnd();
    ~CPowerSlotWnd();

    // Override WndNotification to handle button clicks and list events
    int WndNotification(CXWnd *pWnd, unsigned int Message, void *unknown);

    // Refresh the list/gauge from EdgeStatLabel cache
    void UpdateFromEdgeStats();

    // Show/hide toggle
    void Toggle();

    // Child widget pointers (populated from SIDL XML GetChildItem)
    // Labels use CXWnd* since CLabelWnd is not defined in this DLL's headers.
    CXWnd*          pTitleLabel;
    CGaugeWnd*      pXPGauge;
    CXWnd*          pStatusLabel;
    CListWnd*       pSlotList;
    CButtonWnd*     pRefreshBtn;
    CButtonWnd*     pSalvageBtn;
    CButtonWnd*     pCloseBtn;
    CXWnd*          pInfoLabel;

    bool            bShow;          // visibility flag
    bool            bUIValid;       // true if all child items resolved
    DWORD           dwLastUpdate;   // tick of last update
};

// ---- Global instance ----
extern CPowerSlotWnd* g_pPowerSlotWnd;

// ---- Lifecycle functions (called from hooks in eqgame.cpp / MQ2Pulse.cpp) ----
void PowerSlotWnd_Create();
void PowerSlotWnd_Destroy();
void PowerSlotWnd_Pulse();      // called each frame — auto-refresh
void PowerSlotWnd_CleanUI();
void PowerSlotWnd_ReloadUI();
void PowerSlotWnd_SetGameState(DWORD gs);

// ---- Slash command ----
void PowerSlotCmd(PSPAWNINFO pChar, PCHAR szLine);
