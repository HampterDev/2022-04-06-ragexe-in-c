#include "ui/window_manager_state.h"
#include "login/merc_ai.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct String g_traceAiLast;
INIT_ONCE g_traceAiLastGuard;

int merc_ai_trace(lua_State *L)
{
    luaL_checklstring(L, 1, NULL);
    return 0;
}

int merc_ai_trace_value(lua_State *L)
{
    struct String s;
    const char *text = luaL_checklstring(L, 1, NULL);

    string_init(&s);
    string_assign_cstr(&s, text);
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)string_cstr(&s), 0xffff, 4, 0);
    string_free(&s);
    return 1;
}

int merc_ai_trace_ai(lua_State *L)
{
    struct String line;
    struct File f;
    SYSTEMTIME st;
    char dir[0x100];
    char stamp[0x100];
    const char *text, *p;
    BYTE *old;
    DWORD size, date, time;

    if (g_session1.homun_ai_flags[1] == 0)
        return 0;
    text = luaL_checklstring(L, 1, NULL);
    if (!text)
        return 0;
    if (once_begin(&g_traceAiLastGuard)) {
        string_init(&g_traceAiLast);
        atexit(merc_ai_trace_last_atexit);
        once_complete(&g_traceAiLastGuard);
    }
    if (g_traceAiLast.size == strlen(text) && memcmp(string_cstr(&g_traceAiLast), text, g_traceAiLast.size) == 0)
        return 0;
    string_assign_cstr(&g_traceAiLast, text);

    string_init(&line);
    string_assign_cstr(&line, text);
    string_append_cstr(&line, "\n");
    OutputDebugStringA(string_cstr(&line));
    GetLocalTime(&st);
    date = ((((DWORD)st.wYear << 8) | (short)st.wMonth) << 8) | (short)st.wDay;
    time = ((((DWORD)st.wHour << 8) | (short)st.wMinute) << 8) | (short)st.wSecond;

    file_construct(&f);
    GetCurrentDirectoryA(0x100, dir);
    p = strstr(dir, "\\data");
    size = 0;
    if (p)
        memcpy(dir, dir, p - dir);
    strcat(dir, "\\TraceAI.txt");
    if (file_open(&f, dir, 0, 1, 0)) {
        size = f.size;
        old = mem_alloc(size);
        file_read(&f, old, size);
        file_close(&f);
    } else {
        old = (BYTE *)text;
    }
    file_open(&f, dir, 0x1001, 1, 0);
    if (size)
        file_write(&f, old, size);
    sprintf(stamp, "%d/%d/%d - %d:%d:%d", date >> 16, (BYTE)(date >> 8), (BYTE)date,
                (BYTE)(time >> 16), (BYTE)(time >> 8), (BYTE)time);
    file_write(&f, stamp, strlen(stamp));
    file_write(&f, text, strlen(text));
    file_write(&f, "\r\n", 2);
    file_close(&f);
    if (size)
        mem_free(old);
    file_destruct(&f);
    string_free(&line);
    return 0;
}

void merc_ai_trace_last_atexit(void)
{
    string_free(&g_traceAiLast);
}
