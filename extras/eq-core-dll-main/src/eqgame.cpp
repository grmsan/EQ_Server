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

void LogDebug(const char* format, ...) {
	FILE* file;
	if (fopen_s(&file, "dinput8_debug.log", "a") == 0) {
		std::time_t now = std::time(nullptr);
		char buf[20];
		std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
		fprintf(file, "[%s] ", buf);

		va_list args;
		va_start(args, format);
		vfprintf(file, format, args);
		fprintf(file, "\n");
		va_end(args);
		fclose(file);
	}
}

static void ConditionalDumpOpcode(uint16_t op, const char* buf, size_t size, const char* toggle_filename)
{
	// Toggle file lives in the repo logs folder. If present, dump the packet for offline analysis.
	const char* repo_toggle_prefix = "C:\\Users\\marsh\\OneDrive\\Documents\\GitHub\\EQ_Server\\logs\\";
	char fulltoggle[1024];
	snprintf(fulltoggle, sizeof(fulltoggle), "%s%s", repo_toggle_prefix, toggle_filename);
	FILE* tf = nullptr;
	if (fopen_s(&tf, fulltoggle, "r") == 0 && tf) {
		fclose(tf);
		// toggle file exists -> write dump
		time_t now = time(nullptr);
		struct tm* tmv = localtime(&now);
		char tb[32] = {0};
		strftime(tb, sizeof(tb), "%Y%m%d_%H%M%S", tmv);
		char dumpname[1024];
		snprintf(dumpname, sizeof(dumpname), "C:\\Users\\marsh\\OneDrive\\Documents\\GitHub\\EQ_Server\\logs\\dumps\\packet_%s_%04x.bin", tb, op & 0xFFFF);
		FILE* df = nullptr;
		// Ensure dumps directory exists (best-effort)
		// We won't create directories portably here; assume it exists or the user will create it.
		if (fopen_s(&df, dumpname, "wb") == 0 && df) {
			fwrite(buf, 1, size, df);
			fclose(df);
		}
		// Also append a short human-readable line to the repo stats log
		FILE* lf = nullptr;
		if (fopen_s(&lf, "C:\\Users\\marsh\\OneDrive\\Documents\\GitHub\\EQ_Server\\logs\\stats_debug.log", "a") == 0 && lf) {
			fprintf(lf, "%s DUMP opcode=0x%04x size=%zu -> %s\n", tb, op & 0xFFFF, size, dumpname);
			fclose(lf);
		}
	}
}

// Use simple integers for last-logged detour values to avoid global C++
// container construction during DLL load which can be fragile.
static int g_last_logged_hp = INT_MIN;
static int g_last_logged_mana = INT_MIN;
static int g_last_logged_end = INT_MIN;

static void ConditionalLogEdgeStat(const char* buf, size_t size, const char* /*toggle_filename*/)
{
	// Always parse and append EdgeStat entries to the repo stats_debug.log
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
};

ServerProfileCache g_serverProfile;
static bool logged_profile_candidate = false;
static int packetCount = 0;

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

static void ResetServerCaches()
{
    g_serverCurHP   = -1;
    g_serverMaxHP   = -1;
    g_serverCurEnd  = -1;
    g_serverMaxEnd  = -1;
    g_serverCurMana = -1;
    g_serverMaxMana = -1;
    g_serverProfile = ServerProfileCache{};
    logged_profile_candidate = false;
}

// --- Write-watch for local-player stat writes (VEH) ---
static PVOID g_stat_watch_handler = nullptr;
static void* g_stat_watch_page = nullptr;
static size_t g_stat_watch_page_size = 0;
static bool g_stat_watch_active = false;

static void DisableStatWriteWatch();

static LONG CALLBACK StatWriteWatchHandler(PEXCEPTION_POINTERS ep)
{
	if (!ep || !ep->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;
	DWORD code = (DWORD)ep->ExceptionRecord->ExceptionCode;
	// Access violation on write (1 == write)
	if (code == EXCEPTION_ACCESS_VIOLATION && ep->ExceptionRecord->NumberParameters >= 2) {
		uintptr_t fault_addr = (uintptr_t)ep->ExceptionRecord->ExceptionInformation[1];
		uintptr_t page_base = (uintptr_t)g_stat_watch_page;
		size_t page_size = g_stat_watch_page_size;
		if (g_stat_watch_active && page_base && fault_addr >= page_base && fault_addr < page_base + page_size) {
			// Log the faulting IP and target address (x86 uses Eip)
#ifdef _M_X64
			void* ip = (void*)ep->ContextRecord->Rip;
#else
			void* ip = (void*)ep->ContextRecord->Eip;
#endif
			LogDebug("[WRITE_WATCH_FAULT] ip=%p target=%p", ip, (void*)fault_addr);

			// Temporarily make the page writable so the instruction can complete.
			DWORD old = 0;
			if (VirtualProtect((LPVOID)page_base, page_size, PAGE_READWRITE, &old)) {
				// Allow the instruction to continue and perform the write.
				return EXCEPTION_CONTINUE_EXECUTION;
			} else {
				// If we can't change protection, remove the watch to avoid lockout.
				DisableStatWriteWatch();
				return EXCEPTION_CONTINUE_SEARCH;
			}
		}
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

static void EnableStatWriteWatchForSpawn(void* spawn_ptr)
{
	if (!spawn_ptr) return;
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	size_t page = si.dwPageSize;
	uintptr_t addr = (uintptr_t)spawn_ptr;
	uintptr_t page_base = addr & ~(page - 1);

	g_stat_watch_page = (void*)page_base;
	g_stat_watch_page_size = page; // watch a single page covering spawn

	// Protect page to trigger VEH on writes
	DWORD old = 0;
	if (VirtualProtect(g_stat_watch_page, g_stat_watch_page_size, PAGE_NOACCESS, &old)) {
		if (!g_stat_watch_handler) {
			g_stat_watch_handler = AddVectoredExceptionHandler(1, StatWriteWatchHandler);
		}
		g_stat_watch_active = true;
		LogDebug("[WRITE_WATCH_ENABLE] page=%p size=%zu", g_stat_watch_page, g_stat_watch_page_size);
		// Start a short-lived timer to automatically disable the watch after 15 seconds
		std::thread([]() {
			std::this_thread::sleep_for(std::chrono::seconds(15));
			DisableStatWriteWatch();
		}).detach();
	} else {
		LogDebug("[WRITE_WATCH_ENABLE_FAIL] could not protect page=%p", g_stat_watch_page);
		g_stat_watch_active = false;
	}
}

static void DisableStatWriteWatch()
{
	if (g_stat_watch_page && g_stat_watch_page_size) {
		DWORD old = 0;
		VirtualProtect(g_stat_watch_page, g_stat_watch_page_size, PAGE_READWRITE, &old);
	}
	if (g_stat_watch_handler) {
		RemoveVectoredExceptionHandler(g_stat_watch_handler);
		g_stat_watch_handler = nullptr;
	}
	g_stat_watch_page = nullptr;
	g_stat_watch_page_size = 0;
	g_stat_watch_active = false;
	LogDebug("[WRITE_WATCH_DISABLE]");
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
					// Dump recent packet previews to help diagnose what packets arrived during zoning
					{
						FILE* rf = nullptr;
						if (fopen_s(&rf, "dinput8_debug.log", "a") == 0 && rf) {
							fprintf(rf, "[CACHE_RESET_RECENT] spawn=%u recent packet dump:\n", me->SpawnID);
							DumpRecentPackets(rf, 40);
							fclose(rf);
						}
					}
                } else {
                    LogDebug("[CACHE_RESET] new spawn id %u (old %u), keeping profile cache", me->SpawnID, g_localSpawnId);
					// Also dump recent packets when we keep profile cache (initial spawn)
					{
						FILE* rf = nullptr;
						if (fopen_s(&rf, "dinput8_debug.log", "a") == 0 && rf) {
							fprintf(rf, "[CACHE_RESET_RECENT] spawn=%u recent packet dump (initial):\n", me->SpawnID);
							DumpRecentPackets(rf, 40);
							fclose(rf);
						}
					}
                }
				g_localSpawnId = me->SpawnID;

				// Enable a short-lived write-watch on both the local spawn and character info pages
				// to catch unexpected writers who may be double-applying item stats during zone-in.
				EnableStatWriteWatchForSpawn((void*)me);
				if (GetCharInfo()) {
					EnableStatWriteWatchForSpawn((void*)GetCharInfo());
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
				ConditionalDumpOpcode(lop, buf, size, "enable_dump_1338");
				// Also write a readable, parsed EdgeStat log when toggled
				ConditionalLogEdgeStat(buf, size, "enable_log_1338");
			}
			if (lop == 0x575b) {
				ConditionalDumpOpcode(lop, buf, size, "enable_dump_575b");
			}
		}

        // Only parse PlayerProfile opcode (RoF2 0x6506) for stats snapshot; ignore other large packets.
        const uint16_t player_profile_opcode = 0x6506; // OP_PlayerProfile (RoF2)
		if (!logged_profile_candidate && (opcode & 0xFFFF) == player_profile_opcode && size > 9000) {
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

    if (size >= 10 && (op == hp_opcode || op == end_opcode || op == mob_end_opcode || op == mana_opcode || op == mob_mana_opcode)) {
        try {
#pragma pack(push,1)
            struct HpUpdatePayload {
                uint32_t cur_hp;
                int32_t  max_hp;
                uint16_t spawn_id;
            };
#pragma pack(pop)

            const auto* hp = reinterpret_cast<const HpUpdatePayload*>(buf);
            uint32_t cur   = hp->cur_hp;
            int32_t  max   = hp->max_hp;
            uint16_t spawn = hp->spawn_id;

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
				} else if (op == end_opcode || op == mob_end_opcode) { // endurance updates
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
DETOUR_TRAMPOLINE_EMPTY(int __cdecl EQCharacter_MaxHP_Tramp(int, int));
int __cdecl EQCharacter_MaxHP_Detour(int a1, int a2)
{
	int nativeVal = EQCharacter_MaxHP_Tramp(a1, a2);
	int used = nativeVal;
	int server = g_serverMaxHP;
	if (server > 0) {
		used = server;
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

DETOUR_TRAMPOLINE_EMPTY(int __cdecl EQCharacter_MaxMana_Tramp(int));
int __cdecl EQCharacter_MaxMana_Detour(int a1)
{
	int nativeVal = EQCharacter_MaxMana_Tramp(a1);
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

DETOUR_TRAMPOLINE_EMPTY(int __cdecl EQCharacter_MaxEnd_Tramp(int));
int __cdecl EQCharacter_MaxEnd_Detour(int a1)
{
	int nativeVal = EQCharacter_MaxEnd_Tramp(a1);
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

// NOTE: TotalEffect detour removed — attempting to detour member functions
// with the wrong calling convention caused instability on character select.
// We keep the EdgeStat cache and Max_* detours active; further member-
// function detours should be added only after confirming exact calling
// conventions and offsets for this client build.

signed int ProcessGameEvents_Hook()
{
   DWORD oldTimeGetTimeVal = 0;
   return return_ProcessGameEvents();
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
	//rename("arena.eqg", "arena.eqg.bak");
	//rename("highpasshold.eqg", "highpasshold.eqg.bak");
	//rename("nektulos.eqg", "nektulos.eqg.bak");
	//rename("lavastorm.eqg", "lavastorm.eqg.bak");
	LogDebug("InitHooks: Calling InitOffsets");
	InitOffsets();
	LogDebug("InitHooks: InitOffsets returned");
	GetEQPath(gszEQPath);
	InitializeCriticalSection(&gDetourCS);

	if (isMQInjectsEnabled) {
		LogDebug("InitHooks: Applying mq2 injects");
		DebugSpew("Applying mq2 injects");
		InitializeDisplayHook();
		InitializeChatHook();
		InitializeMQ2Commands();
		InitializeMQ2Pulse();
		InitializeMQ2Spawns();
		InitializeMapPlugin();
		InitializeMQ2ItemDisplay();
		InitializeMQ2Labels();
	}

	if (!baseAddress) {
		LogDebug("InitHooks: baseAddress is null, returning");
		return;
	}
	InitOptions();

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
#if 0
	// Disabled: installing the world-message detour caused instability/crashes
	// during recent investigations. Keep the code here for reference but do
	// not activate it so the client runs with the unmodified packet handler.
	EzDetour((DWORD)var, HandleWorldMessage_Detour, HandleWorldMessage_Trampoline);
#endif

	// Install detours so that the game's native Max_* accessors return the
	// authoritative server-provided values when we have them. This keeps
	// UI and other systems consistent without mutating spawn memory.
	// Temporarily disable Max_* accessor detours to restore a stable baseline
	// for the DLL while we investigate crash causes. Re-enable only after
	// the packet-order/write-source is identified and a surgical detour is
	// implemented.
#if 0
	EzDetour((DWORD)EQ_Character__Max_HP, EQCharacter_MaxHP_Detour, EQCharacter_MaxHP_Tramp);
	EzDetour((DWORD)EQ_Character__Max_Mana, EQCharacter_MaxMana_Detour, EQCharacter_MaxMana_Tramp);
	EzDetour((DWORD)EQ_Character__Max_Endurance, EQCharacter_MaxEnd_Detour, EQCharacter_MaxEnd_Tramp);
#endif

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
bool WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
   static HMODULE dinput8dll = nullptr;
   CHAR szFilename[MAX_STRING] = {0};

   switch (dwReason) {
   case DLL_PROCESS_ATTACH:
	   LogDebug("DllMain: DLL_PROCESS_ATTACH started");
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
	   LogDebug("DllMain: Calling InitHooks");
	   InitHooks();
	   LogDebug("DllMain: InitHooks returned");
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
