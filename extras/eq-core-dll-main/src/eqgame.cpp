#include <Windows.h>
#pragma comment(lib, "Iphlpapi.lib")
#include "MQ2Main.h"
#include "EQData.h"
#include "EQClasses.h"
#include "eqgame.h"
#include <stdio.h>
#include <map>
#include "dinput8.h"
#include "detours.h"
#include "eqmac.h"
#include "eqmac_functions.h"
#include "d3d9.h"
#include "xorstr.h"
#include <tlhelp32.h>
#include <list>
#include <Psapi.h>
#include <cstdint>
#include <vector>
#include <set>
#include <dinput.h>
#include <intrin.h>
#include <iphlpapi.h>
#include <IPTypes.h>
#include "spaghetti.h"
#include <thread>
#include <chrono>
#include <limits.h>
#include <intrin.h>

#include "core_init.h"
#include <ctime>

//#pragma comment(lib, "Iphlpapi.lib")

static const char* kDebugLogLocalPath = "dinput8_debug.log";
static const char* kDebugLogRepoPath = "C:\\Users\\marsh\\OneDrive\\Documents\\GitHub\\EQ_Server\\logs\\dinput8_debug.log";

static void WriteDebugLineToFile(const char* file_path, const char* line)
{
	if (!file_path || !line) {
		return;
	}
	FILE* file = nullptr;
	if (fopen_s(&file, file_path, "a") == 0 && file) {
		std::time_t now = std::time(nullptr);
		char ts[20];
		std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
		fprintf(file, "[%s] %s\n", ts, line);
		fclose(file);
	}
}

static void WriteDebugLineBoth(const char* line)
{
	// Mirror logs to both client working-dir and repo logs path so diagnostics are visible in either workflow.
	WriteDebugLineToFile(kDebugLogLocalPath, line);
	CreateDirectoryA("C:\\Users\\marsh\\OneDrive\\Documents\\GitHub\\EQ_Server\\logs", nullptr);
	WriteDebugLineToFile(kDebugLogRepoPath, line);
}

void LogDebug(const char* format, ...) {
	if (!isDebugLoggingEnabled) {
		return;
	}
	char msg[4096] = { 0 };
	va_list args;
	va_start(args, format);
	vsnprintf(msg, sizeof(msg), format, args);
	va_end(args);
	WriteDebugLineBoth(msg);
}

enum HookTag : LONG {
	Hook_None = 0,
	Hook_HandleWorldMessage = 1,
	Hook_MaxHP = 2,
	Hook_CurHP = 3,
	Hook_GetGaugeValueFromEQ = 4,
	Hook_GetLabelFromEQ = 5,
	Hook_GetUsableClasses = 6,
	Hook_IsSpellcaster = 7,
	Hook_IsSpellcaster2 = 8,
	Hook_IsSpellcaster3 = 9,
	Hook_CanStartMemming = 10,
	Hook_GetSpellLevelNeeded = 11,
	Hook_GetPcSkillLimit = 12
};

static volatile LONG g_last_hook_tag = Hook_None;

// Deferred install helpers for server-authoritative stats detours.
void EnsureServerAuthoritativeStatsInstallArmed();
void EdgeStats_MaybeInstallDetoursFromMainThread();

// Write-watch helper (defined later)
static bool IsWriteWatchExpectedAV(DWORD code, ULONG_PTR info0, ULONG_PTR info1);

// Forward declaration (defined later)
static void DumpRecentPackets(FILE* f, size_t count);

static void LogRawDebug(const char* format, ...)
{
	char msg[4096] = { 0 };
	va_list args;
	va_start(args, format);
	vsnprintf(msg, sizeof(msg), format, args);
	va_end(args);
	WriteDebugLineBoth(msg);
}

static LONG CALLBACK CrashLogVEH(PEXCEPTION_POINTERS ep)
{
	if (!ep || !ep->ExceptionRecord) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	const DWORD code = ep->ExceptionRecord->ExceptionCode;
	// Log only potentially fatal exceptions
	switch (code) {
		case EXCEPTION_ACCESS_VIOLATION:
		case EXCEPTION_ILLEGAL_INSTRUCTION:
		case EXCEPTION_STACK_OVERFLOW:
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		case EXCEPTION_DATATYPE_MISALIGNMENT:
		case EXCEPTION_IN_PAGE_ERROR:
		case EXCEPTION_PRIV_INSTRUCTION:
			break;
		default:
			return EXCEPTION_CONTINUE_SEARCH;
	}

#ifdef _M_X64
	void* ip = (void*)ep->ContextRecord->Rip;
	void* sp = (void*)ep->ContextRecord->Rsp;
#else
	void* ip = (void*)ep->ContextRecord->Eip;
	void* sp = (void*)ep->ContextRecord->Esp;
#endif

	ULONG_PTR info0 = 0;
	ULONG_PTR info1 = 0;
	if (ep->ExceptionRecord->NumberParameters >= 1) {
		info0 = ep->ExceptionRecord->ExceptionInformation[0];
	}
	if (ep->ExceptionRecord->NumberParameters >= 2) {
		info1 = ep->ExceptionRecord->ExceptionInformation[1];
	}

	// Suppress noisy "crash" logs for write-watch induced access violations that are expected to be handled.
	if (IsWriteWatchExpectedAV(code, info0, info1)) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	DWORD tid = GetCurrentThreadId();
	int safe_get_gamestate = -9999;
	__try {
		safe_get_gamestate = GetGameState();
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		safe_get_gamestate = -9998;
	}

	LogRawDebug(
		"CRASH_VEH tid=%lu code=0x%08X ex_addr=%p ip=%p info0=%llu info1=%p last_hook=%ld gamestate=%lu get_gamestate=%d sp=%p",
		(unsigned long)tid,
		code,
		ep->ExceptionRecord->ExceptionAddress,
		ip,
		(unsigned long long)info0,
		(void*)info1,
		(long)g_last_hook_tag,
		(unsigned long)gGameState,
		safe_get_gamestate,
		sp
	);
	LogRawDebug("CRASH_VEH baseAddress=0x%08X ip_rva=0x%08X", (unsigned)baseAddress, (unsigned)((uintptr_t)ip - (uintptr_t)baseAddress));
	// Dump some recent packets if available
	{
		FILE* rf = nullptr;
		if (fopen_s(&rf, "dinput8_debug.log", "a") == 0 && rf) {
			fprintf(rf, "CRASH_VEH recent packets:\n");
			DumpRecentPackets(rf, 40);
			fclose(rf);
		}
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

static void InstallCrashDiagnostics()
{
	static LONG installed = 0;
	if (InterlockedCompareExchange(&installed, 1, 0) != 0) {
		return;
	}
	AddVectoredExceptionHandler(0, CrashLogVEH);
	LogDebug("Crash diagnostics installed (VEH)");
}

static void LogDetourTargetBytes(const char* name, DWORD addr)
{
	unsigned char b[16] = {0};
	bool ok = false;
	__try {
		memcpy(b, (void*)addr, sizeof(b));
		ok = true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		ok = false;
	}

	if (!ok) {
		LogDebug("DetourTarget %s addr=0x%08X read FAILED", name, (unsigned)addr);
		return;
	}

	LogDebug(
		"DetourTarget %s addr=0x%08X bytes=%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
		name, (unsigned)addr,
		b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7],
		b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]
	);
}

static void ConditionalDumpOpcode(uint16_t op, const char* buf, size_t size, const char* toggle_filename)
{
	// Toggle files are intentionally ignored; dump behavior is controlled via `_options.h` booleans.
	(void)toggle_filename;

	if (!isDebugLoggingEnabled) {
		return;
	}

	// Respect per-opcode dump toggles
	const uint16_t lop = op & 0xFFFF;
	if (lop == 0x1338 && !isEdgeStatLabelDumpEnabled) {
		return;
	}
	if (lop == 0x575b && !isOpcode575bDumpEnabled) {
		return;
	}

	// Ensure repo logs/dumps exists
	const char* repo_logs_dir = "C:\\Users\\marsh\\OneDrive\\Documents\\GitHub\\EQ_Server\\logs";
	const char* repo_dumps_dir = "C:\\Users\\marsh\\OneDrive\\Documents\\GitHub\\EQ_Server\\logs\\dumps";
	CreateDirectoryA(repo_logs_dir, nullptr);
	CreateDirectoryA(repo_dumps_dir, nullptr);

	time_t now = time(nullptr);
	struct tm* tmv = localtime(&now);
	char tb[32] = {0};
	strftime(tb, sizeof(tb), "%Y%m%d_%H%M%S", tmv);
	char dumpname[1024];
	snprintf(dumpname, sizeof(dumpname), "C:\\Users\\marsh\\OneDrive\\Documents\\GitHub\\EQ_Server\\logs\\dumps\\packet_%s_%04x.bin", tb, lop);
	FILE* df = nullptr;
	if (fopen_s(&df, dumpname, "wb") == 0 && df) {
		fwrite(buf, 1, size, df);
		fclose(df);
	}

	// Also append a short human-readable line to the repo stats log
	FILE* lf = nullptr;
	if (fopen_s(&lf, "C:\\Users\\marsh\\OneDrive\\Documents\\GitHub\\EQ_Server\\logs\\stats_debug.log", "a") == 0 && lf) {
		fprintf(lf, "%s DUMP opcode=0x%04x size=%zu -> %s\n", tb, lop, size, dumpname);
		fclose(lf);
	}
}

// Use simple integers for last-logged detour values to avoid global C++
// container construction during DLL load which can be fragile.
static int g_last_logged_hp = INT_MIN;
static int g_last_logged_mana = INT_MIN;
static int g_last_logged_end = INT_MIN;

static void ConditionalLogEdgeStat(const char* buf, size_t size, const char* toggle_filename)
{
	(void)toggle_filename;
	if (!isEdgeStatLabelLoggingEnabled) {
		return;
	}

	if (!buf || size < 4) return;
	uint32_t count = 0;
	memcpy(&count, buf, sizeof(uint32_t));
	if (count > 1000) return; // sanity cap

	FILE* lf = nullptr;
	if (fopen_s(&lf, "C:\\Users\\marsh\\OneDrive\\Documents\\GitHub\\EQ_Server\\logs\\stats_debug.log", "a") == 0 && lf) {
		time_t now = time(nullptr);
		struct tm* tmv = localtime(&now);
		char tb[32] = {0};
		strftime(tb, sizeof(tb), "%Y%m%d_%H%M%S", tmv);
		fprintf(lf, "%s EDGE_STAT count=%u\n", tb, count);
		size_t expected = sizeof(uint32_t) + (size_t)count * (sizeof(uint32_t) + sizeof(uint64_t));
		if (size < expected) {
			fprintf(lf, "  WARNING: packet too small for declared count (size=%zu expected=%zu)\n", size, expected);
		}
		for (uint32_t i = 0; i < count; ++i) {
			size_t off = sizeof(uint32_t) + i * (sizeof(uint32_t) + sizeof(uint64_t));
			if (off + sizeof(uint32_t) + sizeof(uint64_t) > size) break;
			uint32_t key = 0;
			uint64_t value = 0;
			memcpy(&key, buf + off, sizeof(uint32_t));
			memcpy(&value, buf + off + sizeof(uint32_t), sizeof(uint64_t));
			fprintf(lf, "  stat[%u] key=%u value=%llu\n", i, key, (unsigned long long)value);
		}
		fclose(lf);
	}
}

extern "C" { __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; }
extern "C" { __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1; }

extern void Pulse();
extern bool was_background;
extern void LoadIniSettings();
extern HMODULE heqwMod;
HANDLE myproc = 0;
bool title_set = false;
bool first_maximize = true;
bool can_fullscreen = false;

bool is_digits(const std::string &str)
{
	return str.find_first_not_of("0123456789") == std::string::npos;
}

bool ResolutionStored = false;
DWORD resx = 0;
DWORD resy = 0;
DWORD bpp = 0;
DWORD refresh = 0;
HMODULE eqmain_dll = 0;
BOOL bExeChecksumrequested = 0;
BOOL g_mouseWheelZoomIsEnabled = true;
unsigned int g_buffWindowTimersFontSize = 3;
bool has_focus = true;
WINDOWINFO stored_window_info;
WINDOWPLACEMENT g_wpPrev = { sizeof(g_wpPrev) };
bool start_fullscreen = false;
bool in_full_screen = false;
bool startup = true;
POINT posPoint;
DWORD o_MouseEvents = 0x0055B3B9;
DWORD o_MouseCenter = 0x0055B722;

typedef signed int(__cdecl* ProcessGameEvents_t)();
ProcessGameEvents_t return_ProcessGameEvents;
ProcessGameEvents_t return_ProcessMouseEvent;
ProcessGameEvents_t return_SetMouseCenter;

DWORD d3ddev = 0;
DWORD eqgfxMod = 0;
BOOL bWindowedMode = true;

#define DLL_VERSION_NUMBER (uint64_t)140

typedef struct _detourinfo
{
	DWORD_PTR tramp;
	DWORD_PTR detour;
}detourinfo;
std::map<DWORD,_detourinfo> ourdetours;


#define FUNCTION_AT_ADDRESS(function,offset) __declspec(naked) function\
{\
	__asm{mov eax, offset};\
	__asm{jmp eax};\
}

#define EQ_FUNCTION_flush_mouse 0x0055B5B9
#ifdef EQ_FUNCTION_flush_mouse
FUNCTION_AT_ADDRESS(signed int EQ_flush_mouse(), EQ_FUNCTION_flush_mouse);
#endif

void PatchA(LPVOID address, const void *dwValue, SIZE_T dwBytes) {
	unsigned long oldProtect;
	VirtualProtect((void *)address, dwBytes, PAGE_EXECUTE_READWRITE, &oldProtect);
	FlushInstructionCache(GetCurrentProcess(), (void *)address, dwBytes);
	memcpy((void *)address, dwValue, dwBytes);
	VirtualProtect((void *)address, dwBytes, oldProtect, &oldProtect);
}

char bMySEQDetected = 3;


//Function Addr's
DWORD CXWndActivateAddr = 0x864100;
DWORD ValueSellMerchantAddr = 0x5E1690;
DWORD IsItemRentable = 0x5E0620;
DWORD IsItemDroppable = 0x5E06A0;
DWORD GetItemValue = 0x5E0800;
DWORD SetCCreateCameraAddr = 0x004950F0;
DWORD SelectCharacterAddr = 0x004F1A03;
typedef int(__fastcall *Activate_t)(CXWnd* thisptr);
typedef int(__fastcall *ValueSellMerchant_t)(DWORD* thisptr, float a2, float a3);
typedef int(__fastcall *SelectCharacter_t)(DWORD* thisptr, int a1, int a2, int a3);
typedef int(__fastcall *SetCCreateCamera_t)(DWORD thisptr);
Activate_t return_ActivateDet;
ValueSellMerchant_t return_ValueSellMerchantDet;
SetCCreateCamera_t return_SetCCreateCameraDet;
SelectCharacter_t return_SelectCharacterDet;
bool FirstSel = false;

DWORD currTime = 0;
bool SetTime = false;

std::map<std::string, LONGLONG> simpleFileList;

DWORD timeGetTimeVal = 0;

// CPUSpeed
const uint32_t CalcCpuTicks_x = 0x809820;  // GetCpuTicks1?
const uint32_t frequency_x = 0x15D3618; // g_i64CPUTicksPerMillisecond, used by GetCpuTicks2 (0x8097E0)
uint32_t  CalcCpuTicks = 0;
uint64_t* freq = nullptr;
uint64_t  orig = 0;
class CPUID {
	uint32_t regs[4];

public:
	explicit CPUID(unsigned i) {
		__cpuid((int*)regs, (int)i);
	}

	const uint32_t& EAX() const { return regs[0]; }
	const uint32_t& EBX() const { return regs[1]; }
	const uint32_t& ECX() const { return regs[2]; }
	const uint32_t& EDX() const { return regs[3]; }
};

void adjustFreq()
{
	LARGE_INTEGER li;
	if (freq && *freq && ::QueryPerformanceFrequency(&li))
	{
		DebugSpew("MQ2CpuSpeedFix adjusting CPU ticks per ms from %llu to %llu", *freq, li.QuadPart);
		orig = *freq;
		*freq = li.QuadPart;
	}
}

void __cdecl CalcCpuTicks_Trampoline(uint32_t, uint32_t);
void __cdecl CalcCpuTicks_Detour(uint32_t cpuSpeed2, uint32_t cpuSpeed3)
{
	CalcCpuTicks_Trampoline(cpuSpeed2, cpuSpeed3);
	adjustFreq();
}

DETOUR_TRAMPOLINE_EMPTY(void __cdecl CalcCpuTicks_Trampoline(uint32_t, uint32_t));



int __fastcall ValueSellMerchantHook(DWORD* thisptr, float a2, float a3)
{
	int retVal = return_ValueSellMerchantDet(thisptr, a2, a3);
	int isDroppable = ((int(__thiscall*) (LPVOID, int)) IsItemRentable) ((LPVOID)thisptr, 1);
	int isRentable = ((int(__thiscall*) (LPVOID, int)) IsItemDroppable) ((LPVOID)thisptr, 1);
	int itemValue = ((int(__thiscall*) (LPVOID, int)) GetItemValue) ((LPVOID)thisptr, 1);
	if (retVal <= 0 && isDroppable && isRentable && itemValue > 0)
		retVal = 1;
	return retVal;
}

bool IsEvil(int race, int class_, int deity)
{
	bool result = false; // eax in disasm

	if (race == 9 || race == 10 || race == 6 || race == 128)
		result = true;
	if (class_ == 11 || class_ == 5)
		result = true;
	if (deity == 1 || deity == 3 || deity == 4 || deity == 201 || deity == 203 || deity == 206)
		result = true;
	return result;
}

char __fastcall SetCCreateCameraHook(DWORD thisptr)
{
	if (pLocalPlayer && pDisplay && GetGameState() == GAMESTATE_CHARSELECT)
	{
		bool evil = IsEvil(pLocalPlayer->Data.Race, pLocalPlayer->Data.Class, pLocalPlayer->Data.Deity);

		pLocalPlayer->Data.X = 0;
		pLocalPlayer->Data.Y = 0;
		pLocalPlayer->Data.Z = 0;

		//if (evil)
		//{

		//	pLocalPlayer->Data.Z = -78.0f;
		//	pLocalPlayer->Data.X = -2466.0f;
		//	pLocalPlayer->Data.Y = -9;
		//	//pLocalPlayer->Data.Z = 12.75f;
		//	//pLocalPlayer->Data.Y = 5.0f;
		//	//pLocalPlayer->Data.X = -316.0f;
		//}
		//else
		//{
		//	//pLocalPlayer->Data.X = 0.0f;
		//	//pLocalPlayer->Data.Y = 0.0f;
		//	//pLocalPlayer->Data.Z = 10.0f;
		//}
		double result = 0.0f;
	}


	return return_SetCCreateCameraDet(thisptr);;
}

int fsize(FILE *fp) {

	int sz = 0;

	if (fp)
	{
		int prev = ftell(fp);
		fseek(fp, 0L, SEEK_END);
		sz = ftell(fp);
		fseek(fp, prev, SEEK_SET); //go back to where we were
	}
	return sz;
}
// 43C187 in Titanium


std::vector<std::string> splitpath(
	const std::string& str
	, const std::set<char> delimiters)
{
	std::vector<std::string> result;

	char const* pch = str.c_str();
	char const* start = pch;
	for (; *pch; ++pch)
	{
		if (delimiters.find(*pch) != delimiters.end())
		{
			if (start != pch)
			{
				std::string str(start, pch);
				result.push_back(str);
			}
			else
			{
				result.push_back("");
			}
			start = pch + 1;
		}
	}
	result.push_back(start);

	return result;
}

typedef HANDLE(__stdcall *CreateFileA_t)(LPCSTR s3dFile, DWORD a2, DWORD a3, LPSECURITY_ATTRIBUTES a4, DWORD a5, DWORD a6, HANDLE hTemplateFile);
CreateFileA_t return_CreateFileA;
HANDLE __stdcall /*CDisplay::*/CreateFileAHook(LPCSTR s3dFile, DWORD a2, DWORD a3, LPSECURITY_ATTRIBUTES a4, DWORD a5, DWORD a6, HANDLE hTemplateFile)
{
	std::string data = s3dFile;
	std::set<char> delims{ '/' };

	std::vector<std::string> path = splitpath(data, delims);

	std::string dataFile = path.back();
	std::transform(dataFile.begin(), dataFile.end(), dataFile.begin(), ::tolower);

	if (strlen(dataFile.c_str()) > 1 && strstr(dataFile.c_str(), "s3d") != 0)
	{

		HANDLE file = return_CreateFileA(s3dFile, GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
		if (file)
		{
			LARGE_INTEGER size;
			if (!GetFileSizeEx(file, &size))
			{
				CloseHandle(file);
				return return_CreateFileA(s3dFile, a2, a3, a4, a5, a6, hTemplateFile);
			}
			simpleFileList[dataFile] = size.QuadPart;
			CloseHandle(file);
		}
	}
	return return_CreateFileA(s3dFile, a2, a3, a4, a5, a6, hTemplateFile);
}


//4EECA0

int __fastcall CXWndActivateHook(CXWnd* thisptr)
{
	//if ((DWORD)thisptr->pvfTable == (DWORD)0x009de408) //AltAdv Again
	//{
	//	((int(__thiscall*) (LPVOID, bool, bool, bool)) 0x00865290) (thisptr, 0, 1, 1);
	//	return 0;
	//}

	/*if ((DWORD*)thisptr->pvfTable == (DWORD*)0x009ECDA8) //Leadership
	{
		((int(__thiscall*) (LPVOID, bool, bool, bool)) 0x00865290) (thisptr, 0, 1, 1);
		return 0;
	}*/

	//if ((DWORD*)thisptr->pvfTable == (DWORD*)0x009EE940) //Leadership
	//{
	//	((int(__thiscall*) (LPVOID, bool, bool, bool)) 0x00865290) (thisptr, 0, 1, 1);
	//	return 0;
	//}

	//if ((DWORD*)thisptr->pvfTable == (DWORD*)0x0064E9C0) //Adventure Window Stats
	//{
	//	((int(__thiscall*) (LPVOID, bool, bool)) 0x005A0A80) (thisptr, 0, 1);
	//	return 0;
	//}

	//if ((DWORD*)thisptr->pvfTable == (DWORD*)0x0064E878) //Adventure Request
	//{
	//	((int(__thiscall*) (LPVOID, bool, bool)) 0x005A0A80) (thisptr, 0, 1);
	//	return 0;
	//}

	//if ((DWORD*)thisptr->pvfTable == (DWORD*)0x00653698) //DZ Window
	//{
	//	((int(__thiscall*) (LPVOID, bool, bool)) 0x005A0A80) (thisptr, 0, 1);
	//	return 0;
	//}

	//if ((DWORD*)thisptr->pvfTable == (DWORD*)0x0065BEB8) //Raid Window
	//{
	//	((int(__thiscall*) (LPVOID, bool, bool)) 0x005A0A80) (thisptr, 0, 1);
	//	return 0;
	//}

	//if ((DWORD*)thisptr->pvfTable == (DWORD*)0x00655B68) //Guild Management
	//{
	//	((int(__thiscall*) (LPVOID, bool, bool)) 0x005A0A80) (thisptr, 0, 1);
	//	return 0;
	//}

	//if ((DWORD*)thisptr->pvfTable == (DWORD*)0x00654598) // Find Location Window
	//{
	//	((int(__thiscall*) (LPVOID, bool, bool)) 0x005A0A80) (thisptr, 0, 1);
	//	return 0;
	//}

	//if ((DWORD*)thisptr->pvfTable == (DWORD*)0x0065DE10) // Quest Journal
	//{
	//	((int(__thiscall*) (LPVOID, bool, bool)) 0x005A0A80) (thisptr, 0, 1);
	//	return 0;
	//}

	//if ((DWORD*)thisptr->pvfTable == (DWORD*)0x006538F0) // DZ Switch List
	//{
	//	((int(__thiscall*) (LPVOID, bool, bool)) 0x005A0A80) (thisptr, 0, 1);
	//	return 0;
	//}

	//if ((DWORD*)thisptr->pvfTable == (DWORD*)0x0065f5b0) // Journal
	//{
	//	((int(__thiscall*) (LPVOID, bool, bool)) 0x005A0A80) (thisptr, 0, 1);
	//	return 0;
	//}

	//if ((DWORD*)thisptr->pvfTable == (DWORD*)0x0064c220) // Tribute
	//{
	//	((int(__thiscall*) (LPVOID, bool, bool)) 0x005A0A80) (thisptr, 0, 1);
	//	return 0;
	//}

	//if ((DWORD*)thisptr->pvfTable == (DWORD*)0x00656f28) // Storyline
	//{
	//	((int(__thiscall*) (LPVOID, bool, bool)) 0x005A0A80) (thisptr, 0, 1);
	//	return 0;
	//}

	return return_ActivateDet(thisptr);
}

void __cdecl ResetMouseFlags() {
	DWORD ptr = *(DWORD *)0x00809DB4;
	if (ptr)
	{
		*(BYTE*)(ptr + 85) = 0;
		*(BYTE*)(ptr + 86) = 0;
		*(BYTE*)(ptr + 87) = 0;
		*(BYTE*)(ptr + 88) = 0;
	}

	*(DWORD*)0x00809320 = 0;
	*(DWORD*)0x0080931C = 0;
	*(DWORD*)0x00809324 = 0;
	*(DWORD*)0x00809328 = 0;
	*(DWORD*)0x0080932C = 0;
}

std::list<std::string> x86ProcessModuleList;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	char buffer[128];
	int written = GetWindowTextA(hwnd, buffer, 128);
	char wFileName[128];
	if (strlen(buffer) > 1)
	{
		std::string buf = buffer;

		if (buf.size() > 0)
		{
			x86ProcessModuleList.push_back("W:" + buf);
		}
	}

	return TRUE;
}

char bDetectedMQ2 = 0;

struct Checksum_Struct {
	uint16_t opcode;
	uint64_t checksum;
	char HDDSerial[512];
	char GPUString[256];
	char CPUBrandString[256];
	char CRC1[4];
	char CRC2[4];
	char CRC3[4];
	char CRC4[4];
	char CRC5[4];
	char DisplayW[4];
	char DisplayH[4];
	uint8_t  data[484];
};

struct SimpleChecksum_Struct {
	uint16_t opcode;
	uint64_t checksum;
	uint8_t  data[3];
};

struct ProcessListEntry_Struct {
	char zoneName[260];
};

struct MacEntry_Struct {
	uint16_t opcode;
	BYTE address[8];
};

struct ProcessList_Struct {
   uint16_t opcode;
   uint8_t count;
   ProcessListEntry_Struct process[200];
};

struct FileList_Struct {
	uint16_t opcode;
	int NumEntries;
    ProcessListEntry_Struct Keys[200];
    uint64_t Values[200];
};

enum Options {
	NothingFound = 0,
	MQ2MainDetected = 1 << 0,
	ShowEQServerDetected = 1 << 1,
	ShowEQClientDetected = 1 << 2,
	MQ2ProcessDetected = 1 << 3,
	KenetixDetected = 1 << 4,
	EQTrainerDetected = 1 << 5,
	MMOLoader = 1 << 6,
	ShowEQTitleBar = 1 << 7
};


__int64 FileSize(const char* name)
{
	struct _stat64 buf;
	if (_stat64(name, &buf) != 0)
		return -1; // error, could use errno to find out more

	return buf.st_size;
}

DETOUR_TRAMPOLINE_EMPTY(DWORD WINAPI GetModuleFileNameA_tramp(HMODULE, LPTSTR, DWORD));

DWORD WINAPI GetModuleFileNameA_detour(HMODULE hMod, LPTSTR outstring, DWORD nSize)
{
	DWORD allocsize = nSize;
	DWORD ret = GetModuleFileNameA_tramp(hMod, outstring, nSize);
	if (bExeChecksumrequested) {
		if (strstr(outstring, "eqgame.exe")) {
			bExeChecksumrequested = 0;
			PCHAR szProcessName = 0;
			szProcessName = strrchr(outstring, '\\');
			szProcessName[0] = '\0';
			sprintf_s(outstring, allocsize, "%s\\dinput8.dll", outstring);
		}
	}
	return ret;
}

DETOUR_TRAMPOLINE_EMPTY(unsigned char __cdecl SendExe_Tramp(DWORD));

unsigned char __cdecl SendExe_Tramp(DWORD);
unsigned char __cdecl SendExe_Detour(DWORD con)
{
	return SendExe_Tramp(con);
}

#pragma pack(push, 1)
struct ItemSerializationHeader
{
    char     unknown000[17];
    uint32_t stacksize;
    uint32_t unknown004;
    uint8_t  slot_type;
    uint16_t main_slot;
    uint16_t sub_slot;
    uint16_t aug_slot;
    uint32_t price;
    uint32_t merchant_slot;
    uint32_t scaled_value;
    uint32_t instance_id;
    uint32_t parcel_item_id;
    uint32_t last_cast_time;
    uint32_t charges;
    uint32_t inst_nodrop;
    uint32_t unknown044;
    uint32_t unknown048;
    uint32_t unknown052;
    uint8_t  isEvolving;
};
#pragma pack(pop)

typedef CHARINFO2* (__thiscall* EQ_Character_GetCharInfo2_t)(EQ_Character*);

// Cached server-reported HP values (from OP_HPUpdate) for the local player.
int g_serverCurHP = -1;
int g_serverMaxHP = -1;
// Cached server-reported Endurance values (from OP_EnduranceUpdate) for the local player.
int g_serverCurEnd = -1;
int g_serverMaxEnd = -1;
// Cached server-reported Mana values (from OP_ManaUpdate) for the local player.
int g_serverCurMana = -1;
int g_serverMaxMana = -1;
// Track which spawn/character the caches belong to so we can clear on character swap.
uint16_t g_localSpawnId = 0;

// Cached stats from the PlayerProfile packet (server authoritative).
struct ServerProfileCache {
    bool   has_profile = false;
    int    str = -1;
    int    sta = -1;
    int    agi = -1;
    int    dex = -1;
    int    intl = -1;
    int    wis = -1;
    int    cha = -1;
    int    hp_cur = -1;
    int    mana_cur = -1;
    int    mana_max = -1;
    int    end_cur = -1; // profile provides total; use it until we have a better source
    int    end_max = -1;
    uint32_t classes_mask = 0;
};

ServerProfileCache g_serverProfile;
static bool logged_profile_candidate = false;
static int packetCount = 0;

// EdgeStatLabel (opcode 0x1338) key/value stat cache
// NOTE: non-static so PowerSlotWnd.cpp (and future SIDL windows) can read the cache.
constexpr uint32_t kEdgeStatMaxKey = 4096;
uint64_t g_edgeStatValue[kEdgeStatMaxKey]{};
uint8_t  g_edgeStatHas[kEdgeStatMaxKey]{};

// Server-reported multiclass classes bitmask (from EdgeStatLabel key 200).
// The server sends this as an "item classes" bitmask: bit positions are (class_id - 1).
// This matches how EQ uses class bitmasks for item/spell usability (and the merchant "Show Usable Items" filter).
static uint32_t g_serverUsableClassesMask = 0;
static int g_last_logged_usable_classes_ret = INT_MIN;
static uint32_t g_charselectUsableClassesMask = 0;

static uint32_t GetEffectiveUsableClassesMask()
{
	const uint32_t mask = (g_serverUsableClassesMask & 0xFFFF);
	if (mask != 0) {
		return mask;
	}
	// THJ-style delivery uses PlayerProfile.classes; allow it as the primary fallback.
	const uint32_t profile_mask = (g_serverProfile.classes_mask & 0xFFFF);
	if (profile_mask != 0) {
		return profile_mask;
	}
	// Last-resort fallback: char-select multiclass mask observed from Deity override.
	return (g_charselectUsableClassesMask & 0xFFFF);
}

// Merchant UI helper state for multiclass spell display/filtering.
// These are intentionally conservative and time-bounded; they are only used to improve "Show Usable Items"
// behavior and avoid labeling multiclass-aggregated spells as the base class in merchant UI.
static volatile LONG g_merchant_spell_best_class = 0;
static volatile LONG g_merchant_spell_best_level = 0;
static volatile LONG g_merchant_spell_base_class = 0;
static volatile LONG g_merchant_spell_tick = 0;
static uint32_t g_last_client_probe_command = 0;
static uint32_t g_last_client_probe_nonce = 0;
static uint32_t g_last_client_probe_test_id = 0;

static void SendClientProbeReplyCommand(uint32_t test_id, uint32_t nonce)
{
	PSPAWNINFO me = reinterpret_cast<PSPAWNINFO>(pLocalPlayer);
	if (!me || !GetCharInfo()) {
		return;
	}

	const uint32_t spawn_id = me->SpawnID;
	uint32_t target_id = 0;
	if (pTarget) {
		auto* target_spawn = reinterpret_cast<PSPAWNINFO>(pTarget);
		if (target_spawn) {
			target_id = target_spawn->SpawnID;
		}
	}

	uint32_t effective_mask = (GetEffectiveUsableClassesMask() & 0xFFFF);
	int game_state = 0;
	__try {
		game_state = GetGameState();
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		game_state = 0;
	}

	const int hp_cur = (g_serverCurHP >= 0) ? g_serverCurHP : 0;
	const int hp_max = (g_serverMaxHP >= 0) ? g_serverMaxHP : 0;

	char cmd[256] = { 0 };
	_snprintf_s(
		cmd,
		sizeof(cmd),
		_TRUNCATE,
		"/say #test clientreply %u %u %d %u %u %u %d %d",
		test_id,
		nonce,
		game_state,
		spawn_id,
		target_id,
		effective_mask,
		hp_cur,
		hp_max
	);

	DoCommand(me, cmd);

	if (isDebugLoggingEnabled) {
		LogDebug(
			"[CLIENT_TEST_PROBE_REPLY] test_id=%u nonce=%u gs=%d spawn=%u target=%u mask=0x%04X hp=%d/%d",
			test_id,
			nonce,
			game_state,
			spawn_id,
			target_id,
			effective_mask,
			hp_cur,
			hp_max
		);
	}
}

static void SendClientProbeReplyCommandV2(uint32_t test_id, uint32_t nonce, uint32_t requested_mask, int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3)
{
	(void)arg0;
	(void)arg1;
	(void)arg2;
	(void)arg3;

	PSPAWNINFO me = reinterpret_cast<PSPAWNINFO>(pLocalPlayer);
	if (!me || !GetCharInfo()) {
		return;
	}

	constexpr uint32_t kProbeFieldGameState = 1u << 0;
	constexpr uint32_t kProbeFieldSpawnId = 1u << 1;
	constexpr uint32_t kProbeFieldTargetId = 1u << 2;
	constexpr uint32_t kProbeFieldHp = 1u << 3;
	constexpr uint32_t kProbeFieldMana = 1u << 4;
	constexpr uint32_t kProbeFieldEndurance = 1u << 5;
	constexpr uint32_t kProbeFieldClassMask = 1u << 6;

	uint32_t response_mask = 0;
	int game_state = 0;
	__try {
		game_state = GetGameState();
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		game_state = 0;
	}
	response_mask |= kProbeFieldGameState;

	const uint32_t spawn_id = me->SpawnID;
	response_mask |= kProbeFieldSpawnId;

	uint32_t target_id = 0;
	if (pTarget) {
		auto* target_spawn = reinterpret_cast<PSPAWNINFO>(pTarget);
		if (target_spawn) {
			target_id = target_spawn->SpawnID;
		}
	}
	response_mask |= kProbeFieldTargetId;

	const int hp_cur = (g_serverCurHP >= 0) ? g_serverCurHP : 0;
	const int hp_max = (g_serverMaxHP >= 0) ? g_serverMaxHP : 0;
	response_mask |= kProbeFieldHp;

	const int mana_cur = (g_serverCurMana >= 0) ? g_serverCurMana : 0;
	const int mana_max = (g_serverMaxMana >= 0) ? g_serverMaxMana : 0;
	response_mask |= kProbeFieldMana;

	const int end_cur = (g_serverCurEnd >= 0) ? g_serverCurEnd : 0;
	const int end_max = (g_serverMaxEnd >= 0) ? g_serverMaxEnd : 0;
	response_mask |= kProbeFieldEndurance;

	uint32_t class_mask = (GetEffectiveUsableClassesMask() & 0xFFFF);
	response_mask |= kProbeFieldClassMask;

	// Keep response lean: only emit requested fields + mandatory routing bits.
	const uint32_t output_mask = response_mask & (requested_mask | kProbeFieldGameState | kProbeFieldSpawnId);

	char cmd[320] = { 0 };
	_snprintf_s(
		cmd,
		sizeof(cmd),
		_TRUNCATE,
		"/say #test clientreplyv2 %u %u %u %d %u %u %d:%d %d:%d %d:%d %u",
		test_id,
		nonce,
		output_mask,
		game_state,
		spawn_id,
		target_id,
		hp_cur,
		hp_max,
		mana_cur,
		mana_max,
		end_cur,
		end_max,
		class_mask
	);

	DoCommand(me, cmd);

	if (isDebugLoggingEnabled) {
		LogDebug(
			"[CLIENT_TEST_PROBE_REPLY_V2] test_id=%u nonce=%u req=0x%04X out=0x%04X gs=%d spawn=%u target=%u hp=%d/%d mana=%d/%d end=%d/%d class=0x%04X",
			test_id,
			nonce,
			requested_mask & 0xFFFF,
			output_mask & 0xFFFF,
			game_state,
			spawn_id,
			target_id,
			hp_cur,
			hp_max,
			mana_cur,
			mana_max,
			end_cur,
			end_max,
			class_mask & 0xFFFF
		);
	}
}

static void ApplyEdgeStatLabelPacket(const char* buf, size_t size)
{
	// Payload:
	//   uint32 count
	//   repeated count times: { uint32 key; uint64 value; }
	if (!buf || size < sizeof(uint32_t)) {
		return;
	}

	uint32_t count = 0;
	memcpy(&count, buf, sizeof(uint32_t));
	if (count > 2000) {
		return;
	}

	size_t off = sizeof(uint32_t);
	for (uint32_t i = 0; i < count; ++i) {
		if (off + sizeof(uint32_t) + sizeof(uint64_t) > size) {
			break;
		}

		uint32_t key = 0;
		uint64_t value = 0;
		memcpy(&key, buf + off, sizeof(uint32_t));
		memcpy(&value, buf + off + sizeof(uint32_t), sizeof(uint64_t));
		off += sizeof(uint32_t) + sizeof(uint64_t);

		if (key < kEdgeStatMaxKey) {
			g_edgeStatValue[key] = value;
			g_edgeStatHas[key] = 1;
		}
	}

	// Keep IDs aligned with extras/classless-dll-main/eqgame_dll/MQ2Labels.cpp (eStatEntry).
	constexpr uint32_t kCurHP   = 2;
	constexpr uint32_t kCurMana = 3;
	constexpr uint32_t kCurEnd  = 4;
	constexpr uint32_t kMaxHP   = 5;
	constexpr uint32_t kMaxMana = 6;
	constexpr uint32_t kMaxEnd  = 7;
	constexpr uint32_t kSTR     = 24;
	constexpr uint32_t kSTA     = 25;
	constexpr uint32_t kDEX     = 26;
	constexpr uint32_t kAGI     = 27;
	constexpr uint32_t kINT     = 28;
	constexpr uint32_t kWIS     = 29;
	constexpr uint32_t kCHA     = 30;
	// Custom keys (server-side: zone/client.cpp::SendEdgeStats)
	constexpr uint32_t kClassesBitmask = 200;
	constexpr uint32_t kTestProbeCommand = 900;
	constexpr uint32_t kTestProbeId      = 901;
	constexpr uint32_t kTestProbeNonce   = 902;
	constexpr uint32_t kTestProbeVersion = 903;
	constexpr uint32_t kTestProbeMask    = 904;
	constexpr uint32_t kTestProbeArg0    = 905;
	constexpr uint32_t kTestProbeArg1    = 906;
	constexpr uint32_t kTestProbeArg2    = 907;
	constexpr uint32_t kTestProbeArg3    = 908;

	auto get_i32 = [](uint32_t key) -> int {
		if (key >= kEdgeStatMaxKey || !g_edgeStatHas[key]) {
			return -1;
		}
		const uint64_t v = g_edgeStatValue[key];
		if (v >= static_cast<uint64_t>(INT_MAX - 1)) {
			return INT_MAX - 1;
		}
		return static_cast<int>(v);
	};

	auto get_u32 = [](uint32_t key) -> uint32_t {
		if (key >= kEdgeStatMaxKey || !g_edgeStatHas[key]) {
			return 0;
		}
		return static_cast<uint32_t>(g_edgeStatValue[key] & 0xFFFFFFFFu);
	};

	const int cur_hp = get_i32(kCurHP);
	const int max_hp = get_i32(kMaxHP);
	if (cur_hp >= 0) g_serverCurHP = cur_hp;
	if (max_hp >= 0) g_serverMaxHP = max_hp;

	const int cur_mana = get_i32(kCurMana);
	const int max_mana = get_i32(kMaxMana);
	if (cur_mana >= 0) g_serverCurMana = cur_mana;
	if (max_mana >= 0) g_serverMaxMana = max_mana;

	const int cur_end = get_i32(kCurEnd);
	const int max_end = get_i32(kMaxEnd);
	if (cur_end >= 0) g_serverCurEnd = cur_end;
	if (max_end >= 0) g_serverMaxEnd = max_end;

	// Seed attribute cache for future label/tooltip overrides.
	g_serverProfile.has_profile = true;
	const int str = get_i32(kSTR);
	const int sta = get_i32(kSTA);
	const int dex = get_i32(kDEX);
	const int agi = get_i32(kAGI);
	const int intl = get_i32(kINT);
	const int wis = get_i32(kWIS);
	const int cha = get_i32(kCHA);
	if (str >= 0) g_serverProfile.str = str;
	if (sta >= 0) g_serverProfile.sta = sta;
	if (dex >= 0) g_serverProfile.dex = dex;
	if (agi >= 0) g_serverProfile.agi = agi;
	if (intl >= 0) g_serverProfile.intl = intl;
	if (wis >= 0) g_serverProfile.wis = wis;
	if (cha >= 0) g_serverProfile.cha = cha;

		// Multiclass classes bitmask used for client-side filtering (AA window, usability, etc).
	if (kClassesBitmask < kEdgeStatMaxKey && g_edgeStatHas[kClassesBitmask]) {
		const uint32_t prev = g_serverUsableClassesMask;
		g_serverUsableClassesMask = static_cast<uint32_t>(g_edgeStatValue[kClassesBitmask] & 0xFFFF);
		if (isDebugLoggingEnabled && prev != g_serverUsableClassesMask) {
			LogDebug(
				"EDGE_STAT multiclass classes_bitmask=0x%04X (%u)",
				g_serverUsableClassesMask,
				g_serverUsableClassesMask
			);
		}
	}

	const uint32_t probe_command = get_u32(kTestProbeCommand);
	const uint32_t probe_test_id = get_u32(kTestProbeId);
	const uint32_t probe_nonce = get_u32(kTestProbeNonce);
	const uint32_t probe_version = get_u32(kTestProbeVersion);
	const uint32_t probe_mask = get_u32(kTestProbeMask);
	const int32_t probe_arg0 = static_cast<int32_t>(get_u32(kTestProbeArg0));
	const int32_t probe_arg1 = static_cast<int32_t>(get_u32(kTestProbeArg1));
	const int32_t probe_arg2 = static_cast<int32_t>(get_u32(kTestProbeArg2));
	const int32_t probe_arg3 = static_cast<int32_t>(get_u32(kTestProbeArg3));
	if ((probe_command == 1 || probe_command == 2) && probe_test_id > 0 && probe_nonce > 0) {
		if (probe_nonce != g_last_client_probe_nonce || probe_test_id != g_last_client_probe_test_id || probe_command != g_last_client_probe_command) {
			g_last_client_probe_command = probe_command;
			g_last_client_probe_nonce = probe_nonce;
			g_last_client_probe_test_id = probe_test_id;
			if (probe_command == 1) {
				SendClientProbeReplyCommand(probe_test_id, probe_nonce);
			} else {
				(void)probe_version;
				SendClientProbeReplyCommandV2(probe_test_id, probe_nonce, probe_mask, probe_arg0, probe_arg1, probe_arg2, probe_arg3);
			}
		}
	}
}

// Recent packet circular buffer to assist debugging crashes/zoning sequences
struct RecentPacket {
	uint16_t opcode;
	size_t size;
	// head/tail hold hex previews (2 chars per byte) plus terminating NUL
	char head[33];
	char tail[33];
	time_t ts;
};
static const size_t kRecentPacketBuf = 256;
static RecentPacket g_recent_packets[kRecentPacketBuf];
static size_t g_recent_idx = 0;
static bool g_verbose_logs = false; // enable by dropping a file or toggle in code

static time_t g_last_recent_dump_ts = 0;
static uint16_t g_last_recent_dump_spawn = 0;

static void AddRecentPacket(unsigned opcode, const char* buf, size_t size) {
	RecentPacket &r = g_recent_packets[g_recent_idx % kRecentPacketBuf];
	r.opcode = static_cast<uint16_t>(opcode & 0xFFFF);
	r.size = size;
	r.ts = std::time(nullptr);
	// head
	size_t h = size < 16 ? size : 16;
	for (size_t i = 0; i < h; ++i) {
		unsigned char c = static_cast<unsigned char>(buf[i]);
		sprintf(r.head + (i * 2), "%02X", c);
	}
	r.head[h*2] = '\0';
	// tail
	size_t t = size < 16 ? 0 : (size - 16);
	size_t tail_len = (size < 16) ? size : 16;
	for (size_t i = 0; i < tail_len; ++i) {
		unsigned char c = static_cast<unsigned char>(buf[t + i]);
		sprintf(r.tail + (i * 2), "%02X", c);
	}
	r.tail[tail_len*2] = '\0';
	++g_recent_idx;
}

static void DumpRecentPackets(FILE* f, size_t count = 20) {
	if (!f) return;
	size_t available = (g_recent_idx < kRecentPacketBuf) ? g_recent_idx : kRecentPacketBuf;
	size_t to_dump = count < available ? count : available;
	size_t start = (g_recent_idx >= to_dump) ? (g_recent_idx - to_dump) : 0;
	for (size_t i = 0; i < to_dump; ++i) {
		size_t idx = (start + i) % kRecentPacketBuf;
		RecentPacket &r = g_recent_packets[idx];
		char tb[64] = {0};
		struct tm *tmv = localtime(&r.ts);
		strftime(tb, sizeof(tb), "%Y-%m-%d %H:%M:%S", tmv);
		fprintf(f, "[%s] RECENT opcode=0x%04x size=%zu head=%s tail=%s\n", tb, r.opcode, r.size, r.head, r.tail);
	}
}

static void MaybeDumpRecentPacketsToLog(const char* header, uint16_t spawn_id)
{
	if (!isDebugLoggingEnabled || !isRecentPacketTraceEnabled) {
		return;
	}

	const time_t now = std::time(nullptr);
	// Throttle to avoid client lockups from excessive disk IO if spawn-id detection flaps.
	if (spawn_id == g_last_recent_dump_spawn && (now - g_last_recent_dump_ts) < 30) {
		return;
	}
	if ((now - g_last_recent_dump_ts) < 5) {
		return;
	}

	g_last_recent_dump_ts = now;
	g_last_recent_dump_spawn = spawn_id;

	FILE* rf = nullptr;
	if (fopen_s(&rf, "dinput8_debug.log", "a") == 0 && rf) {
		fprintf(rf, "%s spawn=%u recent packet dump:\n", header ? header : "[RECENT_DUMP]", (unsigned)spawn_id);
		DumpRecentPackets(rf, 40);
		fclose(rf);
	}
}

static void ResetServerCaches()
{
    g_serverCurHP   = -1;
    g_serverMaxHP   = -1;
    g_serverCurEnd  = -1;
    g_serverMaxEnd  = -1;
    g_serverCurMana = -1;
    g_serverMaxMana = -1;
    g_serverProfile = ServerProfileCache{};
	g_serverUsableClassesMask = 0;
	g_charselectUsableClassesMask = 0;
	g_last_recent_dump_ts = 0;
	g_last_recent_dump_spawn = 0;
    logged_profile_candidate = false;
	memset(g_edgeStatHas, 0, sizeof(g_edgeStatHas));
	memset(g_edgeStatValue, 0, sizeof(g_edgeStatValue));
}

// --- Write-watch for local-player stat writes (VEH) ---
static PVOID g_stat_watch_handler = nullptr;
static bool g_stat_watch_active = false;
static volatile LONG g_stat_watch_timer_started = 0;

static void*  g_stat_watch_pages[4] = { nullptr, nullptr, nullptr, nullptr };
static size_t g_stat_watch_page_sizes[4] = { 0, 0, 0, 0 };
static DWORD  g_stat_watch_old_protect[4] = { 0, 0, 0, 0 };
static int    g_stat_watch_page_count = 0;

static void DisableStatWriteWatch();

static bool IsWriteWatchExpectedAV(DWORD code, ULONG_PTR info0, ULONG_PTR info1)
{
	if (code != EXCEPTION_ACCESS_VIOLATION) {
		return false;
	}
	// 1 == write access
	if (info0 != 1 || !g_stat_watch_active) {
		return false;
	}

	uintptr_t target = (uintptr_t)info1;
	for (int i = 0; i < 4; ++i) {
		uintptr_t page_base = (uintptr_t)g_stat_watch_pages[i];
		size_t page_size = g_stat_watch_page_sizes[i];
		if (!page_base || page_size == 0) continue;
		if (target >= page_base && target < page_base + page_size) {
			return true;
		}
	}
	return false;
}

static LONG CALLBACK StatWriteWatchHandler(PEXCEPTION_POINTERS ep)
{
	if (!ep || !ep->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;
	DWORD code = (DWORD)ep->ExceptionRecord->ExceptionCode;
	// Access violation: ExceptionInformation[0] indicates type: 0=read, 1=write, 8=execute.
	if (code == EXCEPTION_ACCESS_VIOLATION && ep->ExceptionRecord->NumberParameters >= 2) {
		const ULONG_PTR access_type = ep->ExceptionRecord->ExceptionInformation[0];
		const uintptr_t fault_addr = (uintptr_t)ep->ExceptionRecord->ExceptionInformation[1];
		if (g_stat_watch_active && access_type == 1) {
			for (int i = 0; i < 4; ++i) {
				uintptr_t page_base = (uintptr_t)g_stat_watch_pages[i];
				size_t page_size = g_stat_watch_page_sizes[i];
				if (!page_base || page_size == 0) continue;
				if (fault_addr < page_base || fault_addr >= page_base + page_size) continue;

				// Log the faulting IP and target address (x86 uses Eip)
#ifdef _M_X64
				void* ip = (void*)ep->ContextRecord->Rip;
#else
				void* ip = (void*)ep->ContextRecord->Eip;
#endif
				LogDebug("[WRITE_WATCH_FAULT] ip=%p target=%p page=%p", ip, (void*)fault_addr, (void*)page_base);

				// Restore the original page protection so the instruction can complete.
				DWORD tmp = 0;
				DWORD restore = g_stat_watch_old_protect[i] ? g_stat_watch_old_protect[i] : PAGE_READWRITE;
				if (VirtualProtect((LPVOID)page_base, page_size, restore, &tmp)) {
					return EXCEPTION_CONTINUE_EXECUTION;
				} else {
					DisableStatWriteWatch();
					return EXCEPTION_CONTINUE_SEARCH;
				}
			}
		}
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

static void EnableStatWriteWatchForSpawn(void* spawn_ptr)
{
	if (!isStatWriteWatchEnabled) {
		return;
	}
	if (!spawn_ptr) return;
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	size_t page = si.dwPageSize;
	uintptr_t addr = (uintptr_t)spawn_ptr;
	uintptr_t page_base = addr & ~(page - 1);

	// Avoid re-adding a page we already watch.
	for (int i = 0; i < 4; ++i) {
		if (g_stat_watch_pages[i] == (void*)page_base) {
			return;
		}
	}

	// Find a free slot; if none, disable existing watch and reuse slot 0.
	int slot = -1;
	for (int i = 0; i < 4; ++i) {
		if (!g_stat_watch_pages[i]) { slot = i; break; }
	}
	if (slot == -1) {
		DisableStatWriteWatch();
		slot = 0;
	}

	// Protect page as read-only so reads continue normally; first write triggers VEH.
	DWORD old = 0;
	if (VirtualProtect((LPVOID)page_base, page, PAGE_READONLY, &old)) {
		if (!g_stat_watch_handler) {
			g_stat_watch_handler = AddVectoredExceptionHandler(1, StatWriteWatchHandler);
		}
		g_stat_watch_pages[slot] = (void*)page_base;
		g_stat_watch_page_sizes[slot] = page;
		g_stat_watch_old_protect[slot] = old;
		g_stat_watch_page_count++;
		g_stat_watch_active = true;
		LogDebug("[WRITE_WATCH_ENABLE] page=%p size=%zu old=0x%08X", (void*)page_base, page, (unsigned)old);
		// Start a short-lived timer to automatically disable the watch after 15 seconds
		if (InterlockedCompareExchange(&g_stat_watch_timer_started, 1, 0) == 0) {
			std::thread([]() {
				std::this_thread::sleep_for(std::chrono::seconds(15));
				DisableStatWriteWatch();
			}).detach();
		}
	} else {
		LogDebug("[WRITE_WATCH_ENABLE_FAIL] could not protect page=%p", (void*)page_base);
		g_stat_watch_active = false;
	}
}

static void DisableStatWriteWatch()
{
	for (int i = 0; i < 4; ++i) {
		if (g_stat_watch_pages[i] && g_stat_watch_page_sizes[i]) {
			DWORD tmp = 0;
			DWORD restore = g_stat_watch_old_protect[i] ? g_stat_watch_old_protect[i] : PAGE_READWRITE;
			VirtualProtect(g_stat_watch_pages[i], g_stat_watch_page_sizes[i], restore, &tmp);
		}
		g_stat_watch_pages[i] = nullptr;
		g_stat_watch_page_sizes[i] = 0;
		g_stat_watch_old_protect[i] = 0;
	}
	if (g_stat_watch_handler) {
		RemoveVectoredExceptionHandler(g_stat_watch_handler);
		g_stat_watch_handler = nullptr;
	}
	g_stat_watch_active = false;
	g_stat_watch_page_count = 0;
	InterlockedExchange(&g_stat_watch_timer_started, 0);
	LogDebug("[WRITE_WATCH_DISABLE]");
}

static bool ShouldLogSpellLevelNeeded()
{
	static time_t s_last_ts = 0;
	static int s_burst = 0;

	const time_t now = std::time(nullptr);
	if (now != s_last_ts) {
		s_last_ts = now;
		s_burst = 0;
	}

	// Hard cap noisy UI paths (spell merchant lists can call this thousands of times).
	return (s_burst++ < 10);
}

static void LogPacket(const char* tag, unsigned opcode, size_t size)
{
    FILE* f = nullptr;
    if (fopen_s(&f, "dinput8_debug.log", "a") == 0 && f) {
		fprintf(f, "%s opcode=0x%04x size=%zu\n", tag, opcode & 0xFFFF, size);
        fclose(f);
    }
}

// prev_max: the previous cached max value for this stat (or -1 if unknown)
// prev_max: the previous cached max value for this stat (or -1 if unknown)
// src: human-readable source of the packet (e.g., "HPUpdate", "ServerStatsUpdate")
static void LogPacketDetail(const char* tag, unsigned opcode, size_t size, uint32_t cur, int32_t max, uint16_t spawn, int32_t prev_max = -1, const char* src = "")
{
    FILE* f = nullptr;
	// Only log endurance/mana packets when the max value changes (to reduce noise).
	// Always log HP packets.
	bool is_mana = strstr(tag, "MANA") != nullptr;
	bool is_end = strstr(tag, "END") != nullptr;
	if ((is_mana || is_end) && prev_max != -1 && prev_max == max) {
		return; // no meaningful max change, skip noisy logging
	}
	if (fopen_s(&f, "dinput8_debug.log", "a") == 0 && f) {
		if (src && src[0]) {
			fprintf(f, "%s opcode=0x%04x size=%zu cur=%u max=%d spawn=%u src=%s\n", tag, opcode & 0xFFFF, size, cur, max, spawn, src);
		} else {
			fprintf(f, "%s opcode=0x%04x size=%zu cur=%u max=%d spawn=%u\n", tag, opcode & 0xFFFF, size, cur, max, spawn);
		}
		fclose(f);
	}
}

static uint32_t ReadUInt32Safe(const char* buf, size_t size, size_t offset)
{
    if (offset + 4 > size) return 0;
    uint32_t v;
    memcpy(&v, buf + offset, 4);
    return v;
}

	unsigned char __fastcall HandleWorldMessage_Trampoline(DWORD *con, DWORD edx, unsigned __int32 unk, unsigned __int32 opcode, char* buf, size_t size);
unsigned char __fastcall HandleWorldMessage_Detour(DWORD *con, DWORD edx, unsigned __int32 unk, unsigned __int32 opcode, char* buf, size_t size)
{
    try {
        packetCount++;
		g_last_hook_tag = Hook_HandleWorldMessage;
		if (isRecentPacketTraceEnabled) {
			AddRecentPacket(opcode, buf, size);
		}
        // If we swapped characters/spawn, clear cached values so we don't leak old stats to the new toon.
				if (GetCharInfo() && GetCharInfo()->pSpawn) {
            PSPAWNINFO me = reinterpret_cast<PSPAWNINFO>(GetCharInfo()->pSpawn);
            if (me && me->SpawnID != 0 && me->SpawnID != g_localSpawnId) {
                // Only blow away caches if this is an actual toon swap (not the initial 0 -> real spawn id transition).
                if (g_localSpawnId != 0) {
					LogDebug("[CACHE_RESET] new spawn id %u (old %u), clearing server caches", me->SpawnID, g_localSpawnId);
					// Log a short summary of last-known server stats before clearing so we have context.
					LogDebug("[CACHE_RESET_SUMMARY] last HP=%d/%d Mana=%d/%d End=%d/%d",
						g_serverCurHP, g_serverMaxHP, g_serverCurMana, g_serverMaxMana, g_serverCurEnd, g_serverMaxEnd);
					ResetServerCaches();
					MaybeDumpRecentPacketsToLog("[CACHE_RESET_RECENT]", me->SpawnID);
                } else {
                    LogDebug("[CACHE_RESET] new spawn id %u (old %u), keeping profile cache", me->SpawnID, g_localSpawnId);
					MaybeDumpRecentPacketsToLog("[CACHE_RESET_RECENT_INITIAL]", me->SpawnID);
                }
				g_localSpawnId = me->SpawnID;

				// Optional write-watch diagnostics (disabled by default; see `_options.h`).
				if (isStatWriteWatchEnabled) {
					EnableStatWriteWatchForSpawn((void*)me);
					if (GetCharInfo()) {
						EnableStatWriteWatchForSpawn((void*)GetCharInfo());
					}
				}
				// Also log a concise structured client-side spawn parse line for correlation
				{
					FILE* sf3 = nullptr;
					const char* repo_stats_path = "C:\\Users\\marsh\\OneDrive\\Documents\\GitHub\\EQ_Server\\logs\\stats_debug.log";
					if (fopen_s(&sf3, repo_stats_path, "a") == 0 && sf3) {
						time_t now3 = time(nullptr);
						struct tm *tmv3 = localtime(&now3);
						char tb3[32] = {0};
						strftime(tb3, sizeof(tb3), "%Y%m%d_%H%M%S", tmv3);
						const char* nm = (me && me->Name) ? me->Name : "(nil)";
						int curhp = 0;
						int maxhp = 0;
						// PSPAWNINFO exposes HPCurrent/HPMax
						curhp = (me) ? me->HPCurrent : 0;
						maxhp = (me) ? me->HPMax : 0;
						fprintf(sf3, "%s CLIENT_SPAWN_PARSED spawn=%u name=%s cur=%d max=%d\n", tb3, g_localSpawnId, nm, curhp, maxhp);
						fclose(sf3);
					}
				}
            }
        }
		// NOTE: packet logging temporarily disabled to avoid interfering with
		// client stability while we diagnose a crash that can occur on certain
		// incoming packets. These logs can be re-enabled once the crash is
		// resolved.

		// Conditional on-demand dumps for specific opcodes useful for diagnosing zoning stat changes.
		// Create the toggle files under repo `logs/` to enable without rebuilding:
		//  - enable_dump_1338  (EdgeStatLabelPacket)
		//  - enable_dump_575b  (suspected large item/HME packet)
		{
			uint16_t lop = opcode & 0xFFFF;
			if (lop == 0x1338) {
				ApplyEdgeStatLabelPacket(buf, size);
				ConditionalDumpOpcode(lop, buf, size, "enable_dump_1338");
				// Also write a readable, parsed EdgeStat log when toggled
				ConditionalLogEdgeStat(buf, size, "enable_log_1338");
			}
			if (lop == 0x575b) {
				ConditionalDumpOpcode(lop, buf, size, "enable_dump_575b");
			}
		}

        // Parse PlayerProfile for stats snapshot and multiclass classes bitmask.
        // THJ-style servers deliver multiclass through PlayerProfile.classes.
        const uint16_t player_profile_opcode = 0x6506; // OP_PlayerProfile (RoF2)
		if (!logged_profile_candidate && ((opcode & 0xFFFF) == player_profile_opcode || size >= 19000) && size > 9000) {
            logged_profile_candidate = true;
            // Reset per-profile caches so we don't carry over stale values between characters.
            g_serverProfile = ServerProfileCache{};
			// Read a set of offsets from the PlayerProfile; these offsets have historically varied by client
			// build/version. We guard reads and log the offsets we used to aid diagnosing mismatches.
			uint32_t cur_hp   = ReadUInt32Safe(buf, size, 948);
			uint32_t mana     = ReadUInt32Safe(buf, size, 944);
			uint32_t str      = ReadUInt32Safe(buf, size, 952);
			uint32_t sta      = ReadUInt32Safe(buf, size, 956);
			// INT and CHA offsets were swapped in earlier parsing; fix the order to match packet layout.
			uint32_t intl     = ReadUInt32Safe(buf, size, 960);
			uint32_t dex      = ReadUInt32Safe(buf, size, 964);
			uint32_t cha      = ReadUInt32Safe(buf, size, 968);
			uint32_t agi      = ReadUInt32Safe(buf, size, 972);
			uint32_t wis      = ReadUInt32Safe(buf, size, 976);
			uint32_t end_tot  = (size > 13760) ? ReadUInt32Safe(buf, size, 13756) : 0;
			uint32_t mana_tot = (size > 13764) ? ReadUInt32Safe(buf, size, 13760) : 0;

			// THJ parity: PlayerProfile.classes is adjacent to RestTimer in this profile region.
			// In our current struct, classes is at 19564 and char_id at 19568.
			// Keep a compatibility fallback for older assumptions.
			uint32_t classes_mask_19564 = (size >= 19568) ? ReadUInt32Safe(buf, size, 19564) : 0;
			uint32_t classes_mask_19568 = (size >= 19572) ? ReadUInt32Safe(buf, size, 19568) : 0;
			uint32_t classes_mask = 0;
			if (classes_mask_19564 != 0 && classes_mask_19564 <= 0xFFFF) {
				classes_mask = classes_mask_19564;
			} else if (classes_mask_19568 != 0 && classes_mask_19568 <= 0xFFFF) {
				classes_mask = classes_mask_19568;
			}

			// Basic sanity: if mana_tot is absurd (e.g., garbage from struct mismatch), zero it.
			if (mana_tot > 100000000) {
				LogDebug("[PLAYER_PROFILE_WARN] large mana_tot=%u; probable struct mismatch (size=%zu)", mana_tot, size);
				mana_tot = 0;
			}

			// If mana_tot is zero but other fields look okay, log an extra warning so zoning/profile parse issues stand out.
			if (mana_tot == 0 && mana > 0) {
				LogDebug("[PLAYER_PROFILE_WARN] mana_tot==0 while mana=%u (profile size=%zu). Offsets might be wrong.", mana, size);
			}

            g_serverProfile.has_profile = true;
            g_serverProfile.hp_cur   = static_cast<int>(cur_hp);
            g_serverProfile.mana_cur = static_cast<int>(mana);
            g_serverProfile.mana_max = static_cast<int>(mana_tot);
            g_serverProfile.str      = static_cast<int>(str);
            g_serverProfile.sta      = static_cast<int>(sta);
            g_serverProfile.cha      = static_cast<int>(cha);
            g_serverProfile.dex      = static_cast<int>(dex);
            g_serverProfile.intl     = static_cast<int>(intl);
            g_serverProfile.agi      = static_cast<int>(agi);
            g_serverProfile.wis      = static_cast<int>(wis);
            g_serverProfile.end_cur  = static_cast<int>(end_tot);
            g_serverProfile.end_max  = static_cast<int>(end_tot);
            g_serverProfile.classes_mask = classes_mask;

            // If the profile contains a multiclass mask, update the global hook state immediately.
            if (classes_mask != 0) {
                g_serverUsableClassesMask = classes_mask;
                g_last_logged_usable_classes_ret = INT_MIN;
				LogDebug("[PLAYER_PROFILE_MASK] opcode=0x%04x size=%zu classes_mask=0x%04X (off19564=0x%08X off19568=0x%08X)",
					opcode & 0xFFFF, size, classes_mask & 0xFFFF, classes_mask_19564, classes_mask_19568);
			} else {
				LogDebug("[PLAYER_PROFILE_MASK_WARN] opcode=0x%04x size=%zu classes_mask unresolved (off19564=0x%08X off19568=0x%08X)",
					opcode & 0xFFFF, size, classes_mask_19564, classes_mask_19568);
            }

            // Seed live caches from the profile so the UI doesn't show stale HP/Mana/End while waiting for updates.
            g_serverCurHP   = static_cast<int>(cur_hp);
            g_serverMaxHP   = static_cast<int>(cur_hp);    // profile max HP is not present; use cur as best-effort
            g_serverCurMana = static_cast<int>(mana);
            g_serverMaxMana = static_cast<int>(mana_tot);
            g_serverCurEnd  = static_cast<int>(end_tot);
            g_serverMaxEnd  = static_cast<int>(end_tot);

            FILE* f = nullptr;
            if (fopen_s(&f, "dinput8_debug.log", "a") == 0 && f) {
                fprintf(f,
                    "[PROFILE_CACHE] opcode=0x%04x size=%zu HP=%u mana=%u (tot=%u) end=%u STR=%u STA=%u AGI=%u DEX=%u INT=%u WIS=%u CHA=%u\n",
                    opcode & 0xFFFF, size,
                    cur_hp, mana, mana_tot, end_tot,
                    str, sta, agi, dex, intl, wis, cha);

				// Also log a small hex preview (head/tail) to help diagnose offset mismatches without dumping whole packet.
				size_t head_len = size < 64 ? size : 64;
				size_t tail_len = size < 64 ? 0 : 64;
				char head_hex[256] = {0};
				char tail_hex[256] = {0};
				for (size_t i = 0; i < head_len; ++i) {
					unsigned char c = static_cast<unsigned char>(buf[i]);
					sprintf(head_hex + (i * 2), "%02X", c);
				}
				if (tail_len) {
					size_t t = size - tail_len;
					for (size_t i = 0; i < tail_len; ++i) {
						unsigned char c = static_cast<unsigned char>(buf[t + i]);
						sprintf(tail_hex + (i * 2), "%02X", c);
					}
				}
				fprintf(f, "[PROFILE_RAW] size=%zu head=%s tail=%s\n", size, head_hex, tail_hex);

				// If packet is reasonably sized, write full binary dump for offline analysis.
				const size_t kMaxFullDump = 65536; // 64KB
				if (size > 0 && size <= kMaxFullDump) {
					char dumpname[128];
					time_t now = std::time(nullptr);
					struct tm *tmv = localtime(&now);
					char tb[32] = {0};
					strftime(tb, sizeof(tb), "%Y%m%d_%H%M%S", tmv);
					snprintf(dumpname, sizeof(dumpname), "profile_dump_%s_%04x.bin", tb, opcode & 0xFFFF);
					FILE* df = nullptr;
					if (fopen_s(&df, dumpname, "wb") == 0 && df) {
						fwrite(buf, 1, size, df);
						fclose(df);
						fprintf(f, "[PROFILE_DUMP] wrote %s (%zu bytes)\n", dumpname, size);
					}
				} else {
					fprintf(f, "[PROFILE_DUMP] skipped full dump (size=%zu > %zu)\n", size, kMaxFullDump);
				}
                fclose(f);
            }
        }
	} catch (...) {
		// Log unexpected exception while parsing packet headers
		LogDebug("[EXCEPTION] exception in packet header parsing opcode=0x%04x size=%zu", opcode & 0xFFFF, size);
	}

    // Capture server HP / Endurance / Mana updates for the local player so the label hook can use server-authoritative values.
    // OP_HPUpdate / OP_EnduranceUpdate / OP_ManaUpdate payloads are 10 bytes: uint32 cur, int32 max, uint16 spawn_id.
    const uint16_t hp_opcode         = 0x2828; // OP_HPUpdate (RoF2)
    const uint16_t end_opcode        = 0x5f42; // OP_EnduranceUpdate (RoF2)
    const uint16_t mob_end_opcode    = 0x1c81; // OP_MobEnduranceUpdate (RoF2)
    const uint16_t mana_opcode       = 0x3791; // OP_ManaUpdate (RoF2)
    const uint16_t mob_mana_opcode   = 0x2404; // OP_MobManaUpdate (RoF2)
    const uint16_t stats_opcode      = 0x7330; // Custom OP_ServerStatsUpdate
    uint16_t op = opcode & 0xFFFF;

    // Full stats packet (authoritative for stats/HME)
    if (op == stats_opcode && size >= 56) {
#pragma pack(push,1)
        struct ServerStatsUpdatePayload {
            uint16_t spawn_id;
            uint16_t padding;
            int32_t  str;
            int32_t  sta;
            int32_t  agi;
            int32_t  dex;
            int32_t  intl;
            int32_t  wis;
            int32_t  cha;
            int32_t  cur_hp;
            int32_t  max_hp;
            int32_t  cur_mana;
            int32_t  max_mana;
            int32_t  cur_end;
            int32_t  max_end;
        };
        #pragma pack(pop)
        const auto* s = reinterpret_cast<const ServerStatsUpdatePayload*>(buf);
        PSPAWNINFO me = nullptr;
        if (GetCharInfo()) {
            me = reinterpret_cast<PSPAWNINFO>(GetCharInfo()->pSpawn);
        }
		if (me && s->spawn_id == me->SpawnID) {
            g_localSpawnId   = s->spawn_id;
			// Detect changes vs previous server profile and log only deltas to help trace origin of stat changes
			int prev_cur_hp = g_serverCurHP;
			int prev_max_hp = g_serverMaxHP;
			int prev_cur_mana = g_serverCurMana;
			int prev_max_mana = g_serverMaxMana;
			int prev_cur_end = g_serverCurEnd;
			int prev_max_end = g_serverMaxEnd;
			int prev_str = g_serverProfile.str;
			int prev_sta = g_serverProfile.sta;
			int prev_agi = g_serverProfile.agi;
			int prev_dex = g_serverProfile.dex;
			int prev_intl = g_serverProfile.intl;
			int prev_wis = g_serverProfile.wis;
			int prev_cha = g_serverProfile.cha;

			g_serverProfile.has_profile = true;
			g_serverProfile.str      = s->str;
			g_serverProfile.sta      = s->sta;
			g_serverProfile.agi      = s->agi;
			g_serverProfile.dex      = s->dex;
			g_serverProfile.intl     = s->intl;
			g_serverProfile.wis      = s->wis;
			g_serverProfile.cha      = s->cha;
			g_serverCurHP            = s->cur_hp;
			g_serverMaxHP            = s->max_hp;
			g_serverCurMana          = s->cur_mana;
			g_serverMaxMana          = s->max_mana;
			g_serverCurEnd           = s->cur_end;
			g_serverMaxEnd           = s->max_end;

			// Do NOT overwrite the client's in-memory spawn HP fields here.
			// We prefer to enforce authoritative values at accessor/detour
			// boundaries (Max_HP/Max_Mana/Max_Endurance detours) rather than
			// mutating spawn structures in-place. This preserves client-side
			// integrity and avoids masking underlying divergences.

			// Log the full applied server stats (for visibility)
			LogDebug("[SERVER_STATS_APPLIED] STR=%d STA=%d AGI=%d DEX=%d INT=%d WIS=%d CHA=%d HP=%d/%d Mana=%d/%d End=%d/%d",
				s->str, s->sta, s->agi, s->dex, s->intl, s->wis, s->cha,
				s->cur_hp, s->max_hp, s->cur_mana, s->max_mana, s->cur_end, s->max_end);

			// Also write a small raw dump of the ServerStatsUpdate payload to help debug zoning sequences
			{
				FILE* sf = nullptr;
				if (fopen_s(&sf, "stats_debug.log", "a") == 0 && sf) {
					time_t now = std::time(nullptr);
					struct tm *tmv = localtime(&now);
					char tb[32] = {0};
					strftime(tb, sizeof(tb), "%Y%m%d_%H%M%S", tmv);
					fprintf(sf, "[%s] STATS_RAW spawn=%u size=%zu str=%d sta=%d agi=%d dex=%d intl=%d wis=%d cha=%d cur_hp=%d max_hp=%d cur_mana=%d max_mana=%d cur_end=%d max_end=%d\n",
						tb, s->spawn_id, size, s->str, s->sta, s->agi, s->dex, s->intl, s->wis, s->cha, s->cur_hp, s->max_hp, s->cur_mana, s->max_mana, s->cur_end, s->max_end);
					// hex preview of the payload
					size_t preview_len = size < 128 ? size : 128;
					for (size_t i = 0; i < preview_len; ++i) {
						fprintf(sf, "%02X", (unsigned char)buf[i]);
					}
					fprintf(sf, "\n");
					// write full small binary dump for offline analysis
					const size_t kStatsDumpMax = 65536;
					if (size > 0 && size <= kStatsDumpMax) {
						char dumpname[128];
						snprintf(dumpname, sizeof(dumpname), "stats_dump_%s_%04x.bin", tb, opcode & 0xFFFF);
						FILE* df = nullptr;
						if (fopen_s(&df, dumpname, "wb") == 0 && df) {
							fwrite(buf, 1, size, df);
							fclose(df);
							fprintf(sf, "WROTE %s (%zu bytes)\n", dumpname, size);
						}
					}
					fclose(sf);
				}
						// Also mirror the same human-readable log into the workspace logs folder so
						// it's easy to find when running the client from outside the repo.
						{
							FILE* sf2 = nullptr;
							const char* repo_stats_path = "C:\\Users\\marsh\\OneDrive\\Documents\\GitHub\\EQ_Server\\logs\\stats_debug.log";
							if (fopen_s(&sf2, repo_stats_path, "a") == 0 && sf2) {
								time_t now2 = std::time(nullptr);
								struct tm *tmv2 = localtime(&now2);
								char tb2[32] = {0};
								strftime(tb2, sizeof(tb2), "%Y%m%d_%H%M%S", tmv2);
								fprintf(sf2, "[%s] STATS_RAW spawn=%u size=%zu str=%d sta=%d agi=%d dex=%d intl=%d wis=%d cha=%d cur_hp=%d max_hp=%d cur_mana=%d max_mana=%d cur_end=%d max_end=%d\n",
									tb2, s->spawn_id, size, s->str, s->sta, s->agi, s->dex, s->intl, s->wis, s->cha, s->cur_hp, s->max_hp, s->cur_mana, s->max_mana, s->cur_end, s->max_end);
								size_t preview_len2 = size < 128 ? size : 128;
								for (size_t i = 0; i < preview_len2; ++i) {
									fprintf(sf2, "%02X", (unsigned char)buf[i]);
								}
								fprintf(sf2, "\n");
								// small binary dump as well
								const size_t kStatsDumpMax2 = 65536;
								if (size > 0 && size <= kStatsDumpMax2) {
									char dumpname2[256];
									snprintf(dumpname2, sizeof(dumpname2), "C:\\Users\\marsh\\OneDrive\\Documents\\GitHub\\EQ_Server\\logs\\stats_dump_%s_%04x.bin", tb2, opcode & 0xFFFF);
									FILE* df2 = nullptr;
									if (fopen_s(&df2, dumpname2, "wb") == 0 && df2) {
										fwrite(buf, 1, size, df2);
										fclose(df2);
										fprintf(sf2, "WROTE %s (%zu bytes)\n", dumpname2, size);
									}
								}
								fclose(sf2);
							}
						}
			}

			// Now log any deltas found (showing source: ServerStatsUpdate)
			char delta_buf[512];
			delta_buf[0] = '\0';
			int pos = 0;
			auto app = [&](const char* name, int oldv, int newv) {
				if (oldv != newv) {
					pos += snprintf(delta_buf + pos, sizeof(delta_buf) - pos, "%s:%d->%d ", name, oldv, newv);
				}
			};
			app("HP_max", prev_max_hp, g_serverMaxHP);
			app("HP_cur", prev_cur_hp, g_serverCurHP);
			app("Mana_max", prev_max_mana, g_serverMaxMana);
			app("Mana_cur", prev_cur_mana, g_serverCurMana);
			app("End_max", prev_max_end, g_serverMaxEnd);
			app("End_cur", prev_cur_end, g_serverCurEnd);
			app("STR", prev_str, g_serverProfile.str);
			app("STA", prev_sta, g_serverProfile.sta);
			app("AGI", prev_agi, g_serverProfile.agi);
			app("DEX", prev_dex, g_serverProfile.dex);
			app("INT", prev_intl, g_serverProfile.intl);
			app("WIS", prev_wis, g_serverProfile.wis);
			app("CHA", prev_cha, g_serverProfile.cha);
			if (delta_buf[0] != '\0') {
				LogDebug("[SERVER_STATS_CHANGE] %s spawn=%u src=ServerStatsUpdate", delta_buf, s->spawn_id);
			}
        }
    }

    // Capture server HP / Endurance / Mana updates for the local player so the label hook can use server-authoritative values.
    // RoF2 OP_HPUpdate payload ordering differs from emu/common (spawn_id first).
    if (
        (op == hp_opcode && size >= 10) ||
        (op == end_opcode && size >= 10) ||
        (op == mana_opcode && size >= 10) ||
        // mob_* opcodes are percent-based (smaller) and handled elsewhere; keep them out of this 10-byte parser
        false
    ) {
        try {
#pragma pack(push,1)
            struct HpUpdatePayload_EmuOrder {
                uint32_t cur;
                int32_t  max;
                uint16_t spawn_id;
            };

            struct HpUpdatePayload_RoF2HPOrder {
                uint16_t spawn_id;
                uint32_t cur;
                int32_t  max;
            };
#pragma pack(pop)

            uint32_t cur = 0;
            int32_t max = 0;
            uint16_t spawn = 0;

            if (op == hp_opcode) {
                const auto* hp = reinterpret_cast<const HpUpdatePayload_RoF2HPOrder*>(buf);
                cur = hp->cur;
                max = hp->max;
                spawn = hp->spawn_id;
            } else {
                const auto* hp = reinterpret_cast<const HpUpdatePayload_EmuOrder*>(buf);
                cur = hp->cur;
                max = hp->max;
                spawn = hp->spawn_id;
            }

            PSPAWNINFO me = nullptr;
            if (GetCharInfo()) {
                me = reinterpret_cast<PSPAWNINFO>(GetCharInfo()->pSpawn);
            }
            // Only consider packets for our spawn with non-zero values
            if (me && spawn == me->SpawnID && (cur > 0 || max > 0)) {
				if (op == hp_opcode) {
					int prev_max = g_serverMaxHP;
					g_serverCurHP = static_cast<int>(cur);
					g_serverMaxHP = static_cast<int>(max);
					LogPacketDetail("[HP_PACKET_APPLIED]", opcode, size, cur, max, spawn, prev_max, "HPUpdate");
				} else if (op == end_opcode) { // endurance updates
					int prev_max = g_serverMaxEnd;
					g_serverCurEnd = static_cast<int>(cur);
					g_serverMaxEnd = static_cast<int>(max);
					LogPacketDetail("[END_PACKET_APPLIED]", opcode, size, cur, max, spawn, prev_max, "EnduranceUpdate");
				} else { // mana updates
					int prev_max = g_serverMaxMana;
					g_serverCurMana = static_cast<int>(cur);
					g_serverMaxMana = static_cast<int>(max);
					LogPacketDetail("[MANA_PACKET_APPLIED]", opcode, size, cur, max, spawn, prev_max, "ManaUpdate");
				}
            }
        } catch (...) {
            LogDebug("[SERVER_HP_CACHE] exception parsing possible HPUpdate");
        }
    }

    // Intercept OP_ItemPacket and OP_CharInventory to apply custom stats per-instance
    // Server sends: base stats in ItemBodyStruct + appended custom data (magic marker + key-value pairs)
    // We strip the appended custom data and apply it to the local item instance without modifying global cache
    if ((opcode & 0xFFFF) == 0x368e || (opcode & 0xFFFF) == 0x5ca6) { // OP_ItemPacket (0x368e) or OP_CharInventory (0x5ca6) RoF2
        try {
            // Check for magic marker (0x1337C0DE) near end of packet
            if (size >= 6) { // At least marker + count
                // Scan backwards for magic marker (ItemBodyStruct size varies)
                for (int offset = (int)size - 6; offset >= 0; offset--) {
                    DWORD marker = *(DWORD*)(buf + offset);
                    if (marker == 0x1337C0DE) {
                        // Found custom stat data!
                        WORD pair_count = *(WORD*)(buf + offset + 4);

                        // Calculate custom data size: marker(4) + count(2) + (key(32) + val(4)) * pair_count
                        size_t custom_data_size = 6 + (pair_count * 36);
                        size_t base_packet_size = offset;

                        LogDebug("[CUSTOM_STATS] Found %d custom stat pairs, stripping %d bytes (packet was %d, now %d)",
                                 pair_count, custom_data_size, size, base_packet_size);

                        // Parse custom stat pairs
                        int dynamic_level = 0;
                        for (WORD i = 0; i < pair_count; i++) {
                            size_t pair_offset = offset + 6 + (i * 36);
                            if (pair_offset + 36 > size) break; // Safety check

                            char key_buf[33] = {0};
                            memcpy(key_buf, buf + pair_offset, 32);
							int32_t value = *(int32_t*)(buf + pair_offset + 32);

                            if (strcmp(key_buf, "dynamic_level") == 0) {
                                dynamic_level = value;
                                LogDebug("[CUSTOM_STATS] Parsed dynamic_level=%d", dynamic_level);
                            }
								// Log every custom stat pair for visibility
								LogDebug("[CUSTOM_STATS_PAIR] key=%s value=%d", key_buf, value);
                        }

                        // TODO: Apply scaling to the item instance in client memory
                        // For now, just log what we found and strip the data
                        // Future: Find item instance, apply ScaleDynamicItem() formulas client-side

                        // Strip custom data from packet before passing to game
                        size = base_packet_size;
                        break;
                    }
                }
            }
        } catch (...) {
            LogDebug("[CUSTOM_STATS] Exception parsing custom stats");
        }
    }    unsigned char result = HandleWorldMessage_Trampoline(con, edx, unk, opcode, buf, size);
	return result;
}DETOUR_TRAMPOLINE_EMPTY(unsigned char __fastcall HandleWorldMessage_Trampoline(DWORD *con, DWORD edx, unsigned __int32 unk, unsigned __int32 opcode, char* buf, size_t size));

unsigned char __fastcall SendMessage_Trampoline(DWORD*, unsigned __int32, unsigned __int32, char* buf, size_t, DWORD, DWORD);
unsigned char __fastcall SendMessage_Detour(DWORD* con, unsigned __int32 unk, unsigned __int32 channel, char* buf, size_t size, DWORD a6, DWORD a7)
{
	DWORD retval = 0;
	bExeChecksumrequested = 1;
	int16_t opcode = 0;
	memcpy(&opcode, buf, 2);
	if (opcode == 0xf13 || opcode == 0x578f)
	{
		if (isReportHardwareAddressEnabled) {
			IP_ADAPTER_INFO AdapterInfo[16];
			BYTE macAddress[8];
			memset(macAddress, 0, sizeof(macAddress));
			DWORD dwBufLen = sizeof(AdapterInfo);
			DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
			if (dwStatus == ERROR_SUCCESS)
			{

				IP_ADAPTER_INFO AdapterInfo[16];
				DWORD dwBufLen = sizeof(AdapterInfo);
				DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);

				MacEntry_Struct* me = new MacEntry_Struct;
				memset(me, 0, sizeof(MacEntry_Struct));
				me->opcode = 0xf13;
				memcpy(&me->address, AdapterInfo[0].Address, 8);

				SendMessage_Trampoline(con, unk, channel, (char*)me,
					sizeof(MacEntry_Struct), a6, a7);

				delete me;
			}
		}

		if (isMQ2PreventionEnabled) {
			DWORD var = 0;
			auto charToBreak = rand();
			var = (((DWORD)0x009DD250 - 0x400000) + baseAddress);
			PatchA((DWORD*)var, (DWORD*)&charToBreak, 4);

			charToBreak = rand();
			var = (((DWORD)0x009DD254 - 0x400000) + baseAddress);
			PatchA((DWORD*)var, (DWORD*)&charToBreak, 4);

			charToBreak = rand();
			var = (((DWORD)0x009DD258 - 0x400000) + baseAddress);
			PatchA((DWORD*)var, (DWORD*)&charToBreak, 4);

			charToBreak = rand();
			var = (((DWORD)0x009DD25C - 0x400000) + baseAddress);
			PatchA((DWORD*)var, (DWORD*)&charToBreak, 4);

			charToBreak = rand();
			var = (((DWORD)0x009DD260 - 0x400000) + baseAddress);
			PatchA((DWORD*)var, (DWORD*)&charToBreak, 4);
		}

		if (isChecksumFixEnabled && opcode == 0xf13)
		{
			Checksum_Struct* cs = (Checksum_Struct*)buf;
			SimpleChecksum_Struct* scs = new SimpleChecksum_Struct;
			memset(scs, 0, sizeof(SimpleChecksum_Struct));
			scs->opcode = 0xf13;
			scs->checksum = cs->checksum;

			retval = SendMessage_Trampoline(con, unk, channel, (char*)scs,
				sizeof(Checksum_Struct), a6, a7);

			delete scs;

			return retval;
		}
	}
	retval = SendMessage_Trampoline(con, unk, channel, buf, size, a6, a7);
	return retval;
}

DETOUR_TRAMPOLINE_EMPTY(unsigned char __fastcall SendMessage_Trampoline(DWORD*, unsigned __int32, unsigned __int32, char* buf, size_t, DWORD, DWORD));

DETOUR_TRAMPOLINE_EMPTY(unsigned char __fastcall SetDeviceGammaRamp_Trampoline(HDC hdc, LPVOID lpRamp));

// Detours: prefer server-provided max stats when available
// NOTE: These are EQ_Character member functions (thiscall). When detouring, we
// use __fastcall with (this, edx, args...) to preserve calling convention.
DETOUR_TRAMPOLINE_EMPTY(int __fastcall EQCharacter_MaxHP_Tramp(void*, void*, int, int));
int __fastcall EQCharacter_MaxHP_Detour(void* This, void* edx, int a1, int a2)
{
	g_last_hook_tag = Hook_MaxHP;
	int nativeVal = 0;
	__try {
		nativeVal = EQCharacter_MaxHP_Tramp(This, edx, a1, a2);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (isDebugLoggingEnabled) {
			LogDebug("CLIENT_DETOUR stat=HP native trampoline faulted (MaxHP); falling back");
		}
		nativeVal = 0;
	}
	int used = nativeVal;
	int server = g_serverMaxHP;
	if (server > 0) {
		used = server;
	}
	if (used <= 0) {
		// Avoid returning 0 which can break gauge math in some UI paths.
		used = 1;
	}
	// Log first use or changes for local player (simple static int to avoid
	// global container construction order problems)
	if (pLocalPlayer) {
		if (g_last_logged_hp != used) {
			g_last_logged_hp = used;
			LogDebug("CLIENT_DETOUR stat=HP server=%d native=%d used=%d", server, nativeVal, used);
		}
	}
	return used;
}

DETOUR_TRAMPOLINE_EMPTY(int __fastcall EQCharacter_CurHP_Tramp(void*, void*, int, unsigned char));
int __fastcall EQCharacter_CurHP_Detour(void* This, void* edx, int a1, unsigned char a2)
{
	g_last_hook_tag = Hook_CurHP;
	int nativeVal = 0;
	__try {
		nativeVal = EQCharacter_CurHP_Tramp(This, edx, a1, a2);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (isDebugLoggingEnabled) {
			LogDebug("CLIENT_DETOUR stat=CurHP native trampoline faulted; falling back");
		}
		nativeVal = 0;
	}
	int used = nativeVal;
	int server = g_serverCurHP;
	if (server > 0) {
		used = server;
	}
	if (used <= 0 && g_serverProfile.has_profile && g_serverProfile.hp_cur > 0) {
		used = g_serverProfile.hp_cur;
	}
	if (used < 0) {
		used = 0;
	}
	if (pLocalPlayer) {
		static int g_last_logged_cur_hp = INT_MIN;
		if (g_last_logged_cur_hp != used) {
			g_last_logged_cur_hp = used;
			LogDebug("CLIENT_DETOUR stat=CurHP server=%d native=%d used=%d", server, nativeVal, used);
		}
	}
	return used;
}

// UI gauge + label hooks (used by stock UI for HP/Mana/End display)
DETOUR_TRAMPOLINE_EMPTY(int __cdecl GetGaugeValueFromEQ_Tramp(int, class CXStr *, bool *, unsigned long *));
static int __cdecl GetGaugeValueFromEQ_Detour(int eq_type, class CXStr *out, bool *arg3, unsigned long *colorout)
{
	g_last_hook_tag = Hook_GetGaugeValueFromEQ;
	int ret = GetGaugeValueFromEQ_Tramp(eq_type, out, arg3, colorout);

	// Empirically (and in classless), gauge values are 0..1000.
	auto calc_1000 = [](int cur, int max) -> int {
		if (max <= 0 || cur < 0) {
			return -1;
		}
		if (cur > max) {
			cur = max;
		}
		return static_cast<int>((static_cast<double>(cur) / static_cast<double>(max)) * 1000.0);
	};

	if (eq_type == 1) { // HP gauge
		int v = (g_serverMaxHP > 0) ? calc_1000(g_serverCurHP, g_serverMaxHP) : -1;
		if (v >= 0) {
			ret = v;
		}
	} else if (eq_type == 2) { // Mana gauge
		int v = (g_serverMaxMana > 0) ? calc_1000(g_serverCurMana, g_serverMaxMana) : -1;
		if (v >= 0) {
			ret = v;
		}
	} else if (eq_type == 3) { // Endurance gauge
		int v = (g_serverMaxEnd > 0) ? calc_1000(g_serverCurEnd, g_serverMaxEnd) : -1;
		if (v >= 0) {
			ret = v;
		}
	}

	return ret;
}

DETOUR_TRAMPOLINE_EMPTY(int __cdecl GetLabelFromEQ_Tramp(int, class CXStr *, bool *, unsigned long *));
static int __cdecl GetLabelFromEQ_Detour(int eq_type, class CXStr *out, bool *arg3, unsigned long *colorout)
{
	g_last_hook_tag = Hook_GetLabelFromEQ;
	int ret = GetLabelFromEQ_Tramp(eq_type, out, arg3, colorout);

	// In classless, EQType==29 is the HP% label.
	if (eq_type == 29 && out && out->Ptr && g_serverMaxHP > 0 && g_serverCurHP >= 0) {
		int cur = g_serverCurHP;
		int max = g_serverMaxHP;
		if (cur > max) {
			cur = max;
		}
		int pct = static_cast<int>((static_cast<double>(cur) / static_cast<double>(max)) * 100.0);
		char tmp[16] = {0};
		snprintf(tmp, sizeof(tmp), "%d", pct);
		SetCXStr(&out->Ptr, (PCHAR)tmp);
	}

	// Multiclass Class Label (EQType 3)
	if (eq_type == 3 && out && out->Ptr && g_serverUsableClassesMask > 0) {
		char cls_str[256] = {0};
		const char* short_names[] = {
			"WAR", "CLR", "PAL", "RNG", "SHD", "DRU", "MNK", "BRD",
			"ROG", "SHM", "NEC", "WIZ", "MAG", "ENC", "BST", "BER"
		};
		int count = 0;
		for (int i = 0; i < 16; ++i) {
			if (g_serverUsableClassesMask & (1 << i)) {
				if (count > 0) strncat(cls_str, "/", 255 - strlen(cls_str));
				strncat(cls_str, short_names[i], 255 - strlen(cls_str));
				count++;
			}
		}
		if (count > 0) {
			SetCXStr(&out->Ptr, (PCHAR)cls_str);
		}
	}

	return ret;
}

static volatile LONG g_ui_detours_installed = 0;
static void InstallUiDetours()
{
	// MQ2Labels (and other MQ2 components) already detour these UI helpers. Installing our own detours
	// on top of MQ2's can produce invalid trampolines and crash (e.g., null trampoline call).
	if (isMQInjectsEnabled) {
		if (isDebugLoggingEnabled) {
			LogDebug("Skipping UI detours (__GetGaugeValueFromEQ/__GetLabelFromEQ) because MQ2 injects are enabled");
		}
		return;
	}

	if (InterlockedCompareExchange(&g_ui_detours_installed, 1, 0) != 0) {
		return;
	}

	LogDebug("Installing UI detours (__GetGaugeValueFromEQ/__GetLabelFromEQ)...");
	LogDetourTargetBytes("__GetGaugeValueFromEQ", __GetGaugeValueFromEQ);
	LogDetourTargetBytes("__GetLabelFromEQ", __GetLabelFromEQ);
	EzDetour(__GetGaugeValueFromEQ, GetGaugeValueFromEQ_Detour, GetGaugeValueFromEQ_Tramp);
	EzDetour(__GetLabelFromEQ, GetLabelFromEQ_Detour, GetLabelFromEQ_Tramp);
	LogDebug("UI detours installed");
}

DETOUR_TRAMPOLINE_EMPTY(int __fastcall EQCharacter_MaxMana_Tramp(void*, void*, int));
int __fastcall EQCharacter_MaxMana_Detour(void* This, void* edx, int a1)
{
	int nativeVal = 0;
	__try {
		nativeVal = EQCharacter_MaxMana_Tramp(This, edx, a1);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (isDebugLoggingEnabled) {
			LogDebug("CLIENT_DETOUR stat=Mana native trampoline faulted (MaxMana); falling back");
		}
		nativeVal = 0;
	}
	int used = nativeVal;
	int server = g_serverMaxMana;
	if (server > 0) used = server;
	if (pLocalPlayer) {
		if (g_last_logged_mana != used) {
			g_last_logged_mana = used;
			LogDebug("CLIENT_DETOUR stat=Mana server=%d native=%d used=%d", server, nativeVal, used);
		}
	}
	return used;
}

DETOUR_TRAMPOLINE_EMPTY(int __fastcall EQCharacter_MaxEnd_Tramp(void*, void*, int));
int __fastcall EQCharacter_MaxEnd_Detour(void* This, void* edx, int a1)
{
	int nativeVal = 0;
	__try {
		nativeVal = EQCharacter_MaxEnd_Tramp(This, edx, a1);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (isDebugLoggingEnabled) {
			LogDebug("CLIENT_DETOUR stat=Endurance native trampoline faulted (MaxEnd); falling back");
		}
		nativeVal = 0;
	}
	int used = nativeVal;
	int server = g_serverMaxEnd;
	if (server > 0) used = server;
	if (pLocalPlayer) {
		if (g_last_logged_end != used) {
			g_last_logged_end = used;
			LogDebug("CLIENT_DETOUR stat=Endurance server=%d native=%d used=%d", server, nativeVal, used);
		}
	}
	return used;
}

// Override "usable classes" mask for client-side filters (AA window, item/spell usability, equipment validation).
// This is the key hook for multiclass equipment support - the client calls GetUsableClasses to check
// if the player can equip an item.
//
// APPROACH: Default to server multiclass mask for ALL call sites.
// Only force NATIVE on known display-only RVAs where returning the player mask would be misleading.
//
// Known RVAs (discovered via debug logging):
//   0x0004C472 - Equipment slot validation (needs multiclass)
//   0x002F0DA3 - Item use / right-click cast (needs multiclass)
//   0x002F0E05 - Item use / right-click cast (needs multiclass)
//   0x002A9736 - Tooltip class display (needs NATIVE - show item's real classes)
//   0x002A6D16 - Display/filter (needs NATIVE)
//
// Native-only denylist = RVAs that should return native values for display fidelity.
// This avoids equip regressions when client callsites shift between builds.
static const std::set<DWORD> s_native_usable_classes_rvas = {
	0x002A9736,  // Tooltip class display text path
};

// Call counter for verbose logging
static volatile LONG s_usable_classes_call_count = 0;
static bool s_logged_usable_classes_no_mask = false;

// Deduplication: track unique (RVA, native, mask) combos we've already logged
static std::set<uint64_t> s_logged_usable_classes_combos;
static DWORD s_last_usable_classes_rva = 0;
static int s_last_usable_classes_a1 = 0;
static DWORD s_last_usable_classes_a2 = 0;
static int s_last_usable_classes_native = 0;
static uint32_t s_last_usable_classes_base_mask = 0;
static uint32_t s_last_usable_classes_mask = 0;
static bool s_last_usable_classes_force_native = false;
static int s_last_usable_classes_return = 0;
static bool s_has_last_usable_classes_verbose = false;
static volatile LONG s_usable_classes_verbose_suppressed = 0;

static const char* GetUsableClassesContextName(DWORD rva)
{
	switch (rva) {
	case 0x0004C472: return "equip_validation";
	case 0x002F0DA3: return "item_use_check";
	case 0x002F0E05: return "item_use_check_2";
	case 0x002A9736: return "tooltip_classes";
	case 0x002A6D16: return "display_filter";
	default: return "unknown";
	}
}

static uint32_t GetLocalBaseClassMask()
{
	uint8_t base_class_id = 0;
	PCHARINFO2 ci2 = nullptr;
	__try {
		ci2 = GetCharInfo2();
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		ci2 = nullptr;
	}

	if (ci2 && ci2->Class >= 1 && ci2->Class <= 16) {
		base_class_id = static_cast<uint8_t>(ci2->Class);
	} else if (pLocalPlayer && pLocalPlayer->Data.Class >= 1 && pLocalPlayer->Data.Class <= 16) {
		base_class_id = static_cast<uint8_t>(pLocalPlayer->Data.Class);
	}

	if (base_class_id >= 1 && base_class_id <= 16) {
		return (1u << (base_class_id - 1));
	}
	return 0;
}

DETOUR_TRAMPOLINE_EMPTY(int __fastcall EQCharacter_GetUsableClasses_Tramp(void*, void*, int, DWORD));
int __fastcall EQCharacter_GetUsableClasses_Detour(void* This, void* edx, int a1, DWORD a2)
{
	g_last_hook_tag = Hook_GetUsableClasses;

	// Get native value first - this is what the client would normally return
	int nativeVal = -1;
	__try {
		nativeVal = EQCharacter_GetUsableClasses_Tramp(This, edx, a1, a2);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		nativeVal = -1;
	}

	const uint32_t effectiveMask = GetEffectiveUsableClassesMask();

	// If multiclass is disabled or no effective mask, always use native
	if (!isMulticlassUsableClassesOverrideEnabled || effectiveMask == 0) {
		if (isDebugLoggingEnabled && isMulticlassUsableClassesOverrideEnabled && effectiveMask == 0 && !s_logged_usable_classes_no_mask) {
			LogDebug("[USABLE_CLASSES] multiclass override enabled but server mask is 0; using native until mask arrives");
			s_logged_usable_classes_no_mask = true;
		}
		return nativeVal;
	}
	s_logged_usable_classes_no_mask = false;

	// Get calling RVA
	void* ret_addr = _ReturnAddress();
	DWORD ret_rva = (DWORD)((uintptr_t)ret_addr - (uintptr_t)baseAddress);

	// Default to multiclass mask unless this is a native-only display path.
	bool forceNative = (s_native_usable_classes_rvas.find(ret_rva) != s_native_usable_classes_rvas.end());

	// Multiclass override should only apply when the native class mask overlaps owned multiclass bits.
	// This allows WAR/MAG items for a RNG/WAR/MAG character, while preserving native deny for ENC-only items.
	const uint32_t baseClassMask = GetLocalBaseClassMask();
	const uint32_t nativeMask16 = static_cast<uint32_t>(nativeVal) & 0xFFFFu;
	const bool shouldApplyMulticlassOverride =
		!forceNative &&
		nativeMask16 != 0 &&
		(nativeMask16 & effectiveMask) != 0;

	// Determine what we'll return.
	int returnVal = shouldApplyMulticlassOverride ? static_cast<int>(effectiveMask) : nativeVal;

	// Debug logging - deduplicated to avoid spam
	if (isDebugLoggingEnabled) {
		InterlockedIncrement(&s_usable_classes_call_count);

		if (isMulticlassUsableClassesVerboseLoggingEnabled) {
			const bool sameAsLast =
				s_has_last_usable_classes_verbose &&
				s_last_usable_classes_rva == ret_rva &&
				s_last_usable_classes_a1 == a1 &&
				s_last_usable_classes_a2 == a2 &&
				s_last_usable_classes_native == nativeVal &&
				s_last_usable_classes_base_mask == baseClassMask &&
				s_last_usable_classes_mask == effectiveMask &&
				s_last_usable_classes_force_native == forceNative &&
				s_last_usable_classes_return == returnVal;

			if (sameAsLast) {
				InterlockedIncrement(&s_usable_classes_verbose_suppressed);
			} else {
				const LONG suppressed = InterlockedExchange(&s_usable_classes_verbose_suppressed, 0);
				if (suppressed > 0 && s_has_last_usable_classes_verbose) {
					LogDebug("[USABLE_CLASSES] suppressed=%ld repeats for RVA=0x%08X ctx=%s returning=%d",
						suppressed, s_last_usable_classes_rva, GetUsableClassesContextName(s_last_usable_classes_rva), s_last_usable_classes_return);
				}

				LogDebug("[USABLE_CLASSES] RVA=0x%08X ctx=%s a1=%d a2=%u native=%d base_mask=0x%04X mask=0x%04X force_native=%d apply_multi=%d returning=%d",
					ret_rva, GetUsableClassesContextName(ret_rva), a1, a2, nativeVal, baseClassMask & 0xFFFFu, effectiveMask, forceNative ? 1 : 0, shouldApplyMulticlassOverride ? 1 : 0, returnVal);

				s_last_usable_classes_rva = ret_rva;
				s_last_usable_classes_a1 = a1;
				s_last_usable_classes_a2 = a2;
				s_last_usable_classes_native = nativeVal;
				s_last_usable_classes_base_mask = baseClassMask;
				s_last_usable_classes_mask = effectiveMask;
				s_last_usable_classes_force_native = forceNative;
				s_last_usable_classes_return = returnVal;
				s_has_last_usable_classes_verbose = true;
			}
		} else {
			// QUIET MODE: Only log first occurrence per unique (RVA, native, mask) combo
			uint64_t combo = ((uint64_t)ret_rva << 32) | ((uint64_t)(nativeVal & 0xFFFF) << 16) | (effectiveMask & 0xFFFF);
			if (s_logged_usable_classes_combos.find(combo) == s_logged_usable_classes_combos.end()) {
				LogDebug("[USABLE_CLASSES] RVA=0x%08X native=%d base_mask=0x%04X mask=0x%04X force_native=%d apply_multi=%d returning=%d (first occurrence)",
					ret_rva, nativeVal, baseClassMask & 0xFFFFu, effectiveMask, forceNative ? 1 : 0, shouldApplyMulticlassOverride ? 1 : 0, returnVal);
				s_logged_usable_classes_combos.insert(combo);
			}
		}
	}

	return returnVal;
}

// Override spell required-level lookups so client-side usability filters (merchant "Show Usable Items", spellbook, etc)
// behave correctly for multiclass characters.
//
// RoF2 calls this as `EQ_Spell::GetSpellLevelNeeded(int classId)` where `this` is the spell object.
//
// KEY INSIGHT: This function is called for TWO different purposes:
//   1. DISPLAY: Tooltips showing "Bard(2)" - should return native class levels
//   2. FILTERING: "Show Usable Items" checkbox - should return 255 for spells player can't use
//
// We use RVA-based detection (like GetUsableClasses) to determine context:
//   - Filter RVAs: Return 255 if player has no owned class that can use the spell
//   - Display RVAs: Return native values for correct tooltip display
//
// Known filter RVAs (discovered via debug logging - enable s_verbose_spell_level_logging):
static const std::set<DWORD> s_spell_level_filter_rvas = {
	// These RVAs are called when filtering merchant spell lists ("Show Usable Items")
	0x002F0DCD,  // Spell merchant "Show Usable Items" filter check
	0x002A6D26,  // Merchant window item usability check (tomes/disciplines)
};

// Verbose logging for discovering new RVAs
static bool s_verbose_spell_level_logging = false;
static volatile LONG s_spell_level_call_count = 0;

// Deduplication: track unique (RVA, spell_id, class, native, best) combos we've already logged
static std::set<uint64_t> s_logged_spell_level_combos;

DETOUR_TRAMPOLINE_EMPTY(int __fastcall EQSpell_GetSpellLevelNeeded_Tramp(void*, void*, int));
int __fastcall EQSpell_GetSpellLevelNeeded_Detour(void* This, void* edx, int classId)
{
	g_last_hook_tag = Hook_GetSpellLevelNeeded;

	// Try to pull per-class levels directly from the in-memory spell struct. This avoids relying on calling
	// into the game's implementation repeatedly (and helps keep multiclass aggregation deterministic).
	bool have_levels = false;
	uint8_t levels[16] = { 0 };
	uint32_t spell_id = 0;
	char spell_name[65] = { 0 };
	__try {
		PSPELL sp = reinterpret_cast<PSPELL>(This);
		spell_id = sp->ID;
		memcpy(levels, sp->Level, sizeof(levels));
		memcpy(spell_name, sp->Name, 64);
		spell_name[64] = 0;
		have_levels = true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		have_levels = false;
	}

	// IMPORTANT: preserve the client's native return semantics (some call sites do not treat 255 as the unusable sentinel).
	// We only use the raw spell struct levels[] for deterministic multiclass aggregation across owned classes.
	int nativeVal = 255;
	bool nativeFaulted = false;
	__try {
		nativeVal = EQSpell_GetSpellLevelNeeded_Tramp(This, edx, classId);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		nativeFaulted = true;
		if (isDebugLoggingEnabled) {
			LogDebug("CLIENT_DETOUR stat=SpellLevelNeeded native trampoline faulted; falling back");
		}
		nativeVal = 255;
	}

	if (nativeFaulted && have_levels && classId >= 1 && classId <= 16) {
		nativeVal = static_cast<int>(levels[classId - 1]);
	}

	// Calculate best level among owned classes (needed for both filtering and override decisions)
	int bestLevel = 255;
	if (have_levels && isMulticlassUsableClassesOverrideEnabled && g_serverUsableClassesMask != 0) {
		for (int i = 0; i < 16; ++i) {
			uint16_t classBit = (1u << i);
			if ((g_serverUsableClassesMask & classBit) != 0) {
				int lvl = static_cast<int>(levels[i]);
				if (lvl > 0 && lvl < 255 && lvl < bestLevel) {
					bestLevel = lvl;
				}
			}
		}
	}

	// Get calling RVA for context detection
	void* ret_addr = _ReturnAddress();
	DWORD ret_rva = (DWORD)((uintptr_t)ret_addr - (uintptr_t)baseAddress);

	// Check if this is a filter context (needs strict multiclass filtering)
	bool isFilterContext = (s_spell_level_filter_rvas.find(ret_rva) != s_spell_level_filter_rvas.end());

	// Debug logging for RVA discovery - deduplicated
	if (isDebugLoggingEnabled) {
		InterlockedIncrement(&s_spell_level_call_count);

		if (s_verbose_spell_level_logging) {
			// VERBOSE MODE: Log every call (for RVA discovery only)
			LogDebug("[SPELL_LEVEL] RVA=0x%08X spell=%u '%s' class=%d native=%d best=%d mask=0x%04X",
				ret_rva, spell_id, spell_name, classId, nativeVal, bestLevel, g_serverUsableClassesMask);
		} else {
			// QUIET MODE: Only log first occurrence per unique (RVA, spell_id, classId, bestLevel) combo
			uint64_t combo = ((uint64_t)ret_rva << 32) | ((uint64_t)spell_id << 16) | ((uint64_t)classId << 8) | (bestLevel & 0xFF);
			if (s_logged_spell_level_combos.find(combo) == s_logged_spell_level_combos.end()) {
				LogDebug("[SPELL_LEVEL] RVA=0x%08X spell=%u '%s' class=%d native=%d best=%d mask=0x%04X (first)",
					ret_rva, spell_id, spell_name, classId, nativeVal, bestLevel, g_serverUsableClassesMask);
				s_logged_spell_level_combos.insert(combo);
			}
		}
	}

	if (!isMulticlassUsableClassesOverrideEnabled || g_serverUsableClassesMask == 0) {
		return nativeVal;
	}

	// MULTICLASS FILTERING LOGIC:
	// Only apply filtering in filter contexts (spell merchant "Show Usable Items" checkbox).
	// In display contexts (tooltips), always return native values to show correct class/level info.
	if (isFilterContext && bestLevel == 255) {
		// Filter context AND player cannot use this spell with any of their classes
		// Return 255 to hide it from the "Show Usable Items" filter
		return 255;
	}

	// For multiclass: if the queried classId's native level is 255 (can't use), but we have spell levels
	// and one of our owned classes CAN use this spell, return the best (lowest) level among owned classes.
	// This fixes memorization time calculation which uses the queried class's level - if it's 255, mem time is huge.
	if (bestLevel < 255 && bestLevel < nativeVal) {
		if (isDebugLoggingEnabled && spell_id == 1) {
			LogDebug(
				"[SPELL_LEVEL_OVERRIDE] spell=%u class=%d native=%d -> bestLevel=%d",
				spell_id, classId, nativeVal, bestLevel
			);
		}
		return bestLevel;
	}

	return nativeVal;
}

// Override PcZoneClient::GetPcSkillLimit so the Skills window can show skills that the server has granted
// via multiclassing, even if the base class would normally have a 0 cap (and the client would hide them).
DETOUR_TRAMPOLINE_EMPTY(int __fastcall PcZoneClient_GetPcSkillLimit_Tramp(void*, void*, int));
int __fastcall PcZoneClient_GetPcSkillLimit_Detour(void* This, void* edx, int skillId)
{
	g_last_hook_tag = Hook_GetPcSkillLimit;

	int nativeCap = 0;
	__try {
		nativeCap = PcZoneClient_GetPcSkillLimit_Tramp(This, edx, skillId);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (isDebugLoggingEnabled) {
			LogDebug("CLIENT_DETOUR stat=PcSkillLimit native trampoline faulted; falling back");
		}
		nativeCap = 0;
	}

	if (!isMulticlassUsableClassesOverrideEnabled || g_serverUsableClassesMask == 0) {
		return nativeCap;
	}

	if (nativeCap > 0) {
		return nativeCap;
	}

	// If the server has already given us a non-zero skill value, expose it with a minimal cap.
	// This makes newly-granted skills visible/usable without needing to reverse engineer full client cap tables.
	PCHARINFO2 ci2 = nullptr;
	__try { ci2 = GetCharInfo2(); }
	__except (EXCEPTION_EXECUTE_HANDLER) { ci2 = nullptr; }

	if (ci2 && skillId >= 0 && skillId < 0x64) {
		const int skillVal = static_cast<int>(ci2->Skill[skillId]);
		if (skillVal > 0) {
			if (isDebugLoggingEnabled) {
				static std::set<int> s_logged_skills;
				if (s_logged_skills.find(skillId) == s_logged_skills.end()) {
					LogDebug("CLIENT_DETOUR stat=PcSkillLimit skill=%d native=0 used=%d mask=0x%04X (first)", skillId, skillVal, (unsigned)(g_serverUsableClassesMask & 0xFFFF));
					s_logged_skills.insert(skillId);
				}
			}
			return skillVal;
		}
	}

	return nativeCap;
}

// ---- Multiclass class name overrides (char select + /who) ----
static uint8_t CountBits16(uint16_t bits)
{
	uint8_t count = 0;
	while (bits) {
		count += (bits & 1) ? 1 : 0;
		bits >>= 1;
	}
	return count;
}

static void BuildMulticlassAbbrevList(uint16_t mask, char* out, size_t out_size)
{
	if (!out || out_size == 0) {
		return;
	}
	out[0] = '\0';

	// 1..16
	static const char* kClass3[17] = {
		"", "WAR", "CLR", "PAL", "RNG", "SHD", "DRU", "MNK", "BRD",
		"ROG", "SHM", "NEC", "WIZ", "MAG", "ENC", "BST", "BER"
	};

	size_t used = 0;
	for (int class_id = 1; class_id <= 16; ++class_id) {
		const uint16_t bit = static_cast<uint16_t>(1u << (class_id - 1));
		if ((mask & bit) == 0) {
			continue;
		}

		const char* token = kClass3[class_id];
		if (!token || !token[0]) {
			continue;
		}

		if (used != 0) {
			if (used + 1 >= out_size) {
				break;
			}
			out[used++] = '/';
			out[used] = '\0';
		}

		const size_t token_len = strlen(token);
		if (used + token_len >= out_size) {
			break;
		}
		memcpy(out + used, token, token_len);
		used += token_len;
		out[used] = '\0';
	}
}

static bool IsReasonableClassBitmask(uint32_t mask32, uint8_t base_class_id = 0)
{
	if ((mask32 & 0xFFFF0000u) != 0) {
		return false;
	}
	const uint16_t mask = static_cast<uint16_t>(mask32 & 0xFFFFu);
	if (mask == 0) {
		return false;
	}
	if (base_class_id >= 1 && base_class_id <= 16) {
		const uint16_t base_bit = static_cast<uint16_t>(1u << (base_class_id - 1));
		if ((mask & base_bit) == 0) {
			return false;
		}
	}
	return true;
}

DETOUR_TRAMPOLINE_EMPTY(char* __fastcall CEverQuest_GetClassDesc_Tramp(void*, void*, int));
char* __fastcall CEverQuest_GetClassDesc_Detour(void* This, void* edx, int class_id)
{
	// Keep the original behavior unless we have a clear multiclass signal.
	char* native = nullptr;
	__try {
		native = CEverQuest_GetClassDesc_Tramp(This, edx, class_id);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		native = nullptr;
	}

	if (!isMulticlassClassNameOverrideEnabled) {
		return native;
	}

	static char s_buf[64];
	static int s_log_burst = 0;

	// Case 1: server sent a bitmask in a "class id" field (THJServer behavior for /who).
	if (class_id > 16 && IsReasonableClassBitmask(static_cast<uint32_t>(class_id))) {
		const uint16_t mask = static_cast<uint16_t>(class_id & 0xFFFF);
		if (CountBits16(mask) > 1) {
			BuildMulticlassAbbrevList(mask, s_buf, sizeof(s_buf));
			if (isDebugLoggingEnabled && s_log_burst++ < 10) {
				LogDebug("CLIENT_DETOUR stat=ClassDesc class_id=%d mask=0x%04X -> %s", class_id, (unsigned)mask, s_buf);
			}
			return s_buf;
		}
		return native;
	}

	// Case 2: character select screen (server encodes multiclass into selected model's Deity field).
	int gs = -1;
	__try { gs = GetGameState(); }
	__except (EXCEPTION_EXECUTE_HANDLER) { gs = -1; }
	if (gs == GAMESTATE_CHARSELECT && pLocalPlayer) {
		const uint32_t mask32 = static_cast<uint32_t>(pLocalPlayer->Data.Deity);
		const uint8_t base_class = static_cast<uint8_t>(pLocalPlayer->Data.Class);
		if (IsReasonableClassBitmask(mask32, base_class)) {
			const uint16_t mask = static_cast<uint16_t>(mask32 & 0xFFFF);
			if (CountBits16(mask) > 1) {
				g_charselectUsableClassesMask = mask;
				BuildMulticlassAbbrevList(mask, s_buf, sizeof(s_buf));
				if (isDebugLoggingEnabled && s_log_burst++ < 10) {
					LogDebug("CLIENT_DETOUR stat=ClassDesc (charselect) base=%u mask=0x%04X -> %s (cached for runtime fallback)", (unsigned)base_class, (unsigned)mask, s_buf);
				}
				return s_buf;
			}
		}
	}

	return native;
}

DETOUR_TRAMPOLINE_EMPTY(char* __fastcall CEverQuest_GetClassThreeLetterCode_Tramp(void*, void*, int));
char* __fastcall CEverQuest_GetClassThreeLetterCode_Detour(void* This, void* edx, int class_id)
{
	char* native = nullptr;
	__try {
		native = CEverQuest_GetClassThreeLetterCode_Tramp(This, edx, class_id);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		native = nullptr;
	}

	if (!isMulticlassClassNameOverrideEnabled) {
		return native;
	}

	static char s_buf[64];
	static int s_log_burst = 0;

	// Merchant window: when we return a multiclass-aggregated spell level for the base class, the client will label it
	// with the base class three-letter code (e.g. RNG). Rewrite it to the "best" owned class (e.g. MAG) for clarity.
	if (isMulticlassClassNameOverrideEnabled && ppMerchantWnd && pMerchantWnd) {
		const uint32_t now = GetTickCount();
		const uint32_t then = (uint32_t)InterlockedCompareExchange((volatile LONG*)&g_merchant_spell_tick, 0, 0);
		const uint8_t base = (uint8_t)InterlockedCompareExchange((volatile LONG*)&g_merchant_spell_base_class, 0, 0);
		const uint8_t best_cls = (uint8_t)InterlockedCompareExchange((volatile LONG*)&g_merchant_spell_best_class, 0, 0);
		const int best_lvl = (int)InterlockedCompareExchange((volatile LONG*)&g_merchant_spell_best_level, 0, 0);

		// Keep a generous window: the merchant UI may query spell levels and then render class strings later.
		// We rely on the short time window to prevent this from leaking into other UIs.
		if (then != 0 && (uint32_t)(now - then) < 2000 && base >= 1 && base <= 16 && best_cls >= 1 && best_cls <= 16 && best_lvl > 0 && best_lvl < 255) {
			if (class_id == (int)base && best_cls != base) {
				__try {
					return CEverQuest_GetClassThreeLetterCode_Tramp(This, edx, (int)best_cls);
				}
				__except (EXCEPTION_EXECUTE_HANDLER) {
					// fall through
				}
			}
		}
	}

	if (class_id > 16 && IsReasonableClassBitmask(static_cast<uint32_t>(class_id))) {
		const uint16_t mask = static_cast<uint16_t>(class_id & 0xFFFF);
		if (CountBits16(mask) > 1) {
			BuildMulticlassAbbrevList(mask, s_buf, sizeof(s_buf));
			if (isDebugLoggingEnabled && s_log_burst++ < 10) {
				LogDebug("CLIENT_DETOUR stat=Class3 class_id=%d mask=0x%04X -> %s", class_id, (unsigned)mask, s_buf);
			}
			return s_buf;
		}
		return native;
	}

	int gs = -1;
	__try { gs = GetGameState(); }
	__except (EXCEPTION_EXECUTE_HANDLER) { gs = -1; }
	if (gs == GAMESTATE_CHARSELECT && pLocalPlayer) {
		const uint32_t mask32 = static_cast<uint32_t>(pLocalPlayer->Data.Deity);
		const uint8_t base_class = static_cast<uint8_t>(pLocalPlayer->Data.Class);
		if (IsReasonableClassBitmask(mask32, base_class)) {
			const uint16_t mask = static_cast<uint16_t>(mask32 & 0xFFFF);
			if (CountBits16(mask) > 1) {
				g_charselectUsableClassesMask = mask;
				BuildMulticlassAbbrevList(mask, s_buf, sizeof(s_buf));
				if (isDebugLoggingEnabled && s_log_burst++ < 10) {
					LogDebug("CLIENT_DETOUR stat=Class3 (charselect) base=%u mask=0x%04X -> %s (cached for runtime fallback)", (unsigned)base_class, (unsigned)mask, s_buf);
				}
				return s_buf;
			}
		}
	}

	return native;
}

static bool MulticlassHasSpellcastingClass(uint32_t classesMask)
{
	// Bit positions are class_id-1 (Warrior=1 => bit0). Keep in sync with server-side classes bitmask.
	constexpr uint32_t kSpellcasters =
		(1u << (2 - 1))  | // Cleric
		(1u << (3 - 1))  | // Paladin
		(1u << (4 - 1))  | // Ranger
		(1u << (5 - 1))  | // Shadowknight
		(1u << (6 - 1))  | // Druid
		(1u << (8 - 1))  | // Bard
		(1u << (10 - 1)) | // Shaman
		(1u << (11 - 1)) | // Necromancer
		(1u << (12 - 1)) | // Wizard
		(1u << (13 - 1)) | // Magician
		(1u << (14 - 1)) | // Enchanter
		(1u << (15 - 1));  // Beastlord

	return (classesMask & kSpellcasters) != 0;
}

DETOUR_TRAMPOLINE_EMPTY(int __fastcall EQCharacter_IsSpellcaster_Tramp(void*, void*));
int __fastcall EQCharacter_IsSpellcaster_Detour(void* This, void* edx)
{
	g_last_hook_tag = Hook_IsSpellcaster;
	int nativeVal = 0;
	__try {
		nativeVal = EQCharacter_IsSpellcaster_Tramp(This, edx);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (isDebugLoggingEnabled) {
			LogDebug("CLIENT_DETOUR stat=IsSpellcaster native trampoline faulted; falling back");
		}
		nativeVal = 0;
	}
	if (!isMulticlassSpellUiOverrideEnabled || g_serverUsableClassesMask == 0) {
		return nativeVal;
	}

	return MulticlassHasSpellcastingClass(g_serverUsableClassesMask) ? 1 : nativeVal;
}

DETOUR_TRAMPOLINE_EMPTY(int __fastcall EQCharacter_IsSpellcaster2_Tramp(void*, void*, int, int, int, int));
int __fastcall EQCharacter_IsSpellcaster2_Detour(void* This, void* edx, int a1, int a2, int a3, int a4)
{
	g_last_hook_tag = Hook_IsSpellcaster2;
	int nativeVal = 0;
	__try {
		nativeVal = EQCharacter_IsSpellcaster2_Tramp(This, edx, a1, a2, a3, a4);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (isDebugLoggingEnabled) {
			LogDebug("CLIENT_DETOUR stat=IsSpellcaster2 native trampoline faulted; falling back");
		}
		nativeVal = 0;
	}
	if (!isMulticlassSpellUiOverrideEnabled || g_serverUsableClassesMask == 0) {
		return nativeVal;
	}

	return MulticlassHasSpellcastingClass(g_serverUsableClassesMask) ? 1 : nativeVal;
}

DETOUR_TRAMPOLINE_EMPTY(int __fastcall EQCharacter_IsSpellcaster3_Tramp(void*, void*));
int __fastcall EQCharacter_IsSpellcaster3_Detour(void* This, void* edx)
{
	g_last_hook_tag = Hook_IsSpellcaster3;
	int nativeVal = 0;
	__try {
		nativeVal = EQCharacter_IsSpellcaster3_Tramp(This, edx);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (isDebugLoggingEnabled) {
			LogDebug("CLIENT_DETOUR stat=IsSpellcaster3 native trampoline faulted; falling back");
		}
		nativeVal = 0;
	}
	if (!isMulticlassSpellUiOverrideEnabled || g_serverUsableClassesMask == 0) {
		return nativeVal;
	}

	return MulticlassHasSpellcastingClass(g_serverUsableClassesMask) ? 1 : nativeVal;
}

DETOUR_TRAMPOLINE_EMPTY(int __fastcall CSpellBookWnd_CanStartMemming_Tramp(void*, void*, int));
int __fastcall CSpellBookWnd_CanStartMemming_Detour(void* This, void* edx, int spellId)
{
	g_last_hook_tag = Hook_CanStartMemming;
	int nativeVal = 0;
	__try {
		nativeVal = CSpellBookWnd_CanStartMemming_Tramp(This, edx, spellId);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (isDebugLoggingEnabled) {
			LogDebug("CLIENT_DETOUR stat=CanStartMemming native trampoline faulted; falling back");
		}
		nativeVal = 0;
	}

	if (isDebugLoggingEnabled) {
		static std::set<int> s_logged_memming_spells;
		if (s_logged_memming_spells.find(spellId) == s_logged_memming_spells.end()) {
			LogDebug("[CAN_START_MEMMING] spellId=%d nativeVal=%d serverMask=0x%04X hasSpellcaster=%d (first)",
				spellId, nativeVal, g_serverUsableClassesMask,
				MulticlassHasSpellcastingClass(g_serverUsableClassesMask) ? 1 : 0);
			s_logged_memming_spells.insert(spellId);
		}
	}

	if (!isMulticlassSpellUiOverrideEnabled || g_serverUsableClassesMask == 0) {
		return nativeVal;
	}

	// If multiclass has a spellcasting class, allow memorization
	int result = MulticlassHasSpellcastingClass(g_serverUsableClassesMask) ? 1 : nativeVal;
	if (isDebugLoggingEnabled && result != nativeVal) {
		LogDebug("[CAN_START_MEMMING] OVERRIDE spellId=%d native=%d result=%d", spellId, nativeVal, result);
	}
	return result;
}

// NOTE: TotalEffect detour removed - attempting to detour member functions
// with the wrong calling convention caused instability on character select.
// We keep the EdgeStat cache and Max_* detours active; further member-
// function detours should be added only after confirming exact calling
// conventions and offsets for this client build.

signed int ProcessGameEvents_Hook()
{
   DWORD oldTimeGetTimeVal = 0;
   return return_ProcessGameEvents();
}

// ---- Server-authoritative stats: deferred detour install (main thread) ----
//
// We cannot reliably install these detours during DLL_PROCESS_ATTACH or during early
// client startup (pre-charselect). We arm a lightweight main-thread hook and only
// install the real detours once the client is fully in-game.

extern CRITICAL_SECTION gDetourCS;

static volatile LONG g_server_stats_arm_installed = 0;
static volatile LONG g_server_stats_detours_installed = 0;
static volatile LONG g_server_stats_wait_logged = 0;

DETOUR_TRAMPOLINE_EMPTY(BOOL Trampoline_ProcessGameEvents_StatsInstall(VOID));
static BOOL Detour_ProcessGameEvents_StatsInstall(VOID)
{
	// Main thread heartbeat; safe place to query game state and install detours.
	EdgeStats_MaybeInstallDetoursFromMainThread();
	return Trampoline_ProcessGameEvents_StatsInstall();
}

static void InstallServerAuthoritativeStatsDetours_Now()
{
	if (InterlockedCompareExchange(&g_server_stats_detours_installed, 1, 0) != 0) {
		return;
	}

	if (!baseAddress) {
		LogDebug("Server-authoritative stats: baseAddress is null; cannot install detours");
		InterlockedExchange(&g_server_stats_detours_installed, 0);
		return;
	}

	if (!EQ_Character__Max_HP || !EQ_Character__Cur_HP || !EQ_Character__Max_Mana || !EQ_Character__Max_Endurance) {
		LogDebug("Server-authoritative stats: required offsets not initialized; cannot install detours");
		InterlockedExchange(&g_server_stats_detours_installed, 0);
		return;
	}

	EnterCriticalSection(&gDetourCS);
	__try {
		const DWORD handle_world_msg = (((DWORD)0x004C3250 - 0x400000) + baseAddress);
		LogDebug("Server-authoritative stats: installing detours NOW (baseAddress=0x%08X)", (unsigned)baseAddress);
		LogDetourTargetBytes("HandleWorldMessage", (DWORD)handle_world_msg);
		LogDetourTargetBytes("EQ_Character__Max_HP", (DWORD)EQ_Character__Max_HP);
		LogDetourTargetBytes("EQ_Character__Cur_HP", (DWORD)EQ_Character__Cur_HP);
		LogDetourTargetBytes("EQ_Character__Max_Mana", (DWORD)EQ_Character__Max_Mana);
		LogDetourTargetBytes("EQ_Character__Max_Endurance", (DWORD)EQ_Character__Max_Endurance);
		if (EQ_Character__GetUsableClasses) {
			LogDetourTargetBytes("EQ_Character__GetUsableClasses", (DWORD)EQ_Character__GetUsableClasses);
		}
		if (EQ_Character__IsSpellcaster) {
			LogDetourTargetBytes("EQ_Character__IsSpellcaster", (DWORD)EQ_Character__IsSpellcaster);
		}
		if (EQ_Character__IsSpellcaster_2) {
			LogDetourTargetBytes("EQ_Character__IsSpellcaster_2", (DWORD)EQ_Character__IsSpellcaster_2);
		}
		if (EQ_Character__IsSpellcaster_3) {
			LogDetourTargetBytes("EQ_Character__IsSpellcaster_3", (DWORD)EQ_Character__IsSpellcaster_3);
		}
		if (EQ_Spell__GetSpellLevelNeeded) {
			LogDetourTargetBytes("EQ_Spell__GetSpellLevelNeeded", (DWORD)EQ_Spell__GetSpellLevelNeeded);
		}
		if (CSpellBookWnd__CanStartMemming) {
			LogDetourTargetBytes("CSpellBookWnd__CanStartMemming", (DWORD)CSpellBookWnd__CanStartMemming);
		}
		if (PcZoneClient__GetPcSkillLimit) {
			LogDetourTargetBytes("PcZoneClient__GetPcSkillLimit", (DWORD)PcZoneClient__GetPcSkillLimit);
		}
		LogDetourTargetBytes("__GetGaugeValueFromEQ", __GetGaugeValueFromEQ);
		LogDetourTargetBytes("__GetLabelFromEQ", __GetLabelFromEQ);

		EzDetour((DWORD)handle_world_msg, HandleWorldMessage_Detour, HandleWorldMessage_Trampoline);
		EzDetour((DWORD)EQ_Character__Max_HP, EQCharacter_MaxHP_Detour, EQCharacter_MaxHP_Tramp);
		EzDetour((DWORD)EQ_Character__Cur_HP, EQCharacter_CurHP_Detour, EQCharacter_CurHP_Tramp);
		EzDetour((DWORD)EQ_Character__Max_Mana, EQCharacter_MaxMana_Detour, EQCharacter_MaxMana_Tramp);
		EzDetour((DWORD)EQ_Character__Max_Endurance, EQCharacter_MaxEnd_Detour, EQCharacter_MaxEnd_Tramp);
		if (EQ_Character__GetUsableClasses && isMulticlassUsableClassesOverrideEnabled) {
			EzDetour((DWORD)EQ_Character__GetUsableClasses, EQCharacter_GetUsableClasses_Detour, EQCharacter_GetUsableClasses_Tramp);
		}
		if (isMulticlassSpellUiOverrideEnabled) {
			if (EQ_Character__IsSpellcaster) {
				EzDetour((DWORD)EQ_Character__IsSpellcaster, EQCharacter_IsSpellcaster_Detour, EQCharacter_IsSpellcaster_Tramp);
			}
			if (EQ_Character__IsSpellcaster_2) {
				EzDetour((DWORD)EQ_Character__IsSpellcaster_2, EQCharacter_IsSpellcaster2_Detour, EQCharacter_IsSpellcaster2_Tramp);
			}
			if (EQ_Character__IsSpellcaster_3) {
				EzDetour((DWORD)EQ_Character__IsSpellcaster_3, EQCharacter_IsSpellcaster3_Detour, EQCharacter_IsSpellcaster3_Tramp);
			}
			if (CSpellBookWnd__CanStartMemming) {
				EzDetour((DWORD)CSpellBookWnd__CanStartMemming, CSpellBookWnd_CanStartMemming_Detour, CSpellBookWnd_CanStartMemming_Tramp);
			}
		}
		// Re-enabled: Needed for multiclass spell memorization time to work correctly
		if (EQ_Spell__GetSpellLevelNeeded && isMulticlassUsableClassesOverrideEnabled) {
			EzDetour((DWORD)EQ_Spell__GetSpellLevelNeeded, EQSpell_GetSpellLevelNeeded_Detour, EQSpell_GetSpellLevelNeeded_Tramp);
		}
		if (PcZoneClient__GetPcSkillLimit && isMulticlassUsableClassesOverrideEnabled) {
			EzDetour((DWORD)PcZoneClient__GetPcSkillLimit, PcZoneClient_GetPcSkillLimit_Detour, PcZoneClient_GetPcSkillLimit_Tramp);
		}
		InstallUiDetours();
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		LogDebug("Server-authoritative stats: exception while installing detours");
		InterlockedExchange(&g_server_stats_detours_installed, 0);
	}
	LeaveCriticalSection(&gDetourCS);
}

void EdgeStats_MaybeInstallDetoursFromMainThread()
{
	if (!isServerAuthoritativeStatsEnabled) {
		return;
	}

	int gs = -1;
	__try { gs = GetGameState(); }
	__except (EXCEPTION_EXECUTE_HANDLER) { gs = -1; }

	// Only install once we are in-game.
	if (gs != GAMESTATE_INGAME) {
		if (InterlockedCompareExchange(&g_server_stats_wait_logged, 1, 0) == 0) {
			LogDebug("Server-authoritative stats: waiting for GAMESTATE_INGAME (gs=%d gGameState=%lu)", gs, (unsigned long)gGameState);
		}
		return;
	}

	InstallServerAuthoritativeStatsDetours_Now();
}

static void EnsureProcessGameEventsDetourForStatsInstalled()
{
	// When MQ2 injects are enabled, MQ2Pulse already detours ProcessGameEvents.
	// We'll piggyback via MQ2Pulse's detour instead (see MQ2Pulse.cpp).
	if (isMQInjectsEnabled) {
		LogDebug("Server-authoritative stats: MQ2 injects enabled; skipping local ProcessGameEvents detour");
		return;
	}

	if (!ProcessGameEvents) {
		LogDebug("Server-authoritative stats: ProcessGameEvents offset not initialized yet");
		return;
	}

	LogDebug("Server-authoritative stats: arming ProcessGameEvents detour (addr=0x%08X)", (unsigned)(DWORD)ProcessGameEvents);
	LogDetourTargetBytes("ProcessGameEvents", (DWORD)ProcessGameEvents);
	EzDetour(ProcessGameEvents, Detour_ProcessGameEvents_StatsInstall, Trampoline_ProcessGameEvents_StatsInstall);
}

void EnsureServerAuthoritativeStatsInstallArmed()
{
	if (InterlockedCompareExchange(&g_server_stats_arm_installed, 1, 0) != 0) {
		return;
	}
	EnsureProcessGameEventsDetourForStatsInstalled();
}

void SkipLicense()
{
	//char str[255];
	//DWORD ff;
	//sprintf(str, "%d",*(DWORD*)(0x807DFC));
	//MessageBox(NULL, str, NULL, MB_OK);
	//DWORD offset = (DWORD)eqmain_dll + 0x255D2;
	//const char test1[] = { 0xEB }; // , 0x90, 0x90, 0x90, 0x90, 0x90};
	//PatchA((DWORD*)offset, &test1, sizeof(test1));

}

void SkipSplash()
{
	// Set timer for intro splash screens to 0

	////gypsies
	//const char test1[] = { 0x90, 0x90, 0x90 };
	//PatchA((DWORD*)0x004798ED, &test1, sizeof(test1));

	////skeletons
	//const char test2[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
	//PatchA((DWORD*)0x0048276F, &test2, sizeof(test2));

	////PVP Attack
	//const char test3[] = { 0xEB, 0x39, 0x90, 0x90, 0x90, 0x90 };
	//PatchA((DWORD*)0x0047EC22, &test3, sizeof(test3));

	//const char test4[] = { 0xEB, 0x09, 0x90, 0x90, 0x90, 0x90 };
	//PatchA((DWORD*)0x0047EC78, &test4, sizeof(test4));

	//const char test5[] = { 0xEB, 0xD8, 0x90, 0x90, 0x90, 0x90 };
	//PatchA((DWORD*)0x000047EC83, &test5, sizeof(test5)); // 0047EC83 | EB D8                    | jmp eqgame.47EC5D                       |
	//item bonuses
	/*const char test3[] = { 0x2a, 0x06 };
	PatchA((DWORD*)0x0051E323, &test3, sizeof(test3));
	PatchA((DWORD*)0x0051E521, &test3, sizeof(test3));
	PatchA((DWORD*)0x0051E5FB, &test3, sizeof(test3));*/
	/*const char test1[] = { 0x00, 0x00 }


	//gypsies

	DWORD offset = (DWORD)eqmain_dll + 0x21998;
	PatchA((DWORD*)offset, &test1, sizeof(test1));

	const char test2[] = { 0x01 }; // , 0x90, 0x90, 0x90, 0x90, 0x90};

	const char test3[] = { 0x90, 0x90, 0x90, 0xEB, 0x36 }; // , 0x90, 0x90, 0x90, 0x90, 0x90};

	const char test4[] = { 0x57 }; // , 0x90, 0x90, 0x90, 0x90, 0x90};

	const char test5[] = { 0x90, 0x90, 0x90, 0x90 }; // , 0x90, 0x90, 0x90, 0x90,


	const char test6[] = { 0xE9, 0xB6, 0x02, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }; // , 0x90, 0x90, 0x90, 0x90,

	const char test7[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

	const char test8[] = { 0x3B, 0xC1, 0x81, 0xC7, 0xE8, 0x03, 0x00, 0x00 };

	//Dual Wield for rangers
	PatchA((DWORD*)0x004D2800, &test2, sizeof(test2));
	PatchA((DWORD*)0x004D280C, &test2, sizeof(test2));
	PatchA((DWORD*)0x004D2828, &test2, sizeof(test2));
	//Meditate
	PatchA((DWORD*)0x004D304C, &test2, sizeof(test2));
	//Double Attack
	PatchA((DWORD*)0x004D25FD, &test2, sizeof(test2));

	//Mana at any level on rangers fix
	PatchA((DWORD*)0x004B949C, &test3, sizeof(test3));

	//Target label shows name
	PatchA((DWORD*)0x0043622A, &test4, sizeof(test4));

	//Instantly scribe spells
	PatchA((DWORD*)0x00043514F, &test5, sizeof(test5));

	//No local coord evac on zoning fail
	//PatchA((DWORD*)0x0005461F4, &test6, sizeof(test6));

	//Bard songs have modifier when calculating the value sent by the server, even if you're not a bard. Use 1.0 multiplier for non-bard spells on the server.
	PatchA((DWORD*)0x004CA16A, &test7, sizeof(test7));

	//Increase stat cap increase granted by AAs to be 1000 instead of 25 max.
	PatchA((DWORD*)0x004B7D45, &test8, sizeof(test8));
	*/

	//PatchA((void*)0x0050C06E, "\x00", 1); //Group pet health removal
	//PatchA((void*)0x00545001, "\xEB", 1); //Self pet health removal
	//PatchA((void*)0x005540D1, "\xEB", 1); //Skills (open/inspect) removal
	//PatchA((void*)0x0046356A, "\xE9\xD6\x0D\x00\x00\x90\x90\x90\x90", 9); //Find Window removal (Button exists, but does nothing.)
	//PatchA((void*)0x00464141, "\xE9\xFF\x01\x00\x00\x90\x90\x90\x90", 9); //DZ Window (All forms of hotkey disabled.)
	//PatchA((void*)0x004642F2, "\xEB\x51\x90", 3); //Task Selection Window disabled.
	//PatchA((void*)0x004A71D6, "\xE9\x2D\x01\x00", 4); //Left Click Shows Target Help Disabled and cannot be re-enabled.
	//return_ActivateDet = (Activate_t)DetourFunction((PBYTE)CXWndActivateAddr, (PBYTE)CXWndActivateHook); // Almost all non-classic windows have been disabled.
	//return_ValueSellMerchantDet = (ValueSellMerchant_t)DetourFunction((PBYTE)ValueSellMerchantAddr, (PBYTE)ValueSellMerchantHook); // Items sold to greedy merchants that are sold at 0cp are now sold at 1cp
	//return_SetCCreateCameraDet = (SetCCreateCamera_t)DetourFunction((PBYTE)0x507b30, (PBYTE)SetCCreateCameraHook); // Character Creation screen hook for position based on class/race/deity.
	//return_SelectCharacterDet = (SelectCharacter_t)DetourFunction((PBYTE)SelectCharacterAddr, (PBYTE)SelectCharacterHook); // Character Selection screen hook for position based on class/race/deity.
	//PatchA((void*)0x004AAA15, "\xB8", 1); //For Character Selection. Tells client to load "load.s3d" instead of "clz.eqg".
	//PatchA((void*)0x0063EF73, "pickchar.xmi", 12); //Writes "pickchar.xmi" to unused memory in eqgame.exe
	//PatchA((void*)0x009C8C2C, "load\x00", 5); // Use load instead of "CLZ"
	//PatchA((void*)0x0044B7D8, "\x68\x73\xEF\x63", 4); //Makes a PUSH load the above into memory instead of "eqtheme.mp3" for future use.
	//PatchA((void*)0x0044B83D, "\xEB", 1); //Force-loads "opener4.xmi" when opening the character selection screen into theme position 1.
	//PatchA((void*)0x0044B895, "\x14", 1); //Instead of assigning "opener4.xmi" to both positions which Titanium does by default, we overwrite position 4 (char select) with the pickchar.xmi asset

}

void PatchSaveBypass()
{

}

DWORD wpsaddress = 0;
DWORD swAddress = 0;
DWORD cwAddress = 0;
DWORD swlAddress = 0;
DWORD uwAddress = 0;

PVOID pHandler;
bool bInitalized=false;


// DirectXSetupGetVersion
int __stdcall DirectXSetupGetVersion(DWORD *lpdwVersion, DWORD *lpdwMinorVersion)
{
	return 1;
}

BOOL __stdcall SetDeviceGammaRamp_Hook(HDC hdc, LPVOID lpRamp)
{
	return 1;
}

extern CRITICAL_SECTION gDetourCS;
void InitHooks()
{
	LogDebug("InitHooks: Started");
	LogDebug(
		"Multiclass options: usable_override=%d spell_ui_override=%d class_name_override=%d usable_verbose=%d debug=%d",
		isMulticlassUsableClassesOverrideEnabled ? 1 : 0,
		isMulticlassSpellUiOverrideEnabled ? 1 : 0,
		isMulticlassClassNameOverrideEnabled ? 1 : 0,
		isMulticlassUsableClassesVerboseLoggingEnabled ? 1 : 0,
		isDebugLoggingEnabled ? 1 : 0
	);
	InstallCrashDiagnostics();
	//rename("arena.eqg", "arena.eqg.bak");
	//rename("highpasshold.eqg", "highpasshold.eqg.bak");
	//rename("nektulos.eqg", "nektulos.eqg.bak");
	//rename("lavastorm.eqg", "lavastorm.eqg.bak");
	LogDebug("InitHooks: Calling InitOffsets");
	InitOffsets();
	LogDebug("InitHooks: InitOffsets returned");
	GetEQPath(gszEQPath);
	if (isMQ2CoreDetoursEnabled) {
		LogDebug("InitHooks: Initializing MQ2 core detours");
		InitializeMQ2Detours();
	} else {
		LogDebug("InitHooks: MQ2 core detours disabled, initializing detour critical section only");
		InitializeCriticalSection(&gDetourCS);
	}

	if (isMQ2ParserEnabled) {
		LogDebug("InitHooks: Initializing MQ2 parser/data");
		InitializeParser();
	}

	if (isMQInjectsEnabled) {
		LogDebug("InitHooks: Applying mq2 injects");
		DebugSpew("Applying mq2 injects");
		InitializeDisplayHook();
		InitializeChatHook();
		InitializeMQ2Commands();
		InitializeMQ2Pulse();
		InitializeMQ2Spawns();
		InitializeMapPlugin();
		if (isMQ2ItemDisplayEnabled) {
			InitializeMQ2ItemDisplay();
		}
		InitializeMQ2Labels();
		if (isMQ2KeyBindsEnabled) {
			InitializeMQ2KeyBinds();
		}
	} else {
		LogDebug("InitHooks: MQ2 injects disabled");
	}

	if (!baseAddress) {
		LogDebug("InitHooks: baseAddress is null, returning");
		return;
	}
	InitOptions();

	// Multiclass display hooks (safe to install before in-game; used by /who and character select).
	if (isMulticlassClassNameOverrideEnabled && CEverQuest__GetClassDesc && CEverQuest__GetClassThreeLetterCode) {
		EnterCriticalSection(&gDetourCS);
		__try {
			LogDetourTargetBytes("CEverQuest__GetClassDesc", (DWORD)CEverQuest__GetClassDesc);
			LogDetourTargetBytes("CEverQuest__GetClassThreeLetterCode", (DWORD)CEverQuest__GetClassThreeLetterCode);
			EzDetour((DWORD)CEverQuest__GetClassDesc, CEverQuest_GetClassDesc_Detour, CEverQuest_GetClassDesc_Tramp);
			EzDetour((DWORD)CEverQuest__GetClassThreeLetterCode, CEverQuest_GetClassThreeLetterCode_Detour, CEverQuest_GetClassThreeLetterCode_Tramp);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			LogDebug("Multiclass class name overrides: exception while installing detours");
		}
		LeaveCriticalSection(&gDetourCS);
	}

	DWORD var = (((DWORD)0x008C4CE0 - 0x400000) + baseAddress);
/*
	if (isCpuSpeedFixEnabled) {


		CPUID cpuID(0x80000007); // Get CPU vendor

		bool isCandidate = false;
		if ((cpuID.EDX() & (1 << 8)) != 0) {
			DebugSpew("cpu has CMPXCHG8 enabled"); //https://en.wikipedia.org/wiki/CPUID CMPXCHG8 bitflag 8 on edx
			isCandidate = true;
		}

		if (isCandidate && CalcCpuTicks_x && frequency_x) {
			DebugSpew("cpu speed fix needed, applying trampoline");
			CalcCpuTicks = FixOffset(CalcCpuTicks_x);
			freq = reinterpret_cast<uint64_t*>(FixOffset(frequency_x)); // offset to low part of 64 bit var

			// race here, hook CalcCpuTicks if we're early
			// don't allow this to change in game because it will cause a freeze
			EzDetourwName(CalcCpuTicks, &CalcCpuTicks_Detour, &CalcCpuTicks_Trampoline, "MQ2CpuSpeedFix_CalcCpuTicks");
			if (gGameState != GAMESTATE_CHARSELECT && gGameState != GAMESTATE_INGAME) {
				adjustFreq();
			}
		}
		else {
			DebugSpew("cpu is not candidate for speed fix");
		}
	} */
	if (isHeroicDisabled) {
		DebugSpew("disabling heroic stats");
		var = (((DWORD)0x0044410C - 0x400000) + baseAddress);
		PatchA((DWORD*)var, "\x90\x90\xEB", 3); // Remove heroic Stamina
			FILE* f = nullptr;

		DebugSpew("enabling old model mount support");
		var = (((DWORD)0x0058DE28 - 0x400000) + baseAddress);
		PatchA((DWORD*)var, "\x32\xC0", 2); // No mount models
	}

	if (isAllowIllegalAugmentsEnabled) {
		var = (((DWORD)0x006a8448 - 0x400000) + baseAddress);
		PatchA((DWORD*)var, "\x90\x90\x90\x90\x90\x90", 6);
	}

	var = (((DWORD)0x004C3250 - 0x400000) + baseAddress);
	if (isServerAuthoritativeStatsEnabled) {
		// NOTE: Installing these detours too early (and especially from DllMain during
		// DLL_PROCESS_ATTACH) can destabilize the client before character select.
		// We defer detour installation until we are safely in-game on the main thread.
		LogDebug("Server-authoritative stats noticed: deferring detour install until in-game (baseAddress=0x%08X)", (unsigned)baseAddress);
		EnsureServerAuthoritativeStatsInstallArmed();
	}

	// TotalEffect detour disabled (was causing instability). See comment above.

	if (isSpellDataCRCEnabled) {
		DebugSpew("enabling spell data crc");
		//basedata as spell CRC begin
		var = (((DWORD)0x00AA6980 - 0x400000) + baseAddress);
		PatchA((DWORD*)var, "spells_us.txt", 13);

		DWORD varToPatch = (((DWORD)0x00AA6980 - 0x400000) + baseAddress);
		var = (((DWORD)0x004EEAAB - 0x400000) + baseAddress);
		PatchA((DWORD*)var, (void*)&varToPatch, 4);
		//basedata as spell CRC end
	}

	if (isCombatDamageDoubleAppliedFixEnabled) {
		var = (((DWORD)0x0045385D - 0x400000) + baseAddress);
		PatchA((DWORD*)var, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 30); //hp damage in combat abilities fix
	}


	//#pragma comment(lib, "Iphlpapi.lib")

	if (isMaxHPFixEnabled) {
		var = (((DWORD)0x00444158 - 0x400000) + baseAddress); // Fix max HP cap
		PatchA((DWORD*)var, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 12);

		var = (((DWORD)0x00449F64 - 0x400000) + baseAddress); // Fix current HP cap
		PatchA((DWORD*)var, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 13);
	}

	//0065CC71
	//var = (((DWORD)0x0065CC09 - 0x400000) + baseAddress);
	//PatchA((DWORD*)var, "\x90\x90\x90\x90",
		// 4); // Fix tradeskill containers

	//DWORD varArray = (((DWORD)0x009BFF6D - 0x400000) + baseAddress);
	//var = (((DWORD)0x004ED062 - 0x400000) + baseAddress);
	//PatchA((DWORD*)var, "\x08", 1); // Link stuff

	//var = (((DWORD)0x004ED083 - 0x400000) + baseAddress);
	//PatchA((DWORD*)var, "\x08", 1); // Link stuff

	//var = (((DWORD)0x004ED03B - 0x400000) + baseAddress);
	//PatchA((DWORD*)var, "\x4C", 1); // Link stuff
	//var = (((DWORD)0x004ED051 - 0x400000) + baseAddress);
	//PatchA((DWORD*)var, (DWORD*)&varArray, 4); // Link stuff
	//var = (((DWORD)0x004ED072 - 0x400000) + baseAddress);
	//PatchA((DWORD*)var, (DWORD*)&varArray, 4); // Link stuff
	//var = (((DWORD)0x007BBC9A - 0x400000) + baseAddress);
	//PatchA((DWORD*)var, (DWORD*)&varArray, 4); // Link stuff
	//var = (((DWORD)0x007BBD77 - 0x400000) + baseAddress);
	//PatchA((DWORD*)var, (DWORD*)&varArray, 4); // Link stuff

	//var = (((DWORD)0x009BFF6D - 0x400000) + baseAddress);
	//PatchA((DWORD*)var, "\x25\x30\x38\x58", 4); // Link stuff

	//var = (((DWORD)0x00A1ACE0 - 0x400000) + baseAddress);
	//PatchA((DWORD*)var, "\x4F", 1); // Link stuff

	//var = (((DWORD)0x0063C36F - 0x400000) + baseAddress);
	//PatchA((DWORD*)var, "\x90\x90\x90\x90", 4); // Bazaar trader anywhere

	//var = (((DWORD)0x0063978E - 0x400000) + baseAddress);
	//PatchA((DWORD*)var, "\x90\x90\xEB", 3); // Bazaar trader anywhere

	//var = (((DWORD)0x006AB6AF - 0x400000) + baseAddress);
	//PatchA((DWORD*)var, "\x90\x90\xE9\xA5\x00", 5); // nop / jmp dmg bonus

	//var = (((DWORD)0x006AB6B6 - 0x400000) + baseAddress);
	//PatchA((DWORD*)var, "\x90", 1); // nop / jmp dmg bonus

	//var = (((DWORD)0x00632DE6 - 0x400000) + baseAddress);
	//PatchA((DWORD*)var, "\x90\x90", 2); // nop trader check
	//var = ((0x00507b30 - 0x400000) + baseAddress);
	//return_SetCCreateCameraDet = (SetCCreateCamera_t)DetourFunction((PBYTE)var, (PBYTE)SetCCreateCameraHook);

	if (isPatchmeDisabled) {
		DebugSpew("disabling patchme");
		var = (((DWORD)0x005FE751 - 0x400000) + baseAddress);
		PatchA((DWORD*)var, "\xEB\x1C\x90\x90\x90", 5); // patchme req bypass
	}

	if (isFoodDrinkSpamDisabled) {
		DebugSpew("disabling food drink spam");
		var = (((DWORD)0x0045AE9F - 0x400000) + baseAddress);
		PatchA((DWORD*)var, "\x90\x90\xE9\x76\x03\x00\x00\x90",
			8); // Fix food/drink spam
	}

	if (isMQ2PreventionEnabled) {
		DebugSpew("mq2 prevention enabled");
		auto charToBreak = rand();

		var = (((DWORD)0x009DD250 - 0x400000) + baseAddress);
		PatchA((DWORD*)var, (DWORD*)&charToBreak, 4);

		charToBreak = rand();
		var = (((DWORD)0x009DD254 - 0x400000) + baseAddress);
		PatchA((DWORD*)var, (DWORD*)&charToBreak, 4);

		charToBreak = rand();
		var = (((DWORD)0x009DD258 - 0x400000) + baseAddress);
		PatchA((DWORD*)var, (DWORD*)&charToBreak, 4);

		charToBreak = rand();
		var = (((DWORD)0x009DD25C - 0x400000) + baseAddress);
		PatchA((DWORD*)var, (DWORD*)&charToBreak, 4);

		charToBreak = rand();
		var = (((DWORD)0x009DD260 - 0x400000) + baseAddress);
		PatchA((DWORD*)var, (DWORD*)&charToBreak, 4);
	}


	if (isGammaRestoreOnCrashEnabled) {
		DebugSpew("Applying gamma restore on crash fix");
		HMODULE hkernel32Mod = GetModuleHandle("kernel32.dll");
		DWORD gmfadress = (DWORD)GetProcAddress(hkernel32Mod, "GetModuleFileNameA");
		EzDetour(gmfadress, GetModuleFileNameA_detour, GetModuleFileNameA_tramp);

		HMODULE gdi32mod = GetModuleHandle("gdi32.dll");
		DWORD jmpToDeviceGamma = (DWORD)GetProcAddress(gdi32mod, "SetDeviceGammaRamp");
		EzDetour(jmpToDeviceGamma, SetDeviceGammaRamp_Hook, SetDeviceGammaRamp_Trampoline);
	}

	/*
	* // Disable for the time being, causes crashes, offsets appear to be off
	if (isNativeGammaEnabled) {
		var = (((DWORD)0x004972AC - 0x400000) + baseAddress); // Nop the gamma slider
		PatchA((DWORD*)var, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90",35);

		var = (((DWORD)0x00709AC1 - 0x400000) + baseAddress); // Nop the gamma slider
		PatchA((DWORD*)var, "\x90\x90\x90\x90\x90\x90\xE9\xD0\x00\x90", 10);
	}*/
	LogDebug("InitHooks: Finished");

}

void ExitHooks()
{
	if(!bInitalized)
	{
		return;
	}

	//RemoveDetour(0x4E829F); // HandleWorldMessage
}
BOOL ParseINIFile(PCHAR lpINIPath)
{
   CHAR Filename[MAX_STRING] = {0};
   CHAR MQChatSettings[MAX_STRING] = {0};
   CHAR CustomSettings[MAX_STRING] = {0};
   CHAR ClientINI[MAX_STRING] = {0};
   CHAR szBuffer[MAX_STRING] = {0};
   CHAR ClientName[MAX_STRING] = {0};
   CHAR FilterList[MAX_STRING * 10] = {0};
   GetEQPath(gszEQPath);

   sprintf(Filename, "%s\\Edge.ini", lpINIPath);
   sprintf(ClientINI, "%s\\eqgame.ini", lpINIPath);
   strcpy(gszINIFilename, Filename);

   DebugSpew("Expected Client version: %s %s", __ExpectedVersionDate,
             __ExpectedVersionTime);
   DebugSpew("    Real Client version: %s %s", __ActualVersionDate, __ActualVersionTime);

   // note: __ClientOverride is always #defined as 1 or 0
#if (!__ClientOverride)
   if (strncmp(__ExpectedVersionDate, (const char*)__ActualVersionDate,
               strlen(__ExpectedVersionDate)) ||
       strncmp(__ExpectedVersionTime, (const char*)__ActualVersionTime,
               strlen(__ExpectedVersionTime))) {
      MessageBox(NULL, "Incorrect client version", "Edge", MB_OK);
      return FALSE;
   }
#endif

      gbAlwaysDrawMQHUD = false;
   gbHUDUnderUI = false;

   DefaultFilters();

   return TRUE;
}


bool __cdecl MQ2Initialize()
{
   if (!InitOffsets()) {
      DebugSpewAlways("InitOffsets returned false - thread aborted.");
      g_Loaded = FALSE;
      return false;
   }

   if (!ParseINIFile("eqclient.ini")) {
      DebugSpewAlways("ParseINIFile returned false - thread aborted.");
      g_Loaded = FALSE;
      return false;
   }
   srand((unsigned int)time(0));
   ZeroMemory(gDiKeyName, sizeof(gDiKeyName));
   unsigned long i;
   for (i = 0; gDiKeyID[i].Id; i++) {
      gDiKeyName[gDiKeyID[i].Id] = gDiKeyID[i].szName;
   }

   ZeroMemory(szEQMappableCommands, sizeof(szEQMappableCommands));
   for (i = 0; i < nEQMappableCommands; i++) {
      if ((DWORD)EQMappableCommandList[i] == 0 ||
          (DWORD)EQMappableCommandList[i] > (DWORD)__AC1_Data)
         continue;
      szEQMappableCommands[i] = EQMappableCommandList[i];
   }
   gnNormalEQMappableCommands = i;

   // as long nEQMappableCommands is right and these remain at the end, these
   // should never need updating who uses the unknowns anyway? - ieatacid
   szEQMappableCommands[nEQMappableCommands - 23] = "UNKNOWN0x10d";
   szEQMappableCommands[nEQMappableCommands - 22] = "UNKNOWN0x10e";
   szEQMappableCommands[nEQMappableCommands - 21] = "UNKNOWN0x10f";
   szEQMappableCommands[nEQMappableCommands - 20] = "UNKNOWN0x110";
   szEQMappableCommands[nEQMappableCommands - 19] = "CHAT_SEMICOLON";
   szEQMappableCommands[nEQMappableCommands - 18] = "CHAT_SLASH";
   szEQMappableCommands[nEQMappableCommands - 17] = "UNKNOWN0x113";
   szEQMappableCommands[nEQMappableCommands - 16] = "UNKNOWN0x114";
   szEQMappableCommands[nEQMappableCommands - 15] = "INSTANT_CAMP";
   szEQMappableCommands[nEQMappableCommands - 14] = "UNKNOWN0x116";
   szEQMappableCommands[nEQMappableCommands - 13] = "UNKNOWN0x117";
   szEQMappableCommands[nEQMappableCommands - 12] = "CHAT_EMPTY";
   szEQMappableCommands[nEQMappableCommands - 11] = "TOGGLE_WINDOWMODE";
   szEQMappableCommands[nEQMappableCommands - 10] = "UNKNOWN0x11a";
   szEQMappableCommands[nEQMappableCommands - 9] = "UNKNOWN0x11b";
   szEQMappableCommands[nEQMappableCommands - 8] =
      "CHANGEFACE"; // maybe? something that requires models.
   szEQMappableCommands[nEQMappableCommands - 7] = "UNKNOWN0x11d";
   szEQMappableCommands[nEQMappableCommands - 6] = "UNKNOWN0x11e";
   szEQMappableCommands[nEQMappableCommands - 5] = "UNKNOWN0x11f";
   szEQMappableCommands[nEQMappableCommands - 4] = "UNKNOWN0x120";
   szEQMappableCommands[nEQMappableCommands - 3] = "UNKNOWN0x121";
   szEQMappableCommands[nEQMappableCommands - 2] = "UNKNOWN0x122";
   szEQMappableCommands[nEQMappableCommands - 1] = "UNKNOWN0x123";

   for (nColorAdjective = 0; szColorAdjective[nColorAdjective]; nColorAdjective++) {
   }
   for (nColorAdjectiveYou = 0; szColorAdjectiveYou[nColorAdjectiveYou];
        nColorAdjectiveYou++) {
   }
   for (nColorExpletive = 0; szColorExpletive[nColorExpletive]; nColorExpletive++) {
   }
   for (nColorSyntaxError = 0; szColorSyntaxError[nColorSyntaxError];
        nColorSyntaxError++) {
   }
   for (nColorMacroError = 0; szColorMacroError[nColorMacroError]; nColorMacroError++) {
   }
   for (nColorMQ2DataError = 0; szColorMQ2DataError[nColorMQ2DataError];
        nColorMQ2DataError++) {
   }
   for (nColorFatalError = 0; szColorFatalError[nColorFatalError]; nColorFatalError++) {
   }
#ifndef ISXEQ
   InitializeParser();
#endif
   InitializeMQ2Detours();
   if (isMQInjectsEnabled) {
	DebugSpew("initializing mq2 hooks");
	InitializeDisplayHook();
	InitializeChatHook();
	InitializeMQ2Spawns();
	InitializeMQ2Pulse();
	InitializeMQ2Commands();
	InitializeMapPlugin();
	// InitializeMQ2KeyBinds();
   }
   return true;
}


// ***************************************************************************
// Function:    MQ2Start
// Description: Where we start execution during the insertion
// ***************************************************************************
DWORD WINAPI MQ2Start()
{
   PCHAR lpINIPath = "";
   strcpy(gszINIPath, lpINIPath);
   CHAR szBuffer[MAX_STRING] = {0};

   if (!MQ2Initialize()) return 1;
   return 0;
}

// dinput8.cpp : Defines the exported functions for the DLL application.
//

#include "IDirectInput8Hook.h"
AddressLookupTable<void> ProxyAddressLookupTable = AddressLookupTable<void>();

DirectInput8CreateProc m_pDirectInput8Create;
DllCanUnloadNowProc m_pDllCanUnloadNow;
DllGetClassObjectProc m_pDllGetClassObject;
DllRegisterServerProc m_pDllRegisterServer;
DllUnregisterServerProc m_pDllUnregisterServer;
GetdfDIJoystickProc m_pGetdfDIJoystick;

static void EnsureInitHooksOnce()
{
	static volatile LONG done = 0;
	if (InterlockedCompareExchange(&done, 1, 0) != 0) {
		return;
	}
	LogDebug("InitHooksOnce: starting");
	InitHooks();
	LogDebug("InitHooksOnce: finished");
}

bool WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
   static HMODULE dinput8dll = nullptr;
   CHAR szFilename[MAX_STRING] = {0};

   switch (dwReason) {
   case DLL_PROCESS_ATTACH:
	   LogDebug("DllMain: DLL_PROCESS_ATTACH started");
	   DisableThreadLibraryCalls(hModule);
	   InstallCrashDiagnostics();
	   // Load dll
	   char path[MAX_PATH];
	   GetSystemDirectoryA(path, MAX_PATH);
	   strcat_s(path, "\\dinput8.dll");
	   dinput8dll = LoadLibraryA(path);

	   PCHAR szProcessName;
	   ghModule = (HMODULE)hModule;
	   ghInstance = (HINSTANCE)hModule;

	   GetModuleFileName(ghModule, szFilename, MAX_STRING);
	   szProcessName = strrchr(szFilename, '\\');
	   szProcessName[0] = '\0';
	   strcat(szFilename, "\\eqgame.ini");

	   GetModuleFileName(NULL, szFilename, MAX_STRING);

	   szProcessName = strrchr(szFilename, '.');
	   szProcessName[0] = '\0';
	   szProcessName = strrchr(szFilename, '\\') + 1;
	   LogDebug("DllMain: deferred InitHooks (will run on first DirectInput8Create)");
	   // remove full information about my command line
	 // memset(&pbi.PebBaseAddress->ProcessParameters->ImagePathName.Buffer, 0, pbi.PebBaseAddress->ProcessParameters->ImagePathName.Length);


	   // Get function addresses
	   m_pDirectInput8Create =
		   (DirectInput8CreateProc)GetProcAddress(dinput8dll,
			   "DirectInput8Create");
	   m_pDllCanUnloadNow =
		   (DllCanUnloadNowProc)GetProcAddress(dinput8dll, "DllCanUnloadNow");
	   m_pDllGetClassObject =
		   (DllGetClassObjectProc)GetProcAddress(dinput8dll, "DllGetClassObject");
	   m_pDllRegisterServer =
		   (DllRegisterServerProc)GetProcAddress(dinput8dll, "DllRegisterServer");
	   m_pDllUnregisterServer =
		   (DllUnregisterServerProc)GetProcAddress(dinput8dll,
			   "DllUnregisterServer");
	   m_pGetdfDIJoystick =
		   (GetdfDIJoystickProc)GetProcAddress(dinput8dll, "GetdfDIJoystick");
	   break;

   case DLL_PROCESS_DETACH:
	   CoUninitialize();
	   FreeLibrary(dinput8dll);
	   break;
   }

   return true;
}

HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf,
                                  LPVOID* ppvOut, LPUNKNOWN punkOuter)
{
   LogDebug("DirectInput8Create called");
   EnsureInitHooksOnce();
   if (!m_pDirectInput8Create) {
      return E_FAIL;
   }

   HRESULT hr = m_pDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);

   if (SUCCEEDED(hr)) {
      // genericQueryInterface(riidltf, ppvOut);
      LogDebug("DirectInput8Create: genericQueryInterface disabled");
   }

   return hr;
}

HRESULT WINAPI DllCanUnloadNow()
{
   if (!m_pDllCanUnloadNow) {
      return E_FAIL;
   }

   return m_pDllCanUnloadNow();
}

HRESULT WINAPI DllGetClassObject(IN REFCLSID rclsid, IN REFIID riid, OUT LPVOID FAR* ppv)
{
   if (!m_pDllGetClassObject) {
      return E_FAIL;
   }

   HRESULT hr = m_pDllGetClassObject(rclsid, riid, ppv);

   if (SUCCEEDED(hr)) {
      // genericQueryInterface(riid, ppv);
   }

   return hr;
}

HRESULT WINAPI DllRegisterServer()
{
   if (!m_pDllRegisterServer) {
      return E_FAIL;
   }

   return m_pDllRegisterServer();
}

HRESULT WINAPI DllUnregisterServer()
{
   if (!m_pDllUnregisterServer) {
      return E_FAIL;
   }

   return m_pDllUnregisterServer();
}

LPCDIDATAFORMAT WINAPI GetdfDIJoystick()
{
   if (!m_pGetdfDIJoystick) {
      return nullptr;
   }

   return m_pGetdfDIJoystick();
}
