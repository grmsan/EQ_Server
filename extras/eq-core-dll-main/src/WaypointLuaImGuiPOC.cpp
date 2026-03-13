#include "WaypointLuaImGuiPOC.h"
#include "MQ2Main.h"
#include "WaypointPOCWnd.h"
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
extern bool isDebugLoggingEnabled;
extern bool isWaypointPOCLoggingEnabled;

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
CRITICAL_SECTION g_state_cs;
bool g_state_cs_initialized = false;

lua_State* g_lua_state = nullptr;
int g_lua_draw_ref = LUA_NOREF;
bool g_script_loaded = false;

bool g_create_device_hook_installed = false;
bool g_present_hook_installed = false;
bool g_end_scene_hook_installed = false;
bool g_reset_hook_installed = false;
bool g_present_method_detour_installed = false;
bool g_end_scene_method_detour_installed = false;
bool g_reset_method_detour_installed = false;
bool g_imgui_initialized = false;
bool g_runtime_initialized = false;
bool g_wndproc_installed = false;
bool g_logged_first_present = false;
bool g_logged_first_end_scene = false;
bool g_logged_graphics_method_addresses = false;
DWORD g_last_present_tick = 0;
DWORD g_last_end_scene_tick = 0;
DWORD g_last_lua_error_tick = 0;
char g_last_lua_error[256] = { 0 };

HWND g_host_hwnd = nullptr;
HANDLE g_host_thread = nullptr;
DWORD g_host_thread_id = 0;
IDirect3D9* g_host_d3d = nullptr;
IDirect3DDevice9* g_host_device = nullptr;
D3DPRESENT_PARAMETERS g_host_present_params = {};
volatile LONG g_host_window_requested = 0;
volatile LONG g_host_window_running = 0;
volatile LONG g_pending_request_list = 0;
volatile LONG g_pending_reload_script = 0;
volatile LONG g_pending_travel_waypoint_id = 0;

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

using EndSceneFunc = HRESULT __stdcall(
	IDirect3DDevice9&
);

CreateDeviceFunc* g_real_create_device = nullptr;
PresentFunc* g_real_present = nullptr;
ResetFunc* g_real_reset = nullptr;
EndSceneFunc* g_real_end_scene = nullptr;

bool LoadLuaScript();
void CallLuaDraw();

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

void EnsureStateLock()
{
	if (!g_state_cs_initialized) {
		InitializeCriticalSection(&g_state_cs);
		g_state_cs_initialized = true;
	}
}

void LockState()
{
	if (g_state_cs_initialized) {
		EnterCriticalSection(&g_state_cs);
	}
}

void UnlockState()
{
	if (g_state_cs_initialized) {
		LeaveCriticalSection(&g_state_cs);
	}
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
		if (required != size) {
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
	if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
		LogDebug("[WAYPOINT_IMGUI] request list");
	}
	WaypointPOC_BeginPacketTrace("imgui");
	InterlockedExchange(&g_pending_request_list, 1);
}

void TravelToWaypoint(int waypoint_id)
{
	if (waypoint_id <= 0) {
		return;
	}

	char cmd[128] = { 0 };
	sprintf_s(cmd, "/say #wppoc travel %d", waypoint_id);
	if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
		LogDebug("[WAYPOINT_IMGUI] travel waypoint_id=%d cmd=%s", waypoint_id, cmd);
	}
	InterlockedExchange(&g_pending_travel_waypoint_id, waypoint_id);
}

void TravelSelected()
{
	LockState();
	const bool has_selection = !g_entries.empty();
	if (!has_selection) {
		UnlockState();
		WriteChatColor("Lua/ImGui POC: no waypoint selected.", 0x0E);
		return;
	}

	g_selected_index = ClampIndex(g_selected_index, 0, static_cast<int>(g_entries.size()) - 1);
	const auto& selected = g_entries[g_selected_index];
	const int waypoint_id = selected.waypoint_id;
	UnlockState();
	TravelToWaypoint(waypoint_id);
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
	g_logged_first_present = false;
	g_logged_first_end_scene = false;
	g_last_present_tick = 0;
	g_last_end_scene_tick = 0;
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

void ShutdownHostWindow()
{
	if (g_host_hwnd) {
		PostMessage(g_host_hwnd, WM_CLOSE, 0, 0);
	}

	if (g_host_thread) {
		WaitForSingleObject(g_host_thread, 3000);
		CloseHandle(g_host_thread);
		g_host_thread = nullptr;
	}
	g_host_thread_id = 0;
}

bool CreateHostDevice(HWND hwnd)
{
	g_host_d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (!g_host_d3d) {
		return false;
	}

	ZeroMemory(&g_host_present_params, sizeof(g_host_present_params));
	g_host_present_params.Windowed = TRUE;
	g_host_present_params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_host_present_params.EnableAutoDepthStencil = TRUE;
	g_host_present_params.AutoDepthStencilFormat = D3DFMT_D16;
	g_host_present_params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	g_host_present_params.hDeviceWindow = hwnd;
	g_host_present_params.BackBufferFormat = D3DFMT_UNKNOWN;

	if (FAILED(g_host_d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&g_host_present_params,
		&g_host_device))) {
		if (FAILED(g_host_d3d->CreateDevice(
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			hwnd,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING,
			&g_host_present_params,
			&g_host_device))) {
			g_host_d3d->Release();
			g_host_d3d = nullptr;
			return false;
		}
	}

	return true;
}

void DestroyHostDevice()
{
	if (g_host_device) {
		g_host_device->Release();
		g_host_device = nullptr;
	}
	if (g_host_d3d) {
		g_host_d3d->Release();
		g_host_d3d = nullptr;
	}
}

LRESULT CALLBACK WaypointImguiHostWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (g_imgui_initialized && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
		return 1;
	}

	switch (msg) {
	case WM_SIZE:
		if (g_host_device && wParam != SIZE_MINIMIZED) {
			g_host_present_params.BackBufferWidth = LOWORD(lParam);
			g_host_present_params.BackBufferHeight = HIWORD(lParam);
			if (g_imgui_initialized) {
				ImGui_ImplDX9_InvalidateDeviceObjects();
			}
			if (SUCCEEDED(g_host_device->Reset(&g_host_present_params)) && g_imgui_initialized) {
				ImGui_ImplDX9_CreateDeviceObjects();
			}
		}
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

DWORD WINAPI WaypointImguiHostThread(LPVOID)
{
	const char* class_name = "EQWaypointImGuiPOCHost";
	WNDCLASSEXA wc = {};
	wc.cbSize = sizeof(wc);
	wc.style = CS_CLASSDC;
	wc.lpfnWndProc = WaypointImguiHostWndProc;
	wc.hInstance = ghInstance;
	wc.lpszClassName = class_name;
	RegisterClassExA(&wc);

	RECT eq_rect = { 100, 100, 740, 620 };
	HWND eq_hwnd = ResolveEQWindowHandle();
	if (eq_hwnd) {
		GetWindowRect(eq_hwnd, &eq_rect);
	}

	g_host_hwnd = CreateWindowA(
		class_name,
		"Waypoint Lua/ImGui POC",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		eq_rect.left + 40,
		eq_rect.top + 40,
		560,
		460,
		nullptr,
		nullptr,
		ghInstance,
		nullptr
	);

	if (!g_host_hwnd) {
		UnregisterClassA(class_name, ghInstance);
		InterlockedExchange(&g_host_window_running, 0);
		return 0;
	}

	if (!CreateHostDevice(g_host_hwnd)) {
		DestroyWindow(g_host_hwnd);
		g_host_hwnd = nullptr;
		UnregisterClassA(class_name, ghInstance);
		InterlockedExchange(&g_host_window_running, 0);
		return 0;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui_ImplWin32_Init(g_host_hwnd);
	ImGui_ImplDX9_Init(g_host_device);
	g_imgui_initialized = true;
	InterlockedExchange(&g_host_window_running, 1);

	if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
		LogDebug("[WAYPOINT_IMGUI] host window started hwnd=0x%08X", static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_host_hwnd)));
	}

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (InterlockedCompareExchange(&g_host_window_requested, 0, 0) != 0) {
		while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				InterlockedExchange(&g_host_window_requested, 0);
			}
		}

		if (InterlockedCompareExchange(&g_pending_reload_script, 0, 0) != 0) {
			InterlockedExchange(&g_pending_reload_script, 0);
			LoadLuaScript();
		}

		if (g_host_device) {
			HRESULT coop = g_host_device->TestCooperativeLevel();
			if (coop == D3DERR_DEVICELOST) {
				Sleep(50);
				continue;
			}
			if (coop == D3DERR_DEVICENOTRESET) {
				ImGui_ImplDX9_InvalidateDeviceObjects();
				if (SUCCEEDED(g_host_device->Reset(&g_host_present_params))) {
					ImGui_ImplDX9_CreateDeviceObjects();
				}
			}

			g_host_device->SetRenderState(D3DRS_ZENABLE, FALSE);
			g_host_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			g_host_device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
			g_host_device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(18, 18, 24, 255), 1.0f, 0);

			if (SUCCEEDED(g_host_device->BeginScene())) {
				ImGui_ImplDX9_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();
				CallLuaDraw();
				ImGui::EndFrame();
				ImGui::Render();
				ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
				g_host_device->EndScene();
			}

			g_host_device->Present(nullptr, nullptr, nullptr, nullptr);
		}

		Sleep(10);
	}

	ClearImGuiRuntime();
	DestroyHostDevice();
	if (g_host_hwnd) {
		DestroyWindow(g_host_hwnd);
		g_host_hwnd = nullptr;
	}
	UnregisterClassA(class_name, ghInstance);
	InterlockedExchange(&g_host_window_running, 0);
	if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
		LogDebug("[WAYPOINT_IMGUI] host window stopped");
	}
	return 0;
}

void EnsureHostWindowRunning()
{
	if (InterlockedCompareExchange(&g_host_window_requested, 1, 1) != 0 &&
		InterlockedCompareExchange(&g_host_window_running, 1, 1) != 0) {
		return;
	}

	if (g_host_thread) {
		return;
	}

	InterlockedExchange(&g_host_window_requested, 1);
	g_host_thread = CreateThread(nullptr, 0, WaypointImguiHostThread, nullptr, 0, &g_host_thread_id);
}

int Lua_WaypointGetEntries(lua_State* L)
{
	lua_newtable(L);

	LockState();
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
	UnlockState();

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
	LockState();
	lua_pushinteger(L, g_selected_index + 1); // Lua-facing index is 1-based.
	UnlockState();
	return 1;
}

int Lua_WaypointSetSelectedIndex(lua_State* L)
{
	const int requested = static_cast<int>(luaL_checkinteger(L, 1)) - 1;
	LockState();
	if (g_entries.empty()) {
		g_selected_index = 0;
		UnlockState();
		return 0;
	}

	g_selected_index = ClampIndex(requested, 0, static_cast<int>(g_entries.size()) - 1);
	UnlockState();
	return 0;
}

int Lua_WaypointHasPacket(lua_State* L)
{
	LockState();
	lua_pushboolean(L, g_have_packet ? 1 : 0);
	UnlockState();
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
		if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
			LogDebug("[WAYPOINT_IMGUI] lua load failed path=%s err=%s", script_path, g_last_lua_error);
		}
		lua_pop(g_lua_state, 1);
		g_script_loaded = false;
		return false;
	}

	if (lua_pcall(g_lua_state, 0, 1, 0) != LUA_OK) {
		SetLuaError(lua_tostring(g_lua_state, -1));
		if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
			LogDebug("[WAYPOINT_IMGUI] lua exec failed path=%s err=%s", script_path, g_last_lua_error);
		}
		lua_pop(g_lua_state, 1);
		g_script_loaded = false;
		return false;
	}

	if (lua_istable(g_lua_state, -1)) {
		lua_getfield(g_lua_state, -1, "draw");
		if (!lua_isfunction(g_lua_state, -1)) {
			lua_pop(g_lua_state, 2);
			SetLuaError("Lua script must return table with draw() function.");
			if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
				LogDebug("[WAYPOINT_IMGUI] lua missing draw() table path=%s", script_path);
			}
			g_script_loaded = false;
			return false;
		}
		lua_remove(g_lua_state, -2); // remove table, leave function
	}
	else if (!lua_isfunction(g_lua_state, -1)) {
		lua_pop(g_lua_state, 1);
		SetLuaError("Lua script must return function or table with draw() function.");
		if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
			LogDebug("[WAYPOINT_IMGUI] lua return type invalid path=%s", script_path);
		}
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
	std::vector<WaypointEntry> entries_snapshot;
	int selected_index = 0;
	bool have_packet = false;
	LockState();
	entries_snapshot = g_entries;
	selected_index = g_selected_index;
	have_packet = g_have_packet;
	UnlockState();

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
	if (!have_packet) {
		ImGui::TextUnformatted("No waypoint packet yet. Click Refresh.");
	}

	for (int i = 0; i < static_cast<int>(entries_snapshot.size()); ++i) {
		const auto& e = entries_snapshot[i];
		char line[256] = { 0 };
		sprintf_s(line, "[%d] %s", e.waypoint_id, e.name.c_str());
		if (ImGui::Selectable(line, (i == selected_index))) {
			LockState();
			g_selected_index = i;
			UnlockState();
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

void RenderImGuiFrame(IDirect3DDevice9& device)
{
	if (!EnsureImGuiInitialized(device)) {
		return;
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	CallLuaDraw();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
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
	g_last_present_tick = GetTickCount();

	if (g_enabled && gGameState == GAMESTATE_INGAME) {
		if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled && !g_logged_first_present) {
			LogDebug("[WAYPOINT_IMGUI] present hook active entries=%u packet=%d",
				static_cast<unsigned>(g_entries.size()),
				g_have_packet ? 1 : 0);
			g_logged_first_present = true;
		}
		// Prefer EndScene for rendering if it is active; use Present as fallback only.
		if (InterlockedCompareExchange(&g_host_window_running, 0, 0) == 0 &&
			(!g_end_scene_hook_installed || (GetTickCount() - g_last_end_scene_tick) > 250)) {
			RenderImGuiFrame(device);
		}
	}

	return g_real_present(device, source_rect, dest_rect, dest_window_override, dirty_region);
}

HRESULT __stdcall EndSceneHook(IDirect3DDevice9& device)
{
	if (g_enabled && gGameState == GAMESTATE_INGAME) {
		if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled && !g_logged_first_end_scene) {
			LogDebug("[WAYPOINT_IMGUI] endscene hook active entries=%u packet=%d",
				static_cast<unsigned>(g_entries.size()),
				g_have_packet ? 1 : 0);
			g_logged_first_end_scene = true;
		}
		g_last_end_scene_tick = GetTickCount();
		if (InterlockedCompareExchange(&g_host_window_running, 0, 0) == 0) {
			RenderImGuiFrame(device);
		}
	}

	return g_real_end_scene(device);
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
		if (!g_real_present && !g_present_method_detour_installed) {
			g_real_present = hook_vtable<PresentFunc>(**returned_device_interface, 17, PresentHook);
			g_present_hook_installed = (g_real_present != nullptr);
			if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
				LogDebug("[WAYPOINT_IMGUI] hooked present via CreateDevice success=%d", g_present_hook_installed ? 1 : 0);
			}
		}
		if (!g_real_reset && !g_reset_method_detour_installed) {
			g_real_reset = hook_vtable<ResetFunc>(**returned_device_interface, 16, ResetHook);
			g_reset_hook_installed = (g_real_reset != nullptr);
		}
		if (!g_real_end_scene && !g_end_scene_method_detour_installed) {
			g_real_end_scene = hook_vtable<EndSceneFunc>(**returned_device_interface, 42, EndSceneHook);
			g_end_scene_hook_installed = (g_real_end_scene != nullptr);
			if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
				LogDebug("[WAYPOINT_IMGUI] hooked endscene via CreateDevice success=%d", g_end_scene_hook_installed ? 1 : 0);
			}
		}
	}

	return result;
}

void EnsureGraphicsHooks()
{
	IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (!d3d) {
		return;
	}

	if (!g_create_device_hook_installed) {
		g_real_create_device = hook_vtable<CreateDeviceFunc>(*d3d, 16, CreateDeviceHook);
		g_create_device_hook_installed = (g_real_create_device != nullptr);
		if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
			LogDebug("[WAYPOINT_IMGUI] hook create_device success=%d", g_create_device_hook_installed ? 1 : 0);
		}
	}

	if (!g_present_hook_installed) {
		D3DPRESENT_PARAMETERS pp = {};
		pp.Windowed = TRUE;
		pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		pp.hDeviceWindow = ResolveEQWindowHandle();

		IDirect3DDevice9* temp_device = nullptr;
		if (pp.hDeviceWindow &&
			SUCCEEDED(d3d->CreateDevice(
				D3DADAPTER_DEFAULT,
				D3DDEVTYPE_HAL,
				pp.hDeviceWindow,
				D3DCREATE_SOFTWARE_VERTEXPROCESSING,
				&pp,
				&temp_device
			)) &&
			temp_device) {
			auto** temp_vtable = *reinterpret_cast<void***>(temp_device);
			const DWORD present_addr = static_cast<DWORD>(reinterpret_cast<uintptr_t>(temp_vtable[17]));
			const DWORD reset_addr = static_cast<DWORD>(reinterpret_cast<uintptr_t>(temp_vtable[16]));
			const DWORD end_scene_addr = static_cast<DWORD>(reinterpret_cast<uintptr_t>(temp_vtable[42]));

			if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled && !g_logged_graphics_method_addresses) {
				LogDebug("[WAYPOINT_IMGUI] d3d methods present=0x%08X reset=0x%08X endscene=0x%08X",
					static_cast<unsigned>(present_addr),
					static_cast<unsigned>(reset_addr),
					static_cast<unsigned>(end_scene_addr));
				g_logged_graphics_method_addresses = true;
			}

			if (!g_present_method_detour_installed && present_addr) {
				AddDetourf(present_addr, PresentHook, g_real_present);
				g_present_method_detour_installed = (g_real_present != nullptr);
				g_present_hook_installed = g_present_method_detour_installed || g_present_hook_installed;
				if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
					LogDebug("[WAYPOINT_IMGUI] hook present via method detour success=%d", g_present_method_detour_installed ? 1 : 0);
				}
			}
			if (!g_reset_method_detour_installed && reset_addr) {
				AddDetourf(reset_addr, ResetHook, g_real_reset);
				g_reset_method_detour_installed = (g_real_reset != nullptr);
				g_reset_hook_installed = g_reset_method_detour_installed || g_reset_hook_installed;
				if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
					LogDebug("[WAYPOINT_IMGUI] hook reset via method detour success=%d", g_reset_method_detour_installed ? 1 : 0);
				}
			}
			if (!g_end_scene_method_detour_installed && end_scene_addr) {
				AddDetourf(end_scene_addr, EndSceneHook, g_real_end_scene);
				g_end_scene_method_detour_installed = (g_real_end_scene != nullptr);
				g_end_scene_hook_installed = g_end_scene_method_detour_installed || g_end_scene_hook_installed;
				if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
					LogDebug("[WAYPOINT_IMGUI] hook endscene via method detour success=%d", g_end_scene_method_detour_installed ? 1 : 0);
				}
			}
			temp_device->Release();
		}
	}

	d3d->Release();
}

void ProcessPendingActions()
{
	if (!pLocalPlayer) {
		return;
	}

	if (InterlockedExchange(&g_pending_request_list, 0) != 0) {
		char cmd[] = "/say #wppoc list";
		DoCommand((PSPAWNINFO)pLocalPlayer, cmd);
	}

	const LONG waypoint_id = InterlockedExchange(&g_pending_travel_waypoint_id, 0);
	if (waypoint_id > 0) {
		char cmd[128] = { 0 };
		sprintf_s(cmd, "/say #wppoc travel %ld", waypoint_id);
		DoCommand((PSPAWNINFO)pLocalPlayer, cmd);
	}
}

void RenderStatusToChat()
{
	LockState();
	const int entry_count = static_cast<int>(g_entries.size());
	UnlockState();

	char line[256] = { 0 };
	sprintf_s(
		line,
		"Lua/ImGui status: enabled=%s hooks(create=%s present=%s reset=%s) host=%s imgui=%s lua=%s script=%s entries=%d",
		g_enabled ? "yes" : "no",
		g_create_device_hook_installed ? "yes" : "no",
		g_present_hook_installed ? "yes" : "no",
		g_reset_hook_installed ? "yes" : "no",
		InterlockedCompareExchange(&g_host_window_running, 0, 0) != 0 ? "yes" : "no",
		g_imgui_initialized ? "yes" : "no",
		g_lua_state ? "yes" : "no",
		g_script_loaded ? "yes" : "no",
		entry_count
	);
	WriteChatColor(line, 0x0E);
	if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
		LogDebug("[WAYPOINT_IMGUI] %s", line);
	}

	if (g_last_lua_error[0]) {
		char err_line[300] = { 0 };
		sprintf_s(err_line, "Lua/ImGui last error: %s", g_last_lua_error);
		WriteChatColor(err_line, 0x0D);
		if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
			LogDebug("[WAYPOINT_IMGUI] %s", err_line);
		}
	}

	char extra_line[256] = { 0 };
	sprintf_s(
		extra_line,
		"Lua/ImGui hook detail: endscene=%s present_method=%s endscene_method=%s last_present_ms=%lu last_endscene_ms=%lu",
		g_end_scene_hook_installed ? "yes" : "no",
		g_present_method_detour_installed ? "yes" : "no",
		g_end_scene_method_detour_installed ? "yes" : "no",
		g_last_present_tick,
		g_last_end_scene_tick
	);
	WriteChatColor(extra_line, 0x0E);
	if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
		LogDebug("[WAYPOINT_IMGUI] %s", extra_line);
	}
}
}

void WaypointLuaImGuiPOC_Initialize()
{
	if (g_runtime_initialized) {
		return;
	}

	g_runtime_initialized = true;
	EnsureStateLock();
	EnsureGraphicsHooks();
	EnsureLuaRuntime();
}

void WaypointLuaImGuiPOC_Shutdown()
{
	g_enabled = false;
	InterlockedExchange(&g_host_window_requested, 0);
	ShutdownHostWindow();
	ClearImGuiRuntime();
	ClearLuaRuntime();
	if (g_state_cs_initialized) {
		DeleteCriticalSection(&g_state_cs);
		g_state_cs_initialized = false;
	}
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

	LockState();
	g_entries = std::move(parsed);
	g_have_packet = true;
	if (g_selected_index >= static_cast<int>(g_entries.size())) {
		g_selected_index = MaxInt(0, static_cast<int>(g_entries.size()) - 1);
	}
	const unsigned parsed_count = static_cast<unsigned>(g_entries.size());
	const int selected_index = g_selected_index;
	UnlockState();
	if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
		LogDebug("[WAYPOINT_IMGUI] packet parsed enabled=%u selected=%d",
			parsed_count,
			selected_index + 1);
	}
}

void WaypointLuaImGuiPOC_Draw()
{
	if (!g_enabled || gGameState != GAMESTATE_INGAME) {
		return;
	}
}

void WaypointLuaImGuiPOC_Pulse()
{
	ProcessPendingActions();
	if (g_enabled && InterlockedCompareExchange(&g_host_window_running, 0, 0) == 0) {
		EnsureHostWindowRunning();
	}
}

void WaypointLuaImGuiPOC_CleanUI()
{
	LockState();
	g_selected_index = 0;
	g_have_packet = false;
	g_entries.clear();
	UnlockState();
}

void WaypointLuaImGuiPOCCmd(PSPAWNINFO pChar, PCHAR szLine)
{
	(void)pChar;

	if (!szLine || !szLine[0] || !_stricmp(szLine, "toggle") || !_stricmp(szLine, "show")) {
		EnsureGraphicsHooks();
		EnsureLuaRuntime();
		g_enabled = !g_enabled;
		if (g_enabled) {
			EnsureHostWindowRunning();
		} else {
			InterlockedExchange(&g_host_window_requested, 0);
			ShutdownHostWindow();
		}
		LockState();
		const bool have_packet = g_have_packet;
		UnlockState();
		if (g_enabled && !have_packet) {
			RequestList();
		}
		if (isDebugLoggingEnabled && isWaypointPOCLoggingEnabled) {
			LogDebug("[WAYPOINT_IMGUI] toggle enabled=%d hooks(create=%d present=%d)",
				g_enabled ? 1 : 0,
				g_create_device_hook_installed ? 1 : 0,
				g_present_hook_installed ? 1 : 0);
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
		InterlockedExchange(&g_pending_reload_script, 1);
		WriteChatColor("Lua/ImGui waypoint POC: script reload queued.", 0x0D);
		return;
	}

	WriteChatColor("Usage: /waypointimgui [toggle|status|refresh|next|prev|travel|reload]", 0x0E);
}
