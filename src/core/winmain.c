#include "platform/clientinfo.h"
#include "contents/map_info_api.h"
#include "session/session_options.h"
#include "session/session_state.h"
#include "ui/window_manager_lifecycle.h"
#include "contents/frame_timer.h"
#include "render/device.h"
#include "fileio/file_manager.h"
#include "render/math3d.h"
#include "render/texture_cache.h"
#include "resources/granny_res.h"
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
#include "platform/win_types.h"
#include "platform/filesystem.h"
#include <objbase.h>
#include <mmsystem.h>
#include <float.h>
#include <string.h>
#include "std/std.h"
#include "render/render_api.h"
#include "core/app.h"
#include "core/client_init.h"

int g_loginStartMode = 2;

void winmain_on_crash(void)
{
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_unused, LPSTR cmd_unused, int show)
{
    struct InitResult init;
    struct PlatformFind *find;
    TIMECAPS caps;
    HRESULT com_result;
    UINT period;
    int result;

    client_init();
    if (!crash_report_install(winmain_on_crash)) {
        error_msg("minidump install fail", 0);
        return 0;
    }

    memset(g_session1.ssoToken, 0, sizeof g_session1.ssoToken);
    game_file_manager_add_archive(&g_fileMgr, "data.grf");

    clientinfo_load("clientinfo.xml");

    session_init_at_startup(&g_session1);
    read_startup_settings();

    if (render_enum_adapters() < 0)
        MessageBoxA(0, "d3denum init", "Error", 0);
    find = platform_find_first("savedata");
    if (!find)
        platform_directory_create("savedata");
    else
        platform_find_close(find);
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
    if (!connection_startup()) {
        result = 1;
        goto done;
    }

    frame_timer_start(0x3c);
    SetFocus(g_hMainWnd);
    if (g_3dDevice.fullscreen != 0)
        ShowCursor(FALSE);

    ui_window_mgr_set_screen_size(&g_windowMgr, g_renderer->width,
               g_renderer->height);
    ui_window_mgr_init_ui_rect_info_from_reg(&g_windowMgr);
    granny_animation_cache_init(&g_grannyBoneRes);
    skin_mgr_scan(&g_skinMgr);
    math_init_degree_tables();

    mode_mgr_run(&g_modeMgr, 0, "login.rsw");

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
    app_destroy_singletons();
    sound_mgr_destroy();
    if (period != 0)
        timeEndPeriod(period);
    crash_report_uninstall();

    result = 0;
done:
    string_free(&init.what);
com_done:
    if (SUCCEEDED(com_result))
        CoUninitialize();
    return result;
}
