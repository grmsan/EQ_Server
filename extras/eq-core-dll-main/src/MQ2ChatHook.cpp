/*****************************************************************************
MQ2Main.dll: MacroQuest2's extension DLL for EverQuest
Copyright (C) 2002-2003 Plazmic, 2003-2005 Lax

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2, as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
******************************************************************************/

#if !defined(CINTERFACE)
//#error /DCINTERFACE
#endif

#define DBG_SPEW


#include "MQ2Main.h"

static bool RewriteWhoLine(const char *in, char *out, size_t out_size)
{
    if (!in || !out || out_size < 16) {
        return false;
    }

    // Expect a normal /who prefix: "[<level> <class>] ..."
    if (in[0] != '[' || !isdigit((unsigned char)in[1])) {
        return false;
    }

    // Parse level from the first bracket.
    const char *p = in + 1;
    unsigned long level = 0;
    while (*p && isdigit((unsigned char)*p)) {
        level = (level * 10) + (unsigned long)(*p - '0');
        ++p;
    }
    if (*p != ' ') {
        return false;
    }

    const char *close1 = strchr(in, ']');
    if (!close1 || close1 <= in) {
        return false;
    }

    // Look for a multiclass suffix the server can append to the name: " [RNG/MNK/MAG]".
    const char *suffix_space = nullptr;
    const char *suffix_open = nullptr;
    const char *suffix_close = nullptr;
    for (const char *s = close1 + 1; (s = strstr(s, " [")) != nullptr; ++s) {
        const char *open = s + 1; // points at '['
        const char *close = strchr(open, ']');
        if (!close) {
            break;
        }

        bool has_slash = false;
        bool valid = true;
        for (const char *c = open + 1; c < close; ++c) {
            if (*c == '/') {
                has_slash = true;
                continue;
            }
            if (*c >= 'A' && *c <= 'Z') {
                continue;
            }
            valid = false;
            break;
        }

        if (valid && has_slash) {
            suffix_space = s;
            suffix_open = open;
            suffix_close = close;
            break;
        }

        s = close;
    }

    if (!suffix_space || !suffix_open || !suffix_close) {
        return false;
    }

    char classes[64] = {0};
    const size_t classes_len = (size_t)(suffix_close - (suffix_open + 1));
    if (classes_len == 0 || classes_len >= sizeof(classes)) {
        return false;
    }
    memcpy(classes, suffix_open + 1, classes_len);
    classes[classes_len] = 0;

    int n = _snprintf_s(out, out_size, _TRUNCATE, "[%lu %s]", level, classes);
    if (n <= 0) {
        return false;
    }
    size_t off = (size_t)n;

    // Keep the original portion from after the first bracket up to (but not including) the multiclass suffix.
    // This preserves the character name and the rest of the /who line.
    const char *mid_start = close1 + 1;
    const char *mid_end = suffix_space;
    if (mid_end < mid_start) {
        return false;
    }
    const size_t mid_len = (size_t)(mid_end - mid_start);
    if (off + mid_len >= out_size) {
        return false;
    }
    memcpy(out + off, mid_start, mid_len);
    off += mid_len;

    // Append the remainder after the suffix closing bracket.
    const char *rest = suffix_close + 1;
    const size_t rest_len = strlen(rest);
    if (off + rest_len >= out_size) {
        return false;
    }
    memcpy(out + off, rest, rest_len + 1);
    return true;
}

class CChatHook
{
public:
    VOID Trampoline(PCHAR szMsg, DWORD dwColor, bool EqLog, bool dopercentsubst);
    VOID Detour(PCHAR szMsg, DWORD dwColor, bool EqLog, bool dopercentsubst)
    { 
        //DebugSpew("CChatHook::Detour(%s)",szMsg);
        gbInChat = TRUE;

        char rewritten[MAX_STRING] = {0};
        if (RewriteWhoLine(szMsg, rewritten, sizeof(rewritten))) {
            szMsg = rewritten;
        }

        //CheckChatForEvent(szMsg);

        BOOL Filtered=FALSE;
        PFILTER Filter = gpFilters;
        while (Filter && !Filtered) { 
            if (!Filter->pEnabled || (*Filter->pEnabled)) { 
                if (*Filter->FilterText == '*') {
                    if (strstr(szMsg,Filter->FilterText+1)) 
                        Filtered = TRUE;
                } else { 
                    if (!strnicmp(szMsg,Filter->FilterText,Filter->Length)) 
                        Filtered = TRUE; 
                }
            } 
            Filter = Filter->pNext; 
        } 

        if (!Filtered) { 
            //if (gTelnetServer && gTelnetConnection && !gPauseTelnetOutput) TelnetServer_Write(szMsg); 
            BOOL SkipTrampoline;
			//OnDPSIncomingChat(szMsg, dwColor);
            //Benchmark(bmPluginsIncomingChat,SkipTrampoline=PluginsIncomingChat(szMsg,dwColor));
			Trampoline(szMsg, dwColor, EqLog, dopercentsubst); 
        } 
        gbInChat = FALSE; 
    } 

    VOID TellWnd_Trampoline(char *message,char *name,char *name2,void *unknown,int color,bool b);
    VOID TellWnd_Detour(char *message,char *name,char *name2,void *unknown,int color,bool b)
    {
        char szMsg[MAX_STRING];
        BOOL SkipTrampoline;
        gbInChat=true;

        sprintf(szMsg,"%s tells you, '%s'",name,message);

       // Benchmark(bmPluginsIncomingChat,SkipTrampoline=PluginsIncomingChat(szMsg,color));
       TellWnd_Trampoline(message,name,name2,unknown,color,b);

        gbInChat=false;
    }

    VOID UPCNotificationFlush_Trampoline();
    VOID UPCNotificationFlush_Detour()
    {
        PEVERQUEST eq = (PEVERQUEST)this;
        char szBuf[MAX_STRING] = {0};

        if(eq->ChannelQty > 0)
        {
            int len = 0;
            char *pTmp;

            if(eq->bJoinedChannel)
            {
                pTmp = "* %s has entered channel ";
            }
            else
            {
                pTmp = "* %s has left channel ";
            }

            sprintf(szBuf, pTmp, eq->ChannelPlayerName);

            for(DWORD i = 0; i < eq->ChannelQty; i++)
            {
                if(i)
                {
                    pTmp = ", %s:%d";
                }
                else
                {
                    pTmp = "%s:%d";
                }

                len = strlen(szBuf);
                sprintf(&szBuf[len], pTmp, eq->ChannelName[i], eq->ChannelNumber[i] + 1);
            }
        }

        UPCNotificationFlush_Trampoline();
    }
}; 

DETOUR_TRAMPOLINE_EMPTY(VOID CChatHook::Trampoline(PCHAR szMsg, DWORD dwColor, bool EqLog, bool dopercentsubst)); 
DETOUR_TRAMPOLINE_EMPTY(VOID CChatHook::TellWnd_Trampoline(char *message,char *name,char *name2,void *unknown,int color,bool b)); 
DETOUR_TRAMPOLINE_EMPTY(VOID CChatHook::UPCNotificationFlush_Trampoline());

VOID dsp_chat_no_events(const char *Text,int Color,bool EqLog, bool dopercentsubst)
{
    ((CChatHook*)pEverQuest)->Trampoline((PCHAR)Text,Color,EqLog, dopercentsubst);
}

unsigned int __stdcall MQ2DataVariableLookup(char * VarName, char * Value)
{
    strcpy(Value,VarName);
    if (!GetCharInfo()) return strlen(Value);
    return strlen(ParseMacroParameter(GetCharInfo()->pSpawn,Value));
}

VOID InitializeChatHook()
{
    DebugSpew("Initializing chat hook");

    // initialize Blech
    EzDetour(CEverQuest__dsp_chat,&CChatHook::Detour,&CChatHook::Trampoline);
    EzDetour(CEverQuest__DoTellWindow,&CChatHook::TellWnd_Detour,&CChatHook::TellWnd_Trampoline);
    EzDetour(CEverQuest__UPCNotificationFlush,&CChatHook::UPCNotificationFlush_Detour,&CChatHook::UPCNotificationFlush_Trampoline);
}

VOID ShutdownChatHook()
{
    RemoveDetour(CEverQuest__dsp_chat);
    RemoveDetour(CEverQuest__DoTellWindow);
    RemoveDetour(CEverQuest__UPCNotificationFlush);
}
