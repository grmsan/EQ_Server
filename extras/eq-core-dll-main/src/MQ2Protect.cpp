// Minimal page-protect + VEH helper for write-watch tracing
#include "MQ2Main.h"
#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <vector>
#include <cstdio>

#pragma comment(lib, "dbghelp.lib")
// Psapi provides EnumProcessModules/GetModuleInformation
#pragma comment(lib, "psapi.lib")

extern void LogDebug(const char* format, ...);

static std::vector<uintptr_t> g_protectedPages_prot;
static SIZE_T g_pageSize_prot = 0;
static PVOID g_vectoredHandler_prot = NULL;

static PVOID InstallVEH();

extern "C" __declspec(dllexport) void __cdecl MQ2_ProtectPage(uintptr_t addr)
{
    if (!addr) return;
    if (!g_pageSize_prot) {
        SYSTEM_INFO si; GetSystemInfo(&si);
        g_pageSize_prot = si.dwPageSize ? si.dwPageSize : 4096;
    }
    uintptr_t pageBase = addr & ~(g_pageSize_prot - 1);
    for (auto p : g_protectedPages_prot) if (p == pageBase) return;
    g_protectedPages_prot.push_back(pageBase);

    if (!g_vectoredHandler_prot) {
        LogDebug("MQ2Protect: queued page %p (addr %p) - VEH not installed", (void*)pageBase, (void*)addr);
    } else {
        DWORD old = 0;
        if (VirtualProtect((LPVOID)pageBase, g_pageSize_prot, PAGE_READONLY, &old)) {
            LogDebug("MQ2Protect: protected page %p (addr %p)", (void*)pageBase, (void*)addr);
        } else {
            LogDebug("MQ2Protect: failed to protect page %p (addr %p) err=%u", (void*)pageBase, (void*)addr, GetLastError());
        }
    }
    // Ensure VEH installed and apply queued protections
    if (!g_vectoredHandler_prot) {
        g_vectoredHandler_prot = InstallVEH();
        if (g_vectoredHandler_prot) {
            for (auto p : g_protectedPages_prot) {
                DWORD old = 0;
                VirtualProtect((LPVOID)p, g_pageSize_prot, PAGE_READONLY, &old);
            }
        }
    }
}

static LONG WINAPI VehHandler(PEXCEPTION_POINTERS ExceptionInfo)
{
    if (!ExceptionInfo || !ExceptionInfo->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;
    DWORD code = (DWORD)ExceptionInfo->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION) return EXCEPTION_CONTINUE_SEARCH;
    ULONG_PTR op = ExceptionInfo->ExceptionRecord->ExceptionInformation[0];
    // Only care about write access violations
    if (op != 1) return EXCEPTION_CONTINUE_SEARCH;
    uintptr_t target = (uintptr_t)ExceptionInfo->ExceptionRecord->ExceptionInformation[1];
    for (auto page : g_protectedPages_prot) {
        if (target >= page && target < page + g_pageSize_prot) {
            void* ip = nullptr;
#ifdef _M_X64
            ip = (void*)ExceptionInfo->ContextRecord->Rip;
#else
            ip = (void*)ExceptionInfo->ContextRecord->Eip;
#endif
            LogDebug("WRITE_WATCH_FAULT ip=%p target=%p", ip, (void*)target);
            HMODULE hMods[1024]; DWORD cbNeeded;
            if (EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cbNeeded)) {
                for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); ++i) {
                    MODULEINFO mi; if (GetModuleInformation(GetCurrentProcess(), hMods[i], &mi, sizeof(mi))) {
                        uintptr_t base = (uintptr_t)mi.lpBaseOfDll;
                        if ((uintptr_t)ip >= base && (uintptr_t)ip < base + mi.SizeOfImage) {
                            CHAR modName[MAX_PATH];
                            if (GetModuleFileNameA(hMods[i], modName, MAX_PATH)) {
                                LogDebug("WRITE_WATCH_FAULT module=%s base=%p", modName, (void*)base);
                            }
                            break;
                        }
                    }
                }
            }
            DWORD old = 0;
            VirtualProtect((LPVOID)page, g_pageSize_prot, PAGE_READWRITE, &old);
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static PVOID InstallVEH()
{
    if (g_vectoredHandler_prot) return g_vectoredHandler_prot;
    PVOID h = AddVectoredExceptionHandler(1, (PVECTORED_EXCEPTION_HANDLER)VehHandler);
    g_vectoredHandler_prot = h;
    return h;
}
