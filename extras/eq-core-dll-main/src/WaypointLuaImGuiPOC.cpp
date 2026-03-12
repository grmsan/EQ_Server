#include "WaypointLuaImGuiPOC.h"
#include "MQ2Main.h"
#include "hook_vtable.hpp"

#include <Windows.h>
#include <d3d9.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <luajit/lua.hpp>

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <cstdio>

extern void LogDebug(const char* format, ...);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {
struct WaypointEntry {
	int32_t category_id = 0;
	int32_t waypoint_id = 0;
	std::string name;
};

std::vector<WaypointEntry> g_entries;
bool g_enabled = false;
bool g_have_packet = false;
int g_selected_index = 0;

lua_State* g_lua_state = nullptr;
int g_lua_draw_ref = LUA_NOREF;
bool g_script_loaded = false;

bool g_create_device_hook_installed = false;
bool g_present_hook_installed = false;
bool g_imgui_initialized = false;
bool g_runtime_initialized = false;
bool g_wndproc_installed = false;
DWORD g_last_lua_error_tick = 0;
char g_last_lua_error[256] = { 0 };

HWND g_eq_hwnd = nullptr;
WNDPROC g_prev_wndproc = nullptr;

using CreateDeviceFunc = HRESULT __stdcall(
	IDirect3D9&,
	UINT,
	D3DDEVTYPE,
	HWND,
	DWORD,
	D3DPRESENT_PARAMETERS*,
	IDirect3DDevice9**
);

using PresentFunc = HRESULT __stdcall(
	IDirect3DDevice9&,
	const RECT*,
	const RECT*,
	HWND,
	const RGNDATA*
);

using ResetFunc = HRESULT __stdcall(
	IDirect3DDevice9&,
	D3DPRESENT_PARAMETERS*
);

CreateDeviceFunc* g_real_create_device = nullptr;
PresentFunc* g_real_present = nullptr;
ResetFunc* g_real_reset = nullptr;

int MinInt(int a, int b)
{
	return (a < b) ? a : b;
}

int MaxInt(int a, int b)
{
	return (a > b) ? a : b;
}

int ClampIndex(int value, int min_value, int max_value)
{
	if (value < min_value) {
		return min_value;
	}
	if (value > max_value) {
		return max_value;
	}
	return value;
}

void SetLuaError(const char* err)
{
	if (!err) {
		err = "unknown error";
	}
	sprintf_s(g_last_lua_error, "%s", err);
	g_last_lua_error_tick = GetTickCount();
}

bool SelectLayout(const char* buf, size_t size, size_t& entries_off, size_t& stride, uint32_t& count)
{
	struct Candidate { size_t count_off; size_t entry_stride; };
	const Candidate candidates[] = { {8,76}, {8,73}, {5,76}, {5,73} };

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

		entries_off = local_entries_off;
		stride = c.entry_stride;
		count = local_count;
		return true;
	}

	return false;
}

HWND ResolveEQWindowHandle()
{
	if (EQADDR_HWND) {
		auto hwnd_ptr = reinterpret_cast<HWND*>(EQADDR_HWND);
		if (hwnd_ptr && *hwnd_ptr) {
			return *hwnd_ptr;
		}
	}

	HWND fg = GetForegroundWindow();
	return fg;
}

void RequestList()
{
	if (pLocalPlayer) {
		DoCommand((PSPAWNINFO)pLocalPlayer, "/say #wppoc list");
	}
}

void TravelToWaypoint(int waypoint_id)
{
	if (!pLocalPlayer || waypoint_id <= 0) {
		return;
	}

	char cmd[128] = { 0 };
	sprintf_s(cmd, "/say #wppoc travel %d", waypoint_id);
	DoCommand((PSPAWNINFO)pLocalPlayer, cmd);
}

void TravelSelected()
{
	if (!pLocalPlayer || g_entries.empty()) {
		WriteChatColor("Lua/ImGui POC: no waypoint selected.", 0x0E);
		return;
	}

	g_selected_index = ClampIndex(g_selected_index, 0, static_cast<int>(g_entries.size()) - 1);
	const auto& selected = g_entries[g_selected_index];

	TravelToWaypoint(selected.waypoint_id);
}

void ClearLuaRuntime()
{
	if (g_lua_state) {
		if (g_lua_draw_ref != LUA_NOREF) {
			luaL_unref(g_lua_state, LUA_REGISTRYINDEX, g_lua_draw_ref);
			g_lua_draw_ref = LUA_NOREF;
		}
		lua_close(g_lua_state);
		g_lua_state = nullptr;
	}
	g_script_loaded = false;
}

void ClearImGuiRuntime()
{
	if (g_imgui_initialized) {
		ImGui_ImplDX9_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		g_imgui_initialized = false;
	}

	if (g_wndproc_installed && g_eq_hwnd && g_prev_wndproc) {
		SetWindowLongPtr(g_eq_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_prev_wndproc));
		g_wndproc_installed = false;
		g_prev_wndproc = nullptr;
	}
	g_eq_hwnd = nullptr;
}

int Lua_WaypointGetEntries(lua_State* L)
{
	lua_newtable(L);

	int out_index = 1;
	for (const auto& entry : g_entries) {
		lua_newtable(L);

		lua_pushinteger(L, entry.waypoint_id);
		lua_setfield(L, -2, "waypoint_id");

		lua_pushinteger(L, entry.category_id);
		lua_setfield(L, -2, "category_id");

		lua_pushstring(L, entry.name.c_str());
		lua_setfield(L, -2, "name");

		lua_rawseti(L, -2, out_index++);
	}

	return 1;
}

int Lua_WaypointRefresh(lua_State* L)
{
	(void)L;
	RequestList();
	return 0;
}

int Lua_WaypointTravel(lua_State* L)
{
	const int waypoint_id = static_cast<int>(luaL_checkinteger(L, 1));
	TravelToWaypoint(waypoint_id);
	return 0;
}

int Lua_WaypointGetSelectedIndex(lua_State* L)
{
	lua_pushinteger(L, g_selected_index + 1); // Lua-facing index is 1-based.
	return 1;
}

int Lua_WaypointSetSelectedIndex(lua_State* L)
{
	const int requested = static_cast<int>(luaL_checkinteger(L, 1)) - 1;
	if (g_entries.empty()) {
		g_selected_index = 0;
		return 0;
	}

	g_selected_index = ClampIndex(requested, 0, static_cast<int>(g_entries.size()) - 1);
	return 0;
}

int Lua_WaypointHasPacket(lua_State* L)
{
	lua_pushboolean(L, g_have_packet ? 1 : 0);
	return 1;
}

int Lua_ImGuiSetNextWindowSize(lua_State* L)
{
	const float width = static_cast<float>(luaL_optnumber(L, 1, 520.0));
	const float height = static_cast<float>(luaL_optnumber(L, 2, 420.0));
	const int cond = static_cast<int>(luaL_optinteger(L, 3, static_cast<lua_Integer>(ImGuiCond_FirstUseEver)));
	ImGui::SetNextWindowSize(ImVec2(width, height), cond);
	return 0;
}

int Lua_ImGuiBegin(lua_State* L)
{
	const char* title = luaL_checkstring(L, 1);
	const bool visible = ImGui::Begin(title);
	lua_pushboolean(L, visible ? 1 : 0);
	return 1;
}

int Lua_ImGuiEndWindow(lua_State* L)
{
	(void)L;
	ImGui::End();
	return 0;
}

int Lua_ImGuiText(lua_State* L)
{
	const char* text = luaL_checkstring(L, 1);
	ImGui::TextUnformatted(text);
	return 0;
}

int Lua_ImGuiButton(lua_State* L)
{
	const char* label = luaL_checkstring(L, 1);
	const bool pressed = ImGui::Button(label);
	lua_pushboolean(L, pressed ? 1 : 0);
	return 1;
}

int Lua_ImGuiSameLine(lua_State* L)
{
	const float offset_from_start_x = static_cast<float>(luaL_optnumber(L, 1, 0.0));
	const float spacing = static_cast<float>(luaL_optnumber(L, 2, -1.0));
	ImGui::SameLine(offset_from_start_x, spacing);
	return 0;
}

int Lua_ImGuiSeparator(lua_State* L)
{
	(void)L;
	ImGui::Separator();
	return 0;
}

int Lua_ImGuiSelectable(lua_State* L)
{
	const char* label = luaL_checkstring(L, 1);
	const bool selected = lua_toboolean(L, 2) != 0;
	const bool clicked = ImGui::Selectable(label, selected);
	lua_pushboolean(L, clicked ? 1 : 0);
	return 1;
}

void RegisterLuaApi(lua_State* L)
{
	lua_newtable(L); // waypoint table
	lua_pushcfunction(L, Lua_WaypointGetEntries);
	lua_setfield(L, -2, "get_entries");
	lua_pushcfunction(L, Lua_WaypointRefresh);
	lua_setfield(L, -2, "refresh");
	lua_pushcfunction(L, Lua_WaypointTravel);
	lua_setfield(L, -2, "travel");
	lua_pushcfunction(L, Lua_WaypointGetSelectedIndex);
	lua_setfield(L, -2, "get_selected_index");
	lua_pushcfunction(L, Lua_WaypointSetSelectedIndex);
	lua_setfield(L, -2, "set_selected_index");
	lua_pushcfunction(L, Lua_WaypointHasPacket);
	lua_setfield(L, -2, "has_packet");
	lua_setglobal(L, "waypoint");

	lua_newtable(L); // imgui table
	lua_pushcfunction(L, Lua_ImGuiSetNextWindowSize);
	lua_setfield(L, -2, "set_next_window_size");
	lua_pushcfunction(L, Lua_ImGuiBegin);
	lua_setfield(L, -2, "begin");
	lua_pushcfunction(L, Lua_ImGuiEndWindow);
	lua_setfield(L, -2, "end_window");
	lua_pushcfunction(L, Lua_ImGuiText);
	lua_setfield(L, -2, "text");
	lua_pushcfunction(L, Lua_ImGuiButton);
	lua_setfield(L, -2, "button");
	lua_pushcfunction(L, Lua_ImGuiSameLine);
	lua_setfield(L, -2, "same_line");
	lua_pushcfunction(L, Lua_ImGuiSeparator);
	lua_setfield(L, -2, "separator");
	lua_pushcfunction(L, Lua_ImGuiSelectable);
	lua_setfield(L, -2, "selectable");
	lua_pushinteger(L, static_cast<lua_Integer>(ImGuiCond_FirstUseEver));
	lua_setfield(L, -2, "COND_FIRST_USE_EVER");
	lua_setglobal(L, "imgui");
}

bool EnsureLuaRuntime()
{
	if (g_lua_state) {
		return true;
	}

	g_lua_state = luaL_newstate();
	if (!g_lua_state) {
		SetLuaError("Failed to create Lua runtime.");
		return false;
	}

	luaL_openlibs(g_lua_state);
	RegisterLuaApi(g_lua_state);
	return true;
}

bool LoadLuaScript()
{
	if (!EnsureLuaRuntime()) {
		return false;
	}

	if (g_lua_draw_ref != LUA_NOREF) {
		luaL_unref(g_lua_state, LUA_REGISTRYINDEX, g_lua_draw_ref);
		g_lua_draw_ref = LUA_NOREF;
	}

	char script_path[MAX_PATH] = { 0 };
	if (gszEQPath[0]) {
		sprintf_s(script_path, "%s\\scripts\\waypoint_imgui_poc.lua", gszEQPath);
	} else {
		sprintf_s(script_path, ".\\scripts\\waypoint_imgui_poc.lua");
	}

	if (luaL_loadfile(g_lua_state, script_path) != LUA_OK) {
		SetLuaError(lua_tostring(g_lua_state, -1));
		lua_pop(g_lua_state, 1);
		g_script_loaded = false;
		return false;
	}

	if (lua_pcall(g_lua_state, 0, 1, 0) != LUA_OK) {
		SetLuaError(lua_tostring(g_lua_state, -1));
		lua_pop(g_lua_state, 1);
		g_script_loaded = false;
		return false;
	}

	if (lua_istable(g_lua_state, -1)) {
		lua_getfield(g_lua_state, -1, "draw");
		if (!lua_isfunction(g_lua_state, -1)) {
			lua_pop(g_lua_state, 2);
			SetLuaError("Lua script must return table with draw() function.");
			g_script_loaded = false;
			return false;
		}
		lua_remove(g_lua_state, -2); // remove table, leave function
	}
	else if (!lua_isfunction(g_lua_state, -1)) {
		lua_pop(g_lua_state, 1);
		SetLuaError("Lua script must return function or table with draw() function.");
		g_script_loaded = false;
		return false;
	}

	g_lua_draw_ref = luaL_ref(g_lua_state, LUA_REGISTRYINDEX);
	g_script_loaded = true;
	LogDebug("WaypointLuaImGuiPOC: loaded script from %s", script_path);
	return true;
}

void DrawNativeFallbackWindow()
{
	ImGui::SetNextWindowSize(ImVec2(520.0f, 420.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Waypoint Lua/ImGui POC (Native Fallback)")) {
		ImGui::End();
		return;
	}

	if (ImGui::Button("Refresh")) {
		RequestList();
	}
	ImGui::SameLine();
	if (ImGui::Button("Travel")) {
		TravelSelected();
	}

	ImGui::Separator();
	if (!g_have_packet) {
		ImGui::TextUnformatted("No waypoint packet yet. Click Refresh.");
	}

	for (int i = 0; i < static_cast<int>(g_entries.size()); ++i) {
		const auto& e = g_entries[i];
		char line[256] = { 0 };
		sprintf_s(line, "[%d] %s", e.waypoint_id, e.name.c_str());
		if (ImGui::Selectable(line, (i == g_selected_index))) {
			g_selected_index = i;
		}
	}

	ImGui::End();
}

void CallLuaDraw()
{
	if (!EnsureLuaRuntime()) {
		DrawNativeFallbackWindow();
		return;
	}

	if (g_lua_draw_ref == LUA_NOREF && !LoadLuaScript()) {
		DrawNativeFallbackWindow();
		return;
	}

	lua_rawgeti(g_lua_state, LUA_REGISTRYINDEX, g_lua_draw_ref);
	if (!lua_isfunction(g_lua_state, -1)) {
		lua_pop(g_lua_state, 1);
		DrawNativeFallbackWindow();
		return;
	}

	if (lua_pcall(g_lua_state, 0, 0, 0) != LUA_OK) {
		const DWORD previous_error_tick = g_last_lua_error_tick;
		SetLuaError(lua_tostring(g_lua_state, -1));
		lua_pop(g_lua_state, 1);
		if (g_lua_draw_ref != LUA_NOREF) {
			luaL_unref(g_lua_state, LUA_REGISTRYINDEX, g_lua_draw_ref);
			g_lua_draw_ref = LUA_NOREF;
		}
		g_script_loaded = false;

		const DWORD now = GetTickCount();
		if (now - previous_error_tick > 1000) {
			WriteChatColor("Lua/ImGui waypoint POC: draw() error; falling back to native ImGui window.", 0x0E);
		}
		DrawNativeFallbackWindow();
	}
}

LRESULT CALLBACK WaypointImguiWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (g_enabled && g_imgui_initialized) {
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
			return 1;
		}
	}

	if (g_prev_wndproc) {
		return CallWindowProc(g_prev_wndproc, hWnd, msg, wParam, lParam);
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void EnsureWndProcHook()
{
	HWND hwnd = ResolveEQWindowHandle();
	if (!hwnd) {
		return;
	}

	if (g_eq_hwnd != hwnd && g_wndproc_installed && g_prev_wndproc && g_eq_hwnd) {
		SetWindowLongPtr(g_eq_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_prev_wndproc));
		g_wndproc_installed = false;
		g_prev_wndproc = nullptr;
	}

	g_eq_hwnd = hwnd;
	if (g_wndproc_installed) {
		return;
	}

	auto prev = reinterpret_cast<WNDPROC>(
		SetWindowLongPtr(g_eq_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WaypointImguiWndProc))
	);
	if (prev) {
		g_prev_wndproc = prev;
		g_wndproc_installed = true;
	}
}

bool EnsureImGuiInitialized(IDirect3DDevice9& device)
{
	if (g_imgui_initialized) {
		return true;
	}

	EnsureWndProcHook();
	if (!g_eq_hwnd) {
		return false;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	if (!ImGui_ImplWin32_Init(g_eq_hwnd)) {
		ImGui::DestroyContext();
		return false;
	}

	if (!ImGui_ImplDX9_Init(&device)) {
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		return false;
	}

	g_imgui_initialized = true;
	return true;
}

HRESULT __stdcall PresentHook(
	IDirect3DDevice9& device,
	const RECT* source_rect,
	const RECT* dest_rect,
	HWND dest_window_override,
	const RGNDATA* dirty_region
)
{
	(void)dest_window_override;

	if (g_enabled && gGameState == GAMESTATE_INGAME) {
		if (EnsureImGuiInitialized(device)) {
			ImGui_ImplDX9_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			CallLuaDraw();

			ImGui::EndFrame();
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		}
	}

	return g_real_present(device, source_rect, dest_rect, dest_window_override, dirty_region);
}

HRESULT __stdcall ResetHook(IDirect3DDevice9& device, D3DPRESENT_PARAMETERS* params)
{
	if (g_imgui_initialized) {
		ImGui_ImplDX9_InvalidateDeviceObjects();
	}

	HRESULT hr = g_real_reset(device, params);

	if (g_imgui_initialized && SUCCEEDED(hr)) {
		ImGui_ImplDX9_CreateDeviceObjects();
	}

	return hr;
}

HRESULT __stdcall CreateDeviceHook(
	IDirect3D9& self,
	UINT adapter,
	D3DDEVTYPE device_type,
	HWND focus_window,
	DWORD behavior_flags,
	D3DPRESENT_PARAMETERS* presentation_parameters,
	IDirect3DDevice9** returned_device_interface
)
{
	HRESULT result = g_real_create_device(
		self,
		adapter,
		device_type,
		focus_window,
		behavior_flags,
		presentation_parameters,
		returned_device_interface
	);

	if (SUCCEEDED(result) && returned_device_interface && *returned_device_interface) {
		if (!g_real_present) {
			g_real_present = hook_vtable<PresentFunc>(**returned_device_interface, 17, PresentHook);
			g_present_hook_installed = (g_real_present != nullptr);
		}
		if (!g_real_reset) {
			g_real_reset = hook_vtable<ResetFunc>(**returned_device_interface, 16, ResetHook);
		}
	}

	return result;
}

void EnsureGraphicsHooks()
{
	if (g_create_device_hook_installed) {
		return;
	}

	IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (!d3d) {
		return;
	}

	g_real_create_device = hook_vtable<CreateDeviceFunc>(*d3d, 16, CreateDeviceHook);
	g_create_device_hook_installed = (g_real_create_device != nullptr);
	d3d->Release();
}

void RenderStatusToChat()
{
	char line[256] = { 0 };
	sprintf_s(
		line,
		"Lua/ImGui status: enabled=%s hooks(create=%s present=%s) imgui=%s lua=%s script=%s entries=%d",
		g_enabled ? "yes" : "no",
		g_create_device_hook_installed ? "yes" : "no",
		g_present_hook_installed ? "yes" : "no",
		g_imgui_initialized ? "yes" : "no",
		g_lua_state ? "yes" : "no",
		g_script_loaded ? "yes" : "no",
		static_cast<int>(g_entries.size())
	);
	WriteChatColor(line, 0x0E);

	if (g_last_lua_error[0]) {
		char err_line[300] = { 0 };
		sprintf_s(err_line, "Lua/ImGui last error: %s", g_last_lua_error);
		WriteChatColor(err_line, 0x0D);
	}
}
}

void WaypointLuaImGuiPOC_Initialize()
{
	if (g_runtime_initialized) {
		return;
	}

	g_runtime_initialized = true;
	EnsureGraphicsHooks();
	EnsureLuaRuntime();
}

void WaypointLuaImGuiPOC_Shutdown()
{
	g_enabled = false;
	ClearImGuiRuntime();
	ClearLuaRuntime();
}

void WaypointLuaImGuiPOC_OnWaypointListPacket(const char* buf, size_t size)
{
	if (!buf || size < 5) {
		return;
	}

	size_t entries_off = 0;
	size_t stride = 0;
	uint32_t count = 0;
	if (!SelectLayout(buf, size, entries_off, stride, count)) {
		return;
	}

	std::vector<WaypointEntry> parsed;
	parsed.reserve(count);

	for (uint32_t i = 0; i < count; ++i) {
		const size_t off = entries_off + (static_cast<size_t>(i) * stride);
		if (off + 73 > size) {
			break;
		}

		const uint8_t enabled = *(reinterpret_cast<const uint8_t*>(buf + off + 8));
		if (!enabled) {
			continue;
		}

		WaypointEntry e;
		memcpy(&e.category_id, buf + off, sizeof(int32_t));
		memcpy(&e.waypoint_id, buf + off + 4, sizeof(int32_t));

		char name_buf[65] = { 0 };
		memcpy(name_buf, buf + off + 9, 64);
		name_buf[64] = '\0';
		e.name = name_buf;
		parsed.push_back(std::move(e));
	}

	g_entries = std::move(parsed);
	g_have_packet = true;
	if (g_selected_index >= static_cast<int>(g_entries.size())) {
		g_selected_index = MaxInt(0, static_cast<int>(g_entries.size()) - 1);
	}
}

void WaypointLuaImGuiPOC_Draw()
{
	if (!g_enabled || gGameState != GAMESTATE_INGAME) {
		return;
	}
}

void WaypointLuaImGuiPOC_CleanUI()
{
	g_selected_index = 0;
	g_have_packet = false;
	g_entries.clear();
}

void WaypointLuaImGuiPOCCmd(PSPAWNINFO pChar, PCHAR szLine)
{
	(void)pChar;

	if (!szLine || !szLine[0] || !_stricmp(szLine, "toggle") || !_stricmp(szLine, "show")) {
		EnsureGraphicsHooks();
		EnsureLuaRuntime();
		g_enabled = !g_enabled;
		if (g_enabled && !g_have_packet) {
			RequestList();
		}
		WriteChatColor(g_enabled ? "Lua/ImGui waypoint POC: ON" : "Lua/ImGui waypoint POC: OFF", 0x0D);
		return;
	}

	if (!_stricmp(szLine, "status")) {
		RenderStatusToChat();
		return;
	}

	if (!_stricmp(szLine, "refresh")) {
		RequestList();
		return;
	}

	if (!_stricmp(szLine, "next")) {
		if (!g_entries.empty()) {
			g_selected_index = MinInt(static_cast<int>(g_entries.size()) - 1, g_selected_index + 1);
		}
		return;
	}

	if (!_stricmp(szLine, "prev")) {
		if (!g_entries.empty()) {
			g_selected_index = MaxInt(0, g_selected_index - 1);
		}
		return;
	}

	if (!_stricmp(szLine, "travel")) {
		TravelSelected();
		return;
	}

	if (!_stricmp(szLine, "reload")) {
		if (LoadLuaScript()) {
			WriteChatColor("Lua/ImGui waypoint POC: script reloaded.", 0x0D);
		} else {
			WriteChatColor("Lua/ImGui waypoint POC: script reload failed (see /waypointimgui status).", 0x0D);
		}
		return;
	}

	WriteChatColor("Usage: /waypointimgui [toggle|status|refresh|next|prev|travel|reload]", 0x0E);
}
