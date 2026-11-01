#include "actor/map_type_table.h"
#include "core/drive_state.h"
#include "contents/town_info.h"
#include "contents/champion_monster.h"
#include "contents/item_db_to_id.h"
#include "contents/item_reform.h"
#include "contents/enchant_mgr.h"
#include "core/map_sky.h"
#include "effects/effect_mgr.h"
#include "effects/ez2str_effect_mgr.h"
#include "effects/particle.h"
#include "contents/repute_api.h"
#include "contents/player_names.h"
#include "platform/language.h"
#include "security/gameguard.h"
#include "security/themida.h"
#include "security/gg_hash_mgr.h"
#include "contents/schedule.h"
#include "contents/whisper_block.h"
#include "web/work_manager.h"
#include "web/service_managers.h"
#include "web/http_post.h"
#include "resources/state_icon_sheet.h"
#include "ui/social/entry_queue_state.h"
#include "ui/quests/signboard_list.h"
#include "skills/skill_action_registry.h"
#include "platform/client_config.h"
#include "session/state.h"
#include "core/memory.h"
#include "core/fatal.h"
#include <windows.h>
#include <shellapi.h>
#include <string.h>
#include <io.h>
#include <stdio.h>
#include "std/std.h"
#include "core/app.h"
#include "contents/clan_info.h"
#include "contents/guild_mgr.h"
#include "contents/quest.h"
#include "core/text_util.h"
#include "session/session_options.h"
#include "ui/mail/rodex_box.h"
#include "ui/media/connect_sns.h"
#include "ui/misc/roulette_state.h"
#include "ui/net/packet_crypt.h"
#include "ui/social/party_board.h"

HINSTANCE g_hInstance;
int g_quitRequested;
char g_clientDir[0x88];
int g_fullscreenMode = 0;
int g_savedWidth, g_savedHeight;
int g_screenWidth = 1024;
int g_screenHeight = 768;

unsigned char g_windowDragging;
RECT g_windowDragRect;
POINT g_windowDragCursor;
HWND g_windowDragHwnd;

static void window_drag_follow(void)
{
    POINT cursor;
    int dx, dy;

    if (g_windowDragging != 1)
        return;
    GetCursorPos(&cursor);
    dy = cursor.y - g_windowDragCursor.y;
    dx = cursor.x - g_windowDragCursor.x;
    if (dx != 0 || dy != 0)
        MoveWindow(g_windowDragHwnd, g_windowDragRect.left + dx, g_windowDragRect.top + dy,
                   g_windowDragRect.right - g_windowDragRect.left, g_windowDragRect.bottom - g_windowDragRect.top,
                   TRUE);
    if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0)
        g_windowDragging = 0;
}

static void pump_messages(void)
{
    MSG msg;

    if (!PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        return;
    do {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_PROCESSKEY) {
            UINT key = ImmGetVirtualKey(msg.hwnd);

            language_on_reading_key(g_language, (BYTE)key);
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    } while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE));
}

void check_system_message(void)
{
    pump_messages();
    window_drag_follow();
    drive_state_tick();
}

void app_last_pump(void)
{
    pump_messages();
    window_drag_follow();
    DestroyWindow(g_hMainWnd);
}

int app_create_window(HINSTANCE instance)
{
    WNDCLASSA wc;
    RECT rect;
    int x, y;

    g_hInstance = instance;
    wc.style = 0;
    wc.lpfnWndProc = window_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hIcon = LoadIconA(instance, MAKEINTRESOURCEA(0x77));
    wc.hCursor = LoadCursorA(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = g_windowClassName;
    wc.lpszMenuName = 0;
    RegisterClassA(&wc);

    rect.right = g_screenWidth;
    rect.bottom = g_screenHeight;
    rect.left = 0;
    rect.top = 0;
    AdjustWindowRect(&rect, 0x2c20000, FALSE);

    x = 0;
    y = 0;
    if (g_fullscreenMode == 0) {
        GetSystemMetrics(SM_CXSCREEN);
        if (g_savedWidth == g_screenWidth && g_savedHeight == g_screenHeight) {
            x = session_get_option_int(&g_session1, "Window_XPos", (GetSystemMetrics(SM_CXSCREEN) - g_screenWidth) / 2);
            y = session_get_option_int(&g_session1, "Window_YPos", (GetSystemMetrics(SM_CYSCREEN) - g_screenHeight) / 2);
        } else {
            x = (GetSystemMetrics(SM_CXSCREEN) - g_screenWidth) / 2;
            y = (GetSystemMetrics(SM_CYSCREEN) - g_screenHeight) / 2;
        }
        if (x > GetSystemMetrics(SM_CXSCREEN) - g_screenWidth)
            x = (GetSystemMetrics(SM_CXSCREEN) - g_screenWidth) / 2;
        if (x < 0)
            x = 0;
        if (y > GetSystemMetrics(SM_CYSCREEN) - g_screenHeight)
            y = (GetSystemMetrics(SM_CYSCREEN) - g_screenHeight) / 2;
        if (y < 0)
            y = 0;
    }

    g_hMainWnd = CreateWindowExA(0, wc.lpszClassName, g_windowClassName, 0x2c20000, x, y,
                                   rect.right - rect.left, rect.bottom - rect.top, 0, 0, instance, 0);
    if (!g_hMainWnd) {
        MessageBoxA(0, "Window Creation Failed", "ERROR", 0);
        return 0;
    }
    ShowWindow(g_hMainWnd, SW_SHOW);
    UpdateWindow(g_hMainWnd);
    GetCurrentDirectoryA(0x80, g_clientDir);
    return 1;
}

unsigned char app_is_win9x(void)
{
    OSVERSIONINFOEXA info;

    memset(&info, 0, sizeof info);
    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    if (!GetVersionExA((OSVERSIONINFOA *)&info)) {
        info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        if (!GetVersionExA((OSVERSIONINFOA *)&info))
            return 0;
    }
    return info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS;
}

HANDLE app_run(const char *cmd)
{
    char line[0x400];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    memset(line, 0, sizeof line);
    if (g_session1.launchArgsSet)
        _snprintf_s(line, sizeof line, (size_t)-1, "%s %s %s %c %s", cmd, g_session1.ssoToken, g_session1.launchArg2,
                       (int)g_session1.launchArgFlag, g_session1.launchArg3);
    else
        _snprintf_s(line, sizeof line, (size_t)-1, "%s", cmd);
    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;
    memset(&pi, 0, sizeof pi);
    CreateProcessA(0, line, 0, 0, FALSE, 0, 0, 0, &si, &pi);
    return pi.hProcess;
}

void app_swap_ai_folder(void)
{
    char from[16];
    SHFILEOPSTRUCTA op;

    if (_access("AI_patch", 0) != 0)
        return;
    if (_access("AI", 0) != 0)
        return;
    if (_access("AI_backup", 0) == 0) {
        memset(from, 0, sizeof from);
        memcpy(from, "AI_backup", 10);
        op.hwnd = 0;
        op.wFunc = FO_DELETE;
        op.pFrom = from;
        op.pTo = 0;
        op.fFlags = 0x50;
        op.fAnyOperationsAborted = 0;
        op.hNameMappings = 0;
        op.lpszProgressTitle = 0;
        SHFileOperationA(&op);
    }
    rename("AI", "AI_backup");
    rename("AI_patch", "AI");
}

void app_finish_async_work(void)
{
    unsigned i;
    if (!g_asyncWorkMgr)
        return;
    for (i = 0; account_user_data_pending_saves(g_accountUserDataMgr) + adventurer_agency_pending_requests(g_adventurerAgency) != 0;) {
        Sleep(100);
        async_work_mgr_process_responses(1000);
        if (++i >= 30)
            break;
    }
    async_work_mgr_stop();
    async_work_mgr_process_responses(~0u);
    async_work_mgr_destroy();
}

void app_destroy_singletons(void)
{
    app_finish_async_work();

    skill_action_mgr_destroy_instance();

    ez2str_effect_mgr_destroy();

    map_sky_mgr_destroy();
    enchant_mgr_destroy();
    item_reform_mgr_destroy();
    item_db_to_id_destroy();
    repute_mgr_destroy();

    signboards_clear();
    connect_sns_mgr_destroy_instance();
    emblem_mgr_destroy();
    guild_mgr_destroy();
    player_names_destroy();
    whisper_block_destroy();
    account_user_data_destroy();
    char_user_data_destroy();
    twitter_destroy();
    http_client_stop();
    run_faq_web_page();
    set_hwnd_to_game_mon();
    map_property_mgr_release();
    champion_monster_destroy();
    clan_info_destroy();
    gg_hash_manager_shutdown();
    gameguard_shutdown();
    renew_quest_destroy();
    icon_sheet_destroy();
    town_info_destroy();
    time_scheduler_destroy();
    entry_queue_mgr_release();
    effect_memory_destroy();
    effect_mgr_destroy_instance();
    reassembly_packet_mgr_release();
    themida_shutdown();
    select_package_free();
    adventurer_agency_destroy();
    ui_request_join_party_mgr_delete();
}
