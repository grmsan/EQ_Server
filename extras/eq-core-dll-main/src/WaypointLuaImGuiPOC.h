#pragma once

#include "MQ2Main.h"
#include <cstddef>

// Lua/ImGui waypoint POC scaffold.
// Current build can run in stub mode until x86 Lua+ImGui dependencies are added.
void WaypointLuaImGuiPOC_OnWaypointListPacket(const char* buf, size_t size);
void WaypointLuaImGuiPOC_Draw();
void WaypointLuaImGuiPOC_CleanUI();
void WaypointLuaImGuiPOCCmd(PSPAWNINFO pChar, PCHAR szLine);
