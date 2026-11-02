#include "contents/adventurer_agency.h"
#include "contents/character_settings.h"
#include "contents/account_settings.h"
#include "ui/window_keyboard.h"
#include "ui/window.h"
#include "platform/language.h"
#include "contents/frame_timer.h"
#include "render/device.h"
#include "ui/window_messages.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "platform/win_types.h"
#include "std/std.h"
#include "render/render_api.h"
#include "core/app.h"
#include "session/session_options.h"
#include "ui/controls/edit.h"

int g_activatedNow = 0;

int g_windowActive = 1;

static void language_windows(void)
{
    if (g_language->candidate_count != 0) {
        struct String names[9];
        struct CandidatePage info;
        int first, last, count, i;

        for (i = 0; i < 9; i++)
            string_init(&names[i]);
        first = g_language->candidate_selection / 9 * 9;
        last = first + 9;
        if (last >= g_language->candidate_count)
            last = g_language->candidate_count;
        info.selected = g_language->candidate_selection - first;
        info.page = (g_language->candidate_selection + 8) / 9;
        info.last_page = (g_language->candidate_count + 8) / 9;
        count = last - first;
        info.count = count;
        for (i = 0; i < count; i++) {
            const char *name = language_candidate_text(g_language, first + i);

            string_assign_cstr(&names[i], name);
        }
        {
            void *wnd = ui_window_open(0x33);

            ui_window_send(wnd, NULL, 0x41, (intptr_t)names, (intptr_t)&info, 0, 0);
        }
        for (i = 9; i-- > 0;)
            string_free(&names[i]);
    } else {
        ui_window_mgr_post_quit1(&g_windowMgr, 0x33);
    }
    if (language_reading_length(g_language) != 0) {
        void *wnd = ui_window_open(0x55);

        ui_window_refresh(wnd);
    } else {
        ui_window_mgr_post_quit1(&g_windowMgr, 0x55);
    }
}

static LRESULT on_activate(HWND hwnd, WPARAM wparam)
{
    WORD state = LOWORD(wparam);

    if (state == WA_INACTIVE) {
        if (g_mouse.ready != 0) {
            mouse_reset_buttons(&g_mouse);
            g_mouse.ready = 0;
        }
    } else if (state == WA_ACTIVE || state == WA_CLICKACTIVE) {
        if (g_mouse.ready != 1) {
            mouse_reset_buttons(&g_mouse);
            g_mouse.ready = 1;
        }
    }
    if (g_3dDevice.fullscreen != 1) {
        if (g_windowActive == 1)
            g_activatedNow = 1;
        return 1;
    }
    if (g_windowActive == 1) {
        g_activatedNow = 1;
        ShowWindow(hwnd, SW_MAXIMIZE);
        return 1;
    }
    ShowWindow(hwnd, SW_MINIMIZE);
    return 1;
}

static LRESULT on_close(void)
{
    g_quitRequested = 1;
    account_user_data_save(g_accountUserDataMgr);
    if (game_current_mode() != 0)
        char_user_data_save(g_charUserDataMgr);
    if (game_current_mode() != 0)
        adventurer_agency_cancel_own_booking(g_adventurerAgency);
    return 0;
}

static LRESULT on_set_cursor(LPARAM lparam)
{
    if (g_3dDevice.fullscreen != 0)
        return 0;
    if (g_session1.mouseExclusiveOption != 0 || LOWORD(lparam) == HTCLIENT) {
        SetCursor(0);
        return 0;
    }
    SetCursor(LoadCursorA(0, IDC_ARROW));
    mouse_set_x(&g_mouse, -100);
    mouse_set_y(&g_mouse, -100);
    return 0;
}

static LRESULT on_key_down(WPARAM wparam, LPARAM lparam)
{
    struct UIWindow *edit;

    if (ui_window_mgr_on_key_down(&g_windowMgr, wparam, lparam, !(lparam >> 30 & 1)))
        return 0;
    if (g_windowMgr.key_as_text == 0) {
        ui_window_mgr_set_text_at_battle_mode(&g_windowMgr);
        return 0;
    }
    if (ui_window_mgr_is_chat_edit_focused(&g_windowMgr) && g_serviceType != 0xf
        && ui_window_mgr_excute_msg_in_battle_mode(&g_windowMgr, lparam))
        return 0;
    if (ui_window_mgr_get_focus_edit(&g_windowMgr) == 0
        || (edit = ui_window_mgr_get_focus_edit(&g_windowMgr),
            ui_window_send_msg(edit, 0, 0x100, (int)wparam, (int)lparam, 0, 0) != 3))
        language_on_key_down(g_language, wparam);
    if (g_session1.battle_mode == 0)
        return 0;
    if (g_session1.battle_mode_suspended != 0)
        return 0;
    ui_window_mgr_set_text_at_battle_mode(&g_windowMgr);
    return 0;
}

static int sys_command_passes(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    unsigned int cmd = wparam & 0xfff0;

    switch (cmd) {
    case SC_MOVE:
        if (g_3dDevice.fullscreen != 0)
            return 0;
        g_windowDragHwnd = hwnd;
        GetCursorPos(&g_windowDragCursor);
        GetWindowRect(hwnd, &g_windowDragRect);
        g_windowDragging = 1;
        return 0;
    case SC_MINIMIZE:
    case 0xf090:
    case SC_RESTORE:
    case SC_MONITORPOWER:
        return g_3dDevice.fullscreen == 0;
    case SC_KEYMENU:
        return lparam == ' ';
    case SC_CLOSE:
        return 1;
    default:
        return 0;
    }
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    unsigned char handled = 0;
    LRESULT result;

    result = language_on_ime_message(g_language, hwnd, msg, wparam, lparam, &handled);
    if (handled) {
        language_windows();
        return result;
    }
    switch (msg) {
    case WM_CREATE:
    case WM_DESTROY:
        return 0;
    case WM_MOVE:
        if (g_windowActive == 1)
            return 0;
        if (g_3dDevice.fullscreen != 0)
            return 0;
        c3d_device_show_frame(&g_3dDevice, 1);
        return 0;
    case WM_ACTIVATE:
        return on_activate(hwnd, wparam);
    case WM_PAINT:
        ValidateRect(hwnd, 0);
        return 0;
    case WM_CLOSE:
        return on_close();
    case WM_ERASEBKGND:
        return 1;
    case WM_SETCURSOR:
        return on_set_cursor(lparam);
    case WM_NCACTIVATE:
        app_activate(LOWORD(wparam));
        if (wparam != 0)
            frame_timer_reset();
        break;
    case WM_KEYDOWN:
        return on_key_down(wparam, lparam);
    case WM_KEYUP:
        session_reset_option2(&g_session1);
        if (wparam != VK_SNAPSHOT)
            return 0;
        ui_window_mgr_on_key_down(&g_windowMgr, wparam, lparam, 1);
        return 0;
    case WM_CHAR:
        if (g_windowMgr.key_as_text == 0)
            return 0;
        language_on_char(g_language, wparam);
        return 0;
    case WM_SYSKEYDOWN:
        if (ui_window_mgr_on_key_down(&g_windowMgr, wparam, lparam, !(lparam >> 30 & 1)))
            return 0;
        break;
    case WM_SYSCOMMAND:
        if (!sys_command_passes(hwnd, wparam, lparam))
            return 0;
        break;
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MOUSEWHEEL:
        mouse_on_message(&g_mouse, hwnd, msg, wparam, lparam);
        break;
    case WM_ENTERMENULOOP:
    case WM_ENTERSIZEMOVE:
        app_activate(0);
        break;
    case WM_EXITMENULOOP:
    case WM_EXITSIZEMOVE:
        if (GetActiveWindow() != hwnd && IsIconic(hwnd))
            app_activate(0);
        else
            app_activate(1);
        break;
    case WM_IME_SETCONTEXT:
        wparam = 1;
        lparam = 0;
        break;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}
