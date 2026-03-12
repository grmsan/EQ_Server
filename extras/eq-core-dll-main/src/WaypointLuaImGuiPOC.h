#pragma once

#include "MQ2Main.h"
#include <cstddef>

// Lua/ImGui waypoint POC runtime.
// This path embeds Lua and renders with Dear ImGui (DX9 + Win32 backend).
void WaypointLuaImGuiPOC_Initialize();
void WaypointLuaImGuiPOC_Shutdown();
void WaypointLuaImGuiPOC_OnWaypointListPacket(const char* buf, size_t size);
void WaypointLuaImGuiPOC_Draw();
void WaypointLuaImGuiPOC_CleanUI();
void WaypointLuaImGuiPOCCmd(PSPAWNINFO pChar, PCHAR szLine);
