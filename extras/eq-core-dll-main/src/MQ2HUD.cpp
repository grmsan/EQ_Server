/* MQ2HUD.cpp - Minimal custom HUD overlay: player & target bars with debuffs
 * - Draws ASCII bars using `DrawHUDText` for player HP/Mana/Endurance and target
 * - Adds a command `/customhud` to toggle the overlay.
 */

#include "MQ2Main.h"
#include "EQClasses.h"
#include <string>
#include <algorithm>

static std::string MakeBar(float percent, int width)
{
    if (percent < 0.0f) percent = 0.0f;
    if (percent > 1.0f) percent = 1.0f;
    int filled = int(percent * width + 0.5f);
    std::string s;
    s.reserve(width);
    for (int i = 0; i < width; ++i)
        s += (i < filled ? '\xDB' : ' '); // use a block char if available
    return s;
}

// Draw custom HUD overlay; called from PluginsDrawHUD()
EQLIB_API VOID DrawCustomHUD()
{
    if (!gCustomHUD) return;
    if (gGameState != GAMESTATE_INGAME) return;

    PSPAWNINFO pLocal = (PSPAWNINFO)pCharSpawn;
    if (!pLocal) return;

    // Screen coords
    int sX = ((PCXWNDMGR)pWndMgr)->ScreenExtentX;
    int sY = ((PCXWNDMGR)pWndMgr)->ScreenExtentY;
    int x = 10;
    int y = sY - 80; // bottom left

    // Player HP
    float hpPct = 0.0f;
    if (pLocal->HPMax) hpPct = (float)pLocal->HPCurrent / (float)pLocal->HPMax;
    std::string hpBar = MakeBar(hpPct, 24);
    CHAR temp[MAX_STRING] = {0};
    sprintf(temp, "HP: %s %d%%", hpBar.c_str(), (int)(hpPct * 100.0f));
    DrawHUDText(temp, x, y, 0xFF00FF00, 2);
    y += 16;

    // Player Mana
    float manaPct = 0.0f;
    if (pLocal->ManaMax) manaPct = (float)pLocal->ManaCurrent / (float)pLocal->ManaMax;
    std::string manaBar = MakeBar(manaPct, 24);
    sprintf(temp, "Mana: %s %d%%", manaBar.c_str(), (int)(manaPct * 100.0f));
    DrawHUDText(temp, x, y, 0xFF0000FF, 2);
    y += 16;

    // Player Endurance
    float endPct = 0.0f;
    if (pLocal->EnduranceMax) endPct = (float)pLocal->EnduranceCurrent / (float)pLocal->EnduranceMax;
    std::string endBar = MakeBar(endPct, 24);
    sprintf(temp, "End: %s %d%%", endBar.c_str(), (int)(endPct * 100.0f));
    DrawHUDText(temp, x, y, 0xFFFFFF00, 2);
    y += 20;

    // Target (draw near top-left)
    if (pTarget)
    {
        int tx = 10;
        int ty = 40;
        PSPAWNINFO pT = (PSPAWNINFO)pTarget;
        float thpPct = 0.0f;
        if (pT->HPMax) thpPct = (float)pT->HPCurrent / (float)pT->HPMax;
        std::string thpBar = MakeBar(thpPct, 30);
        sprintf(temp, "Target %s: %s %d%%", pT->Name, thpBar.c_str(), (int)(thpPct * 100.0f));
        DrawHUDText(temp, tx, ty, 0xFFC00000, 2);
        ty += 16;

        // Debuffs (read from target window if available)
        if (ppTargetWnd && *ppTargetWnd)
        {
            PCTARGETWND pTW = (PCTARGETWND)*ppTargetWnd;
            // Draw first N debuffs texts
            int maxDebuffs = 10;
            int drawn = 0;
            for (int i = 0; i < NUM_BUFF_SLOTS && drawn < maxDebuffs; ++i)
            {
                int buffID = pTW->BuffSpellID[i];
                if (buffID && buffID != 0xFFFFFFFF)
                {
                    // Lookup spell name
                    CHAR SpellName[MAX_STRING] = {0};
                    if (PSPELL pSpell = GetSpellByID(buffID))
                    {
                        strcpy(SpellName, pSpell->Name); // safe copy
                        DrawHUDText(SpellName, tx, ty, 0xFFFFC000, 1);
                        ty += 12;
                        ++drawn;
                    }
                }
            }
        }
    }
}

// Toggle command
EQLIB_API VOID CustomHUDCmd(PSPAWNINFO pChar, PCHAR szLine)
{
    gCustomHUD = !gCustomHUD;
    if (gCustomHUD)
        WriteChatf("Custom HUD: ON");
    else
        WriteChatf("Custom HUD: OFF");
}
