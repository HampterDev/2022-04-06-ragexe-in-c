#include "contents/adventurer_agency.h"
#include "contents/character_settings.h"
#include "contents/account_settings.h"
#include "actor/map_type_table.h"
#include "core/drive_state.h"
#include "contents/town_info.h"
#include "contents/champion_monster.h"
#include "core/map_sky.h"
#include "effects/effect_mgr.h"
#include "effects/ez2str_effect_mgr.h"
#include "effects/particle.h"
#include "contents/player_names.h"
#include "platform/language.h"
#include "contents/schedule.h"
#include "contents/whisper_block.h"
#include "resources/state_icon_sheet.h"
#include "skills/skill_action_registry.h"
#include "platform/client_config.h"
#include "session/state.h"
#include "core/memory.h"
#include "core/fatal.h"
#include <windows.h>
#include <string.h>
#include "std/std.h"
#include "core/app.h"
#include "contents/guild_mgr.h"
#include "contents/quest.h"
#include "core/text_util.h"
#include "session/session_options.h"
#include "ui/mail/rodex_box.h"
#include "ui/social/party_board.h"
#include "platform/win_utf8.h"

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

    if (!PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        return;
    do {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    } while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE));
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
    WNDCLASSW wc;
    WCHAR class_name[64];
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
    /* A Unicode window: WM_CHAR delivers UTF-16, whatever the system code page. */
    if (!MultiByteToWideChar(CP_UTF8, 0, g_windowClassName, -1, class_name, 64))
        class_name[0] = 0;
    wc.lpszClassName = class_name;
    wc.lpszMenuName = 0;
    RegisterClassW(&wc);

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

    g_hMainWnd = CreateWindowExW(0, class_name, class_name, 0x2c20000, x, y,
                                   rect.right - rect.left, rect.bottom - rect.top, 0, 0, instance, 0);
    if (!g_hMainWnd) {
        win_utf8_message_box(0, "Window Creation Failed", "ERROR", 0);
        return 0;
    }
    ShowWindow(g_hMainWnd, SW_SHOW);
    UpdateWindow(g_hMainWnd);
    GetCurrentDirectoryA(0x80, g_clientDir);
    return 1;
}

void app_destroy_singletons(void)
{
    skill_action_mgr_destroy_instance();

    ez2str_effect_mgr_destroy();

    map_sky_mgr_destroy();

    guild_mgr_destroy();
    player_names_destroy();
    whisper_block_destroy();
    account_user_data_destroy();
    char_user_data_destroy();
    set_hwnd_to_game_mon();
    map_property_mgr_release();
    champion_monster_destroy();

    renew_quest_destroy();
    icon_sheet_destroy();
    town_info_destroy();
    time_scheduler_destroy();
    effect_memory_destroy();
    effect_mgr_destroy_instance();
    adventurer_agency_destroy();
    ui_request_join_party_mgr_delete();
}
