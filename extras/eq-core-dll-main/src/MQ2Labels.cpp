// MQ2Labels.cpp : Defines the entry point for the DLL application.
//

// MQ2 Custom Labels


#include "MQ2Main.h"
#include <map>
#include <stdint.h>

// Ensure we have a C-style declaration matching the exported helper in MQ2Main.cpp
extern "C" void __cdecl MQ2_ProtectPage(uintptr_t addr);

// Runtime toggles are defined once in eqgame.cpp via `_options.h`.
extern bool isDebugLoggingEnabled;
extern bool isMQ2LabelsInitLoggingEnabled;
extern bool isMQ2LabelsPerSidlLoggingEnabled;
extern bool isMQ2LabelsWriteUILoggingEnabled;
extern bool isMQ2LabelsWriteWatchEnabled;

extern "C" uint32_t __cdecl EQCore_GetEffectiveUsableClassesMask();
extern uint64_t g_edgeStatValue[];
extern uint8_t g_edgeStatHas[];

typedef string(*pEqTypesFunc)();

// Provided by eqgame.cpp to cache server-reported HP for the local player.
extern int g_serverCurHP;
extern int g_serverMaxHP;
extern int g_serverCurEnd;
extern int g_serverMaxEnd;
extern int g_serverCurMana;
extern int g_serverMaxMana;
extern struct ServerProfileCache {
    bool   has_profile;
    int    str;
    int    sta;
    int    agi;
    int    dex;
    int    intl;
    int    wis;
    int    cha;
    int    hp_cur;
    int    mana_cur;
    int    mana_max;
    int    end_cur;
    int    end_max;
} g_serverProfile;

map<DWORD, pEqTypesFunc> eqTypesMap;

static DWORD CapStat(DWORD value, DWORD cap)
{
	return (value > cap) ? cap : value;
}

static void BuildClassAbbrevList(uint32_t mask, char* out, size_t out_size)
{
	if (!out || out_size == 0) {
		return;
	}
	out[0] = '\0';

	static const char* kClass3[16] = {
		"WAR", "CLR", "PAL", "RNG", "SHD", "DRU", "MNK", "BRD",
		"ROG", "SHM", "NEC", "WIZ", "MAG", "ENC", "BST", "BER"
	};

	size_t used = 0;
	for (int i = 0; i < 16; ++i) {
		if ((mask & (1u << i)) == 0) {
			continue;
		}

		const char* token = kClass3[i];
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


static bool GetEdgeStatLabel(DWORD key, std::string& value)
{
	if (key >= 8192 || !g_edgeStatHas[key]) {
		return false;
	}
	value = std::to_string(static_cast<unsigned long long>(g_edgeStatValue[key]));
	return true;
}

static bool ResolveTHJInventoryLabel(DWORD sidl, std::string& value)
{
	PCHARINFO ci = GetCharInfo();
	if (!ci) {
		return false;
	}

	if ((sidl >= 251 && sidl <= 286) || (sidl >= 6667 && sidl <= 6678) || (sidl >= 6703 && sidl <= 6706)) {
		if (GetEdgeStatLabel(sidl, value)) {
			return true;
		}
	}

	switch (sidl) {
	case 3:
	case 6666:
	{
		char classText[64] = {0};
		uint32_t mask = EQCore_GetEffectiveUsableClassesMask() & 0xFFFFu;
		if (mask == 0) {
			if (PCHARINFO2 ci2 = GetCharInfo2()) {
				if (ci2->Class >= 1 && ci2->Class <= 16) {
					mask = (1u << (ci2->Class - 1));
				}
			}
		}
		BuildClassAbbrevList(mask, classText, sizeof(classText));
		if (classText[0] == '\0') {
			return false;
		}
		value = classText;
		return true;
	}
	case 6667:
		value = std::to_string(CapStat(ci->AvoidanceBonus, ci->AvoidanceCap));
		return true;
	case 6668:
		value = std::to_string(CapStat(ci->DamageShieldMitigationBonus, ci->DamageShieldMitigationCap));
		return true;
	case 6669:
		value = std::to_string(ci->AttackBonus);
		return true;
	case 6671:
	case 6672:
	case 6673:
	case 6674:
	case 6675:
	case 6676:
	case 6677:
	case 6678:
		value = "0";
		return true;
	default:
		break;
	}

	return false;
}

// CSidlManager::CreateLabel 0x5F2470

// the tool tip is already copied out of the
// in class CControlTemplate.  use this struct
// to mock up the class, so we don't have to
// worry about class instatiation and crap

struct _CControl {
    /*0x000*/    DWORD Fluff[0x24]; // if this changes update ISXEQLabels.cpp too
    /*0x090*/    CXSTR * EQType;
};

// optimize off because the tramp looks blank to the compiler
// and it doesn't respect the fact the it will be a real routine
#pragma optimize ("g", off)

class CSidlManagerHook {
public:
    class CXWnd * CreateLabel_Trampoline(class CXWnd *, struct _CControl *);
    class CXWnd * CreateLabel_Detour(class CXWnd *CWin, struct _CControl *CControl)
    {
        CLABELWND *p;
        class CXWnd *tmp = CreateLabel_Trampoline(CWin, CControl);
        p = (CLABELWND *)tmp;
        if (CControl->EQType) {
            *((DWORD *)&p->SidlPiece) = atoi(CControl->EQType->Text);
        } else {
            *((DWORD *)&p->SidlPiece) = 0;
        }

        return tmp;
    }
};

DETOUR_TRAMPOLINE_EMPTY(class CXWnd * CSidlManagerHook::CreateLabel_Trampoline(class CXWnd *, struct _CControl *));

#pragma optimize ("g", on)

// CLabelHook::Draw_Detour

class CLabelHook {
public:
    VOID Draw_Trampoline(VOID);
    VOID Draw_Detour(VOID)
    {
        PCLABELWND pThisLabel;
        __asm {mov [pThisLabel], ecx};
        //          (PCLABELWND)this;
        Draw_Trampoline();
        CHAR Buffer[MAX_STRING] = {0};
        BOOL Found=FALSE;
        DWORD index;

		std::string eqtypesString = "";
		static bool s_loggedOverride = false;
		static bool s_fileLogged = false;
        static bool s_seenDraw = false;
        static int  s_logMissCount = 0;
		static int  s_logValueCount[256] = {0};
		static long long s_lastLoggedValue[256];
		static bool s_lastLoggedInit = false;

        // safety: if character data isn't ready, don't override
        if (!pCharData || !((PCHARINFO)pCharData)->pSpawn) {
            Found = FALSE;
        } else {
			auto eval_macro = [&](const char* macro_expr) -> bool {
				memset(Buffer, 0, sizeof(Buffer));
				strcpy_s(Buffer, macro_expr);
				ParseMacroParameter(((PCHARINFO)pCharData)->pSpawn, Buffer);
				if (!strcmp(Buffer, "NULL") || Buffer[0] == 0) {
					return false;
				}
				eqtypesString = Buffer;
				return !eqtypesString.empty();
			};
			auto eval_direct = [&](DWORD sidl, __int64 &out) -> bool {
				PCHARINFO ci = GetCharInfo();
				if (!ci) return false;
				out = -1;
				switch (sidl) {
				case 5:   // STR legacy
					out = (__int64)ci->STR; break;
				case 6:
					out = (__int64)ci->STA; break;
				case 7:
					out = (__int64)ci->DEX; break;
				case 8:
					out = (__int64)ci->AGI; break;
				case 9:
					out = (__int64)ci->INT; break;
				case 10:
					out = (__int64)ci->WIS; break;
				case 11:
					out = (__int64)ci->CHA; break;
				case 17: // current HP legacy
					out = (__int64)GetCurHPS(); break;
				case 18: // max HP legacy
					out = (__int64)GetMaxHPS(); break;
				case 126: // current endurance
					if (auto ci2 = GetCharInfo2()) out = (__int64)ci2->Endurance;
					break;
				case 127: // max endurance
					out = (__int64)GetMaxEndurance(); break;
				case 124: // current mana (inventory stats tab)
					if (auto ci2 = GetCharInfo2()) out = (__int64)ci2->Mana;
					break;
				case 125: // max mana (inventory stats tab)
					out = (__int64)GetMaxMana(); break;
				default:
					break;
				}
				return out >= 0;
			};

		    // Override legacy EQTypes (5-11 stats, 17/18 HP) to pull uncapped server values via MQ2 data
		    DWORD sidl = (DWORD)pThisLabel->SidlPiece;
			// Log first draw we see, to confirm detour is active
			if (!s_seenDraw && isDebugLoggingEnabled && isMQ2LabelsInitLoggingEnabled) {
				FILE* f = nullptr;
				if (fopen_s(&f, "dinput8_debug.log", "a") == 0 && f) {
					fprintf(f, "MQ2Labels: Draw_Detour hit, first SidlPiece=%lu\n", sidl);
					fclose(f);
				}
				s_seenDraw = true;
			}
			if (ResolveTHJInventoryLabel(sidl, eqtypesString)) {
				Found = TRUE;
			}

		    switch (sidl) {
		    case 5:  case 6:  case 7:  case 8:  case 9:  case 10: case 11:
			case 17: case 18:
			case 124: case 125:
			case 126: case 127:
			{
				// Pull directly from client memory (populated by server packet). No macro fallback since it has proven unreliable.
				static bool s_loggedFirstDraw = false;
				if (!s_loggedFirstDraw && isDebugLoggingEnabled && isMQ2LabelsInitLoggingEnabled) {
					FILE* f2 = nullptr;
					if (fopen_s(&f2, "dinput8_debug.log", "a") == 0 && f2) {
						fprintf(f2, "MQ2Labels: Draw_Detour first hit sidl=%lu\n", sidl);
						fclose(f2);
					}
					s_loggedFirstDraw = true;
				}

				__int64 direct_val = -1;
				bool direct_ok = eval_direct(sidl, direct_val);

				// Stats: prefer server packet cache; fall back to profile; then client
				if (g_serverProfile.has_profile) {
					switch (sidl) {
					case 5:  if (g_serverProfile.str   >= 0) { direct_val = g_serverProfile.str;   direct_ok = true; } break;
					case 6:  if (g_serverProfile.sta   >= 0) { direct_val = g_serverProfile.sta;   direct_ok = true; } break;
					case 7:  if (g_serverProfile.dex   >= 0) { direct_val = g_serverProfile.dex;   direct_ok = true; } break; // UI shows DEX on eqtype 7
					case 8:  if (g_serverProfile.agi   >= 0) { direct_val = g_serverProfile.agi;   direct_ok = true; } break; // UI shows AGI on eqtype 8
					case 9:  if (g_serverProfile.wis   >= 0) { direct_val = g_serverProfile.wis;   direct_ok = true; } break; // UI shows WIS on eqtype 9
					case 10: if (g_serverProfile.intl  >= 0) { direct_val = g_serverProfile.intl;  direct_ok = true; } break; // UI shows INT on eqtype 10
					case 11: if (g_serverProfile.cha   >= 0) { direct_val = g_serverProfile.cha;   direct_ok = true; } break;
					default: break;
					}
				}
				// If we still don't have stats, fall back to the profile snapshot even when the server caches have been reset.
				if (!direct_ok && g_serverProfile.has_profile) {
					switch (sidl) {
					case 5:  if (g_serverProfile.str   >= 0) { direct_val = g_serverProfile.str;   direct_ok = true; } break;
					case 6:  if (g_serverProfile.sta   >= 0) { direct_val = g_serverProfile.sta;   direct_ok = true; } break;
					case 7:  if (g_serverProfile.dex   >= 0) { direct_val = g_serverProfile.dex;   direct_ok = true; } break;
					case 8:  if (g_serverProfile.agi   >= 0) { direct_val = g_serverProfile.agi;   direct_ok = true; } break;
					case 9:  if (g_serverProfile.wis   >= 0) { direct_val = g_serverProfile.wis;   direct_ok = true; } break;
					case 10: if (g_serverProfile.intl  >= 0) { direct_val = g_serverProfile.intl;  direct_ok = true; } break;
					case 11: if (g_serverProfile.cha   >= 0) { direct_val = g_serverProfile.cha;   direct_ok = true; } break;
					default: break;
					}
				}

				// HP/Max HP: prefer server cache, then profile, then client
				if (sidl == 17) { // current HP
					if (g_serverCurHP > 0) { direct_val = g_serverCurHP; direct_ok = true; }
					else if (g_serverProfile.has_profile && g_serverProfile.hp_cur > 0) { direct_val = g_serverProfile.hp_cur; direct_ok = true; }
				} else if (sidl == 18) { // max HP
					if (g_serverMaxHP > 0) { direct_val = g_serverMaxHP; direct_ok = true; }
					else if (g_serverProfile.has_profile && g_serverProfile.hp_cur > 0) { direct_val = g_serverProfile.hp_cur; direct_ok = true; }
				} else if (sidl == 124) { // current mana
					if (g_serverCurMana > 0) { direct_val = g_serverCurMana; direct_ok = true; }
					else if (g_serverProfile.has_profile && g_serverProfile.mana_cur > 0) { direct_val = g_serverProfile.mana_cur; direct_ok = true; }
				} else if (sidl == 125) { // max mana
					if (g_serverMaxMana > 0) { direct_val = g_serverMaxMana; direct_ok = true; }
					else if (g_serverProfile.has_profile && g_serverProfile.mana_max > 0) { direct_val = g_serverProfile.mana_max; direct_ok = true; }
				} else if (sidl == 126) { // current endurance
					if (g_serverCurEnd > 0) { direct_val = g_serverCurEnd; direct_ok = true; }
					else if (g_serverProfile.has_profile && g_serverProfile.end_cur > 0) { direct_val = g_serverProfile.end_cur; direct_ok = true; }
				} else if (sidl == 127) { // max endurance
					if (g_serverMaxEnd > 0) { direct_val = g_serverMaxEnd; direct_ok = true; }
					else if (g_serverProfile.has_profile && g_serverProfile.end_max > 0) { direct_val = g_serverProfile.end_max; direct_ok = true; }
				}

				if (direct_ok && direct_val >= 0) {
					eqtypesString = std::to_string(direct_val);
					Found = true;

					// One-time snapshot of all server-fed values to help verify the profile packet contents.
					static bool s_loggedSnapshot = false;
					if (!s_loggedSnapshot && isDebugLoggingEnabled && isMQ2LabelsPerSidlLoggingEnabled) {
						PCHARINFO ci = GetCharInfo();
						if (ci) {
							FILE* f = nullptr;
							if (fopen_s(&f, "dinput8_debug.log", "a") == 0 && f) {
								fprintf(
									f,
									"MQ2Labels: first server snapshot STR=%d STA=%d AGI=%d DEX=%d INT=%d WIS=%d CHA=%d HP=%d/%d (cached_server_hp=%d/%d)\n",
									ci->STR, ci->STA, ci->AGI, ci->DEX, ci->INT, ci->WIS, ci->CHA,
									(int)GetCurHPS(), (int)GetMaxHPS(), g_serverCurHP, g_serverMaxHP
								);
								fclose(f);
							}
							s_loggedSnapshot = true;
						}
					}

					// Debug: log first few values per EQType to verify what we are pushing to the UI
					if (isDebugLoggingEnabled && isMQ2LabelsPerSidlLoggingEnabled && sidl < 256 && s_logValueCount[sidl] < 3) {
						FILE* f = nullptr;
						if (fopen_s(&f, "dinput8_debug.log", "a") == 0 && f) {
							fprintf(f, "MQ2Labels: sidl=%lu direct=%lld chosen=%lld (server_hp_cache=%d/%d)\n", sidl, direct_val, direct_val, g_serverCurHP, g_serverMaxHP);
							fclose(f);
						}
						s_logValueCount[sidl]++;
					}
				}
				break;
			}
		    default: break;
		    }

			if (Found && !s_loggedOverride) {
				DebugSpewAlways("MQ2Labels: overriding legacy EQType %lu with server values (first occurrence)", sidl);
				// also drop a breadcrumb to dinput8_debug.log in case MQ2 logging is disabled
				if (!s_fileLogged && isDebugLoggingEnabled && isMQ2LabelsInitLoggingEnabled) {
					FILE* f = nullptr;
					if (fopen_s(&f, "dinput8_debug.log", "a") == 0 && f) {
						fprintf(f, "MQ2Labels: legacy EQType override active (first occurrence sidl=%lu)\n", sidl);
						fclose(f);
						s_fileLogged = true;
					}
				}
				s_loggedOverride = true;
			} else if (!Found) {
				// Log only the first few misses to avoid spam
				if (isDebugLoggingEnabled && isMQ2LabelsPerSidlLoggingEnabled && ((sidl >= 5 && sidl <= 11) || sidl == 17 || sidl == 18)) {
					if (s_logMissCount < 10) {
						FILE* f = nullptr;
						if (fopen_s(&f, "dinput8_debug.log", "a") == 0 && f) {
							fprintf(f, "MQ2Labels: Draw_Detour sidl=%lu not replaced (Buffer='%s')\n", sidl, Buffer);
							fclose(f);
						}
						++s_logMissCount;
					}
				}
			}
        }


        if ((DWORD)pThisLabel->SidlPiece==9999) {
            if (!pThisLabel->Wnd.XMLToolTip) {
                strcpy(Buffer,"BadCustom");
                Found=TRUE;
            } else {
                //strcpy(Buffer,&pThisLabel->XMLToolTip->Text[0]);
                STMLToPlainText(&pThisLabel->Wnd.XMLToolTip->Text[0],Buffer);
                ParseMacroParameter(((PCHARINFO)pCharData)->pSpawn,Buffer);
                if (!strcmp(Buffer,"NULL"))
                    Buffer[0]=0;
                Found=TRUE;
            }
        } else if ((DWORD)pThisLabel->SidlPiece>=1000) {
            for (auto eqtype : eqTypesMap) {
                if (eqtype.first==(DWORD)pThisLabel->SidlPiece) {

					auto func = eqtype.second;
					if (func)
					{
						eqtypesString = (*func)();
						Found = TRUE;
						break;
					}
                }
            }
        }
		if (Found && !eqtypesString.empty()) {
			// Detailed debug: log exact value about to be written to the UI for correlation.
			{
				const bool want_write_ui_log = (isDebugLoggingEnabled && isMQ2LabelsWriteUILoggingEnabled);
				// Initialize last-logged table once
				if (!s_lastLoggedInit) {
					for (int i = 0; i < 256; ++i) s_lastLoggedValue[i] = LLONG_MIN;
					s_lastLoggedInit = true;
				}
				PCHARINFO ci = GetCharInfo();
				int ci_val = -999;
				void* ci_addr = nullptr;
				if (ci) {
					switch ((DWORD)pThisLabel->SidlPiece) {
					case 5:  ci_val = (int)ci->STR; ci_addr = (void*)&ci->STR; break;
					case 6:  ci_val = (int)ci->STA; ci_addr = (void*)&ci->STA; break;
					case 7:  ci_val = (int)ci->DEX; ci_addr = (void*)&ci->DEX; break;
					case 8:  ci_val = (int)ci->AGI; ci_addr = (void*)&ci->AGI; break;
					case 9:  ci_val = (int)ci->INT; ci_addr = (void*)&ci->INT; break;
					case 10: ci_val = (int)ci->WIS; ci_addr = (void*)&ci->WIS; break;
					case 11: ci_val = (int)ci->CHA; ci_addr = (void*)&ci->CHA; break;
					default: ci_val = -999; ci_addr = (void*)ci; break;
					}
				}
				if (ci_addr && isMQ2LabelsWriteWatchEnabled) {
					MQ2_ProtectPage((uintptr_t)ci_addr);
					if (want_write_ui_log) {
						FILE* lf2 = nullptr;
						if (fopen_s(&lf2, "dinput8_debug.log", "a") == 0 && lf2) {
							fprintf(lf2, "MQ2Labels: Requested Protect (queued) for ci_addr=%p\n", ci_addr);
							fclose(lf2);
						}
					}
				}

				if (want_write_ui_log) {
					FILE* lf = nullptr;
					if (fopen_s(&lf, "dinput8_debug.log", "a") == 0 && lf) {
						time_t now = time(nullptr);
						struct tm *tmv = localtime(&now);
						char tb[32] = {0};
						strftime(tb, sizeof(tb), "%Y%m%d_%H%M%S", tmv);

						int server_val = -999;
						if (g_serverProfile.has_profile) server_val = g_serverProfile.dex;
						long long dv = atoll(eqtypesString.c_str());

						DWORD sidl_val = (DWORD)pThisLabel->SidlPiece;
						if (sidl_val < 256) {
							if (dv != s_lastLoggedValue[sidl_val]) {
								fprintf(lf, "%s MQ2Labels: WRITE_UI sidl=%lu chosen=%s chosen_val=%lld ci_val=%d ci_addr=%p server_profile=%d has_profile=%d\n",
									tb, sidl_val, eqtypesString.c_str(), dv, ci_val, ci_addr, server_val, g_serverProfile.has_profile ? 1 : 0);
								s_lastLoggedValue[sidl_val] = dv;
							}
						} else {
							fprintf(lf, "%s MQ2Labels: WRITE_UI sidl=%lu chosen=%s chosen_val=%lld ci_val=%d ci_addr=%p server_profile=%d has_profile=%d\n",
								tb, sidl_val, eqtypesString.c_str(), dv, ci_val, ci_addr, server_val, g_serverProfile.has_profile ? 1 : 0);
						}
						fclose(lf);
					}
				}
			}
			SetCXStr(&(pThisLabel->Wnd.WindowText),(PCHAR)eqtypesString.c_str());
		}
    }
};

DETOUR_TRAMPOLINE_EMPTY(VOID CLabelHook::Draw_Trampoline(VOID));

BOOL StealNextGauge=FALSE;
DWORD NextGauge=0;

std::string testDisplayFunction()
{
	return "Test";
}

// Called once, when the plugin is to initialize
PLUGIN_API VOID InitializeMQ2Labels(VOID)
{
 //   DebugSpewAlways("Initializing MQ2Labels");
	eqTypesMap[1000] = testDisplayFunction; //and so forth

	// Add commands, macro parameters, hooks, etc.
	//EasyClassDetour(CLabel__Draw,CLabelHook,Draw_Detour,VOID,(VOID),Draw_Trampoline);
	EzDetour(CLabel__Draw,&CLabelHook::Draw_Detour,&CLabelHook::Draw_Trampoline);
	EzDetour(CSidlManager__CreateLabel,&CSidlManagerHook::CreateLabel_Detour,&CSidlManagerHook::CreateLabel_Trampoline);

	// Drop a breadcrumb to dinput8_debug.log on load so we know the DLL is active
	if (isDebugLoggingEnabled && isMQ2LabelsInitLoggingEnabled) {
		FILE* f = nullptr;
		if (fopen_s(&f, "dinput8_debug.log", "a") == 0 && f) {
			fprintf(f, "MQ2Labels: initialized and hooks installed\n");
			fclose(f);
		}
	}

	// Also ensure the repo stats_debug.log exists and note DLL load there.
	if (isDebugLoggingEnabled && isMQ2LabelsInitLoggingEnabled) {
		FILE* rf = nullptr;
		if (fopen_s(&rf, "C:\\Users\\marsh\\OneDrive\\Documents\\GitHub\\EQ_Server\\logs\\stats_debug.log", "a") == 0 && rf) {
			fprintf(rf, "MQ2Labels: DLL loaded and initialized\n");
			fclose(rf);
		}
	}

	auto logDebug = [](const char* fmt, ...) {
		if (!isDebugLoggingEnabled || !isMQ2LabelsInitLoggingEnabled) {
			return;
		}
		FILE* lf = nullptr;
		if (fopen_s(&lf, "dinput8_debug.log", "a") != 0 || !lf) {
			return;
		}
		va_list args;
		va_start(args, fmt);
		vfprintf(lf, fmt, args);
		va_end(args);
		fclose(lf);
	};

	// Patch client stat cap so profile values from the server are not clamped to 255.
	auto patchBytesLog = [&](DWORD address, const char* bytes, size_t len, const char* label) {
		DWORD oldProtect;
		unsigned char original[32] = {0};
		size_t copyLen = (len < sizeof(original)) ? len : sizeof(original);
		memcpy(original, reinterpret_cast<void*>(address), copyLen);
		bool ok = VirtualProtect(reinterpret_cast<LPVOID>(address), len, PAGE_EXECUTE_READWRITE, &oldProtect) != 0;
		if (ok) {
			memcpy(reinterpret_cast<void*>(address), bytes, len);
			FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(address), len);
			VirtualProtect(reinterpret_cast<LPVOID>(address), len, oldProtect, &oldProtect);
		}
		unsigned char patched[32] = {0};
		memcpy(patched, reinterpret_cast<void*>(address), copyLen);
		// Log original and patched bytes so we can diagnose mismatches/hangs
		logDebug("MQ2Labels: patch %-12s addr=0x%08X len=%zu protect_ok=%d orig=", label, address, len, ok ? 1 : 0);
		FILE* lf = nullptr;
		if (fopen_s(&lf, "dinput8_debug.log", "a") == 0 && lf) {
			for (size_t i = 0; i < copyLen; ++i) {
				fprintf(lf, "%02X", original[i]);
				if (i + 1 < copyLen) fprintf(lf, " ");
			}
			fprintf(lf, " patched=");
			for (size_t i = 0; i < copyLen; ++i) {
				fprintf(lf, "%02X", patched[i]);
				if (i + 1 < copyLen) fprintf(lf, " ");
			}
			fprintf(lf, "\n");
			fclose(lf);
		}
	};
	// Byte-patching of the client can destabilize the game. Guarded by a runtime flag so
	// we can disable/enable it without removing the original code. Default: disabled.
	bool apply_mq_injects = false; // set to true for manual testing only
	if (apply_mq_injects) {
		DWORD baseAddress = reinterpret_cast<DWORD>(GetModuleHandle(nullptr));
		// Stat cap patch lifted from classless DLL (raises cap to 0x0FFFFFFF)
		DWORD statCapAddr = ((0x0057F2C7 - 0x400000) + baseAddress);
		const char statCapPatch[] = "\xBF\xFF\xFF\xFF\x0F\x90\x90\x90\xE9\xF4\x01\x00\x00\x90";
		patchBytesLog(statCapAddr, statCapPatch, sizeof(statCapPatch) - 1, "statCap");

		// HP/Mana/Endurance cap patches (from classless DLL)
		// hpMax: classless DLL patched 12 bytes; keep length identical to avoid stomping adjacent code.
		patchBytesLog(((0x00444158 - 0x400000) + baseAddress), "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 12, "hpMax");
		patchBytesLog(((0x00449E3B - 0x400000) + baseAddress), "\x90\x90\x90\x90\x90\x90\x90\xE9\x1B\x01\x00\x00\x90", 13, "hpCur1");
		patchBytesLog(((0x00449F62 - 0x400000) + baseAddress), "\x90\x90", 2, "hpCur2a");
		patchBytesLog(((0x00449F64 - 0x400000) + baseAddress), "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 13, "hpCur2b");
		patchBytesLog(((0x004ED062 - 0x400000) + baseAddress), "\x08", 1, "hpAlt1");
		patchBytesLog(((0x004ED083 - 0x400000) + baseAddress), "\x08", 1, "hpAlt2");
		patchBytesLog(((0x00444308 - 0x400000) + baseAddress), "\x90\x90\xEB\x64", 4, "mana");
		patchBytesLog(((0x00444198 - 0x400000) + baseAddress), "\x90\x90\xEB\x64", 4, "end");
	}

	// currently in testing:
	//    EasyClassDetour(CGauge__Draw,CGaugeHook,Draw_Detour,VOID,(VOID),Draw_Trampoline);
	//    EasyDetour(__GetGaugeValueFromEQ,GetGaugeValueFromEQ_Hook,int,(int,class CXStr *,bool *),GetGaugeValueFromEQ_Trampoline);
}

// Called once, when the plugin is to shutdown
PLUGIN_API VOID ShutdownLabelsPlugin(VOID)
{
   // DebugSpewAlways("Shutting down MQ2Labels");

    // Remove commands, macro parameters, hooks, etc.
    RemoveDetour(CSidlManager__CreateLabel);
    RemoveDetour(CLabel__Draw);
    //RemoveDetour(CGaugeWnd__Draw);
    //RemoveDetour(__GetGaugeValueFromEQ);
}
