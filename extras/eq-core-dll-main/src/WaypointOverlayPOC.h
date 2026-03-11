#pragma once

#include "MQ2Main.h"
#include <cstddef>

// HUD/overlay POC (no SIDL, no ImGui): DrawHUDText + command-driven interaction.
void WaypointOverlayPOC_OnWaypointListPacket(const char* buf, size_t size);
void WaypointOverlayPOC_Draw();
void WaypointOverlayPOC_CleanUI();
void WaypointOverlayPOCCmd(PSPAWNINFO pChar, PCHAR szLine);
