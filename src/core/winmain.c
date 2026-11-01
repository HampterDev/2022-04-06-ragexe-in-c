#include "platform/clientinfo.h"
#include "fileio/process.h"
#include "contents/map_info.h"
#include "core/msg_str.h"
#include "session/option_pairs.h"
#include "session/session_options.h"
#include "session/session_state.h"
#include "ui/window_manager_lifecycle.h"
#include "contents/frame_timer.h"
#include "render/device.h"
#include "core/archive_startup.h"
#include "security/gameguard.h"
#include "security/gg_hash_mgr.h"
#include "fileio/file_manager.h"
#include "render/math3d.h"
#include "render/texture_cache.h"
#include "resources/granny_res.h"
#include "platform/service.h"
#include "replay/replay.h"
#include "platform/bink_api.h"
#include "fileio/resource_api.h"
#include "contents/sound.h"
#include "render/display_devices.h"
#include "platform/client_config.h"
#include "ui/skin_manager.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "diag/crash_report.h"
#include "world/mode_manager.h"
#include "contents/connection_api.h"
#include "web/work_manager.h"
#include <windows.h>
#include <shellapi.h>
#include <objbase.h>
#include <mmsystem.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <io.h>
#include "std/std.h"
#include "render/render_api.h"
#include "core/app.h"
#include "core/client_init.h"

unsigned char g_openLauncherPageAtExit = 0;

static const char g_configFileName[16] = "clientinfo.xml";

int g_loginStartMode = 3;

unsigned char g_replayLaunch;

void winmain_on_crash(void)
{
}

static void read_launch_args(const char *cmd)
{
    const char *first, *end, *second, *third;
    int n, len;

    g_session1.launchArgsSet = 1;
    first = strstr(cmd, "-t:") + 3;
    end = strstr(first, " ");
    if (end == 0)
        end = first + strlen(first);
    n = (int)(end - first);
    strncpy(g_session1.ssoToken, first, n < 500 ? n : 500);
    g_session1.ssoToken[n < 499 ? n : 499] = 0;

    second = end + 1;
    end = strstr(second, " ");
    n = (int)(end - second);
    strncpy(g_session1.launchArg2, second, n < 0x18 ? n : 0x18);
    g_session1.launchArg2[n < 0x17 ? n : 0x17] = 0;
    g_session1.launchArgFlag = end[1];

    third = end + 3;
    len = (int)strlen(cmd);
    n = (int)(cmd + len - third);
    strncpy(g_session1.launchArg3, third, n < 0x18 ? n : 0x18);
    g_session1.launchArg3[n < 0x17 ? n : 0x17] = 0;
}

static void choose_login_start(void)
{
    int service = g_serviceType;
    int region;

    if (service == 4 || service == 8 || service == 9) {
        g_loginStartMode = 1;
        return;
    }
    if (service == 6) {
        region = g_serverType;
        if (region == 0) {
            g_loginStartMode = 1;
            return;
        }
    } else {
        if (service == 0xc) {
            g_loginStartMode = 1;
            return;
        }
        if (service == 0) {
            g_loginStartMode = 0;
            return;
        }
        region = g_serverType;
        if (region == 0 || service == 2) {
            g_loginStartMode = 2;
            return;
        }
    }
    if (region == 5 || service == 3 || service == 0x13)
        g_loginStartMode = 2;
    else
        g_loginStartMode = 0;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_unused, LPSTR cmd, int show)
{
    struct InitResult init;
    char config[0x40];
    const char *account;
    WIN32_FIND_DATAA found;
    WIN32_FIND_DATAA replay_found, navi_found;
    HANDLE find, replay_find, navi_find;
    TIMECAPS caps;
    HRESULT com_result;
    UINT period;
    int result;

    client_init();
    create_data_archive_if_missing();
    _BinkSetSoundSystem(_BinkOpenDirectSound, 0);
    if (!crash_report_install(winmain_on_crash)) {
        error_msg("minidump install fail", 0);
        return 0;
    }

    g_replayLaunch = strstr(cmd, "Replay") != 0;
    replay_recorder_set_mode(replay_recorder_instance(), 1);
    if (strstr(cmd, "1qm1") != 0)
        _snprintf_s(g_session1.launchArg3, 0x18, (size_t)-1, strstr(cmd, "1sak1") != 0 ? "QmSakray" : "Dev");

    if (strstr(cmd, "-t:") != 0 && g_replayLaunch == 0) {
        read_launch_args(cmd);
    } else {
        g_session1.launchArgsSet = 0;
        memset(g_session1.ssoToken, 0, 500);
        memset(g_session1.launchArg2, 0, 0x18);
    }

    strstr(cmd, "1rag1");

    g_session1.pcBang = strstr(cmd, "pc") != 0;
    game_file_manager_add_archive(&g_fileMgr, "data.grf");
    memcpy(config, g_configFileName, 15);
    account = strstr(cmd, "/account:");
    if (account != 0) {
        const char *src = account + 9;
        char *dst = config;

        while (*src != 0 && *src != '\t' && *src != ' ')
            *dst++ = *src++;
        *dst = 0;
        g_accountOnCmdLine = 1;
    }

    if (!gameguard_startup()) {
        error_msg("CNProtectGameGuardMgr::CreateInstance() Failed", 0);
        return 0;
    }
    if (strstr(cmd, "ApplyNPGE") != 0)
        return 1;
    if (!gg_hash_manager_startup()) {
        error_msg("GGHash Init Error", 0);
        return 0;
    }
    gg_hash_manager_is_intact(g_ggHashMgr);
    clientinfo_load(config);
    app_swap_ai_folder();

    session_init_at_startup(&g_session1);
    if (read_registry() == 0) {
        do {
            struct String setup;

            string_init(&setup);
            string_assign_cstr(&setup, "Setup.exe");
            if (_access(string_cstr(&setup), 0) == 0)
                WaitForSingleObject(app_run(string_cstr(&setup)), INFINITE);
            string_free(&setup);
        } while (read_registry() == 0);
    }

    if (render_enum_adapters() < 0)
        MessageBoxA(0, "d3denum init", "Error", 0);
    find = FindFirstFileA("savedata", &found);
    if (find == INVALID_HANDLE_VALUE)
        CreateDirectoryA("savedata", 0);
    else
        FindClose(find);
    _controlfp(0x20000, 0x30000);

    period = 0;
    if (timeGetDevCaps(&caps, sizeof caps) == 0) {
        UINT p = caps.wPeriodMin > 1 ? caps.wPeriodMin : 1;

        period = caps.wPeriodMax;
        if (p < period)
            period = p;
        timeBeginPeriod(period);
    }
    frame_timer_reset();
    com_result = CoInitialize(0);
    if (!sound_mgr_create()) {
        error_msg("Audio System Init Failed", 0);
        result = 0;
        goto com_done;
    }

    session_startup(&g_session1);
    if (g_serviceType == 0) {
        if (app_is_win9x())
            create_depends_dll_msg();
        else
            i_process_construct();
    }

    res_mgr_read_res_name_table(g_resMgr());
    if (app_create_window(instance) == 0) {
        result = 1;
        goto com_done;
    }
    app_init_managers(&init);
    if (!init.ok) {
        MessageBoxA(0, string_cstr(&init.what), "ERROR", 0);
        result = 1;
        goto done;
    }
    if (!app_init_3d()) {
        result = 1;
        goto done;
    }
    g_renderer = c3d_device_create_renderer(&g_3dDevice);
    if (!g_renderer) {
        result = 1;
        goto done;
    }
    if (g_replayLaunch == 0 && !connection_startup()) {
        result = 1;
        goto done;
    }

    frame_timer_start(0x3c);
    SetFocus(g_hMainWnd);
    if (g_3dDevice.fullscreen != 0)
        ShowCursor(FALSE);
    gameguard_set_window(g_gameGuard);
    ui_window_mgr_set_screen_size(&g_windowMgr, g_renderer->width,
               g_renderer->height);
    ui_window_mgr_init_ui_rect_info_from_reg(&g_windowMgr);
    granny_animation_cache_init(&g_grannyBoneRes);
    skin_mgr_scan(&g_skinMgr);
    math_init_degree_tables();

    choose_login_start();
    find = FindFirstFileA("memo", &found);
    if (find == INVALID_HANDLE_VALUE)
        CreateDirectoryA("memo", 0);
    else
        FindClose(find);
    g_loginStartMode = g_replayLaunch != 0 ? 0x1d : g_loginStartMode;
    replay_find = FindFirstFileA("Replay", &replay_found);
    if (replay_find == INVALID_HANDLE_VALUE)
        CreateDirectoryA("Replay", 0);
    else
        FindClose(replay_find);
    navi_find = FindFirstFileA("Navigationdata", &navi_found);
    if (navi_find == INVALID_HANDLE_VALUE)
        CreateDirectoryA("Navigationdata", 0);
    else
        FindClose(navi_find);

    mode_mgr_run(&g_modeMgr, 0, "login.rsw");

    app_finish_async_work();
    window_mgr_clear_backdrop(&g_windowMgr);
    connection_disconnect(rag_connection_transport(rag_connection_instance()));
    connection_shutdown();
    ui_window_mgr_close_all_windows(&g_windowMgr);
    ui_window_mgr_write_ui_rect_info_to_reg(&g_windowMgr);
    session_save_options(&g_session1);
    map_info_destroy();
    res_mgr_delete_all(g_resMgr());
    texcache_clear(&g_textureCache);
    c3d_device_delete_renderer(&g_3dDevice, g_renderer);
    c3d_device_release_all(&g_3dDevice);
    app_last_pump();
    delete_hack_files();
    app_destroy_singletons();
    sound_mgr_destroy();
    if (period != 0)
        timeEndPeriod(period);
    crash_report_uninstall();

    if (g_openLauncherPageAtExit != 0) {
        if (strstr(g_session1.launchArg3, "Dev") != 0) {
            const char *url = g_serverType == 0
                ? "http://admtools.gnjoy.com/gamestarter/devro.asp?GameCode=4011"
                : "http://admtools.gnjoy.com/gamestarter/devro.asp?GameCode=6011";

            ShellExecuteA(0, "OPEN", url, url, 0, SW_SHOWNORMAL);
        } else {
            const char *params = msg_str(0xd75);
            const char *file = msg_str(0xd75);

            ShellExecuteA(0, "OPEN", file, params, 0, SW_SHOWNORMAL);
        }
    }
    result = 0;
done:
    async_work_mgr_destroy();
    string_free(&init.what);
com_done:
    if (SUCCEEDED(com_result))
        CoUninitialize();
    return result;
}
