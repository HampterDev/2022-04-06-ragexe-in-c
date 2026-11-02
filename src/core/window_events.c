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
#include "core/window_events.h"
#include "platform/vk.h"

int g_activatedNow = 0;

int g_windowActive = 1;

void window_events_language_changed(void)
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

void window_events_activate(int active)
{
    if (!active) {
        if (g_mouse.ready != 0) {
            mouse_reset_buttons(&g_mouse);
            g_mouse.ready = 0;
        }
    } else if (g_mouse.ready != 1) {
        mouse_reset_buttons(&g_mouse);
        g_mouse.ready = 1;
    }
}

void window_events_close(void)
{
    g_quitRequested = 1;
    account_user_data_save(g_accountUserDataMgr);
    if (game_current_mode() != 0)
        char_user_data_save(g_charUserDataMgr);
    if (game_current_mode() != 0)
        adventurer_agency_cancel_own_booking(g_adventurerAgency);
}

void window_events_key_down(int vk, int lparam)
{
    struct UIWindow *edit;

    if (ui_window_mgr_on_key_down(&g_windowMgr, vk, lparam, !(lparam >> 30 & 1)))
        return;
    if (g_windowMgr.key_as_text == 0) {
        ui_window_mgr_set_text_at_battle_mode(&g_windowMgr);
        return;
    }
    if (ui_window_mgr_is_chat_edit_focused(&g_windowMgr) && g_serviceType != 0xf
        && ui_window_mgr_excute_msg_in_battle_mode(&g_windowMgr, lparam))
        return;
    if (ui_window_mgr_get_focus_edit(&g_windowMgr) == 0
        || (edit = ui_window_mgr_get_focus_edit(&g_windowMgr),
            ui_window_send_msg(edit, 0, 0x100, vk, lparam, 0, 0) != 3))
        language_on_key_down(g_language, vk);
    if (g_session1.battle_mode == 0)
        return;
    if (g_session1.battle_mode_suspended != 0)
        return;
    ui_window_mgr_set_text_at_battle_mode(&g_windowMgr);
    return;
}

void window_events_key_up(int vk, int lparam)
{
    session_reset_option2(&g_session1);
    if (vk != VK_SNAPSHOT)
        return;
    ui_window_mgr_on_key_down(&g_windowMgr, vk, lparam, 1);
}

void window_events_char(int ch)
{
    if (g_windowMgr.key_as_text == 0)
        return;
    language_on_char(g_language, ch);
}
